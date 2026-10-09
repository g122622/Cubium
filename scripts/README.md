# 项目脚本

## 目录结构

```text
scripts/
├── ci/                 # nightly 结果汇总、性能对比、崩溃符号化和缓存清理
├── test/               # 测试执行与报告生成
├── build/              # 构建辅助脚本
├── configure.sh        # 本地构建入口
├── format/             # C++ 代码格式化
├── baking/             # 资源预处理
├── diag/               # 运行诊断工具
├── iwyu/               # 头文件依赖检查
├── tidy/               # 静态检查辅助工具
└── trace_analysis/     # 性能追踪分析
```

## 内部关系与外部依赖

构建脚本产出测试二进制，测试脚本执行测试，CI 脚本读取归档结果生成报告。
本地开发和 `.github/workflows` 调用这些工具；依赖按脚本分别为 Python 标准库、Node.js、
CMake 或 GitHub CLI。CI 使用说明见 `docs/CI.md`。

## 容易踩的坑

- Windows 构建入口为 `./scripts/configure.sh build`。
- C++ 格式化脚本要求 Node.js 24，仅处理 `.cpp`、`.hpp`。
- 测试输出可能很大；CI 解析必须读取完整输入，长度限制只适用于展示结果。
