# 这是官方未完成版 starter（fa24-proj4-starter）

来源：`https://github.com/61c-teach/fa24-proj4-starter`（61c-teach 官方组织）。
判定依据：本仓库 11 个 starter 文件（Makefile / src/*.h / src/io.o / tools/*）的
md5 与本地已完成版 `../proj4` 内嵌的 check_hashes 列表**逐字节一致**——同一份
starter，`../proj4` 是它的完成态实现（compute_naive.c 94 行 / compute_optimized.c
140 行，本仓库只有 48/51 行的 TODO 桩）。

## 你需要实现什么

只有两个函数体（其它代码都已提供，`execute_task`、io.o、coordinator、Makefile、
测试框架全是现成的）：

- `src/compute_naive.c` → `convolve()`：朴素二维卷积（参考 spec 的"任务 1"，
  语义：输出尺寸 a-b+1、卷积核两维翻转、uint32 取模 2^32 累加）
- `src/compute_optimized.c` → `convolve()`：SIMD(AVX2) + OpenMP + 算法优化
  （spec 的"任务 2"，hive 上 4 线程跑分）

完成前后用 `make task_1` / `task_2` 验证（本地需模拟 hive oracle，见下）。

## 本地跑测试（模拟 hive 环境）

本目录已从完成版移植了 `tools/hive_sim/`（本地 oracle + 启动器，原理见
`../proj4/tools/hive_sim/README.md`，注意那份 README 里的"实测结果"是完成版
实现的数据，**不是**你应该达到的参考，只是说明本机 oracle/对拍是通的）：

```bash
bash tools/hive_sim/run.sh task_1 test_tiny      # 未实现时预期: convolve 返回非零, 任务失败
# ... 实现 naive 后应输出 N/N tests passed
bash tools/hive_sim/run.sh task_2 test_large
OMP_NUM_THREADS=4 bash tools/hive_sim/run.sh task_2 test_ag_random   # 跑分
```

- 官方跑分口径：`test_ag_random/increasing/decreasing`，目标加速比
  8.70 / 8.05 / 8.70，得分 = ln(实际加速比)/ln(目标)。
- 完整 spec 见 `../../readings/lab/lab08/OPTIONAL Lab 8  Project 4 61kaChow.md`。

## 建议

- 先自己在两个文件里实现，跑绿后再看 `../proj4/src/` 的完成版对照
  （那是一个可用的参考答案，也能当"对拍 oracle"的另一种实现）。
- 不要改动任何 starter 文件：每次 `make` 会跑 md5 哈希门（`tools/check_hashes.py`），
  只有 `tools/hive_sim/` 下新增文件是自由的。
