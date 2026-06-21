# Theming

The throttle supports **Dark** and **Light** display themes, switchable at runtime from the Settings screen. All colour decisions go through a central palette so changing a token updates every screen that uses it.

---

## Files

| File | Purpose |
|------|---------|
| `src/ui/Theme.h` | `ThemeColor` enum and `tc()` declaration |
| `src/ui/Theme.cpp` | `DARK[]` and `LIGHT[]` palettes; `tc()` implementation |
| `src/ui/LVGL_Layouts.cpp` | `apply_theme()` — wires the LVGL base theme and override callbacks |

---

## How it works

### 1. Palette lookup — `tc()`

```cpp
lv_color_t tc(ThemeColor color);
```

Every UI file calls `tc()` instead of a raw `lv_color_hex()` or `lv_color_make()`. At call time it reads `Settings.theme` and returns the matching colour from the active palette. Because the check is at call time (not construction time), dynamically-rebuilt widgets (WiFi bars, power status dots, direction labels) always pick up the current theme automatically.

### 2. LVGL base theme — `apply_theme()`

`apply_theme()` in `LVGL_Layouts.cpp` is called once at startup and again whenever the user switches theme in Settings.

```cpp
void apply_theme() {
    lv_theme_t* base = lv_theme_default_init(disp,
        lv_color_make(50, 150, 255),   // primary (blue)
        lv_color_make(255, 50, 50),    // secondary (red)
        is_dark,
        &lv_font_montserrat_14);
    // ...attaches override callback
}
```

This re-initialises LVGL's built-in default theme, which automatically recolours buttons, text areas, checkboxes, sliders, and keyboards.

### 3. Override callbacks

LVGL's default theme adds a blue tint to plain `lv_obj_t` containers. The override callbacks strip that tint and enforce the palette's neutral hierarchy:

```cpp
// Applied on every new widget via lv_theme_set_apply_cb()
static void dark_override_cb(lv_theme_t*, lv_obj_t* obj) {
    if (screen)  bg = 0x0a0a0a
    if (panel)   bg = 0x1a1a1a, border = 0x333333
}

static void light_override_cb(lv_theme_t*, lv_obj_t* obj) {
    if (screen)  bg = 0xf0f0f0
    if (panel)   bg = 0xf5f5f5, border = 0xdddddd
}
```

The callbacks only touch `lv_obj_t` class objects (containers/panels), leaving buttons, labels, arcs, and other widget types to the base theme and explicit `tc()` calls.

---

## Colour tokens

Defined in `src/ui/Theme.h` as the `ThemeColor` enum.

### Surfaces

| Token | Dark | Light | Used for |
|-------|------|-------|---------|
| `TC_SURFACE` | `#1e1e1e` | `#f5f5f5` | Card / panel background |
| `TC_SURFACE_RAISED` | `#2e2e2e` | `#ffffff` | Neutral buttons, elevated elements |
| `TC_SURFACE_DEEP` | `#1a1a1a` | `#e8e8e8` | Tab bar, join button, deeper containers |

### Borders

| Token | Dark | Light | Used for |
|-------|------|-------|---------|
| `TC_BORDER` | `#333333` | `#dddddd` | Standard container borders, slider track |
| `TC_BORDER_STRONG` | `#555555` | `#bbbbbb` | Strong dividers, button borders |

### Text

| Token | Dark | Light | Used for |
|-------|------|-------|---------|
| `TC_TEXT_PRIMARY` | `#eeeeee` | `#111111` | Main body text, active direction label |
| `TC_TEXT_SECONDARY` | `#cccccc` | `#444444` | De-emphasised labels, sub-text |
| `TC_TEXT_HINT` | `#888888` | `#666666` | Placeholders, status text, dim labels |
| `TC_TEXT_MUTED` | `#666666` | `#999999` | Inactive direction label, chevrons |

### Accents

| Token | Dark | Light | Used for |
|-------|------|-------|---------|
| `TC_ACCENT` | `#4488ff` | `#2266cc` | Interactive highlight blue |
| `TC_SECTION` | `#26a69a` | `#00897b` | Section headings, panel titles |

### Status indicators

| Token | Dark | Light | Used for |
|-------|------|-------|---------|
| `TC_ICON_ACTIVE` | `#cccccc` | `#333333` | Active WiFi bar, active CS waveform |
| `TC_ICON_INACTIVE` | `#555555` | `#cccccc` | Inactive bar / waveform segment, dots |

### Badges

| Token | Dark | Light | Used for |
|-------|------|-------|---------|
| `TC_BADGE_BG` | `#1a2c3e` | `#e3f0fc` | Info badge background (blue-tinted) |
| `TC_BADGE_FG` | `#64b5f6` | `#1a6cb0` | Info badge foreground text |
| `TC_TEAL_BG` | `#1a3a3a` | `#e0f2f1` | Teal-tinted surface (keypad chips, write button) |

### Overlays / modals

| Token | Dark | Light | Used for |
|-------|------|-------|---------|
| `TC_OVERLAY_BG` | `#1e1e1e` | `#fafafa` | Msgbox / popup background |
| `TC_OVERLAY_BORDER` | `#383838` | `#dddddd` | Msgbox border |
| `TC_OVERLAY_TEXT` | `#aaaaaa` | `#555555` | Msgbox body text |

---

## Adding a new colour

1. Add a new entry to the `ThemeColor` enum in `Theme.h` (before `TC_COUNT`).
2. Add the hex value to both `DARK[]` and `LIGHT[]` in `Theme.cpp`, at the matching index position.
3. Use `tc(YOUR_TOKEN)` in the UI code.

## Switching theme at runtime

The Settings screen writes to `Settings.theme`, saves, then calls:

```cpp
apply_theme();              // re-initialises LVGL base theme + callbacks
create_main_ui();           // rebuilds the full UI so all tc() calls re-evaluate
```

Widgets that update dynamically (e.g. WiFi signal bars, CS status, power dots, direction labels) call `tc()` inside their update functions, so they automatically reflect the current theme without a rebuild.

## Adding a new palette (third theme)

1. Add a new entry to `SettingsClass::Theme` enum in `Settings.h`.
2. Add a third palette array in `Theme.cpp` (e.g. `HICON[]`).
3. Extend the `tc()` switch to select it.
4. Add a third `lv_theme_t*` pointer and override callback in `LVGL_Layouts.cpp`.
