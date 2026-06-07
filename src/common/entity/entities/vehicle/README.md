# 车辆实体

本目录包含可乘坐的车辆实体。

## 目录结构

```
vehicle/
├── BoatEntity.hpp          # 船实体定义（水上交通工具，6种木材变体）
├── BoatEntity.cpp          # 船实体实现（浮力、控制、乘客管理）
├── MinecartEntity.hpp      # 矿车实体定义（基类 + 6种变体）
├── MinecartEntity.cpp      # 矿车实体实现（铁轨移动、变体逻辑）
└── README.md               # 本文档
```

## 内部模块关系

```
Entity (基类)
    │
    ├── BoatEntity
    │   └── 水上交通工具，不实现 IRideable（船不需要鞍）
    │
    └── AbstractMinecartEntity
        ├── RideableMinecartEntity    # 普通矿车（可乘坐）
        ├── ChestMinecartEntity       # 箱子矿车（27格库存）
        ├── FurnaceMinecartEntity     # 熔炉矿车（燃料驱动）
        ├── TNTMinecartEntity         # TNT矿车（可点燃爆炸）
        ├── HopperMinecartEntity      # 漏斗矿车（物品收集，实现 IHopper）
        └── CommandBlockMinecartEntity # 命令方块矿车
```

## 上下游外部依赖

### 上游依赖（本目录依赖的模块）

| 模块 | 用途 |
|------|------|
| `entity/core/Entity.hpp` | 实体基类 |
| `entity/core/BoostHelper.hpp` | 加速辅助类（鞍和加速状态管理） |
| `entity/interfaces/IRideable.hpp` | 可骑乘接口（船不实现，供其他骑乘实体用） |
| `entity/damage/DamageSource.hpp` | 伤害源系统 |
| `world/block/blocks/redstone/AbstractRailBlock.hpp` | 铁轨形状定义 |
| `world/blockentity/transport/IHopper.hpp` | 漏斗接口（HopperMinecartEntity 实现） |
| `world/blockentity/core/SimpleInventory.hpp` | 简单库存实现 |

### 下游依赖（依赖本目录的模块）

| 模块 | 用途 |
|------|------|
| `entity/core/VanillaEntities.hpp` | 实体类型注册 |
| `item/items/vehicle/BoatItem.hpp` | 放置船的物品 |
| `item/items/vehicle/MinecartItem.hpp` | 放置矿车的物品 |
| `world/block/dispense/` | 发射器行为（放置车辆） |
| `client/renderer/trident/entity/renderer/vehicle/` | 车辆渲染器 |
| `server/core/PacketHandler.cpp` | 骑乘网络包处理 |

## 容易踩的坑

### 1. 船与 IRideable 接口

**问题**：船不实现 `IRideable` 接口，因为船不需要鞍即可骑乘，也不支持加速功能。

**解决方案**：船直接使用 `Entity` 的乘客系统，通过 `handleInput()` 处理玩家控制。

### 2. 矿车速度限制

**问题**：不同类型矿车有不同的最大速度，容易混淆。

**要点**：
- 普通矿车：0.4 (DEFAULT_MAX_SPEED)
- 熔炉矿车：0.2（`getMaxSpeed()` 返回 0.2）
- 空中横向速度：0.4 (DEFAULT_MAX_SPEED_AIR_LATERAL)

### 3. TNT矿车伤害处理

**问题**：TNT矿车对不同伤害类型有不同反应，处理逻辑复杂。

**要点**：
- 火焰/爆炸伤害 → 点燃TNT（不掉落物品）
- 普通伤害 + 低速度(<0.01) → 掉落矿车 + TNT方块
- 普通伤害 + 高速度(≥0.01) → 碰撞爆炸
- 燃烧箭矢 → 根据箭矢速度计算爆炸威力

参考 MC 1.16.5：`TNTMinecartEntity.attackEntityFrom()` 和 `killMinecart()`

### 4. 熔炉矿车掉落逻辑

**问题**：熔炉矿车的掉落逻辑与伤害类型相关。

**要点**：
- 爆炸伤害 → 只掉落矿车
- 非爆炸伤害 → 掉落矿车 + 熔炉方块

参考 MC 1.16.5：`FurnaceMinecartEntity.killMinecart()`

### 5. 漏斗矿车红石禁用

**问题**：漏斗矿车在充能的激活铁轨或探测铁轨上应暂停工作。

**解决方案**：`onActivatorRailPass()` 中调用 `setDisabled(true)` 禁用漏斗功能。

### 6. 矿车斜坡高度调整

**问题**：矿车在斜坡上的高度调整需要精确计算。

**要点**：
- 斜坡调整值：`SLOPE_ADJUSTMENT = 0.0078125` (1/128)
- 斜坡上升：Y + 0.0625 或 Y + 0.5625

### 7. 伤害源参数传递

**问题**：`dropItem()` 方法现在需要 `DamageSource*` 参数。

**解决方案**：在 `hurt()` 方法中调用 `dropItem(&source)` 传递伤害源，用于判断掉落逻辑。非伤害导致的销毁传 `nullptr`。

### 8. 船的乘客限制

**问题**：船最多承载2名乘客，但水下状态的船不能承载乘客。

**解决方案**：`canFitPassenger()` 检查 `m_status != BoatStatus::UnderWater`。

### 9. 铁轨形状与移动

**问题**：铁轨有10种形状，需要正确处理每种形状的移动方向。

**参考**：`AbstractRailBlock` 定义了所有 `RailShape`，`_getRailDirectionVectors()` 计算方向向量。

### 10. 网络同步

**问题**：骑乘相关网络包需要在正确的时机发送。

**要点**：
- `PlayerInputPacket`：客户端→服务端，发送玩家输入
- `MoveVehiclePacket`：客户端→服务端，发送载具位置
- `VehicleMovePacket`：服务端→客户端，校正载具位置
- `SetPassengersPacket`：服务端→客户端，同步乘客列表
