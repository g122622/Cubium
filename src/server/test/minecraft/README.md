# minecraft/ — GameTest 的 ServerWorld 具体绑定层

把 `framework/` 的引擎无关抽象绑定到项目的 `ServerWorld`/`Template`/`WeatherManager` 等具体类型。本目录所有类**不对外**——外部经 `facade/` 门面间接使用。仅依赖 `framework/`+`base/`+`mc` server 类型，被 `facade/`+`runner/` 持有。

## 目录结构

```
minecraft/
├── structure/                            # 结构放置与包围盒
│   ├── StructureBounds.hpp               # 旋转后包围盒计算（rotatedSize/bounds/paddingBounds）
│   ├── StructureBounds.cpp
│   ├── MinecraftStructurePlacer.hpp      # 经 JigsawAssembler::getTemplateManager 取 Template + placeInWorld + 屏障
│   └── MinecraftStructurePlacer.cpp
├── instance/                             # 实例状态机具体实现
│   ├── MinecraftGameTestInstance.hpp     # BaseGameTestInstance 的 ServerWorld 实现（spawn/clear/tick）
│   └── MinecraftGameTestInstance.cpp
├── batch/                                # 批次 runner 具体实现
│   ├── MinecraftGameTestBatchRunner.hpp  # BaseGameTestBatchRunner 的 ServerWorld 实现（_createInstance/_runTest）
│   └── MinecraftGameTestBatchRunner.cpp
├── helper/                               # helper provider 具体实现
│   ├── MinecraftGameTestHelperProvider.hpp  # 创建绑 ServerWorld 的 GameTestHelper（facade）
│   └── MinecraftGameTestHelperProvider.cpp
├── listener/                             # 游戏内可视化监听器
│   ├── WorldVisualizationListener.hpp    # IGameTestListener 实现（spdlog 输出 + TODO 信标光束）
│   ├── WorldVisualizationListener.cpp
│   └── TestInstanceBlockEntity.hpp       # TODO: 方块实体可视化（StructureBlockEntity 未实现）
└── environment/                          # 环境应用器
    ├── MinecraftEnvironmentApplier.hpp   # 把 framework 环境意图应用到 ServerWorld（绕过 stub）
    └── MinecraftEnvironmentApplier.cpp
```

## 内部模块关系

- `structure/`：`StructureBounds`（纯几何，依赖 `base/coords/TestTransform` 旋转公式一致）← `MinecraftStructurePlacer`（依赖 `Template`/`TemplateManager`/`JigsawAssembler`/`BlockRegistry`）。
- `instance/`：`MinecraftGameTestInstance` 依赖 `structure/`（placer+bounds）+ `framework/instance/BaseGameTestInstance`。
- `helper/`：`MinecraftGameTestHelperProvider` 依赖 `facade/GameTestHelper`（1F）+ `instance/`（向下转型取 origin/bounds）。
- `batch/`：`MinecraftGameTestBatchRunner` 依赖 `framework/batch/BaseGameTestBatchRunner` + `instance/` + `helper/`（创建 provider）。
- `listener/`：`WorldVisualizationListener` 依赖 `framework/listener/IGameTestListener` + `framework/instance/`。`TestInstanceBlockEntity` 为 TODO 占位。
- `environment/`：`MinecraftEnvironmentApplier` 依赖 `framework/environment/` 全部具体类 + `ServerWorld`/`TimeManager`/`WeatherManager`。

## 上下游外部依赖关系

**上游（本目录依赖）**：`base/`+`framework/`（全部抽象）；`mc::server::ServerWorld`（`src/server/world/`）、`mc::server::core::TimeManager`、`mc::server::WeatherManager`、`mc::world::gen::feature::template_::Template`/`TemplateManager`/`PlacementSettings`、`mc::world::gen::jigsaw::JigsawAssembler`、`mc::BlockRegistry`、`mc::resource::ResourceLocation`、spdlog。

