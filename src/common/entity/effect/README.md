# 状态效果系统 (Effect System)

## 目录结构

```
effect/
├── EffectType.hpp/cpp           # 效果类型枚举和工具函数
├── EffectInstance.hpp/cpp       # 效果实例（持续时间、等级）
├── EffectManager.hpp/cpp        # 效果管理器（实体身上的效果集合）
├── EffectAttributeModifiers.hpp/cpp # 效果属性修改器定义
└── README.md                    # 本文档
```

## 核心类

### EffectType

效果类型枚举，定义所有MC 1.16.5状态效果：

```cpp
enum class EffectType : u8 {
    Speed = 1,              // 速度
    Slowness = 2,           // 缓慢
    Haste = 3,              // 急迫
    MiningFatigue = 4,      // 挖掘疲劳
    Strength = 5,           // 力量
    InstantHealth = 6,      // 瞬间治疗
    InstantDamage = 7,      // 瞬间伤害
    JumpBoost = 8,          // 跳跃提升
    Nausea = 9,             // 反胃
    Regeneration = 10,      // 生命恢复
    Resistance = 11,        // 抗性提升
    FireResistance = 12,    // 防火
    WaterBreathing = 13,    // 水下呼吸
    Invisibility = 14,      // 隐身
    Blindness = 15,         // 失明
    NightVision = 16,       // 夜视
    Hunger = 17,            // 饥饿
    Weakness = 18,          // 虚弱
    Poison = 19,            // 中毒
    Wither = 20,            // 凋零
    HealthBoost = 21,       // 生命提升
    Absorption = 22,        // 伤害吸收
    Saturation = 23,        // 饱和
    Levitation = 25,        // 漂浮
    Luck = 26,              // 幸运
    BadLuck = 27,           // 霉运
    SlowFalling = 28,       // 缓降
    ConduitPower = 29,      // 潮涌能量
    DolphinsGrace = 30,     // 海豚的恩惠
    BadOmen = 31,           // 不祥之兆
    HeroOfTheVillage = 32,  // 村庄英雄
};
```

### EffectInstance

效果实例，包含持续时间、等级等属性：

```cpp
EffectInstance effect(
    EffectType::Speed,    // 效果类型
    600,                  // 持续时间（tick）
    1,                    // 等级（0 = I, 1 = II）
    false,                // 是否环境效果
    true,                 // 是否显示粒子
    true                  // 是否显示图标
);

// 获取属性
effect.type();          // EffectType
effect.duration();      // 持续时间
effect.amplifier();     // 等级（0-based）
effect.getEffectLevel(); // 等级（1-based，用于显示）
effect.isExpired();     // 是否过期
effect.isPermanent();   // 是否永久（duration < 0）

// 更新
effect.tick(entity);    // 每tick调用，返回是否仍有效
effect.merge(other);    // 合并另一个效果
effect.apply(entity);   // 应用效果（添加时）
effect.remove(entity);  // 移除效果（移除时）
```

### EffectManager

管理实体身上的所有效果：

```cpp
// 添加效果
manager.addEffect(EffectInstance(EffectType::Speed, 200, 0), entity);

// 检查效果
manager.hasEffect(EffectType::Speed);
manager.getEffect(EffectType::Speed);
manager.getEffectLevel(EffectType::Speed);

// 移除效果
manager.removeEffect(EffectType::Speed, entity);
manager.removeAllEffects(entity);

// 更新（在LivingEntity::tick()中调用）
manager.tick(entity);
```

## 效果属性修改器

某些效果会修改实体属性：

