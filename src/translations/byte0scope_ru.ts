<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="ru_RU">
<context>
    <name>AboutDialog</name>
    <message>
        <location filename="../aboutdialog.cpp" line="+33"/>
        <source>Byte-0-scope v%1 by pav13</source>
        <translation>Байт-0-скоп v%1, автор pav13</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Interactive binary data visualizer.
License: GPL-3.0-or-later</source>
        <translation>Интерактивный визуализатор бинарных данных.
Лицензия: GPL-3.0-or-later</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Key features:
  * Visualization of files of any size (KB – GB)
  * Palettes for 1/2/3/4/8/16/24/32/64 bits per pixel
  * Multi-byte formats: RGB, RGBA, ABGR, RGB565/555, integers, float
  * Zooming and panning, panorama overview
  * Info panel with byte and pixel details
  * PNG export
  * Compare two files via XOR
  * Save/load display settings to .b0s next to the file
  * Dark and light themes
  * Horizontal and vertical mirroring
  * Configurable tile size and cache limit</source>
        <translation>Основные возможности:
  * Визуализация файлов любого размера (КБ – ГБ)
  * Палитры для 1/2/3/4/8/16/24/32/64 бит на пиксель
  * Многобайтовые форматы: RGB, RGBA, ABGR, RGB565/555, целые, float
  * Масштабирование и панорамирование, панорамный обзор
  * Информационная панель с детализацией байта и пикселя
  * Экспорт в PNG
  * Сравнение двух файлов через XOR
  * Сохранение/загрузка настроек отображения в .b0s рядом с файлом
  * Тёмная и светлая темы
  * Зеркальное отражение по горизонтали и вертикали
  * Настройка размера тайлов и лимита кэша</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>Controls:
  * Left-click a pixel — lock the byte
  * Right-click + drag — pan
  * Wheel — zoom
  * Ctrl + wheel — row width (in manual mode)
  * Ctrl+O — open file
  * Ctrl+S — save .b0s next to the file
  * Ctrl+L — load .b0s
  * Ctrl+R — reset display settings
  * Ctrl+E — export to PNG
  * Ctrl+X — compare two files (XOR)
  * Ctrl+W — close current file
  * Drag &amp; drop — open a file by dragging</source>
        <translation>Управление:
  * ЛКМ по пикселю — закрепить байт
  * ПКМ + перемещение — панорамирование
  * Колесо — масштаб
  * Ctrl + колесо — ширина строки (в ручном режиме)
  * Ctrl+O — открыть файл
  * Ctrl+S — сохранить .b0s рядом с файлом
  * Ctrl+L — загрузить .b0s
  * Ctrl+R — сбросить настройки отображения
  * Ctrl+E — экспорт в PNG
  * Ctrl+X — сравнить два файла (XOR)
  * Ctrl+W — закрыть текущий файл
  * Drag &amp; drop — открыть файл перетаскиванием</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Formats and settings:
  * b0s-settings.ini — global settings (theme, window geometry,
    performance, recent files). Stored next to the
    executable; if writing is not possible —
    in the user configuration directory.
  * *.b0s — display settings for a specific file (JSON).
    Stored next to the source file and picked up
    automatically on open.</source>
        <translation>Форматы и настройки:
  * b0s-settings.ini — глобальные настройки (тема, геометрия окна,
    производительность, последние файлы). Хранится рядом
    с исполняемым файлом; если запись невозможна —
    в пользовательском каталоге конфигурации.
  * *.b0s — настройки отображения конкретного файла (JSON).
    Хранится рядом с исходным файлом и подхватывается
    автоматически при открытии.</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Performance:
  * Memory-mapping (mmap) for large files
  * Tile rendering of the visible region only
  * LRU tile cache with automatic eviction
  * Prefetching of neighboring tiles
  * Tile size 128–1024 px, cache limit 180–2000 MB</source>
        <translation>Производительность:
  * Memory-mapping (mmap) для больших файлов
  * Тайловый рендеринг только видимой области
  * LRU-кэш тайлов с автоматической очисткой
  * Prefetching соседних тайлов
  * Размер тайла 128–1024 px, лимит кэша 180–2000 МБ</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Note: part of the code was generated or refined
using AI.</source>
        <translation>Примечание: часть кода была сгенерирована или доработана
с использованием ИИ.</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>About</source>
        <translation>О программе</translation>
    </message>
    <message>
        <location line="+67"/>
        <source>Close</source>
        <translation>Закрыть</translation>
    </message>
</context>
<context>
    <name>BytesSource</name>
    <message>
        <location filename="../bytesource.cpp" line="+78"/>
        <location line="+50"/>
        <source>Failed to open file:
