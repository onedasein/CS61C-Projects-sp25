#!/usr/bin/env python3
"""
Synthetic MNIST-style demo for CS61C Proj2.

Draws 0-9 as 28x28 digit images from 5x7 bitmap templates, adds random
shift + noise, trains a *linear* softmax classifier (bias folded into an
extra constant-1 input row -> 785 inputs), and exports the weights in the
.bin format read_matrix/write_matrix expect.

It runs *unchanged* through the student's classify.s:
    m0 = W            (10 x 785)   -- rows=10, cols=785
    m1 = Identity     (10 x 10)
    input = [pixels(784); 1]       -- rows=785, cols=1
classify computes h = relu(m0 @ input), o = m1 @ h, argmax(o).
Since o == relu(Wx+b) and relu is monotonic, argmax(o) == argmax(Wx+b),
i.e. exactly the linear classifier's prediction. Integer quantization is
a positive scaling of W, which does not change argmax.

Usage:
    python3 gen_mnist.py            # train, self-check, export demo digits
"""
import struct
import numpy as np
from PIL import Image

np.random.seed(0)

DIGITS = {
    0: ["01110", "10001", "10011", "10101", "11001", "10001", "01110"],
    1: ["00100", "01100", "00100", "00100", "00100", "00100", "01110"],
    2: ["01110", "10001", "00001", "00010", "00100", "01000", "11111"],
    3: ["11111", "00010", "00100", "00010", "00001", "10001", "01110"],
    4: ["00010", "00110", "01010", "10010", "11111", "00010", "00010"],
    5: ["11111", "10000", "11110", "00001", "00001", "10001", "01110"],
    6: ["00110", "01000", "10000", "11110", "10001", "10001", "01110"],
    7: ["11111", "00001", "00010", "00100", "01000", "01000", "01000"],
    8: ["01110", "10001", "10001", "01110", "10001", "10001", "01110"],
    9: ["01110", "10001", "10001", "01111", "00001", "00010", "01100"],
}

TEMPLATE = {}
for d, rows in DIGITS.items():
    img = np.zeros((7, 5), dtype=np.float64)
    for r, line in enumerate(rows):
        for c, ch in enumerate(line):
            if ch == "1":
                img[r, c] = 1.0
    big = np.zeros((28, 28), dtype=np.float64)
    big[0:28, 4:24] = np.kron(img, np.ones((4, 4)))   # 7x5 -> 28x20, centered
    TEMPLATE[d] = big


def perturb(img, shift=2, noise=0.15):
    out = np.zeros_like(img)
    dx = np.random.randint(-shift, shift + 1)
    dy = np.random.randint(-shift, shift + 1)
    sx0, sx1 = max(0, dx), min(28, 28 + dx)
    sy0, sy1 = max(0, dy), min(28, 28 + dy)
    tx0, tx1 = max(0, -dx), min(28, 28 - dx)
    ty0, ty1 = max(0, -dy), min(28, 28 - dy)
    out[ty0:ty1, tx0:tx1] = img[sy0:sy1, sx0:sx1]
    out += np.random.normal(0, noise, out.shape)
    return np.clip(out, 0, 1)


def make_dataset(n_per_digit=200):
    X, Y = [], []
    for d in range(10):
        for _ in range(n_per_digit):
            X.append(perturb(TEMPLATE[d]).reshape(-1))
            Y.append(d)
    return np.array(X, dtype=np.float64), np.array(Y, dtype=np.int64)


