#马类实体模块

包含所有马类实体的实现。

## 目录结构

```
horse/
├── AbstractHorseEntity.hpp / cpp   # 马类抽象基类，实现骑乘、驯服、装备、交互等通用功能
├── AbstractChestedHorseEntity.hpp / cpp  # 可装备箱子的马类中间层（驴、骡、羊驼）
├── CoatColors.hpp                  # 马的毛色枚举（7种）
├── CoatTypes.hpp                   # 马的花纹枚举（5种）
├── HorseEntity.hpp / cpp           # 马（35种变体：7色×5花纹）
├── DonkeyEntity.hpp / cpp          # 驴（可装备箱子，15格）
├── MuleEntity.hpp / cpp            # 骡（不育，马+驴杂交）
├── SkeletonHorseEntity.hpp / cpp   # 骷髅马（亡灵，陷阱马）
├── ZombieHorseEntity.hpp / cpp     # 僵尸马（亡灵）
├── LlamaEntity.hpp / cpp           # 羊驼（商队、吐口水）
├── TraderLlamaEntity.hpp / cpp     # 商队羊驼（随流浪商人生成，消失机制，保卫商人）
└── README.md                       # 本文件
```

## 继承层次

```
AnimalEntity
└── AbstractHorseEntity (IJumpingMount, IEquipable)
    ├── HorseEntity                  # 马（支持马铠）
    ├── AbstractChestedHorseEntity   # 可装箱子的马类中间层
    │   ├── DonkeyEntity             # 驴（15格箱子）
    │   └── MuleEntity               # 骡（15格箱子，不育）
    ├── SkeletonHorseEntity          # 骷髅马（陷阱触发）
    ├── ZombieHorseEntity            # 僵尸马
    └── LlamaEntity                  # 羊驼（支持地毯装饰）
        └── TraderLlamaEntity        # 商队羊驼
```

## 内部模块关系

- **AbstractHorseEntity** 是所有马类的核心基类，提供：
  - 骑乘系统（`IJumpingMount` 接口）
  - 驯服系统（temper 进度机制）
  - 装备系统（`IEquipable` 接口：鞍槽 + 马铠/装饰槽）
  - 交互系统（`interactMob` 处理玩家右键交互）
  - 动画状态（扬蹄、进食、张嘴）
  - 属性遗传（速度、跳跃力、生命值）

- **AbstractChestedHorseEntity** 是驴、骡、羊驼的中间层，提供箱子装备能力

- **CoatColors / CoatTypes** 是马外观的支撑类型，独立于实体类

- **各子类差异**：

| 实体 | 箱子 | 马铠/地毯 | 驯服方式 | 繁殖 | 日光燃烧 |
|------|------|-----------|----------|------|----------|
| HorseEntity | ❌ | 马铠 | 骑乘 | 金苹果/金胡萝卜 | ❌ |
| DonkeyEntity | 15格 | ❌ | 骑乘 | 与马/驴 | ❌ |
| MuleEntity | 15格 | ❌ | 骑乘 | 不育 | ❌ |
| SkeletonHorseEntity | ❌ | ❌ | 无需 | 不育 | ❌ |
| ZombieHorseEntity | ❌ | ❌ | 无需 | 不育 | ✓（Chest槽位防护） |
| LlamaEntity | 3-15格 | 地毯 | 骑乘 | 干草块 | ❌ |

## 玩家交互逻辑（interactMob）

马类实体的玩家交互逻辑按继承层次分派：

### AbstractHorseEntity::interactMob（基类逻辑）

1. **被骑乘中 / 幼年** → 交给 `AnimalEntity::interactMob`
2. **已驯服 + 潜行** → 打开马背包界面（`openInventory`）
3. **手持物品时**：
   - 先让物品自身执行交互（如 `SaddleItem::itemInteractionForEntity` 装备鞍）
   - 再尝试装备马铠/装饰到槽位 1（`equipArmor`）
4. **空手** → 让玩家骑乘（`doPlayerRide`，触发驯服流程）

### AbstractChestedHorseEntity::interactMob（箱子中间层）

