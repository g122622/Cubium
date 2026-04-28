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
│   ├── ChestModel.hpp/cpp             # 箱子模型
│   ├── BeaconModel.hpp/cpp            # 信标模型
│   └── BellModel.hpp/cpp              # 钟模型
└── renderers/                         # 具体渲染器实现
    ├── PistonRenderer.hpp/cpp         # 活塞渲染器 ✓
    ├── ChestRenderer.hpp/cpp          # 箱子渲染器 ✓
    ├── BeaconRenderer.hpp/cpp         # 信标渲染器 ✓
    └── ...                            # 其他渲染器（待实现）
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
dispatcher.registerRenderer<PistonBlockEntity, PistonRenderer>();
dispatcher.registerRenderer<ChestEntity, ChestRenderer>();
```

### 实现渲染器

```cpp
class ChestRenderer : public BlockEntityRendererBase,
                      public IBlockEntityRenderer<ChestEntity> {
public:
    void render(const ChestEntity& entity, float partialTick, u32 light) override {
        // 获取插值后的盖子角度
        float angle = entity.getLidAngle(partialTick);

        // 非线性缓动
        angle = 1.0f - angle;
        angle = 1.0f - angle * angle * angle;

        // 渲染箱体
        renderBlock(*entity.getBlockState(), entity.getPos(), light);

        // 渲染盖子（旋转）
        // ... 应用旋转变换
    }
};
```

## 依赖项

### 内部依赖

- `common/world/blockentity/` - 方块实体定义
- `common/world/block/` - 方块状态
- `client/resource/BlockModelCache` - 方块模型缓存
- `client/renderer/trident/core/` - 渲染核心组件

### 外部依赖

- `spdlog` - 日志
- `glm` - 数学库（矩阵变换）

## 测试用例

方块实体逻辑测试位于 `tests/common/world/blockentity/`:
- `ChestEntityTest.cpp` - 箱子实体测试（开合计数、盖子动画、序列化）
- `TrappedChestEntityTest.cpp` - 陷阱箱测试
- `BlockEntityTodoTest.cpp` - 方块实体综合测试（活塞、信标、末影箱等）

## 实现状态

| 组件 | 状态 | 说明 |
|------|------|------|
| `IBlockEntityRenderer` | ✅ 完成 | 渲染器接口模板 |
| `BlockEntityRenderer` | ✅ 完成 | 渲染器基类 |
| `BlockEntityRendererDispatcher` | ⚠️ 部分 | 需要实现类型安全的渲染分派 |
| `PistonRenderer` | ⚠️ 部分 | 核心逻辑完成，渲染待实现 |
| `ChestRenderer` | ⚠️ 部分 | 插值算法完成，渲染待实现 |
| `BeaconRenderer` | ⚠️ 部分 | 光束旋转公式完成，渲染待实现 |

## MC 1.16.5 对齐要点

1. **活塞插值**：`getProgress(partialTick)` 使用 `lerp(lastProgress, progress, partialTick)`
2. **活塞偏移**：`getExtendedProgress(progress)` 返回 `extending ? progress - 1.0 : 1.0 - progress`
3. **信标光束旋转**：`floorMod(gameTime, 40) + partialTick) * 2.25 - 45` 度
4. **箱子盖缓动**：`angle = 1.0 - angle; angle = 1.0 - angle * angle * angle`
5. **动画精灵帧切换**：帧索引变化时才上传，插值模式持续上传
