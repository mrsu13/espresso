  * Источник: порт FabGL на ESP-IDF 6, ветка idf6-port репозитория https://github.com/shaeberling/FabGL/
    (запрос на слияние [#423](https://github.com/fdivitto/FabGL/pull/423) в https://github.com/fdivitto/FabGL/)
  * Версия: FabGL 1.0.9, коммит порта [3f21e97](https://github.com/shaeberling/FabGL/commit/3f21e97ff276e778d2940b4e32f1f54a8a6193d2) от 3 октября 2026 г.
  * Лицензия: [GPL-3.0](LICENSE)

## Что взято

Только вывод на VGA, клавиатура и мышь PS/2, звук и общее ядро, без которого они не работают.
Расположение файлов в каталоге src такое же, как в FabGL.

  * Ядро: fabutils, fabglconf.h, displaycontroller, canvas, codepages, images/cursors.h (указатели
    мыши), fabgl.h (общий заголовок).
  * VGA: контроллеры dispdrivers/vga* на 2, 4, 8, 16 и 64 цвета и прямой вывод (vgadirectcontroller);
    devdrivers/swgenerator — вывод сигнала через I2S, на котором построены контроллеры VGA.
  * PS/2: comdrivers/ps2controller, comdrivers/ps2device, ulp_macro_ex (программа сопроцессора ULP),
    devdrivers/keyboard, devdrivers/kbdlayouts, devdrivers/mouse.
  * Звук: devdrivers/soundgen.

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
  * fabgl.h: включает только взятые модули и объявляет через using только их типы; добавлено
    включение canvas.h, который раньше попадал через fabui.h и terminal.h. Описание библиотеки
    больше не обещает встроенных шрифтов.
  * canvas.cpp: drawText() без выбранного шрифта ничего не рисует; раньше он выбирал FONT_8x8.
  * fabutils.h, fabutils.cpp: удалены FileBrowser (со структурой DirItem и перечислением DriveType)
    и функции АЦП вместе с их включениями.

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
