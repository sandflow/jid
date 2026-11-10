#!/usr/bin/env python3
"""Encodes a TIFF sequence to IMF-compliant HTJ2K using OpenJPH, then wraps the
codestreams into an IMF Image Track File using jid-writer. Lossless by default;
lossy if --qstep is given."""

import argparse
import concurrent.futures
import os
import struct
import subprocess
import sys
import tempfile
from pathlib import Path


def encode(ojph_compress, src, dst, bit_depth, num_decomps, qstep):
    # IMF profile: CPRL, 32x32 code-blocks, 128/256 precincts, TLM, component tile-parts.
    # -bit_depth keeps the MSBs of the 16-bit TIFF samples.
    cmd = [ojph_compress, "-i", str(src), "-o", str(dst),
           "-bit_depth", str(bit_depth),
           "-colour_trans", "true",
           "-num_decomps", str(num_decomps),
           "-prog_order", "CPRL",
           "-block_size", "{32,32}",
           "-precincts", "{128,128},{256,256}",
           "-profile", "IMF"]
    if qstep is None:
        cmd += ["-reversible", "true"]
    else:
        cmd += ["-reversible", "false", "-qstep", str(qstep)]
    subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("tiff_dir", type=Path, help="directory containing the .tif/.tiff frames")
    ap.add_argument("out", type=Path, help="output MXF file")
    ap.add_argument("--max-frames", type=int, default=0, help="number of frames to encode (default: all)")
    ap.add_argument("--qstep", type=float, help="quantization step (0.00001...0.5); "
                    "if given, use lossy coding (9/7 wavelet) instead of lossless (5/3)")
    ap.add_argument("--bit-depth", type=int, default=12)
    ap.add_argument("--num-decomps", type=int, default=6, help="wavelet decomposition levels (IMF 4K allows up to 6)")
    ap.add_argument("--jobs", type=int, default=os.cpu_count(), help="parallel encodes")
    ap.add_argument("--fps", default="24000/1001")
    ap.add_argument("--components", default="RGB", choices=["RGB", "YCbCr", "XYZ"])
    ap.add_argument("--color", default="COLOR.3",
                    choices=["COLOR.1", "COLOR.2", "COLOR.3", "COLOR.4", "COLOR.5",
                             "COLOR.6", "COLOR.7", "COLOR.APP4.2"],
                    help="colorimetry (default: %(default)s)")
    ap.add_argument("--quantization", default="QE.2", choices=["QE.1", "QE.2"])
    ap.add_argument("--ojph-compress", required=True, help="path to ojph_compress")
    ap.add_argument("--jid-writer", required=True, help="path to jid-writer")
    args = ap.parse_args()

    frames = sorted(f for f in args.tiff_dir.iterdir() if f.suffix.lower() in (".tif", ".tiff"))
    if args.max_frames > 0:
        frames = frames[:args.max_frames]
    if not frames:
        sys.exit("no .tif/.tiff files found in " + str(args.tiff_dir))

    with tempfile.TemporaryDirectory() as work:
        work = Path(work)
        j2cs = [work / (f.stem + ".j2c") for f in frames]

        with concurrent.futures.ThreadPoolExecutor(args.jobs) as ex:
            for fut in [ex.submit(encode, args.ojph_compress, f, j, args.bit_depth,
                                  args.num_decomps, args.qstep) for f, j in zip(frames, j2cs)]:
                fut.result()

        # MJC (VBR) stream: 16-byte header "MJC2" + 12 zero bytes, then each
        # codestream preceded by its 4-byte big-endian length; piped to jid-writer.
        cmd = [args.jid_writer, "--format", "MJC", "--components", args.components,
               "--color", args.color, "--quantization", args.quantization,
               "--fps", args.fps,
               "--out", str(args.out)]
        jid = subprocess.Popen(cmd, stdin=subprocess.PIPE)
        try:
            jid.stdin.write(b"MJC2" + bytes(12))
            for j in j2cs:
                d = j.read_bytes()
                jid.stdin.write(struct.pack(">I", len(d)) + d)
            jid.stdin.close()
        except BrokenPipeError:
            pass
        if jid.wait() != 0:
            sys.exit("jid-writer failed")


if __name__ == "__main__":
    main()
