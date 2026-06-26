# 马类实体模块

包含所有马类实体的实现。

## 目录结构

```
horse/
├── AbstractHorseEntity.hpp/cpp   # 马类抽象基类，实现骑乘、驯服、装备等通用功能
├── AbstractChestedHorseEntity.hpp # 可装备箱子的马类中间层（驴、骡、羊驼）
├── CoatColors.hpp                # 马的毛色枚举（7种）
├── CoatTypes.hpp                 # 马的花纹枚举（5种）
├── HorseEntity.hpp/cpp           # 马（35种变体：7色×5花纹）
├── DonkeyEntity.hpp/cpp          # 驴（可装备箱子，15格）
├── MuleEntity.hpp/cpp            # 骡（不育，马+驴杂交）
├── SkeletonHorseEntity.hpp/cpp   # 骷髅马（亡灵，陷阱马）
├── ZombieHorseEntity.hpp/cpp     # 僵尸马（亡灵）
├── LlamaEntity.hpp/cpp           # 羊驼（商队、吐口水）
├── TraderLlamaEntity.hpp         # 商队羊驼（随流浪商人生成）
└── README.md                     # 本文件
```

## 继承层次

```
AnimalEntity
└── AbstractHorseEntity (IJumpingMount, IEquipable)
    ├── HorseEntity               # 马（支持马铠）
    ├── AbstractChestedHorseEntity # 可装箱子的马类中间层
    │   ├── DonkeyEntity          # 驴（15格箱子）
    │   └── MuleEntity            # 骡（15格箱子，不育）
    ├── SkeletonHorseEntity       # 骷髅马（陷阱触发）
    ├── ZombieHorseEntity         # 僵尸马
    └── LlamaEntity               # 羊驼（支持地毯装饰）
        └── TraderLlamaEntity     # 商队羊驼
```

## 内部模块关系

- **AbstractHorseEntity** 是所有马类的核心基类，提供：
  - 骑乘系统 (`IJumpingMount` 接口)
  - 驯服系统（temper 进度机制）
  - 装备系统 (`IEquipable` 接口：鞍槽 + 马铠/装饰槽)
  - 动画状态（扬蹄、进食、张嘴）
  - 属性遗传（速度、跳跃力、生命值）

- **AbstractChestedHorseEntity** 是驴、骡、羊驼的中间层，提供箱子装备能力

- **CoatColors/CoatTypes** 是马外观的支撑类型，独立于实体类

- **各子类差异**：
  | 实体 | 箱子 | 马铠/地毯 | 驯服方式 | 繁殖 | 日光燃烧 |
  |------|------|----------|----------|------|----------|
  | HorseEntity | ❌ | 马铠 | 骑乘 | 金苹果/金胡萝卜 | ❌ |
  | DonkeyEntity | 15格 | ❌ | 骑乘 | 与马/驴 | ❌ |
  | MuleEntity | 15格 | ❌ | 骑乘 | 不育 | ❌ |
  | SkeletonHorseEntity | ❌ | ❌ | 无需 | 不育 | ❌ |
  | ZombieHorseEntity | ❌ | ❌ | 无需 | 不育 | ✓（Chest槽位防护） |
  | LlamaEntity | 3-15格 | 地毯 | 骑乘 | 干草块 | ❌ |

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

- `common/entity/entities/passive/basic/AnimalEntity.hpp` - 动物基类
- `common/entity/interfaces/IJumpingMount.hpp` - 跳跃挂载接口
- `common/entity/interfaces/IEquipable.hpp` - 装备接口
- `common/entity/ai/goal/` - AI 目标系统（SwimGoal, PanicGoal, BreedGoal 等）
- `common/world/blockentity/core/SimpleInventory.hpp` - 简单库存实现
- `common/resource/ResourceLocation.hpp` - 资源位置（音效）

### 下游依赖（依赖本模块）

- `common/entity/core/VanillaEntities.hpp` - 实体注册
- `client/renderer/trident/entity/renderer/animal/HorseRenderer.hpp` - 马渲染器
- `client/renderer/trident/entity/renderer/animal/LlamaRenderer.hpp` - 羊驼渲染器
- `common/item/items/armor/HorseArmorItem.hpp` - 马铠物品
- `common/entity/entities/villager/VillagerEntity.cpp` - 村民（商队羊驼关联）
- `common/entity/entities/passive/tamable/WolfEntity.cpp` - 狼（羊驼防御目标）

## 容易踩的坑

### 1. 继承层次不要遗漏中间层

MC 1.16.5 中驴、骡、羊驼都继承自 `AbstractChestedHorseEntity`，而不是直接继承 `AbstractHorseEntity`。这个中间层负责箱子状态和库存大小计算。

### 2. 羊驼的装备槽位与其他马类不同

