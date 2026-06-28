# Attribute 模块

实体属性系统，用于管理实体的各种可修改属性值（如生命值、移动速度、攻击力等）。

参考 MC 1.16.5 的 Attribute 系统。

## 目录结构

```
src/common/entity/attribute/
├── Attribute.hpp              # 属性基类定义（注册名称、默认值、范围）
├── AttributeModifier.hpp      # 属性修改器和操作类型枚举
├── AttributeInstance.hpp      # 属性实例（管理单个属性值和修改器，负责计算）
├── AttributeMap.hpp           # 属性映射表（管理实体的所有属性，线程安全，提供resetBaseValue/hasModifier/getModifierValue）
├── AttributeRegistry.hpp      # 属性注册表（集中管理属性定义，提供名称查询、范围查询、名称规范化，替代硬编码数据）
├── Attributes.hpp             # 标准属性定义（16种属性工厂+名称常量）
├── AttributeModifierUUIDs.hpp # 属性修改器 UUID 常量
├── EntityDefaultAttributes.hpp # 各实体类型的属性默认值常量
```

## 内部模块关系

```
Attribute.hpp (属性定义)
      ↓
AttributeModifier.hpp (修改器定义)
      ↓
AttributeInstance.hpp (实例管理+计算) ← 依赖 Attribute + AttributeModifier
      ↓
AttributeMap.hpp (属性集合管理) ← 依赖 AttributeInstance
      ↓
AttributeRegistry.hpp (属性注册表) ← 依赖 Attribute + Attributes 工厂函数，提供全局属性查询
      ↓
Attributes.hpp (标准属性工厂) ← 依赖 Attribute
```

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

- `common/core/Types.hpp`：基础类型定义（std::string, f64, u8 等）
- 标准库：`<mutex>`、`<memory>`、`<unordered_map>`、`<vector>`、`<algorithm>`

### 下游依赖（依赖本模块）

- `entity/core/LivingEntity.hpp`：生物实体基类，持有 AttributeMap 管理属性
- `entity/effect/EffectAttributeModifiers.hpp`：药水效果属性修改器
- `entity/entities/`：各类实体实现（ZombieEntity、WolfEntity、EnderDragonEntity 等 30+ 文件）
- `entity/ai/controller/VexMovementController.cpp`：Vex 移动控制器
- `entity/ai/goal/goals/attack/RangedAttackGoals.cpp`：远程攻击目标
- `entity/serialization/NbtHelper.cpp`：NBT 序列化/反序列化
- `server/command/commands/AttributeCommand.cpp`：属性命令（通过 AttributeRegistry 查询属性信息）

## 容易踩的坑

### 1. 修改器操作顺序

修改器按操作类型分阶段应用，而非按添加顺序：阶段1是所有 Addition、阶段2是所有 MultiplyBase、阶段3是所有 MultiplyTotal。例如：基础值=10，Addition+5→15，MultiplyBase 0.5→15+(10*0.5)=20，MultiplyTotal 0.1→20*1.1=22。

### 2. 值范围限制

设置基础值或计算结果会被自动 clamp 到属性范围。**重要**：MC 1.16.5 中 `MAX_HEALTH` 的最小值为 `1.0`（生命值不能为 0），而非 `0.0`。AttributeRegistry 从工厂函数获取范围数据，确保与 Attributes.hpp 定义一致。

### 3. 修改器 ID 唯一性

修改器使用 ID 进行比较和查找，相同 ID 的修改器被视为相同。移除时会移除第一个匹配的。

### 4. 线程安全

AttributeMap 和 AttributeInstance 的单次操作是原子的，但多次操作需要外部同步。例如 `getInstance()` 后连续调用 `setBaseValue()` 和 `addModifier()` 这两步之间可能被其他线程打断。

### 5. 属性注册

必须先注册属性才能使用。未注册时调用 `setBaseValue()` 会返回 false 且无效果。AttributeRegistry 在首次访问时自动注册所有内置属性。

### 6. copyFrom() 的行为

`copyFrom()` 只复制已注册的属性值和修改器，不会注册新属性。目标 AttributeMap 必须先注册对应属性。

### 7. 缓存机制

AttributeInstance 使用脏标记缓存计算结果。调用 `getValue()` 后会缓存，后续修改器变化会设置 dirty，下次 getValue() 才重新计算。需要注意：如果直接修改 AttributeModifier 对象的 amount（通过 setAmount），不会触发 dirty，需要手动调用 addModifier 或确保重新计算。

### 8. 属性数据单一来源

新增属性时，只需在 `Attributes.hpp` 中添加工厂函数和名称常量，然后在 `AttributeRegistry::_registerBuiltinAttributes()` 中注册即可。**不要**在命令或其他地方硬编码属性范围数据，一律通过 `AttributeRegistry::instance()` 查询。

### 9. MOVEMENT_EFFICIENCY 属性

`generic.movement_efficiency` 是移动效率属性，决定实体在减速方块（灵魂沙、蜂蜜块等）上的移动效率。
- 默认值: 0.0（不抵消减速效果）
- 范围: 0.0 ~ 1.0
- 在 `LivingEntity::getBlockSpeedFactor()` 中的使用方式：`finalSpeedFactor = lerp(movementEfficiency, blockSpeedFactor, 1.0)`
- 当 movementEfficiency=0.0 时，使用方块原始 speedFactor（灵魂沙=0.4，正常=1.0）
- 当 movementEfficiency=1.0 时，完全忽略方块减速效果（speedFactor 插值到 1.0）
- 灵魂疾行附魔通过 Addition 操作为此属性添加 +1.0 修饰符，从而完全抵消灵魂沙/灵魂土的减速效果
