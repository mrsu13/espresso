#!/usr/bin/env python3
"""Преобразует шрифт TrueType или шрифт консоли Linux (PSF) в растровый
шрифт для esp::gfx::font.

Из шрифта берутся все знаки базовой многоязычной плоскости Unicode, кроме
управляющих, форматирующих и комбинируемых знаков и области для частного
использования. Каждый знак записывается изображением 1 бит на точку:
строка точек — целое число байтов, старший бит — левая точка.

Шрифт TrueType (--font) растеризуется в знакоместо заданного размера:
сглаживание, затем порог яркости. Знаки псевдографики U+2500–U+259F
(линии рамок и блоки) из него не берутся, а рисуются программно по размеру
знакоместа, чтобы линии соседних знакомест стыковались.

Шрифт консоли (--psf, форматы PSF1 и PSF2, можно сжатый gzip) уже
растровый: знаки берутся как есть, размер знакоместа — из файла, коды
Unicode — из таблицы шрифта. Знак, у которого несколько кодов, повторяется
для каждого из них. Программно рисуются только знаки U+2500–U+259F,
которых в шрифте нет.

Результат — файлы <имя>.h и <имя>.cpp: таблица кодов знаков по
возрастанию (для двоичного поиска) и изображения знаков в том же порядке.

Параметр --bold делает начертание полужирным: каждая точка знака шрифта
повторяется правее на одну.

Нужны пакеты fontTools и Pillow. Так получен шрифт ter_10x18 проекта:

    python3 ttf2font.py --psf /usr/share/consolefonts/Uni2-Terminus18x10.psf.gz --name ter_10x18

Шрифт TrueType преобразуется так (пример — шрифт IBM 3270, который
пробовали раньше):

    python3 ttf2font.py --font /usr/share/fonts/truetype/3270/3270-Regular.ttf \\
        --width 9 --height 18 --name font3270_9x18
"""

import argparse
import gzip
import pathlib
import struct
import sys
import unicodedata

from fontTools.ttLib import TTFont
from PIL import Image, ImageDraw, ImageFont


# Толщина линий псевдографики.
LIGHT, HEAVY, DOUBLE = 1, 2, 3

# Слова имён знаков Unicode, задающие толщину линии и направления.
WEIGHTS = {'LIGHT': LIGHT, 'SINGLE': LIGHT, 'HEAVY': HEAVY, 'DOUBLE': DOUBLE}
DIRECTIONS = {
    'UP': ('up',),
    'DOWN': ('down',),
    'LEFT': ('left',),
    'RIGHT': ('right',),
    'VERTICAL': ('up', 'down'),
    'HORIZONTAL': ('left', 'right'),
}

# Псевдографика, которую сценарий рисует сам.
BOX_FIRST, BOX_LAST = 0x2500, 0x257F
BLOCK_FIRST, BLOCK_LAST = 0x2580, 0x259F

# Категории Unicode, знаки которых не попадают в шрифт: управляющие,
# форматирующие, суррогаты, частного использования, неназначенные,
# комбинируемые (их не нарисовать в отдельном знакоместе) и разделители
# строк и абзацев.
SKIPPED_CATEGORIES = {'Cc', 'Cf', 'Cs', 'Co', 'Cn', 'Mn', 'Me', 'Zl', 'Zp'}


class Cell:
    """Знакоместо: матрица точек ширины width и высоты height."""

    def __init__(self, width, height):
        self.width = width
        self.height = height
        self.pixels = [[False] * width for _ in range(height)]

    def set(self, x, y):
        """Зажигает точку; точки за пределами знакоместа отбрасываются."""
        if 0 <= x < self.width and 0 <= y < self.height:
            self.pixels[y][x] = True

    def hline(self, y, x0, x1):
        """Горизонтальный отрезок от x0 до x1 включительно."""
        for x in range(x0, x1 + 1):
            self.set(x, y)

    def vline(self, x, y0, y1):
        """Вертикальный отрезок от y0 до y1 включительно."""
        for y in range(y0, y1 + 1):
            self.set(x, y)

    def fill(self, x0, y0, x1, y1):
        """Закрашенный прямоугольник, углы включительно."""
        for y in range(y0, y1 + 1):
            self.hline(y, x0, x1)

    def to_bytes(self):
        """Изображение в формате esp::gfx: строки по целому числу байтов,
        старший бит — левая точка."""
        data = []
        for row in self.pixels:
            for start in range(0, self.width, 8):
                byte = 0
                for bit, on in enumerate(row[start:start + 8]):
                    if on:
                        byte |= 0x80 >> bit
                data.append(byte)
        return data


