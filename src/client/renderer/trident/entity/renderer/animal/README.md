# 动物渲染器

本目录包含动物实体的渲染器实现。

## 目录结构

```
animal/
├── AnimalRenderers.hpp/cpp      # 猪、牛、羊、哞菇、鸡、兔子、蝙蝠、鱿鱼、发光鱿鱼渲染器
├── CatRenderer.hpp/cpp          # 猫渲染器（11种皮肤）
├── HorseRenderer.hpp/cpp        # 马渲染器（含驴、骡、骷髅马、僵尸马）
├── LlamaRenderer.hpp/cpp        # 羊驼渲染器（4种颜色变体）
├── OcelotRenderer.hpp/cpp       # 豹猫渲染器
├── VillagerRenderer.hpp/cpp     # 村民渲染器（多层纹理：类型+职业+等级）
├── WolfRenderer.hpp/cpp         # 狼渲染器
└── README.md
```

## 内部模块关系

所有渲染器继承自 `core/` 下的基类：

- `LivingRenderer<EntityT, ModelT>` — 生物渲染器基类，提供动画计算、层渲染器管理
- `EntityRenderer` — 实体渲染器基类，提供渲染接口

继承关系：
- PigRenderer、CowRenderer、SheepRenderer、MooshroomRenderer、ChickenRenderer、RabbitRenderer、BatRenderer、SquidRenderer、GlowSquidRenderer → `LivingRenderer<LivingEntity, ModelT>`（定义在 AnimalRenderers.hpp）
- VillagerRenderer → `LivingRenderer<VillagerEntity, VillagerModel>`
- CatRenderer、HorseRenderer、LlamaRenderer、OcelotRenderer、WolfRenderer → `EntityRenderer`（自行管理模型和渲染）

## WolfRenderer/CatRenderer/OcelotRenderer/HorseRenderer/LlamaRenderer 的 GPU 管线集成

`WolfRenderer`、`CatRenderer`、`OcelotRenderer`、`HorseRenderer`、`LlamaRenderer` 都直接继承 `EntityRenderer`（而非 `LivingRenderer`），通过重写 `supportsAnimation()` 返回 `true` 进入 GPU 管线：

- `supportsAnimation()` → `true`：使 `EntityRendererManager::renderWithPipeline` 进入 Path B（ModelFactory + AnimatedMeshCache），通过 `ModelRegistration` 已注册的模型工厂生成主模型网格，消除“No mesh path”告警。模型状态设置（`setLivingAnimations`/`setAnimState`/`setCatAnimState` 等）由 `EntityRendererManager::_createModelForEntity` 的对应分支（wolf/cat/ocelot/horse/llama）统一处理，渲染器自身无需实现 `computeAnimationContext()`。

`WolfRenderer` 额外重写以支持层渲染：
- `supportsLayers()` → `true`：使主模型网格绘制后调用 `renderLayersPipelineClient`。
- `renderLayersPipelineClient(ClientEntity&, ...)`：分发到已注册的层（`WolfCollarLayer`），层通过 `ClientEntity` 的元数据镜像字段（`wolfTamed`/`wolfCollarColor`）读取驯服状态和颈圈颜色。

> 注：Cat/Ocelot/Horse/Llama 的层渲染（猫颈圈、马铠、羊驼地毯等）尚未实现，`supportsLayers()` 保持基类默认 `false`。各实体的专用状态（马的鞍/骑乘/吃草/前蹄抬起、羊驼的 hasChest、豹猫的蹲伏/逃跑）尚未同步到 `ClientEntity`，对应分支暂仅推进通用步态动画，待元数据镜像补齐后再补 `setSaddled`/`setHasChest`/`setCrouching` 等专用姿态。

## 上下游外部依赖关系

**上游依赖：**
- `core/EntityRenderer.hpp` — 实体渲染器基类
- `core/LivingRenderer.hpp` — 生物渲染器基类
- `core/EntityRendererManager.hpp` — 渲染器注册管理
- `model/animal/*` — 对应的动物模型
- `layer/entity/VillagerLayer.hpp` — 村民层渲染器

**下游使用：**
- `core/EntityRendererManager.cpp` — 渲染器注册
- `renderer/RendererRegistration.cpp` — 统一注册入口

## 容易踩的坑

1. **AnimalRenderers.hpp 中的渲染器是内联实现**：这些渲染器的 `getEntityTexture()` 直接在类定义中实现，不需要额外的 cpp 文件。注册函数已移至 `RendererRegistration.cpp` 统一管理。

2. **VillagerRenderer 多层纹理**：村民外观由 4 层纹理叠加实现（基础层、类型层、职业层、等级徽章层），需要调用 `setTextureAtlas()` 设置纹理图集才能正确渲染层。

3. **CatRenderer/LlamaRenderer/HorseRenderer 需要幼体模型**：这些渲染器有 `m_model` 和 `m_modelBaby` 两个模型实例，渲染时需要根据实体年龄选择。

4. **HorseModel 变体**：同一模型支持马、驴、骡、骷髅马、僵尸马，通过纹理路径区分，需要在 `getEntityTexture()` 中根据实体类型返回正确的纹理。

5. **阴影大小不同**：不同动物的阴影大小不同，需要在构造函数中设置 `m_shadowSize`。例如：鸡/兔子/蝙蝠为 0.3，猪/牛/羊/鱿鱼/马为 0.7。

6. **自行管理渲染的渲染器必须调用 `setLivingAnimations()`**：CatRenderer、OcelotRenderer、WolfRenderer 等直接继承 `EntityRenderer` 的渲染器，需要自行管理模型动画。调用顺序必须是：(1) 设置模型状态（如 `setCrouching`、`setSprinting`、`setAnimState`、`setCatAnimState`）→ (2) 调用 `model.setLivingAnimations()` 根据状态调整模型部件位置 → (3) 调用 `model.setAngles()` 设置具体角度 → (4) 调用 `model.render()` 渲染。如果跳过步骤(2)，模型的蹲伏/坐下/奔跑等姿态调整不会生效。使用 `LivingRenderer` 模板的渲染器（如 PigRenderer）不需要手动调用，模板方法会自动处理。
