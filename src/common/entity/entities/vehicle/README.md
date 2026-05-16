# 车辆实体

本目录包含可乘坐的车辆实体。

## 目录结构

```
vehicle/
├── BoatEntity.hpp/cpp       # 船实体
├── MinecartEntity.hpp/cpp   # 矿车实体（包含多种变体）
└── README.md                # 本文档
```

## 实体列表

| 实体 | 说明 | 变体 |
|------|------|------|
| BoatEntity | 船 | 橡木、云杉、白桦、丛林、金合欢、深色橡木 |
| RideableMinecartEntity | 普通矿车 | - |
| ChestMinecartEntity | 箱子矿车 | 带库存 |
| FurnaceMinecartEntity | 熔炉矿车 | 可添加燃料 |
| TNTMinecartEntity | TNT矿车 | 可被激活爆炸 |
| HopperMinecartEntity | 漏斗矿车 | 可收集物品 |
| CommandBlockMinecartEntity | 命令方块矿车 | 可执行命令 |

## 接口继承

- **BoatEntity**: `Entity`, `IRideable`
- **AbstractMinecartEntity**: `Entity`
- **HopperMinecartEntity**: `AbstractMinecartEntity`, `IHopper`

## 骑乘系统

### IRideable 接口

所有可骑乘实体实现 `IRideable` 接口（定义在 `entity/interfaces/IRideable.hpp`）：

```cpp
class IRideable {
public:
    virtual bool hasSaddle() const = 0;
    virtual void setSaddle(bool saddle) = 0;
    virtual void onPlayerStartRiding(Player* player) = 0;
    virtual void onPlayerStopRiding(Player* player) = 0;
    virtual f32 getSteeringSpeed() const = 0;
    virtual bool boost() = 0;
    virtual i32 getBoostTime() const = 0;
    virtual void setBoostTime(i32 time) = 0;
    virtual bool canBeSteered() const = 0;
    virtual void travelTowards(const Vector3& travelVec) = 0;
    
    // 静态辅助方法
    static void ride(IRideable& rideable, BoostHelper& helper, const Vector3& travelVec);
};
```

### BoostHelper 辅助类

`BoostHelper`（定义在 `entity/core/BoostHelper.hpp`）管理可骑乘实体的鞍和加速状态：

| 方法 | 说明 |
|------|------|
| `init(manager, boostTimeParam, saddledParam)` | 初始化数据管理器引用 |
| `syncFromDataManager()` | 从 EntityDataManager 同步数据 |
| `boost(Random&)` | 触发加速，返回是否成功 |
| `tick()` | 每tick更新加速状态 |
| `isBoosting()` | 是否正在加速 |
| `setSaddledFromBoolean(bool)` | 设置鞍状态（通过DataManager同步） |
| `getSaddled()` | 获取鞍状态 |
| `getBoostTime()` | 获取加速时间 |
| `setBoostTime(i32)` | 设置加速时间 |

**注意**: `BoostHelper` 现在需要与 `EntityDataManager` 集成以支持网络同步。使用前必须调用 `init()` 方法初始化。

### 网络同步

骑乘相关的网络包（定义在 `network/packet/EntityPackets.hpp`）：

| 包名 | 方向 | 说明 |
|------|------|------|
| PlayerInputPacket | C→S | 玩家输入（移动、跳跃、潜行） |
| MoveVehiclePacket | C→S | 载具位置同步 |
| VehicleMovePacket | S→C | 服务端校正载具位置 |
| EntityActionPacket | C→S | 实体动作（潜行、疾跑、马跳跃） |
| SetPassengersPacket | S→C | 乘客列表同步 |

## 船的特性

- 水上行驶
- 可被玩家控制方向
- 受水流影响
- 碰撞推动实体
- 6种木材变体
- 伤害系统（碰撞、摔落、岩浆）

## 矿车的特性

### 基础功能
- 铁轨行驶（支持10种铁轨形状）
- 受动力轨道加速
- 受激活轨道触发
- 斜坡高度调整
- 物品掉落