def parse_lines(text):
    """Разбирает имя вида «UP HEAVY AND RIGHT DOWN LIGHT».

    Возвращает словарь «направление → толщина». Толщина стоит до или после
    направлений своей части имени; если в части её нет, действует толщина
    предыдущей части («LIGHT DOWN AND RIGHT»).
    """
    lines = {}
    weight = None
    for part in text.split(' AND '):
        words = part.split()
        for word in words:
            if word in WEIGHTS:
                weight = WEIGHTS[word]
        for word in words:
            for direction in DIRECTIONS.get(word, ()):
                lines[direction] = weight
    if not lines or None in lines.values():
        raise ValueError(f'cannot parse box drawing name: {text}')
    return lines


def draw_lines(cell, lines):
    """Рисует линии рамок: тонкие, жирные и двойные, в том числе смешанные.

    Тонкая линия проходит через центральную точку знакоместа, жирная
    занимает ещё одну точку вправо или вниз, двойная — две тонкие линии на
    расстоянии одной точки от центральной по обе стороны. У двойных линий
    стыки строятся так, чтобы внутренние и внешние линии углов, развилок и
    пересечений сходились, как в шрифтах VGA.
    """
    w, h = cell.width, cell.height
    cx, cy = (w - 1) // 2, (h - 1) // 2
    get = lines.get

    def vertical_end(direction, side):
        """Конец вертикальной линии у центра: последняя строка для «up»,
        первая — для «down».

        side — сторона штриха двойной линии («left» или «right»); для
        одинарной линии None.
        """
        sign = -1 if direction == 'up' else 1
        opposite = 'down' if direction == 'up' else 'up'
        if side is None:
            doubles = [get(s) == DOUBLE for s in ('left', 'right')]
            if not any(doubles) or get(opposite):
                return cy                   # до центра или насквозь
            if all(doubles):
                return cy + sign            # до ближней линии двойной пары
            return cy - sign                # до дальней линии двойной пары
        other = 'right' if side == 'left' else 'left'
        if get(side) == DOUBLE:
            return cy + sign                # внутренний угол
        if get(opposite) == DOUBLE:
            return cy                       # двойная линия идёт насквозь
        if get(other) == DOUBLE:
            return cy - sign                # внешний угол
        return cy                           # до тонкой поперечной линии

    def horizontal_end(direction, side):
        """Конец горизонтальной линии у центра: последний столбец для
        «left», первый — для «right»; side — «up» или «down» либо None."""
        sign = -1 if direction == 'left' else 1
        opposite = 'right' if direction == 'left' else 'left'
        if side is None:
            doubles = [get(s) == DOUBLE for s in ('up', 'down')]
            if not any(doubles) or get(opposite):
                return cx
            if all(doubles):
                return cx + sign
            return cx - sign
        other = 'down' if side == 'up' else 'up'
        if get(side) == DOUBLE:
            return cx + sign
        if get(opposite) == DOUBLE:
            return cx
        if get(other) == DOUBLE:
            return cx - sign
        return cx

    for direction in ('up', 'down'):
        weight = get(direction)
        if not weight:
            continue
        if weight == DOUBLE:
            strokes = [(cx - 1, 'left'), (cx + 1, 'right')]
        else:
            strokes = [(cx + i, None) for i in range(2 if weight == HEAVY else 1)]
        for x, side in strokes:
            end = vertical_end(direction, side)
            if direction == 'up':
                cell.vline(x, 0, end)
            else:
                cell.vline(x, end, h - 1)

    for direction in ('left', 'right'):
        weight = get(direction)
        if not weight:
            continue
        if weight == DOUBLE:
            strokes = [(cy - 1, 'up'), (cy + 1, 'down')]
        else:
            strokes = [(cy + i, None) for i in range(2 if weight == HEAVY else 1)]
        for y, side in strokes:
            end = horizontal_end(direction, side)
            if direction == 'left':
                cell.hline(y, 0, end)
            else:
                cell.hline(y, end, w - 1)


