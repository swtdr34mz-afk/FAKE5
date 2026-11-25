#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# Copyright (C) 2026 swtdr34mz (FAKE5)
"""The MPC skin (Plugin Skins/: TUI.json, Q-Links.json and the pictures) from the renderer's output.

    python3 mpc/skin/make_skin.py <render folder> mpc/plugin/skins

The renderer (render_skin.cpp) draws each MPC screen with the VST3's own art and cuts every control out at every
value; this places each picture where it was cut from, bound to the MPC plugin's parameter:

  knobs      film strips (a frame a value), dragged; a double tap opens the MPC's editor (its list for a choice)
  buttons    a picture for each value; a tap switches it (or steps to the next value: SHAPE)
  LED lists  each row a radio button

The pages (the function keys): OSC, FILTER, LFO, MOD, FX, PLAY; OSC, FILTER and MOD have a second half (the key
pressed again). On each the 16 Q-Links turn up to four groups, the top row of Q-Links the first group and so on down.
Standard library only (Pillow, if there, makes the pictures smaller).
"""
import json
import os
import shutil
import sys

# The MPC plugin's parameters (mpc/plugin/src/Fake5Mpc.cpp): 0 VOLUME, then the program's 137 (dsp/Params.h's order,
# by their VST ids), then VOICES, QUALITY, HOLD.
PARAM_IDS = """o1_oct o1_pitch o1_shape sync o2_oct o2_pitch o2_shape o1_key o2_key o2_bypass
o1_level sub o2_level fm noise cutoff reso drive keyamt e1_del
e1_att e1_dec e1_sus e1_rel e1_amt e1_vel e1_dest e2_del e2_att e2_dec
e2_sus e2_rel e2_amt e2_vel env_routing env_repeat lfo1_freq lfo1_amt lfo1_shape lfo1_sync
lfo1_dest lfo1_reset lfo1_slew lfo2_freq lfo2_amt lfo2_shape lfo2_sync lfo2_dest lfo2_reset lfo2_slew
mod1_src mod1_amt mod1_dst mod2_src mod2_amt mod2_dst mod3_src mod3_amt mod3_dst mod4_src
mod4_amt mod4_dst mod5_src mod5_amt mod5_dst mod6_src mod6_amt mod6_dst mod7_src mod7_amt
mod7_dst mod8_src mod8_amt mod8_dst mod9_src mod9_amt mod9_dst mod10_src mod10_amt mod10_dst
mod11_src mod11_amt mod11_dst mod12_src mod12_amt mod12_dst mod13_src mod13_amt mod13_dst mod14_src
mod14_amt mod14_dst mod15_src mod15_amt mod15_dst mod16_src mod16_amt mod16_dst fx_on fx_type
fx_time fx_mix fx_misc fx_sync rev_on rev_mix rev_damp rev_predelay rev_decay rev_tone
vintage overdrive glide glide_rate glide_mode o1_glide o2_glide unison uni_voices uni_detune
uni_keymode uni_retrig bpm divide arp_on arp_mode arp_oct arp_repeat arp_relatch seq_play
pgm_vol pb_up pb_down split1 split2 split_point spread""".split()
assert len(PARAM_IDS) == 137
NP = len(PARAM_IDS)
GLOBAL = {"vol": 0, "voices": 1 + NP, "quality": 2 + NP, "hold": 3 + NP}
NPARAMS = 4 + NP


def mpc(pid):
    """the MPC index of a page's id"""
    if pid in GLOBAL:
        return GLOBAL[pid]
    return 1 + PARAM_IDS.index(pid)