1. **手持食物时** → 优先喂食
2. **未驯服时** → 让马愤怒（`makeMad`）
3. **手持箱子且未装备箱子时** → 装备箱子（`equipChest`）
4. 其余交给 `AbstractHorseEntity::interactMob`

### HorseEntity::interactMob（马特有）

1. **手持食物时** → 优先喂食
2. **未驯服时** → 让马愤怒
3. 其余交给 `AbstractHorseEntity::interactMob`

### 新增方法

| 方法 | 所在类 | 说明 |
|------|--------|------|
| `openInventory(Player&)` | AbstractHorseEntity | 打开马背包 GUI（TODO: 等 ContainerMenu 系统实现） |
| `equipArmor(Player&, ItemStack&)` | AbstractHorseEntity | 装备马铠/地毯到槽位 1 |
| `doPlayerRide(Player&)` | AbstractHorseEntity | 让玩家骑乘马匹 |
| `equipChest(Player&, ItemStack&)` | AbstractChestedHorseEntity | 装备箱子，重建背包 |
| `getChestEquipSound()` | AbstractChestedHorseEntity | 获取箱子装备音效（子类可覆写） |
| `interactMob()` | AbstractChestedHorseEntity | 箱子马交互覆写 |
| `interactMob()` | HorseEntity | 马交互覆写（食物优先+未驯服愤怒） |
| `canPerformRearing()` | AbstractHorseEntity | 是否可扬蹄（默认 true，LlamaEntity 覆写返回 false） |
| `makeHorseRear()` | AbstractHorseEntity | 扬蹄（MC 1.21.11 standIfPossible，含条件检查） |
| `clearRearing()` | AbstractHorseEntity | 清除扬蹄状态和计数器（MC 1.21.11 clearStanding） |
| `openMouth()` | AbstractHorseEntity | 张开嘴巴（仅服务端，30 tick 后自动关闭） |
| `canEatGrass()` | AbstractHorseEntity | 是否可吃草（默认 true，骷髅马/僵尸马覆写返回 false） |
| `hasOwner()` | AbstractHorseEntity | 检查是否有主人（UUID 非空） |
| `getOwner()` | AbstractHorseEntity | 通过 UUID 查找主人 LivingEntity |
| `clearOwnerUuid()` | AbstractHorseEntity | 清除主人 UUID |
| `aiStep()` | AbstractHorseEntity | AI 步进（尾巴触发、自然恢复、吃草触发/计数器） |

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

- `common/entity/entities/passive/basic/AnimalEntity.hpp` - 动物基类
- `common/entity/interfaces/IJumpingMount.hpp` - 跳跃挂载接口
- `common/entity/interfaces/IEquipable.hpp` - 装备接口
- `common/entity/ai/goal/` - AI 目标系统（SwimGoal, PanicGoal, BreedGoal 等）
- `common/world/blockentity/core/SimpleInventory.hpp` - 简单库存实现
- `common/resource/ResourceLocation.hpp` - 资源位置（音效）
- `common/item/items/armor/HorseArmorItem.hpp` - 马铠物品
- `common/item/tag/ItemTags.hpp` - 物品标签（地毯等）

### 下游依赖（依赖本模块）

- `common/entity/registry/VanillaEntities.hpp` - 实体注册
- `client/renderer/trident/entity/renderer/animal/HorseRenderer.hpp` - 马渲染器
- `client/renderer/trident/entity/renderer/animal/LlamaRenderer.hpp` - 羊驼渲染器
- `common/item/items/armor/HorseArmorItem.hpp` - 马铠物品
- `common/entity/entities/villager/VillagerEntity.cpp` - 村民（商队羊驼关联）
- `common/entity/entities/passive/tamable/WolfEntity.cpp` - 狼（羊驼防御目标）

## 容易踩的坑

### #1. 继承层次不要遗漏中间层

驴、骡、羊驼都继承自 `AbstractChestedHorseEntity`，而不是直接继承 `AbstractHorseEntity`。这个中间层负责箱子状态和库存大小计算。

### #2. 羊驼的装备槽位与其他马类不同