%1</source>
        <translation>Не удалось открыть файл:
%1</translation>
    </message>
    <message>
        <location line="-42"/>
        <location line="+50"/>
        <source>Failed to determine file size.</source>
        <translation>Не удалось определить размер файла.</translation>
    </message>
    <message>
        <location line="-38"/>
        <source>Failed to map the file into memory.

File size: %1 MB
Reason: %2

Possible solutions:
• Enable &quot;Load whole file into memory&quot; in the settings
• Restart the program
• Check free space in the swap file</source>
        <translation>Не удалось отобразить файл в память.

Размер файла: %1 МБ
Причина: %2

Возможные решения:
• Включите «Загружать файл целиком в память» в настройках
• Перезапустите программу
• Проверьте свободное место в файле подкачки</translation>
    </message>
    <message>
        <location line="+44"/>
        <source>The file is too large to load into memory.

File size:    %1 MB (%2 bytes)
QByteArray max: %3 MB (%4 bytes)

Disable &quot;Load whole file into memory&quot; in the settings — the program will use mmap and can open this file.</source>
        <translation>Файл слишком большой для загрузки в память.

Размер файла:    %1 МБ (%2 байт)
Максимум QByteArray: %3 МБ (%4 байт)

Отключите «Загружать файл целиком в память» в настройках — программа будет использовать mmap и сможет открыть этот файл.</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Not enough free memory to load the file.

File size:    %1 MB
Memory available: %2 MB
Reserve needed:   %3 MB

Disable &quot;Load whole file into memory&quot; in the settings or close other applications.</source>
        <translation>Недостаточно свободной памяти для загрузки файла.

Размер файла:    %1 МБ
Доступно памяти: %2 МБ
Нужен резерв:    %3 МБ

Отключите «Загружать файл целиком в память» в настройках или закройте другие приложения.</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>File read error.
Read %1 of %2 bytes.
%3</source>
        <translation>Ошибка чтения файла.
Прочитано %1 из %2 байт.
%3</translation>
    </message>
</context>
<context>
    <name>ControlsPanel</name>
    <message>
        <location filename="../controlspanel.cpp" line="+76"/>
        <source>End:</source>
        <translation>Конец:</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Length:</source>
        <translation>Длина:</translation>
    </message>
    <message>
        <location line="+103"/>
        <source>Pixel format</source>
        <translation>Формат пикселей</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Bits per pixel:</source>
        <translation>Бит на пиксель:</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Bit order:</source>
        <translation>Порядок битов:</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Padding (bits):</source>
        <translation>Паддинг (бит):</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Row alignment. 0 — no padding.
8 — rows are multiples of 1 byte.
16 — multiples of 2 bytes.
32 — multiples of 4 bytes.</source>
        <translation>Выравнивание строк. 0 — без паддинга.
8 — строки кратны 1 байту.
16 — кратны 2 байтам.
32 — кратны 4 байтам.</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Pixel order:</source>
        <translation>Порядок пикселей:</translation>
    </message>
    <message>
        <location line="+18"/>
        <source>Palettes</source>
        <translation>Палитры</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Region</source>
        <translation>Регион</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Start:</source>
        <translation>Начало:</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Region start (default 0)</source>
        <translation>Начало региона (по умолчанию 0)</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>Pick region start: click, then left-click the desired byte.
Click again or press Esc to cancel.</source>
        <translation>Выбрать начало региона: нажмите, затем ЛКМ по нужному байту.
Повторное нажатие или Esc — отмена.</translation>
    </message>
    <message>
        <location line="+18"/>
        <source>Region end, or length if length mode is enabled</source>
        <translation>Конец региона, или длина, если включён режим длины</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>Pick region end: click, then left-click the desired byte.
Click again or press Esc to cancel.</source>
        <translation>Выбрать конец региона: нажмите, затем щёлкните ЛКМ по нужному байту.
Повторное нажатие или Esc — отмена.</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Length mode</source>
        <translation>Режим длины</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>If checked, the &apos;End&apos; field is interpreted as &apos;Length&apos;</source>
        <translation>Если включено, поле «Конец» трактуется как «Длина»</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>hex</source>
        <translation>hex</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Use hexadecimal input (0x…)</source>
        <translation>Использовать шестнадцатеричный ввод (0x…)</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>X</source>
        <translation>X</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Reset region: display all data</source>
        <translation>Сбросить регион: отображать все данные</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Display</source>
        <translation>Отображение</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Width mode:</source>
        <translation>Режим ширины:</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Manual</source>
        <translation>Ручной</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Auto (1:1)</source>
        <translation>Авто (1:1)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Auto (power of 2)</source>
        <translation>Авто (степень двойки)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Auto (16:9)</source>
        <translation>Авто (16:9)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Auto (4:3)</source>
        <translation>Авто (4:3)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Bytes per row</source>
        <translation>Байт в строке</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Nearest &quot;round&quot; value</source>
        <translation>Ближайшее «круглое» значение</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Divisor ≤ N</source>
        <translation>Делитель ≤ N</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Width / N:</source>
        <translation>Ширина / N:</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>For &quot;Manual&quot; — width in pixels.
