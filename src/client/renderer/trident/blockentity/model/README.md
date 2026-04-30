# blockentity/model/ - 方块实体模型

方块实体模型模块，提供方块实体的模型定义和网格生成。

## 目录结构

```
model/
├── BlockEntityModel.hpp/cpp     # 方块实体模型基类
├── ChestModel.hpp/cpp           # 箱子模型
├── BeaconBeamModel.hpp/cpp      # 信标光束模型
├── README.md                    # 本文件
```

## 设计原则

### 参考 MC 1.16.5 模式

大多数方块实体模型**不在单独的模型类中定义**，而是直接在渲染器中创建 `ModelRenderer` 部件。

**例外：**
- `DragonHeadModel` - 龙首模型（单独文件）
- `SignModel` - 告示牌模型（内部类）

### 复用实体模型系统

本项目复用 `client/renderer/trident/entity/model/core/ModelRenderer.hpp` 作为模型部件：

```cpp
// 箱子模型示例
class ChestModel : public BlockEntityModel {
public:
    ChestModel() {
        // 创建部件
        m_lid = createPart("lid", 64, 64);
        m_lid->addBox(1.0, 0.0, 0.0, 14.0, 5.0, 14.0, 0.0);
        m_lid->setRotationPoint(0.0, 9.0, 1.0);

        // ...
    }
};
```

## 已实现的模型

### BlockEntityModel（基类）

提供方块实体模型的通用功能：
- 部件管理
- 网格生成
- 纹理尺寸设置

### ChestModel（箱子模型）

渲染箱子的三个部件：
- `singleBottom`: 箱体 (1,0,1) 到 (15,10,15)
- `singleLid`: 盖子 (1,0,0) 到 (15,5,14)，旋转点 (0,9,1)
- `singleLatch`: 锁扣 (7,-1,15) 到 (9,3,16)，旋转点 (0,8,0)

支持三种类型：
- `Single`: 单箱
- `Left`: 双箱左半
- `Right`: 双箱右半

**缓动函数（MC 1.16.5 对齐）：**
```cpp
f32 ChestModel::applyEasing(f32 angle) {
    f32 eased = 1.0f - angle;
    eased = 1.0f - eased * eased * eased;
    return eased;
}
```

### BeaconBeamModel（信标光束模型）

渲染信标的垂直光束效果：
- 双层渲染：内层光束(radius=0.2) + 外层光晕(radius=0.25)
- 支持多段光束（每段不同颜色）
- 旋转动画：`floorMod(gameTime, 40) + partialTick) * 2.25 - 45` 度
- 最后一段高度固定为 1024 格

**光束段数据结构：**
```cpp
using BeamSegment = mc::blockentity::BeaconBeamSegment;
// 包含：colors (RGB数组) 和 height (高度)
```

**渲染参数（MC 1.16.5 对齐）：**
- 内层光束：alpha = 1.0, radius = 0.2
- 外层光晕：alpha = 0.125, radius = 0.25
- 纹理 V 坐标：`frac(f * 0.2 - floor(f * 0.1))`

## 待实现的模型

### BellModel（钟模型）

钟体和支架，支持摆动动画。

### SignModel（告示牌模型）

告示牌的牌子 + 支柱。

### BedModel（床模型）

床头、床尾和四条床腿。

### ShulkerBoxModel（潜影盒模型）

可复用实体模型系统中的 `ShulkerModel`。

### LecternModel（讲台模型）

讲台和书本。

### BannerModel（旗帜模型）

旗帜和旗杆，支持飘动动画。

## 与实体模型的关系

| 特性 | 实体模型 | 方块实体模型 |
|------|----------|--------------|
| 基类 | EntityModel | BlockEntityModel |
| 部件 | ModelRenderer | ModelRenderer（复用） |
| 动画 | setAngles() | 部件旋转 |
| 位置 | 世界坐标 | 方块坐标 + 偏移 |
| 光照 | 实体光照 | 方块光照 |

## 使用方法

```cpp
// 在渲染器中使用模型
void ChestRenderer::render(const ChestEntity& entity, f32 partialTick, u32 light) {
    // 更新模型状态
    f32 lidAngle = entity.getInterpolatedLidAngle(partialTick);
    m_model.setLidAngle(lidAngle);

    // 设置箱子类型
    m_model.setChestType(ChestModel::ChestType::Single);

    // 生成网格
    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;
    m_model.generateMesh(vertices, indices);

    // 提交到渲染管线
    // ...
}
```

## 命名空间

```cpp
namespace mc::client::renderer::blockentity::model {
    class BlockEntityModel;
    class ChestModel;
    class BeaconBeamModel;
    // ...
}
```
