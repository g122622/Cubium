# 工作流复用步骤

## 目录结构

```text
actions/
├── setup-linux-build-env/action.yml # 安装 clang 工具链与系统依赖，恢复构建缓存
├── setup-pococc-toolchain/action.yml # 安装独立实验工具链，不操作构建缓存
└── setup-datapack/action.yml        # 安装测试与基准所需的原版数据包
```

## 内部关系与外部依赖

nightly 的测试、发布和 fuzz 构建复用 Linux 环境安装步骤；测试及 benchmark 复用数据包安装。
依赖 apt.llvm.org、vcpkg、GitHub Actions cache 和数据包标签归档。

## 容易踩的坑

- 编译器版本与 LLVM 运行库须匹配；构建缓存 scope 必须区分测试、发布和 fuzz。
- vcpkg 二进制依赖缓存可共用，ccache 的对象缓存必须隔离不同构建选项。
- 数据包是世界生成的必需输入，不能因 benchmark 改用 Release 而省略。
