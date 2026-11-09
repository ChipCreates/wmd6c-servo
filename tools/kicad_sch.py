"""
kicad_sch.py — generate KiCad 10 schematics from Python descriptions.

Used by tools/gen_servo_board.py and tools/gen_power_board.py to write the DSR-1
Servo Board and Power Board schematics. Every part is placed automatically; each
connected pin gets a short wire stub ending in a net label or power symbol, unused
pins get a no-connect flag. After writing, `verify()` exports the netlist with
kicad-cli and checks that every pin landed on exactly the intended net.

Pin-placement rule (verified against kicad-cli, see docs/handoff/kicad-schematic-handoff.md
section 6.3): library coordinates are Y-up, sheet is Y-down; rotate counter-clockwise
first, then apply (mirror x) = vertical flip / (mirror y) = horizontal flip.
"""
from __future__ import annotations

import copy
import math
import re
import subprocess
import uuid as _uuid
from dataclasses import dataclass, field
from pathlib import Path

STOCK_SYMBOLS = Path("/usr/share/kicad/symbols")
GRID = 1.27
STUB = 2.54
CHAR_W = 1.1          # approx. width of a 1.27 mm label character, for spacing


# --------------------------------------------------------------------------
# S-expression parse / serialise
# --------------------------------------------------------------------------
class Str(str):
    """A quoted string atom."""


def parse(text: str):
    tokens = re.finditer(r'\(|\)|"(?:[^"\\]|\\.)*"|[^\s()"]+', text)
    stack, cur = [], None
    root = []
    cur = root
    for m in tokens:
        tok = m.group(0)
        if tok == "(":
            new = []
            cur.append(new)
            stack.append(cur)
            cur = new
        elif tok == ")":
            cur = stack.pop()
        elif tok.startswith('"'):
            cur.append(Str(tok[1:-1].replace('\\"', '"').replace("\\\\", "\\")))
        else:
            cur.append(tok)
    return root[0]


def _atom(a) -> str:
    if isinstance(a, Str):
        return '"' + a.replace("\\", "\\\\").replace('"', '\\"') + '"'
    if isinstance(a, float):
        return fmt(a)
    return str(a)


def dump(node, indent=0) -> str:
    if not isinstance(node, list):
        return _atom(node)
    simple = all(not isinstance(x, list) for x in node)
    if simple:
        return "(" + " ".join(_atom(x) for x in node) + ")"
    tab = "\t" * (indent + 1)
    head = []
    rest = []
    for x in node:
        if isinstance(x, list) or rest:
            rest.append(x)
        else:
            head.append(x)
    out = "(" + " ".join(_atom(x) for x in head)
    for x in rest:
        out += "\n" + tab + dump(x, indent + 1)
    return out + "\n" + "\t" * indent + ")"


def fmt(v: float) -> str:
    v = round(v, 4)
    s = f"{v:.4f}".rstrip("0").rstrip(".")
    return "0" if s in ("-0", "") else s


def find(node, key):
    return [x for x in node if isinstance(x, list) and x and x[0] == key]


def first(node, key):
    r = find(node, key)
    return r[0] if r else None


def uid() -> Str:
    return Str(str(_uuid.uuid4()))


# --------------------------------------------------------------------------
# Library access and flattening
# --------------------------------------------------------------------------
class Libraries:
    def __init__(self, extra: dict[str, Path] | None = None):
        self.paths = dict(extra or {})
        self._cache: dict[str, dict] = {}

    def _lib(self, nick: str) -> dict:
        if nick not in self._cache:
            path = self.paths.get(nick, STOCK_SYMBOLS / f"{nick}.kicad_sym")
            tree = parse(path.read_text(encoding="utf-8"))
            self._cache[nick] = {s[1]: s for s in find(tree, "symbol")}
        return self._cache[nick]

    def flat(self, lib_id: str):
        """Return a flattened, renamed symbol definition for lib_symbols."""
        nick, name = lib_id.split(":", 1)
        node = self._flatten(nick, name)
        node = copy.deepcopy(node)
        node[1] = Str(lib_id)
        return node

    def _flatten(self, nick, name):
        syms = self._lib(nick)
        node = copy.deepcopy(syms[name])
        ext = first(node, "extends")
        if not ext:
            return node
        parent = self._flatten(nick, ext[1])
        base = copy.deepcopy(parent)
        base[1] = Str(name)
        # rename unit sub-symbols Parent_U_B -> Name_U_B
        pname = ext[1]
        for sub in find(base, "symbol"):
            sub[1] = Str(name + sub[1][len(pname):])
        # child properties override parent ones
        child_props = {p[1]: p for p in find(node, "property")}
        for i, x in enumerate(base):
            if isinstance(x, list) and x and x[0] == "property" and x[1] in child_props:
                base[i] = child_props.pop(x[1])
        insert_at = max(i for i, x in enumerate(base) if isinstance(x, list) and x and x[0] == "property") + 1
        for p in child_props.values():
            base.insert(insert_at, p)
            insert_at += 1
        for key in ("in_bom", "on_board", "exclude_from_sim", "power"):
            c = first(node, key)
            if c:
                for i, x in enumerate(base):
                    if isinstance(x, list) and x and x[0] == key:
                        base[i] = c
        return base


