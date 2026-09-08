#!/usr/bin/env python3
"""
Cross-validate the local stand-in oracle against an independent numpy uint32
reference (numpy uint32 ops wrap mod 2^32, the same ring the C code uses).

Run:  python3 tools/hive_sim/verify_oracle.py
"""
import os
import struct
import subprocess
import sys
import tempfile

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ORACLE = os.path.join(HERE, "convolve_oracle")


def write_bin(path, m):
    rows, cols = m.shape
    flat = [int(v) & 0xFFFFFFFF for v in m.flatten()]
    with open(path, "wb") as f:
        f.write(struct.pack("<II", rows, cols))
        f.write(struct.pack("<%dI" % len(flat), *flat))


def read_bin(path):
    with open(path, "rb") as f:
        d = f.read()
    rows, cols = struct.unpack("<II", d[:8])
    vals = np.frombuffer(d[8:], dtype="<u4").reshape(rows, cols).copy()
    return vals


def ref_np(a, b):
    """Independent reference: uint32 windowed multiply-accumulate (wraps mod 2^32)."""
    a = a.astype(np.uint32)
    b = b.astype(np.uint32)
    br, bc = b.shape
    orow, ocol = a.shape[0] - br + 1, a.shape[1] - bc + 1
    bf = b[::-1, ::-1]  # flip B in both dims, like the C code
    out = np.zeros((orow, ocol), dtype=np.uint32)
    for i in range(orow):
        for j in range(ocol):
            out[i, j] = (a[i:i + br, j:j + bc] * bf).sum(dtype=np.uint32)
    return out


def rng_u32(seed, rows, cols):
    rs = np.random.RandomState(seed)
    # full uint32 range, assembled from two 16-bit draws (covers mod-2^32 wrap)
    n = rows * cols
    lo = rs.randint(0, 65536, size=n).astype(np.uint32)
    hi = rs.randint(0, 65536, size=n).astype(np.uint32)
    return ((hi << np.uint32(16)) | lo).reshape(rows, cols)


def rng_small(seed, rows, cols):
    """Values like staff tests: randint(-1000,1000) & 0xFFFFFFFF."""
    rs = np.random.RandomState(seed)
    return (rs.randint(-1000, 1001, size=rows * cols) & 0xFFFFFFFF).astype(
        np.uint32).reshape(rows, cols)


def main():
    if not os.path.exists(ORACLE):
        sys.stderr.write("Missing %s -- run bash tools/hive_sim/build_oracle.sh\n" % ORACLE)
        return 1

    cases = [
        ("tiny", rng_small(1, 5, 6), rng_small(2, 2, 3)),
        ("rect", rng_small(3, 8, 3), rng_small(4, 3, 2)),
        ("neg-and-wrap", rng_u32(5, 7, 7), rng_u32(6, 4, 4)),
        ("full-range", rng_u32(7, 12, 9), rng_u32(8, 5, 6)),
        ("1x1", rng_u32(9, 1, 1), rng_u32(10, 1, 1)),
        ("row-1d", rng_small(11, 1, 17), rng_small(12, 1, 7)),
        ("a==b", rng_small(13, 4, 4), rng_small(14, 4, 4)),
        ("kernel-1x1", rng_u32(15, 9, 9), rng_u32(16, 1, 1)),
    ]

    ok = True
    with tempfile.TemporaryDirectory() as td:
        for name, a, b in cases:
            ap, bp, op = (os.path.join(td, s) for s in ("a.bin", "b.bin", "ref.bin"))
            write_bin(ap, a)
            write_bin(bp, b)
            r = subprocess.run([ORACLE, ap, bp, op], capture_output=True)
            if r.returncode != 0:
                print("[FAIL] %-12s oracle exited %d: %s" % (name, r.returncode, r.stderr.decode()))
                ok = False
                continue
            ref = ref_np(a, b)
            got = read_bin(op)
            match = (got.shape == ref.shape) and np.array_equal(got, ref)
            print("[%s] %s  oracle %s == numpy-uint32 ref (%dx%d)" %
                  ("PASS" if match else "FAIL", name,
                   "MATCHES" if match else "DIFFERS", ref.shape[0], ref.shape[1]))
            ok = ok and match
    print("all checks passed" if ok else "SOME CHECKS FAILED")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
