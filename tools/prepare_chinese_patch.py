"""Build a read-only BKPT2 Chinese/BKPT patch from original and 3DH data.

The 3DH green release is already patched.  This tool compares its Data tree
with an unmodified Japanese Data tree and keeps only the localization
resources.  Optionally it also folds in the original 3DH ``去马赛克`` Patch.3dh,
producing flags=3 (Chinese + uncensored).  It does not run a Windows executable
and does not modify either input tree.

Output is the standard runtime ``patch.pp``: an inline PP containing
``patch.cfg`` (BKPT, version 2, flags 1 or 3) and zlib-compressed records.
Fonts are merged into the original Type_S/G tables so the runtime keeps
Japanese built-in glyphs while adding the 3DH GBK glyphs.
"""
from __future__ import annotations

import argparse
import configparser
import hashlib
import json
from pathlib import Path
import struct
import sys
import zlib

from bk3_assets import Archive, NEGATE

FONT_START = 0x60004
CHINESE_FLAG = 1
UNCENSORED_FLAG = 2
FONT_NAMES = ("Type_S.FTT", "Type_G.FTT")
IGNORED_NAMES = ("bk3_Red.b3f", "setting.cfg")
UNCENSORED_HASH = "c6cd21d57f81bf6f121534b99dd7aa3f4d47f5850f0b9711250c92fbd779bdf2"
EXPECTED_EXE = {649908: 12970249, 851237: 68585, 855081: 68585,
                858850: 68585, 865783: 68585, 869007: 68585,
                942024: (-1017256565) & 0xffffffff}

Record = tuple[int, int, list[tuple[int, bytes]]]


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1 << 20), b""):
            digest.update(chunk)
    return digest.hexdigest()


def data_dir(root: Path) -> Path:
    candidate = root / "Data"
    return candidate if candidate.is_dir() else root


def font_overlay(original: bytes, chinese: bytes) -> bytes:
    """Add the 3DH bitmap glyphs to a Japanese FTT without dropping originals.

    The original game indexes the two text bytes directly.  Keeping the base
    table preserves built-in dates and labels; only records with a real bitmap
    in the Chinese font are re-pointed to the appended Chinese payload.
    """
    if original[:4] != chinese[:4] or len(chinese) < FONT_START:
        raise ValueError("font cell/header mismatch")
    result = bytearray(original)
    shift = len(original) - FONT_START
    for code in range(32768):
        pos = 4 + code * 12
        offset, pitch, width, height, reserved = struct.unpack_from("<I4H", chinese, pos)
        if pitch and height:
            if offset + pitch * height > len(chinese) - FONT_START:
                raise ValueError("Chinese glyph outside bitmap payload")
            struct.pack_into("<I4H", result, pos, shift + offset, pitch, width, height,
                             reserved)
    result += chinese[FONT_START:]
    return bytes(result)


def write_pp(path: Path, entries: dict[str, bytes]) -> None:
    names = sorted(entries)
    if any(len(name.encode("ascii")) > 31 for name in names):
        raise ValueError("patch entry name exceeds inline PP capacity")
    total = sum(len(entries[name]) for name in names)
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("wb") as stream:
        stream.write(struct.pack("<II", len(names), total))
        for name in names:
            stream.write(name.encode("ascii").ljust(32, b"\0").translate(NEGATE))
        for name in names:
            stream.write(struct.pack("<I", len(entries[name])))
        for name in names:
            stream.write(entries[name].translate(NEGATE))


def block(raw: bytes) -> bytes:
    """Decode one 3DH Patch.3dh block (negated prefix + zlib)."""
    if len(raw) > 31:
        raw = raw[:32].translate(NEGATE) + raw[32:]
    size, = struct.unpack_from("<I", raw)
    result = zlib.decompress(raw[4:])
    if len(result) != size:
        raise ValueError("3DH block length mismatch")
    return result


def add_full(records: dict[str, Record], expected: dict[str, bytes],
             evidence: dict[str, dict], translated: set[str], key: str,
             base: bytes, result: bytes, pack: str, name: str) -> None:
    if base == result:
        return
    key = key.lower()
    records[key] = (len(base), len(result), [(0, result)])
    expected[key] = result
    translated.add(key)
    evidence[key] = {
        "pack": pack,
        "name": name,
        "base_size": len(base),
        "result_size": len(result),
        "base_sha256": sha(base),
        "result_sha256": sha(result),
    }