def draw_dashes(cell, weight, count, horizontal):
    """Штриховая линия из count штрихов через всё знакоместо."""
    w, h = cell.width, cell.height
    cx, cy = (w - 1) // 2, (h - 1) // 2
    length = w if horizontal else h
    gap = 1 if length < 12 else 2
    for i in range(count):
        start = round(i * length / count)
        stop = round((i + 1) * length / count) - gap
        for p in range(start, stop):
            for t in range(2 if weight == HEAVY else 1):
                if horizontal:
                    cell.set(p, cy + t)
                else:
                    cell.set(cx + t, p)


def draw_arc(cell, lines):
    """Скруглённый угол: две тонкие линии без угловой точки, соединённые
    по диагонали."""
    w, h = cell.width, cell.height
    cx, cy = (w - 1) // 2, (h - 1) // 2
    if 'up' in lines:
        cell.vline(cx, 0, cy - 1)
    if 'down' in lines:
        cell.vline(cx, cy + 1, h - 1)
    if 'left' in lines:
        cell.hline(cy, 0, cx - 1)
    if 'right' in lines:
        cell.hline(cy, cx + 1, w - 1)


def draw_diagonals(cell, rising, falling):
    """Диагонали из угла в угол: по точке в каждой строке."""
    w, h = cell.width, cell.height
    for y in range(h):
        t = y / (h - 1)
        if rising:
            cell.set(round((w - 1) * (1 - t)), y)
        if falling:
            cell.set(round((w - 1) * t), y)


def draw_box(cell, codepoint):
    """Знак рамки U+2500–U+257F по его имени в Unicode."""
    name = unicodedata.name(chr(codepoint)).removeprefix('BOX DRAWINGS ')
    words = name.split()
    if 'DASH' in words:
        # «LIGHT TRIPLE DASH HORIZONTAL»
        count = {'DOUBLE': 2, 'TRIPLE': 3, 'QUADRUPLE': 4}[words[1]]
        draw_dashes(cell, WEIGHTS[words[0]], count, words[-1] == 'HORIZONTAL')
    elif 'ARC' in words:
        draw_arc(cell, parse_lines(name.replace('ARC ', '')))
    elif 'DIAGONAL' in words:
        cross = 'CROSS' in words
        draw_diagonals(cell, cross or 'UPPER RIGHT' in name, cross or 'UPPER LEFT' in name)
    else:
        draw_lines(cell, parse_lines(name))


def draw_block(cell, codepoint):
    """Знак блока U+2580–U+259F: части знакоместа и штриховки."""
    w, h = cell.width, cell.height
    half_w, half_h = w // 2, h // 2
    if codepoint == 0x2580:                          # ▀ верхняя половина
        cell.fill(0, 0, w - 1, half_h - 1)
    elif 0x2581 <= codepoint <= 0x2588:              # ▁…█ нижние восьмые
        rows = round((codepoint - 0x2580) * h / 8)
        cell.fill(0, h - rows, w - 1, h - 1)
    elif 0x2589 <= codepoint <= 0x258F:              # ▉…▏ левые восьмые
        columns = round((0x2590 - codepoint) * w / 8)
        cell.fill(0, 0, columns - 1, h - 1)
    elif codepoint == 0x2590:                        # ▐ правая половина
        cell.fill(half_w, 0, w - 1, h - 1)
    elif codepoint in (0x2591, 0x2592, 0x2593):      # ░ ▒ ▓ штриховки
        for y in range(h):
            for x in range(w):
                if codepoint == 0x2591:
                    on = (x + 2 * y) % 4 == 0
                elif codepoint == 0x2592:
                    on = (x + y) % 2 == 0
                else:
                    on = (x + 2 * y) % 4 != 0
                if on:
                    cell.set(x, y)
    elif codepoint == 0x2594:                        # ▔ верхняя восьмая
        cell.fill(0, 0, w - 1, max(1, round(h / 8)) - 1)
    elif codepoint == 0x2595:                        # ▕ правая восьмая
        cell.fill(w - max(1, round(w / 8)), 0, w - 1, h - 1)
    else:                                            # ▖…▟ четверти
        quadrants = {'UPPER LEFT': (0, 0), 'UPPER RIGHT': (1, 0),
                     'LOWER LEFT': (0, 1), 'LOWER RIGHT': (1, 1)}
        name = unicodedata.name(chr(codepoint)).removeprefix('QUADRANT ')
        for part in name.split(' AND '):
            qx, qy = quadrants[part]
            cell.fill(qx * half_w, qy * half_h,
                      half_w - 1 if qx == 0 else w - 1,
                      half_h - 1 if qy == 0 else h - 1)


