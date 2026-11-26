/**
 * @file esp_shell_history_file.h
 * @brief Объявление файла истории строк esp::shell::history_file.
 */

/**
 * @file esp_shell_history_file.cpp
 * @brief Файл истории строк на SD-карте: чтение при запуске, дописывание каждой строки.
 */

#include "esp_shell_history_file.h"

#include "esp_vt_config.h"

#include <cerrno>
#include <cstdio>
#include <cstring>

#include <esp_log.h>


/**
 * @class esp::shell::history_file
 * @brief Файл истории строк на SD-карте, @c /sdcard/history.ff (раздел 11 документа об устройстве).
 *
 * При запуске оболочки load() читает файл в историю строк
 * esp::vt::line_history, а append() дописывает в него каждую новую строку
 * истории: строка текста на строку истории. Дописывание открывает и
 * закрывает файл, так что выполненная строка не теряется и при выключении
 * платы. Когда файл становится вдвое больше истории, load() переписывает
 * его строками, которые в истории остались.
 *
 * Без SD-карты файла нет, и история живёт только в памяти. Если писать в
 * файл не удалось — например, карту вынули, — журнал один раз
 * предупреждает об этом, и дальше история тоже живёт только в памяти.
 * Карту, вставленную после запуска, программа не видит до перезапуска.
 */


namespace esp::shell::p
{

/**
 * @brief Открытый файл: закрывается вместе с объектом.
 */
class file : esp::non_copyable
{

public:

    file(const char *path, const char *mode);
    ~file();

    std::FILE *get() const;
    bool close();

private:

    /** @brief Файл стандартной библиотеки; @c nullptr — не открылся или уже закрыт. */
    std::FILE *_file;
};

/** @brief Имя файла истории в корне SD-карты. */
const char *const HISTORY_FILE_NAME = "history.ff";

/** @brief Размер файла, при котором load() его переписывает: вдвое больше истории строк, байты. */
constexpr long HISTORY_FILE_COMPACT_SIZE = 2L * esp::vt::HISTORY_BYTES;

/** @brief Тег журнала модуля. */
const char *const TAG = "shell";

}


// Открытые

/**
 * @brief Создаёт объект; файл находит load().
 * @param sd_card Карта памяти; должна жить дольше объекта.
 */
esp::shell::history_file::history_file(const esp::sd_card &sd_card)
    : _sd_card{sd_card}
    , _path{}
    , _is_writable{false}
{
}

/**
 * @brief Читает файл в историю строк.
 *
 * Без SD-карты ничего не делает. Файла может ещё не быть: его создаст
 * первая append(). Строки, которые история не принимает, — слишком длинные
 * или с управляющими знаками (esp::vt::line_history::add()) —
 * пропускаются.
 *
 * @param history История строк.
 */
void esp::shell::history_file::load(esp::vt::line_history &history)
{
    if (!_sd_card.mounted())
        return;
    std::snprintf(_path.data(), _path.size(), "%s/%s", _sd_card.mount_point(),
                  esp::shell::p::HISTORY_FILE_NAME);
    _is_writable = true;

    esp::shell::p::file file{_path.data(), "r"};
    if (!file.get())
        return;

    // Строка, перевод строки и завершающий нуль; длинная строка
    // приходит кусками и пропускается целиком.
    std::array<char, esp::vt::MAX_LINE_BYTES + 2> buffer{};
    long size = 0;
    int count = 0;
    bool is_long_line = false;
    while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), file.get()))
    {
        std::string_view line{buffer.data()};
        size += static_cast<long>(line.size());
        const bool is_complete = !line.empty() && line.back() == '\n';
        if (is_long_line || (!is_complete && !std::feof(file.get())))
        {
            is_long_line = !is_complete;
            continue;
        }
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
            line.remove_suffix(1);
        if (history.add(line))
            ++count;
    }
    file.close();
    ESP_LOGD(esp::shell::p::TAG, "History: %d lines from %s.", count, _path.data());

    if (size > esp::shell::p::HISTORY_FILE_COMPACT_SIZE)
        rewrite(history);
}

/**
 * @brief Дописывает строку в файл.
 *
 * Без SD-карты и после ошибки записи ничего не делает.
 *
 * @param line Строка истории без перевода строки.
 */
void esp::shell::history_file::append(std::string_view line)
{
    if (!_is_writable)
        return;
    esp::shell::p::file file{_path.data(), "a"};
    const bool is_written = file.get()
        && std::fwrite(line.data(), 1, line.size(), file.get()) == line.size()
        && std::fputc('\n', file.get()) != EOF;
    if (!file.close() || !is_written)
        fail("append to");
}


// Закрытые

/**
 * @brief Переписывает файл строками истории, от самой старой к самой новой.
 * @param history История строк.
 */
void esp::shell::history_file::rewrite(const esp::vt::line_history &history)
{
    esp::shell::p::file file{_path.data(), "w"};
    bool is_written = file.get() != nullptr;
    for (int age = history.size() - 1; is_written && age >= 0; --age)
    {
        const std::string_view line = history.line(age);
        is_written = std::fwrite(line.data(), 1, line.size(), file.get()) == line.size()
            && std::fputc('\n', file.get()) != EOF;
    }
    if (!file.close() || !is_written)
        fail("rewrite");
    else
        ESP_LOGD(esp::shell::p::TAG, "History file %s rewritten.", _path.data());
}

/**
 * @brief Сообщает об ошибке записи и перестаёт писать в файл.
 * @param action Что не удалось, по-английски: «append to», «rewrite».
 */
void esp::shell::history_file::fail(const char *action)
{
    ESP_LOGW(esp::shell::p::TAG, "Cannot %s %s: %s. The history is kept in memory only.", action,
             _path.data(), std::strerror(errno));
    _is_writable = false;
}


/**
 * @brief Открывает файл.
 * @param path Путь.
 * @param mode Режим @c fopen().
 */
esp::shell::p::file::file(const char *path, const char *mode)
    : _file{std::fopen(path, mode)}
{
}

/**
 * @brief Закрывает файл, если он открыт.
 */
esp::shell::p::file::~file()
{
    close();
}

/**
 * @brief Возвращает файл.
 * @return Файл; @c nullptr — не открылся или уже закрыт.
 */
std::FILE *esp::shell::p::file::get() const
{
    return _file;
}

/**
 * @brief Закрывает файл; при записи это и сброс буфера на карту.
 * @return @c true, если файл был открыт и закрылся без ошибки.
 */
bool esp::shell::p::file::close()
{
    if (!_file)
        return false;
    const bool is_closed = std::fclose(_file) == 0;
    _file = nullptr;
    return is_closed;
}
