# EX-T3-WiFi — Technical Reference

A DCC-EX WiFi throttle running on the **ESP32-2432S028R** ("Cheap Yellow Display" / CYD). The device connects over WiFi to a DCC-EX Command Station and exposes a web UI for loco/consist/function-set management.

---

## Table of Contents

1. [Hardware Target](#hardware-target)
2. [Repository Layout](#repository-layout)
3. [Build System](#build-system)
4. [Firmware Architecture](#firmware-architecture)
   - [Entrypoint — `src/main.cpp`](#entrypoint--srcmaincpp)
   - [Core Layer — `src/core/`](#core-layer--srccore)
   - [UI Layer — `src/ui/`](#ui-layer--srcui)
   - [Fonts & Icons — `src/fonts/`, `src/icons/`, `src/ui/`](#fonts--icons)
5. [Web Interface](#web-interface)
   - [Static Assets — `data/www/`](#static-assets--datawww)
   - [REST API](#rest-api)
   - [Data Files — `data/` and `sd/`](#data-files)
6. [Tooling & Scripts](#tooling--scripts)
7. [Coding Guidelines](#coding-guidelines)
8. [Key Data Flows](#key-data-flows)

---

## Hardware Target

| Item | Detail |
|------|--------|
| MCU | ESP32 (dual-core Xtensa LX6) |
| Board ID | `esp32-2432S028R` |
| Display | 2.8″ ILI9341 TFT, 240×320, SPI (HSPI) |
| Touch | XPT2046 resistive, bit-banged SPI |
| Storage | LittleFS (two partitions) + SD card (VSPI) |
| Rotary encoder | GPIO 22/27/35 (CLK / DT / BTN) |
| Backlight | GPIO 21, active-LOW PWM |

---

## Repository Layout

```
EX-T3-WiFi/
├── platformio.ini          # PlatformIO project & build config
├── partitions.csv          # Custom partition table
├── include/
│   └── Version.h           # Firmware version constants (major.minor.patch)
├── src/
│   ├── main.cpp            # Arduino setup() / loop() — wires everything together
│   ├── core/               # Business logic, no UI
│   │   ├── DCCExCS.*       # (legacy) low-level DCC-EX protocol wrapper
│   │   ├── Events.*        # Observer/event-listener mixin (Events base class)
│   │   ├── FileSystems.*   # Two LittleFS mounts: ConfigFS + WebsiteFS
│   │   ├── Functions.h     # Small inline utilities (divideAndCeil, fromChars…)
│   │   ├── Locos.*         # Active-loco list with prev/next navigation
│   │   ├── Settings.*      # Persistent settings (JSON, LittleFS)
│   │   ├── ThrottleServer.*# AsyncWebServer: REST API + static file serving
│   │   └── XPT2046_Bitbang.* # Bit-bang SPI driver for the touch controller
│   ├── ui/                 # LVGL screen classes (one per app tab)
│   │   ├── LVGL_Layouts.*  # Root layout: header bar, tabview, mutex
│   │   ├── lv_port_fs.*    # LVGL filesystem driver (maps to SD card)
│   │   ├── LocoUI.*        # Throttle tab: speed arc, direction, fn buttons
│   │   ├── AccessoriesUI.* # Accessories tab: turnout/output control
│   │   ├── PowerUI.*       # Power tab: MAIN / PROG track power
│   │   ├── SettingsUI.*    # Settings tab: WiFi, CS, display, calibration
│   │   ├── AboutUI.*       # About tab: version info, QR code
│   │   ├── ConsistUI.*     # Consist-builder overlay (launched from LocoUI)
│   │   ├── CalibrationUI.* # Touch calibration wizard
│   │   ├── ProgramUI.*     # CV read/write programming track screen
│   │   ├── WiFiUI.*        # WiFi scan / credential entry overlay
│   │   ├── logo.*          # Boot splash image (LVGL image descriptor)
│   │   ├── fa_gauge_high_16.c # Compiled Font Awesome gauge icon
│   │   └── LVGL_Layouts.h  # (see above)
│   ├── fonts/
│   │   ├── fa_icons_18.c   # Compiled Font Awesome subset at 18 px (LVGL font)
│   │   └── *.ttf / *.otf   # Source font files (not flashed)
│   ├── icons/
│   │   ├── dcc_icon.*      # App icon (C array + header)
│   │   └── train_icon.*    # Alternate icon
│   ├── scripts/
│   │   ├── convert_a8.py   # Convert BMP to LVGL A8 alpha image
│   │   └── invert_bytes.py # Byte-inversion utility for bitmap data
│   └── utils/
│       ├── Screenshot.*    # LVGL screenshot → BMP (debug utility)
│       └── TeeStream.h     # Stream that forwards to two sinks + callback
├── lib/                    # (empty — no local libraries)
├── data/
│   ├── default.json        # Default function-button layout (page array of slot arrays)
│   ├── fonts/              # Processing font (.vlw) for TFT_eSPI text (legacy)
│   ├── header/             # BMP icons shown in the LVGL header bar
│   └── www/                # Web UI source (served from WebsiteFS / LittleFS)
├── data.gz/                # Auto-generated: gzip-compressed copy of data/ (flashed)
├── sd/
│   ├── locos/1.json        # Example per-loco config file
│   ├── consists/1.json     # Example consist config file
│   └── groups.json         # Loco group definitions
├── 3d/                     # Printable enclosure files (.step + .stl)
├── emulators/
│   ├── cs_emulator.py      # Python mock of a DCC-EX Command Station (TCP)
│   ├── throttle_server_emulator.py # Mock throttle REST server
│   └── generate_icon.py    # Icon generation helper
├── icons/                  # Marketing / README artwork
└── tools/
    └── gen_logo.py         # Generates logo.c from a PNG
```

---

## Build System

**PlatformIO** (`platformio.ini`) manages the full build.

### Key build flags

| Flag | Purpose |
|------|---------|
| `ILI9341_2_DRIVER` + TFT pin defines | Configures TFT_eSPI for the CYD display |
| `LV_USE_TFT_ESPI=1` | LVGL uses TFT_eSPI as its display driver |
| `LV_USE_FONT_COMPRESSED=1` | Enables compressed glyph storage |
| `LV_USE_BMP=1` | LVGL can decode BMP image files |
| `LV_USE_QRCODE=1` | QR code widget used on the About screen |
| `ASYNCWEBSERVER_REGEX` | Enables regex route matching in ESPAsyncWebServer |
| `CONFIG_ASYNC_TCP_RUNNING_CORE=1` | Pins async TCP to core 1 (same as Arduino loop) |
| `-std=gnu++17` | C++17 required for `std::from_chars`, structured bindings, etc. |

### Extra scripts (run by PlatformIO)

| Script | What it does |
|--------|-------------|
| `data_gzip.py` | Compresses `data/` → `data.gz/` before flashing the LittleFS image |
| `build_release.py` | Post-build: packages firmware + filesystem into a release ZIP |
| `upload_website.py` | Uploads only the website partition over OTA/serial without reflashing firmware |

### Partition table (`partitions.csv`)

Two LittleFS partitions are used:
- **config** — stores `settings.json` and per-loco / per-function JSON files (small, survives firmware updates)
- **website** — stores the compressed web UI (`data.gz/www/`)

---

## Firmware Architecture

### Entrypoint — `src/main.cpp`

`main.cpp` owns global state and wires together every subsystem:

| Symbol | Type | Purpose |
|--------|------|---------|
| `touchscreen` | `XPT2046_Bitbang` | Bit-bang SPI touch controller |
| `csClient` | `WiFiClient` | TCP socket to the DCC-EX Command Station |
| `csMonitor` | `TeeStream` | Splits CS traffic: raw bytes → `csClient`; text lines → `onCSCommand` callback |
| `dccexProtocol` | `DCCEXProtocol` | DCC-EX protocol library; `check()` called every loop tick |
| `appDelegate` | `AppDelegate` | Receives `DCCEXProtocol` callbacks and forwards to the appropriate `*UI` object |
| `locos` | `Locos` | Tracks which loco addresses are currently acquired |
| `rotaryEncoder` | `AiEsp32RotaryEncoder` | ISR-driven rotary encoder; drives speed nudge via LVGL timer |
| `throttleServer` | `ThrottleServer` | AsyncWebServer on port 80 |
| `locoUI / accUI / pwrUI / setUI_ptr` | `unique_ptr<*UI>` | LVGL screen objects for each tab |
| `lvgl_mutex` | `SemaphoreHandle_t` | Guards all LVGL calls from background FreeRTOS tasks |

**FreeRTOS tasks created at boot:**

| Task | Stack | Core | Responsibility |
|------|-------|------|----------------|
| `keepWiFiAlive` | 4 KB | 1 | Reconnects WiFi + TCP to the CS; updates header WiFi icon |
| `powerCheck` | 2 KB | 1 | Samples battery ADC (10× average, every 2 min); updates header power icon |

**LVGL timers (run inside `lv_timer_handler()`):**

| Period | Purpose |
|--------|---------|
| 20 ms | Poll rotary encoder delta → `locoUI->nudgeSpeed()` |
| 50 ms | Process DNS requests (captive portal when AP mode active) |

**`loop()` responsibilities:**
1. Detect `csConnectPending` flag set by `keepWiFiAlive` and call `dccexProtocol.connect()` (must happen on the main thread).
2. Detect CS TCP disconnect and update header.
3. Call `dccexProtocol.check()` to parse incoming CS traffic (fires `AppDelegate` callbacks).
4. Call `lv_timer_handler()` to drive LVGL redraws and timer callbacks.

All of the above is wrapped in a single `lvgl_mutex` take/give to serialise LVGL access with background tasks.

---

### Core Layer — `src/core/`

#### `FileSystems.h / .cpp`
Declares two global `LittleFSFS` instances:
- `ConfigFS` — mounted at `/config`
- `WebsiteFS` — mounted at `/website`

Both are opened in `setup()` before anything else.

#### `Settings.h / .cpp`
A singleton `SettingsClass Settings` loaded from `ConfigFS:/settings.json`.

Key sub-structs:

| Struct | Fields | Notes |
|--------|--------|-------|
| `CS` | `_ssid`, `_password`, `_server`, `_port` | Command Station WiFi + TCP connection details |
| `AP` | `SSID`, `password`, `enabled` | Soft-AP (captive portal) settings |
| `TouchCal` | `xMin/xMax/yMin/yMax` | Resistive touch calibration values |
| `LocoUI` | `speedStep`, `acceleration` | Encoder behaviour |

`SettingsClass` extends `Events` — callers register lambdas for events such as `CS_CHANGE`, `THEME_CHANGE`, `ROTATION_CHANGE`, etc. The emitter pattern keeps `main.cpp` reactions decoupled from the setting structs themselves.

#### `Locos.h / .cpp`
Manages the ordered list of acquired loco addresses. Wraps a `std::vector<uint16_t>` with an active-index pointer. Exposes `add()`, `remove()`, `next()`, `prev()` and fires `COUNT_CHANGE` events.

Casting the object to `uint16_t` returns the currently active loco address.

#### `ThrottleServer.h / .cpp`
An `AsyncWebServer` on port 80. Two handlers are registered:

1. **`ThrottleAPIHandler`** — handles the REST API for loco / function / icon / consist / group files. Routes are matched against URL prefix and file extension; files are read from / written to `ConfigFS` or SD card depending on `Settings.storageMode`.
2. **`WebLoggerHandler`** — logs every incoming request to serial (debug only; always returns `canHandle = false` so it never intercepts).

Static files in `WebsiteFS` are served with gzip encoding (the `.gz` files are stored directly and the `Content-Encoding: gzip` header is set automatically).

#### `XPT2046_Bitbang.h / .cpp`
Software SPI driver for the XPT2046 touch IC on the CYD. Reads raw ADC coordinates and battery voltage. Used instead of a hardware SPI driver because the CYD wires the touch SPI onto different pins from the display SPI.

#### `Events.h / .cpp`
Observer/event-listener mixin. Any class that inherits `Events` gains `addEventListener(event, callback)`, `removeEventListener(event, id)`, and `dispatchEvent(event, data*)`. Used by `Settings`, `Locos`, and `DCCExCS`.

#### `DCCExCS.h / .cpp`
Legacy class — wraps raw `AsyncClient` sends/receives using regex matching and a response-callback pattern. The current codebase uses the upstream **DCCEXProtocol** library instead; this file remains for reference / possible future use.

#### `Functions.h`
Header-only utility functions:
- `divideAndCeil(i, div)` — integer ceiling division
- `fromChars<T>(str)` — `std::from_chars` wrapper for safe integer parsing
- `randomChars(buf, len)` — fills a buffer with random alphanumeric characters

#### `TeeStream.h` (`src/utils/`)
A `Stream` subclass that writes to two underlying streams simultaneously. Also accepts a `std::function<void(const char*)>` callback invoked on each newline-terminated line, used to parse CS server-description broadcasts without blocking the main protocol loop.

---

### UI Layer — `src/ui/`

All screens use **LVGL 9** with the `LVGL_CYD` board support library. Each screen class inherits from `UIView` (an empty virtual base for lifetime management).

#### `LVGL_Layouts.h / .cpp`
Creates and owns the top-level LVGL object tree:
- **Header bar** (`header_bar`) — fixed 30 px strip at the top. Shows: loco count badge, WiFi RSSI icon, Command Station connected/disconnected icon, battery voltage icon.
- **Main tabview** (`main_tabview`) — four tabs: Loco, Accessories, Power, Settings.
- Tab container pointers (`loco_tab`, `acc_tab`, `pwr_tab`, `set_tab`) are passed to the `*UI` constructors.
- Exports `apply_theme()` which reads `Settings.theme` and applies LVGL palette overrides.
- `lvgl_mutex` semaphore is created here.

#### `LocoUI.h / .cpp`
Throttle screen. Two sub-views:

1. **Selection menu** — lists saved locos (fetched from the REST API), allows entering an address directly, selecting a group, or building/joining a consist.
2. **Control screen** — speed arc (0–126), direction toggle switch, up to 8 function buttons per page with a page-forward button.

Internal state is a `LocoState` struct `{address, speed, direction, functions}`. A `SPEED_LOCAL_HOLD_MS` (400 ms) guard prevents server-echo from snapping the arc back while the user is still turning the encoder.

Function buttons are built from a per-loco JSON file (loaded via REST). Each button has idle/pressed label, colour, fill, border, and an optional Font Awesome icon codepoint.

Key public methods:
- `nudgeSpeed(delta)` — called from the encoder timer; increments speed and sends a throttle command.
- `onLocoUpdate(Loco*)` — called by `AppDelegate` when the CS broadcasts a state change.
- `onConsistUpdate(int, CSConsist*)` — updates display when consist membership changes.

#### `AccessoriesUI.h / .cpp`
Accessories tab. Displays turnouts and outputs; calls `dccexProtocol` accessory commands. `receivedTurnoutAction()` updates toggle button state on CS-side changes.

#### `PowerUI.h / .cpp`
Power tab. Shows MAIN / PROG track power state with ON/OFF buttons.
- `onPowerUpdate(main, prog, join)` — handles global `<p X>` broadcasts.
- `onIndividualPowerUpdate(state, track)` — handles per-track updates.

#### `SettingsUI.h / .cpp`
Settings tab with nested pages: Command Station, Display (theme, brightness, rotation), Touch Calibration, About, Programming Track. Owns a `ProgramUI` instance; exposes `getProgramUI()` so `AppDelegate` can forward CV read/write callbacks.

#### `ProgramUI.h / .cpp`
Programming track CV read/write screen. Launched from within `SettingsUI`. Fires `THROTTLE_PROGRAM_ENTER` / `THROTTLE_PROGRAM_EXIT` settings events, which cause `main.cpp` to destroy and recreate the tabview to free heap while the programming screen is active.

#### `CalibrationUI.h / .cpp`
Step-by-step resistive touch calibration wizard. Saves new `TouchCal` values to `Settings`.

#### `WiFiUI.h / .cpp`
WiFi scan / credential entry overlay. Scans for SSIDs and provides a text-input form.

#### `ConsistUI.h / .cpp`
Consist builder — launched as an overlay from `LocoUI`. Lets the user assemble a CS consist from saved locos.

#### `AboutUI.h / .cpp`
Shows firmware version and a QR code linking to the project page.

#### `lv_port_fs.h / .cpp`
Registers an LVGL filesystem driver backed by the SD card so LVGL image sources starting with `S:` are resolved from SD.

#### `logo.c / logo.h`
LVGL image descriptor for the boot splash logo (generated by `tools/gen_logo.py`).

---

### Fonts & Icons

| File | Description |
|------|-------------|
| `src/fonts/fa_icons_18.c` | LVGL compressed font file — Font Awesome 6 Solid glyphs at 18 px. Used for header icons and function buttons |
| `src/fonts/fa-solid-900.ttf` | Source TTF (not flashed; used to regenerate `fa_icons_18.c`) |
| `src/icons/dcc_icon.c/.h` | Boot icon bitmap (C array) |
| `src/ui/fa_gauge_high_16.c` | Single compiled glyph — the gauge icon at 16 px |
| `src/ui/logo.c/.h` | Boot splash image descriptor |
| `data/header/*.bmp` | BMP images for the header bar status icons (battery, WiFi, CS, menu…) |

---

## Web Interface

The web UI is a **Vue 3** (CDN global build) single-page app served from `WebsiteFS`. Bootstrap 5 provides base styling. Font Awesome 6 Solid is served as a subset `woff2` font containing only the icons actually used.

### Static Assets — `data/www/`

| File | Purpose |
|------|---------|
| `index.html` | Shell: CSS custom properties (dark theme palette), Bootstrap 5 overrides, Vue app mount |
| `vue.global.prod.js` | Vue 3 runtime (CDN copy, served locally) |
| `bs.min.css` | Bootstrap 5 CSS |
| `bs.icons.svg` | Bootstrap Icons SVG sprite (used for non-FA icons in the UI) |
| `fa-icons.css` | Font Awesome class definitions (`.fa-solid`, glyph map) |
| `fa-icons.js` | JS export: `FA_ICONS` array, `FA_BY_CP` lookup, `FA_LV_RANGE` — the subset of icons available on the device LVGL font. Shared between the web icon picker and the fn.editor |
| `fa-solid-subset.woff2` | Subset Web font with only the icons in `FA_LV_RANGE` |
| `fn-defaults.js` | Exports `FN_DEFAULTS` — pre-built named function-button layouts (Diesel, Steam, etc.) |
| `fn.editor.js` | `FnEditor` Vue component — the full function-button page layout editor |
| `fn.editor.css` | Styles for `FnEditor` |
| `locos.js` | `Modal` Vue component — per-loco editor (address, name, function set picker) |
| `functions.js` | `Modal` Vue component — named function-set editor |
| `consists.js` | Vue component — consist editor |
| `groups.js` | Vue component — loco group editor |
| `settings.js` | Vue component — device settings form |
| `icons.js` | Vue component — custom icon (BMP) manager |
| `utils.js` | Shared helpers: `rand()`, `elementIndex()` |
| `not_programming.html` | Standalone page shown when the device is in programming mode and the normal UI is unavailable |

#### `fn.editor.js` in detail
`FnEditor` is a self-contained Vue 3 component that renders and edits a function-button layout. A layout is a 2-D array: **pages × slots**. Each slot is `{fn, latching, btn: {idle, pressed}}` where `btn` carries `{label, color, fill, border, icon}`. Colors are stored as RGB565 integers (to match LVGL's native color format). The icon field is a Font Awesome Unicode codepoint string.

The editor provides:
- Add / remove / reorder pages and slots via drag-drop.
- Per-button colour pickers (a fixed palette of RGB565-safe colours).
- Icon picker filtered to the LVGL-available glyph range (`FA_LV_RANGE`).
- Import from a `FN_DEFAULTS` named preset.

#### `fn-defaults.js`
A static list of common loco function-button presets (e.g., Headlight, Bell, Horn, Mute, Fan, Compressor…). Each entry is a named object with a `functions` array in the same schema as `fn.editor.js`. Referenced by both `functions.js` and `locos.js` to offer "start from a preset" workflows.

---

### REST API

All API routes are handled by `ThrottleAPIHandler` in `ThrottleServer.cpp`. The base path prefix `/$` is accepted as an alias for `/` to allow direct-device and proxied access.

| Method | Path pattern | Description |
|--------|-------------|-------------|
| `GET` | `/locos` | List loco files (`[{file, name}]`) |
| `GET` | `/locos/{n}.json` | Fetch single loco config |
| `PUT` | `/locos/{n}.json` | Save loco config |
| `DELETE` | `/locos/{n}.json` | Delete loco |
| `HEAD` | `/locos/{n}.json` | Check if loco exists |
| `GET` | `/fns` | List function-set files |
| `GET` | `/fns/{n}.json` | Fetch function set |
| `PUT` | `/fns/{n}.json` | Save function set |
| `DELETE` | `/fns/{n}.json` | Delete function set |
| `GET/PUT/DELETE` | `/icons/{n}.bmp` | Custom loco icon (16-bit BMP) |
| `GET/PUT/DELETE/HEAD` | `/consists/{n}.json` | Consist configs |
| `GET/PUT` | `/groups.json` | Loco group definitions |
| `GET/PUT` | `/groups.bmp` | Group icon |
| `GET` | `/` (and all other paths) | Static files from `WebsiteFS` |

Settings-related API routes (WiFi scan, save settings, reboot, etc.) are handled by a separate handler inside `SettingsUI.cpp` via a pointer to `ThrottleServer`.

---

### Data Files

#### `data/default.json`
The built-in fallback function-button layout, referenced as `"/default.json"` from a loco config's `functions` field. Contains two pages of four buttons each (F0–F7), white-on-black, no icons.

#### `sd/locos/{n}.json` schema
```json
{
  "name": "My Loco",
  "functions": [ ... ]   // array of pages, or "/default.json", or "builtin:N"
}
```
`functions` can be:
- `"/default.json"` — use the built-in default layout
- `"builtin:N"` — use built-in preset N from `FN_DEFAULTS`
- A 2-D array inline — custom layout (same schema as `fn.editor.js`)
- A path string like `"/fns/2.json"` — reference a named function set file

#### `sd/consists/{n}.json`
Consist membership list: lead address + follower addresses with direction flags.

#### `sd/groups.json`
Named groups of loco addresses for quick multi-loco selection.

---

## Tooling & Scripts

| Script | Location | Purpose |
|--------|----------|---------|
| `data_gzip.py` | root | PlatformIO extra-script. Walks `data/` and gzip-compresses each file into `data.gz/` before the LittleFS image is built |
| `build_release.py` | root | Post-build extra-script. Collects the compiled firmware `.bin` and LittleFS image into a versioned release archive |
| `upload_website.py` | root | Uploads only the `website` LittleFS partition (avoids full flash for web-only changes) |
| `emulators/cs_emulator.py` | root | Python TCP server that emulates a DCC-EX Command Station. Useful for development without hardware |
| `emulators/throttle_server_emulator.py` | root | Python HTTP server that mimics the throttle REST API |
| `emulators/generate_icon.py` | root | Generates a test BMP icon file |
| `src/scripts/convert_a8.py` | src | Converts a BMP image to an LVGL A8 (alpha-channel-only) C array |
| `src/scripts/invert_bytes.py` | src | Byte-swaps a raw binary file (used when preparing bitmap assets) |
| `tools/gen_logo.py` | root | Converts a PNG to `src/ui/logo.c` — an LVGL image descriptor with the pixel data embedded as a C array |

---

## Coding Guidelines

### Language & Standard
- C++17 (`-std=gnu++17`). Use structured bindings, `std::from_chars`, `if constexpr`, etc. where they improve clarity.
- Arduino framework — `setup()` / `loop()` entry points with FreeRTOS tasks for background work.

### LVGL threading
- **All LVGL calls must be made while holding `lvgl_mutex`.**
- Background FreeRTOS tasks acquire the mutex with `xSemaphoreTake(lvgl_mutex, portMAX_DELAY)` before touching any `lv_obj_t*`.
- LVGL timers and event callbacks run inside `lv_timer_handler()` which is called in `loop()` while the mutex is already held — do not re-acquire it from within a timer callback.

### Object ownership
- UI objects are `std::unique_ptr<*UI>` in `main.cpp`. Reset them (which calls the destructor and runs `lv_obj_del`) before recreating to avoid LVGL object leaks.
- LVGL objects created inside a `*UI` constructor are children of a tab container; they are freed automatically when the parent is deleted.

### Settings events
- Use `Settings.addEventListener(Event::X, lambda)` to react to setting changes rather than polling.
- Lambdas registered this way must be safe to call from any context that calls `Settings.save()` — typically the main loop thread.

### Web UI
- Vue components are defined as plain JS objects (Options API) and registered globally or locally — no build step, no TypeScript.
- Colors passed to the device are **RGB565 integers** (not CSS strings). The `fn.editor.js` `Colors` map translates between `rgb(…)` strings and their RGB565 equivalents.
- The LVGL glyph range for Font Awesome is exported as `FA_LV_RANGE` in `fa-icons.js`; the icon picker must filter to this range so selected icons actually render on-device.

### File naming
- Firmware source: `PascalCase` for class files (`LocoUI.cpp`), `camelCase` for standalone helpers (`lv_port_fs.cpp`).
- Web source: `kebab-case` (`fn.editor.js`, `fn-defaults.js`).
- Data files: numeric IDs for user-created records (`locos/1.json`, `fns/2.json`).

### No-comment default
Only add a comment when the *why* is non-obvious. Method names and variable names should be self-documenting. Existing comments in the codebase follow this rule — do not add boilerplate or JSDoc blocks.

---

## Key Data Flows

### Boot sequence
```
setup()
  ├─ Mount ConfigFS + WebsiteFS (LittleFS)
  ├─ LVGL_CYD::begin() — display driver init
  ├─ XPT2046_Bitbang::begin() — touch init
  ├─ SD.begin() — SD card init
  ├─ lv_port_fs_init() — register SD as LVGL filesystem
  ├─ Settings.load() — read settings.json
  ├─ apply_theme() + event listeners
  ├─ Splash screen (logo)
  ├─ setup_lvgl_layouts() — create header + tabview
  ├─ build_ui_objects() — create LocoUI, AccessoriesUI, PowerUI, SettingsUI
  ├─ WiFi init (STA or AP+STA)
  ├─ xTaskCreate: powerCheck, keepWiFiAlive
  └─ throttleServer.begin()
```

### Throttle speed change (encoder)
```
Encoder ISR → AiEsp32RotaryEncoder internal counter
  └─ LVGL timer (20 ms)
       ├─ rotaryEncoder.encoderChanged() → delta
       ├─ locoUI->nudgeSpeed(-delta * speedStep)
       │    ├─ _loco.speed += delta (clamped 0–126)
       │    ├─ Update speed arc widget
       │    └─ dccexProtocol.setThrottle(address, speed, direction)
       └─ lv_tabview_set_act(0) — switch to Loco tab if on another tab
```

### CS connection lifecycle
```
keepWiFiAlive (FreeRTOS task, core 1, 5 s period)
  ├─ WiFi.begin() if disconnected
  └─ csClient.connect() if WiFi up but CS not connected
       └─ csConnectPending = true

loop() — detects csConnectPending
  ├─ dccexProtocol.connect(&csMonitor) — start protocol
  ├─ dccexProtocol.sendCommand("s") — request server info
  └─ set_header_cs_status(true)

loop() — each iteration
  └─ dccexProtocol.check() → fires AppDelegate callbacks
       ├─ receivedLocoUpdate() → locoUI->onLocoUpdate()
       ├─ receivedTrackPower() → pwrUI->onPowerUpdate()
       └─ receivedTurnoutAction() → accUI->receivedTurnoutAction()
```

### Web UI — saving a loco
```
Browser PUT /locos/1.json  (JSON body)
  └─ ThrottleAPIHandler::handleRequest()
       ├─ Determine storage (ConfigFS or SD)
       ├─ Open file for writing
       ├─ Stream request body → file
       └─ 200 OK

Next GET /locos/1.json (from LocoUI on-device or browser)
  └─ Serve file content
```