@dataclass
class Pin:
    number: str
    name: str
    etype: str
    x: float
    y: float
    angle: float
    hidden: bool


def symbol_pins(flat, unit=1) -> list[Pin]:
    pins = []
    for sub in find(flat, "symbol"):
        m = re.search(r"_(\d+)_(\d+)$", sub[1])
        if not m:
            continue
        u, body = int(m.group(1)), int(m.group(2))
        if u not in (0, unit) or body not in (0, 1):
            continue
        for p in find(sub, "pin"):
            at = first(p, "at")
            hidden = any(isinstance(x, list) and x[:2] == ["hide", "yes"] for x in p) or "hide" in p
            pins.append(Pin(first(p, "number")[1], first(p, "name")[1], p[1],
                            float(at[1]), float(at[2]), float(at[3]) if len(at) > 3 else 0.0, hidden))
    return pins


def symbol_bbox(flat, unit=1):
    xs, ys = [], []

    def walk(n):
        if not isinstance(n, list):
            return
        if n and n[0] in ("xy", "start", "end", "center", "mid") and len(n) >= 3:
            try:
                xs.append(float(n[1])); ys.append(float(n[2]))
            except ValueError:
                pass
        if n and n[0] == "circle":
            c, r = first(n, "center"), first(n, "radius")
            if c and r:
                xs.extend([float(c[1]) - float(r[1]), float(c[1]) + float(r[1])])
                ys.extend([float(c[2]) - float(r[1]), float(c[2]) + float(r[1])])
        for x in n:
            walk(x)

    for sub in find(flat, "symbol"):
        walk([x for x in sub if not (isinstance(x, list) and x and x[0] == "pin")])
    for p in symbol_pins(flat, unit):
        xs.append(p.x); ys.append(p.y)
    if not xs:
        return (-2.54, -2.54, 2.54, 2.54)
    return (min(xs), min(ys), max(xs), max(ys))


def transform(rot, mirror, x, y):
    """Library (Y-up) vector -> sheet (Y-down) vector, rotation then mirror."""
    x, y = x, -y
    r = math.radians(rot)
    x, y = x * math.cos(r) + y * math.sin(r), -x * math.sin(r) + y * math.cos(r)
    if mirror == "x":
        y = -y
    elif mirror == "y":
        x = -x
    return x, y


def snap(v, g=GRID):
    return round(round(v / g) * g, 4)


# --------------------------------------------------------------------------
# Schematic model
# --------------------------------------------------------------------------
POWER_DIR = {"down": 0}  # placeholder; computed per symbol below


@dataclass
class Part:
    ref: str
    lib_id: str
    value: str
    nets: dict            # pin number -> net name, or None for no-connect
    rot: float = 0
    mirror: str | None = None
    props: dict = field(default_factory=dict)
    footprint: str | None = None   # None = keep library default
    x: float = 0
    y: float = 0
    dnp: bool = False


