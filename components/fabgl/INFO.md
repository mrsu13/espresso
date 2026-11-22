  * Источник: порт FabGL на ESP-IDF 6, ветка idf6-port репозитория https://github.com/shaeberling/FabGL/
    (запрос на слияние [#423](https://github.com/fdivitto/FabGL/pull/423) в https://github.com/fdivitto/FabGL/)
  * Версия: FabGL 1.0.9, коммит порта [3f21e97](https://github.com/shaeberling/FabGL/commit/3f21e97ff276e778d2940b4e32f1f54a8a6193d2) от 3 октября 2026 г.
  * Лицензия: [GPL-3.0](LICENSE)

## Что взято

Только вывод на VGA, клавиатура и мышь PS/2, звук и общее ядро, без которого они не работают.
Код разнесён по пространствам имён и каталогам модулей (раздел «Пространства имён и файлы»).

  * Графика, esp::gfx: контроллер дисплея, холст, растры, спрайты, указатели мыши, геометрия и
    цвета (в FabGL — displaycontroller, canvas, images/cursors.h и часть fabutils). Здесь же шрифт
    проекта esp::gfx::font.
  * VGA, esp::vga: контроллеры на 2, 4, 8, 16 и 64 цвета, прямой вывод и генератор сигнала через
    I2S GPIOStream, на котором они построены (dispdrivers/vga*, devdrivers/swgenerator).
  * PS/2, esp::ps2: контроллер и устройства PS/2 с программой сопроцессора ULP, клавиатура,
    раскладки, кодовые страницы, мышь и виртуальные клавиши (comdrivers/ps2controller,
    comdrivers/ps2device, ulp_macro_ex, devdrivers/keyboard, devdrivers/kbdlayouts,
    devdrivers/mouse, codepages).
  * Звук, esp::sound: генератор звука и формы волны (devdrivers/soundgen).
  * Общие помощники, esp: tmax(), tmin(), tclamp(), tswap(), msToTicks(), I2SModuleEnable(),
    esp_intr_alloc_pinnedToCore(), CoreUsage и VideoMode — то, чем пользуются несколько модулей.

## Что не взято

  * Шрифты: fabfonts и все шрифты (fonts/). Шрифты FabGL восьмибитные, по 256 знаков; проект
    выводит текст своим шрифтом Unicode (компонент consolefonts).
  * Текстовый режим VGA (dispdrivers/vgatextcontroller): он показывает только восьмибитные шрифты
    шириной 8 точек.
  * Из fabutils — файловый браузер FileBrowser (карта SD и SPIFFS) и функции АЦП ADC1_setup() и
    ADC1_read(); для карты памяти у проекта свой класс esp::sd_card. Вместе с ними ушли зависимости
    компонента от fatfs, spiffs, sdmmc, esp_driver_sdspi, esp_adc и vfs.
  * Графический интерфейс: fabui, inputbox, images/bitmaps.h.
  * Эмуляция терминала: terminal, terminfo.
  * Игровые сцены и столкновения спрайтов: scene, collisiondetector.
  * Другие устройства вывода: композитный видеосигнал (dispdrivers/cvbs*, devdrivers/cvbsgenerator),
    дисплеи TFT (dispdrivers/TFTController*) и OLED SSD1306 (dispdrivers/SSD1306Controller).
  * Другие устройства: comdrivers/serialport, comdrivers/tsi2c, devdrivers/DS3231, devdrivers/MCP23S17.
  * Эмуляторы процессоров и микросхем (emudevs), сеть (network).
  * Примеры, документация, инструменты, файлы сборки для Arduino и PlatformIO.

## Пространства имён и файлы

Код перенесён из пространства имён fabgl во вложенные пространства имён esp и разложен по
каталогам модулей src/gfx, src/vga, src/ps2 и src/sound; общие помощники лежат прямо в src. Имя
файла начинается с пространства имён, как требует руководство по стилю кода проекта
(doc/md/10-codestyle.md), а имя из другого модуля записывается полностью, от корня:
esp::gfx::Rect. Общий заголовок fabgl.h удалён вместе с объявлениями using, которые выносили
35 имён FabGL в глобальное пространство имён: программа включает заголовки нужных модулей.

| FabGL                             | Здесь                                                         |
|-----------------------------------|---------------------------------------------------------------|
| canvas                            | gfx/esp_gfx_canvas                                            |
| displaycontroller                 | gfx/esp_gfx_display_controller                                |
| images/cursors.h                  | gfx/esp_gfx_cursors.h                                         |
| dispdrivers/vga2controller        | vga/esp_vga_vga2_controller                                   |
| dispdrivers/vga4controller        | vga/esp_vga_vga4_controller                                   |
| dispdrivers/vga8controller        | vga/esp_vga_vga8_controller                                   |
| dispdrivers/vga16controller       | vga/esp_vga_vga16_controller                                  |
| dispdrivers/vgacontroller         | vga/esp_vga_vga_controller (64 цвета)                         |
| dispdrivers/vgabasecontroller     | vga/esp_vga_vga_base_controller                               |
| dispdrivers/vgapalettedcontroller | vga/esp_vga_vga_paletted_controller                           |
| dispdrivers/vgadirectcontroller   | vga/esp_vga_vga_direct_controller                             |
| devdrivers/swgenerator            | vga/esp_vga_gpio_stream                                       |
| comdrivers/ps2controller          | ps2/esp_ps2_ps2_controller                                    |
| comdrivers/ps2device              | ps2/esp_ps2_ps2_device                                        |
| devdrivers/keyboard               | ps2/esp_ps2_keyboard                                          |
| devdrivers/kbdlayouts             | ps2/esp_ps2_keyboard_layouts                                  |
| devdrivers/mouse                  | ps2/esp_ps2_mouse                                             |
| codepages                         | ps2/esp_ps2_codepages                                         |
| ulp_macro_ex                      | ps2/esp_ps2_ulp_macro_ex                                      |
| devdrivers/soundgen               | sound/esp_sound_sound_generator                               |
| fabutils                          | esp_utils, gfx/esp_gfx_utils, vga/esp_vga_utils, ps2/esp_ps2_utils, sound/esp_sound_utils |
| fabglconf.h                       | gfx/esp_gfx_config.h, vga/esp_vga_config.h, vga/esp_vga_modelines.h (строки режимов), ps2/esp_ps2_config.h; ядро для видеовывода — в esp_utils.h |
| fabgl.h                           | удалён                                                        |

Содержимое fabutils и fabglconf.h разложено по модулям, которые им пользуются. Файл
ulp_macro_ex — копия ulp_macro.c из ESP-IDF под лицензией Apache 2.0 — тоже перенесён в
пространство имён esp::ps2. Имена классов, функций и макросов пока прежние, из FabGL. В разделах
ниже файлы названы так, как в FabGL.

## Изменения

Изменения убирают связь с невзятыми графическим интерфейсом (класс fabgl::uiApp), шрифтами и
функциями файловой системы и АЦП.

  * devdrivers/keyboard.h, devdrivers/keyboard.cpp: удалены метод setUIApp(), поле m_uiApp, отправка
    событий клавиатуры в uiApp и включение fabui.h. Виртуальные клавиши по-прежнему проходят через
    обработчик onVirtualKey и попадают в очередь; запись в очередь ждёт свободного места без ограничения
    времени (portMAX_DELAY), как в оригинале при незаданном uiApp.
  * devdrivers/mouse.h, devdrivers/mouse.cpp: удалены метод setUIApp(), поле m_uiApp, параметр app
    метода setupAbsolutePositioner() и отправка событий мыши в uiApp. Вместо fabui.h включается
    displaycontroller.h, который раньше попадал через него.
  * fabgl.h удалён (раздел «Пространства имён и файлы»).
  * canvas.cpp: drawText() без выбранного шрифта ничего не рисует; раньше он выбирал FONT_8x8.
  * fabutils.h, fabutils.cpp: удалены FileBrowser (со структурой DirItem и перечислением DriveType)
    и функции АЦП вместе с их включениями.
  * Удалён код, которым после удаления терминала, графического интерфейса, текстового режима и
    файлового браузера никто не пользовался:
      * fabutils: Print и Stream, StringList с realloc32() и free32(), Stack и removeRectangle(),
        AutoSemaphore, таймер FRC1, digit2hex() и hex2digit(), replacePathSep(), getApbFrequency(),
        changeEndiannesWord() и changeEndiannesDWord(), taskExit(), twrap() и iswap(), а также
        getChipPackage() с ChipPackage и int2gpio(): у проекта свои esp::chip_package() и
        esp::int2gpio();
      * fabglconf.h: 23 настройки терминала, кэша шрифтов, отладочного вывода терминала и
        композитного видеосигнала;
      * displaycontroller, canvas: TextualDisplayController (основа удалённого текстового режима) и
        значение DisplayControllerType::Textual; вывод буфера знакомест — GlyphsBuffer,
        GlyphsBufferRenderInfo, макросы GLYPHMAP_* и функции glyphMapItem_*(),
        Canvas::renderGlyphsBuffer(), BitmappedDisplayController::renderGlyphsBuffer() и команда
        RenderGlyphsBuffer; указатель Sprite::collisionDetectorObject и объявление QuadTreeObject
        для невзятого CollisionDetector.

Примеры в комментариях canvas.h по-прежнему ссылаются на удалённые шрифты FONT_8x8 и
getPresetFontInfo().

## Исправления из других вариантов FabGL

Вариант vdp-gl (https://github.com/AgonConsole8/vdp-gl/) — FabGL 1.0.8, доработанный для
компьютера Agon; вариант Olimex (https://github.com/OLIMEX/FabGL/) добавляет только поддержку
платы ESP32-SBC-FabGL и к этой копии не относится. Из vdp-gl взято одно исправление:

  * displaycontroller.h: Sprite::nextFrame() (коммит c798533, Dean Belfield). Прежний код
    сначала увеличивал currentFrame и лишь затем сбрасывал его в ноль; рисование спрайта, которое
    идёт одновременно, могло застать номер кадра, равный framesCount, и прочитать указатель за
    концом массива кадров — Agon падал через несколько минут анимации. Теперь номер кадра
    записывается одним присваиванием.

Остальные исправления vdp-gl сюда не нужны:

  * раскладки клавиатуры (коммиты 2950fb2, 39d73fd, c2c5a79) — французская, бельгийская и
    испанская с мёртвыми клавишами; здесь используются раскладки без мёртвых клавиш;
  * аппаратные спрайты, «копер», преобразования растров, дуги и секторы — собственные возможности
    vdp-gl, вместе с исправлениями к ним (b4f3b5d, b41ffe1, 4f98779, cb16709, 66d199a и другие);
  * стек задачи рисования 3000 байт вместо 1200 (37f6c55) — его переполняло рисование сектора,
    которого в FabGL нет. В этой сборке (GCC 15, -Os) кадры стека функций рисования не больше
    160 байт, а самая глубокая цепочка вызовов — около 500 байт.

Файл CMakeLists.txt написан для этого проекта. Настройки ESP-IDF, которых требует FabGL, находятся
в sdkconfig.defaults проекта.
