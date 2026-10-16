import sys
from pathlib import Path
import tempfile
import unittest
import zipfile
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from check_android_package import validate

class AndroidPackageTests(unittest.TestCase):
    def build(self, root, rom):
        apk = root / "app.apk"
        with zipfile.ZipFile(apk, "w") as package:
            package.writestr("assets/payload/variants/mother3en/game.toml", '[rom]\npath = "../../roms/' + rom + '"\n')
            package.writestr("lib/arm64-v8a/libmain.so", b"Java_org_gbarecomp_GbaNative_applyRomPatch")
        manifest = root / "manifest.xml"
        manifest.write_text('<manifest xmlns:android="http://schemas.android.com/apk/res/android"><application>'
            '<meta-data android:name="gbarecomp.rom_file" android:value="mother3_en_v13.gba" />'
            '</application></manifest>')
        return apk, manifest

    def test_fresh_install_paths_match(self):
        with tempfile.TemporaryDirectory() as temp:
            self.assertEqual(validate(*self.build(Path(temp), "mother3_en_v13.gba")), "mother3_en_v13.gba")

    def test_rejects_105_wrong_japanese_path(self):
        with tempfile.TemporaryDirectory() as temp:
            with self.assertRaisesRegex(ValueError, "runtime opens mother3_jpn.gba"):
                validate(*self.build(Path(temp), "mother3_jpn.gba"))

    def test_rejects_bundled_rom(self):
        with tempfile.TemporaryDirectory() as temp:
            apk, manifest = self.build(Path(temp), "mother3_en_v13.gba")
            with zipfile.ZipFile(apk, "a") as package:
                package.writestr("assets/payload/roms/private.gba", b"private")
            with self.assertRaisesRegex(ValueError, "Private game"):
                validate(apk, manifest)

if __name__ == "__main__":
    unittest.main()
