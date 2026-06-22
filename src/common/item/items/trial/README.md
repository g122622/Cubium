# 试炼密室物品 (Trial Chambers)

MC 1.21+ 新增的试炼密室相关物品。

## 目录结构

```
trial/
├── MaceItem.hpp/cpp              # 重锤（下落攻击伤害加成、三种专属魔咒）
├── OminousBottleItem.hpp/cpp     # 不祥之瓶（饮用获得不祥效果）
├── OminousTrialKeyItem.hpp/cpp   # 不祥试炼钥匙
├── TrialChamberSpecialItems.hpp  # 试炼密室物品汇总头文件（锻造模板、陶片、音乐唱片等）
├── TrialKeyItem.hpp/cpp          # 试炼钥匙
├── WindChargeItem.hpp/cpp        # 风弹（右键投掷，产生风爆击退效果）
└── README.md
```

## 内部模块关系

```
Item (基类)
  ├── WindChargeItem   — 直接继承，自行实现 onItemRightClick（投掷 + 冷却）
  ├── MaceItem         — 直接继承，实现下落攻击、postHitEntity 回调
  │     ├── hitEntity()      → 砸地攻击：停止下落、播放音效、AOE击退
  │     ├── postHitEntity()  → 重置下落距离
  │     ├── canSmashAttack() → 判断砸地攻击条件
  │     ├── calculateSmashAttackDamage() → 分段伤害公式
  │     ├── getSmashAttackDamageBonus()   → 含致密魔咒的完整伤害加成
  │     └── applySmashAttackKnockback()   → AOE击退计算
  ├── OminousBottleItem — 继承 FoodItem，饮用获得不祥效果
  ├── OminousTrialKeyItem — 直接继承
  └── TrialKeyItem     — 直接继承
```

## 上下游外部依赖关系

**依赖本模块的地方：**
- `Items::initialize()` — 注册所有试炼密室物品
- `DispenseItemBehaviorRegistry` — 风弹发射器行为注册
- `BreezeEntity::shootWindCharge()` — 旋风人发射风弹
- `Player::attack()` — 重锤下落攻击伤害加成、风爆弹起效果
- `AttackContext::calculateFinalDamage()` — 破甲魔咒护甲削减

**本模块依赖：**
- `Item` / `ItemStack` / `ItemProperties` — 物品基类
- `WindChargeEntity` — 风弹弹射物实体
- `CooldownTracker` — 物品冷却系统
- `IWorld` — 世界接口（spawnEntity、playSound、getEntitiesInAABB）
- `Player` / `LivingEntity` / `Entity` — 实体系统
- `EnchantmentHelper` / `DensityEnchantment` / `WindBurstEnchantment` — 魔咒查询
- `SoundEvents` / `SoundCategory` — 音效系统
- `AxisAlignedBB` — 击退范围计算
- `DamageSources` — MaceSmash 伤害类型

## 容易踩的坑

1. **WindChargeItem 不继承 ThrowableItem**：风弹物品直接继承 `Item` 并自行实现 `onItemRightClick`，因为它需要冷却机制（10 tick），而 `ThrowableItem` 不支持冷却。

2. **风弹实体由发射者类型区分参数**：`WindChargeEntity` 通过检查 `getShooter()` 的实体类型来区分玩家风弹（爆炸半径 1.2、击退 1.22）和旋风人风弹（爆炸半径 3.0、击退 1.0），无需两个实体类。

3. **投掷散布不同**：WindChargeItem 使用 `THROW_INACCURACY = 1.0f`（有散布），而雪球/鸡蛋/末影珍珠等使用 `ThrowableItem` 默认的 `0.0f`（无散布），更接近原版行为。

4. **重锤下落攻击分段伤害公式**：砸地攻击伤害不是简单线性，而是三段函数：0~3格(4*fd)、3~8格(12+2*(fd-3))、8格以上(22+(fd-8))，必须使用 `MaceItem::calculateSmashAttackDamage()` 计算而非自行推导。

5. **重锤下落攻击不触发普通暴击**：`Player::attack()` 中重锤下落攻击时跳过普通暴击判定（`!isMaceSmashAttack &&`），因为下落攻击有自己的伤害加成机制。

6. **DAMAGE_EXCLUSIVE 互斥组**：锋利/亡灵杀手/节肢杀手/穿刺/致密/破甲六者互斥，但风爆与任何伤害魔咒都不互斥。互斥检查需要在双方的 `isCompatibleWith()` 中都添加。

7. **破甲修改的是护甲有效率而非独立减伤**：破甲在 `AttackContext::calculateFinalDamage()` 中修改护甲有效率（每级 -0.15），不是独立的伤害加成，计算时需 clamp 到 [0, 1]。