# The pages (the function keys), each a screen or two (the renderer's names), and each screen's Q-Link groups top down
# (a group as (ids, False): not framed)
PAGES = [
    ("OSC", [("OSC", [["o1_oct", "o1_pitch", "o1_shape", "o1_level"],
                      ["o2_oct", "o2_pitch", "o2_shape", "o2_level"],
                      ["sub", "noise", "vintage", "overdrive"],
                      (["drive", "pgm_vol", "spread"], False)]),
             ("OSC 2", [["glide_rate", "o1_glide", "o2_glide", "glide_mode"],
                        (["pb_up", "pb_down", "split_point"], False),
                        ["uni_voices", "uni_detune", "uni_keymode"]])]),
    ("FILTER", [("FILTER", [(["reso", "keyamt", "e1_amt"], False),
                            (["cutoff", "drive"], False),
                            ["e1_del", "e1_att", "e1_dec", "e1_sus"],
                            (["e1_rel", "e1_dest"], False)]),
                ("FILTER 2", [["e2_del", "e2_att", "e2_dec", "e2_sus"],
                              (["e2_rel", "e2_amt"], False),
                              (["e1_dest"], False)])]),
    ("LFO", [("LFO", [["lfo1_freq", "lfo1_amt", "lfo1_dest", "lfo1_slew"],
                      (["lfo1_shape"], False),
                      ["lfo2_freq", "lfo2_amt", "lfo2_dest", "lfo2_slew"],
                      (["lfo2_shape"], False)])]),
    ("MOD", [("MOD", [(["mod1_amt", "mod2_amt", "mod3_amt", "mod4_amt"], False),
                      (["mod5_amt", "mod6_amt", "mod7_amt", "mod8_amt"], False),
                      (["mod1_dst", "mod2_dst", "mod3_dst", "mod4_dst"], False),
                      (["mod5_dst", "mod6_dst", "mod7_dst", "mod8_dst"], False)]),
             ("MOD 2", [(["mod9_amt", "mod10_amt", "mod11_amt", "mod12_amt"], False),
                        (["mod13_amt", "mod14_amt", "mod15_amt", "mod16_amt"], False),
                        (["mod9_dst", "mod10_dst", "mod11_dst", "mod12_dst"], False),
                        (["mod13_dst", "mod14_dst", "mod15_dst", "mod16_dst"], False)])]),
    ("FX", [("FX", [["fx_type", "fx_time", "fx_mix", "fx_misc"],
                    (["overdrive"], False),
                    ["rev_mix", "rev_damp", "rev_predelay", "rev_decay"],
                    (["rev_tone"], False)])]),
    ("PLAY", [("PLAY", [(["arp_mode", "arp_oct", "arp_repeat", "divide"], False),
                        (["pgm_vol", "vol", "bpm"], False)])]),
]
# Program (Track) mode: the knobs as the MPC's own program knobs, bottom left Q-Link 1
PROGRAM_QLINKS = ["rev_mix", "fx_mix", "lfo1_amt", "vol",
                  "e2_att", "e2_dec", "e2_sus", "e2_rel",
                  "e1_att", "e1_dec", "e1_sus", "e1_rel",
                  "cutoff", "reso", "e1_amt", "drive"]


class O(list):
    """A JSON object kept in order, with repeated keys (the overlays' two "version"s)."""
    def __init__(self, *pairs):
        super().__init__(pairs)


def dump(v, ind=0):
    sp = "  " * ind
    if isinstance(v, O):
        if not v:
            return "{}"
        return "{\n" + ",\n".join("%s  %s: %s" % (sp, json.dumps(k), dump(x, ind + 1)) for k, x in v) + "\n" + sp + "}"
    if isinstance(v, list):
        if not v:
            return "[]"
        if all(not isinstance(x, (O, list)) for x in v):
            return "[" + ", ".join(json.dumps(x) for x in v) + "]"
        return "[\n" + ",\n".join(sp + "  " + dump(x, ind + 1) for x in v) + "\n" + sp + "]"
    return json.dumps(v)


def box(r):
    return "%d %d %d %d" % tuple(r)


def image_data(img):
    return O(("version", 2), ("imageType", "Regular"), ("colour", "0"), ("image", img))


def bounds(r, focus="No", hide_handles=None, show="Show"):
    if hide_handles:
        return O(("version", 2), ("acceptsHWFocus", focus), ("showWhenDataModelInvalid", show), ("whenVisible", "Always"),
                 ("boundsType", "Absolute"), ("bounds", box(r)), ("additionalInvalidatingHandles", hide_handles))
    return O(("version", 1), ("acceptsHWFocus", focus), ("showWhenDataModelInvalid", show), ("whenVisible", "Always"),
             ("boundsType", "Absolute"), ("bounds", box(r)))


def entry(name, typ, data, b, remap=None):
    return O(("version", 2),
             ("componentData", O(("version", 1), ("name", name), ("type", typ), ("data", data))),
             ("handle remapping", O(("version", 1), ("map", [O(("key", k), ("value", v)) for k, v in (remap or [])]))),
             ("bounds", b))


def action(on, handler, handle, extra):
    return O(("version", 1), ("onAction", on), ("handler", handler), ("handleName", handle), ("additionalData", extra))


def component(actions, children, ignore_mouse=False):
    clear = O(("version", 1), ("colour", "0"), ("image", ""))
    return O(("version", 2), ("actions", actions),
             ("backgroundData", O(("version", 1), ("focussed", clear), ("unfocussed", clear))),
             ("ignoreMousePresses", ignore_mouse), ("disableCoarseDataWheel", False), ("componentsData", children))


FOCUS_LOST = O(("version", 1), ("onAction", "Focus Lost"), ("handler", "Focus Lost"), ("handleName", "Focus Lost"),
               ("additionalData", ""))