def merge_uncensored(records: dict[str, Record], expected: dict[str, bytes],
                     evidence: dict[str, dict], translated: set[str],
                     base_data: Path, path: Path) -> tuple[int, list[dict]]:
    """Map 3DH Patch.3dh resource operations onto base archive offsets.

    Executable writes are not applied to the native runtime; they are validated
    as the known Windows-only operations and reported separately.  If a range
    lands on a resource already fully replaced by the Chinese patch, the range
    is applied to the Chinese result instead of the base bytes.
    """
    raw = path.read_bytes()
    tail, = struct.unpack_from("<I", raw)
    index = block(raw[-tail:])

    def payload(slot: int) -> bytes:
        pos, length = struct.unpack_from("<II", index, slot * 8)
        if not length or pos + length > len(raw) - tail:
            raise ValueError("3DH directory entry outside data")
        return block(raw[pos:pos + length])

    config = configparser.ConfigParser(interpolation=None)
    config.read_string(payload(0).decode("gbk"))
    archives: dict[str, Archive] = {}
    exe_operations: list[dict] = []
    operations = 0

    def archive(pack: str) -> Archive:
        if pack not in archives:
            archives[pack] = Archive(base_data / f"{pack}.pp")
        return archives[pack]

    for key, command in config["Start"].items():
        if not key.isdigit() or key == "0":
            continue
        kind, args = command.split(" ", 1)
        value, offset, size, target = args.rsplit(",", 3)
        offset, size = int(offset), int(size)
        if target.lower().endswith(".exe"):
            exe_operations.append({"offset": offset,
                                   "value": int(value) & 0xffffffff})
            continue
        pack = Path(target.rsplit("\\", 1)[-1]).stem.lower()
        a = archive(pack)
        found = [e for e in a.entries
                 if e.offset <= offset and offset + size <= e.offset + e.size]
        if len(found) != 1:
            raise ValueError(f"3DH operation does not map to one resource: {command}")
        entry = found[0]
        if kind == "pth":
            data = payload(int(key) + 8)
        elif kind == "wrt" and size == 4:
            data = struct.pack("<I", int(value) & 0xffffffff)
        else:
            raise ValueError(f"unsupported 3DH operation: {kind}")
        if len(data) != size:
            raise ValueError("3DH replacement size mismatch")
        if a.negated:
            data = data.translate(NEGATE)
        resource_key = f"{pack}.{entry.name}".lower()
        relative = offset - entry.offset
        if resource_key in translated:
            # Chinese full replacement: apply the uncensored range to its result.
            before = a.read(entry)
            result = bytearray(expected[resource_key])
            if len(result) != len(before) or relative + len(data) > len(result):
                raise ValueError(f"uncensored range does not fit Chinese resource: {command}")
            result[relative:relative + len(data)] = data
            records[resource_key] = (len(before), len(result), [(0, bytes(result))])
            expected[resource_key] = bytes(result)
            evidence[resource_key]["result_size"] = len(result)
            evidence[resource_key]["result_sha256"] = sha(expected[resource_key])
        else:
            # Existing uncensored range: append another range to the same record.
            # Do not replace it with a full copy of the resource.
            if resource_key not in expected:
                before = a.read(entry)
                expected[resource_key] = bytes(before)
                records[resource_key] = (len(before), len(before), [])
            result = bytearray(expected[resource_key])
            if relative + len(data) > len(result):
                raise ValueError(f"uncensored range outside resource: {command}")
            result[relative:relative + len(data)] = data
            expected[resource_key] = bytes(result)
            records[resource_key][2].append((relative, data))
            evidence.setdefault(resource_key, {
                "pack": pack,
                "name": entry.name,
                "base_size": len(result),
                "result_size": len(result),
                "base_sha256": sha(a.read(entry)),
                "result_sha256": sha(expected[resource_key]),
            })
            evidence[resource_key]["result_size"] = len(result)
            evidence[resource_key]["result_sha256"] = sha(expected[resource_key])
        operations += 1

    if len(exe_operations) != 21 or any(EXPECTED_EXE.get(x["offset"]) != x["value"]
                                         for x in exe_operations):
        raise ValueError("unexpected executable patch operation")
    return operations, exe_operations


