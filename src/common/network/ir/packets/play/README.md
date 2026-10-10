# Play 包定义

## 目录结构

```text
play/
├── PlayPackets.hpp          # 基础移动、实体、世界和容器包
└── PlayPacketsExtended.hpp  # 扩展状态、界面与会话包
```

## 内部模块关系

两个头文件定义叶子结构；IrPacket.hpp 的 PlayPacket 统一聚合这些结构。

## 上下游外部依赖关系

依赖公共基础类型、物品视图和 NBT；Java codec、服务端路由和客户端 visitor 使用这些结构。

## 容易踩的坑

- 同名双向包的字段可能不同，应在必要时使用不同结构和 codec。
- ContainerClose 两个方向均为容器 ID，但协议 ID 分别为 18 和 17。
- 变体新增项追加到末尾，避免已有登记的下标漂移。
