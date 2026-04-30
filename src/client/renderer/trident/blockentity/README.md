# blockentity/ - 方块实体渲染器

方块实体渲染器模块，负责渲染需要动态效果的方块实体（如箱子、活塞、信标等）。

## 架构

参考 `entity/` 模块的设计模式：

| 组件 | 说明 |
|------|------|
| `IBlockEntityRenderer<TEntity>` | 渲染器接口模板，定义渲染契约 |
| `BlockEntityRendererBase` | 渲染器基类，提供方块渲染辅助方法 |
| `BlockEntityRendererDispatcher` | 渲染器调度器，管理注册和分派 |

## 目录结构

```text
blockentity/
├── IBlockEntityRenderer.hpp          # 渲染器接口模板
├── BlockEntityRenderer.hpp/cpp       # 渲染器基类
├── BlockEntityRendererDispatcher.hpp/cpp  # 渲染器调度器
├── README.md                          # 本文件
├── model/                             # 方块实体模型
│   ├── BlockEntityModel.hpp/cpp       # 方块实体模型基类
│   ├── ChestModel.hpp/cpp             # 箱子模型
│   └── BeaconBeamModel.hpp/cpp        # 信标光束模型
└── renderers/                         # 具体渲染器实现
    ├── PistonRenderer.hpp/cpp         # 活塞渲染器
    ├── ChestRenderer.hpp/cpp          # 箱子渲染器
    └── BeaconRenderer.hpp/cpp         # 信标渲染器
```

## 渲染流程

1. **初始化阶段**
   - `BlockEntityRendererDispatcher::initializeDefaults()` 注册所有渲染器
   - 设置模型缓存和纹理图集

2. **渲染阶段**
   - 遍历世界中的方块实体
   - `BlockEntityRendererDispatcher::render()` 根据类型查找渲染器
   - 渲染器使用 `partialTick` 进行插值动画
   - 调用 `BlockModelCache` 获取方块模型

## 与 EntityRenderer 的区别

| 特性 | EntityRenderer | BlockEntityRenderer |
|------|----------------|---------------------|
| 数据来源 | `ClientEntity` | `BlockEntity` |
| 渲染位置 | 世界坐标（实体位置） | 方块坐标 + 动画偏移 |
| 光照 | 实体光照计算 | 方块光照（天空光+方块光） |
| 动画 | `AnimationContext` | `partialTick` 插值 |
| 模型 | `EntityModel` | `BlockAppearance` |
| 全局可见 | 少数实体（如发光鱿鱼） | 信标光束等 |

## 动画模式

方块实体动画使用 `partialTick` 插值实现平滑效果：

```cpp
// 箱子盖子角度插值
float getLidAngle(float partialTick) const {
    return lerp(m_prevLidAngle, m_lidAngle, partialTick);
}

// 非线性缓动（MC风格）
float angle = getLidAngle(partialTick);
angle = 1.0f - angle;
angle = 1.0f - angle * angle * angle;  // 三次缓动
```

## 全局渲染器

某些方块实体需要跨区块可见（如信标光束），需实现 `isGlobalRenderer() = true`：

```cpp
class BeaconRenderer : public BlockEntityRendererBase,
                       public IBlockEntityRenderer<BeaconEntity> {
public:
    [[nodiscard]] bool isGlobalRenderer() const override {
        return true;  // 光束需要远距离可见
    }
};
```

## 使用示例

### 注册渲染器

```cpp
// 初始化时注册
dispatcher.registerRenderer(BlockEntityType::Beacon, []() {
    return std::make_unique<BeaconRenderer>();
});
dispatcher.registerRenderer(BlockEntityType::Chest, []() {
    return std::make_unique<ChestRenderer>();
});
```

### 实现渲染器

```cpp
class ChestRenderer : public BlockEntityRendererBase,
                      public IBlockEntityRenderer<ChestEntity> {
public:
    void render(const ChestEntity& entity, float partialTick, u32 light) override {
        // 获取插值后的盖子角度
        float angle = entity.getInterpolatedLidAngle(partialTick);

        // 应用 MC 风格缓动
        m_model.setLidAngle(angle);

        // 设置箱子类型
        m_model.setChestType(determineChestType(entity));

        // 生成网格
        std::vector<ModelVertex> vertices;
        std::vector<u32> indices;
        m_model.generateMesh(vertices, indices);
    }
};
```

## 依赖项

### 内部依赖

- `common/world/blockentity/` - 方块实体定义
- `common/world/block/` - 方块状态
- `client/resource/BlockModelCache` - 方块模型缓存
- `client/renderer/trident/core/` - 渲染核心组件
- `client/renderer/trident/entity/model/core/ModelRenderer.hpp` - 模型渲染器

### 外部依赖

- `spdlog` - 日志
- `glm` - 数学库（矩阵变换）

## 测试用例

方块实体逻辑测试位于 `tests/common/world/blockentity/`:
- `ChestEntityTest.cpp` - 箱子实体测试（开合计数、盖子动画、序列化）
- `FurnaceEntityTest.cpp` - 熔炉实体测试
- `HopperEntityTest.cpp` - 漏斗实体测试
- `BlockEntityTodoTest.cpp` - 方块实体综合测试（活塞、信标等）

模型测试位于 `tests/client/renderer/trident/blockentity/`:
- `ChestModelTest.cpp` - 箱子模型测试（类型切换、缓动函数、网格生成）
- `BeaconBeamModelTest.cpp` - 信标光束模型测试（旋转计算、光束段管理）

## 实现状态

| 组件 | 状态 | 说明 |
|------|------|------|
| `IBlockEntityRenderer` | ✅ 完成 | 渲染器接口模板 |
| `BlockEntityRenderer` | ✅ 完成 | 渲染器基类 |
| `BlockEntityRendererDispatcher` | ✅ 完成 | 渲染器注册和分派 |
| `BlockEntityModel` | ✅ 完成 | 方块实体模型基类 |
| `ChestModel` | ✅ 完成 | 箱子模型（缓动函数、类型切换） |
| `BeaconBeamModel` | ✅ 完成 | 信标光束模型（旋转公式、光束段） |
| `PistonRenderer` | ✅ 完成 | 活塞渲染器（插值、偏移计算） |
| `ChestRenderer` | ✅ 完成 | 箱子渲染器（圣诞节纹理检测、类型判断） |
| `BeaconRenderer` | ✅ 完成 | 信标渲染器（光束段渲染、旋转计算） |

## MC 1.16.5 对齐要点

1. **活塞插值**：`getProgress(partialTick)` 使用 `lerp(lastProgress, progress, partialTick)`
2. **活塞偏移**：`getExtendedProgress(progress)` 返回 `extending ? progress - 1.0 : 1.0 - progress`
3. **信标光束旋转**：`floorMod(gameTime, 40) + partialTick) * 2.25 - 45` 度
4. **信标光束半径**：内层 `BEAM_RADIUS = 0.2f`，外层光晕 `GLOW_RADIUS = 0.25f`
5. **信标光束透明度**：内层 `alpha = 1.0f`，外层光晕 `alpha = 0.125f`
6. **信标最大高度**：`MAX_BEAM_HEIGHT = 1024` 格
7. **箱子盖缓动**：`angle = 1.0 - angle; angle = 1.0 - angle * angle * angle`
8. **圣诞节纹理**：12月24-26日使用 `textures/entity/chest/christmas.png`