def overlay(typ, data):
    return component([], [entry("overlay", typ, data, bounds((0, 0, 1280, 800), "Yes"))], ignore_mouse=True)


def range_handle(a, b, steps, h):
    return ("IndexedEnabling/%d/%d/%s" % (a, steps, h)) if a == b else ("RangedEnabling/%d/%d/%d/%s" % (a, b, steps, h))


def main(argv):
    if len(argv) != 3:
        print(__doc__)
        return 2
    src, out = argv[1], argv[2]
    m = json.load(open(os.path.join(src, "manifest.json")))
    pages = {p["name"]: p for p in m["pages"]}
    for p in m["pages"]:
        for c in p["controls"]:
            if c["id"] not in GLOBAL and c["id"] not in PARAM_IDS:
                sys.exit("%s: the page's %s is not one of the MPC plugin's parameters" % (p["name"], c["id"]))

    if os.path.isdir(out):
        shutil.rmtree(out)
    os.makedirs(out)
    used = set()

    def use(f):
        used.add(f)
        return f

    def knob_overlay(bipolar):
        return overlay("KnobOverlay", O(("version", 2), ("version", 1), ("actions", [FOCUS_LOST]), ("overlayType", "Knob With Header"),
                                        ("scaleType", "Linear"), ("polarityType", "Bipolar" if bipolar else "Unipolar"),
                                        ("showMinusSign", True), ("handleName", "Data")))
    defs = [("knob overlay", knob_overlay(False)), ("knob overlay bipolar", knob_overlay(True)),
            ("menu overlay", overlay("MenuOverlay", O(("version", 1), ("version", 1), ("actions", [FOCUS_LOST]),
                                                      ("withNumbers", False), ("handleName", "Data"))))]
    focus = O(("version", 1), ("backgroundColour", "19ffffff"), ("outlineColour", "ffe81f40"), ("backgroundInset", 1.0),
              ("outlineThickness", 1.0))
    names = {}                           # the discrete parameters' names by MPC index (replacementStrings)

    def tab_children(page):
        kids = [entry("panel", "Image", image_data(use(page["background"])), bounds((0, 0, 1280, 628)))]
        boxes, legends = [], []

        for i, c in enumerate(page["controls"]):
            idx = mpc(c["id"])
            if c["names"]:
                names[idx] = c["names"]
            h = "Parameter %d" % idx
            if c["kind"] == "knob":
                fr = c["frame"]
                kx, ky, w, hh = c["rect"]
                ox, oy = kx - fr[0], ky - fr[1]
                ov = "menu overlay" if c["names"] else "knob overlay bipolar" if c["bipolar"] else "knob overlay"
                name = "%s knob %d %s" % (page["name"].lower().replace(" ", ""), i, c["id"])
                knob = O(("version", 3), ("knobType", "FilmStrip"), ("filmStrip", use(c["strip"])), ("numFrames", c["frames"]),
                         ("invert", False), ("dragOrientation", "Vertical"), ("handleName", "Data"))
                defs.append((name, component(
                    [action("Mouse Down", "Q-Link", "Data", ""), action("Double Click", "Show Overlay", "Data", ov),
                     action("Enter Pressed", "Show Overlay", "Data", ov)],
                    [entry("knob", "Knob", knob, bounds((ox, oy, w, hh))),
                     entry("focus", "Focus", focus, O(("version", 1), ("acceptsHWFocus", "No"), ("showWhenDataModelInvalid", "Show"),
                                                      ("whenVisible", "WhenFocussed"), ("boundsType", "Absolute"),
                                                      ("bounds", box((0, 0, fr[2], fr[3])))))])))
                kids.append(entry(c["id"], name, O(("version", 1), ("handleName", "Data")), bounds(fr, "Yes"), [("Data", h)]))
                boxes.append((c["id"], fr))
                legends.append((c["id"], fr))
            elif c["kind"] == "button":
                rs = c["ranges"]
                for r in rs:
                    hide = [range_handle(r["a"], r["b"], c["steps"], h)] if len(rs) > 1 else None
                    data = O(("version", 2), ("onImage", use(r["image"])), ("offImage", r["image"]), ("buttonId", r["next"]),
                             ("numButtonsInGroup", c["steps"]), ("handleName", h), ("gestureBehaviour", "Instant"))
                    kids.append(entry(c["id"], "Button", data, bounds(c["rect"], "No", hide, "Hide")))
                boxes.append((c["id"], c["rect"]))
            else:
                if len(c["rows"]) != c["steps"]:
                    sys.exit("%s: %d LEDs for %d values" % (c["id"], len(c["rows"]), c["steps"]))
                for k, row in enumerate(c["rows"]):
                    data = O(("version", 2), ("onImage", use(row["on"])), ("offImage", use(row["off"])), ("buttonId", k),
                             ("numButtonsInGroup", c["steps"]), ("handleName", h), ("gestureBehaviour", "Instant"))
                    kids.append(entry(c["id"], "Button", data, bounds(row["rect"])))
                    boxes.append((c["id"], row["rect"]))
        return kids, boxes, legends

    def frame(group, boxes, legends):
        """The Q-Link group's frame: around its controls with their legends, with an even gap that cuts no other."""
        mine = [r for p, r in boxes + legends if p in group]
        if not mine:
            return None
        x0, y0 = min(r[0] for r in mine), min(r[1] for r in mine)
        x1, y1 = max(r[0] + r[2] for r in mine), max(r[1] + r[3] for r in mine)

        def cuts(a, b, c, d):
            for _pid, r in boxes:
                ix = min(c, r[0] + r[2]) - max(a, r[0])
                iy = min(d, r[1] + r[3]) - max(b, r[1])
                inside = r[0] >= a and r[1] >= b and r[0] + r[2] <= c and r[1] + r[3] <= d
                if ix > 0 and iy > 0 and not inside:
                    return r
            return None
        while True:
            r = cuts(x0, y0, x1, y1)
            if r is None:
                break
            x0, y0, x1, y1 = min(x0, r[0]), min(y0, r[1]), max(x1, r[0] + r[2]), max(y1, r[1] + r[3])
        for pad in (6, 5, 4, 3, 2, 1, 0):
            a, b, c, d = max(0, x0 - pad), max(0, y0 - pad), min(1280, x1 + pad), min(628, y1 + pad)
            if cuts(a, b, c, d) is None:
                return (a, b, c - a, d - b)
        return (x0, y0, x1 - x0, y1 - y0)

    tabs, qmaps = [], []
    for key, (name, halves) in enumerate(PAGES):
        for sub, (screen, groups) in enumerate(halves):
            page = pages[screen]
            kids, boxes, legends = tab_children(page)
            rects, q = [], {}
            for row, g in enumerate(groups):
                ids, framed = (g, True) if isinstance(g, list) else g
                if framed:
                    r = frame(ids, boxes, legends)
                    if r:
                        rects.append(r)
                for col, pid in enumerate(ids):
                    q[13 + col - 4 * row] = mpc(pid)
            tabs.append(O(("version", 1), ("tabName", screen), ("fnKeyIndex", key), ("fnKeySubIndex", sub),
                          ("componentDefinition", component([], kids)), ("qlinkBoundsData", [box(r) for r in rects])))
            qmaps.append(O(("Tab", key + 1), ("SubTab", sub + 1), ("Bank Direction", "RowTopDown"),
                           ("Q-Links", O(*[("Q-Link %d" % n, q.get(n, -1)) for n in range(1, 17)]))))

    repl = [O(("key", "Parameter %d" % k), ("value", v)) for k, v in sorted(names.items())]
    tui = O(("pageData", O(("version", 1),
                           ("componentDefinitions", O(("version", 2), ("importFiles", []),
                                                      ("localComponentDefinitions", [O(("key", k), ("value", v)) for k, v in defs]))),
                           ("replacementStrings", repl),
                           ("info", O(("version", 1), ("type", "CompleteDescription"))),
                           ("tabs", tabs))))
    ql = O(("version", 4), ("info", O(("version", 1), ("type", "CompleteDescription"))),
           ("Screen Mode Q-Links", O(("version", 4), ("map", qmaps))),
           ("Program Mode Q-Links", O(*[("Q-Link %d" % (n + 1), mpc(pid)) for n, pid in enumerate(PROGRAM_QLINKS)])))
    with open(os.path.join(out, "TUI.json"), "w") as f:
        f.write(dump(tui) + "\n")
    with open(os.path.join(out, "Q-Links.json"), "w") as f:
        f.write(dump(ql) + "\n")
    try:                                 # (256 colours: the pictures a fraction of the size)
        from PIL import Image
    except ImportError:
        Image = None
        print("(no Pillow: the pictures copied as they are, many times larger)")
    for f in sorted(used):
        if Image:
            im = Image.open(os.path.join(src, f)).convert("RGB")
            im.quantize(256, method=Image.MEDIANCUT, dither=Image.NONE).save(os.path.join(out, f), optimize=True)
        else:
            shutil.copy2(os.path.join(src, f), os.path.join(out, f))
    print("TUI.json: %d tabs, %d components; Q-Links.json; %d pictures in %s" % (len(tabs), len(defs), len(used), out))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
