"""Self-test for tools/kicad_sch.py: place asymmetric symbols in every rotation and
mirror, give every pin a unique net, write the sheet, and confirm with kicad-cli that
each pin landed on exactly its intended net."""
import sys, tempfile, uuid
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent))
from kicad_sch import Libraries, Sheet, Part, symbol_pins

def main():
    libs = Libraries()
    sh = Sheet("selftest", str(uuid.uuid4()), libs,
               {"GND": "power:GND", "+3V3": "power:+3V3"},
               {"title": "kicad_sch self-test"}, paper="A0")
    parts = []
    n = 0
    for lib_id in ("Transistor_BJT:MMBT3904", "Regulator_Switching:MT3608",
                   "Power_Protection:USBLC6-2SC6", "Battery_Management:BQ24074RGT"):
        pins = symbol_pins(libs.flat(lib_id))
        for rot in (0, 90, 180, 270):
            for mir in (None, "x", "y"):
                n += 1
                ref = f"U{n}"
                # stacked pins (same position) must share a net
                pos = {}
                for p in pins:
                    pos.setdefault((p.x, p.y), p.number)
                nets = {p.number: f"N{n}_{pos[(p.x, p.y)]}" for p in pins if p.etype != "no_connect"}
                first_pin = sorted(nets)[0]
                fp = [p for p in pins if p.number == first_pin][0]
                for p in pins:   # exercise power symbols (incl. any stacked twins)
                    if (p.x, p.y) == (fp.x, fp.y) and p.number in nets:
                        nets[p.number] = "GND" if n % 2 else "+3V3"
                parts.append(Part(ref, lib_id, "X", nets, rot=rot, mirror=mir))
    sh.place_row(parts, 20, 20, 1100)
    out = Path(sys.argv[1] if len(sys.argv) > 1 else tempfile.mkdtemp())
    path = out / "selftest.kicad_sch"
    sh.write(path)
    problems = sh.verify(path, out)
    print(f"{len(sh.expected)} pin/net expectations, {len(problems)} problems")
    for p in problems[:20]:
        print("  ", p)
    return 1 if problems else 0

if __name__ == "__main__":
    sys.exit(main())
