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

**维度切换状态**:
```
None → Leaving → Loading → Entering → None
```

**使用示例**:
```cpp
ClientDimensionManager dimManager;

// 初始化
dimManager.initialize({0, 1, 2});  // 主世界、下界、末地

// 开始维度切换
dimManager.beginDimensionChange(DimensionManager::NETHER, Vector3d(100, 64, 200));

// 完成切换
dimManager.completeDimensionChange();

// 获取当前维度信息
DimensionId dim = dimManager.currentDimension();
auto type = dimManager.currentDimensionType();
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
