#!/usr/bin/env python3
"""
用 proj4 的卷积程序给一张图片做高斯模糊。

原理（和 staff 的 filters.py / test_gif_kachow_blur 完全一致）：
  1. 灰度图像素 0..255 组成矩阵 A（写入 a.bin）
  2. 高斯核（离散二维高斯，归一化使和=1）乘 1e6 取整，作为矩阵 B（写入 b.bin）
     -> 卷积输出 ≈ 1e6 * 模糊结果
  3. 跑 proj4 的 convolve 二进制，读回 out.bin，÷1e6、截断到 0..255 存图

注意：本项目的卷积是 valid 卷积（无 padding），输出尺寸 = A - B + 1，
即图像四周会收窄 (B-1)/2 像素；想要同尺寸输出需先给 A 补零，这里从简。

用法:
  python3 tools/hive_sim/blur_image.py 输入图片 输出图片 [--size 17] [--sigma 7]
  (默认调用 convolve_naive_naive；写完 task2 后可 --bin convolve_naive_optimized)
"""
import argparse, os, struct, subprocess, sys, time
from pathlib import Path
import numpy as np
from PIL import Image, ImageOps

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent            # proj4-starter 根目录
MULT = 1e6                            # 与 tools/filters.py 的 FILTER_MULTIPLIER 一致


def gaussian_kernel(size: int, sigma: float) -> np.ndarray:
    """与 tools/filters.create_gaussian_filter 相同的高斯核（和=1, float64）"""
    g = np.zeros((size, size), dtype=np.float64)
    m = size // 2
    for x in range(-m, m + 1):
        for y in range(-m, m + 1):
            g[x + m, y + m] = np.exp(-(x*x + y*y) / (2 * sigma*sigma))
    g /= np.sum(g)                    # 归一化: 和 = 1
    return g


def write_bin(path, rows, cols, vals_uint32):
    with open(path, "wb") as f:
        f.write(struct.pack("<II", rows, cols))
        f.write(struct.pack("<%dI" % (rows*cols), *[int(v) & 0xFFFFFFFF for v in vals_uint32]))


def read_bin(path):
    d = open(path, "rb").read()
    rows, cols = struct.unpack("<II", d[:8])
    vals = np.frombuffer(d[8:], dtype="<u4").reshape(rows, cols).astype(np.float64)
    return rows, cols, vals


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("input")
    ap.add_argument("output")
    ap.add_argument("--size", type=int, default=17)     # 核边长(须为奇数)
    ap.add_argument("--sigma", type=float, default=7.0)
    ap.add_argument("--bin", default="convolve_naive_naive")
    args = ap.parse_args()

    # 1) 图像 -> 灰度矩阵 A (0..255)
    img = ImageOps.grayscale(Image.open(args.input))
    a = np.asarray(img, dtype=np.int64)
    print(f"输入图像: {img.size[0]}x{img.size[1]}  (灰度)")

    # 2) 高斯核 -> B, 乘 1e6 取整 (uint32 位模式, 与 staff 相同)
    kernel = (gaussian_kernel(args.size, args.sigma) * MULT).astype(np.uint32)
    kh, kw = kernel.shape

    # 3) 搭一个 1 任务的测试目录, 写 a.bin/b.bin/input.txt (格式同 tests/ 下的任务)
    task_dir = ROOT / "tests" / "demo_blur"
    t0 = task_dir / "task0"
    t0.mkdir(parents=True, exist_ok=True)
    write_bin(t0 / "a.bin", *a.shape, a.flatten())
    write_bin(t0 / "b.bin", kh, kw, kernel.flatten())
    with open(task_dir / "input.txt", "w") as f:
        f.write("1\n./task0\n")

    # 4) 跑 proj4 的卷积程序
    if not (ROOT / args.bin).exists():
        print(f"缺少 {args.bin}, 先编译...")
        subprocess.run(["make", args.bin, "COORDINATOR=naive", "COMPUTE=naive"], cwd=ROOT, check=True)
    t0_ = time.perf_counter()
    subprocess.run([str(ROOT / args.bin), str(task_dir / "input.txt")], cwd=ROOT, check=True)
    print(f"卷积耗时: {(time.perf_counter()-t0_)*1e3:.1f} ms")

    # 5) out.bin -> 图: ÷1e6, 截断到 0..255
    orow, ocol, out = read_bin(t0 / "out.bin")
    out = np.clip(out / MULT, 0, 255).astype(np.uint8)
    print(f"输出尺寸: {ocol}x{orow}  (收窄了 {(a.shape[0]-orow)} 行 {(a.shape[1]-ocol)} 列 = 核-1)")
    Image.fromarray(out, mode="L").save(args.output)
    print(f"已保存: {args.output}")

    # 6) 验证模糊效果: 像素方差应该显著下降
    print(f"像素 std: 原图 {a.std():.1f} -> 模糊后 {out.std():.1f}")


if __name__ == "__main__":
    sys.exit(main())
