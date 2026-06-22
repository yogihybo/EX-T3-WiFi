"""
Convert an SVG path to a single-glyph OTF font for use with lv_font_conv.

Reads the first <path> element from the SVG, converts it to a CFF glyph,
and saves a minimal OTF that lv_font_conv can consume.

Usage:
    python tools/svg_to_otf.py <input.svg> <output.otf> [hex_codepoint]

    Default codepoint: E900 (PUA, safe to add to fa_icons_18)

Example:
    python tools/svg_to_otf.py icons/train_icon_new.svg src/fonts/train_icon.otf E900

Then add the glyph to the LVGL font by appending to the existing lv_font_conv command:
    --font src/fonts/train_icon.otf --range 0xe900

Requirements:
    pip install fonttools
"""

import re
import sys
import xml.etree.ElementTree as ET
from fontTools.fontBuilder import FontBuilder
from fontTools.misc.psCharStrings import T2CharString


# ---------------------------------------------------------------------------
# SVG path parser (handles absolute M, C, L, Z only — what this SVG uses)
# ---------------------------------------------------------------------------

def _tokenize(d):
    """Return list of (is_command, value) from an SVG path d string."""
    return re.findall(r'([MmCcLlHhVvZz])|(-?[0-9]*\.?[0-9]+(?:e[-+]?[0-9]+)?)', d)


def parse_svg_d(d):
    """Parse SVG path d string into list of (cmd, [float, ...]) tuples."""
    segments = []
    cmd = None
    args = []
    for is_cmd, val in _tokenize(d):
        if is_cmd:
            if cmd is not None:
                segments.append((cmd, args))
            cmd = is_cmd
            args = []
        elif val:
            args.append(float(val))
    if cmd is not None:
        segments.append((cmd, args))
    return segments


def segments_to_abs_ops(segments):
    """
    Convert parsed SVG segments to absolute (op, args) operations.
    Returns list of ('moveto'|'lineto'|'curveto'|'closepath', ...) tuples.
    """
    ops = []
    cx, cy = 0.0, 0.0

    for cmd, args in segments:
        if cmd == 'M':
            for i in range(0, len(args) - 1, 2):
                op = 'moveto' if i == 0 else 'lineto'
                cx, cy = args[i], args[i + 1]
                ops.append((op, cx, cy))
        elif cmd == 'L':
            for i in range(0, len(args) - 1, 2):
                cx, cy = args[i], args[i + 1]
                ops.append(('lineto', cx, cy))
        elif cmd == 'C':
            for i in range(0, len(args) - 5, 6):
                x1, y1 = args[i],     args[i + 1]
                x2, y2 = args[i + 2], args[i + 3]
                x,  y  = args[i + 4], args[i + 5]
                cx, cy = x, y
                ops.append(('curveto', x1, y1, x2, y2, x, y))
        elif cmd in ('Z', 'z'):
            ops.append(('closepath',))

    return ops


def make_t2_program(ops, advance_width, transform):
    """
    Build T2CharString program list from absolute ops.
    transform(x, y) -> (fx, fy) converts SVG coords to font units.
    Coordinates are made relative (T2 uses relative commands).
    """
    program = [advance_width]   # width before first drawing op (CFF encoding)
    cx, cy = 0, 0               # current point in font coords

    def rel(fx, fy):
        nonlocal cx, cy
        dx, dy = round(fx - cx), round(fy - cy)
        cx, cy = round(fx), round(fy)
        return dx, dy

    for op in ops:
        if op[0] == 'moveto':
            fx, fy = transform(op[1], op[2])
            dx, dy = rel(fx, fy)
            program += [dx, dy, 'rmoveto']

        elif op[0] == 'lineto':
            fx, fy = transform(op[1], op[2])
            dx, dy = rel(fx, fy)
            program += [dx, dy, 'rlineto']

        elif op[0] == 'curveto':
            fx1, fy1 = transform(op[1], op[2])
            fx2, fy2 = transform(op[3], op[4])
            fx,  fy  = transform(op[5], op[6])
            dx1 = round(fx1 - cx);  dy1 = round(fy1 - cy)
            dx2 = round(fx2 - fx1); dy2 = round(fy2 - fy1)
            dx  = round(fx  - fx2); dy  = round(fy  - fy2)
            cx, cy = round(fx), round(fy)
            program += [dx1, dy1, dx2, dy2, dx, dy, 'rrcurveto']

        elif op[0] == 'closepath':
            pass  # CFF closes subpaths implicitly before next rmoveto / endchar

    program.append('endchar')
    return program