- 羊驼槽位 1 只能放**地毯**，不是马铠
- 羊驼**不能装备鞍**（`canEquipSaddle()` 返回 false）
- 羊驼可骑乘但**不可控制方向**（无鞍也能骑，但无法控制）

### #3. 骷髅马的陷阱机制

`TriggerSkeletonTrapGoal` 是动态注册的，只有 `setTrap(true)` 的骷髅马才会触发陷阱。雷暴天气生成的骷髅马需要正确设置陷阱状态。

`triggerTrap()` 触发时会在骷髅马位置生成一个纯视觉效果的闪电实体（`LightningBoltEntity`，`setEffectOnly(true)`），该闪电不造成伤害、不点燃方块。困难模式下额外生成 3 只骷髅马+骑手，普通/简单模式只生成 1 只骷髅骑手骑原马。

### #4. 骷髅马与僵尸马的日光燃烧行为差异

骷髅马和僵尸马在日光下的行为截然不同：

| 行为 | 骷髅马 | 僵尸马 |
|------|--------|--------|
| BURN_IN_DAYLIGHT 标签 | ✗ | ✓ |
| 阳光下燃烧 | ✗ | ✓ |
| `sunProtectionSlot()` | N/A（不燃烧） | `EquipmentSlot::Chest`（马铠槽位） |
| `canBreatheUnderwater()` | ✓ | ✗ |
| 日光燃烧实现方式 | 无 | 直接调用 `burnUndead()` |

- **骷髅马** 不在 `BURN_IN_DAYLIGHT` 标签中，不会在阳光下燃烧，不需要任何防护
- **僵尸马** 在 `BURN_IN_DAYLIGHT` 标签中，在 `tick()` 中直接调用 `burnUndead()` 处理日光燃烧
- 僵尸马的 `sunProtectionSlot()` 返回 `EquipmentSlot::Chest`（对应马铠槽位），而非默认的 `EquipmentSlot::Head`（头盔槽位），因为马类的装备槽位映射与人类不同

### #5. 驯服进度机制

驯服不是概率事件，而是进度累积：
- 每次骑乘 tick 有 1/50 概率检查驯服
- `temper += 5`，当 `random(maxTemper) < temper` 时驯服成功
- 不同马类 `maxTemper` 不同（马 100，驴 80 等）

### #6. 装备验证的多态模式

使用虚方法实现装备验证多态，不要用 instanceof 检查：
- `canEquipSaddle()` - 检查是否能装鞍
- `hasArmorSlot()` - 检查是否有第二槽位
- `isValidArmorForSlot(item)` - 检查物品是否有效（子类重写）

### #7. 商队系统的链表结构

羊驼商队是双向链表：`m_caravanHead` 指向前方，`m_caravanTail` 指向后方。商队最多 8 只，超过限制时递归深度检查会失败。

`LlamaFollowCaravanGoal` 的商队加入条件：
- 自己未被拴绳拴住且未在商队中
- 优先寻找商队链尾部（`isInCaravan() && !hasCaravanTail()`）的羊驼
- 其次寻找被拴住且无尾部的羊驼（`isLeashed() && !hasCaravanTail()`）
- 候选羊驼自身或其商队链头部必须被拴绳拴住（`_firstIsLeashed()` 递归检查）
- 被拴在栅栏柱上的羊驼在 `tick()` 中不移动跟随商队

### #8. 扬蹄动画影响乘客位置

`updatePassengerPosition()` 会根据 `prevRearingAmount` 调整乘客偏移，渲染时需要使用 `getRearingAmount(partialTicks)` 进行插值。

### #9. 跳跃提升药水效果

跳跃提升效果直接修改跳跃力度：`跳跃力度 = 基础值 + 等级 × 0.1`。这影响骑乘时的跳跃高度，需要正确处理 `Effects::JUMP_BOOST`。

### #10. 马的属性遗传公式

后代属性 = `(父本基础值 + 母本基础值 + 随机变异值) / 3`

马的随机范围：
- 生命值：15-30
- 跳跃力：0.4-1.0
- 速度：0.1125-0.3375

### #11. 状态标志位掩码

