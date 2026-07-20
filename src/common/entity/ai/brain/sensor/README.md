# Sensor 传感器系统

Brain AI 的环境感知模块，周期性扫描周围环境并将结果写入记忆模块。

## 目录结构

```
sensor/
├── Sensor.hpp               # 传感器模板基类（周期更新、记忆声明）
├── SensorType.hpp           # 传感器类型工厂（按名称创建实例）
├── Sensors.hpp              # 12种传感器声明
└── Sensors.cpp              # 传感器实现
```

## 传感器列表

| 传感器 | 间隔 | 写入记忆 | 说明 |
|--------|------|----------|------|
| `NearestPlayersSensor<E>` | 20 tick | NEAREST_PLAYERS, NEAREST_VISIBLE_PLAYER, NEAREST_VISIBLE_TARGETABLE_PLAYER | 检测附近玩家，过滤旁观/创造模式 |
| `NearestVisibleLivingEntitySensor<E>` | 可配置 | VISIBLE_MOBS, NEAREST_VISIBLE_NEMESIS | 检测可见生物 |
| `HurtBySensor<E>` | 1 tick | HURT_BY, HURT_BY_ENTITY | 检测最近受到的伤害来源 |
| `MobSensor<E>` | 可配置 | MOBS | 检测附近 MobEntity，仅存储列表不判断敌对 |
| `VillagerHostilesSensor<E>` | 20 tick | MOBS, NEAREST_HOSTILE | 村民专用敌对检测，使用精确实体类型到距离映射 |
| `WorkStationSensor<E>` | 40 tick | JOB_SITE, POTENTIAL_JOB_SITE | 根据村民职业动态搜索工作站POI；有职业村民搜索对应工作站，无职业村民搜索所有可获取工作站，傻子不搜索 |
| `VillagePoiSensor<E>` | 40 tick | HOME, MEETING_POINT, NEAREST_BED | 检测村民家/集会点 |
| `BabySensor<E>` | 20 tick | VISIBLE_VILLAGER_BABIES, NEAREST_VISIBLE_ADULT | 检测附近幼年/成年实体 |
| `AvoidEntitySensor<E>` | 可配置 | AVOID_TARGET, NEAREST_REPELLENT | 通用避险传感器，使用 IMob 标记接口判断敌对 |
| `TemptingPlayerSensor<E>` | 可配置 | TEMPTING_PLAYER | 检测手持可诱惑物品的最近玩家，用于 TemptTask |
| `InteractableDoorsSensor<E>` | 可配置 | INTERACTABLE_DOORS, OPENED_DOORS | 扫描实体附近的木门，用于 InteractWithDoorTask |
| `OwnerHurtBySensor<E>` | 1 tick | OWNER_HURT_BY | 检测驯服动物主人是否被攻击，用于 ProtectOwnerTask |

## 敌对检测机制

### VillagerHostilesSensor（村民专用）

参考原版 `VillagerHostilesSensor`，使用硬编码的实体类型到检测距离映射，而非统一判断所有 MobEntity 为敌对。村民检测以下实体：

| 实体类型 | 检测距离（方块） |
|----------|------------------|
| 溺尸 (Drowned) | 8 |
| 唤魔者 (Evoker) | 12 |
| 尸壳 (Husk) | 8 |
| 幻术师 (Illusioner) | 12 |
| 掠夺者 (Pillager) | 15 |
| 劫掠兽 (Ravager) | 12 |
| 恼鬼 (Vex) | 8 |
| 卫道士 (Vindicator) | 10 |
| 疣猪兽 (Zoglin) | 10 |
| 僵尸 (Zombie) | 8 |
| 僵尸村民 (Zombie Villager) | 8 |

注意：苦力怕、骷髅、蜘蛛、女巫、末影人等 **不在** 村民的敌对列表中。

### AvoidEntitySensor（通用）

默认实现使用 `IMob` 标记接口判断敌对生物（即所有继承 `MonsterEntity` 的实体）。玩家在创造/旁观模式下不触发避险。特定实体的避险逻辑应通过专门的传感器或 `AvoidEntityGoal` 配合谓词来实现。

### MobSensor（通用附近实体）

