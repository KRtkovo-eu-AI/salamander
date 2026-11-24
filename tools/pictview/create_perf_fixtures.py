#!/usr/bin/env python3
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-21 | Add deterministic synthetic image fixtures. | Measure PictView decoding and EXIF rotation costs. | | GPT-6 (Codex) |
"""Create synthetic PictView performance inputs; these are not real photographs.

Requires Pillow and NumPy. Usage: python create_perf_fixtures.py OUTDIR
Keep OUTDIR outside the repository: the generated images are large binary files.
"""

from __future__ import annotations

import argparse
import hashlib
import io
import json
import platform
import struct
from pathlib import Path

import numpy as np
from PIL import Image, __version__ as pillow_version


SEED = 20261121
JPEG_QUALITY = 92
ORIENTATION_TAG = 274
SIZES = ((3000, 2000), (6000, 4000), (8000, 6000))


def synthetic_pixels(width: int, height: int) -> np.ndarray:
    """Integer gradients, waves, checker detail and seeded noise; no photo input."""
    rng = np.random.Generator(np.random.PCG64(SEED + width * 10000 + height))
    pixels = np.empty((height, width, 3), dtype=np.uint8)
    x = np.arange(width, dtype=np.int32)[None, :]
    for top in range(0, height, 256):
        bottom = min(top + 256, height)
        y = np.arange(top, bottom, dtype=np.int32)[:, None]
        checker = ((x // 47 + y // 61) % 2) * 18
        wave = np.abs(((x * 3 + y * 5) % 256) - 128) // 4
        noise = rng.integers(-20, 21, size=(bottom - top, width, 3), dtype=np.int16)
        channels = (
            30 + x * 170 // width + checker + wave,
            25 + y * 170 // height + checker + wave,
            35 + (x + y) * 155 // (width + height) + checker - wave,
        )
        for channel, base in enumerate(channels):
            pixels[top:bottom, :, channel] = np.clip(base + noise[:, :, channel], 0, 255)
    return pixels


def exif_bytes(orientation: int) -> bytes:
    exif = Image.Exif()
    exif[ORIENTATION_TAG] = orientation
    return exif.tobytes()


def jpeg_with_orientation(jpeg: bytes, orientation: int) -> bytes:
    """Insert only EXIF; all orientation variants retain identical JPEG pixels."""
    if not jpeg.startswith(b"\xff\xd8"):
        raise ValueError("JPEG encoder did not return a JPEG stream")
    payload = exif_bytes(orientation)
    return jpeg[:2] + b"\xff\xe1" + struct.pack(">H", len(payload) + 2) + payload + jpeg[2:]


def record(path: Path, width: int, height: int, orientation: int, **extra: object) -> dict:
    # Validate dimensions and EXIF from the file actually written, not expectations.
    with Image.open(path) as image:
        if image.size != (width, height) or image.getexif().get(ORIENTATION_TAG) != orientation:
            raise ValueError(f"Unexpected dimensions or EXIF orientation in {path.name}")
        image_format = image.format
        image.verify()
    with path.open("rb") as stream:
        file_digest = hashlib.file_digest(stream, "sha256").hexdigest()
    return {
        "file": path.name,
        "synthetic": True,
        "format": image_format,
        "width": width,
        "height": height,
        "megapixels": width * height / 1_000_000,
        "orientation": orientation,
        "size_bytes": path.stat().st_size,
        "sha256": file_digest,
        **extra,
    }


def create_fixtures(output: Path) -> dict:
    output.mkdir(parents=True, exist_ok=True)
    names = {"manifest.json"}
    for width, height in SIZES:
        megapixels = width * height // 1_000_000
        orientations = (1, 3, 6, 8) if megapixels == 24 else (1, 6)
        names.update(f"synthetic_{megapixels}mp_orientation{value}.jpg" for value in orientations)
    names.add("synthetic_24mp_orientation1.png")
    if any((output / name).exists() for name in names):
        raise ValueError("Output already contains fixture files; choose a fresh directory")

    entries = []
    for width, height in SIZES:
        megapixels = width * height // 1_000_000
        pixels = synthetic_pixels(width, height)
        source_digest = hashlib.sha256(memoryview(pixels)).hexdigest()
        with Image.fromarray(pixels) as image:
            encoded = io.BytesIO()
            image.save(encoded, format="JPEG", quality=JPEG_QUALITY, subsampling=0, optimize=False)
            jpeg = encoded.getvalue()
            encoded_digest = hashlib.sha256(jpeg).hexdigest()
            orientations = (1, 3, 6, 8) if megapixels == 24 else (1, 6)
            for orientation in orientations:
                path = output / f"synthetic_{megapixels}mp_orientation{orientation}.jpg"
                path.write_bytes(jpeg_with_orientation(jpeg, orientation))
                entries.append(record(path, width, height, orientation,
                                      source_rgb_sha256=source_digest,
                                      jpeg_without_exif_sha256=encoded_digest))
                print(f"Created {path.name}: {path.stat().st_size} bytes", flush=True)
            if megapixels == 24:
                path = output / "synthetic_24mp_orientation1.png"
                image.save(path, format="PNG", compress_level=1, exif=exif_bytes(1))
                entries.append(record(path, width, height, 1, source_rgb_sha256=source_digest))
                print(f"Created {path.name}: {path.stat().st_size} bytes", flush=True)
        del pixels

    manifest = {
        "schema_version": 1,
        "description": "Deterministic synthetic textures, not real photographs.",
        "seed": SEED,
        "generator": "NumPy PCG64 with integer texture patterns",
        "versions": {"python": platform.python_version(), "numpy": np.__version__, "pillow": pillow_version},
        "generator_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "jpeg": {"quality": JPEG_QUALITY, "subsampling": "4:4:4", "optimize": False},
        "png": {"compress_level": 1},
        "orientation_notes": "1=normal, 3=180 degrees, 6=90 degrees clockwise, 8=90 degrees counterclockwise. Stored raster dimensions do not change.",
        "comparison_notes": "Each resolution uses identical compressed JPEG image data for every EXIF orientation. The 24 MP PNG uses the same source RGB pixels before JPEG loss.",
        "files": entries,
    }
    (output / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    return manifest


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("outdir", type=Path, help="New output directory outside the repository")
    args = parser.parse_args()
    output = args.outdir.resolve()
    repository = Path(__file__).resolve().parents[2]
    if output.is_relative_to(repository):
        parser.error("Keep generated binary fixtures outside the repository")
    try:
        manifest = create_fixtures(output)
    except (OSError, ValueError) as error:
        parser.error(str(error))
    print(f"Wrote {len(manifest['files'])} synthetic fixtures and manifest.json", flush=True)


if __name__ == "__main__":
    main()
