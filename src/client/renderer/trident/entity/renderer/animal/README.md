# 动物渲染器

本目录包含动物实体的渲染器实现。

## 目录结构

```
animal/
├── AnimalRenderers.hpp/cpp      # 猪、牛、羊、哞菇、鸡、兔子、蝙蝠、鱿鱼渲染器
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
- PigRenderer、CowRenderer、SheepRenderer、MooshroomRenderer、ChickenRenderer、RabbitRenderer、BatRenderer、SquidRenderer → `LivingRenderer<LivingEntity, ModelT>`（定义在 AnimalRenderers.hpp）
- VillagerRenderer → `LivingRenderer<VillagerEntity, VillagerModel>`
- CatRenderer、HorseRenderer、LlamaRenderer、OcelotRenderer、WolfRenderer → `EntityRenderer`（自行管理模型和渲染）

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
