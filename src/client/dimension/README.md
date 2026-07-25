# Client Dimension 模块

客户端维度管理，处理维度状态和维度切换。

## 目录结构

```
client/dimension/
├── ClientDimensionManager.hpp  # 客户端维度管理器，维护当前维度状态和维度切换流程
├── ClientDimensionManager.cpp  # 实现
└── README.md                   # 本文档
```

## 内部模块关系

本模块非常简单，仅包含一个核心类 `ClientDimensionManager`：

- `ClientDimensionInfo`：存储从服务器接收的维度信息（ID、名称、天空光照、天花板、环境光）
- `TransitionState`：维度切换状态机（None → Leaving → Loading → Entering → None）

## 上下游外部依赖关系

### 依赖的上游模块

| 模块 | 用途 |
|------|------|
| `common/world/dimension/DimensionManager.hpp` | 维度ID常量（OVERWORLD、NETHER、THE_END） |
| `common/world/dimension/DimensionType.hpp` | 维度类型属性（名称、天空光照、天花板等），单一真相来源 |

### 被下游模块依赖

本模块被客户端网络层使用：
- `ClientPlayVisitor` 接收 `ir::play::Login`（含 `spawnInfo.dimension`）后调用 `initialize()` 初始化可用维度列表
- `ClientPlayVisitor` 接收 `ir::play::Respawn` 后调用 `beginDimensionChange()` 开始维度切换

### 与服务端的对应关系

| 服务端 | 客户端 |
|--------|--------|
| `ServerDimensionManager` | `ClientDimensionManager` |
| 持有 `Dimension` 实例，管理多维度世界 | 只持有 `DimensionType` 信息，管理当前维度状态 |
| 广播维度切换（`ir::play::Respawn`） | 接收并处理维度切换 |
| 接收切换确认 | 回送确认 |

## 容易踩的坑

1. **维度属性的单一真相来源**：所有维度属性（名称、天空光照、天花板、环境光等）在 `DimensionType` 类中定义。`ClientDimensionManager::initialize(const std::vector<DimensionId>&)` 方法内部调用 `DimensionType::fromId()` 获取属性，确保维度属性不会在多处重复定义。

2. **维度ID值**：MC 1.16.5 的维度ID分别是主世界=0、下界=-1、末地=1，不要混淆。

3. **维度切换状态机**：状态转换顺序为 `None → Leaving → Loading → Entering → None`，渲染层需要根据 `needsRenderReset()` 判断是否清除区块缓存。

4. **getDimensionType() 返回静态变量指针**：该方法使用函数内静态变量存储预定义的维度类型实例，避免动态内存分配。对于未知的维度ID返回 `nullptr`，调用者需要检查返回值。
