# Frustum 模块

视锥剔除（Frustum Culling）工具库，用于判断物体是否在相机视锥内可见。

## 目录结构

```
frustum/
├── Frustum.hpp         # 视锥类定义
├── Frustum.cpp         # 视锥类实现
└── README.md           # 本文档
```

## 核心概念

### 视锥体（Frustum）

视锥体是由 6 个平面围成的截头锥体，定义了相机可见的空间范围：

- **Left/Right**: 左右裁剪面
- **Top/Bottom**: 上下裁剪面
- **Near/Far**: 近远裁剪面

### 平面方程

每个平面由方程 `Ax + By + Cz + D = 0` 表示，其中：
- `(A, B, C)` 是平面法向量（归一化后指向视锥内部）
- `D` 是平面到原点的距离

### p-vertex 优化

对于 AABB-视锥相交测试，使用 p-vertex（正向极值点）优化：
- 对每个平面，找到 AABB 上离平面法向量方向最远的顶点
- 如果 p-vertex 在平面外侧，则整个 AABB 在平面外侧
- 只需计算一个顶点而非 8 个角点

## 使用方法

### 基本用法

```cpp
#include "common/util/math/frustum/Frustum.hpp"

using namespace mc::math::frustum;

// 创建视锥体
Frustum frustum;

// 从视图-投影矩阵提取平面（每帧调用一次）
frustum.extractFromMatrix(viewProjectionMatrix);

// 设置相机位置（用于世界坐标测试）
frustum.setCameraPosition(cameraPosition);

// 测试点可见性
if (frustum.isPointVisible(point)) {
    // 点在视锥内
}

// 测试球可见性（适用于粒子、小物体）
if (frustum.isSphereVisible(center, radius)) {
    // 球与视锥相交
}

// 测试 AABB 可见性（适用于区块、实体）
AxisAlignedBB aabb = FrustumUtils::createChunkAABB(chunkX, chunkZ, minY, maxY);
if (frustum.isAABBVisibleWorld(aabb)) {
    // AABB 与视锥相交
}

// 直接测试区块可见性
if (frustum.isChunkVisible(chunkX, chunkZ, minY, maxY)) {
    // 区块可见
}
```

### 与渲染器集成

```cpp
// 每帧更新视锥
void Renderer::renderFrame(const Camera& camera) {
    // 更新视锥
    m_frustum.extractFromMatrix(camera.viewProjectionMatrix());
    m_frustum.setCameraPosition(camera.position());

    // 渲染可见区块
    for (const auto& [chunkId, buffer] : m_chunkBuffers) {
        if (m_frustum.isChunkVisible(chunkId.x, chunkId.z, m_minY, m_maxY)) {
            drawChunk(buffer);
        }
    }

    // 渲染可见实体
    for (Entity* entity : m_entities) {
        if (m_frustum.isAABBVisibleWorld(entity->boundingBox())) {
            drawEntity(entity);
        }
    }

    // 渲染可见粒子
    for (const auto& particle : m_particles) {
        if (m_frustum.isSphereVisible(particle->position(), particle->size() * 0.5f)) {
            drawParticle(particle);
        }
    }
}
```

## API 参考

### FrustumPlane

```cpp
struct FrustumPlane {
    Vector3 normal;   // 平面法向量（归一化，指向视锥内部）
    f32 distance;     // 平面到原点的距离

    f32 distanceToPoint(const Vector3& point) const;  // 计算点到平面的距离
    void normalize();                                  // 归一化平面方程
};
```

### Frustum

```cpp
class Frustum {
public:
    enum PlaneIndex { Left, Right, Bottom, Top, Near, Far };

    // 平面提取
    void extractFromMatrix(const glm::mat4& viewProjectionMatrix);
    void extractFromMatrices(const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix);

    // 相机位置设置
    void setCameraPosition(const Vector3& position);
    void setCameraPosition(const glm::vec3& position);

    // 可见性测试
    bool isPointVisible(const Vector3& point) const;
    bool isSphereVisible(const Vector3& center, f32 radius) const;
    bool isAABBVisible(const AxisAlignedBB& aabb) const;          // 相机相对坐标
    bool isAABBVisibleWorld(const AxisAlignedBB& aabb) const;     // 世界坐标
    bool isChunkVisible(i32 chunkX, i32 chunkZ, i32 minY, i32 maxY) const;
    bool isChunkSectionVisible(i32 chunkX, i32 sectionY, i32 chunkZ) const;

    // 访问器
    const FrustumPlane& getPlane(PlaneIndex index) const;
    const Vector3& getCameraPosition() const;
    bool isValid() const;
};
```

### FrustumUtils

```cpp
namespace FrustumUtils {
    AxisAlignedBB createChunkAABB(i32 chunkX, i32 chunkZ, i32 minY, i32 maxY);
    AxisAlignedBB createSectionAABB(i32 chunkX, i32 sectionY, i32 chunkZ, i32 sectionHeight = 16);
    AxisAlignedBB createEntityAABB(const Vector3& position, f32 width, f32 height);
    AxisAlignedBB createBlockAABB(i32 x, i32 y, i32 z);
    AxisAlignedBB expandAABB(const AxisAlignedBB& aabb, f32 margin);
}
```

## 性能提示

1. **每帧只提取一次**：`extractFromMatrix()` 每帧调用一次即可
2. **使用正确的测试方法**：
   - 点测试：最精确，适合小物体
   - 球测试：快速，适合粒子和圆形物体
   - AABB 测试：适合区块和实体
3. **相机相对坐标**：`isAABBVisibleWorld()` 自动转换，提高大坐标精度
4. **提前退出**：测试从左平面开始，依次测试所有平面

## 容易踩的坑

### 1. 矩阵顺序

```cpp
// 错误：矩阵顺序不对
frustum.extractFromMatrix(viewMatrix * projectionMatrix);  // 错误！

// 正确：projection * view
frustum.extractFromMatrix(projectionMatrix * viewMatrix);  // 正确
// 或者直接使用 Camera 提供的 VP 矩阵
frustum.extractFromMatrix(camera.viewProjectionMatrix());  // 推荐
```

### 2. 坐标系

```cpp
// isAABBVisible() 期望相机相对坐标
AxisAlignedBB relativeAABB(
    worldAABB.minX - cameraPos.x,
    worldAABB.minY - cameraPos.y,
    worldAABB.minZ - cameraPos.z,
    ...
);

// isAABBVisibleWorld() 自动处理转换
frustum.setCameraPosition(cameraPos);
frustum.isAABBVisibleWorld(worldAABB);  // 自动转换
```

### 3. 保守测试

视锥剔除是保守测试：
- 可能报告可见但实际不可见（false positive）
- 不会报告不可见但实际可见（false negative）

这是设计如此，确保不会遗漏任何可见物体。

### 4. 平面归一化

平面方程必须归一化才能正确计算距离。`extractFromMatrix()` 会自动调用 `normalize()`。

## 算法参考

- **Gribb-Hartmann 方法**：从 VP 矩阵提取视锥平面
- **p-vertex 优化**：快速 AABB-平面相交测试
- 参考：Minecraft 1.16.5 `ClippingHelper.java`
