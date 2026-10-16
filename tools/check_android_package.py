"""Check the built APK against its merged manifest before distribution."""
import argparse
from pathlib import PurePosixPath
import tomllib
import xml.etree.ElementTree as ET
import zipfile

ANDROID = "{http://schemas.android.com/apk/res/android}"

def validate(apk, manifest):
    root = ET.parse(manifest).getroot()
    metadata = {n.attrib[ANDROID + "name"]: n.attrib.get(ANDROID + "value", "")
                for n in root.findall("./application/meta-data")}
    rom_file = metadata["gbarecomp.rom_file"]
    with zipfile.ZipFile(apk) as package:
        names = package.namelist()
        configs = [n for n in names if n.startswith("assets/payload/variants/") and n.endswith("/game.toml")]
        if len(configs) != 1:
            raise ValueError("APK must contain exactly one runtime game.toml")
        config = tomllib.loads(package.read(configs[0]).decode("utf-8"))
        actual = PurePosixPath(config["rom"]["path"]).name
        if actual != rom_file:
            raise ValueError(f"Setup imports {rom_file}, but runtime opens {actual}")
        forbidden = [n for n in names if n.lower().endswith((".gba", ".ups", ".bps", ".ips", ".p12", ".keystore"))
                     or n.lower().endswith("gba_bios.bin")]
        if forbidden:
            raise ValueError("Private game or signing files in APK: " + ", ".join(forbidden))
        natives = [n for n in names if n.startswith("lib/") and n.endswith("/libmain.so")]
        if not natives:
            raise ValueError("APK is missing libmain.so")
        for native in natives:
            if b"Java_org_gbarecomp_GbaNative_applyRomPatch" not in package.read(native):
                raise ValueError("Native patch entry point is missing")
    return rom_file

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("apk")
    parser.add_argument("manifest", help="Merged release AndroidManifest.xml")
    args = parser.parse_args()
    print("APK package checks passed:", validate(args.apk, args.manifest))
