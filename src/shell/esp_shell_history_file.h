#pragma once

#include "esp_non_copyable.h"
#include "esp_sdcard.h"
#include "esp_vt_line_history.h"

#include <array>
#include <string_view>


namespace esp::shell
{

class history_file : esp::non_copyable
{

public:

    explicit history_file(const esp::sd_card &sd_card);

    void load(esp::vt::line_history &history);
    void append(std::string_view line);

private:

    void rewrite(const esp::vt::line_history &history);
    void fail(const char *action);

    /** @brief Карта памяти, на которой лежит файл; живёт дольше объекта. */
    const esp::sd_card &_sd_card;

    /** @brief Путь к файлу: точка монтирования карты и имя файла; пусто, пока load() его не задала. */
    std::array<char, 32> _path;

    /** @brief Дописываются ли строки в файл: карта смонтирована, и ошибок записи не было. */
    bool _is_writable;
};

}