class Sheet:
    def __init__(self, project, root_uuid, libs: Libraries, power_nets: dict,
                 title_block: dict, paper="D"):
        """power_nets: net name -> lib_id of the power symbol to use for it."""
        self.project = project
        self.root = root_uuid
        self.libs = libs
        self.power_nets = power_nets
        self.title_block = title_block
        self.paper = paper
        self.parts: list[Part] = []
        self.items: list = []
        self.lib_cache: dict[str, list] = {}
        self.pwr_n = 0
        self.expected: dict[tuple, str] = {}

    # ---- library cache --------------------------------------------------
    def flat(self, lib_id):
        if lib_id not in self.lib_cache:
            self.lib_cache[lib_id] = self.libs.flat(lib_id)
        return self.lib_cache[lib_id]

    # ---- geometry helpers ----------------------------------------------
    def extents(self, part: Part):
        """Sheet-space bbox (relative to origin) including stubs and labels."""
        f = self.flat(part.lib_id)
        x0, y0, x1, y1 = symbol_bbox(f)
        pts = [transform(part.rot, part.mirror, x, y) for x, y in
               ((x0, y0), (x0, y1), (x1, y0), (x1, y1))]
        xs = [p[0] for p in pts]; ys = [p[1] for p in pts]
        for pin in symbol_pins(f):
            net = part.nets.get(pin.number, "__unset__")
            px, py = transform(part.rot, part.mirror, pin.x, pin.y)
            dx, dy = self.out_dir(part, pin)
            if net in (None, "__unset__"):
                xs.append(px); ys.append(py)
                continue
            ln = STUB + (6 if (net in self.power_nets and dx == 0) else len(net) * CHAR_W + 3)
            xs.append(px + dx * ln); ys.append(py + dy * ln)
            xs.append(px + dy * 2); ys.append(py + dx * 2)
        # small parts carry Reference/Value text to their right
        if (max(q[0] for q in pts) - min(q[0] for q in pts)) <= 6:
            tw = max(len(part.ref), len(part.value)) * CHAR_W + 2.54
            xs.append(max(q[0] for q in pts) + tw)
        if "Conn_" in part.lib_id:
            ys.append(min(q[1] for q in pts) - 7.62)
            xs.append(min(q[0] for q in pts) + len(part.value) * CHAR_W)
        return min(xs) - 1.27, min(ys) - 1.27, max(xs) + 1.27, max(ys) + 6   # room for value text

    def out_dir(self, part, pin):
        a = math.radians(pin.angle)
        vx, vy = -math.cos(a), -math.sin(a)          # outward, library Y-up
        dx, dy = transform(part.rot, part.mirror, vx, vy)
        return round(dx), round(dy)

    # ---- placement ------------------------------------------------------
    def place_row(self, parts: list[Part], x0, y0, width, gap=7.62):
        """Pack parts left-to-right inside a zone, wrapping rows. Returns bottom y."""
        x, y, row_h = x0, y0, 0
        for p in parts:
            ex0, ey0, ex1, ey1 = self.extents(p)
            w, h = ex1 - ex0, ey1 - ey0
            if x + w > x0 + width and x > x0:
                x, y, row_h = x0, y + row_h + gap, 0
            p.x = snap(x - ex0, 2.54)
            p.y = snap(y - ey0, 2.54)
            x += w + gap
            row_h = max(row_h, h)
            self.parts.append(p)
        return y + row_h

    # ---- emit -----------------------------------------------------------
    def _power_symbol(self, net, x, y, d):
        lib_id = self.power_nets[net]
        f = self.flat(lib_id)
        # direction of the symbol body away from its pin, library default
        x0, y0, x1, y1 = symbol_bbox(f)
        body_up = (y0 + y1) / 2 > 0          # Y-up library: body above the pin
        want = {(0, -1): "up", (0, 1): "down", (1, 0): "right", (-1, 0): "left"}[d]
        if body_up:
            rot = {"up": 0, "left": 90, "down": 180, "right": 270}[want]
        else:
            rot = {"down": 0, "right": 90, "up": 180, "left": 270}[want]
        self.pwr_n += 1
        ref = f"#PWR{self.pwr_n:03d}"
        if lib_id.endswith("PWR_FLAG"):
            ref = f"#FLG{self.pwr_n:03d}"
        val = "PWR_FLAG" if lib_id.endswith("PWR_FLAG") else net
        self.items.append(self._symbol_node(lib_id, ref, val, x, y, rot, None,
                                            hide_ref=True, pins=symbol_pins(f), footprint=""))

    def _symbol_node(self, lib_id, ref, value, x, y, rot, mirror, hide_ref=False,
                     pins=(), footprint=None, extra_props=None, dnp=False, ref_at=None, val_at=None):
        f = self.flat(lib_id)
        lib_props = {p[1]: p[2] for p in find(f, "property")}
        fp = lib_props.get("Footprint", "") if footprint is None else footprint

        def prop(name, val, px, py, hide=False, size=1.27):
            node = ["property", Str(name), Str(val), ["at", px, py, 0]]
            if hide:
                node.append(["hide", "yes"])
            node.append(["effects", ["font", ["size", size, size]]])
            return node

        node = ["symbol", ["lib_id", Str(lib_id)], ["at", x, y, rot]]
        if mirror:
            node.append(["mirror", mirror])
        node += [["unit", 1], ["body_style", 1], ["exclude_from_sim", "no"],
                 ["in_bom", "no" if ref.startswith("#") else "yes"],
                 ["on_board", "no" if ref.startswith("#") else "yes"],
                 ["in_pos_files", "no" if ref.startswith("#") else "yes"],
                 ["dnp", "yes" if dnp else "no"], ["uuid", uid()]]
        rx, ry = ref_at or (x, y - 3)
        vx, vy = val_at or (x, y + 3)
        node.append(prop("Reference", ref, rx, ry, hide=hide_ref))
        node.append(prop("Value", value, vx, vy, hide=False))
        node.append(prop("Footprint", fp, x, y, hide=True))
        node.append(prop("Datasheet", lib_props.get("Datasheet", ""), x, y, hide=True))
        node.append(prop("Description", lib_props.get("Description", ""), x, y, hide=True))
        for k, v in (extra_props or {}).items():
            node.append(prop(k, v, x, y, hide=True))
        seen = set()
        for p in pins:
            if p.number in seen:
                continue
            seen.add(p.number)
            node.append(["pin", Str(p.number), ["uuid", uid()]])
        node.append(["instances", ["project", Str(self.project),
                                   ["path", Str("/" + self.root), ["reference", Str(ref)], ["unit", 1]]]])
        return node

    def emit_part(self, p: Part):
        f = self.flat(p.lib_id)
        pins = symbol_pins(f)
        x0, y0, x1, y1 = symbol_bbox(f)
        pts = [transform(p.rot, p.mirror, x, y) for x, y in ((x0, y0), (x1, y1), (x0, y1), (x1, y0))]
        top = min(q[1] for q in pts); bot = max(q[1] for q in pts)
        left = min(q[0] for q in pts); right = max(q[0] for q in pts)
        cx = p.x + (left + right) / 2
        wide = (right - left) > 6
        ref_at = (snap(p.x + right + 1.27) if not wide else snap(cx), snap(p.y + top - 1.27))
        val_at = (snap(p.x + right + 1.27) if not wide else snap(cx), snap(p.y + bot + 2.54))
        if not wide:
            ref_at = (snap(p.x + right + 1.27), snap(p.y - 1.27))
            val_at = (snap(p.x + right + 1.27), snap(p.y + 1.27))
        if "Conn_" in p.lib_id:          # connectors: text above, clear of pin stubs
            ref_at = (snap(p.x + left), snap(p.y + top - 5.08))
            val_at = (snap(p.x + left), snap(p.y + top - 2.54))
        node = self._symbol_node(p.lib_id, p.ref, p.value, p.x, p.y, p.rot, p.mirror,
                                 hide_ref=p.ref.startswith("#"),
                                 pins=pins, footprint=p.footprint, extra_props=p.props,
                                 dnp=p.dnp, ref_at=ref_at, val_at=val_at)
        # left-justify short labels next to small parts
        if not wide:
            for prop_ in find(node, "property")[:2]:
                eff = first(prop_, "effects")
                eff.append(["justify", "left"])
        self.items.append(node)
        done = {}
        unknown = [pin.number for pin in pins if pin.number not in p.nets and pin.etype != "no_connect"]
        if unknown:
            raise ValueError(f"{p.ref} ({p.lib_id}): no net given for pins {unknown}")
        for pin in pins:
            net = p.nets.get(pin.number)
            px, py = transform(p.rot, p.mirror, pin.x, pin.y)
            ax, ay = round(p.x + px, 4), round(p.y + py, 4)
            key = (ax, ay)
            if key in done:
                if done[key] != net:
                    raise ValueError(f"{p.ref}: stacked pins at {key} on different nets")
                if net is not None:
                    self.expected[(p.ref, pin.number)] = net
                continue
            done[key] = net
            if net is None:
                self.items.append(["no_connect", ["at", ax, ay], ["uuid", uid()]])
                continue
            self.expected[(p.ref, pin.number)] = net
            dx, dy = self.out_dir(p, pin)
            ex, ey = round(ax + dx * STUB, 4), round(ay + dy * STUB, 4)
            self.items.append(["wire", ["pts", ["xy", ax, ay], ["xy", ex, ey]],
                               ["stroke", ["width", 0], ["type", "default"]], ["uuid", uid()]])
            if net in self.power_nets and dx == 0:
                self._power_symbol(net, ex, ey, (dx, dy))
            elif net in self.power_nets:
                # sideways rail pin: a global label joins the same power net (verified
                # with kicad-cli) and avoids rotated symbols crowding adjacent pins
                ang, just = (0, "left") if dx > 0 else (180, "right")
                self.items.append(["global_label", Str(net), ["shape", "passive"],
                                   ["at", ex, ey, ang], ["fields_autoplaced", "yes"],
                                   ["effects", ["font", ["size", 1.27, 1.27]], ["justify", just]],
                                   ["uuid", uid()]])
            else:
                ang, just = {(1, 0): (0, ["justify", "left", "bottom"]),
                             (-1, 0): (180, ["justify", "right", "bottom"]),
                             (0, -1): (90, ["justify", "left", "bottom"]),
                             (0, 1): (270, ["justify", "right", "bottom"])}[(dx, dy)]
                self.items.append(["label", Str(net), ["at", ex, ey, ang],
                                   ["effects", ["font", ["size", 1.27, 1.27]], just],
                                   ["uuid", uid()]])

    def text(self, s, x, y, size=2.54, bold=False):
        font = ["font", ["size", size, size]]
        if bold:
            font.append(["bold", "yes"])
        self.items.append(["text", Str(s), ["exclude_from_sim", "no"], ["at", x, y, 0],
                           ["effects", font, ["justify", "left", "top"]], ["uuid", uid()]])

    def box(self, x0, y0, x1, y1):
        self.items.append(["rectangle", ["start", x0, y0], ["end", x1, y1],
                           ["stroke", ["width", 0.254], ["type", "dash"]],
                           ["fill", ["type", "none"]], ["uuid", uid()]])

    def zone(self, title, x0, y0, width, parts, notes=()):
        """Draw a titled zone, place its parts, return the bottom edge."""
        self.text(title, x0 + 2.54, y0 + 2.54, size=2.54, bold=True)
        ny = y0 + 7.62
        for n in notes:
            self.text(n, x0 + 2.54, ny, size=1.524)
            ny += 2.54
        bottom = self.place_row(parts, x0 + 5.08, ny + 5.08, width - 10.16)
        self.box(x0, y0, x0 + width, bottom + 5.08)
        return bottom + 5.08

    # ---- write ----------------------------------------------------------
    def write(self, path: Path):
        for p in list(self.parts):
            self.emit_part(p)
        tb = ["title_block"] + [[k, Str(v)] for k, v in self.title_block.items() if k != "comments"]
        for i, c in enumerate(self.title_block.get("comments", []), 1):
            tb.append(["comment", i, Str(c)])
        sch = ["kicad_sch", ["version", 20260306], ["generator", Str("eeschema")],
               ["generator_version", Str("10.0")], ["uuid", Str(self.root)],
               ["paper", Str(self.paper)], tb,
               ["lib_symbols"] + list(self.lib_cache.values())]
        sch += self.items
        sch += [["sheet_instances", ["path", Str("/"), ["page", Str("1")]]],
                ["embedded_fonts", "no"]]
        path.write_text(dump(sch) + "\n", encoding="utf-8")

    # ---- verification ---------------------------------------------------
    def verify(self, path: Path, workdir: Path) -> list[str]:
        """Export netlist via kicad-cli; return list of mismatches (empty = OK)."""
        xml = workdir / (path.stem + ".net.xml")
        r = subprocess.run(["kicad-cli", "sch", "export", "netlist", "--format", "kicadxml",
                            "-o", str(xml), str(path)], capture_output=True, text=True)
        if r.returncode != 0:
            return [f"kicad-cli netlist export failed: {r.stderr.strip()}"]
        x = xml.read_text()
        actual = {}
        for name, body in re.findall(r'<net code="\d+" name="([^"]+)"[^>]*>(.*?)</net>', x, re.S):
            for ref, pin in re.findall(r'<node ref="([^"]+)" pin="([^"]+)"', body):
                actual[(ref, pin)] = name.lstrip("/")
        problems = []
        groups: dict[str, set] = {}
        for key, net in self.expected.items():
            if key[0].startswith("#"):      # power/flag symbols are not in the netlist
                continue
            got = actual.get(key)
            if got is None:
                problems.append(f"{key[0]}.{key[1]}: expected {net}, not in netlist")
                continue
            groups.setdefault(net, set()).add(got)
        for net, gots in groups.items():
            if len(gots) != 1:
                problems.append(f"net {net} split across {sorted(gots)}")
        owner = {}
        for net, gots in groups.items():
            for g in gots:
                if g in owner and owner[g] != net:
                    problems.append(f"nets {owner[g]} and {net} merged as {g}")
                owner[g] = net
        return problems


