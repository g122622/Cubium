# GitHub 自动化

## 目录结构

```text
.github/
├── actions/       # 工作流复用的 Linux 构建环境和数据包安装步骤
└── workflows/     # nightly CI 和代码统计工作流
```

## 内部关系与外部依赖

工作流调用 composite actions 和 `scripts/ci`，通过 GitHub Actions 缓存、artifact、
Release 和 issue 服务归档结果。详细规则见 `docs/CI.md`。

## 容易踩的坑

- nightly 只按定时或手动事件运行；仓库约束禁止代理主动启动工作流。
- 非阻塞测试 job 的成功结论不能用作全部用例通过的证据。
- GitHub issue 正文有长度上限，全部失败及跳过清单另存完整报告 artifact。
- nightly-pococc 是仅手动触发的独立构建实验，不修改现有 nightly，说明见 docs/CI.md。
