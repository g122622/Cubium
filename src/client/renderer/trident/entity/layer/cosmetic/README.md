# 外观层渲染器

本目录包含外观相关的层渲染器。

## 目录结构

```
cosmetic/
├── CapeLayer.hpp/cpp    # 斗篷层渲染器（动态摆动动画）
├── ElytraLayer.hpp/cpp  # 鞘翅层渲染器（滑翔展开动画）
└── README.md
```

## 内部模块关系

两个层渲染器相互独立，都继承自 `core::LayerRenderer` 基类：
- 使用 `pipeline::EntityPipeline` 进行 GPU 管线渲染
- 使用 `model::ModelVertex` 构建网格顶点
- 使用角度分桶缓存网格，避免每帧重建

## 上下游外部依赖关系

**上游依赖：**
- `core/LayerRenderer.hpp` - 层渲染器基类
- `pipeline/EntityPipeline.hpp` - 实体渲染管线
- `core/AnimationContext.hpp` - 动画上下文
- `model/core/ModelRenderer.hpp` - 模型渲染器
- `common/entity/entities/player/Player.hpp` - 玩家实体
- `common/entity/core/LivingEntity.hpp` - 生物实体基类
- `common/entity/entities/player/PlayerModelPart.hpp` - 玩家模型部件
- `common/item/Items.hpp` - 物品定义（Items::ELYTRA）

**下游依赖：**
- `renderer/player/PlayerRenderer.cpp` - 注册并使用这两个层渲染器

## 容易踩的坑

1. **ElytraLayer 是模板类**：支持 `LivingEntity` 和 `Player` 两种类型，需在 cpp 末尾显式实例化。

2. **shouldRender() 检查条件**：
   - CapeLayer：需检查玩家是否开启 `PlayerModelPart::Cape`、是否有披风纹理、胸甲槽是否装备鞘翅（鞘翅覆盖披风）
   - ElytraLayer：需检查胸甲槽是否装备 `Items::ELYTRA`、是否有鞘翅或披风纹理

3. **渲染顺序**：披风必须在鞘翅之前渲染，这样鞘翅才能正确覆盖披风（见上层 README.md）。

4. **纹理设置**：两层都支持自定义纹理设置（`setCapeTexture()`/`setElytraTexture()`），由 `PlayerRenderer` 在渲染前通过 `dynamic_cast` 设置。

5. **CPU 路径已废弃**：`render()` 方法保留仅为向后兼容，实际渲染使用 `renderPipeline()` 方法。

## 参考

- MC 1.16.5 CapeLayer
- MC 1.16.5 ElytraLayer