For &quot;Divisor ≤ N&quot; — upper width limit.</source>
        <translation>Для «Ручной» — ширина в пикселях.
Для «Делитель ≤ N» — верхний предел ширины.</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Width step:</source>
        <translation>Шаг ширины:</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Width change step with Ctrl+mouse wheel.
Used in manual mode.</source>
        <translation>Шаг изменения ширины колесом мыши с Ctrl.
Используется в ручном режиме.</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Bytes per row:</source>
        <translation>Байт в строке:</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Only for &quot;Bytes per row&quot; mode.
Pixels per row = bytes / bytesPerPixel.</source>
        <translation>Только для режима «Байт в строке».
Пикселей в строке = байт / bytesPerPixel.</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Zoom:</source>
        <translation>Масштаб:</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Auto zoom</source>
        <translation>Автомасштаб</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Pick a zoom so the image takes up
maximum space in the window, but without shrinking.
Mutually exclusive with &quot;Fit to window&quot;.</source>
        <translation>Подобрать масштаб, чтобы изображение занимало
максимум места в окне, но без уменьшения.
Взаимоисключающе с «Вписать в окно».</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Fit to window</source>
        <translation>Вписать в окно</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Fit the entire image within the window bounds,
shrinking if necessary.
Mutually exclusive with &quot;Auto zoom&quot;.</source>
        <translation>Вписать изображение целиком в границы окна,
при необходимости уменьшая.
Взаимоисключающе с «Автомасштаб».</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>X Reset display settings</source>
        <translation>X Сбросить настройки отображения</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Restore display parameters to default values</source>
        <translation>Восстановить параметры отображения по умолчанию</translation>
    </message>
    <message>
        <location line="+200"/>
        <source>Controlled by &quot;Auto zoom&quot; or &quot;Fit to window&quot;.</source>
        <translation>Управляется «Автомасштабом» или «Вписать в окно».</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Display zoom.</source>
        <translation>Масштаб отображения.</translation>
    </message>
</context>
<context>
    <name>DemoBuilder</name>
    <message>
        <location filename="../demobuilder.cpp" line="+324"/>
        <source>&lt;b&gt;Training: restore the image&lt;/b&gt;&lt;br&gt;&lt;br&gt;The file is loaded, but the current display settings are incorrect — you are seeing noise. Set the parameters to the correct values, and the &apos;BYTE&apos; heading, text, and graphics will appear.&lt;br&gt;&lt;br&gt;&lt;b&gt;Correct parameters:&lt;/b&gt;&lt;br&gt;&amp;nbsp;&amp;nbsp;1. &lt;b&gt;Bits per pixel&lt;/b&gt; -&gt; &lt;code&gt;4 bpp&lt;/code&gt;&lt;br&gt;&amp;nbsp;&amp;nbsp;2. &lt;b&gt;Bit order&lt;/b&gt; -&gt; &lt;code&gt;LSB-first (LE)&lt;/code&gt;&lt;br&gt;&amp;nbsp;&amp;nbsp;3. &lt;b&gt;Pixel order&lt;/b&gt; -&gt; &lt;code&gt;LSB-first&lt;/code&gt;&lt;br&gt;&amp;nbsp;&amp;nbsp;4. &lt;b&gt;Padding (bits)&lt;/b&gt; -&gt; &lt;code&gt;16&lt;/code&gt;&lt;br&gt;&amp;nbsp;&amp;nbsp;5. &lt;b&gt;Palette&lt;/b&gt; -&gt; &lt;code&gt;Any&lt;/code&gt;&lt;br&gt;&amp;nbsp;&amp;nbsp;6. &lt;b&gt;Width mode&lt;/b&gt; -&gt; &lt;code&gt;Manual&lt;/code&gt;, width &lt;code&gt;96&lt;/code&gt;&lt;br&gt;&amp;nbsp;&amp;nbsp;7. &lt;b&gt;View -&gt; Mirror horizontally&lt;/b&gt;&lt;br&gt;&lt;br&gt;</source>
        <translation>&lt;b&gt;Обучение: восстановите изображение&lt;/b&gt;&lt;br&gt;&lt;br&gt;Файл загружен, но текущие настройки отображения неверны — вы видите шум. Приведите параметры к правильным, и появятся заголовок «BYTE», текст и графика.&lt;br&gt;&lt;br&gt;&lt;b&gt;Правильные параметры:&lt;/b&gt;&lt;br&gt;&amp;nbsp;&amp;nbsp;1. &lt;b&gt;Бит на пиксель&lt;/b&gt; -&gt; &lt;code&gt;4 bpp&lt;/code&gt;&lt;br&gt;&amp;nbsp;&amp;nbsp;2. &lt;b&gt;Порядок битов&lt;/b&gt; -&gt; &lt;code&gt;LSB-first (LE)&lt;/code&gt;&lt;br&gt;&amp;nbsp;&amp;nbsp;3. &lt;b&gt;Порядок пикселей&lt;/b&gt; -&gt; &lt;code&gt;LSB-first&lt;/code&gt;&lt;br&gt;&amp;nbsp;&amp;nbsp;4. &lt;b&gt;Паддинг (бит)&lt;/b&gt; -&gt; &lt;code&gt;16&lt;/code&gt;&lt;br&gt;&amp;nbsp;&amp;nbsp;5. &lt;b&gt;Палитра&lt;/b&gt; -&gt; &lt;code&gt;Любая&lt;/code&gt;&lt;br&gt;&amp;nbsp;&amp;nbsp;6. &lt;b&gt;Режим ширины&lt;/b&gt; -&gt; &lt;code&gt;Ручной&lt;/code&gt;, ширина &lt;code&gt;96&lt;/code&gt;&lt;br&gt;&amp;nbsp;&amp;nbsp;7. &lt;b&gt;Вид -&gt; Отразить по горизонтали&lt;/b&gt;&lt;br&gt;&lt;br&gt;</translation>
    </message>
