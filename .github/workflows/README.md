# 工作流

## 目录结构

```text
workflows/
├── ci.yml            # PR 与合并队列的变更 C++ 文件 clang-tidy 门禁
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

`ci.yml` 只在 PR 和 merge queue 运行，只检查本次变更的 C++ 源文件与头文件；头文件使用临时编译数据库条目。
失败判定收窄到本次 diff 的新增行（`scripts/ci/changed_lines.py` 生成行区间，`tidy.sh --changed-lines`
据此过滤），区间外的存量告警不影响 job 结果；区间内的告警或错误仍会令 job 失败。

## 容易踩的坑

- cron 使用 UTC；北京时间 21:00 对应 `0 13 * * *`。
- 汇总必须使用 `if: always()`，确保前置 job 失败后仍上报。
- `nightly-report-full.md` 必须随简要报告上传，保留全部失败和跳过用例。
- 每个 job 在结果上传前用 `if: always()` 记录机器配置；report 同样采集自身配置，再生成 Matplotlib 图表。
- 图表随报告归档，并发布到 image-data 的各运行目录；发布步骤须保留其他文件，避免徽章更新擦除历史图表。
- 配置采集不阻塞测试；配置文件随各结果 artifact 上传，完整合集为 `nightly-runners.json`。
- 测试 artifact 不得公开发布；Release artifact 必须通过无调试信息/profiler/sanitizer 的核验。
- 单测宿主需要完整行为包：测试产物须同时携带编译后的 scripts 和构建时复制的共享 structures，不能只上传 JS。
- 测试、发布、fuzz 缓存独立；benchmark 必须使用相同配置的服务端，切换配置后重新建立基线。
- nightly 使用 34 个 worker（test/release/fuzz 为 24/4/6），每节点四个槽位，三个池共 136 个远端槽位；三个协调节点关闭 PCH 并严格分发。
- worker 矩阵的 `max-parallel` 必须允许全部 34 个实例同时启动；池内 worker 会一直等待协调节点结束，较低上限会令后排 worker 无法启动并触发协调节点超时。
- 单元测试分为 20 片，最多 20 台并行；GameTest 分为 10 片，最多 10 台并行。选择步长、矩阵和覆盖校验的分片数须同步。
- nightly-legacy 没有自动触发器；concurrency、Release 标签和 issue 标签与正式流程分开。
- 正式 nightly 没有旧实验的活跃检查，避免查询自身运行后跳过；report 始终执行。
- ccache/vcpkg 使用持久缓存；按构建范围保留最新 ccache，容量紧张时清理旧 Nightly 缓存。