| 效果 | 属性 | 操作 | 基础值 |
|------|------|------|--------|
| 速度 | MOVEMENT_SPEED | MultiplyTotal | +20%/级 |
| 缓慢 | MOVEMENT_SPEED | MultiplyTotal | -15%/级 |
| 急迫 | ATTACK_SPEED | MultiplyTotal | +10%/级 |
| 挖掘疲劳 | ATTACK_SPEED | MultiplyTotal | -10%/级 |
| 力量 | ATTACK_DAMAGE | Addition | +3.0/级 |
| 跳跃提升 | JUMP_BOOST | Addition | +0.1/级 |
| 虚弱 | ATTACK_DAMAGE | Addition | -4.0/级 |
| 生命提升 | MAX_HEALTH | Addition | +4.0/级 |
| 幸运 | LUCK | Addition | +1.0/级 |
| 霉运 | LUCK | Addition | -1.0/级 |

## 效果Tick逻辑

| 效果 | 逻辑 | 实现状态 |
|------|------|---------|
| 生命恢复 | 每 50/(level+1) tick 治疗 1 HP | ✅ 已实现 |
| 中毒 | 每 25/(level+1) tick 造成 1 HP 伤害（不致死） | ✅ 已实现 |
| 凋零 | 每 40/(level+1) tick 造成 1 HP 伤害 | ✅ 已实现 |
| 饥饿 | 每tick增加饥饿消耗 `exhaustion += 0.005 * (level+1)` | ✅ 已实现 |
| 缓降 | 减少摔落速度，取消摔落伤害（在物理tick处理） | ⏳ 框架完成 |
| 潮涌能量 | 水下呼吸+挖掘速度+视野（在水下时激活） | ⏳ 框架完成 |
| 海豚的恩惠 | 增加游泳速度（在水中时激活） | ⏳ 框架完成 |

### 饥饿效果实现详情

饥饿效果（Hunger）每tick增加玩家的饥饿消耗值：

```cpp
// MC 1.16.5: EffectInstance.performEffect() 第455-459行
// exhaustion += 0.005F * (amplifier + 1)
if (auto* player = dynamic_cast<Player*>(&entity)) {
    player->addExhaustion(0.005f * static_cast<f32>(m_amplifier + 1));
}
```

消耗值对照表：
| 效果等级 | amplifier | 每tick消耗 |
|---------|-----------|-----------|
| 饥饿 I | 0 | 0.005 |
| 饥饿 II | 1 | 0.010 |
| 饥饿 III | 2 | 0.015 |

## 水域更新效果

### 缓降 (Slow Falling)

- 无属性修改
- 效果：减少摔落速度，取消摔落伤害
- 在 `LivingEntity::travel()` 中检测

### 潮涌能量 (Conduit Power)

- 无属性修改
- 效果：水下呼吸+挖掘速度+视野
- 需要在水下时激活

### 海豚的恩惠 (Dolphins Grace)

- 无属性修改
- 效果：增加游泳速度
- 需要在水中时激活

## 使用示例

```cpp
// 给实体添加速度 II 效果（30秒）
entity.addEffect(EffectInstance(
    EffectType::Speed,
    600,  // 30秒 = 600 tick
    1     // II级 = amplifier 1
));

// 检查效果等级
i32 level = entity.getEffectLevel(EffectType::Speed);
if (level > 0) {
    // 实体有速度效果
}

// 移除效果
entity.removeEffect(EffectType::Speed);

// 检查是否有有益效果
if (entity.effectManager().hasBeneficialEffect()) {
    // 实体至少有一个有益效果
}
```

## 与MC 1.16.5的差异

1. **瞬间效果**：瞬间治疗/伤害在tick中处理，MC中在添加时立即执行
2. **属性修改**：MC使用AttributeModifier的UUID来管理，这里简化为字符串ID
3. **隐藏效果**：MC的EffectInstance支持hiddenEffects，用于效果升级时的平滑过渡，暂未实现

## 待完善

- [ ] 瞬间效果立即执行逻辑
- [ ] 隐藏效果（效果升级过渡）
- [ ] 效果粒子生成
- [ ] 效果图标显示
- [ ] NBT序列化/反序列化
- [ ] 药水物品集成