</context>
<context>
    <name>FileAssoc</name>
    <message>
        <location filename="../fileassoc.cpp" line="+77"/>
        <location line="+69"/>
        <source>Executable path not found.</source>
        <translation>Не найден путь к exe.</translation>
    </message>
    <message>
        <location line="-47"/>
        <source>Failed to write to the registry.</source>
        <translation>Не удалось записать в реестр.</translation>
    </message>
    <message>
        <location line="+26"/>
        <source>Failed to clean up the registry.</source>
        <translation>Не удалось очистить реестр.</translation>
    </message>
    <message>
        <location line="+40"/>
        <source>Failed to create MIME XML.</source>
        <translation>Не удалось создать MIME XML.</translation>
    </message>
    <message>
        <location line="+29"/>
        <source>Failed to create .desktop file.</source>
        <translation>Не удалось создать .desktop файл.</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>xdg-mime did not respond within the timeout.</source>
        <translation>xdg-mime не ответил за отведённое время.</translation>
    </message>
    <message>
        <location line="+48"/>
        <source>File association registration is not supported on this platform.</source>
        <translation>Регистрация сопоставления файлов не поддерживается на этой платформе.</translation>
    </message>
    <message>
        <location line="+7"/>
        <location line="+33"/>
        <source>Association</source>
        <translation>Сопоставление</translation>
    </message>
    <message>
        <location line="-32"/>
        <source>Extension %1 has been registered to this program.</source>
        <translation>Расширение %1 зарегистрировано для этой программы.</translation>
    </message>
    <message>
        <location line="+4"/>
        <location line="+33"/>
        <source>Error</source>
        <translation>Ошибка</translation>
    </message>
    <message>
        <location line="-32"/>
        <source>Failed to register %1.
%2</source>
        <translation>Не удалось зарегистрировать %1.
%2</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>File association removal is not supported on this platform.</source>
        <translation>Удаление сопоставления файлов не поддерживается на этой платформе.</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Association for %1 has been removed.</source>
        <translation>Сопоставление для %1 удалено.</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Failed to remove %1.
%2</source>
        <translation>Не удалось удалить %1.
%2</translation>
    </message>