仅收集附近的 MobEntity 列表，**不**设置 NEAREST_HOSTILE 记忆。敌对判断应由专用传感器（如 VillagerHostilesSensor）负责。

## 新增传感器说明

### TemptingPlayerSensor（诱惑玩家传感器）

参考 MC 原版 `TemptingSensor`，检测附近手持诱惑物品的玩家，将最近的诱惑玩家写入 TEMPTING_PLAYER 记忆。

- 构造参数：`ItemPredicate`（判断物品是否为诱惑物品的谓词）、`range`（检测范围，默认 10.0f）、`interval`（更新间隔，默认 20 tick）
- 过滤条件：排除旁观者模式玩家、排除骑乘实体的玩家、要求实体可见、检查主手和副手物品
- 用途：配合 `TemptTask` 实现动物被食物吸引的行为

### InteractableDoorsSensor（可交互门传感器）

扫描实体附近的木门方块，将可交互的门位置写入 INTERACTABLE_DOORS 记忆。

- 构造参数：`range`（检测范围，默认 4.0f）、`interval`（更新间隔，默认 20 tick）
- 检测逻辑：仅扫描木门（使用 `DoorBlock::isWooden()` 过滤铁门），仅登记门下半部分（DOUBLE_BLOCK_HALF == Lower），仅当实体 `canEnterDoors()` 为 true 时扫描
- 初始化 OPENED_DOORS 记忆为空集合
- 用途：配合 `InteractWithDoorTask` 实现实体自动开关门

### OwnerHurtBySensor（主人受伤传感器）

检测驯服动物的主人是否受到攻击，将攻击者写入 OWNER_HURT_BY 记忆。

- 仅适用于 TameableEntity 子类
- 如果实体未驯服或主人不存在/已死亡，清除 OWNER_HURT_BY 记忆
- 不让宠物攻击自己或主人
- TTL 100 tick（5秒），与 HurtBySensor 一致
- 用途：配合 `ProtectOwnerTask` 实现宠物保护主人的行为

## 上下游外部依赖关系

**被依赖（下游）**：
- `entity/entities/villager/VillagerEntity.hpp` - 村民使用传感器系统
- `entity/entities/passive/tamable/` - 驯服动物使用 OwnerHurtBySensor

**依赖（上游）**：
- `entity/core/MobEntity.hpp` - 生物实体基类
- `entity/core/LivingEntity.hpp` - 活体实体基类
- `entity/registry/VanillaEntityTypeKeys.hpp` - 实体类型指针缓存（VillagerHostilesSensor 使用）
- `entity/interfaces/IMob.hpp` - 敌对标记接口（AvoidEntitySensor 使用）
- `entity/entities/passive/tamable/TameableEntity.hpp` - 驯服实体接口（OwnerHurtBySensor 使用）
- `world/IWorld.hpp` - 世界访问

## 容易踩的坑

### 1. VillagerHostilesSensor 依赖 VanillaEntityTypeKeys 初始化

`VanillaEntityTypeKeys` 中的指针在 `VanillaEntities::registerAll()` 之后才有值。如果在实体注册前使用 `VillagerHostilesSensor`，距离映射表中的类型指针将为 nullptr，导致所有敌对生物都无法被检测到。

### 2. MobSensor 不再设置 NEAREST_HOSTILE

`MobSensor` 仅收集 MOBS 列表。如果某个实体需要 NEAREST_HOSTILE 记忆，必须使用专用传感器（如 VillagerHostilesSensor），而不是 MobSensor。

### 3. 传感器更新频率

传感器的构造参数是更新间隔（tick），不要设置过小。例如 `NearestPlayersSensor` 默认 20 tick 更新一次。

### 4. OwnerHurtBySensor 仅适用于 TameableEntity

OwnerHurtBySensor 使用 `dynamic_cast<TameableEntity*>` 检查实体是否可驯服。非 TameableEntity 子类将始终清除 OWNER_HURT_BY 记忆。

### 5. TemptingPlayerSensor 需要自定义谓词

TemptingPlayerSensor 构造时需要传入 `ItemPredicate`（`std::function<bool(const ItemStack&)>`），用于判断玩家手持的物品是否为诱惑物品。不同动物有不同的食物偏好，需要在使用时传入对应的谓词。
