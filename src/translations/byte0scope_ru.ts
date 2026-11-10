<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="ru_RU">
<context>
    <name>AboutDialog</name>
    <message>
        <location filename="../ui_aboutdialog.cpp" line="+30"/>
        <source>Byte-0-scope v%1 by pav13 (2026)</source>
        <translation>Байт-0-скоп v%1, автор pav13 (2026)</translation>
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
  * Hex view (bytes + ASCII) with configurable layout
  * Info panel with byte and pixel details
  * Optional byte tooltip that follows the cursor
  * Dec / Hex byte converter
  * Debug info panel
  * PNG export (visible area / 1:1 / current scale)
  * Compare two files via XOR
  * Save/load display settings to .b0s next to the file
  * Dark and light themes
  * Horizontal and vertical mirroring
  * Configurable tile size and cache limit
  * Built-in tutorial demo (&quot;Training&quot; mode)</source>
        <translation>Ключевые возможности:
  * Визуализация файлов любого размера (КБ – ГБ)
  * Палитры для 1/2/3/4/8/16/24/32/64 бит на пиксель
  * Многобайтовые форматы: RGB, RGBA, ABGR, RGB565/555, целые числа, float
  * Масштабирование и панорамирование, панорамный обзор
  * Hex-просмотр (байты + ASCII) с настраиваемым расположением
  * Информационная панель с детализацией байта и пикселя
  * Опциональная всплывающая подсказка с деталями байта, следующая за курсором
  * Конвертер байтов Dec / Hex
  * Панель отладочной информации
  * Экспорт в PNG (видимая область / 1:1 / текущий масштаб)
  * Сравнение двух файлов через XOR
  * Сохранение/загрузка настроек отображения в .b0s рядом с файлом
  * Тёмная и светлая темы
  * Зеркальное отражение по горизонтали и вертикали
  * Настраиваемый размер тайла и лимит кэша
  * Встроенный обучающий демо-файл (режим «Обучение»)</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>Controls:
  * Left-click a pixel — lock the byte
  * Right-click + drag — pan
  * Wheel — zoom
  * Ctrl + wheel — row width (switches to manual width mode)
  * Left-click in the hex view — select a byte (synced with canvas)
  * Drag the top edge of the hex view — resize it
  * &quot;|+|&quot; / &quot;+|&quot; buttons — pick region start / end by click
  * Ctrl+O — open file
  * Ctrl+S — save .b0s next to the file
  * Ctrl+L — load .b0s
  * Ctrl+R — reset display settings
  * Ctrl+E — export to PNG (full image, 1:1)
  * Ctrl+X — compare two files (XOR)
  * Ctrl+W — close current file
  * Drag &amp; drop — open a file (two files -&gt; XOR comparison)</source>
        <translation>Управление:
  * ЛКМ по пикселю — закрепить байт
  * ПКМ + перемещение — панорамирование
  * Колесо — масштаб
  * Ctrl + колесо — ширина строки (переключает в ручной режим ширины)
  * ЛКМ в hex-просмотре — выбрать байт (синхронизируется с холстом)
  * Перетаскивание верхнего края hex-просмотра — изменение высоты
  * Кнопки &quot;|+|&quot; / &quot;+|&quot; — выбор начала / конца региона кликом
  * Ctrl+O — открыть файл
  * Ctrl+S — сохранить .b0s рядом с файлом
  * Ctrl+L — загрузить .b0s
  * Ctrl+R — сбросить настройки отображения
  * Ctrl+E — экспорт в PNG (всё изображение, 1:1)
  * Ctrl+X — сравнить два файла (XOR)
  * Ctrl+W — закрыть текущий файл
  * Drag &amp; drop — открыть файл (два файла -&gt; XOR-сравнение)</translation>
    </message>
    <message>
        <location line="+18"/>
        <source>Formats and settings:
  * byte0scope-settings.ini — global settings (theme, language,
    window geometry, hex-view state, performance,
    recent files, default display parameters). Stored next
    to the executable; if writing is not possible —
    in the user configuration directory.
  * *.b0s — display settings for a specific file (JSON).
    Stored next to the source file and picked up
    automatically on open.</source>
        <translation>Форматы и настройки:
  * byte0scope-settings.ini — глобальные настройки (тема, язык,
    геометрия окна, состояние hex-просмотра, производительность,
    недавние файлы, параметры отображения по умолчанию). Хранится рядом
    с исполняемым файлом; если запись невозможна —
    в пользовательском каталоге конфигурации.
  * *.b0s — настройки отображения для конкретного файла (JSON).
    Хранится рядом с исходным файлом и подхватывается
    автоматически при открытии.</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Performance:
  * Memory-mapping (mmap) for large files
  * Tile rendering of the visible region only
  * LRU tile cache with automatic eviction
  * Prefetching of neighboring tiles
  * Tile size 256–2048 px, cache limit 180–1024 MB
    (up to 400 MB on 32-bit builds)</source>
        <translation>Производительность:
  * Отображение в память (mmap) для больших файлов
  * Тайловый рендеринг только видимой области
  * LRU-кэш тайлов с автоматическим вытеснением
  * Prefetching соседних тайлов
  * Размер тайла 256–2048 px, лимит кэша 180–1024 MB
    (до 400 MB в 32-битных сборках)</translation>
    </message>
    <message>
        <location line="+9"/>
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
        <location line="+64"/>
        <source>Close</source>
        <translation>Закрыть</translation>
    </message>
