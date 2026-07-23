# Client Application 模块

客户端应用模块是 Cubium 客户端的核心入口点，负责整合所有客户端子系统并协调游戏主循环。

## 目录结构

```text
src/client/application/
├── ClientApplication.hpp          # 客户端应用主接口
├── ClientApplication.cpp          # 生命周期编排、主循环和跨功能共享逻辑
├── ClientAppStateMachine.hpp      # 客户端状态机（生命周期状态和转换）
├── features/                      # 功能实现拆分目录
│   ├── ClientApplicationBootstrap.cpp   # 初始化骨架
│   ├── ClientApplicationHelpers.*pp     # 通用辅助函数
│   ├── ClientApplicationAudio.cpp       # 音频初始化、音效、听者同步
│   ├── ClientApplicationInput.cpp       # 输入绑定、相机、鼠标捕获、挖掘状态机
│   ├── ClientApplicationNetwork.cpp     # 网络回调、补全候选、聊天命令
│   ├── ClientApplicationResource.cpp    # 资源初始化、重载、资源包变更回调
│   ├── ClientApplicationUi.cpp          # 背包/创造屏、屏幕切换、事件分发
│   ├── ClientApplicationUiFrame.cpp     # 每帧 UI 状态更新
│   ├── ClientApplicationTargetInfo.cpp  # 射线检测结果更新
│   ├── ClientApplicationTargetInfoUi.cpp # 准星目标信息与调试屏幕更新
│   ├── ClientApplicationSettings.cpp    # 设置读取、应用、回调绑定
│   ├── ClientApplicationSession.cpp     # 游戏会话管理
│   ├── MemoryTraceThread.*pp            # 内存追踪线程
│   └── README.md                        # 功能拆分说明
└── README.md
```

## 内部模块关系

ClientApplication 作为协调中心，整合以下子系统：

```
ClientApplication.cpp
    ├── ClientApplicationBootstrap     → 初始化调度
    ├── ClientApplicationSession       → 会话生命周期
    ├── ClientApplicationAudio         → 音频子系统
    ├── ClientApplicationInput         → 输入子系统
    ├── ClientApplicationNetwork       → 网络子系统
    ├── ClientApplicationResource      → 资源子系统
    ├── ClientApplicationUi            → UI 子系统
    ├── ClientApplicationUiFrame       → UI 每帧更新
    ├── ClientApplicationTargetInfo    → 射线检测
    ├── ClientApplicationTargetInfoUi  → 目标信息显示
    ├── ClientApplicationSettings      → 设置管理
    └── ClientApplicationHelpers       → 通用辅助函数
```

状态机转换流程：
```
Initializing -> MainMenu -> LoadingWorld -> InGame <-> Paused
                  ^                              |
                  +-------- LeavingWorld <-------+
                               |
                               v
                          ShuttingDown
```

## 上下游外部依赖关系

**上游依赖（使用该目录的模块）：**
- `src/client/main.cpp` - 客户端入口点

**下游依赖（该目录依赖的模块）：**
- `common/entity`、`common/item`、`common/world`、`common/screen`
- `common/util/thread/UniversalWorkerPool` - ClientCompute 客户端统一计算池（见下）
- `client/input/InputManager`
- `client/network/NetworkClient`
- `client/resource/ResourceManager`
- `client/sound/AudioService`
- `client/world/ClientWorld`
- `client/renderer/trident/core/TridentEngine`
- `client/ui/*`（ScreenManager、ChatWidget、InventoryScreen、CreativeScreen、CraftingScreen、DebugScreenWidget）
- `server/application/IntegratedServer` - 内置服务端

## 客户端统一计算池 ClientCompute

`ClientApplication` 以值成员持有 `util::UniversalWorkerPool m_clientComputeWorkerPool{-1, "ClientCompute", 300}`，
作为客户端进程级统一计算池，承接 chunkmesh 构建、皮肤异步加载等客户端计算任务。

生命周期与关停顺序：
- **start**：`initializeShell` 阶段（早于 mesh 系统/皮肤管理器等消费者初始化）`pool.start()`。
- **shutdown**：`ClientApplication::shutdown()` 中，`m_world.destroy()`（已关停 mesh scheduler 并等在途归零）
  之后、`m_window.destroy()` 之前 `pool.shutdown()`。此处保证 mesh scheduler 与 skin manager 均已销毁，
  晚到的 mesh 回调走 `weak_ptr<MeshResultQueue>` 失败路径，安全。

注入：mesh 系统经 `ClientWorld::initializeMeshSystem(pool, dataPool, resultQueue, config)` 注入池引用；
皮肤管理器经 `ClientSkinManager::setWorkerPool(&pool)` 注入裸指针（填上既存缺口，皮肤加载异步化）。

## 容易踩的坑

### 1. 初始化顺序依赖

ClientApplication 的初始化有严格的顺序依赖：

```
资源系统 → 渲染器 → GUI 图集 → UI 引擎
```

错误的顺序会导致纹理加载失败或 UI 无法渲染。严格遵循 `initialize()` 中的初始化顺序，不要随意调整。

### 2. GUI 精灵图集加载顺序

GUI 精灵图集需要按正确顺序初始化：
1. 初始化图集对象
2. 加载纹理（设置正确的图集尺寸）
3. 注册精灵（使用正确的图集尺寸计算 UV）
4. 注册到 GuiRenderer

在加载纹理前注册精灵会导致 UV 坐标计算错误。

### 3. 鼠标捕获状态管理

