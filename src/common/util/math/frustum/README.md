# Frustum 模块

视锥剔除（Frustum Culling）工具库，用于判断物体是否在相机视锥内可见。

## 目录结构

```
frustum/
├── Frustum.hpp         # 视锥类定义，包含 FrustumPlane 结构体、Frustum 类和 FrustumUtils 命名空间
├── Frustum.cpp         # 视锥类实现，包含 Gribb-Hartmann 平面提取算法和 p-vertex AABB 测试
└── README.md           # 本文档
```

## 内部模块关系

```
FrustumPlane (结构体)
     │
     └─存储于─> Frustum (类)
                    │
                    └─使用──> FrustumUtils (命名空间)
                                    │
                                    └─创建 AABB 辅助函数
```

- **FrustumPlane**：存储平面方程 `Ax + By + Cz + D = 0`，提供 `distanceToPoint()` 和 `normalize()` 方法
- **Frustum**：核心视锥类，存储 6 个裁剪平面，提供点/球/AABB/区块可见性测试
- **FrustumUtils**：AABB 创建工具函数，用于快速创建区块/区块段/实体/方块的 AABB

## 上下游外部依赖关系

### 本模块依赖

| 依赖项 | 用途 |
|--------|------|
| `glm::mat4` | VP 矩阵输入类型 |
| `Vector3` | 向量类型 |
| `AxisAlignedBB` | AABB 碰撞盒类型 |
| `CHUNK_WIDTH`, `CHUNK_SECTION_HEIGHT` | 区块尺寸常量 |

### 被依赖（上游模块）

| 模块 | 使用方式 |
|------|----------|
| `ChunkRenderer` | 区块渲染剔除 |
| `EntityRendererManager` | 实体渲染剔除 |
| `ParticleManager` | 粒子渲染剔除 |
| `WeatherRenderer` | 天气效果剔除 |
| `NameTagRenderer`, `WorldTextRenderer` | 文字渲染剔除 |
| `ClientWorld` | 世界可见性查询 |
| `MeshBuildScheduler` | 网格构建剔除 |
| `TridentEngine` | 持有视锥实例 |

## 容易踩的坑

### 1. 矩阵顺序错误

```cpp
// 错误：矩阵顺序不对
frustum.extractFromMatrix(viewMatrix * projectionMatrix);  // 错误！

// 正确：projection * view
frustum.extractFromMatrix(projectionMatrix * viewMatrix);  // 正确
// 或者直接使用 Camera 提供的 VP 矩阵
frustum.extractFromMatrix(camera.viewProjectionMatrix());  // 推荐
```

### 2. 坐标系混淆

- `isAABBVisible()` 期望**相机相对坐标**，需要手动转换
- `isAABBVisibleWorld()` 自动转换为相机相对坐标，使用前必须调用 `setCameraPosition()`

```cpp
// 推荐：使用 World 版本，自动处理坐标转换
frustum.setCameraPosition(cameraPos);
frustum.isAABBVisibleWorld(worldAABB);  // 自动转换
```

### 3. 视锥剔除是保守测试

- 可能报告可见但实际不可见（false positive）
- 不会报告不可见但实际可见（false negative）

这是设计如此，确保不会遗漏任何可见物体。

### 4. 平面归一化

平面方程必须归一化才能正确计算距离。`extractFromMatrix()` 会自动调用 `normalize()`，手动构造平面时需要自行归一化。

### 5. 每帧更新

相机移动后必须调用 `extractFromMatrix()` 更新视锥平面，否则剔除结果会过时。

## 算法参考

- **Gribb-Hartmann 方法**：从 VP 矩阵提取视锥平面
- **p-vertex 优化**：快速 AABB-平面相交测试，只需计算一个顶点而非 8 个角点
- 参考：Minecraft 1.16.5 `ClippingHelper.java`