def build(base_root: Path, chinese_root: Path, output: Path, force: bool,
          uncensored: Path | None = None,
          chinese_font: Path | None = None) -> tuple[dict, dict[str, bytes]]:
    base_data = data_dir(base_root)
    chinese_data = data_dir(chinese_root)
    if not base_data.is_dir() or not chinese_data.is_dir():
        raise ValueError("base or Chinese Data directory not found")
    if output.exists() and not force:
        raise ValueError(f"output already exists: {output}")
    if uncensored and sha_file(uncensored) != UNCENSORED_HASH:
        raise ValueError("unsupported uncensored Patch.3dh version")
    if chinese_font and not chinese_font.is_file():
        raise ValueError(f"Chinese font not found: {chinese_font}")

    base_pp = {p.name.lower(): p for p in base_data.glob("*.pp")}
    chinese_pp = {p.name.lower(): p for p in chinese_data.glob("*.pp")}
    if set(base_pp) != set(chinese_pp):
        raise ValueError("base and Chinese .pp sets differ")

    records: dict[str, Record] = {}
    expected: dict[str, bytes] = {}
    evidence: dict[str, dict] = {}
    translated: set[str] = set()
    changed_packs: list[str] = []
    identical_packs: list[str] = []

    for pack_file in sorted(base_pp):
        pack = Path(pack_file).stem
        base_path, chinese_path = base_pp[pack_file], chinese_pp[pack_file]
        same_file = (base_path.stat().st_size == chinese_path.stat().st_size and
                     sha_file(base_path) == sha_file(chinese_path))
        if same_file:
            identical_packs.append(pack)
            continue
        changed_packs.append(pack)
        base_archive, chinese_archive = Archive(base_path), Archive(chinese_path)
        base_entries = {e.name.lower(): e for e in base_archive.entries}
        chinese_entries = {e.name.lower(): e for e in chinese_archive.entries}
        if set(base_entries) != set(chinese_entries):
            raise ValueError(f"resource name sets differ in {pack}")
        for name in sorted(base_entries):
            before = base_archive.read(base_entries[name])
            after = chinese_archive.read(chinese_entries[name])
            if before == after:
                continue
            add_full(records, expected, evidence, translated, f"{pack}.{name}",
                     before, after, pack, chinese_entries[name].name)

    # Fonts are loose resources in Data and are mounted by the runtime as the
    # "fonts" pack.  Merge rather than replacing so Japanese built-ins survive.
    for name in FONT_NAMES:
        base_path = next((p for p in base_data.iterdir()
                          if p.name.lower() == name.lower()), None)
        chinese_path = next((p for p in chinese_data.iterdir()
                             if p.name.lower() == name.lower()), None)
        if not base_path or (not chinese_font and not chinese_path):
            raise ValueError(f"missing {name} in base or Chinese Data")
        before = base_path.read_bytes()
        after = font_overlay(before, (chinese_font or chinese_path).read_bytes())
        add_full(records, expected, evidence, translated, f"fonts.{name}",
                 before, after, "fonts", name)

    ignored = [name for name in IGNORED_NAMES
               if (base_data / name).exists() and (chinese_data / name).exists() and
               (base_data / name).read_bytes() != (chinese_data / name).read_bytes()]

    flags = CHINESE_FLAG
    uncensored_operations = 0
    exe_operations: list[dict] = []
    if uncensored:
        flags |= UNCENSORED_FLAG
        uncensored_operations, exe_operations = merge_uncensored(
            records, expected, evidence, translated, base_data, uncensored)

    entries: dict[str, bytes] = {
        "patch.cfg": b"BKPT" + struct.pack("<II", 2, flags)
    }
    for key, (old_size, new_size, segments) in sorted(records.items()):
        record = struct.pack("<III", old_size, new_size, len(segments))
        record += b"".join(struct.pack("<II", offset, len(data)) + data
                           for offset, data in segments)
        entries[key] = struct.pack("<I", len(record)) + zlib.compress(record, 1)
    write_pp(output, entries)
    output_digest = sha_file(output)
    report = {
        "format": "BKPT2 within inline PP",
        "version": 2,
        "flags": flags,
        "compression": "zlib level 1 per resource",
        "base_data": str(base_data.resolve()),
        "chinese_data": str(chinese_data.resolve()),
        "uncensored_source": str(uncensored.resolve()) if uncensored else None,
        "uncensored_source_sha256": sha_file(uncensored) if uncensored else None,
        "chinese_font": str(chinese_font.resolve()) if chinese_font else None,
        "chinese_font_sha256": sha_file(chinese_font) if chinese_font else None,
        "changed_packs": changed_packs,
        "identical_packs": identical_packs,
        "ignored_non_localization_files": ignored,
        "resources": len(records),
        "uncensored_resource_operations": uncensored_operations,
        "native_executable_equivalents": exe_operations,
        "output": str(output.resolve()),
        "output_bytes": output.stat().st_size,
        "output_sha256": output_digest,
        "entries": evidence,
    }
    return report, expected