</context>
<context>
    <name>MainWindow</name>
    <message>
        <location filename="../mainwindow.cpp" line="+281"/>
        <source>File</source>
        <translation>Файл</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Open…</source>
        <translation>Открыть…</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Recent files</source>
        <translation>Недавние файлы</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Compare two files (XOR) and show differences…</source>
        <translation>Сравнить два файла (XOR) и показать различия…</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Save .b0s settings for file</source>
        <translation>Сохранить настройки .b0s для файла</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Load .b0s settings for file</source>
        <translation>Загрузить настройки .b0s для файла</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Reset display settings</source>
        <translation>Сбросить настройки отображения</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Export to PNG…</source>
        <translation>Экспорт в PNG…</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Close current file</source>
        <translation>Закрыть текущий файл</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Exit</source>
        <translation>Выход</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>View</source>
        <translation>Вид</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Flip horizontally</source>
        <translation>Отразить по горизонтали</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Flip vertically</source>
        <translation>Отразить по вертикали</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Reset flips</source>
        <translation>Сбросить отражения</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Settings</source>
        <translation>Настройки</translation>
    </message>
    <message>
        <location line="+3"/>
        <location line="+862"/>
        <source>Light theme</source>
        <translation>Светлая тема</translation>
    </message>
    <message>
        <location line="-862"/>
        <location line="+862"/>
        <source>Dark theme</source>
        <translation>Тёмная тема</translation>
    </message>
    <message>
        <location line="-858"/>
        <location line="+890"/>
        <source>Language</source>
        <translation>Язык</translation>
    </message>
    <message>
        <location line="-885"/>
        <location line="+881"/>
        <source>English</source>
        <translation>Английский</translation>
    </message>
    <message>
        <location line="-876"/>
        <location line="+876"/>
        <source>Russian</source>
        <translation>Русский</translation>
    </message>
    <message>
        <location line="-862"/>
        <source>Byte tooltip</source>
        <translation>Подсказка по байту</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Load the entire file into memory</source>
        <translation>Загружать файл целиком в память</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Off (default) — the file is mapped into memory via mmap.
Suitable for large files: the OS loads pages on demand,
RAM is not allocated in full.

On — the file is read into a single QByteArray.
Faster, but requires a contiguous RAM block the size of the file.
On 32-bit builds files &gt;1.5 GB may not fit.

Applied the next time a file is opened.</source>
        <translation>Выкл (по умолчанию) — файл отображается в память через mmap.
Подходит для больших файлов: ОС подгружает страницы по мере
обращения, ОЗУ не выделяется целиком.

Вкл — файл читается в QByteArray одним куском.
Быстрее, но требует непрерывного блока ОЗУ размером с файл.
На 32-bit сборке файлы &gt;1.5 ГБ могут не поместиться.

Применяется при следующем открытии файла.</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Load mode changed. It will take effect the next time a file is opened.</source>
        <translation>Режим загрузки изменён. Применится при следующем открытии файла.</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Associate .b0s with this program</source>
        <translation>Сопоставить .b0s с этой программой</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Remove .b0s association</source>
        <translation>Удалить сопоставление .b0s</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Debug information (right panel)</source>
        <translation>Отладочная информация (правая панель)</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Open settings folder</source>
        <translation>Открыть папку настроек</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Help</source>
        <translation>Справка</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>About…</source>
        <translation>О программе…</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>(no recent files)</source>
        <translation>(нет недавних файлов)</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Clear list</source>
        <translation>Очистить список</translation>
    </message>
    <message>
        <location line="+55"/>
        <location line="+363"/>
        <source>&lt;b&gt;Byte-0-scope - Interactive binary data visualizer&lt;/b&gt;</source>
        <translation>&lt;b&gt;Байт-0-скоп — интерактивный визуализатор бинарных данных&lt;/b&gt;</translation>
    </message>
    <message>
        <location line="-355"/>
        <source># Tutorial on a demo file</source>
        <translation># Обучение на демо-файле</translation>
    </message>
    <message>
        <location line="+33"/>
        <source>Panorama</source>
        <translation>Панорама</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>RAM usage</source>
        <translation>Использование ОЗУ</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>· Process memory: — MB
· Tile cache: — MB</source>
        <translation>· Память процесса: — МБ
· Кэш тайлов: — МБ</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>-</source>
        <translation>-</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>128 px per tile · cache limit 180 MB</source>
        <translation>128 px на тайл · лимит кэша 180 МБ</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Small tiles — more frequent updates, smaller pause per tile,
but more overhead and more objects in the cache.

Large tiles — less overhead,
but each tile renders longer (in the background).

In &quot;Fit to window&quot; mode tiles are stored downscaled
(according to the fit scale), so the cache uses
little memory even for gigabyte-sized files.

Cache limit — target for invisible tiles. Visible tiles are protected
from eviction, so momentarily the cache may exceed the limit.</source>
        <translation>Мелкие тайлы — чаще обновления, меньше пауз на один тайл,
но больше накладных расходов и объектов в кэше.

Крупные тайлы — меньше накладных расходов,
но каждый тайл рендерится дольше (в фоне).

В режиме «Вписать в окно» тайлы хранятся в уменьшенном виде
(в соответствии с масштабом вписывания), поэтому кэш занимает
мало памяти даже для гигабайтных файлов.

