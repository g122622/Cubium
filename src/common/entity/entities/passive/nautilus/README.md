# 鹦鹉螺类实体 (Nautilus)

对应 MC 1.21.11 `net.minecraft.world.entity.animal.nautilus` 包下的实体族。

## 目录结构

```
nautilus/
├── AbstractNautilusEntity.hpp/.cpp   # 抽象基类：实现可骑乘跳跃、装备栏、水中移动、冲刺、气泡粒子等通用功能
├── NautilusEntity.hpp/.cpp           # 活体鹦鹉螺：可驯服/可繁殖/可骑乘，水下/陆地/幼体音效分支
├── ZombieNautilusEntity.hpp/.cpp     # 僵尸鹦鹉螺：亡灵变体，阳光下燃烧，按生物群系选择气候变体
└── ZombieNautilusVariant.hpp         # 气候变体枚举（温带/寒冷/温暖）
```

## 内部模块关系

```
AbstractNautilusEntity (继承 TameableEntity + IJumpingMount + IEquipable)
├── NautilusEntity        (活体，可繁殖，4 路音效分支)
└── ZombieNautilusEntity  (亡灵，2 路音效分支，按生物群系选变体，sunProtectionSlot=Body)
```

- `AbstractNautilusEntity` 手动实现水生行为（`canBreatheUnderwater`/`maxAir`/`getPathWeight`），
  原因：`TameableEntity` 继承 `AnimalEntity` 而非 `WaterMobEntity`，两者父类不共享。
- `NautilusEntity::updateAirSupply` 覆写：水中恢复 300，陆地以 -20 为阈值承受干涸伤害
  （对应 MC `Nautilus.handleAirSupply`，区别于 `WaterMobEntity` 的 `drown` 伤害）。
- `ZombieNautilusEntity::finalizeSpawn` 根据生物群系 ID 选择 `ZombieNautilusVariant`，
  简化 MC 1.21.11 的 `PriorityProvider` + `SpawnContext` 注册表。

## 上下游外部依赖关系

**依赖：**
- `common/entity/entities/passive/tamable/TameableEntity` - 驯服系统基类
- `common/entity/interfaces/IJumpingMount` - 跳跃骑乘接口
- `common/entity/interfaces/IEquipable` - 装备栏接口（slot 0=鞍, slot 1=鹦鹉螺铠甲）
- `common/world/blockentity/core/SimpleInventory` - 物品栏实现
- `common/entity/damage/DamageSources::dryout()` - 鹦鹉螺干涸伤害
- `common/entity/effect/EffectType::WaterBreathing` - 骑乘者水下呼吸（简化版 BREATH_OF_THE_NAUTILUS）
- `common/world/biome/Biomes` - 僵尸鹦鹉螺变体选择
- `common/sound/SoundEvents` - ENTITY_NAUTILUS_* / ENTITY_BABY_NAUTILUS_* / ENTITY_ZOMBIE_NAUTILUS_*

**被依赖：**
- `common/entity/registry/VanillaEntities.hpp` - 实体注册（NAUTILUS + ZOMBIE_NAUTILUS）
- `common/entity/registry/VanillaEntityTypeKeys` - 实体类型指针缓存
- `common/entity/core/EntitySpawnPlacementRegistry.cpp` - 生成规则（nautilus 带 Y 范围谓词）
- `common/world/spawn/MobSpawnInfo.cpp` - 海洋生物群系 spawn 条目（ocean/lukewarmOcean/warmOcean/coldOcean/frozenOcean）
- `common/item/Items.cpp` - 鹦鹉螺刷怪蛋 + 僵尸鹦鹉螺刷怪蛋
- `client/resource/EntityTextureLoader.cpp` - 实体纹理加载（nautilus + zombie_nautilus 双纹理变体）

## 容易踩的坑

1. **`registerData()` 必须在派生类构造函数中显式调用** - C++ 虚函数在基类构造函数中不会派发到派生类，
   参考 `WolfEntity` 模式。`AbstractNautilusEntity` 的构造函数不调用 `registerData()`/`registerAttributes()`，
   `NautilusEntity`/`ZombieNautilusEntity` 构造函数需显式调用两者（参考 .cpp 文件头部注释）。

2. **僵尸鹦鹉螺在 MC 1.21.11 中分类为 MONSTER，不是 WaterCreature** - 当前项目为简化保持 `WaterCreature`，
   如需严格对齐 MC 应改为 `EntityClassification::Monster`。

3. **僵尸鹦鹉螺无 SpawnPlacements 注册（MC 原版）** - 仅作为持有三叉戟的溺尸的骑乘者生成
   （`Drowned.finalizeSpawn` 中 50% 概率），项目当前未实现此 jockey 逻辑。

4. **鹦鹉螺空气供应使用 `dryout` 伤害类型**，不是 `drown`。MC 原版 `Nautilus.handleAirSupply` 允许
   空气降到 -20 才触发伤害（共 320 tick 陆地生存时间），与 `WaterMobEntity::updateAirSupply` 的
   `shouldTakeDrowningDamage` 阈值不同。

5. **骑乘者效果简化为 `WaterBreathing`** - MC 原版使用 `BREATH_OF_THE_NAUTILUS` 效果，项目未实现该效果，
   使用 `EffectType::WaterBreathing` 替代。

6. **物品栏 NBT 序列化** - 使用 `Items` 列表 + Slot 索引模式保存完整 ItemStack（含附魔、耐久、自定义名称等），与 ChestBoatEntity、LootableContainerBlockEntity、PlayerInventory 保持一致。加载时优先读取 `Items` 列表，回退兼容旧版 `SaddleItem` 布尔标记。

7. **气泡粒子生成** - `AbstractNautilusEntity::spawnBubbles` 通过 `IWorld::addParticle(ParticleTypeId::Bubble, ...)` 生成气泡，双端执行（服务端广播，客户端本地生成），与 MC 1.21.11 原版 `level().addParticle` 语义一致。

8. **`getLookAngle()` 是 `LivingEntity` 的 protected 方法**，不是 `Entity` 的方法。在 `executeRidersJump`
   中使用时需通过 `Player&`（继承自 `LivingEntity`）访问。

9. **鹦鹉螺背包 GUI 未实现** - `openInventory` 仍为 TODO，阻塞点：`ServerWorld::openEntityContainer`
   恒返回 false（实体容器回调从未接线，setter 已移除）、`ContainerManager` 不支持实体容器、
   `NautilusContainer` 菜单类未实现、客户端 Screen 未实现。
   与 `AbstractHorseEntity::openInventory` 的 TODO 是同一阻塞点，应一起收敛。
