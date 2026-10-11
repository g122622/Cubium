# 工作流

## 目录结构

```text
workflows/
├── nightly.yml       # pococc 分布式构建、Linux 全量测试、发布和性能汇总
├── nightly-legacy.yml # 旧构建流程，仅允许手动触发
└── code-stats.yml    # 代码行数统计及徽章更新
```

## 内部关系与外部依赖

nightly 的构建产物供各测试 job 复用，report 在所有 job 结束后调用
`scripts/ci/summarize_failures.py`。环境安装依赖 `.github/actions`，归档和 issue
依赖 GitHub Actions 服务及 GitHub CLI。
测试链使用带 ASan/UBSan 的 RelWithDebInfo；独立 Release 构建负责公开发布，并把无 sanitizer 的产物供
独立 benchmark runner 使用。

## 容易踩的坑

- cron 使用 UTC；北京时间 21:00 对应 `0 13 * * *`。
- 汇总必须使用 `if: always()`，确保前置 job 失败后仍上报。
- `nightly-report-full.md` 必须随简要报告上传，保留全部失败和跳过用例。
- 每个 job 在结果上传前用 `if: always()` 记录机器配置；report 同样采集自身配置，再生成表格。
- 配置采集不阻塞测试；配置文件随各结果 artifact 上传，完整合集为 `nightly-runners.json`。
- 测试 artifact 不得公开发布；Release artifact 必须通过无调试信息/profiler/sanitizer 的核验。
- 测试、发布、fuzz 缓存独立；benchmark 必须使用相同配置的服务端，切换配置后重新建立基线。
- nightly 使用 32 个 worker（test/release/fuzz 为 16/10/6），每节点两个槽位；三个协调节点关闭 PCH 并严格分发。
- nightly-legacy 没有自动触发器；concurrency、Release 标签和 issue 标签与正式流程分开。
- 正式 nightly 没有旧实验的活跃检查，避免查询自身运行后跳过；report 始终执行。
- ccache/vcpkg 使用持久缓存；按构建范围保留最新 ccache，容量紧张时清理旧 Nightly 缓存。
