#!/usr/bin/env python3
"""Add or update a non-Steam shortcut without a Steam client GUI.

Steam stores shortcuts in
  <Steam>/userdata/<account-id>/config/shortcuts.vdf
as Valve binary VDF.  This tool preserves existing entries, computes the
shortcut app id from the quoted executable path plus the app name, writes the
file atomically, and optionally installs grid artwork.

Examples:
  add_steam_shortcut.py --exe "$HOME/.local/bin/biko3" --name '尾行3'
  add_steam_shortcut.py --exe ... --name ... --artwork cover.png --hero hero.png
"""
from __future__ import annotations

import argparse
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import zlib
from pathlib import Path

TYPE_OBJECT = 0
TYPE_STRING = 1
TYPE_INT32 = 2
TYPE_FLOAT32 = 3
TYPE_UINT64 = 7
TYPE_END = 8

STEAM64_BASE = 76561197960265728


def cstr(text: str) -> bytes:
    return text.encode("utf-8") + b"\0"


def read_cstr(data: bytes, pos: int) -> tuple[str, int]:
    end = data.index(0, pos)
    return data[pos:end].decode("utf-8", "replace"), end + 1


def parse_object(data: bytes, pos: int) -> tuple[dict, int]:
    result: dict = {}
    while True:
        if pos >= len(data):
            raise ValueError("unexpected end of binary VDF")
        kind = data[pos]
        pos += 1
        if kind == TYPE_END:
            return result, pos
        key, pos = read_cstr(data, pos)
        if kind == TYPE_OBJECT:
            value, pos = parse_object(data, pos)
        elif kind == TYPE_STRING:
            value, pos = read_cstr(data, pos)
        elif kind == TYPE_INT32:
            value = struct.unpack_from("<i", data, pos)[0]
            pos += 4
        elif kind == TYPE_FLOAT32:
            value = struct.unpack_from("<f", data, pos)[0]
            pos += 4
        elif kind == TYPE_UINT64:
            value = struct.unpack_from("<Q", data, pos)[0]
            pos += 8
        else:
            raise ValueError(f"unsupported binary VDF type {kind} at {pos - 1}")
        result[key] = value


def parse_shortcuts(data: bytes) -> dict:
    kind = data[0]
    if kind != TYPE_OBJECT:
        raise ValueError("shortcuts.vdf does not start with an object")
    key, pos = read_cstr(data, 1)
    if key != "shortcuts":
        raise ValueError(f"unexpected root object {key!r}")
    entries, _ = parse_object(data, pos)
    return entries


def encode_value(key: str, value) -> bytes:
    if isinstance(value, dict):
        return b"\0" + cstr(key) + encode_object(value) + b"\x08"
    if isinstance(value, str):
        return b"\x01" + cstr(key) + cstr(value)
    if isinstance(value, bool):
        return b"\x02" + cstr(key) + struct.pack("<i", 1 if value else 0)
    if isinstance(value, int):
        return b"\x02" + cstr(key) + struct.pack("<i", value)
    raise ValueError(f"unsupported value for {key!r}: {type(value)}")


def encode_object(values: dict) -> bytes:
    return b"".join(encode_value(key, value) for key, value in values.items())


def encode_shortcuts(entries: dict) -> bytes:
    body = b"".join(
        b"\0" + cstr(order) + encode_object(entry) + b"\x08"
        for order, entry in entries.items()
    )
    return b"\0" + cstr("shortcuts") + body + b"\x08\x08"


def steam_root() -> Path:
    candidates = []
    if os.environ.get("STEAM_ROOT"):
        candidates.append(Path(os.environ["STEAM_ROOT"]))
    candidates += [
        Path.home() / ".local/share/Steam",
        Path.home() / ".steam/steam",
        Path.home() / ".steam/root",
    ]
    for path in candidates:
        if (path / "userdata").is_dir():
            return path.resolve()
    raise SystemExit("找不到 Steam 目录，请用 --steam-root 指定")


def active_userdata(root: Path, override: str | None) -> Path:
    if override:
        user = root / "userdata" / override
        if user.is_dir():
            return user
        raise SystemExit(f"Steam userdata 目录不存在: {user}")

    login = root / "config/loginusers.vdf"
    if login.is_file():
        text = login.read_text(encoding="utf-8", errors="replace")
        best = None
        for match in re.finditer(r'"(\d{17})"\s*\{(.*?)\}', text, re.S):
            steam64 = int(match.group(1))
            block = match.group(2)
            auto = re.search(r'"AutoLogin"\s+"([01])"', block)
            stamp = re.search(r'"timestamp"\s+"(\d+)"', block)
            score = (1 if auto and auto.group(1) == "1" else 0,
                     int(stamp.group(1)) if stamp else 0)
            if best is None or score > best[0]:
                best = (score, steam64)
        if best:
            account = best[1] - STEAM64_BASE
            user = root / "userdata" / str(account)
            if user.is_dir():
                return user

    # Fallback: the userdata tree touched most recently by Steam.
    dirs = [p for p in (root / "userdata").iterdir() if p.name.isdigit()]
    if not dirs:
        raise SystemExit("没有可用的 Steam 用户，请先登录 Steam")
    return max(dirs, key=lambda p: (p / "config/localconfig.vdf").stat().st_mtime
               if (p / "config/localconfig.vdf").exists() else p.stat().st_mtime)


def steam_running(root: Path) -> bool:
    if shutil.which("pgrep"):
        if subprocess.run(["pgrep", "-x", "steam"],
                          stdout=subprocess.DEVNULL,
                          stderr=subprocess.DEVNULL).returncode == 0:
            return True
    pid_file = root / "steam.pid"
    try:
        pid = int(pid_file.read_text().strip())
    except (OSError, ValueError):
        return False
    try:
        os.kill(pid, 0)
    except OSError:
        return False
    return True


