# Client Application Features

客户端应用功能模块目录，承载从 `ClientApplication` 中逐步拆分出来的功能域实现。

## 目录结构

```text
src/client/application/features/
├── ClientApplicationBootstrap.cpp      # 客户端应用初始化骨架
├── ClientApplicationHelpers.hpp        # 通用辅助函数声明（容器同步、鼠标捕获、挖掘增量计算等）
├── ClientApplicationHelpers.cpp        # 非模板辅助函数实现
├── ClientApplicationAudio.cpp          # 音频初始化、音效、听者同步
├── ClientApplicationInput.cpp          # 输入绑定、相机初始化、鼠标捕获、挖掘状态机
├── ClientApplicationNetwork.cpp        # 网络回调、补全候选、聊天命令、维度切换、世界事件处理
├── ClientApplicationResource.cpp       # 资源初始化、重载、资源包变更回调
├── ClientApplicationUi.cpp             # 背包/创造屏、屏幕切换、事件分发
├── ClientApplicationUiFrame.cpp        # 每帧 UI 状态更新
├── ClientApplicationTargetInfo.cpp     # 射线检测结果更新
├── ClientApplicationTargetInfoUi.cpp   # 准星目标信息与调试屏幕更新
├── ClientApplicationSettings.cpp       # 设置读取、应用、回调绑定、GUI 缩放
├── ClientApplicationSession.cpp        # 游戏会话管理（状态机、世界创建、会话销毁）
├── MemoryTraceThread.hpp               # 内存追踪线程头文件
├── MemoryTraceThread.cpp               # 内存追踪线程实现（独立线程定期采样内存）
└── README.md
```

## 内部模块关系

该目录按功能域拆分，各模块相对独立：

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

## 上下游外部依赖关系

**上游依赖（使用该目录的模块）：**
- `src/client/application/ClientApplication.cpp`

**下游依赖（该目录依赖的模块）：**
- `common/entity`、`common/item`、`common/world`、`common/screen`
- `client/input/InputManager`
- `client/network/NetworkClient`
- `client/resource/ResourceManager`
- `client/sound/AudioService`
- `client/world/ClientWorld`
- `client/renderer/trident/core/TridentEngine`
- `client/ui/*`（ScreenManager、ChatWidget、InventoryCraftingScreen、CreativeScreen、DebugScreenWidget）

## 容易踩的坑

- 模板函数放在头文件中，非模板函数放在 `.cpp` 中
- 该目录只放与客户端应用直接相关的通用功能，不要把子系统实现继续堆回来
- 新模块加入后要同步更新 `src/client/CMakeLists.txt`
- 中文注释要保留得足够详细，重构时不要把原有逻辑说明删薄
- UI 事件分发文件要保留”为什么要这样分发”的注释
- `ClientApplicationHelpers` 中的辅助函数位于 `mc::client::application::features` 命名空间，调用处要显式限定或引入对应作用域
- `ClientApplication` 主文件已经不再保留 `setupInputBindings()` / `setupCamera()` 的实现，相关逻辑以 `features/` 内的同名成员函数为准
- 粒子网络回调（`onParticle` / `onBlockParticle` / `onItemParticle` / `onVibrationParticle` / `onTrailParticle` / `onEntityEffectParticle`）在 `ClientApplicationNetwork.cpp` 中注册。携带附加数据的粒子（方块/物品/EntityEffect/Vibration/Trail）通过对应的 `ParticleData` 子类走 `ParticleManager::addPendingParticle()` 数据管线创建粒子，而非直接调用 `ClientWorld::addXxxParticle()`。物品粒子（Item/ItemSlime/ItemCobweb/ItemSnowball）从 `ParticlePacket::decodeItemStack()` 解码 `ItemStack`，封装为 `ItemParticleData` 后按 count 重复投递
