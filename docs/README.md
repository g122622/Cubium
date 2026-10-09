# 项目文档

## 目录结构

```text
docs/
├── SETUP.md                  # 环境和仓库初始化
├── BUILD.md                  # 构建规则和工具链
├── CI.md                     # nightly 测试、归档和失败汇总
├── CODE_CONVENTIONS.md       # 代码编写规范
├── PROJECT_CONVENTIONS.md    # 项目架构和开发规则
├── BENCHMARK.md              # 性能基准使用说明
├── test/                    # 各类测试指南
└── iterations/              # 迭代记录
```

## 内部关系与外部依赖

README 和 CLAUDE.md 指向这些开发指南；CI 文档连接工作流、测试脚本与报告 artifact，
构建文档连接本地脚本及工具链配置。文档描述仓库实现，并引用 GitHub Actions 等外部服务。

## 容易踩的坑

- 修改实现或流程时同步更新对应指南，以实际脚本和工作流为准。
- nightly 的执行结论与测试结果分开读取，跳过不能算作通过。
