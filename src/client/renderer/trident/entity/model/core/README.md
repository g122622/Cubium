# 模型核心

本目录包含实体模型系统的核心类，是整个实体模型系统的基础设施。

## 目录结构

```
core/
├── EntityModel.hpp/cpp       # 实体模型基类，定义动画和渲染接口
├── ModelRenderer.hpp/cpp     # 模型部件渲染器，管理盒子、旋转和子部件
├── AgeableModel.hpp/cpp      # 可成长模型基类，支持幼体/成年状态切换
├── SegmentedModel.hpp/cpp    # 分段模型基类，用于复杂实体（如末影龙）的分段渲染
├── ModelFactory.hpp/cpp      # 模型工厂，注册表模式管理模型创建
└── README.md
```

## 内部模块关系

```
ModelRenderer          ← 基础构件，无依赖
    ↓
EntityModel            ← 持有 ModelRenderer 列表
    ↓
AgeableModel           ← 继承 EntityModel，添加幼体缩放逻辑
SegmentedModel         ← 继承 EntityModel，添加分段渲染

ModelFactory           ← 创建 EntityModel 实例（依赖所有模型类）
```

## 上下游外部依赖关系

**本目录依赖：**
- `common/core/Types.hpp` - 基础类型定义（i32, f32, f64 等）
- `common/util/math/Vector2.hpp` - 2D 向量（UV 坐标）
- `common/util/math/Vector3.hpp` - 3D 向量（位置、法线）
- `common/util/assert/AssertAll.hpp` - 断言宏

**依赖本目录：**
- `model/base/` - BipedModel、QuadrupedModel 继承 EntityModel
- `model/animal/` - 各种动物模型继承 AgeableModel
- `model/monster/` - 各种怪物模型继承 EntityModel
- `model/player/` - PlayerModel 继承 BipedModel
- `model/projectile/` - 投掷物模型继承 EntityModel
- `model/nether/` - 下界生物模型
- `model/aquatic/` - 水生生物模型
- `renderer/core/LivingRenderer.hpp` - 使用 EntityModel 接口
- `layer/` - 各种层渲染器使用 ModelRenderer 访问模型部件

## 容易踩的坑

### 1. ModelRenderer::render() 已废弃

项目已改用 GPU 管线路径，`render()` 方法为遗留的 CPU 立即模式接口。应使用 `generateMesh()` 生成网格数据，然后通过 EntityPipeline 提交到 GPU。

### 2. 幼体模型缩放逻辑

`AgeableModel` 的幼体渲染需要分离头身矩阵变换，子类必须实现 `getHeadParts()` 和 `getBodyParts()` 以正确缩放头部和身体。默认 `getHeadParts()` 返回空列表，`getBodyParts()` 返回 `m_parts`。

### 3. 坐标单位和缩放

模型坐标使用 MC 1/16 单位，默认 `scale = 1.0 / 16.0` 将其转换为渲染坐标。盒子顶点按 scale 缩放，旋转点也按 scale 平移。

### 4. copyAnglesTo/copyAnglesFrom 的前提条件

这两个方法要求源模型和目标模型的 `m_parts` 数量相同，否则会触发断言失败。用于模型动画同步（如盔甲架与玩家模型）。

### 5. EntityModel 的默认 m_isChild 值

`EntityModel::m_isChild` 默认为 `true`，这可能不符合预期。使用前应显式调用 `setChild(false)` 设置成年状态。

### 6. ModelFactory 实体类型 ID 规范化

ModelFactory 会自动规范化实体类型 ID，确保有命名空间前缀（如 `pig` → `minecraft:pig`）。注册和查询时使用规范形式更安全。

## 命名空间

```cpp
namespace mc::client::renderer::entity::model {
    class EntityModel;
    class ModelRenderer;
    class AgeableModel;
    class SegmentedModel;
    class ModelFactory;
    struct ModelVertex;
    struct TexturedQuad;
    struct ModelBox;
}
```
