# Entity Interfaces

实体行为能力接口模块，定义实体可实现的各类行为能力抽象接口。

## 目录结构

```
interfaces/
├── IAngerable.hpp         # 愤怒接口，可记住攻击者并复仇
├── ICrossbowUser.hpp      # 弩使用者接口（继承IRangedAttackMob）
├── IEquipable.hpp         # 可装备接口，支持鞍、马铠等装备槽
├── IFlinging.hpp/.cpp     # 撞飞型近战生物接口
├── IFlyingAnimal.hpp      # 飞行动物接口
├── IJumpingMount.hpp      # 可跳跃骑乘接口
├── IMob.hpp               # 敌对生物标记接口（空接口）
├── IRangedAttackMob.hpp   # 远程攻击接口
├── IRideable.hpp/.cpp     # 可骑乘接口，支持鞍和加速
└── IShearable.hpp         # 可剪毛接口
```

## 内部模块关系

接口继承关系：
- `ICrossbowUser` 继承 `IRangedAttackMob`

## 上下游外部依赖关系

**上游依赖（本模块依赖）：**
- `common/core/Types.hpp` - 基础类型定义
- `common/util/math/Vector3.hpp` - 向量类型（IRideable）
- `LivingEntity` - 生物实体（前向声明）
- `Player` - 玩家实体（前向声明）
- `ItemStack` - 物品堆（前向声明）
- `MobEntity` - 生物实体（前向声明，IRideable）
- `BoostHelper` - 加速辅助类（前向声明，IRideable）

**下游依赖（依赖本模块）：**
- `entity/` 下的实体实现类（如 `SheepEntity`, `WolfEntity`, `PigEntity`, `AbstractHorseEntity` 等）
- `entity/core/BoostHelper.hpp` - 与 IRideable 配合使用

## 容易踩的坑

1. **ICrossbowUser 继承自 IRangedAttackMob**：实现 ICrossbowUser 时需要同时实现 IRangedAttackMob 的纯虚函数 `attackEntityWithRangedAttack()`。

2. **IRideable 需要与 BoostHelper 配合**：`ride()` 方法需要传入 `BoostHelper` 引用，需要在实体中正确初始化 BoostHelper（调用 `init()` 绑定 DataManager 参数）。

3. **前向声明类型不可直接使用完整类型**：接口中大量使用前向声明（`LivingEntity`, `Player`, `ItemStack`），调用时需确保完整类型定义可见。

4. **IAngerable 默认实现**：`updateAnger()` 提供了默认实现，子类需在 `tick()` 中显式调用。

5. **IRideable::ride() 有实现**：不是纯虚函数，提供了默认的骑乘移动逻辑，子类可选择性重写。