</context>
<context>
    <name>ByteConverterWidget</name>
    <message>
        <location filename="../ui_byteconverter.cpp" line="+61"/>
        <source>Dec:</source>
        <translation>Dec:</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Hex:</source>
        <translation>Hex:</translation>
    </message>
</context>
<context>
    <name>BytesSource</name>
    <message>
        <location filename="../core_bytesource.cpp" line="+83"/>
        <location line="+52"/>
        <source>Failed to open file:
%1</source>
        <translation>Не удалось открыть файл:
%1</translation>
    </message>
    <message>
        <location line="-44"/>
        <location line="+52"/>
        <source>Failed to determine file size.</source>
        <translation>Не удалось определить размер файла.</translation>
    </message>
    <message>
        <location line="-40"/>
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
        <location line="+12"/>
        <source>

This is a 32-bit build: it can address only 2-4 GB of memory in total. Use the 64-bit build to open large files.</source>
        <translation>

Это 32-разрядная сборка: она может адресовать в общей сложности лишь 2–4 ГБ памяти. Для открытия больших файлов используйте 64-разрядную сборку.</translation>
    </message>
    <message>
        <location line="+34"/>
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
        <location filename="../ui_controlspanel.cpp" line="+34"/>
        <source>End:</source>
        <translation>Конец:</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Length:</source>
        <translation>Длина:</translation>
    </message>
    <message>
        <location line="+71"/>
        <source>Pixel format</source>
        <translation>Формат пикселей</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Bits per pixel:</source>
        <translation>Бит на пиксель:</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Bit order:</source>
        <translation>Порядок битов:</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Byte order:</source>
        <translation>Порядок байтов:</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Byte order within multi-byte pixel values (16/24/32/64 bpp).
For RGB/RGBA palettes it also reverses the channel order
(e.g. RGB + little = BGR).</source>
        <translation>Порядок байтов в многобайтовых значениях пикселей (16/24/32/64 бита на пиксель).
Для палитр RGB/RGBA также меняется порядок каналов
(например, RGB + little-endian = BGR).</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Padding (bits):</source>
        <translation>Паддинг (бит):</translation>
    </message>
    <message>
        <location line="+6"/>
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
        <location line="+7"/>
        <source>Pixel order:</source>
        <translation>Порядок пикселей:</translation>
    </message>
    <message>
        <location line="+12"/>
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
        <location line="+6"/>
        <source>Region start (default 0)</source>
        <translation>Начало региона (по умолчанию 0)</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Pick region start: click, then left-click the desired byte.
Click again or press Esc to cancel.</source>
        <translation>Выбрать начало региона: нажмите, затем ЛКМ по нужному байту.
Повторное нажатие или Esc — отмена.</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Region end, or length if length mode is enabled</source>
        <translation>Конец региона, или длина, если включён режим длины</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Pick region end: click, then left-click the desired byte.
Click again or press Esc to cancel.</source>
        <translation>Выбрать конец региона: нажмите, затем щёлкните ЛКМ по нужному байту.