def embolden(cell):
    """Полужирное начертание: точки знака повторяются правее на одну.

    Вертикальные штрихи становятся шириной в две точки, как в шрифтах VGA;
    такой текст лучше переносит растяжение изображения 640 × 480 на экране
    монитора с другим разрешением. Промежуток в одну точку не заполняется,
    иначе многоточие сливается в черту, а близкие штрихи — в пятно.
    """
    for y, row in enumerate(cell.pixels):
        old = row[:]
        for x in range(1, cell.width):
            right = old[x + 1] if x + 1 < cell.width else False
            if old[x - 1] and not right:
                row[x] = True


def selected(codepoint):
    """Попадает ли знак с кодом codepoint в растровый шрифт."""
    if codepoint > 0xFFFF or codepoint in (0xFFFE, 0xFFFF):
        return False
    return unicodedata.category(chr(codepoint)) not in SKIPPED_CATEGORIES


def font_codepoints(tt):
    """Коды знаков шрифта TrueType, которые растеризуются (псевдографика
    рисуется программно)."""
    return {c for c in tt.getBestCmap() if selected(c) and not BOX_FIRST <= c <= BLOCK_LAST}


def render_ttf(args):
    """Растеризует шрифт TrueType; возвращает параметры и список (код, знакоместо)."""
    tt = TTFont(args.font)
    cmap = tt.getBestCmap()
    upem = tt['head'].unitsPerEm
    ascent, descent = tt['hhea'].ascent, -tt['hhea'].descent

    # Шрифт моноширинный: кегль подбирается так, чтобы ширина знака
    # равнялась ширине знакоместа, а строка шрифта (выносной элемент вверх
    # и вниз) ставится посередине знакоместа по высоте.
    advance = tt['hmtx'][cmap[ord('0')]][0]
    size = args.width * upem / advance
    line = size * (ascent + descent) / upem
    baseline = round((args.height - line) / 2 + size * ascent / upem)
    font = ImageFont.truetype(args.font, size)

    glyphs = []
    for codepoint in sorted(font_codepoints(tt) | set(range(BOX_FIRST, BLOCK_LAST + 1))):
        cell = Cell(args.width, args.height)
        if BOX_FIRST <= codepoint <= BOX_LAST:
            draw_box(cell, codepoint)
        elif BLOCK_FIRST <= codepoint <= BLOCK_LAST:
            draw_block(cell, codepoint)
        else:
            # Сглаженный рисунок с порогом яркости даёт у шрифта без
            # хинтинга более ровные штрихи, чем монохромная растеризация.
            image = Image.new('L', (args.width, args.height), 0)
            ImageDraw.Draw(image).text((0, baseline), chr(codepoint), font=font, anchor='ls', fill=255)
            for y in range(args.height):
                for x in range(args.width):
                    if image.getpixel((x, y)) >= args.threshold:
                        cell.set(x, y)
            if args.bold:
                embolden(cell)
        glyphs.append((codepoint, cell))

    fallback = 0xFFFD if 0xFFFD in cmap else ord('?')
    command = (f'--font {args.font} --width {args.width} --height {args.height} '
               f'--threshold {args.threshold} --name {args.name}' + (' --bold' if args.bold else ''))
    return {
        'width': args.width,
        'height': args.height,
        'baseline': baseline,
        'fallback': fallback,
        'source': f'шрифта {tt["name"].getDebugName(4)}\n'
                  f'// ({pathlib.Path(args.font).name}, {tt["name"].getDebugName(5)})',
        'command': command,
        'note': f'size {size:.2f} px',
    }, glyphs