def train_linear(X, Y, epochs=400, batch=128, lr=3.0):
    """Softmax linear classifier on [x; 1] (bias folded in). 10 x 785."""
    N = X.shape[0]
    Xb = np.hstack([(X > 0.5).astype(np.float64), np.ones((N, 1))])   # N x 785
    W = np.random.randn(10, 785) * 0.01
    Yoh = np.eye(10)[Y]
    n_batches = max(1, N // batch)
    for ep in range(epochs):
        perm = np.random.permutation(N)
        for b in range(n_batches):
            idx = perm[b * batch : (b + 1) * batch]
            xb = Xb[idx].T                        # 785 x B
            o = W @ xb                            # 10 x B
            p = np.exp(o - o.max(axis=0, keepdims=True))
            p /= p.sum(axis=0, keepdims=True)
            g = (p - Yoh[idx].T) @ xb.T           # 10 x 785
            W -= lr * g / len(idx)
    return W


def int_forward(Q1, inp_bin):
    """m0 = Q1 (10x785), m1 = identity; returns argmax(o)."""
    h = np.maximum(Q1 @ inp_bin, 0)               # 10
    return int(np.argmax(h))


def write_matrix(path, rows, cols, data):
    with open(path, "wb") as f:
        f.write(struct.pack("<ii", rows, cols))
        f.write(struct.pack(f"<{rows*cols}i", *[int(v) for v in data]))


def save_png(path, arr, scale=10, invert=True):
    """arr: 2D float array (rows x cols). Upscale with nearest neighbor, save grayscale PNG."""
    a = np.asarray(arr)
    if a.ndim != 2:
        raise ValueError("expected 2D array")
    img = Image.fromarray(((a - a.min()) / ((a.max() - a.min()) or 1) * 255).astype(np.uint8), "L")
    if invert:
        img = img.point(lambda v: 255 - v)          # black bg, white ink (MNIST style)
    img = img.resize((a.shape[1] * scale, a.shape[0] * scale), Image.NEAREST)
    img.save(path)


def render(img, rows=28, cols=28):
    lo, hi = img.min(), img.max()
    rng = (hi - lo) or 1
    ramp = " .:-=+*#%@"
    return "\n".join(
        "".join(ramp[int((img[i * cols + j] - lo) / rng * (len(ramp) - 1))] for j in range(cols))
        for i in range(rows)
    )


def main():
    print("generating synthetic dataset...")
    X, Y = make_dataset()
    Xr = X.reshape(10, -1, 784)                     # [digit, sample, px]
    Xte = Xr[:, -50:].reshape(-1, 784)
    Yte = np.repeat(np.arange(10), 50)
    Xtr = Xr[:, :-50].reshape(-1, 784)
    Ytr = np.repeat(np.arange(10), Xr.shape[1] - 50)

    print("training linear softmax classifier (10x785)...")
    W = train_linear(Xtr, Ytr)

    # int-quantized weights (m0); m1 = identity
    q = 1000
    Q1 = np.round(np.clip(W * q, -2**30, 2**30)).astype(np.int64)
    acc = 0
    for x, y in zip(Xte, Yte):
        inp = np.r_[(x > 0.5).astype(np.int64), np.int64(1)]   # 785 x 1
        if int_forward(Q1, inp) == y:
            acc += 1
    print(f"int-quantized test accuracy: {acc}/{len(Yte)} = {acc/len(Yte):.3f}")

    chosen = [0, 3, 7]
    print("\n--- exported demo digits ---")
    for d in chosen:
        img = Xte[np.where(Yte == d)[0][0]]
        inp = np.r_[(img > 0.5).astype(np.int32), np.int32(1)]   # 785 x 1
        pred = int_forward(Q1, inp)
        if pred != d:
            print(f"  !! digit {d}: predicts {pred}, skipping (try a different seed/digit)")
            continue
        write_matrix("demo/m0.bin", 10, 785, Q1.reshape(-1))
        write_matrix("demo/m1.bin", 10, 10, np.eye(10, dtype=np.int32).reshape(-1))
        write_matrix(f"demo/input_{d}.bin", 785, 1, inp)
        pix = inp[:784].reshape(28, 28)
        save_png(f"demo/input_{d}.png", pix, scale=10)
        print(f"  digit {d}: m0 (10x785) m1 (identity) input_{d}.bin  [int argmax == {d} OK]")
        for r in range(28):
            print("".join("█" if pix[r, c] else " " for c in range(28)))

    # visualize the learned weight rows (each class -> 28x28 template, last col = bias)
    Wimg = np.zeros((10, 28 * 28 + 8))
    for d in range(10):
        Wimg[d, : 28 * 28] = Q1[d, : 784]
        Wimg[d, 28 * 28:] = Q1[d, 784] / (abs(Q1[d, 784]) or 1)
    save_png("demo/weights.png", Wimg, scale=6, invert=True)
    print("saved demo/weights.png  (10 rows: each class's learned 28x28 weight template)")

    print("\nRun through classify.s, e.g.:")
    print("  cd mnist-demo && java -jar ../tools/venus.jar --maxsteps -1 ../src/main.s demo/m0.bin demo/m1.bin demo/input_7.bin demo/output.bin")


if __name__ == "__main__":
    main()
