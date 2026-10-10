# 工作流

## 目录结构

```text
workflows/
├── nightly.yml       # Linux 全量测试、服务端构建、归档和失败汇总
└── code-stats.yml    # 代码行数统计及徽章更新
```

## 内部关系与外部依赖

nightly 的构建产物供各测试 job 复用，report 在所有 job 结束后调用
`scripts/ci/summarize_failures.py`。环境安装依赖 `.github/actions`，归档和 issue
依赖 GitHub Actions 服务及 GitHub CLI。

## 容易踩的坑

- cron 使用 UTC；北京时间 21:00 对应 `0 13 * * *`。
- 汇总必须使用 `if: always()`，确保前置 job 失败后仍上报。
- `nightly-report-full.md` 必须随简要报告上传，保留全部失败和跳过用例。
- 每个 job 在结果上传前用 `if: always()` 记录机器配置；report 同样采集自身配置，再生成表格。
- 配置采集不阻塞测试；配置文件随各结果 artifact 上传，完整合集为 `nightly-runners.json`。