def base_resource(base_data: Path, key: str) -> tuple[bytes, int]:
    pack, name = key.split(".", 1)
    if pack.lower() == "fonts":
        path = next(p for p in base_data.iterdir()
                    if p.name.lower() == name.lower())
        data = path.read_bytes()
        return data, len(data)
    archive = Archive(base_data / f"{pack}.pp")
    entry = next(e for e in archive.entries if e.name.lower() == name.lower())
    return archive.read(entry), entry.size


def verify(output: Path, report: dict, expected: dict[str, bytes]) -> None:
    """Decode the generated PP and compare every record with the built result."""
    patch = Archive(output)
    cfg = patch.read(next(e for e in patch.entries if e.name == "patch.cfg"))
    want = b"BKPT" + struct.pack("<II", 2, report["flags"])
    if cfg != want:
        raise ValueError("generated patch.cfg has wrong version/flags")
    checked = 0
    for entry in patch.entries:
        if entry.name == "patch.cfg":
            continue
        raw = patch.read(entry)
        if len(raw) < 4:
            raise ValueError(f"short record: {entry.name}")
        expanded, = struct.unpack_from("<I", raw)
        decoded = zlib.decompress(raw[4:])
        if len(decoded) != expanded or expanded < 12:
            raise ValueError(f"expanded size mismatch: {entry.name}")
        old_size, new_size, count = struct.unpack_from("<III", decoded)
        if not count or count > (len(decoded) - 12) // 8:
            raise ValueError(f"invalid range count: {entry.name}")
        key = entry.name.lower()
        before, base_size = base_resource(Path(report["base_data"]), key)
        if old_size != base_size or len(before) != old_size:
            raise ValueError(f"base size mismatch: {entry.name}")
        cursor = 12
        if count == 1:
            offset, length = struct.unpack_from("<II", decoded, cursor)
            cursor += 8
            if offset == 0 and length == new_size and cursor + length == len(decoded):
                # Full replacement, including size-changing text and fonts.
                result = decoded[cursor:cursor + length]
                cursor += length
            else:
                result = bytearray(before)
                if offset + length > len(result) or cursor + length > len(decoded):
                    raise ValueError(f"range outside resource: {entry.name}")
                result[offset:offset + length] = decoded[cursor:cursor + length]
                cursor += length
                for _ in range(count - 1):
                    offset, length = struct.unpack_from("<II", decoded, cursor)
                    cursor += 8
                    if offset + length > len(result) or cursor + length > len(decoded):
                        raise ValueError(f"range outside resource: {entry.name}")
                    result[offset:offset + length] = decoded[cursor:cursor + length]
                    cursor += length
        else:
            result = bytearray(before)
            for _ in range(count):
                offset, length = struct.unpack_from("<II", decoded, cursor)
                cursor += 8
                if offset + length > len(result) or cursor + length > len(decoded):
                    raise ValueError(f"range outside resource: {entry.name}")
                result[offset:offset + length] = decoded[cursor:cursor + length]
                cursor += length
        if cursor != len(decoded) or len(result) != new_size:
            raise ValueError(f"record size mismatch: {entry.name}")
        if bytes(result) != expected[key]:
            raise ValueError(f"result mismatch: {entry.name}")
        checked += 1
    if checked != report["resources"]:
        raise ValueError("verified resource count does not match report")
    print(f"verified {checked} resources; patch SHA256 {report['output_sha256']}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("base", type=Path, help="original Japanese game root or Data directory")
    parser.add_argument("chinese", type=Path, help="3DH Chinese game root or Data directory")
    parser.add_argument("output", type=Path, help="output patch.pp")
    parser.add_argument("--uncensored", type=Path,
                        help="optional 3DH 去马赛克 Patch.3dh; sets flags=3")
    parser.add_argument("--font", type=Path,
                        help="optional Chinese FTT to use instead of the 3DH font")
    parser.add_argument("--force", action="store_true", help="replace an existing output")
    args = parser.parse_args()
    try:
        report, expected = build(args.base, args.chinese, args.output,
                                 args.force, args.uncensored, args.font)
        verify(args.output, report, expected)
    except (OSError, ValueError, StopIteration, KeyError) as error:
        print(f"error: {error!r}", file=sys.stderr)
        raise SystemExit(1)
    sidecar = args.output.with_suffix(".json")
    sidecar.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n")
    print(f"created {args.output}: {report['resources']} resources, "
          f"flags={report['flags']}, {report['output_bytes']} bytes, "
          f"SHA256 {report['output_sha256']}")
    print(f"metadata: {sidecar}")


if __name__ == "__main__":
    main()