Повторное нажатие или Esc — отмена.</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Length mode</source>
        <translation>Режим длины</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>If checked, the &apos;End&apos; field is interpreted as &apos;Length&apos;</source>
        <translation>Если включено, поле «Конец» трактуется как «Длина»</translation>
    </message>
    <message>
        <location line="+117"/>
        <source>[X] Reset display
settings</source>
        <translation>[X] Сбросить настройки
отображения</translation>
    </message>
    <message>
        <location line="-127"/>
        <source>hex</source>
        <translation>hex</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Use hexadecimal input (0x…)</source>
        <translation>Использовать шестнадцатеричный ввод (0x…)</translation>
    </message>
    <message>
        <location line="+13"/>
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
        <location line="+21"/>
        <source>Width / N:</source>
        <translation>Ширина / N:</translation>
    </message>
    <message>
        <location line="+5"/>
        <location line="+343"/>
        <source>For &quot;Manual&quot; — width in pixels.
For &quot;Divisor ≤ N&quot; — upper width limit.</source>
        <translation>Для «Ручной» — ширина в пикселях.
Для «Делитель ≤ N» — верхний предел ширины.</translation>
    </message>
    <message>
        <location line="-338"/>
        <source>Width step:</source>
        <translation>Шаг ширины:</translation>
    </message>
    <message>
        <location line="+6"/>
        <location line="+343"/>
        <source>Width change step with Ctrl+mouse wheel.
Used in manual mode.</source>
        <translation>Шаг изменения ширины колесом мыши с Ctrl.
Используется в ручном режиме.</translation>
    </message>
    <message>
        <location line="-372"/>
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
        <location line="+29"/>
        <source>Zoom:</source>
        <translation>Масштаб:</translation>
    </message>
    <message>
        <location line="+8"/>
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
        <location line="+20"/>
        <source>Restore display parameters to default values</source>
        <translation>Восстановить параметры отображения по умолчанию</translation>
    </message>
    <message>
        <location line="+225"/>
        <source>Bit extraction order for bit-packed pixels.
MSB-first: highest bit of byte first.
LSB-first: lowest bit of byte first.</source>
        <translation>Порядок извлечения битов для упакованных пикселей.
MSB-first: сначала старший бит байта.
LSB-first: сначала младший бит байта.</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Bit order within each byte.
MSB-first: normal byte.
LSB-first: reverse bits in byte.</source>
        <translation>Порядок битов внутри каждого байта.