def load_psf(path):
    """Читает шрифт консоли Linux в формате PSF1 или PSF2.

    Возвращает ширину и высоту знакоместа, изображения знаков (байты в
    формате esp::gfx) и словарь «код Unicode → номер знака». Из таблицы
    Unicode берутся только одиночные коды; последовательности кодов
    (буква с комбинируемым знаком) пропускаются.
    """
    data = pathlib.Path(path).read_bytes()
    if data[:2] == b'\x1f\x8b':
        data = gzip.decompress(data)
    table = {}
    if data[:2] == b'\x36\x04':
        # PSF1: ширина всегда 8, 256 или 512 знаков, коды — по 16 бит.
        mode, size = data[2], data[3]
        count = 512 if mode & 1 else 256
        width, height, offset = 8, size, 4
        glyphs = [data[offset + i * size:offset + (i + 1) * size] for i in range(count)]
        if mode & 2:
            pos = offset + count * size
            for i in range(count):
                in_sequence = False
                while True:
                    (code,) = struct.unpack_from('<H', data, pos)
                    pos += 2
                    if code == 0xFFFF:
                        break
                    if code == 0xFFFE:
                        in_sequence = True
                    elif not in_sequence:
                        table.setdefault(code, i)
        return width, height, glyphs, table
    magic, _, header_size, flags, count, size, height, width = struct.unpack_from('<8I', data)
    if magic != 0x864AB572:
        raise ValueError(f'{path}: not a PSF font')
    glyphs = [data[header_size + i * size:header_size + (i + 1) * size] for i in range(count)]
    if flags & 1:
        # PSF2: коды знака в UTF-8 до байта 0xFF; после 0xFE — последовательности.
        pos = header_size + count * size
        for i in range(count):
            end = data.index(b'\xff', pos)
            singles = data[pos:end].split(b'\xfe')[0].decode('utf-8')
            pos = end + 1
            for ch in singles:
                table.setdefault(ord(ch), i)
    return width, height, glyphs, table


