#pragma once

// Сгенерировано сценарием ttf2font.py из шрифта консоли Linux Uni2-Terminus18x10.psf.gz;
// не редактировать. Команда: python3 ttf2font.py --psf /usr/share/consolefonts/Uni2-Terminus18x10.psf.gz --name ter_10x18

// Растровый шрифт для Canvas::drawGlyph() из FabGL: знакоместо
// 10 × 18 точек, число знаков Unicode — 819. Строка
// точек занимает целое число байтов (2), старший бит — левая точка.

#include <cstddef>
#include <cstdint>


namespace ter_10x18
{

/** @brief Ширина знакоместа, точки. */
constexpr int WIDTH = 10;

/** @brief Высота знакоместа, точки. */
constexpr int HEIGHT = 18;

/** @brief Строка базовой линии в знакоместе (считая от нуля). */
constexpr int BASELINE = 15;

/** @brief Размер изображения одного знака, байты. */
constexpr std::size_t GLYPH_SIZE = 36;

/** @brief Число знаков. */
constexpr std::size_t COUNT = 819;

/** @brief Код знака, который выводится вместо отсутствующих. */
constexpr std::uint16_t FALLBACK = 0xFFFD;

/** @brief Коды знаков по возрастанию. */
extern const std::uint16_t CODEPOINTS[COUNT];

/** @brief Изображения знаков в порядке CODEPOINTS, по GLYPH_SIZE байт. */
extern const std::uint8_t GLYPHS[COUNT * GLYPH_SIZE];

}
