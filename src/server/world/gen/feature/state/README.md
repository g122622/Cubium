# state 模块（方块状态提供者）

数据驱动方块状态提供者体系。统一多态基类 `BlockStateProvider` + 8 种子类，由
`parser::BlockStateProviderParser` 按 JSON 的 `type` 字段构造对应子类实例，供各类
feature config 持有并在运行期采样方块状态。

## 目录结构

```
state/
├── BlockStateProvider.hpp/cpp                 # 多态基类 + SimpleBlockStateProvider
├── WeightedBlockStateProvider.hpp             # 加权随机（继承基类，保留 entries() 供 createFlower 平铺）
├── RuleBasedBlockStateProvider.hpp/cpp        # 规则匹配（fallback + rules，递归子提供者）
├── RotatedBlockStateProvider.hpp/cpp          # 随机朝向（defaultState + 随机 AXIS）
├── NoiseThresholdBlockStateProvider.hpp/cpp   # 噪声阈值二分
├── NoiseBlockStateProvider.hpp/cpp            # 噪声采样
├── DualNoiseBlockStateProvider.hpp/cpp        # 双噪声（快噪声 + 慢噪声）
├── RandomizedIntBlockStateProvider.hpp/cpp    # 随机化整型属性（source 子提供者 + IntProvider）
└── NoiseStateUtils.hpp                        # 噪声类子类共享匿名辅助（getNoiseValue/getRandomState*/findIntegerProperty/InclusiveRange）
```

## 核心接口

`BlockStateProvider::getState(const IWorld&, math::IRandom&, i32 x, i32 y, i32 z)` 是统一
采样入口。`IWorld` 参数：RuleBased 谓词测试与 RandomizedInt 属性查找需要世界上下文，其余
子类忽略。返回 `const BlockState*`，提供者无可用状态时返回 `nullptr`（指针生命周期由
`BlockRegistry` 管理，程序级有效）。

两个辅助虚方法：
- `asSingleState()`：解析期降级用。仅 Simple 返回其固定状态；其余返回 `nullptr`。
  某些 feature config 字段是裸 `const BlockState*`（如 trunk/ground_state），解析期用
  `asSingleState()` 取单一状态，非 simple provider 返回 nullptr 时严格报错。
- `clone()`：多态深拷贝。配置结构的拷贝构造/赋值通过 `clone()` 实现深拷贝，避免浅拷贝
  double-free。

## 8 种子类

| 子类 | JSON type | 说明 |
|---|---|---|
| SimpleBlockStateProvider | `simple_state_provider` | 始终返回同一状态 |
| WeightedBlockStateProvider | `weighted_state_provider` | 加权随机，线性减权法 |
| RuleBasedBlockStateProvider | `rule_based_state_provider` | 遍历 rules 命中谓词则取 then，否则 fallback；fallback/then 为递归子提供者 |
| RotatedBlockStateProvider | `rotated_block_provider` | defaultState + `Axes::all()[nextInt(3)]`，有 AXIS 属性则 with |
| NoiseThresholdBlockStateProvider | `noise_threshold_provider` | 噪声值与阈值比较二分高低状态集 |
| NoiseBlockStateProvider | `noise_provider` | 噪声采样映射状态集 |
| DualNoiseBlockStateProvider | `dual_noise_provider` | 快噪声选状态集 + 慢噪声加扰动 |
| RandomizedIntBlockStateProvider | `randomized_int_state_provider` | 在 source 子提供者基础上用 IntProvider 随机化某个整型属性 |

## 解析与消费路径

解析：`BlockStateProviderParser::parse` 返回 `Result<unique_ptr<BlockStateProvider>>`，
按 `type` 分流构造子类。`parseRuleBased` 处理规则子结构（ifTrue 谓词 + then 子提供者）。

消费：feature config 统一持有 `unique_ptr<BlockStateProvider>`，运行期
`provider->getState(world, rng, x, y, z)`。无 kind 分流，无 tagged union。

## 容易踩的坑

### 1. clone() 必须递归

RuleBased 的 `fallback`/`rules[].then`、RandomizedInt 的 `source` 是 `unique_ptr<基类>`
递归结构。各子类 `clone()` 必须递归 clone 子提供者，否则 config 拷贝浅拷贝 double-free。

### 2. asSingleState() 解析期降级

`trunk`、`big_mushroom` 的 cap/stem、`vegetation_patch` 的 ground_state、`root_system`
的 required_vertical_space_for_tree 等字段是裸 `const BlockState*`，解析期调用
`asSingleState()` 取单一状态。非 simple provider 返回 `nullptr` 时严格报错中断，不接受
weighted/noise 等需运行期采样的 provider 降级。

### 3. createFlower 平铺用 dynamic_cast

`createFlower` 需把 weighted provider 的多条目平铺为等概率花卉列表，通过
`dynamic_cast<WeightedBlockStateProvider*>` 取 `entries()`；非 weighted 则用 `asSingleState()`。

### 4. IWorld 贯穿所有采样点

迁移后所有 `getState` 调用点都补传了 `world` 与坐标。feature place 上下文需确保已有
world/pos 可用（多数 feature place 已持有 `IWorld&`/`WorldGenRegion&` 与原点坐标）。

### 5. GeodeBlockSettings 仅移动

`GeodeFeature` 的 5 个 provider 成员是 `unique_ptr<BlockStateProvider>`，`GeodeBlockSettings`
与 `GeodeConfig` 因此变为仅移动类型（拷贝已删除）。涉及 Geode 配置传递处需用移动语义。

### 6. TreeFeatureConfig 双轨设计

`TreeFeatureConfig` 同时保留 `foliageBlock`（裸指针，FoliagePlacer 第一遍急切放置用）与
`foliageProvider`（多态指针，第二遍逐叶重采样用），以保留 FoliagePlacer 的双遍放置架构。
`hasFoliageProvider()` 判定 `foliageProvider != nullptr`。