# --------------------------------------------------------------------------
# Simple box symbols for parts missing from the stock libraries
# --------------------------------------------------------------------------
def _font():
    return ["effects", ["font", ["size", 1.27, 1.27]]]


def _lib_prop(name, value, x=0.0, y=0.0, hide=False):
    node = ["property", Str(name), Str(value), ["at", x, y, 0],
            ["show_name", "no"], ["do_not_autoplace", "no"]]
    if hide:
        node.append(["hide", "yes"])
    node.append(_font())
    return node


def box_symbol(name, ref, value, left=(), right=(), top=(), bottom=(), *,
               footprint="", datasheet="", description="", keywords="", body_w=None):
    """Build a rectangular symbol. Each pin entry: (numbers, name, electrical_type);
    numbers is a str or a list of str (stacked pins; extras are hidden)."""
    rows = max(len(left), len(right), 1)
    cols = max(len(top), len(bottom), 0)
    h = (rows + 1) * 2.54
    def longest(side):
        return max([len(e[1]) for e in side] or [0])
    name_w = (longest(left) + longest(right)) * 1.27 + 5.08   # pin names inside the body
    w = body_w or max(10.16, (cols + 1) * 2.54, name_w)
    w = math.ceil(w / 5.08) * 5.08
    h = math.ceil(h / 5.08) * 5.08
    hx, hy = w / 2, h / 2
    pins = []

    def add(entry, x, y, ang):
        nums, pname, etype = entry
        nums = [nums] if isinstance(nums, str) else list(nums)
        for i, n in enumerate(nums):
            # hidden stacked twins are passive, as in the stock libraries: a hidden
            # power_in pin would implicitly join a global net named after the pin
            p = ["pin", etype if i == 0 else "passive", "line", ["at", x, y, ang], ["length", 2.54]]
            if i:
                p.append(["hide", "yes"])
            p += [["name", Str(pname), _font()], ["number", Str(n), _font()]]
            pins.append(p)

    for i, e in enumerate(left):
        add(e, -hx - 2.54, snap(hy - 2.54 * (i + 1), 2.54), 0)
    for i, e in enumerate(right):
        add(e, hx + 2.54, snap(hy - 2.54 * (i + 1), 2.54), 180)
    for i, e in enumerate(top):
        add(e, snap(-hx + 2.54 * (i + 1), 2.54), hy + 2.54, 270)
    for i, e in enumerate(bottom):
        add(e, snap(-hx + 2.54 * (i + 1), 2.54), -hy - 2.54, 90)
    body = ["symbol", Str(f"{name}_0_1"),
            ["rectangle", ["start", -hx, hy], ["end", hx, -hy],
             ["stroke", ["width", 0.254], ["type", "default"]], ["fill", ["type", "background"]]]]
    return ["symbol", Str(name), ["exclude_from_sim", "no"], ["in_bom", "yes"], ["on_board", "yes"],
            ["in_pos_files", "yes"], ["duplicate_pin_numbers_are_jumpers", "no"],
            _lib_prop("Reference", ref, -hx, hy + 1.27),
            _lib_prop("Value", value, hx, hy + 1.27),
            _lib_prop("Footprint", footprint, hide=True),
            _lib_prop("Datasheet", datasheet, hide=True),
            _lib_prop("Description", description, hide=True),
            _lib_prop("ki_keywords", keywords, hide=True),
            body, ["symbol", Str(f"{name}_1_1")] + pins, ["embedded_fonts", "no"]]


def write_symbol_lib(path: Path, symbols: list):
    lib = ["kicad_symbol_lib", ["version", 20251024], ["generator", Str("kicad_symbol_editor")],
           ["generator_version", Str("10.0")]] + symbols
    path.write_text(dump(lib) + "\n", encoding="utf-8")


def write_sym_lib_table(path: Path, entries: dict):
    lines = ["(sym_lib_table", "\t(version 7)"]
    for nick, uri in entries.items():
        lines.append(f'\t(lib (name "{nick}") (type "KiCad") (uri "{uri}") (options "") (descr ""))')
    lines.append(")")
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")