Лимит кэша — цель для невидимых тайлов. Видимые защищены
от вытеснения, поэтому на мгновение кэш может его превышать.</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>+</source>
        <translation>+</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>1024 px per tile · cache limit 2 GB</source>
        <translation>1024 px на тайл · лимит кэша 2 ГБ</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Debug</source>
        <translation>Отладка</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>(enable in Settings -&gt; Debug information)</source>
        <translation>(включается в Настройки -&gt; Отладочная информация)</translation>
    </message>
    <message>
        <location line="+31"/>
        <source>Ready</source>
        <translation>Готово</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Open file</source>
        <translation>Открыть файл</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>All files (*.*)</source>
        <translation>Все файлы (*.*)</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>.b0s config</source>
        <translation>Конфиг .b0s</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Data file for the config was not found:
%1

Expected: %2</source>
        <translation>Файл данных для конфига не найден:
%1

Ожидался: %2</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Failed to open file</source>
        <translation>Не удалось открыть файл</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Reason unknown.</source>
        <translation>Причина неизвестна.</translation>
    </message>
    <message>
        <location line="+53"/>
        <source>File 1</source>
        <translation>Файл 1</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>File 2</source>
        <translation>Файл 2</translation>
    </message>
    <message>
        <location line="+15"/>
        <location line="+543"/>
        <source>Error</source>
        <translation>Ошибка</translation>
    </message>
    <message>
        <location line="-542"/>
        <source>Failed to open one of the files for comparison.</source>
        <translation>Не удалось открыть один из файлов для сравнения.</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Files are too large</source>
        <translation>Файлы слишком большие</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>XOR requires ~%1 MB, but the maximum QByteArray is %2 MB.
Build a 64-bit version or reduce the file sizes.</source>
        <translation>Для XOR нужно ~%1 МБ, а максимум QByteArray — %2 МБ.
Соберите 64-битную версию или уменьшите файлы.</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Not enough memory</source>
        <translation>Недостаточно памяти</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>XOR requires ~%1 MB, but only %2 MB is available.</source>
        <translation>Для XOR нужно ~%1 МБ, а доступно только %2 МБ.</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Read error</source>
        <translation>Ошибка чтения</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Failed to read the files completely.</source>
        <translation>Не удалось прочитать файлы полностью.</translation>
    </message>
    <message>
        <location line="+443"/>
        <source>Language set to %1.

Restart the application to apply the change.</source>
        <translation>Установлен язык: %1.

Перезапустите приложение, чтобы применить изменение.</translation>
    </message>
    <message>
        <location line="+34"/>
        <source>Save PNG</source>
        <translation>Сохранить PNG</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>PNG (*.png)</source>
        <translation>PNG (*.png)</translation>
    </message>
    <message>
        <location line="+22"/>
        <source>Failed to save.</source>
        <translation>Не удалось сохранить.</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Byte-0-scope v%1</source>
        <translation>Байт-0-скоп v%1</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>%1: [%2] [0x%3 / %4 bytes] [%5×%6]</source>
        <translation>%1: [%2] [0x%3 / %4 байт] [%5×%6]</translation>
    </message>
    <message>
        <location line="+309"/>
        <source>[ Saving to PNG...]</source>
        <translation>[ Сохранение в PNG...]</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>[ Rendering...]</source>
        <translation>[ Рендеринг...]</translation>
    </message>
    <message>
        <location line="+43"/>
        <source>· Process memory: %1 MB</source>
        <translation>· Память процесса: %1 МБ</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>· Process memory: — MB</source>
        <translation>· Память процесса: — МБ</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>· Tile cache: %1 MB / %2 MB</source>
        <translation>· Кэш тайлов: %1 МБ / %2 МБ</translation>
    </message>
    <message>
        <location line="+87"/>
        <source>The .b0s settings file is empty.</source>
        <translation>Файл настроек .b0s пуст.</translation>
    </message>
    <message>
        <location line="+169"/>
        <source>GB</source>
        <translation>ГБ</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>MB</source>
        <translation>МБ</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>KB</source>
        <translation>КБ</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>B</source>
        <translation>Б</translation>
    </message>
    <message>
        <location line="+223"/>
        <source>Tutorial: adjust the display settings to reveal the image.</source>
        <translation>Обучение: настройте параметры отображения, чтобы увидеть изображение.</translation>
    </message>
