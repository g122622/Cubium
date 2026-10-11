# pococc 工具链

## 目录结构

    setup-pococc-toolchain/
    └── action.yml   # 安装 Clang 22、distcc、SSH 和可选 Cubium 系统依赖

## 模块关系与外部依赖

由正式 nightly 的工具准备、构建和 worker job 使用。依赖 Ubuntu apt 与 apt.llvm.org，legacy 使用旧环境 action。

## 容易踩的坑

- coordinator 与 worker 必须使用相同 Clang 版本和路径。
- LLVM 安装脚本固定上游提交和 SHA-256，校验通过后才执行；升级时必须同时更新两者。
- 本 action 不恢复或保存 Actions cache；build-pococc 负责持久缓存和容量清理。