def entry_name(entry: dict) -> str | None:
    """Steam writes `appname`; older tools write `AppName`."""
    for key, value in entry.items():
        if key.lower() == "appname" and isinstance(value, str):
            return value
    return None


def install_artwork(grid: Path, appid: int, artwork: Path | None,
                    hero: Path | None, icon: Path | None) -> list[Path]:
    grid.mkdir(parents=True, exist_ok=True)
    written: list[Path] = []
    if artwork and artwork.is_file():
        target = grid / f"{appid}.png"
        shutil.copy2(artwork, target)
        written.append(target)
    if hero and hero.is_file():
        target = grid / f"{appid}_hero.png"
        shutil.copy2(hero, target)
        written.append(target)
    if icon and icon.is_file():
        target = grid / f"{appid}_icon.png"
        if shutil.which("ffmpeg"):
            subprocess.run(
                ["ffmpeg", "-loglevel", "error", "-y", "-i", str(icon),
                 "-vf", "scale=64:64", str(target)],
                check=False,
            )
        else:
            shutil.copy2(icon, target)
        if target.exists():
            written.append(target)
    return written


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, help="launcher/binary path")
    parser.add_argument("--name", default="尾行3")
    parser.add_argument("--icon", default="",
                        help="icon path stored in the shortcut")
    parser.add_argument("--steam-root", default=None)
    parser.add_argument("--userdata", default=None,
                        help="Steam account id, e.g. 1127854643")
    parser.add_argument("--artwork", default=None, help="600x900 grid cover")
    parser.add_argument("--hero", default=None, help="1920x620 hero image")
    parser.add_argument("--icon-image", default=None,
                        help="image resized to 64x64 for the Steam icon")
    parser.add_argument("--allow-running-steam", action="store_true",
                        help="write even if Steam is running (may be lost)")
    args = parser.parse_args()

    root = steam_root()
    if steam_running(root) and not args.allow_running_steam:
        raise SystemExit("Steam 正在运行：请先完全退出 Steam 再运行本脚本。")

    user = active_userdata(root, args.userdata)
    config = user / "config"
    config.mkdir(parents=True, exist_ok=True)
    vdf = config / "shortcuts.vdf"

    entries: dict = {}
    if vdf.is_file() and vdf.stat().st_size:
        try:
            entries = parse_shortcuts(vdf.read_bytes())
        except Exception as exc:  # keep a copy, then start from scratch
            backup = vdf.with_suffix(".vdf.broken")
            shutil.copy2(vdf, backup)
            print(f"warning: 无法解析 {vdf} ({exc}); 原文件备份到 {backup}",
                  file=sys.stderr)
            entries = {}

    exe = str(Path(args.exe).expanduser().resolve())
    quoted_exe = f'"{exe}"'
    appid = (zlib.crc32((quoted_exe + args.name).encode("utf-8"))
             & 0xFFFFFFFF) | 0x80000000

    entry = {
        "appid": appid - 0x100000000 if appid >= 0x80000000 else appid,
        "AppName": args.name,
        "Exe": quoted_exe,
        "StartDir": f'"{Path(exe).parent}"',
        "icon": args.icon,
        "ShortcutPath": "",
        "LaunchOptions": "",
        "IsHidden": 0,
        "AllowDesktopConfig": 1,
        "AllowOverlay": 1,
        "openvr": 0,
        "Devkit": 0,
        "DevkitGameID": "",
        "DevkitOverrideAppID": 0,
        "LastPlayTime": 0,
        "FlatpakAppID": "",
        "tags": {},
    }

    # Update in place when the same display name already exists (Steam may
    # have rewritten the entry with lowercase `appname` and an Armada launch
    # wrapper), and remove duplicate entries created by older revisions.
    matches = [order for order, old in entries.items()
               if isinstance(old, dict) and entry_name(old) == args.name]
    if matches:
        first = matches[0]
        old = entries[first]
        old["Exe"] = quoted_exe
        old["StartDir"] = f'"{Path(exe).parent}"'
        if args.icon:
            old["icon"] = args.icon
        if not any(key.lower() == "appname" for key in old):
            old["AppName"] = args.name
        if "appid" not in old:
            old["appid"] = entry["appid"]
        # Keep Steam-managed LaunchOptions/LastPlayTime/sortas and unknown keys.
        entries[first] = old
        for order in matches[1:]:
            del entries[order]
    else:
        entries[str(len(entries))] = entry

    # Keep the original order but close gaps left by removed duplicates.
    ordered = sorted(
        entries.items(),
        key=lambda item: int(item[0]) if item[0].isdigit() else 1 << 30,
    )
    entries = {str(index): value for index, (_, value) in enumerate(ordered)}
    data = encode_shortcuts(entries)
    fd, tmp_name = tempfile.mkstemp(prefix="shortcuts.", suffix=".vdf",
                                    dir=str(config))
    try:
        with os.fdopen(fd, "wb") as stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(tmp_name, vdf)
    finally:
        if os.path.exists(tmp_name):
            os.unlink(tmp_name)

    art = install_artwork(
        config / "grid",
        appid,
        Path(args.artwork).expanduser() if args.artwork else None,
        Path(args.hero).expanduser() if args.hero else None,
        Path(args.icon_image or args.icon).expanduser()
        if (args.icon_image or args.icon) else None,
    )
    print(f"Steam user : {user.name}")
    print(f"shortcut   : {args.name}  appid={appid} (0x{appid:08x})")
    print(f"executable : {exe}")
    print(f"file       : {vdf} ({len(data)} bytes)")
    for path in art:
        print(f"artwork    : {path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
