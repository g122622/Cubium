# Nightly 辅助脚本

## 目录结构

```text
ci/
├── summarize_failures.py       # 汇总失败、跳过和报告错误，生成简要及完整报告
├── collect_runner_info.py      # 采集各 job 的硬件、系统镜像和实际工具版本
├── verify_pococc_build.py       # 核验无 PCH sanitizer 配置与严格编译分发记录
├── prepare_build_artifacts.py  # 核验 ELF 与构建配置，归档实际动态库依赖
├── compare_benchmark.py        # 归一化两轮实际耗时与内存指标，输出图表及 JSON
├── report_plots.py             # 渲染性能、硬件容量、型号分布与工具版本图表
├── symbolize_crash.py          # 用调试二进制还原崩溃栈
├── prune_caches.py             # 清理指定前缀的旧缓存
└── tests/                     # 汇总脚本回归验证
```

## 内部关系与外部依赖

`.github/workflows/nightly.yml` 调用这些独立脚本。性能对比和符号化先处理各 job 的结果，
汇总脚本再读取 artifacts 并决定是否建 issue。绘图依赖 Matplotlib、NumPy 与中文字体，其余 Python 侧依赖标准库；缓存操作依赖
GitHub CLI，符号化依赖 LLVM 工具和带调试信息的二进制。
机器配置随各 job 的结果归档；汇总器按任务展示图表，并输出完整的 `nightly-runners.json`。
机器配置与 Benchmark 图片分别放在默认折叠的章节内，展开后查看。
PNG/SVG 汇总到 `_report_charts`，由工作流发布到 `image-data/ci-reports/<run-id>` 后供 summary 与 issue 引用。

## 容易踩的坑

- pococc 实验同时核验真实 PCH 开关和编译命令；缓存命中不要求出现在 pococc journal。

- JUnit 和日志不能截断输入，否则后段失败会被判为不存在。
- 跳过与失败分别列出，跳过不能算作通过；报告缺失或损坏必须公开上报。
- 服务端每轮退出码是失败用例数，不能据此标记崩溃；协调脚本最终退出码 1 表示用例失败，2 才表示流水线错误。
- 简要报告遵守 GitHub 正文上限，完整报告保留全部失败和跳过用例。
- 构建工具版本优先读取 CMakeCache 选中的路径；测试机器预装的编译器不是产物编译器。
- 未运行与未采集的机器配置要明确区分；采集/解析错误只影响配置展示，不改变测试结论。
- 回归验证入口为 `python -m unittest discover -s scripts/ci/tests`。
- 发布二进制不能含调试节或 sanitizer 符号；测试二进制必须有两种 sanitizer 和调试信息。
- Release 的通用与配置专用编译/链接参数都须在 strip 前检查；归档保留 Release 链接参数。
- benchmark 只比较相同 `ci_build_profile` 的结果，避免跨优化配置产生错误告警。
- 时间先统一为纳秒再取代表值，优先使用 median 聚合；stddev/CV 不参与耗时比较。测量错误必须进入失败判定。
- 同一非矩阵任务的重复配置快照只取最新，矩阵分片与编译 worker 全部展示；原始快照完整归档。