MSB-first: обычный байт.
LSB-first: биты в байте расположены в обратном порядке.</translation>
    </message>
    <message>
        <location line="+54"/>
        <source>Controlled by the current width mode.</source>
        <translation>Управляется текущим режимом ширины.</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Available only in &quot;Manual&quot; mode.</source>
        <translation>Доступно только в «ручном» режиме.</translation>
    </message>
    <message>
        <location line="+15"/>
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
        <location filename="../core_demobuilder.cpp" line="+269"/>
        <source>&lt;b&gt;Training: restore the image&lt;/b&gt;&lt;br&gt;&lt;br&gt;The file is loaded, but the current display settings are incorrect — you are seeing noise. Set the parameters to the correct values, and the &apos;BYTE&apos; heading, text, and graphics will appear.&lt;br&gt;&lt;br&gt;&lt;b&gt;Correct parameters:&lt;/b&gt;&lt;br&gt;&amp;nbsp;&amp;nbsp;1. &lt;b&gt;Bits per pixel&lt;/b&gt; -&gt; &lt;code&gt;4 bpp&lt;/code&gt;&lt;br&gt;&amp;nbsp;&amp;nbsp;2. &lt;b&gt;Bit order&lt;/b&gt; -&gt; &lt;code&gt;LSB-first&lt;/code&gt;&lt;br&gt;&amp;nbsp;&amp;nbsp;3. &lt;b&gt;Pixel order&lt;/b&gt; -&gt; &lt;code&gt;LSB-first&lt;/code&gt;&lt;br&gt;&amp;nbsp;&amp;nbsp;4. &lt;b&gt;Palette&lt;/b&gt; -&gt; &lt;code&gt;Any&lt;/code&gt;&lt;br&gt;&amp;nbsp;&amp;nbsp;5. &lt;b&gt;Width mode&lt;/b&gt; -&gt; &lt;code&gt;Manual&lt;/code&gt;&lt;br&gt;&amp;nbsp;&amp;nbsp;6. &lt;b&gt;Width&lt;/b&gt; -&gt; &lt;code&gt;96&lt;/code&gt;&lt;br&gt;&amp;nbsp;&amp;nbsp;7. &lt;b&gt;Padding (bits)&lt;/b&gt; -&gt; &lt;code&gt;16&lt;/code&gt;&lt;br&gt;&amp;nbsp;&amp;nbsp;8. &lt;b&gt;View&lt;/b&gt; -&gt; &lt;code&gt;Flip horizontally&lt;/code&gt;&lt;br&gt;</source>
        <translation>&lt;b&gt;Обучение: восстановите изображение&lt;/b&gt;&lt;br&gt;&lt;br&gt;Файл загружен, но текущие настройки отображения неверны — вы видите шум. Приведите параметры к правильным значениям, и появятся заголовок «BYTE», текст и графика.&lt;br&gt;&lt;br&gt;&lt;b&gt;Правильные параметры:&lt;/b&gt;&lt;br&gt;&amp;nbsp;&amp;nbsp;1. &lt;b&gt;Бит на пиксель&lt;/b&gt; -&gt; &lt;code&gt;4 bpp&lt;/code&gt;&lt;br&gt;&amp;nbsp;&amp;nbsp;2. &lt;b&gt;Порядок битов&lt;/b&gt; -&gt; &lt;code&gt;LSB-first&lt;/code&gt;&lt;br&gt;&amp;nbsp;&amp;nbsp;3. &lt;b&gt;Порядок пикселей&lt;/b&gt; -&gt; &lt;code&gt;LSB-first&lt;/code&gt;&lt;br&gt;&amp;nbsp;&amp;nbsp;4. &lt;b&gt;Палитра&lt;/b&gt; -&gt; &lt;code&gt;Любая&lt;/code&gt;&lt;br&gt;&amp;nbsp;&amp;nbsp;5. &lt;b&gt;Режим ширины&lt;/b&gt; -&gt; &lt;code&gt;Ручной&lt;/code&gt;&lt;br&gt;&amp;nbsp;&amp;nbsp;6. &lt;b&gt;Ширина&lt;/b&gt; -&gt; &lt;code&gt;96&lt;/code&gt;&lt;br&gt;&amp;nbsp;&amp;nbsp;7. &lt;b&gt;Паддинг (бит)&lt;/b&gt; -&gt; &lt;code&gt;16&lt;/code&gt;&lt;br&gt;&amp;nbsp;&amp;nbsp;8. &lt;b&gt;Вид&lt;/b&gt; -&gt; &lt;code&gt;Отразить по горизонтали&lt;/code&gt;&lt;br&gt;
</translation>
    </message>
</context>
<context>
    <name>FileAssoc</name>
    <message>
        <location filename="../app_fileassoc.cpp" line="+80"/>
        <location line="+75"/>
        <source>Executable path not found.</source>
        <translation>Не найден путь к exe.</translation>
    </message>
    <message>
        <location line="-53"/>
        <source>Failed to write to the registry.</source>
        <translation>Не удалось записать в реестр.</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>Failed to clean up the registry.</source>
        <translation>Не удалось очистить реестр.</translation>
    </message>
    <message>
        <location line="+49"/>
        <source>Failed to create MIME XML.</source>
        <translation>Не удалось создать MIME XML.</translation>
    </message>
    <message>
        <location line="+28"/>
        <source>Failed to create .desktop file.</source>
        <translation>Не удалось создать .desktop файл.</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>xdg-mime could not be started.</source>
        <translation>Не удалось запустить xdg-mime.</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>xdg-mime did not respond within the timeout.</source>
        <translation>xdg-mime не ответил за отведённое время.</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>xdg-mime failed to set the default application.</source>
        <translation>Утилите xdg-mime не удалось задать приложение по умолчанию.</translation>
    </message>
    <message>
        <location line="+47"/>
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
    <name>HexViewWidget</name>
    <message>
        <location filename="../ui_hexviewwidget.cpp" line="+383"/>
        <source>Decrease bytes per line by 8</source>
        <translation>Уменьшить количество байтов в строке на 8</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Increase bytes per line by 8</source>
        <translation>Увеличить количество байтов в строке на 8</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Go to selection</source>
        <translation>Перейти к выделенному</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Decrease font size</source>
        <translation>Уменьшить размер шрифта</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Increase font size</source>
        <translation>Увеличить размер шрифта</translation>
    </message>
