"""Turns the raw PPM renders into submission evidence.

    outputs/cp1/*.ppm         -> outputs/cp1/png/*.png  + docs/CP1_Evidence.pdf
    outputs/cp2/*.ppm         -> outputs/cp2/png/*.png  + docs/CP2_Evidence.pdf
    outputs/cp2/frames/<a>/   -> outputs/cp2/gif/<a>.gif (one animated GIF per animation)
    outputs/cp3/*.ppm         -> outputs/cp3/png/*.png  + docs/CP3_Evidence.pdf

Usage (from the project root, after `make run`, `make animate` and
`sh tools/cp3_screenshots.sh`):
    python tools/make_evidence.py
Requires Pillow (pip install pillow). Only used for evidence; every image is
rendered by our own C++ code.
"""
import pathlib
import sys

from PIL import Image

ROOT = pathlib.Path(__file__).resolve().parent.parent
OUT = ROOT / "outputs"
DOCS = ROOT / "docs"


def convert_pages(checkpoint: str) -> list:
    src = OUT / checkpoint
    pages = []
    for ppm in sorted(src.glob("*.ppm")):
        img = Image.open(ppm).convert("RGB")
        (src / "png").mkdir(parents=True, exist_ok=True)
        img.save(src / "png" / (ppm.stem + ".png"), optimize=True)
        pages.append(img)
        print("png   ", src / "png" / (ppm.stem + ".png"))
    if pages:
        DOCS.mkdir(parents=True, exist_ok=True)
        pdf = DOCS / f"{checkpoint.upper()}_Evidence.pdf"
        pages[0].save(pdf, save_all=True, append_images=pages[1:], resolution=100)
        print("pdf   ", pdf)
    return pages


def make_gifs() -> None:
    frames_root = OUT / "cp2" / "frames"
    if not frames_root.is_dir():
        return
    (OUT / "cp2" / "gif").mkdir(parents=True, exist_ok=True)
    for folder in sorted(p for p in frames_root.iterdir() if p.is_dir()):
        frames = [Image.open(f).convert("RGB") for f in sorted(folder.glob("frame_*.ppm"))]
        if not frames:
            continue
        # One shared adaptive palette keeps the GIF small and flicker-free.
        palette = frames[len(frames) // 2].quantize(colors=128, method=Image.Quantize.MEDIANCUT)
        gif_frames = [f.quantize(palette=palette, dither=Image.Dither.NONE) for f in frames]
        gif = OUT / "cp2" / "gif" / (folder.name + ".gif")
        gif_frames[0].save(gif, save_all=True, append_images=gif_frames[1:], duration=1000 // 24, loop=0,
                           optimize=True)
        print("gif   ", gif, f"({len(frames)} frames)")


def main() -> int:
    total = 0
    for checkpoint in ("cp1", "cp2", "cp3"):
        total += len(convert_pages(checkpoint))
    make_gifs()
    if total == 0:
        print("No renders found - run `make run`, `make animate` and tools/cp3_screenshots.sh first.")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
