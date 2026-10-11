# pococc 分布式构建步骤

## 目录结构

```text
build-pococc/
└── action.yml  # 配置无 PCH 构建、连接 worker、缓存和性能采集
```

## 内部关系与外部依赖

nightly 的 test/release/fuzz 协调节点复用本步骤，使用 setup-pococc-toolchain、
pococc 的 CI 适配器和 scripts/ci。依赖 GitHub artifacts/cache、vcpkg 和认证 SSH 隧道。

## 容易踩的坑

- worker 与协调节点必须同时运行，且工具链一致；构建失败后仍需发布完成状态并清理隧道。
- ccache 在 pococc 前面，命中时不会产生远端调用；性能统计使用本轮计数差。
- test/release/fuzz 使用不同 ccache 范围；保存和容量清理需要 actions: write。
- 关闭 PCH、modules 扫描和 native CPU；网络失败禁止本地回退，明确的本地路由仍允许。