def render_psf(args):
    """Берёт знаки шрифта консоли; возвращает параметры и список (код, знакоместо)."""
    width, height, data, table = load_psf(args.psf)
    row_bytes = (width + 7) // 8

    def cell_of(index):
        cell = Cell(width, height)
        for y in range(height):
            for x in range(width):
                if data[index][y * row_bytes + x // 8] & (0x80 >> (x % 8)):
                    cell.set(x, y)
        return cell

    codepoints = {c for c in table if selected(c)}
    glyphs = []
    for codepoint in sorted(codepoints | set(range(BOX_FIRST, BLOCK_LAST + 1))):
        if codepoint in codepoints:
            cell = cell_of(table[codepoint])
            if args.bold:
                embolden(cell)
        else:
            # Псевдографика, которой нет в шрифте, рисуется программно.
            cell = Cell(width, height)
            if codepoint <= BOX_LAST:
                draw_box(cell, codepoint)
            else:
                draw_block(cell, codepoint)
        glyphs.append((codepoint, cell))

    # Базовая линия — под самой нижней строкой точек буквы «H».
    h_cell = cell_of(table[ord('H')])
    baseline = max(y for y in range(height) if any(h_cell.pixels[y])) + 1
    drawn = sum(1 for c in range(BOX_FIRST, BLOCK_LAST + 1) if c not in codepoints)
    return {
        'width': width,
        'height': height,
        'baseline': baseline,
        'fallback': 0xFFFD if 0xFFFD in codepoints else ord('?'),
        'source': f'шрифта консоли Linux {pathlib.Path(args.psf).name}',
        'command': f'--psf {args.psf} --name {args.name}' + (' --bold' if args.bold else ''),
        'note': f'{len(data)} glyphs in the PSF file, {drawn} box/block glyphs drawn',
    }, glyphs


def comment(codepoint):
    """Комментарий к знаку: код и сам знак, если он видим. Знак берётся в
    кавычки, чтобы обратная косая черта не оказалась в конце строки."""
    ch = chr(codepoint)
    if unicodedata.category(ch)[0] in 'ZC':
        return f'U+{codepoint:04X}'
    return f"U+{codepoint:04X} '{ch}'"


def write(args, info, glyphs):
    """Записывает <имя>.h и <имя>.cpp."""
    out = pathlib.Path(args.output)
    out.mkdir(parents=True, exist_ok=True)
    name = args.name
    width, height = info['width'], info['height']
    row_bytes = (width + 7) // 8
    origin = (f'// Сгенерировано сценарием ttf2font.py из {info["source"]};\n'
              f'// не редактировать. Команда: python3 ttf2font.py {info["command"]}\n')

    header = f'''#pragma once

{origin}
// Растровый шрифт для esp::gfx::font: знакоместо
// {width} × {height} точек, число знаков Unicode — {len(glyphs)}. Строка
// точек занимает целое число байтов ({row_bytes}), старший бит — левая точка.

#include <cstddef>
#include <cstdint>


namespace {name}
{{

/** @brief Ширина знакоместа, точки. */
constexpr int WIDTH = {width};

/** @brief Высота знакоместа, точки. */
constexpr int HEIGHT = {height};

/** @brief Строка базовой линии в знакоместе (считая от нуля). */
constexpr int BASELINE = {info['baseline']};

/** @brief Размер изображения одного знака, байты. */
constexpr std::size_t GLYPH_SIZE = {row_bytes * height};

/** @brief Число знаков. */
constexpr std::size_t COUNT = {len(glyphs)};

/** @brief Код знака, который выводится вместо отсутствующих. */
constexpr std::uint16_t FALLBACK = 0x{info['fallback']:04X};

/** @brief Коды знаков по возрастанию. */
extern const std::uint16_t CODEPOINTS[COUNT];

/** @brief Изображения знаков в порядке CODEPOINTS, по GLYPH_SIZE байт. */
extern const std::uint8_t GLYPHS[COUNT * GLYPH_SIZE];

}}
'''

    lines = [f'{origin}\n#include "{name}.h"\n\n\nnamespace {name}\n{{\n',
             'const std::uint16_t CODEPOINTS[COUNT] =\n{']
    for i in range(0, len(glyphs), 12):
        lines.append('    ' + ', '.join(f'0x{c:04X}' for c, _ in glyphs[i:i + 12]) + ',')
    lines.append('};\n')
    lines.append('const std::uint8_t GLYPHS[COUNT * GLYPH_SIZE] =\n{')
    for codepoint, cell in glyphs:
        data = ', '.join(f'0x{b:02X}' for b in cell.to_bytes())
        lines.append(f'    {data}, // {comment(codepoint)}')
    lines.append('};\n\n}\n')

    (out / f'{name}.h').write_text(header, encoding='utf-8')
    (out / f'{name}.cpp').write_text('\n'.join(lines), encoding='utf-8')


def preview(path, info, glyphs):
    """Сохраняет таблицу всех знаков в PNG (увеличение ×2) для проверки."""
    width, height = info['width'], info['height']
    columns = 64
    rows = (len(glyphs) + columns - 1) // columns
    pad = 2
    image = Image.new('L', (columns * (width + pad), rows * (height + pad)), 40)
    for i, (_, cell) in enumerate(glyphs):
        ox = (i % columns) * (width + pad)
        oy = (i // columns) * (height + pad)
        for y in range(height):
            for x in range(width):
                image.putpixel((ox + x, oy + y), 255 if cell.pixels[y][x] else 0)
    image = image.resize((image.width * 2, image.height * 2), Image.NEAREST)
    image.save(path)


def main():
    here = pathlib.Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(
        description='Convert a TrueType font or a Linux console (PSF) font to an esp::gfx::font bitmap font '
                    'indexed by Unicode code point.')
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument('--font', help='TrueType font file')
    source.add_argument('--psf', help='Linux console font file, PSF1 or PSF2, optionally gzipped')
    parser.add_argument('--width', type=int, help='cell width, pixels (TrueType only)')
    parser.add_argument('--height', type=int, help='cell height, pixels (TrueType only)')
    parser.add_argument('--threshold', type=int, default=110, help='brightness threshold, 0-255 (TrueType only, default 110)')
    parser.add_argument('--bold', action='store_true', help='embolden: vertical strokes two pixels wide')
    parser.add_argument('--name', required=True, help='C++ namespace and file name')
    parser.add_argument('--output', default=str(here), help='output directory (default: the directory of this script)')
    parser.add_argument('--preview', help='also save a PNG with all glyphs')
    args = parser.parse_args()

    if args.font:
        if not args.width or not args.height:
            parser.error('--width and --height are required with --font')
        info, glyphs = render_ttf(args)
    else:
        if args.width or args.height:
            parser.error('--width and --height come from the font file with --psf')
        info, glyphs = render_psf(args)

    write(args, info, glyphs)
    if args.preview:
        preview(args.preview, info, glyphs)
    size = len(glyphs) * ((info['width'] + 7) // 8) * info['height']
    print(f'{args.name}: {len(glyphs)} glyphs {info["width"]}x{info["height"]}, {info["note"]}, '
          f'baseline {info["baseline"]}, {size} bytes of glyphs.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