### 各变体特性

| 变体 | 特性 | 实现状态 |
|------|------|----------|
| RideableMinecartEntity | 可乘坐、物品掉落、激活铁轨弹出乘客 | ✅ 完成 |
| ChestMinecartEntity | 27格库存、物品掉落 | ✅ 完成 |
| FurnaceMinecartEntity | 燃料系统、自动推进、激活铁轨改变方向 | ✅ 完成 |
| TNTMinecartEntity | 激活铁轨点燃、速度影响爆炸威力 | ✅ 完成 |
| HopperMinecartEntity | 物品收集、向下传输 | ✅ 完成 |
| CommandBlockMinecartEntity | 激活铁轨执行命令 | ✅ 完成 |

### 熔炉矿车 (FurnaceMinecartEntity)
- **燃料系统**: 玩家交互添加燃料（3600 tick = 3分钟）
- **自动推进**: 有燃料时自动沿推动方向前进
- **激活铁轨**: 可改变推进方向
- **最大速度**: 0.2（普通矿车为0.4）
- **掉落逻辑**: 爆炸伤害时只掉落矿车，非爆炸伤害时掉落矿车+熔炉方块（MC 1.16.5）

### TNT矿车 (TNTMinecartEntity)
- **点燃方式**: 激活铁轨点燃，引信80 tick（4秒）
- **燃烧箭矢引爆**: MC 1.16.5 attackEntityFrom() 第67-77行
  - 燃烧箭矢命中时使用箭矢速度计算爆炸威力
  - 使用 `dynamic_cast<AbstractArrowEntity*>` 检测箭矢实体
  - 使用 `Entity::isOnFire()` 检测燃烧状态
  - 爆炸威力 = 4.0 + random(0~1) * 1.5 * min(sqrt(speedSq), 5.0)
- **爆炸威力**: 基础4.0，速度加成最大到5.0
- **爆炸模式**: Break模式（破坏方块不掉落）
- **掉落逻辑** (MC 1.16.5 killMinecart):
  - 火焰伤害 → 点燃TNT矿车（不掉落物品）
  - 爆炸伤害 → 点燃TNT矿车（不掉落物品）
  - 普通伤害 + 低速度(<0.01) → 掉落矿车 + TNT方块
  - 普通伤害 + 高速度(≥0.01) → 碰撞爆炸

### 漏斗矿车 (HopperMinecartEntity)
- **库存**: 5格（与漏斗方块相同）
- **物品吸取**: 从上方区域吸取物品实体
- **物品传输**: 向下方容器传输物品
- **冷却时间**: 4 tick
- **红石禁用**: 在充能的探测铁轨或激活铁轨上暂停工作
- **实现接口**: `IHopper`

### 命令方块矿车 (CommandBlockMinecartEntity)
- **激活方式**: 通过激活铁轨触发
- **命令存储**: 存储命令字符串
- **输出记录**: 记录上次输出和成功次数

### 铁轨系统
- AbstractRailBlock: 铁轨基类，支持10种形状
- RailBlock: 普通铁轨，自动连接
- PoweredRailBlock: 动力铁轨，红石加速
- DetectorRailBlock: 探测铁轨，矿车检测
- ActivatorRailBlock: 激活铁轨，触发矿车

### 矿车物品
- MinecartItem: 放置矿车物品
- 6种矿车物品注册（Items::MINECART等）
- 斜坡高度调整（Y + 0.0625 或 Y + 0.5625）

## 实现状态

| 组件 | 状态 |
|------|------|
| BoatEntity | ✅ 完成 |
| AbstractMinecartEntity | ✅ 完成 |
| RideableMinecartEntity | ✅ 完成 |
| ChestMinecartEntity | ✅ 完成 |
| FurnaceMinecartEntity | ✅ 完成 |
| TNTMinecartEntity | ✅ 完成 |
| HopperMinecartEntity | ✅ 完成 |
| CommandBlockMinecartEntity | ✅ 完成 |
| 铁轨逻辑 | ✅ 完成 |
| 库存系统 | ✅ 完成 |
| 爆炸系统对接 | ✅ 完成 |
| 骑乘网络包 | ✅ 完成 |
| BoostHelper | ✅ 完成 |

