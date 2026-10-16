"""Fetch pinned source dependencies without resetting existing user changes."""
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]

def git(folder, *args):
    subprocess.run(["git", "-C", str(folder), *args], check=True)

def prepare(folder, lock):
    if not (folder / ".git").exists():
        subprocess.run(["git", "clone", lock["url"], str(folder)], check=True)
        git(folder, "checkout", "--detach", lock["revision"])
    head = subprocess.check_output(["git", "-C", str(folder), "rev-parse", "HEAD"], text=True).strip()
    if head != lock["revision"]:
        raise SystemExit(f"{folder} is at {head}; expected {lock['revision']}. Preserve local changes before switching.")
    if "patch" in lock:
        patch = ROOT / lock["patch"]
        if hashlib.sha256(patch.read_bytes()).hexdigest() != lock["sha256"]:
            raise SystemExit(f"Patch checksum changed: {patch}")
        applied = subprocess.run(["git", "-C", str(folder), "apply", "--reverse", "--check", str(patch)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode == 0
        if not applied:
            git(folder, "apply", "--check", str(patch))
            git(folder, "apply", str(patch))
    for name, nested in lock.get("submodules", {}).items():
        prepare(folder / name, nested)

if __name__ == "__main__":
    for name, lock in json.loads((ROOT / "dependencies.lock.json").read_text()).items():
        prepare(ROOT / name, lock)
    print("Pinned source dependencies are ready.")
