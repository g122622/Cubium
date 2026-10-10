# Nightly 辅助脚本

## 目录结构

```text
ci/
├── summarize_failures.py       # 汇总失败、跳过和报告错误，生成简要及完整报告
├── collect_runner_info.py      # 采集各 job 的硬件、系统镜像和实际工具版本
├── compare_benchmark.py        # 比较两轮 CPU 和内存指标
├── symbolize_crash.py          # 用调试二进制还原崩溃栈
├── prune_caches.py             # 清理指定前缀的旧缓存
└── tests/                     # 汇总脚本回归验证
```

## 内部关系与外部依赖

`.github/workflows/nightly.yml` 调用这些独立脚本。性能对比和符号化先处理各 job 的结果，
汇总脚本再读取 artifacts 并决定是否建 issue。Python 侧依赖标准库；缓存操作依赖
GitHub CLI，符号化依赖 LLVM 工具和带调试信息的二进制。
机器配置随各 job 的结果归档；汇总器按 job 展示紧凑表格，并输出完整的 `nightly-runners.json`。

## 容易踩的坑

- JUnit 和日志不能截断输入，否则后段失败会被判为不存在。
- 跳过与失败分别列出，跳过不能算作通过；报告缺失或损坏必须公开上报。
- 简要报告遵守 GitHub 正文上限，完整报告保留全部失败和跳过用例。
- 构建工具版本优先读取 CMakeCache 选中的路径；测试机器预装的编译器不是产物编译器。
- 未运行与未采集的机器配置要明确区分；采集/解析错误只影响配置展示，不改变测试结论。
- 回归验证入口为 `python -m unittest discover -s scripts/ci/tests`。