</context>
<context>
    <name>PaletteManager</name>
    <message>
        <location filename="../palettemanager.cpp" line="+896"/>
        <source>Grayscale</source>
        <translation>Оттенки серого</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Inverted</source>
        <translation>Инвертированная</translation>
    </message>
    <message>
        <location line="+1"/>
        <location line="+183"/>
        <source>Heat</source>
        <translation>Тепловая</translation>
    </message>
    <message>
        <location line="-182"/>
        <source>Thermal</source>
        <translation>Термальная</translation>
    </message>
    <message>
        <location line="+1"/>
        <location line="+182"/>
        <source>Viridis</source>
        <translation>Вирдис</translation>
    </message>
    <message>
        <location line="-181"/>
        <location line="+182"/>
        <source>Plasma</source>
        <translation>Плазма</translation>
    </message>
    <message>
        <location line="-181"/>
        <location line="+182"/>
        <source>Rainbow</source>
        <translation>Радуга</translation>
    </message>
    <message>
        <location line="-181"/>
        <source>HSV</source>
        <translation>HSV</translation>
    </message>
    <message>
        <location line="+1"/>
        <location line="+181"/>
        <source>Solarize</source>
        <translation>Соляризация</translation>
    </message>
    <message>
        <location line="-180"/>
        <location line="+181"/>
        <source>Hash color</source>
        <translation>Хеш-цвет</translation>
    </message>
    <message>
        <location line="-180"/>
        <source>Bit scale</source>
        <translation>Битовая шкала</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Bit rotation</source>
        <translation>Битовая ротация</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Zeros/Ones</source>
        <translation>Нули/Единицы</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Non-zero</source>
        <translation>Не-Ноль</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>High bit set</source>
        <translation>Установлен старший бит</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Low bit set</source>
        <translation>Установлен младший бит</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Nibbles</source>
        <translation>Полубайты</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>High nibble</source>
        <translation>Старший полубайт</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Low nibble</source>
        <translation>Младший полубайт</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Threshold</source>
        <translation>Порог</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Entropy bands</source>
        <translation>Полосы энтропии</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Bit plane</source>
        <translation>Битовая плоскость</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Custom</source>
        <translation>Пользовательская</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Byte frequency</source>
        <translation>Частота байтов</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Repeats</source>
        <translation>Повторы</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>From file</source>
        <translation>Из файла</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Mono 1bpp</source>
        <translation>Моно 1bpp</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Inverted mono</source>
        <translation>Инвертированный моно</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Amber mono</source>
        <translation>Янтарный моно</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Green mono</source>
        <translation>Зелёный моно</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Red mono</source>
        <translation>Красный моно</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Blue mono</source>
        <translation>Синий моно</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Grayscale 2bpp</source>
        <translation>Оттенки серого 2bpp</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Game Boy (classic)</source>
        <translation>Game Boy (классический)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Game Boy (Pocket)</source>
        <translation>Game Boy (Pocket)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>CGA palette 0</source>
        <translation>CGA палитра 0</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>CGA palette 1</source>
        <translation>CGA палитра 1</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>CGA high</source>
        <translation>CGA высокая</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Green-amber</source>
        <translation>Зелёно-янтарный</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Neon 2bpp</source>
        <translation>Неоновый 2bpp</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Grayscale 3bpp</source>
        <translation>Оттенки серого 3bpp</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>RGB 3-3-2</source>
        <translation>RGB 3-3-2</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Bright rainbow 8</source>
        <translation>Яркая радуга 8</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Warm 3bpp</source>
        <translation>Тёплая 3bpp</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Cool 3bpp</source>
        <translation>Холодная 3bpp</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Grayscale 4bpp</source>
        <translation>Оттенки серого 4bpp</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>EGA 16 colors</source>
        <translation>EGA 16 цветов</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>EGA alternative</source>
        <translation>EGA альтернативная</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Windows 16 colors</source>
        <translation>Windows 16 цветов</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Commodore 64</source>
        <translation>Commodore 64</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>ZX Spectrum</source>
        <translation>ZX Spectrum</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Amber CRT</source>
        <translation>Amber CRT</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Green terminal</source>
        <translation>Зелёный терминал</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Heat gradient 16</source>
        <translation>Тепловой градиент 16</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Rainbow 16</source>
        <translation>Радуга 16</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>RGB565</source>
        <translation>RGB565</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>BGR565</source>
        <translation>BGR565</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>RGB555</source>
        <translation>RGB555</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>ARGB1555</source>
        <translation>ARGB1555</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Integer 16</source>
        <translation>Integer 16</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>RGB</source>
        <translation>RGB</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>BGR</source>
        <translation>BGR</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>GBR</source>
        <translation>GBR</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>BRG</source>
        <translation>BRG</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>GRB</source>
        <translation>GRB</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>RBG</source>
        <translation>RBG</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>RGBA</source>
        <translation>RGBA</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>ARGB</source>
        <translation>ARGB</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>BGRA</source>
        <translation>BGRA</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>ABGR</source>
        <translation>ABGR</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Integer 32</source>
        <translation>Integer 32</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Float 32</source>
        <translation>Float 32</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Integer 64</source>
        <translation>Integer 64</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Float 64</source>
        <translation>Float 64</translation>
    </message>
