#!/usr/bin/env python3
"""Package the data-only widescreen mod (requires Python 3.11+)."""
import argparse
import hashlib
from pathlib import Path
import tomllib
import zipfile


def main():
    repo = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=repo / "release-stage")
    args = parser.parse_args()
    package = repo / "mods/preloaded/packages/pokemon-emerald.enhancement.widescreen/0.2.0"
    manifest = tomllib.loads((package / "manifest.toml").read_text(encoding="utf-8"))
    assert manifest["id"] == package.parent.name and manifest["version"] == package.name
    assert manifest["feature"][0]["default_enabled"] is False
    # This version is metadata for a trusted compiled-in renderer. Keep an
    # explicit file allowlist so local artifacts cannot enter the download.
    names = ["manifest.toml", "README.md", "LICENSE"]
    assert {p.name for p in package.iterdir()} == set(names)
    args.output.mkdir(parents=True, exist_ok=True)
    archive = args.output / f'{manifest["id"]}-{manifest["version"]}.gbamod'
    with zipfile.ZipFile(archive, "w", compression=zipfile.ZIP_DEFLATED) as output:
        for name in names:
            entry = zipfile.ZipInfo(name, date_time=(2026, 9, 20, 0, 0, 0))
            entry.compress_type = zipfile.ZIP_DEFLATED
            output.writestr(entry, (package / name).read_bytes())
    with zipfile.ZipFile(archive) as check:
        assert check.testzip() is None and set(check.namelist()) == set(names)
        for name in names:
            assert check.read(name) == (package / name).read_bytes()
    print(f"{archive.name}: {archive.stat().st_size} bytes, SHA256 {hashlib.sha256(archive.read_bytes()).hexdigest()}")


if __name__ == "__main__":
    main()