状态使用 `STATUS_PARAM`（i8 DataParameter）的位掩码存储，6 bool 真相源在 `HorseStatusComponent`：
- bit 1(2)：已驯服（`m_tame`）
- bit 2(4)：已装备鞍（`m_saddled`）
- bit 3(8)：已繁殖（`m_bred`）
- bit 4(16)：正在吃（`m_eating`）
- bit 5(32)：正在扬蹄（`m_rearing`）
- bit 6(64)：嘴张开（`m_mouthOpen`）

ECS 迁移后（批次8 B8.1）：6 个 setter（`setSaddle`/`setTame`/`setEating`/`setBred`/`setMouthOpen`/`setRearing`）写完 `HorseStatusComponent` 字段后调 `_syncStatusFlags()` 聚合成 i8 一次 `m_dataManager.set(STATUS_PARAM, ...)` 下发客户端。不要直接操作位；旧 `getHorseWatchableBoolean()`/`setHorseWatchableBoolean()`（每次 read-modify-write）已删除。

客户端 `ClientEntity::syncMetadataFromDataManager` 有 horse 分支：读 `STATUS_PARAM` i8 按 6 bit 掩码拆解写入 ClientEntity 自有的 6 horse 成员（`m_horseTamed`/`m_horseSaddled`/...）。`STATUS_FLAG_*` 常量与 `getStatusParamId()` 均为 public，供客户端解析使用。

### #12. 动画计数器系统

MC 1.21.11 的 `AbstractHorse.tick()` 和 `aiStep()` 管理以下动画计数器：

| 计数器 | 字段 | 触发 | 行为 |
|--------|------|------|------|
| 张嘴计数器 | `m_openMouthCounter` | `openMouth()` | 从 1 递增，>30 时关闭嘴巴 |
| 扬蹄计数器 | `m_jumpRearingCounter` | `makeHorseRear()` 设为 20 | 递减，≤0 时清除扬蹄 |
| 尾巴计数器 | `m_tailCounter` | `aiStep()` 中 1/200 概率 | 从 1 递增，>8 时重置 |
| 冲刺计数器 | `m_sprintCounter` | 由渲染/动画系统外部触发 | 递增，>300 时重置 |
| 吃草计数器 | `m_eatingCounter` | `aiStep()` 中吃草条件满足时 | 递增，>50 时停止吃草 |

吃草触发条件（在 `aiStep()` 中，仅服务端）：
1. `canEatGrass()` 返回 true（骷髅马/僵尸马返回 false）
2. 未在吃草 (`!isEating()`)
3. 未被骑乘 (`!hasPassengers()`)
4. 1/300 概率
5. 脚下方块为草方块 (`VanillaBlocks::GRASS_BLOCK`)

自然恢复（在 `aiStep()` 中，仅服务端）：
- 每tick 1/900 概率，且死亡时间 `deathTime() == 0`，恢复 1.0 生命值

扬蹄条件检查（MC 1.21.11 `standIfPossible()`）：
- `canPerformRearing()` 必须为 true（羊驼和骆驼返回 false）
- `canPassengerSteer() || !isClientSide()` 必须为 true

愤怒条件检查（MC 1.21.11 `makeMad()`）：
- 未在扬蹄 (`!isRearing()`)
- 非客户端 (`!isClientSide()`)

### #13. NBT 序列化

ECS 迁移后（批次8 B8.1），字段级 NBT 读写已从 `AbstractHorseEntity::addAdditionalSaveData`/`readAdditionalSaveData` 搬到 `ComponentSerializerRegistry`（`HorseComponentSerialization.cpp` 注册 4 序列化器）。`addAdditionalSaveData` override 已删除（回落基类空实现），`readAdditionalSaveData` 改薄壳（仅调基类 + `initHorseChest`，因 `initHorseChest` 需在所有组件 load 完成后按新 NBT 重置库存规模）。

