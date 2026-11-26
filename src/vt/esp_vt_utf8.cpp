/**
 * @file esp_vt_utf8.h
 * @brief Объявление действий над текстом UTF-8 esp::vt::utf8.
 */

/**
 * @file esp_vt_utf8.cpp
 * @brief Действия над текстом UTF-8: запись знака, шаг по знакам, проверка.
 */

#include "esp_vt_utf8.h"

#include "esp_vt_config.h"

#include <algorithm>


/**
 * @class esp::vt::utf8
 * @brief Действия над текстом UTF-8, которые нужны редактору строки и истории строк.
 *
 * Шаг по знакам и их счёт полагаются на то, что текст — правильный UTF-8:
 * знак начинается с любого байта, кроме продолжающего (0x80–0xBF). Такой
 * текст получается из кодов знаков (encode()) или проверяется
 * is_printable_line(). Один знак занимает одно знакоместо: знаков двойной
 * ширины и комбинируемых знаков в шрифте нет (раздел 6.1 документа об
 * устройстве).
 */


namespace esp::vt::p
{

/**
 * @brief Сообщает, продолжающий ли это байт UTF-8.
 * @param byte Байт текста.
 * @return @c true для байтов 0x80–0xBF: знак с них не начинается.
 */
constexpr bool is_utf8_continuation(char byte)
{
    return (static_cast<std::uint8_t>(byte) & 0xC0) == 0x80;
}

}


// Открытые

/**
 * @brief Записывает знак в UTF-8.
 *
 * Половинка суррогатной пары и код больше U+10FFFF записываются как знак
 * замены U+FFFD.
 *
 * @param code_point Код знака.
 * @param bytes Куда записать байты.
 * @return Число записанных байтов, 1–4.
 */
int esp::vt::utf8::encode(std::uint32_t code_point, std::array<char, MAX_BYTES> &bytes)
{
    if (code_point > 0x10FFFF || (code_point >= 0xD800 && code_point <= 0xDFFF))
        code_point = esp::vt::REPLACEMENT_CHARACTER;

    if (code_point < 0x80)
    {
        bytes[0] = static_cast<char>(code_point);
        return 1;
    }
    if (code_point < 0x800)
    {
        bytes[0] = static_cast<char>(0xC0 | (code_point >> 6));
        bytes[1] = static_cast<char>(0x80 | (code_point & 0x3F));
        return 2;
    }
    if (code_point < 0x10000)
    {
        bytes[0] = static_cast<char>(0xE0 | (code_point >> 12));
        bytes[1] = static_cast<char>(0x80 | ((code_point >> 6) & 0x3F));
        bytes[2] = static_cast<char>(0x80 | (code_point & 0x3F));
        return 3;
    }
    bytes[0] = static_cast<char>(0xF0 | (code_point >> 18));
    bytes[1] = static_cast<char>(0x80 | ((code_point >> 12) & 0x3F));
    bytes[2] = static_cast<char>(0x80 | ((code_point >> 6) & 0x3F));
    bytes[3] = static_cast<char>(0x80 | (code_point & 0x3F));
    return 4;
}

/**
 * @brief Возвращает начало следующего знака.
 * @param text Текст UTF-8.
 * @param index Начало знака в @p text или его конец.
 * @return Начало следующего знака; в конце текста — его длина.
 */
std::size_t esp::vt::utf8::next(std::string_view text, std::size_t index)
{
    if (index >= text.size())
        return text.size();
    ++index;
    while (index < text.size() && esp::vt::p::is_utf8_continuation(text[index]))
        ++index;
    return index;
}

/**
 * @brief Возвращает начало предыдущего знака.
 * @param text Текст UTF-8.
 * @param index Начало знака в @p text или его конец.
 * @return Начало предыдущего знака; в начале текста — 0.
 */
std::size_t esp::vt::utf8::previous(std::string_view text, std::size_t index)
{
    if (index == 0)
        return 0;
    index = std::min(index, text.size()) - 1;
    while (index > 0 && esp::vt::p::is_utf8_continuation(text[index]))
        --index;
    return index;
}

/**
 * @brief Считает знаки.
 * @param text Текст UTF-8.
 * @return Число знаков — и знакомест, которые текст займёт на экране.
 */
int esp::vt::utf8::length(std::string_view text)
{
    int count = 0;
    for (const char byte : text)
    {
        if (!esp::vt::p::is_utf8_continuation(byte))
            ++count;
    }
    return count;
}

/**
 * @brief Сообщает, печатный ли знак.
 *
 * Не печатные — управляющие знаки C0 (0x00–0x1F), DEL и C1 (0x80–0x9F), а
 * также коды за пределами Unicode и половинки суррогатных пар.
 *
 * @param code_point Код знака.
 * @return @c true, если знак можно поставить в строку.
 */
bool esp::vt::utf8::is_printable(std::uint32_t code_point)
{
    return code_point >= 0x20 && !(code_point >= 0x7F && code_point <= 0x9F)
        && code_point <= 0x10FFFF && !(code_point >= 0xD800 && code_point <= 0xDFFF);
}

/**
 * @brief Проверяет, что текст — правильный UTF-8 из одних печатных знаков.
 *
 * Испорченная запись — продолжающий байт без начала, недопустимый первый
 * байт, оборванный знак, избыточная запись — делает текст неправильным, как
 * и управляющий знак (is_printable()).
 *
 * @param text Текст.
 * @return @c true, если текст можно показать и править как строку.
 */
bool esp::vt::utf8::is_printable_line(std::string_view text)
{
    std::size_t index = 0;
    while (index < text.size())
    {
        const std::uint8_t byte = static_cast<std::uint8_t>(text[index]);
        std::uint32_t code_point = 0;
        std::uint32_t min_code_point = 0;
        std::size_t count = 0;
        if (byte < 0x80)
            code_point = byte;
        else if (byte >= 0xC2 && byte <= 0xDF)
        {
            code_point = byte & 0x1F;
            min_code_point = 0x80;
            count = 1;
        }
        else if (byte >= 0xE0 && byte <= 0xEF)
        {
            code_point = byte & 0x0F;
            min_code_point = 0x800;
            count = 2;
        }
        else if (byte >= 0xF0 && byte <= 0xF4)
        {
            code_point = byte & 0x07;
            min_code_point = 0x10000;
            count = 3;
        }
        else
            return false;

        // Знак оборван концом текста.
        if (index + count >= text.size())
            return false;
        for (std::size_t i = 1; i <= count; ++i)
        {
            if (!esp::vt::p::is_utf8_continuation(text[index + i]))
                return false;
            code_point = (code_point << 6) | (static_cast<std::uint8_t>(text[index + i]) & 0x3F);
        }
        if (code_point < min_code_point || !is_printable(code_point))
            return false;
        index += count + 1;
    }
    return true;
}