# ---------------------------------------------------------------------------
# Font builder
# ---------------------------------------------------------------------------

def make_font(svg_path, out_path, codepoint):
    tree = ET.parse(svg_path)
    root = tree.getroot()

    # viewBox
    vb_raw = root.get('viewBox', '0 0 100 100').split()
    vb_x, vb_y, vb_w, vb_h = map(float, vb_raw)

    # First path element (namespace-agnostic)
    path_elem = root.find('.//{http://www.w3.org/2000/svg}path')
    if path_elem is None:
        path_elem = root.find('.//path')
    if path_elem is None:
        raise ValueError("No <path> element found in SVG")
    d = path_elem.get('d', '')

    # Coordinate system
    UPM       = 1000
    ASCENDER  = 800
    DESCENDER = -200
    scale = UPM / vb_h
    advance_width = round(vb_w * scale)

    def transform(x, y):
        """SVG absolute → font units (Y flipped, baseline at bottom of icon)."""
        fx = (x - vb_x) * scale
        fy = ASCENDER - (y - vb_y) * scale
        return fx, fy

    # Build charstring
    segments = parse_svg_d(d)
    ops      = segments_to_abs_ops(segments)
    program  = make_t2_program(ops, advance_width, transform)

    loco_cs = T2CharString()
    loco_cs.program = program

    notdef_cs = T2CharString()
    notdef_cs.program = [500, 'endchar']

    glyph_name = 'loco'

    fb = FontBuilder(UPM, isTTF=False)
    fb.setupGlyphOrder(['.notdef', glyph_name])
    fb.setupCharacterMap({codepoint: glyph_name})
    fb.setupHorizontalMetrics({
        '.notdef': (500, 0),
        glyph_name: (advance_width, 0),
    })
    fb.setupHorizontalHeader(ascent=ASCENDER, descent=DESCENDER)
    fb.setupOS2(
        sTypoAscender=ASCENDER,
        sTypoDescender=DESCENDER,
        sTypoLineGap=0,
        usWinAscent=ASCENDER,
        usWinDescent=abs(DESCENDER),
        fsType=0,
        fsSelection=0x40,
    )
    fb.setupPost()
    fb.setupHead(unitsPerEm=UPM)
    fb.setupNameTable({'familyName': 'TrainIcon', 'styleName': 'Regular'})
    fb.setupCFF(
        psName='TrainIcon',
        fontInfo={'version': '0.1', 'FullName': 'TrainIcon', 'Weight': 'Regular'},
        charStringsDict={'.notdef': notdef_cs, glyph_name: loco_cs},
        privateDict={'defaultWidthX': 0, 'nominalWidthX': 0},
    )

    fb.font.save(out_path)
    print(f"Saved {out_path}")
    print(f"  Codepoint : U+{codepoint:04X}  (UTF-8: \\x{codepoint >> 12 | 0xE0:02X}\\x{(codepoint >> 6 & 0x3F) | 0x80:02X}\\x{(codepoint & 0x3F) | 0x80:02X})")
    print(f"  Advance   : {advance_width} / {UPM} units")
    print(f"  Glyphs    : {len(ops)} ops from SVG path")
    print()
    print("Next step — regenerate fa_icons_18.c by appending to the existing lv_font_conv command:")
    print(f"  --font src/fonts/train_icon.otf --range 0x{codepoint:x}")


if __name__ == '__main__':
    svg_in  = sys.argv[1] if len(sys.argv) > 1 else 'icons/train_icon_new.svg'
    otf_out = sys.argv[2] if len(sys.argv) > 2 else 'src/fonts/train_icon.otf'
    cp      = int(sys.argv[3], 16) if len(sys.argv) > 3 else 0xE900
    make_font(svg_in, otf_out, cp)
