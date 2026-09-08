# hive_sim — 在本地模拟 hive 机器的 proj4 测试环境

CS61C proj4（卷积 / 性能优化项目）的测试流程是：

```
make task_1|task_2 TEST=tests/<name>
  ├─ python3 tools/create_tests.py <name>   # 生成输入 a.bin/b.bin + 参考 ref.bin
  ├─ ./convolve_naive_naive / convolve_naive_optimized <test>/input.txt
  └─ check_output.sh                         # diff 每个 task 的 out.bin 与 ref.bin
```

其中生成 `ref.bin` 的**工作人员 oracle** 只装在 hive 机器上：
`tools/framework.py` 里硬编码了

```python
oracle_path = Path("/home/ff/cs61c/fa24/proj4/convolve_oracle")
...
raise RuntimeError("Oracle does not exist, please run on the hive machines")
```

所以在自己机器上 `make task_1 ...` 会直接报这个错，测试永远跑不起来
（你仓库里那个残缺的 `tests/test_example`——只有 `input.txt` 一行、没有
`ref.bin`——就是一次“没有 oracle 就生成测试”失败留下的半成品）。

## 本目录做了什么

不改任何 starter 文件（`tools/check_hashes.py` 的哈希门仍然通过），
新增一套“本地 oracle + 启动器”：

| 文件 | 作用 |
|---|---|
| `convolve_oracle.c` | 自建的等价 oracle：读两个 .bin 矩阵，按参考语义做卷积（输出尺寸 `a-b+1`，卷积核两维翻转，所有运算在 uint32 上取模 2^32），写出 `ref.bin`。与 `src/compute_naive.c` 注释描述、以及优化版实现的位模式完全一致。 |
| `convolve_oracle` | 编译出的 oracle 二进制（`build_oracle.sh` 生成） |
| `build_oracle.sh` | `gcc -O2 -std=c99` 编译 oracle |
| `sim.py` | hive 模拟启动器：把 `framework.oracle_path` 指到本地 `convolve_oracle`，再原样执行 `tools/create_tests.py`（传入参数完全一致）。框架代码一个字节都没动。 |
| `verify_oracle.py` | 用 numpy uint32（加法/乘法自动按 2^32 回绕，和 C 是同一个环）做**独立交叉验证**：8 组边界用例（含全 uint32 回绕、负数位型、1×1、1×N 一维、a==b、核 1×1）全部 bit 级一致。 |
| `run.sh` | 本地版 `make task_1|task_2 TEST=...`：必要时构建二进制 → 用模拟 oracle 重新生成测试 → 跑二进制 → 对拍。缺 `ref.bin` 的旧测试目录会自动强制重生成 ref。 |

## 用法

```bash
# 1) 构建 oracle（只需要一次）
bash tools/hive_sim/build_oracle.sh

# 2) 可选：交叉验证 oracle 与 numpy 独立参照
python3 tools/hive_sim/verify_oracle.py

# 3) 跑测试（等价于 make task_1/task_2 TEST=tests/<name>）
bash tools/hive_sim/run.sh task_1 test_tiny
bash tools/hive_sim/run.sh task_2 test_large

# 4) 只重新生成某套测试的输入 + ref.bin（不带跑）
python3 tools/hive_sim/sim.py test_tiny            # 同 create_tests.py 的用法
python3 tools/hive_sim/sim.py test_*               # 支持通配符
```

## 本地实测结果（Ubuntu 24.04, 6 核, gcc 13, numpy 1.26.4）

正确性（`run.sh` 输出 `N/N tests passed`，逐 task diff `out.bin` vs `ref.bin`）：

| 测试 | task_1 (naive) | task_2 (optimized) |
|---|---|---|
| test_example | 1/1 | 1/1 |
| test_tiny | 4/4 | 4/4 |
| test_small | 10/10 | 10/10 |
| test_large | 50/50 | 50/50 |
| test_ag_random / increasing / decreasing | 50/50, 27/27, 27/27 | 同左 |

性能（与 autograder 相同的口径：优化版 vs 朴素版，`OMP_NUM_THREADS=4`；
整段计时含 I/O，一次运行）：

| 基准 | naive | optimized(4线程) | 加速比 | 目标 | 得分 ln(x)/ln(t) |
|---|---|---|---|---|---|
| test_ag_random | 5531 ms | 289 ms | 19.1× | 8.70× | ≈ 1.36 |
| test_ag_increasing | 3728 ms | 248 ms | 15.0× | 8.05× | ≈ 1.30 |
| test_ag_decreasing | 3085 ms | 150 ms | 20.6× | 8.70× | ≈ 1.40 |

（注：这个仓库里的 `compute_optimized.c` 是完成态实现——AVX2 向量化 + OpenMP
按输出行并行 + 卷积核预翻转。若你换成自己的实现重新 `make` 即可对比。）

## 和真实 hive 环境的对应关系

hive 上跑测试只有两样本机没有的东西：

1. **oracle 二进制** → 本目录的 `convolve_oracle` 等价替代。
2. （仅可选视觉测试需要）`/home/ff/cs61c/fa24/proj4/gifs/kachow.gif`
   素材 → 本机没有（课程站点也 404），`test_gif_kachow_blur/sharpen`
   因此跳过；它们是选做的 GIF 输出演示，不影响打分。

若你在**能写 `/home/ff` 的机器**（真实 hive、或有 root 的机器）上：

```bash
sudo mkdir -p /home/ff/cs61c/fa24/proj4
sudo cp tools/hive_sim/convolve_oracle /home/ff/cs61c/fa24/proj4/
```

之后**原生的 `make task_1 TEST=...` 一行不改就能跑**——这正说明本地模拟
与 hive 是一比一的：差别只在 oracle 的落盘路径。

环境对照：

| 依赖 | hive | 本机 |
|---|---|---|
| python3 + numpy + Pillow | ✓ | ✓ (3.12 / 1.26.4 / 10.2.0) |
| gcc + `-fopenmp -mavx -mavx2 -mfma` | ✓ | ✓ (gcc 13) |
| 官方 oracle @ `/home/ff/...` | ✓ | 用 `tools/hive_sim/convolve_oracle` |
| MPI (`compute_optimized_mpi.c`) | ✓ | ✗（不需要：Makefile 只有 task_1/task_2，官方测试不编译 MPI 版本） |
| `kachow.gif` | ✓ | ✗（仅 gif 测试需要） |

## 注意事项

- **确定性**：测试输入由 `numpy.random`（legacy 种子）生成。本机 numpy
  1.26 重新生成的 `test_example` 输入与仓库原文件的 md5 **完全一致**
  （`0b53b019…` / `40d0e49e…`），说明复现了官方序列。若日后 numpy 大版本
  升级导致序列变化，重新生成后本地自洽（ref 与输入同步更新），不影响对拍。
- **计时波动**：hive 是多用户共享机器，负载会明显影响计时（spec 里也建议
  选低负载 hive）。本地计时只反映这台机器的相对加速比；打分公式
  `score = ln(x)/ln(t)` 用的是同一台机器上的 naive/optimized 之比，负载
  影响会被约掉大部分。
- **不要改 starter 文件**：`Makefile` 每次构建都会跑 `check_hashes.py`
  校验 starter 文件 md5。本方案新增文件不在此列，哈希门保持绿色。
- 知乎那篇旧学期笔记里"找不到 dumbpy / 没法对拍"的痛点，在本项目里就是
  "oracle 只在 hive"。等价解法：自建 reference（这里的 C oracle），再用
  numpy 独立验证一遍 reference 本身是对的——两条腿都齐了。