| NBT 键 | 类型 | 组件（真相源） | priority | 说明 |
|--------|------|----------------|----------|------|
| `OwnerUUIDMost` | i64 | HorseTamingComponent | 0 | 主人 UUID 高 64 位 |
| `OwnerUUIDLeast` | i64 | HorseTamingComponent | 0 | 主人 UUID 低 64 位 |
| `Temper` | i32 | HorseTamingComponent | 0 | 驯服进度 |
| `JumpStrength` | f32 | HorseJumpComponent | 0 | 跳跃强度（load 后同步 AttributeMap） |
| `Tame` | i8(bool) | HorseStatusComponent | 10 | 是否已驯服（走 setter 写 STATUS_PARAM） |
| `Bred` | i8(bool) | HorseStatusComponent | 10 | 是否已繁殖 |
| `Saddle` | i8(bool) | HorseStatusComponent | 10 | 是否装备鞍 |
| `EatingHaystack` | i8(bool) | HorseStatusComponent | 10 | 是否在吃草 |
| `Speed` | f32 | HorseAttributeComponent | 20 | 移动速度（load 后同步 AttributeMap） |
| `HorseHealth` | f32 | HorseAttributeComponent | 20 | 生命值（load 后同步 AttributeMap） |

load 按 priority 升序：Taming=0/Jump=0 先 load（`ownerUuid` 联动 `setOwnerUuid` 触发 `setTame(true)` 写 STATUS_PARAM），Status=10 后 load（tame/bred/saddle/eating 走 setter，ownerUuid 联动的 setTame 覆盖幂等），Attribute=20 最后 load（speed/horseHealth 同步 AttributeMap）。

UUID 存储使用 `OwnerUUIDMost`/`OwnerUUIDLeast` 双 long 格式（与 MC 原版兼容），通过 `util::uuidFromString()` / `util::uuidToString()` 进行字符串与字节转换。布尔值以 i8(0/1) 存储，读取时使用 `nbt_helper::tryGetBool()`。

属性字段（speed/horseHealth/jumpStrength）是 NBT 真相源，`registerAttributes` 拷贝到 AttributeMap；load 后序列化器调 `setBaseValue` 同步 AttributeMap。`getSpeed()`/`getHorseHealth()` 为组件读取的公开 getter，供叶子类 `registerAttributes` 使用。

### #12. 马的 AI 目标优先级

```
优先级数字越小，优先级越高：
0 : SwimGoal（最高，水中上浮）
1 : PanicGoal + RunAroundLikeCrazyGoal（可同时触发）
2 : BreedGoal
4 : FollowParentGoal
6 : WaterAvoidingRandomWalkingGoal
7-8: LookAtGoal + LookRandomlyGoal（最低）
```

### #13. 马铠防御值

马铠不使用常规护甲计算，直接提供固定伤害减免：
- 钻石马铠：减免 12 点（6颗心）
- 金马铠：减免 6 点（3颗心）
- 铁马铠：减免 5 点（2.5颗心）

### #14. 商队羊驼的特殊机制

`TraderLlamaEntity` 是 `LlamaEntity` 的子类，具有以下特殊行为：

- **消失机制**：与流浪商人绑定，通过 `m_despawnDelay` 倒计时消失。被拴在流浪商人身上时，同步商人的消失倒计时（-1）；否则自行递减倒计时。默认 47999 tick。
- **canDespawn() 逻辑**：驯服、被拴绳拴住、有玩家骑乘时不消失。注意：被拴绳拴住（包括拴在流浪商人身上）时不被 DespawnManager 距离判断移除，仅通过 `maybeDespawn()` 定时消失。
- **防御目标**：`TraderLlamaDefendWanderingTraderGoal`（优先级1）在被拴在流浪商人身上时，当商人受到攻击会反击攻击者。
- **攻击目标**：攻击僵尸（排除僵尸猪灵）和灾厄村民（`NearestAttackableTargetGoal`，优先级2）。
- **骑乘限制**：被拴在流浪商人身上时，`interactMob()` 返回 `ActionResultType::Pass`，阻止玩家骑乘。
- **NBT 序列化**：额外存储 `DespawnDelay` 字段。
- **finalizeSpawn()**：确保消失倒计时在自然生成时被正确初始化。
- **与流浪商人的集成**：`WanderingTraderEntity::spawnLlamas()` 创建 TraderLlamaEntity 并通过 `setLeashedToEntity()` 将拴绳绑定到商人。
