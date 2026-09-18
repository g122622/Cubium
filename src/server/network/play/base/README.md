# network/play/base - 处理器基座

`play/` 子树的**依赖链末梢**：只前置声明 `MinecraftServer`，不含任何 `server/application`
具体依赖。各包族处理器都继承它，因此它们的头文件不必拖入 `server/application/MinecraftServer.hpp`。

## 目录结构

```
src/server/network/play/base/
├── README.md
└── PlayHandlerBase.hpp    # 提供 MinecraftServer& m_server，禁拷贝/移动
```

## 内部模块关系

```
play/base/PlayHandlerBase
        ▲
        ├── MovementHandler        （play/）
        ├── BlockActionHandler     （play/）
        ├── EntityActionHandler    （play/）
        ├── ChatHandler            （play/）
        ├── PlayerStateHandler     （play/）
        ├── SessionSignalHandler   （play/）
        └── ServerPlayHandler      （play/，聚合门面，同时值持上述 6 个处理器）
```

## 上下游外部依赖关系

| 依赖 | 用途 |
|---|---|
| `mc::server::MinecraftServer`（仅前置声明） | `m_server` 成员的引用类型 |

| 依赖本目录 | 用途 |
|---|---|
| `play/` 全部处理器 | 继承以获得 `m_server` |

## 容易踩的坑

### 1. 必须持 `MinecraftServer&` 而非 `IServer&`

部分处理体要调 `MinecraftServer` 自身的纯虚（`getHeldItemForPlacement` /
`getSelectedHotbarSlot` / `setInventoryItem` / `syncPlayerInventory` /
`tryOpenCraftingContainer`），这些**不在** `IServer` 上。把成员类型"顺手优化"成 `IServer&`
会直接编译失败。

### 2. 前向声明命名空间要放对

`Player` / `ItemStack` / `Entity` 定义在 `mc` 命名空间。若在 `mc::server::net` 里写
`class Player;`，会声明出一个**同名的另一个类型** `mc::server::net::Player`，遮蔽真实类型，
报错形如 "cannot bind to a value of unrelated type 'Player' (aka 'mc::Player')"。
前向声明一律放在 `namespace mc { ... }` 中。
