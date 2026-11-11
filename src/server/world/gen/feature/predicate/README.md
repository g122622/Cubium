# 方块谓词

数据包方块谓词（`BlockPredicate`）的判定实现，供特征放置与结构生成判断"某坐标能否放置"。

## 目录结构树

```text
predicate/
├── BlockPredicate.hpp/cpp              # 抽象基类，定义 test(WorldGenRegion&, BlockPos) 接口
├── Predicates.hpp                      # 聚合头文件（供外部一次性引入全部谓词）
├── TrueBlockPredicate.hpp/cpp          # 恒真，对应数据包 {"type": "true"}
├── AllOfPredicate.hpp/cpp              # 全部子谓词通过；同文件还有 AnyOfPredicate / NotPredicate
├── HasSturdyFacePredicate.hpp/cpp      # 指定相邻面为坚固面
├── InsideWorldBoundsPredicate.hpp/cpp  # 坐标在世界构建范围内
├── MatchingBlockPredicate.hpp/cpp      # 方块状态匹配一组给定状态
├── MatchingFluidsPredicate.hpp/cpp     # 流体匹配
├── OnlyInAirPredicate.hpp/cpp          # 仅空气
├── ReplaceablePredicate.hpp/cpp        # 方块可被替换（走 BlockState::canBeReplaced）
├── SolidBlockPredicate.hpp/cpp         # 固体方块
├── TagMatchPredicate.hpp/cpp           # 方块标签匹配
└── WouldSurvivePredicate.hpp/cpp       # 该方块放在此处能否存活
```

## 内部模块关系

所有具体谓词都继承 `BlockPredicate`，只实现 `test`。组合类（`AllOfPredicate` /
`AnyOfPredicate` / `NotPredicate`）持有子谓词集合并逐个转发。

## 上下游外部依赖关系

上游依赖 `WorldGenRegion`（读方块状态与标签）、`BlockState`、方块标签系统。
下游由数据包特征加载器（`parser/`）解析 JSON 后构造，交给 `TreeFeature`、
植被特征与结构生成在落位前调用。

## 容易踩的坑

- **除 `BlockPredicate.cpp` 外，各谓词的 `.cpp` 都不在 `CMakeLists.txt` 里**：
  `BlockPredicate.cpp` 用 `#include "AllOfPredicate.cpp"` 这种形式把它们并入同一个
  翻译单元。两个后果：① 在 CMake 里搜不到某个谓词的 `.cpp` 属正常，不要因此判定
  它是死文件；② **绝对不要**再把被包含的 `.cpp` 补进 CMakeLists，否则同一批符号
  会被编译两次，触发重复定义。同一模式也见于 `tree/foliage/FoliagePlacers.cpp`
  与 `tree/trunk/TrunkPlacers.cpp`。
- `test` 的签名是 `test(const IWorld& world, const BlockPos& pos)`——**接收的是
  `IWorld`，不是 `WorldGenRegion`**。世界生成阶段调用方传进来的通常就是生成区域，
  但谓词本身不该依赖生成期专有的能力，否则无法在非生成场景（如结构放置校验）复用。
- `WouldSurvivePredicate` 走的是 `Block::isValidPosition`（方块自身的存活判定），
  与放置特征时的碰撞/支撑检查是两套逻辑，不能用它替代后者。