**下游（依赖本目录）**：
- `facade/GameTestHelper`（1F）构造收 `MinecraftGameTestInstance` 的 origin/bounds。
- `runner/GameTestRunner`（1D）创建 `MinecraftGameTestBatchRunner` 编排。
- `facade/GameTestServer`/`GameTestCommand`（1F）经 runner 间接使用。

## 容易踩的坑

1. **`TemplateManager` 是全局单例**，经 `JigsawAssembler::getTemplateManager()` 取，**不是** `MinecraftServer`/`ServerWorld` 的成员。资源包/数据包由 `RegistryBootstrap` 在服务端启动期注入；GameTestServer/IntegratedServer 启动后 `getTemplate` 才有效。结构名走 `ResourceLocation`（如 `"gametest:empty_3x3"`）。
2. **`PlacementSettings` 须 include `Template.hpp`**（非 `PlacementSettings.hpp`——后者注释明确说不参与编译，仅供参考）。`setRotation`/`setIgnoreEntities`/`setMirror` 链式返回 `*this`。
3. **命名空间遮蔽坑**（见 BossBarState 内存）：`mc::test` 内非限定名两段查找不回退 `mc::world`，故引用 `StructureBoundingBox`/`Template`/`TemplateManager`/`JigsawAssembler` 等须**全限定** `mc::world::gen::...` 或在文件顶部 `using` 别名。`StructureBounds.hpp`/`MinecraftStructurePlacer.cpp` 已用别名规避。
4. **`ServerWorld::setBlockState(x,y,z,state)` 返回 bool（不返回旧状态）**；`getBlockState` 返回 `const BlockState*`（区块未加载可能 nullptr）。Air 状态取 `BlockRegistry::instance().airState()`（非 `Blocks::AIR()`，项目无此 API）。
5. **时间/天气不在 `ServerWorld` 直接方法**：时间走 `ServerWorld::timeManager()->setDayTime(i64)`，天气走 `ServerWorld::weatherManager()->setClear/setRain/setThunder/resetWeather`。`MinecraftEnvironmentApplier` 据此应用 `TimeOfDayEnvironment`/`WeatherEnvironment`。
6. **`SetGameRulesEnvironment` 应用为 TODO**：项目 `GameRules` 用类型化键（`BooleanGameRuleKey`/`IntegerGameRuleKey`）+ `setBoolean`/`setInt`，非字符串名；需建规则名→键映射表后才能应用，当前 applier 仅记 warn 跳过。默认 `"default"` 环境是空 `AllOf`，不触此分支。
7. **`MinecraftGameTestInstance::spawnStructure` 失败即 fail**（结构找不到/放置失败），错误码用 `MethodNotImplemented` 占位（TODO 细化）。`_isTestReady` 判 `m_structurePlaced`，未放置则 `tick()` 等待。
8. **`MinecraftGameTestBatchRunner` 原点布局为 `StructureGridSpawner` 网格**：批次内每个测试由 `StructureGridSpawner` 按 `testsPerRow`（默认 8）换行网格排列，间距 `SPACE_BETWEEN_COLUMNS/ROWS=32`（覆盖实体 FOLLOW_RANGE，避免相邻结构跨测试目标搜索污染）。两步协议：`peekOrigin()` 取本测试原点（不推进），放结构后 `advance(sizeX, sizeZ, padding)` 用真实旋转尺寸推进游标。游标跨 batch 累积，整个运行连续网格编号。
9. **`MinecraftGameTestHelperProvider` 向下转型**：`createGameTestHelper` 内 `static_cast<MinecraftGameTestInstance&>` 取 origin/bounds，故 provider 仅适用于 `MinecraftGameTestInstance`（不适用于 headless 单测的 `NullGameTestHelper`）。
10. **`TestInstanceBlockEntity` 整类 TODO**：项目 `StructureBlockEntity` 类未实现（结构方块仅有 Block 子类，无 BlockEntity 子类），游戏内信标光束可视化待方块实体体系就绪后接入。第一阶段可视化由 `WorldVisualizationListener` 的 spdlog 输出临时承载。