</context>
<context>
    <name>MainWindow</name>
    <message>
        <location filename="../ui_mainwindow.cpp" line="+278"/>
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
        <location line="+36"/>
        <source>Reset display settings</source>
        <translation>Сбросить настройки отображения</translation>
    </message>
    <message>
        <location line="-32"/>
        <location line="+1124"/>
        <location line="+17"/>
        <location line="+31"/>
        <location line="+12"/>
        <location line="+30"/>
        <source>Export to PNG</source>
        <translation>Экспорт в PNG</translation>
    </message>
    <message>
        <location line="-1212"/>
        <source>Current visible area…</source>
        <translation>Текущая видимая область…</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Entire image (1:1)…</source>
        <translation>Всё изображение (1:1)…</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Entire image (current scale)…</source>
        <translation>Всё изображение (текущий масштаб)…</translation>
    </message>
    <message>
        <location line="+7"/>
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
        <location line="+7"/>
        <source>Flip horizontally</source>
        <translation>Отразить по горизонтали</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Flip vertically</source>
        <translation>Отразить по вертикали</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Reset flips</source>
        <translation>Сбросить отражения</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Hex view</source>
        <translation>Hex вид</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Settings</source>
        <translation>Настройки</translation>
    </message>
    <message>
        <location line="+3"/>
        <location line="+933"/>
        <source>Light theme</source>
        <translation>Светлая тема</translation>
    </message>
    <message>
        <location line="-933"/>
        <location line="+933"/>
        <source>Dark theme</source>
        <translation>Тёмная тема</translation>
    </message>
    <message>
        <location line="-929"/>
        <location line="+963"/>
        <source>Language</source>
        <translation>Язык</translation>
    </message>
    <message>
        <location line="-958"/>
        <location line="+954"/>
        <source>English</source>
        <translation>Английский</translation>
    </message>
    <message>
        <location line="-949"/>
        <location line="+949"/>
        <source>Russian</source>
        <translation>Русский</translation>
    </message>
    <message>
        <location line="-935"/>
        <source>Byte tooltip</source>
        <translation>Всплывающая подсказка по байту</translation>
    </message>
    <message>
        <location line="+14"/>
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
        <location line="+13"/>
        <source>Clear list</source>
        <translation>Очистить список</translation>
    </message>
    <message>
        <location line="+76"/>
        <location line="+397"/>
        <source>&lt;b&gt;Byte-0-scope - Interactive binary data visualizer&lt;/b&gt;</source>
        <translation>&lt;b&gt;Байт-0-скоп — интерактивный визуализатор бинарных данных&lt;/b&gt;</translation>
    </message>
    <message>
        <location line="-389"/>
        <source># Tutorial on a demo file</source>
        <translation># Обучение на демо-файле</translation>
    </message>
    <message>
        <location line="+32"/>
        <source>Panorama</source>
        <translation>Панорама</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Byte converter</source>
        <translation>Конвертер байтов</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>RAM usage</source>
        <translation>Использование ОЗУ</translation>
    </message>
    <message>
        <location line="+3"/>
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
        <source>256 px per tile · cache limit 180 MB</source>
        <translation>128 px на тайл · лимит кэша 180 МБ {256 ?} {180 ?}</translation>
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
        <source>2048 px per tile · cache limit 1 GB</source>
        <translation>2048 px на тайл · лимит кэша 1 ГБ</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Debug</source>
        <translation>Отладка</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>(enable in Settings -&gt; Debug information)</source>
        <translation>(включается в Настройки -&gt; Отладочная информация)</translation>
    </message>
    <message>
        <location line="+37"/>
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
        <location line="+16"/>
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
        <location line="+11"/>
        <source>Failed to open file</source>
        <translation>Не удалось открыть файл</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Reason unknown.</source>
        <translation>Причина неизвестна.</translation>
    </message>
    <message>
        <location line="+65"/>
        <source>File 1</source>
        <translation>Файл 1</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>File 2</source>
        <translation>Файл 2</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Error</source>
        <translation>Ошибка</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Failed to open one of the files for comparison.</source>
        <translation>Не удалось открыть один из файлов для сравнения.</translation>
    </message>
    <message>
        <location line="+11"/>
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
        <location line="+45"/>
        <source>File sizes differ (%1 and %2 bytes): only the first %3 bytes are compared.</source>
        <translation>Размеры файлов различаются (%1 и %2 байт): сравниваются только первые %3 байт.</translation>
    </message>
    <message>
        <location line="+7"/>
        <location line="+4"/>
        <source>XOR comparison</source>
        <translation>XOR сравнение</translation>
    </message>
    <message>
        <location line="-3"/>
        <source>The files are identical: there are no differences (%1 bytes compared).</source>
        <translation>Файлы идентичны: различий нет (сравнено байтов: %1).</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>The first %1 bytes of the files are identical, but the file sizes differ (%2 and %3 bytes).</source>
        <translation>Первые %1 байтов файлов идентичны, но размеры файлов различаются (%2 и %3 байта).</translation>
    </message>
    <message>
        <location line="+420"/>
        <source>Language set to %1.