</context>
<context>
    <name>PaletteSettingsPanel</name>
    <message>
        <location filename="../palettesettings.cpp" line="+105"/>
        <source>Reset palette settings</source>
        <translation>Сбросить настройки палитры</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Auto-apply settings</source>
        <translation>Автоприменение настроек</translation>
    </message>
    <message>
        <location line="+156"/>
        <source>Step (bands):</source>
        <translation>Шаг (полосы):</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>High values — fewer bands,
low values — more.</source>
        <translation>Высокие значения — меньше полос,
низкие — больше.</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Bit plane (0–7):</source>
        <translation>Битовая плоскость (0–7):</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>0 = least significant bit,
7 = most significant bit.</source>
        <translation>0 = младший бит,
7 = старший бит.</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Threshold (0–255):</source>
        <translation>Порог (0–255):</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Bytes appearing at least N times
are highlighted in red.</source>
        <translation>Байты, встречающиеся не меньше N раз,
подсвечиваются красным.</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Min. repeat count:</source>
        <translation>Мин. число повторов:</translation>
    </message>
    <message>
        <location line="+13"/>
        <location line="+29"/>
        <location line="+29"/>
        <location line="+55"/>
        <source>Byte order:</source>
        <translation>Порядок байтов:</translation>
    </message>
    <message>
        <location line="-109"/>
        <source>16-bit: R=5, G=6, B=5
Typical for TFT screens,
BMP-16, GBA, old video cards.</source>
        <translation>16-бит: R=5, G=6, B=5
Формат типичен для TFT-экранов,
BMP-16, GBA, старых видеокарт.</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Same as RGB565, but
the byte order is swapped.</source>
        <translation>То же, что RGB565, но
порядок байтов обратный.</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>16-bit: R=5, G=5, B=5
The most significant bit is usually unused.</source>
        <translation>16-бит: R=5, G=5, B=5
Старший бит обычно не используется.</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>16-bit: A=1, R=5, G=5, B=5
Transparency is ignored.</source>
        <translation>16-бит: A=1, R=5, G=5, B=5
Прозрачность игнорируется.</translation>
    </message>
    <message>
        <location line="+18"/>
        <source>Signed</source>
        <translation>Знаковый</translation>
    </message>
    <message>
        <location line="+44"/>
        <source>Start address:</source>
        <translation>Начальный адрес:</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Max length (bytes):</source>
        <translation>Макс. длина (байт):</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Color format:</source>
        <translation>Цветовой формат:</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Size of one palette record in bytes.
RGB — 3, RGB565 — 2, RGBA — 4.</source>
        <translation>Размер одной записи палитры в байтах.
Для RGB — 3, для RGB565 — 2, для RGBA — 4.</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Entry size (bytes):</source>
        <translation>Размер записи (байт):</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Number of palette records.
0 = auto: 1&lt;&lt;bpp for bpp&lt;=4, otherwise 256.</source>
        <translation>Количество записей палитры.
0 = авто: 1&lt;&lt;bpp для bpp&lt;=4, иначе 256.</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Entries (0=auto):</source>
        <translation>Записи (0=авто):</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>%1 entries (from bpp)</source>
        <translation>Записей: %1 (из bpp)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>256 entries</source>
        <translation>256 записей</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Format &apos;auto&apos; by record size:
1 → grayscale, 2 → RGB565,
3 → RGB, 4+ → RGBA.
Auto-count: %1.</source>
        <translation>Формат «auto» по размеру записи:
1 → grayscale, 2 → RGB565,
3 → RGB, 4+ → RGBA.
Авто-количество: %1.</translation>
    </message>
    <message>
        <location line="+18"/>
        <source>Color outside ranges:</source>
        <translation>Цвет вне диапазонов:</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>&lt;b&gt;Range rules&lt;/b&gt;</source>
        <translation>&lt;b&gt;Правила диапазонов&lt;/b&gt;</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>+ Add rule</source>
        <translation>+ Добавить правило</translation>
    </message>
    <message>
        <location line="+203"/>
        <source>Color: %1</source>
        <translation>Цвет: %1</translation>
    </message>
</context>
<context>
    <name>PanoramaWidget</name>
    <message>
        <location filename="../panoramawidget.cpp" line="+99"/>
        <source>No image</source>
        <translation>Нет изображения</translation>
    </message>
</context>
</TS>
