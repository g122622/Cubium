# BossBar 模块

Boss 栏系统，用于创建和管理自定义 Boss 血量条。

## 目录结构

```
bossbar/
├── BossInfo.hpp/cpp               # Boss 栏基类（颜色、样式枚举、核心属性）
├── ServerBossInfo.hpp/cpp         # 服务端 Boss 栏（玩家可见性管理、属性变更通知）
├── CustomServerBossInfo.hpp/cpp   # /bossbar 命令创建的自定义 Boss 栏（持久化、数值管理）
├── CustomServerBossInfoManager.hpp/cpp  # Boss 栏管理器（生命周期、网络同步）
└── README.md
```

## 内部模块关系

```
BossInfo (基类)
    │
    └── ServerBossInfo (服务端扩展：玩家管理)
            │
            └── CustomServerBossInfo (自定义 Boss 栏：持久化、数值管理)
                    │
                    └── 由 CustomServerBossInfoManager 管理
```

- **BossInfo**：定义核心属性（UUID、名称、百分比、颜色、样式、标志位）
- **ServerBossInfo**：添加玩家可见性管理，属性变更时标记更新类型
- **CustomServerBossInfo**：添加资源位置 ID、value/max 数值管理、NBT 持久化、玩家 UUID 集合
- **CustomServerBossInfoManager**：管理所有自定义 Boss 栏的生命周期

## 外部依赖

### 依赖的上游模块

- `common/core/Types.hpp` - 基础类型（u64, f32, i32, PlayerId 等）
- `common/resource/ResourceLocation.hpp` - 资源位置 ID
- `common/util/text/ITextComponent.hpp` - 文本组件
- `common/util/text/ComponentUtils.hpp` - wrapInSquareBrackets 方括号包裹工具（formattedName 使用）
- `common/util/nbt/Nbt.hpp` - NBT 序列化

### 被谁依赖

目前暂无上游模块依赖此模块。未来 `/bossbar` 命令实现后将依赖 `CustomServerBossInfoManager`。

## 容易踩的坑

### UUID 生成

当前实现使用资源位置 ID 的哈希值作为 UUID。对于生产环境，应考虑使用真正的 UUID 生成器：

```cpp
u64 uuid = std::hash<std::string>{}(id.toString());
```

### 网络包尚未实现

`sendAddPacket()`, `sendRemovePacket()`, `broadcastUpdate()` 方法当前仅标记数据为脏。需要实现 `BossInfoPacket` 后才能进行真正的网络同步。

### 玩家登出处理

`onPlayerLogout()` 不发送网络包，因为玩家已经断开连接。只清理内存中的可见性状态，但保留 UUID 记录用于重连恢复。

### 线程安全

`CustomServerBossInfoManager` 不是线程安全的。如果需要在多线程环境使用，需要添加互斥锁保护。

### 玩家 UUID 集合

`CustomServerBossInfo` 维护两套玩家集合：
- `m_players`（继承自 ServerBossInfo）：当前在线可见玩家的 PlayerId
- `m_playerUuids`：持久化的玩家 UUID 字符串集合，用于玩家重连后恢复可见性

两者需要同步维护。