鼠标捕获状态需要与 UI 状态正确同步。屏幕关闭后鼠标没有被重新捕获，会导致游戏视角控制失效。

### 4. 方块破坏进度管理

方块破坏需要同时更新本地 BreakProgressManager 状态和发送网络包。忘记更新 BreakProgressManager 会导致破坏动画不显示。

### 5. 网络回调生命周期

网络回调中访问成员变量时需要检查指针有效性。在关闭过程中，m_player 可能在回调执行前已被重置。

### 6. 时间同步精度

客户端渲染时间需要平滑过渡到服务端时间：
- 纠正因子基于 deltaTime 计算，与帧率无关
- 每秒纠正约 50% 的差值，在平滑性和响应性之间取得平衡
- 避免直接使用服务端时间导致的天空/太阳跳变

### 7. 资源重载时机

资源重载后需要标记所有区块为脏，否则已加载区块会使用旧纹理/模型。

### 8. 关闭顺序

关闭时需要按依赖关系的逆序释放资源：
1. 外部渲染依赖对象（皮肤管理器、UI引擎、图集等）
2. 渲染器
3. 玩家和物理引擎
4. 世界
5. 窗口

在渲染器销毁后再析构皮肤图集或 GUI 图集，会触发 Vulkan 资源销毁崩溃。

### 9. 功能拆分结构

`ClientApplication` 已按功能域拆到 `src/client/application/features/`，主文件只保留编排与生命周期：
- `setupInputBindings()` / `setupCamera()` 等逻辑已迁移，不要再往主文件补回重复实现
- `ClientApplicationBootstrap.cpp` 负责客户端初始化骨架，`initialize()` 只做调度

### 10. 命名空间使用

`ClientApplicationHelpers` 的公共辅助函数位于 `mc::client::application::features` 命名空间。调用处必须显式限定或导入作用域。

### 11. UI 组件命名空间

`TargetInfoWidget` 位于 `mc::client::ui::minecraft::targetinfo`，`DebugScreenWidget` 位于 `mc::client::ui::minecraft`。不要把这两个命名空间混用。

### 12. handleEvents 职责

`ClientApplication::handleEvents()` 现在只做输入轮询和分流：
- 覆盖层输入放在 `handleUiOverlayInput()`
- 游戏快捷键放在 `handleGameplayShortcutInput()`
- 玩家视角/移动放在 `handleMouseAndMovementInput()`

不要把新逻辑塞回 `handleEvents()`。

### 13. 挖掘和放置输入分离

`ClientApplication::handleBlockMiningInput()` 和 `handleBlockPlacementInput()` 已分开。挖掘的取消、开始、完成逻辑继续留在独立 helper 里，不要重新合并成一个大输入状态机。

### 14. 网络回调状态维护

`ClientApplicationNetwork.cpp` 里的网络回调必须同时维护世界、实体、容器和经验状态。本地玩家、远程玩家、普通实体、经验球和当前打开的容器屏幕都要分别同步，不能把回调留成只接收不落地的空壳。铁傀儡的攻击/持花状态通过 `onEntityStatus` 回调处理（`IronGolemAttack`、`IronGolemHoldRose`、`IronGolemStopRose`），必须同时更新 `ClientEntity` 的对应字段和播放音效。TNT矿车的引信状态也通过 `onEntityStatus` 回调处理（`EatBlock` status 10），客户端根据 `entityType() == VanillaEntityTypeKeys::TNT_MINECART` 区分，调用 `setFuseTimer(80)`，音效由服务端 `_ignite()` 中 `playSound()` 播放。世界事件（`onWorldEvent` 回调）由 `_handleWorldEvent()` 处理，根据事件ID播放音效和生成粒子效果，事件常量定义在 `common/world/WorldEvents.hpp`。

### 15. NaturalSpawner 密度管理器

`NaturalSpawner::createDensityManager()` 如果保存 `EntityManager::countEntitiesByClassification()` 结果的引用，会导致悬垂引用。必须按值持有。`MobDensityTracker` 的密度采用逆衰减公式 `sum(charge / sqrt(distSq)) * multiplier`，与查询位置重合的点电荷贡献无穷大（阻止同位置堆叠生成）。

### 16. 玩家物理与渲染插值

玩家物理如果随渲染帧率运行，行走速度会随 FPS 变化；如果只在 20TPS 更新位置，镜头又会出现 tick 级跳变。

解决方案：`ClientApplication` 只在固定 20TPS 中调用 `Player::updatePhysics()`，输入由 `Player::handleMovementInput()` 缓存；每次物理 tick 开始由 `Player::updatePhysics()` 冻结 `prevPosition()`，每帧相机用 partial tick 在 `prevPosition()` 与 `position()` 之间插值到玩家眼睛位置。传送或纠错应通过 `Player::setPosition()` 同步重置采样。

### 17. 视野晃动与真实相机位置

把视野晃动写进 `Camera::position()` 会污染视锥剔除、区块调度、云/天气和破坏覆盖层使用的真实相机位置。

解决方案：客户端每帧相机只同步到玩家眼睛的真实世界位置；玩家移动视野晃动通过 `Camera::setViewTransform()` 附加到 view matrix。

### 18. UI 层 Z-Order

UI 层按 Z 值分层，从低到高：准星(0) → HUD(10) → 目标信息(15) → 聊天框(20) → Screen栈(30) → 调试屏幕(100)。新增 UI 层时注意选择合适的 Z 值避免遮挡问题。
