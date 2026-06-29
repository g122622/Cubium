#物品网格构建与渲染

本目录包含物品（Item）的3D网格构建和渲染相关模块，负责将物品数据转换为可渲染的3D网格数据。

        ##目录结构

``` item /
├── ItemMeshBuilder.hpp /
        cpp #物品网格构建器，静态工具类
├── ItemRenderer.hpp /
        cpp #物品渲染器，管理物品渲染流程
└── README.md
```

        ##内部模块关系

``` ItemMeshBuilder         ← 静态工具类，无实例依赖
    ↓ 提供网格数据 ItemRenderer            ← 调用 ItemMeshBuilder 生成网格，提交渲染管线
```

        ##ItemMeshBuilder 公开接口

        ## #网格构建

    | 方法 | 说明 | | -- -- --| -- -- --|
    | `buildHeldItemMesh(itemStack, transformType)` | 构建手持 / 物品栏物品网格，返回 `(vertices, indices)` |
    | `buildGroundItemMesh(itemStack, rotation)` | 构建地面掉落物品网格 |
    | `buildHeadItemMesh(itemStack)` | 构建头盔（戴在头上）物品网格 |
    | `buildArmorMesh(itemStack, armorSlot, poseMatrix)` | 构建盔甲物品网格 |
    | `buildIconMesh(textureRegion, size)` | 构建2D图标网格（单面四边形） |
    | `getItemTransform(transformType, guiPx, guiPy, leftHanded)` | 获取指定变换类型的4x4矩阵 |

    ## #纹理图集注入

    | 方法 | 说明 | | -- -- --| -- -- --|
    | `setItemTextureAtlas(atlas)` | 注入 `ItemTextureAtlas` 指针，用于纹理解析。`nullptr` 表示不使用图集 |

    > **重要**：`setItemTextureAtlas()` 必须在 `TridentEngine::
                initializeItemRenderer()` 中调用以注入图集实例，并在 `TridentEngine::
                    destroy()` 中传入 `nullptr` 清除指针，避免悬垂引用。

            ## #内部网格构建策略

`buildHeldItemMesh` 的内部路径：

``` buildHeldItemMesh()
  └─> _build3DItemMesh()
       ├─ model
        ==
        nullptr → _buildFallbackMesh() // 6面立方体回退
       ├─ Generated /
                Handheld → _buildGeneratedMesh() // 2D层叠物品
       ├─ Block → _buildBlockItemMesh() // 方块物品
       ├─ Custom → _buildCustomMesh() // 自定义模型
       └─ default → _buildFallbackMesh() // 未知类型回退
```

                ##上下游外部依赖关系

                    ** 本目录依赖：* *
                - `client / renderer / trident / entity / model / core / ModelRenderer.hpp` - `ModelVertex` 顶点结构体
            - `client / renderer / api / texture / TextureRegion.hpp` - 纹理区域定义
            - `client / resource / ItemModelCache.hpp` - 物品模型缓存查询
            - `client / resource / ItemTextureAtlas.hpp` - 物品纹理图集（通过 `setItemTextureAtlas()` 注入）
            - `common / item / core / Item.hpp` - 物品基类 - `common / item / core / ItemStack.hpp` - 物品堆栈
            - `common / item / items / block / BlockItem.hpp` - 方块物品判断
            - `<glm / glm.hpp>` / `<glm / gtc / matrix_transform.hpp>` -
            矩阵运算

                    ** 依赖本目录：* *
                - `client / renderer / trident / core / TridentEngine.cpp` -
            初始化 / 销毁时注入 / 清除 `ItemTextureAtlas` - `client / renderer / trident / firstperson /` -
            第一人称手持物品渲染 - `client / renderer / trident / entity / layer / equipment /` -
            实体装备层渲染

                ##容易踩的坑

                ## #1. ItemTextureAtlas 是注入式依赖

`ItemMeshBuilder` 不持有 `ItemTextureAtlas` 的所有权，通过 `setItemTextureAtlas()` 静态方法注入。必须确保在 `TridentEngine` 销毁时清除指针（设为 `nullptr`），否则产生悬垂指针。

                ## #2. 回退网格是6面立方体

                当 `ItemModelCache` 中没有对应物品模型时，`_buildFallbackMesh()` 会生成一个完整的6面立方体（24顶点、36索引），每面法线朝外、UV覆盖完整 `
                    [0, 1]` 范围。这不是简单的2D四边形。

                ## #3. 法线变换使用逆法线矩阵

`_applyMatrixToVertices()` 和 `ModelRenderer::_transformVertex()` 均使用 `glm::transpose(
                    glm::inverse(mat3))` 进行法线变换，确保非均匀缩放下法线方向正确。

                ## #4. 坐标单位

                物品网格坐标使用 MC 1 /
                16 单位，`ITEM_SCALE = 1.0 /
    16.0`。回退立方体的半边长为 `ITEM_SCALE* 16.0 * 0.5 = 0.5`。

                                                          ## #5. 元素旋转（ModelRotation）

`_buildBlockItemMesh()` 支持方块物品模型的元素旋转，参考 MC 原版 `FaceBakery.bakeQuad()` 实现：

- **旋转矩阵构建**（`buildElementRotationMatrix()`）：根据 `ModelRotation` 构建组合变换 `T(origin) * R * T(-origin)`，支持两种旋转格式：
  - 传统 axis+angle 格式（`isEulerXYZ == false`）：单轴旋转
  - EulerXYZ 格式（`isEulerXYZ == true`，MC 1.21.11 新增）：三轴 ZYX 欧拉角旋转，参考 MC `BlockElementRotation.EulerXYZRotation.transformation()`
- **Rescale 补偿**（`computeRescaleFactors()`）：当 `ModelRotation.rescale == true` 时，计算各轴缩放因子补偿旋转投影收缩。例如 Y 轴旋转 45° 时 X/Z 缩放因子为 √2 ≈ 1.414，确保旋转后的十字植物模型不会显得过窄。对两种旋转格式均通用，因为基于最终旋转矩阵计算
- **法线重算**：旋转后通过叉积 `cross(edge1, edge2)` 重新计算面法线，与 MC 原版 `calculateFacing()` 行为一致

            ## #6. UV 旋转（ModelFaceUV
                .rotation）

`_getRotatedUV()` 实现了 MC 原版的 `Quadrant.rotateVertexIndex()` 顶点索引排列算法：

        - `face.uv.rotation` 字段（0 / 90 / 180 / 270 度）通过 `shift = rotation / 90` 计算索引偏移
        - 每个顶点 `i` 使用无旋转时顶点 `(i + shift) % 4` 的 UV 坐标 -
        无旋转时各顶点 UV 映射：顶点0→(u0, v1)、顶点1→(u1, v1)、顶点2→(u1, v0)、顶点3→(u0, v0)

            ## #7. 旋转辅助函数

        旋转计算的核心逻辑提取为独立头文件 `ElementRotation.hpp` 中的自由函数，便于单元测试直接调用：

    | 函数 | 说明 | | -- -- --| -- -- --|
    | `buildElementRotationMatrix(rotation, scale)` | 构建元素旋转矩阵（含 rescale 缩放） |
    | `computeRescaleFactors(rotMatrix3x3)` | 计算旋转后各轴的 rescale 缩放因子 |
    | `getRotatedUV(vertexIndex, uvRotation, u0, v0, u1, v1)` | 获取 UV 旋转后的顶点 UV 坐标 |

    ##命名空间

```cpp namespace mc::client::renderer::entity::item
{
    class ItemMeshBuilder;
    class ItemRenderer;
    enum class ItemTransformType : u8;
}
```
