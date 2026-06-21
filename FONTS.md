# Font Generation Guide

The project uses a subset of icon font glyphs in two forms:

| Font | File | Used by |
|------|------|---------|
| LVGL C font | `src/fonts/fa_icons_18.c` | Device display (ESP32 / LVGL) |
| WOFF2 web font | `data/www/fa-solid-subset.woff2` | Web interface icon picker |

Both fonts must include **exactly the same set of codepoints**, defined in `data/www/fa-icons.js`.

---

## When to regenerate

Regenerate both fonts whenever you add or remove an icon from the `FA_ICONS` array in `data/www/fa-icons.js`.

---

## Step 1 — Get the current codepoint range

The `FA_LV_RANGE` export in `fa-icons.js` generates the full comma-separated hex list automatically. To print it, run:

```bash
node -e "
const { readFileSync } = require('fs');
const src = readFileSync('data/www/fa-icons.js', 'utf8');
const matches = [...src.matchAll(/cp:\s*(0x[0-9a-fA-F]+)/g)];
console.log(matches.map(m => m[1]).join(','));
"
```

Copy the output — you'll need it for both commands below.

---

## Step 2 — Regenerate the device font (`fa_icons_18.c`)

**Requires:** `lv_font_conv` — install with `npm install -g lv_font_conv`

```bash
lv_font_conv \
  --font "src/fonts/<your-icon-font>.otf" \
  --range <PASTE_RANGE_HERE> \
  --size 18 \
  --format lvgl \
  --bpp 2 \
  -o src/fonts/fa_icons_18.c
```

The generated file is compiled into the firmware. Rebuild and flash after regenerating.

---

## Step 3 — Regenerate the web font (`fa-solid-subset.woff2`)

**Requires:** `fonttools` — install with `pip install fonttools brotli`

Convert the codepoint list from `0xXXXX` format to `XXXX,XXXX,...` (strip the `0x` prefix):

```bash
node -e "
const { readFileSync } = require('fs');
const src = readFileSync('data/www/fa-icons.js', 'utf8');
const matches = [...src.matchAll(/cp:\s*0x([0-9a-fA-F]+)/g)];
console.log(matches.map(m => m[1]).join(','));
"
```

Then run pyftsubset with that output:

```bash
python -m fontTools.subset \
  "src/fonts/<your-icon-font>.otf" \
  --unicodes="<PASTE_RANGE_WITHOUT_0x_PREFIX>" \
  --output-file="data/www/fa-solid-subset.woff2" \
  --flavor=woff2 \
  --layout-features=""
```

The generated file is served by the device's web server. Upload the filesystem after regenerating.

---

## Source font file

Both commands reference the source `.otf` font file placed at:

```
src/fonts/<your-icon-font>.otf
```

---

## Quick reference — last generated range

The range used for the most recent build of `fa_icons_18.c`:

```
0xf185,0xf0eb,0xf672,0xf673,0xe019,0xe018,0xe41f,0xe420,0xf0e7,0xf06d,0xe4f1,
0xf06e,0xf070,0xf028,0xf027,0xf026,0xf6a9,0xf0f3,0xf1f6,0xf001,0xf0a1,0xf460,
0xe02d,0xe02e,0xf2ea,0xf2f9,0xf021,0xf074,0xf062,0xf063,0xf060,0xf061,0xf04e,
0xf050,0xf049,0xf04d,0xf140,0xf0c2,0xf256,0xe0c7,0xf633,0xf071,0xf06a,0xf057,
0xf058,0xf05a,0xf807,0xf013,0xf085,0xf863,0xf0ad,0xf6e3,0xf7d9,0xf5f2,0xf066,
0xe465,0xf0b0,0xf622,0xf89b,0xf8ba,0xf275,0xf613,0xe205,0xf614,0xf52f,0xf043,
0xf773,0xf628,0xf627,0xf624,0xf625,0xf626,0xf62d,0xf62c,0xf629,0xf62a,0xf62b,
0xe161,0xe15d,0xe160,0xe15f,0xe15b,0xe15c,0xe15e,0xe162,0xe1e8,0xe1e9,0xe1ea,
0xe1eb,0xf1de,0xf3f1,0xf2cb,0xf2ca,0xf2c8,0xf2c7,0xf769,0xf76b,0xf491,0xe00c,
0xf238,0xf239,0xe453,0xe454,0xe2a2,0xf0c1,0xf127,0xe1cb,0xe1cc,0xf1e6,0xf011,
0xf00c,0xf00d,0xf024,0xf005,0xf023,0xf09c,0xf2dc,0xf72e,0xf760,0xf776,0xf017,
0xf2f2,0xe29e,0xf084,0xf030,0xf015
```
