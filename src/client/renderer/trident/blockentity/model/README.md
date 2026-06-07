# blockentity/model/ - 方块实体模型

方块实体模型模块，提供方块实体的模型定义和网格生成。

## 目录结构

```
model/
├── BlockEntityModel.hpp/cpp     # 方块实体模型基类，提供部件管理和网格生成
├── ChestModel.hpp/cpp           # 箱子模型，支持单箱和双箱模式
├── BeaconBeamModel.hpp/cpp      # 信标光束模型，双层渲染和旋转动画
├── BannerModel.hpp/cpp          # 旗帜模型，支持站立和墙壁两种形态
└── README.md                    # 本文件
```

## 内部模块关系

```
BlockEntityModel (基类)
    ├── ChestModel      (箱子模型，继承 BlockEntityModel)
    ├── BannerModel     (旗帜模型，继承 BlockEntityModel)
    └── BeaconBeamModel (信标光束模型，独立实现，不继承 BlockEntityModel)
```

- `BlockEntityModel`：提供部件管理（`createPart`）、网格生成（`generateMesh`）、可见性控制等通用功能
- `ChestModel`/`BannerModel`：继承基类，复用 ModelRenderer 部件系统
- `BeaconBeamModel`：独立实现，直接生成光束几何体（不使用 ModelRenderer）

## 上下游外部依赖关系

### 依赖（上游）

- `client/renderer/trident/entity/model/core/ModelRenderer.hpp` - 模型部件系统（被 BlockEntityModel、ChestModel、BannerModel 使用）
- `common/world/blockentity/processing/BeaconEntity.hpp` - BeaconBeamSegment 数据结构
- `common/util/math/MathConstants.hpp` - 数学常量（PI 等）

### 被依赖（下游）

- `blockentity/renderers/ChestRenderer.hpp` - 使用 ChestModel
- `blockentity/renderers/BeaconRenderer.hpp` - 使用 BeaconBeamModel
- `blockentity/renderers/BannerRenderer.hpp` - 使用 BannerModel

## 容易踩的坑

1. **BeaconBeamModel 不继承 BlockEntityModel**：信标光束是程序化生成的几何体，不使用 ModelRenderer 部件系统，调用 `generateMesh` 时需要额外传入 `gameTime` 和 `partialTick` 参数

2. **箱子缓动函数**：MC 1.16.5 的箱子开合使用三次缓动，公式为 `eased = 1.0 - (1.0 - angle)³`，而非线性插值

3. **箱子类型切换**：`setChestType()` 会切换内部部件指针，切换后需重新生成网格

4. **旗帜类型差异**：站立旗帜有16个方向旋转，墙壁旗帜只有4个方向，初始化部件不同

5. **信标光束旋转公式**：`(floorMod(gameTime, 40) + partialTick) * 2.25 - 45` 度，不要直接使用 gameTime

6. **信标光束双层渲染**：内层光束 radius=0.2、alpha=1.0，外层光晕 radius=0.25、alpha=0.125，需分别渲染

7. **纹理尺寸**：ChestModel 和 BannerModel 的纹理尺寸都是 64x64，创建部件时需正确设置

## 命名空间

```cpp
namespace mc::client::renderer::blockentity::model {
    class BlockEntityModel;
    class ChestModel;
    class BeaconBeamModel;
    class BannerModel;
}
```