## 测试覆盖

测试文件位于 `tests/entity/MinecartTests.cpp`，包含：
- RailShape isAscending 辅助函数测试
- ChestMinecartEntity 库存测试
- AbstractMinecartEntity 基础功能测试
- FurnaceMinecartEntity 燃料系统测试
- TNTMinecartEntity 引信系统测试
- MinecartItem 构造测试

新增测试文件 `tests/common/entity/entities/vehicle/MinecartDropItemTest.cpp`：
- TNTMinecartDropTest.FireDamage_IgnitesTNT - 火焰伤害检测
- TNTMinecartDropTest.ExplosionDamage_IgnitesTNT - 爆炸伤害检测
- TNTMinecartDropTest.NormalDamage_DropsItems - 普通伤害掉落
- TNTMinecartDropTest.NullptrSource_HandledSafely - nullptr 安全处理
- TNTMinecartDropTest.SpeedThreshold_AffectsDrop - 速度阈值测试
- FurnaceMinecartDropTest.ExplosionDamage_NoFurnaceDrop - 爆炸伤害不掉熔炉
- FurnaceMinecartDropTest.NormalDamage_DropsFurnace - 普通伤害掉熔炉
- ChestMinecartDropTest.AlwaysDropsInventory - 箱子矿车总是掉库存
- AbstractMinecartDropTest.AllDamageTypes_CorrectClassification - 所有伤害类型分类测试
- TNTMinecartArrowTest.NonBurningArrow_DoesNotIgnite - 普通箭矢不引爆
- TNTMinecartArrowTest.BurningArrow_IgnitesTNT - 燃烧箭矢引爆
- TNTMinecartArrowTest.ArrowVelocity_AffectsExplosionPower - 箭矢速度影响爆炸威力
- TNTMinecartArrowTest.DynamicCast_IdentifiesArrowEntity - dynamic_cast 检测箭矢
- TNTMinecartArrowTest.OtherFireProjectiles_CompatibleDetection - 其他投射物兼容检测
- TNTMinecartArrowTest.SpectralArrow_CanIgnite - 光灵箭也能引爆
- TNTMinecartArrowTest.SetFire_OnlyIncreases - setFire 只增不减
- TNTMinecartArrowTest.NegativeFire_NotOnFire - 负值不算燃烧

骑乘相关网络包测试位于 `tests/network/EntityPacketsTest.cpp`：
- PlayerInputPacket 序列化/反序列化测试
- MoveVehiclePacket 序列化/反序列化测试
- VehicleMovePacket 序列化/反序列化测试
- EntityActionPacket 序列化/反序列化测试

## 伤害源检测 (2026-05-17 更新)

矿车的 `dropItem()` 方法现在接受 `DamageSource*` 参数，用于判断伤害类型：

### API 变更

```cpp
// 旧签名
virtual void dropItem();

// 新签名 (MC 1.16.5 killMinecart)
virtual void dropItem(DamageSource* source = nullptr);
```

### 使用示例

```cpp
// 在 hurt() 方法中调用
bool AbstractMinecartEntity::hurt(DamageSource& source, f32 amount) {
    // ... 受伤处理 ...
    dropItem(&source);  // 传递伤害源
}

// 伤害类型检测
void TNTMinecartEntity::dropItem(DamageSource* source) {
    bool isFire = (source != nullptr && source->isFire());
    bool isExplosion = (source != nullptr && source->isExplosion());
    
    if (!isFire && !isExplosion && speedSq < 0.01) {
        // 正常掉落
    } else {
        // 点燃 TNT
    }
}

void FurnaceMinecartEntity::dropItem(DamageSource* source) {
    bool isExplosion = (source != nullptr && source->isExplosion());
    
    if (!isExplosion) {
        // 掉落熔炉方块
    }
}
```