Restart the application to apply the change.</source>
        <translation>Установлен язык: %1.

Перезапустите приложение, чтобы применить изменение.</translation>
    </message>
    <message>
        <location line="+28"/>
        <source>A PNG export is already running.</source>
        <translation>Экспорт в формат PNG уже выполняется.</translation>
    </message>
    <message>
        <location line="+45"/>
        <source>Save visible area as PNG</source>
        <translation>Сохранить видимую область как PNG</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Save entire image (1:1) as PNG</source>
        <translation>Сохранить всё изображение (1:1) в формате PNG</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Save entire image at current scale as PNG</source>
        <translation>Сохранить всё изображение в текущем масштабе как PNG</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>There is no visible image area to export.</source>
        <translation>Нет видимой области изображения для экспорта.</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Please use the .png file extension for PNG exports.</source>
        <translation>Для экспорта в формате PNG используйте расширение файла .png.</translation>
    </message>
    <message>
        <location line="+31"/>
        <source>The PNG destination must not be the currently opened source file.</source>
        <translation>Файл назначения PNG не должен быть тем же самым файлом, который открыт в данный момент.</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>PNG export started…</source>
        <translation>Начат экспорт в PNG…</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>PNG saved: %1</source>
        <translation>PNG сохранено: %1</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Failed to save PNG:
%1</source>
        <translation>Не удалось сохранить PNG:
%1</translation>
    </message>
    <message>
        <location line="-83"/>
        <source>PNG (*.png)</source>
        <translation>PNG (*.png)</translation>
    </message>
    <message>
        <location line="-1032"/>
        <source>Load the entire file into memory at once</source>
        <translation>Загружать файл сразу целиком в память</translation>
    </message>
    <message>
        <location line="+1086"/>
        <source>The requested PNG dimensions are invalid, or the export task could not be started.</source>
        <translation>Запрошенные размеры PNG-файла недопустимы, либо задачу экспорта не удалось запустить.</translation>
    </message>
    <message>
        <location line="+38"/>
        <source>Byte-0-scope v%1</source>
        <translation>Байт-0-скоп v%1</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>%1: [%2] [0x%3 / %4 bytes] [%5×%6]</source>
        <translation>%1: [%2] [0x%3 / %4 байт] [%5×%6]</translation>
    </message>
    <message>
        <location line="+6"/>
        <source> [differs: %1%]</source>
        <translation> [различие: %1%]</translation>
    </message>
    <message>
        <location line="+221"/>
        <source>[ Saving to PNG...]</source>
        <translation>[ Сохранение в PNG...]</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>[ Rendering...]</source>
        <translation>[ Рендеринг...]</translation>
    </message>
    <message>
        <location line="+44"/>
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
        <location filename="../ui_mainwindowconfig.cpp" line="+309"/>
        <source>The .b0s settings file is empty.</source>
        <translation>Файл настроек .b0s пуст.</translation>
    </message>
    <message>
        <location line="+50"/>
        <source>Could not save the .b0s settings: %1</source>
        <translation>Не удалось сохранить настройки .b0s: %1</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Settings saved: %1</source>
        <translation>Настройки сохранены: %1</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>No .b0s settings file found for this file.</source>
        <translation>Для этого файла не найден файл настроек .b0s.</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Could not read the .b0s settings: %1</source>
        <translation>Не удалось прочитать настройки .b0s: %1</translation>
    </message>
    <message>
        <location filename="../ui_mainwindow.cpp" line="+99"/>
        <source>%1 files were given: only the first two are compared (XOR).</source>
        <translation>Было передано %1 файлов: сравниваются только первые два (XOR).</translation>
    </message>
    <message>
        <location filename="../ui_mainwindowdebug.cpp" line="+34"/>
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
        <location filename="../ui_mainwindow.cpp" line="+62"/>
        <source>Tutorial: adjust the display settings to reveal the image.</source>
        <translation>Обучение: настройте параметры отображения, чтобы увидеть изображение.</translation>
    </message>
