# Client Dimension 模块

客户端维度管理，处理维度状态和维度切换。

## 目录结构

```
client/dimension/
├── ClientDimensionManager.hpp  # 客户端维度管理器
├── ClientDimensionManager.cpp  # 实现
└── README.md                   # 本文档
```

## 文件详解

### ClientDimensionManager.hpp/cpp

**职责**: 管理客户端维度状态，处理维度切换流程。

**主要功能**:
- 维护当前维度ID
- 处理维度切换状态机
- 提供维度类型查询
- 存储服务器发送的维度信息

**核心数据结构**:

```cpp
/**
 * @brief 客户端维度信息结构体
 *
 * 存储从服务器接收的维度信息，包含维度属性。
 */
struct ClientDimensionInfo {
    DimensionId id = 0;           ///< 维度ID (0=主世界, -1=下界, 1=末地)
    std::string name;             ///< 维度名称 (如 "minecraft:overworld")
    bool hasSkyLight = true;      ///< 是否有天空光照
    bool hasCeiling = false;      ///< 是否有天花板
    f32 ambientLight = 0.0f;      ///< 环境光照强度
};
```

**维度切换状态**:
```
None → Leaving → Loading → Entering → None
```

**使用示例**:
```cpp
ClientDimensionManager dimManager;

// 使用完整维度信息初始化
std::vector<ClientDimensionInfo> infos = {
    {0, "minecraft:overworld", true, false, 0.0f},
    {-1, "minecraft:the_nether", false, true, 0.1f},
    {1, "minecraft:the_end", false, false, 0.0f}
};
dimManager.initialize(infos);

// 或使用ID列表初始化（自动推断属性）
dimManager.initialize({0, -1, 1});

// 开始维度切换
dimManager.beginDimensionChange(DimensionManager::NETHER, Vector3d(100, 64, 200));

// 完成切换
dimManager.completeDimensionChange();

// 获取当前维度信息
DimensionId dim = dimManager.currentDimension();
auto type = dimManager.currentDimensionType();
const ClientDimensionInfo* info = dimManager.getDimensionInfo(dim);
```

## 维度切换流程

```
服务端发送 ChangeDimensionPacket
            │
            ▼
ClientDimensionManager::beginDimensionChange()
            │
            ▼
    TransitionState::Leaving
            │
            ├─ 清除区块缓存
            │
            ├─ 清除实体管理器
            │
            └─ 标记渲染重置
            │
            ▼
    TransitionState::Loading
            │
            ├─ 等待新维度区块数据
            │
            └─ 发送 ConfirmDimensionChangePacket
            │
            ▼
    TransitionState::Entering
            │
            ├─ 加载新区块
            │
            └─ 更新玩家位置
            │
            ▼
ClientDimensionManager::completeDimensionChange()
            │
            ▼
    TransitionState::None
```

## 与服务端的关系

| 服务端 | 客户端 |
|--------|--------|
| `ServerDimensionManager` | `ClientDimensionManager` |
| 持有 `ServerDimension` 实例 | 只持有 `DimensionType` 信息 |
| 管理多维度世界 | 只管理当前维度状态 |
| 发送 `ChangeDimensionPacket` | 接收并处理维度切换 |
| 接收 `ConfirmDimensionChangePacket` | 发送确认包 |

## 维度信息同步

服务器通过 `DimensionInfoPacket` 发送可用维度列表，客户端通过 `onDimensionInfo` 回调接收：

```cpp
callbacks.onDimensionInfo =
    [this](const std::vector<std::tuple<DimensionId, std::string, bool, bool, f32>>& dimensions) {
        // 转换为 ClientDimensionInfo 格式
        std::vector<ClientDimensionInfo> infos;
        for (const auto& [id, name, hasSkyLight, hasCeiling, ambientLight] : dimensions) {
            ClientDimensionInfo info;
            info.id = id;
            info.name = name;
            info.hasSkyLight = hasSkyLight;
            info.hasCeiling = hasCeiling;
            info.ambientLight = ambientLight;
            infos.push_back(info);
        }
        m_dimensionManager.initialize(infos);
    };
```

## API 参考

### 初始化方法

| 方法 | 说明 |
|------|------|
| `initialize(const std::vector<DimensionId>&)` | 使用ID列表初始化（自动推断属性） |
| `initialize(const std::vector<ClientDimensionInfo>&)` | 使用完整维度信息初始化 |
| `reset()` | 重置所有状态 |

### 当前维度

| 方法 | 说明 |
|------|------|
| `currentDimension()` | 获取当前维度ID |
| `setCurrentDimension(DimensionId)` | 设置当前维度 |
| `currentDimensionType()` | 获取当前维度类型 |

### 维度切换

| 方法 | 说明 |
|------|------|
| `beginDimensionChange(id, pos)` | 开始维度切换 |
| `completeDimensionChange()` | 完成维度切换 |
| `cancelDimensionChange()` | 取消维度切换 |
| `transitionState()` | 获取切换状态 |
| `isChangingDimension()` | 是否正在切换 |
| `targetDimension()` | 获取目标维度 |
| `targetPosition()` | 获取目标位置 |

### 维度信息查询

| 方法 | 说明 |
|------|------|
| `availableDimensions()` | 获取可用维度ID列表 |
| `availableDimensionInfos()` | 获取完整维度信息列表 |
| `isDimensionAvailable(id)` | 检查维度是否可用 |
| `getDimensionInfo(id)` | 获取维度信息 |
| `getDimensionType(id)` | 获取维度类型 |

### 渲染设置

| 方法 | 说明 |
|------|------|
| `needsRenderReset()` | 是否需要清除渲染状态 |
| `markRenderReset()` | 标记渲染已重置 |

## 测试

测试文件位于 `tests/client/test_client_dimension_manager.cpp`，覆盖以下场景：

- 默认构造和初始化
- 使用ID列表初始化
- 使用完整维度信息初始化
- 空初始化自动添加主世界
- 重置功能
- 维度切换流程
- 取消维度切换
- 设置当前维度
- 获取维度类型
- 渲染重置标记
- 获取不存在维度的信息