- 羊驼槽位 1 只能放**地毯**，不是马铠
- 羊驼**不能装备鞍**（`canEquipSaddle()` 返回 false）
- 羊驼可骑乘但**不可控制方向**（无鞍也能骑，但无法控制）

### 3. 骷髅马的陷阱机制

`TriggerSkeletonTrapGoal` 是动态注册的，只有 `setTrap(true)` 的骷髅马才会触发陷阱。雷暴天气生成的骷髅马需要正确设置陷阱状态。

`triggerTrap()` 触发时会在骷髅马位置生成一个纯视觉效果的闪电实体（`LightningBoltEntity`，`setEffectOnly(true)`），该闪电不造成伤害、不点燃方块。困难模式下额外生成 3 只骷髅马+骑手，普通/简单模式只生成 1 只骷髅骑手骑原马。

### 4. 骷髅马与僵尸马的日光燃烧行为差异

MC 1.16.5 中，骷髅马和僵尸马在日光下的行为截然不同：

| 行为 | 骷髅马 | 僵尸马 |
|------|--------|--------|
| BURN_IN_DAYLIGHT 标签 | ✗ | ✓ |
| 阳光下燃烧 | ✗ | ✓ |
| `sunProtectionSlot()` | N/A（不燃烧） | `EquipmentSlot::Chest`（马铠槽位） |
| `canBreatheUnderwater()` | ✓ | ✗ |
| 日光燃烧实现方式 | 无 | 直接调用 `burnUndead()` |

- **骷髅马**不在 `BURN_IN_DAYLIGHT` 标签中，不会在阳光下燃烧，不需要任何防护
- **僵尸马**在 `BURN_IN_DAYLIGHT` 标签中，在 `tick()` 中直接调用 `burnUndead()` 处理日光燃烧
- 僵尸马的 `sunProtectionSlot()` 返回 `EquipmentSlot::Chest`（对应马铠槽位），而非默认的 `EquipmentSlot::Head`（头盔槽位），因为马类的装备槽位映射与人类不同

### 5. 驯服进度机制

驯服不是概率事件，而是进度累积：
- 每次骑乘 tick 有 1/50 概率检查驯服
- `temper += 5`，当 `random(maxTemper) < temper` 时驯服成功
- 不同马类 `maxTemper` 不同（马 100，驴 80 等）

### 6. 装备验证的多态模式

MC 1.16.5 使用虚方法实现装备验证多态，不要用 instanceof 检查：
- `canEquipSaddle()` - 检查是否能装鞍
- `hasArmorSlot()` - 检查是否有第二槽位
- `isValidArmorForSlot(item)` - 检查物品是否有效（子类重写）

### 7. 商队系统的链表结构

羊驼商队是双向链表：`m_caravanHead` 指向前方，`m_caravanTail` 指向后方。商队最多 8 只，超过限制时 `joinCaravan()` 会失败。

### 8. 扬蹄动画影响乘客位置

`updatePassengerPosition()` 会根据 `prevRearingAmount` 调整乘客偏移，渲染时需要使用 `getRearingAmount(partialTicks)` 进行插值。

### 9. 跳跃提升药水效果

MC 1.16.5 中跳跃提升效果直接修改跳跃力度：`跳跃力度 = 基础值 + 等级 × 0.1`。这影响骑乘时的跳跃高度，需要正确处理 `Effects::JUMP_BOOST`。

### 10. 马的属性遗传公式

后代属性 = `(父本基础值 + 母本基础值 + 随机变异值) / 3`

马的随机范围：
- 生命值：15-30
- 跳跃力：0.4-1.0
- 速度：0.1125-0.3375

### 11. 状态标志位掩码

状态使用 `STATUS_PARAM` 的位掩码存储：
- bit 1 (2): 已驯服
- bit 2 (4): 已装备鞍
- bit 3 (8): 已繁殖
- bit 4 (16): 正在吃
- bit 5 (32): 正在扬蹄
- bit 6 (64): 嘴张开

使用 `getHorseWatchableBoolean()` 和 `setHorseWatchableBoolean()` 操作，不要直接操作位。

### 12. 马的 AI 目标优先级

```
优先级数字越小，优先级越高：
0: SwimGoal（最高，水中上浮）
1: PanicGoal + RunAroundLikeCrazyGoal（可同时触发）
2: BreedGoal
4: FollowParentGoal
6: WaterAvoidingRandomWalkingGoal
7-8: LookAtGoal + LookRandomlyGoal（最低）
```

### 13. 马铠防御值

马铠不使用常规护甲计算。MC 1.16.5 中马铠直接提供固定伤害减免：
- 钻石马铠：减免 12 点（6颗心）
- 金马铠：减免 6 点（3颗心）
- 铁马铠：减免 5 点（2.5颗心）