</context>
<context>
    <name>PaletteManager</name>
    <message>
        <location filename="../pal_palettemanager.cpp" line="+984"/>
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
        <location line="+80"/>
        <source>Heat</source>
        <translation>Тепловая</translation>
    </message>
    <message>
        <location line="-79"/>
        <source>Thermal</source>
        <translation>Термальная</translation>
    </message>
    <message>
        <location line="+1"/>
        <location line="+79"/>
        <source>Viridis</source>
        <translation>Вирдис</translation>
    </message>
    <message>
        <location line="-78"/>
        <location line="+79"/>
        <source>Plasma</source>
        <translation>Плазма</translation>
    </message>
    <message>
        <location line="-78"/>
        <location line="+79"/>
        <source>Rainbow</source>
        <translation>Радуга</translation>
    </message>
    <message>
        <location line="-78"/>
        <source>HSV</source>
        <translation>HSV</translation>
    </message>
    <message>
        <location line="+1"/>
        <location line="+78"/>
        <source>Solarize</source>
        <translation>Соляризация</translation>
    </message>
    <message>
        <location line="-77"/>
        <location line="+78"/>
        <source>Hash color</source>
        <translation>Хеш-цвет</translation>
    </message>
    <message>
        <location line="-77"/>
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
        <location line="+3"/>
        <source>Threshold</source>
        <translation>Порог</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>ASCII</source>
        <translation>ASCII</translation>
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
        <location line="+5"/>
        <source>Custom</source>
        <translation>Пользовательская</translation>
    </message>
    <message>
        <location line="-2"/>
        <source>Byte frequency</source>
        <translation>Частота байтов</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Repeats</source>
        <translation>Повторы</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>From file</source>
        <translation>Из файла</translation>
    </message>
    <message>
        <location line="-73"/>
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
        <location line="+3"/>
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
        <location line="+3"/>
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
        <location line="+3"/>
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
        <location line="+44"/>
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
        <source>Integer 16 (unsigned)</source>
        <translation>Integer 16 (unsigned)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Integer 16 (signed)</source>
        <translation>Integer 16 (signed)</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Integer 32 (unsigned)</source>
        <translation>Integer 32 (unsigned)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Integer 32 (signed)</source>
        <translation>Integer 32 (signed)</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Integer 64 (unsigned)</source>
        <translation>Integer 64 (unsigned)</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Integer 64 (signed)</source>
        <translation>Integer 64 (signed)</translation>
    </message>
    <message>
        <location line="-18"/>
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
        <location line="+3"/>
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
        <location line="+3"/>
        <source>Float 32</source>
        <translation>Float 32</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Float 64</source>
        <translation>Float 64</translation>
    </message>
</context>
<context>
    <name>PaletteSettingsPanel</name>
    <message>
        <location filename="../pal_palettesettings.cpp" line="+97"/>
        <source>Reset palette settings</source>
        <translation>Сбросить настройки палитры</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Auto-apply settings</source>
        <translation>Автоприменение настроек</translation>
    </message>
    <message>
        <location line="+131"/>
        <source>Printable (20–7E)</source>
        <translation>Печатные (20–7E)</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Controls (00-1F, 7F)</source>
        <translation>Управляющие (00-1F, 7F)</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Whitespace (09-0D, 20)</source>
        <translation>Пробельные (09-0D, 20)</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Digits (30-39)</source>
        <translation>Цифры (30-39)</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Uppercase (41-5A)</source>
        <translation>Заглавные (41-5A)</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Lowercase (61-7A)</source>
        <translation>Строчные (61-7A)</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Symbols(21-2F,3A-40,5B-60,7B-7E)</source>
        <translation>Символы(21-2F,3A-40,5B-60,7B-7E)</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Extended (80-FF)</source>
        <translation>Расширенные (80-FF)</translation>
    </message>
    <message>
        <location line="+65"/>
        <source>Step (bands):</source>
        <translation>Шаг (полосы):</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Bit plane (0–7):</source>
        <translation>Битовая плоскость (0–7):</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Threshold (0–255):</source>
        <translation>Порог (0–255):</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Bytes appearing at least N times
