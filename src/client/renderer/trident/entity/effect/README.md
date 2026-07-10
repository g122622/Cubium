# 实体特效系统

本目录包含实体渲染特效的实现。

## 目录结构

```
effect/
├── glow/                            # 发光效果
│   ├── GlowEffect.hpp               # 发光效果管理器（静态工具类）
│   ├── GlowEffect.cpp               # 发光效果实现
│   └── README.md                    # 发光效果文档
├── fire/                            # 着火效果
│   ├── FireEffect.hpp               # 着火效果渲染器（静态类）
│   ├── FireEffect.cpp               # 纹理加载、tick 推进、billboard 渲染实现
│   ├── FireAnimationState.hpp       # 火焰动画播放状态机（纯逻辑，不依赖 Vulkan）
│   ├── FireAnimationState.cpp       # 动画状态机实现（双计数器帧推进）
│   ├── FireTextureLoader.hpp        # 纹理解码接口（纯 CPU，可单元测试）
│   ├── FireTextureLoader.cpp        # PNG + mcmeta 解析、帧提取实现
│   └── README.md                    # 着火效果文档
├── hurt/                            # 受伤闪烁效果
│   ├── HurtFlashEffect.hpp          # 受伤闪烁效果头文件
│   ├── HurtFlashEffect.cpp          # 受伤闪烁效果实现
│   └── README.md                    # 受伤闪烁文档
└── README.md                        # 本文档
```

## 内部模块关系

三个特效模块相互独立，各自为静态工具类：

```
effect/
├── glow/GlowEffect         # 发光轮廓（依赖后处理管线，待完善）
├── fire/FireEffect         # 火焰 billboard 渲染（含 .mcmeta 动画）
└── hurt/HurtFlashEffect    # 受伤红色闪烁（着色器方案）
```

渲染调用顺序：实体主体渲染 → 受伤闪烁（覆盖层）→ 着火效果 → 发光效果（后处理）

火焰纹理动画通过 `FireEffect::tick()` 推进（由 `TridentEngine::tickTextureAnimations()` 调用），`FireAnimationState` 管理双计数器帧切换，支持 mcmeta 自定义帧序列和每帧独立时长。

## 上下游外部依赖关系

**本目录依赖的上游模块**：
- `client/renderer/trident/entity/model/core/ModelRenderer.hpp` - 模型顶点类型
- `client/renderer/trident/entity/pipeline/EntityPipeline.hpp` - 实体渲染管线
- `client/world/entity/ClientEntity.hpp` - 客户端实体
- `common/entity/core/Entity.hpp` - 实体基类
- `common/entity/core/LivingEntity.hpp` - 生物实体
- `common/resource/pack/IResourcePack.hpp` - 资源包接口

**依赖本目录的下游模块**：
- `client/renderer/trident/core/TridentEngine.cpp` - 初始化/清理调用
- `client/renderer/trident/entity/core/EntityRendererManager.cpp` - 渲染时调用

## 容易踩的坑

### 1. 发光鱿鱼不存在于 MC 1.16.5

发光鱿鱼（Glow Squid）是 MC 1.17+ 添加的实体，本项目目标版本为 MC 1.16.5，不要错误引用。

### 2. 发光效果后处理管线尚未完成

当前 `GlowEffect::renderGlow()` 和 `renderAllGlowing()` 仅有框架代码，等待渲染管线支持多渲染目标(MRT)和模糊着色器后才能完整实现。

### 3. 团队颜色获取链

`Entity::getTeam()` 在基类中默认返回 `nullptr`，只有 `ServerPlayer` 重写了该方法。客户端实体需要通过其他方式获取团队信息。

### 4. hurtTime 递减方向

hurtTime 从 10 递减到 0，受伤开始时 hurtTime=10，结束时 hurtTime=0。进度计算应为 `1.0 - (hurtTime / 10.0)`。

### 5. 着色器方案 vs 纹理方案

本项目受伤闪烁采用着色器内置计算，而非 MC 1.16.5 的 OverlayTexture 纹理采样方式。`getPackedOverlay()` 方法保留用于兼容性，但当前着色器不使用此值。

### 6. FireEffect 的 Vulkan 资源生命周期

`FireEffect` 持有 Vulkan 资源（Image、ImageView、Sampler、Memory），必须在 `cleanup()` 中正确销毁。初始化和清理必须配对调用，否则会泄漏 GPU 资源。

### 7. 火焰纹理后备机制与动画

火焰纹理从资源包加载 `textures/block/fire_0.png` 和 `fire_1.png`，如果不存在会自动生成程序化纹理作为后备。初始化不会失败，但纹理质量可能不符合预期。

火焰支持 `.mcmeta` 动画：`FireTextureLoader` 读取同名 `.png.mcmeta`，提取动画条带中的所有帧（而非仅首帧），`FireAnimationState` 按 frametime 和自定义帧序列驱动帧切换。所有帧预上传到单张 VkImage，动画推进仅改变 UV 偏移。interpolate 插值尚未实现（见 `FireAnimationState.hpp` 中的 TODO）。