are highlighted in red.</source>
        <translation>Байты, встречающиеся не меньше N раз,
подсвечиваются красным.</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Min. repeat count:</source>
        <translation>Мин. число повторов:</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Byte values that occur at least N times in the file
are highlighted in red. The rest — grayscale.
Counted over the whole file, not the visible region.
Default = average count of distinct byte values
in the current file.</source>
        <translation>Значения байтов, встречающиеся в файле не менее N раз,
выделены красным цветом. Остальные — оттенками серого.
Подсчет ведется по всему файлу, а не только по видимой области.
По умолчанию — среднее количество вхождений различных значений байтов
в текущем файле.</translation>
    </message>
    <message>
        <location line="+44"/>
        <source>Byte order:</source>
        <translation>Порядок байтов:</translation>
    </message>
    <message>
        <location line="-85"/>
        <source>High values - fewer bands,
low values - more.</source>
        <translation>Высокие значения — меньше полос,
низкие значения — больше.</translation>
    </message>
    <message>
        <location line="-88"/>
        <source>Other:</source>
        <translation>Другие:</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Extended colors</source>
        <translation>Расширенные цвета</translation>
    </message>
    <message>
        <location line="+54"/>
        <source>Choose color (%1)</source>
        <translation>Выберите цвет (%1)</translation>
    </message>
    <message>
        <location line="+38"/>
        <source>Select bit plane.</source>
        <translation>Выберите битовую плоскость.</translation>
    </message>
    <message>
        <location line="+45"/>
        <source>Start address:</source>
        <translation>Начальный адрес:</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Max length (bytes):</source>
        <translation>Макс. длина (байт):</translation>
    </message>
    <message>
        <location line="+6"/>
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
        <location line="+3"/>
        <source>Entry size (bytes):</source>
        <translation>Размер записи (байт):</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Number of palette records.
0 = auto: 1&lt;&lt;bpp for bpp&lt;=4, otherwise 256.</source>
        <translation>Количество записей палитры.
0 = авто: 1&lt;&lt;bpp для bpp&lt;=4, иначе 256.</translation>
    </message>
    <message>
        <location line="+3"/>
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
        <location line="+191"/>
        <source>Color: %1</source>
        <translation>Цвет: %1</translation>
    </message>
</context>
<context>
    <name>PanoramaWidget</name>
    <message>
        <location filename="../ui_panorama.cpp" line="+127"/>
        <source>No image</source>
        <translation>Нет изображения</translation>
    </message>
</context>
<context>
    <name>TileManager</name>
    <message>
        <location filename="../render_tilemanager.cpp" line="+561"/>
        <location line="+199"/>
        <source>Invalid export dimensions.</source>
        <translation>Недопустимые размеры экспорта.</translation>
    </message>
    <message>
        <location line="-183"/>
        <source>The requested export area is outside the image.</source>
        <translation>Запрашиваемая область экспорта находится за пределами изображения.</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>The PNG row is too wide for this build.</source>
        <translation>Строка PNG слишком широка для этой сборки.</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Not enough memory to buffer a single PNG row.</source>
        <translation>Недостаточно памяти для буферизации одной строки PNG.</translation>
    </message>
    <message>
        <location line="+153"/>
        <source>PNG export was cancelled.</source>
        <translation>Экспорт в PNG отменён.</translation>
    </message>
    <message>
        <location line="+116"/>
        <location line="+6"/>
        <source>The export task could not be started.</source>
        <translation>Не удалось запустить задачу экспорта.</translation>
    </message>
    <message>
        <location line="-4"/>
        <source>A PNG export is already running.</source>
        <translation>Экспорт в PNG уже выполняется.</translation>
    </message>
</context>
</TS>
