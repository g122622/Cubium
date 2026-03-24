#pragma once

#include "../../../../common/core/Types.hpp"
#include "../../../../common/world/chunk/ChunkData.hpp"
#include "../../../../common/world/block/Block.hpp"
#include "../../MeshTypes.hpp"
#include <array>

namespace mc {

// 前向声明
class BlockModelCache;
struct BlockAppearance;
struct TextureRegion;

namespace fluid {
class Fluid;
}

namespace client::renderer {

/**
 * @brief 流体网格生成器
 *
 * 参考 MC 1.16.5 FluidBlockRenderer.java 实现。
 * 负责生成水面、水下边界面和流动水的网格数据。
 *
 * 流体渲染特性：
 * - 水面高度根据流体 level 属性动态计算
 * - 源方块高度 = 0.888... (8/9)
 * - 流动水高度 = level / 9.0
 * - 流动方向通过 UV 旋转体现
 *
 * 使用示例：
 * @code
 * MeshData fluidMesh;
 * FluidMesher::generateFluidMesh(chunk, fluidMesh, neighbors);
 * @endcode
 */
class FluidMesher {
public:
    // ========================================================================
    // 流体网格生成
    // ========================================================================

    /**
     * @brief 生成流体网格
     *
     * 遍历区块中所有流体方块，生成水面和边界面。
     * 流体网格会被添加到透明网格中单独渲染。
     *
     * @param chunk 区块数据
     * @param outMesh 输出网格（透明网格）
     * @param neighbors 周围6个区块 (用于边界面的剔除)
     *                  顺序: -X, +X, -Z, +Z, -Y, +Y (可以是nullptr)
     */
    static void generateFluidMesh(
        const ChunkData& chunk,
        MeshData& outMesh,
        const ChunkData* neighbors[6]);

    // ========================================================================
    // 配置
    // ========================================================================

    /**
     * @brief 设置 BlockModelCache
     *
     * 必须在使用 FluidMesher 之前调用。
     *
     * @param cache 模型缓存指针
     */
    static void setModelCache(BlockModelCache* cache);

    /**
     * @brief 获取 BlockModelCache
     */
    static BlockModelCache* modelCache() { return s_modelCache; }

private:
    // ========================================================================
    // 流体高度计算
    // ========================================================================

    /**
     * @brief 获取流体渲染高度
     *
     * 根据流体状态计算渲染高度：
     * - 源方块（isSource() == true）: 返回 0.888... (8/9)
     * - 流动水：返回 level / 9.0
     * - 空气：返回 0.0
     *
     * @param blockState 方块状态
     * @return 渲染高度 (0.0 - 1.0)
     */
    [[nodiscard]] static f32 getFluidHeight(const BlockState* blockState);

    /**
     * @brief 获取流体实际高度（用于边界面）
     *
     * 参考 FluidBlockRenderer.getActualHeight()
     * 与 getFluidHeight 的区别在于对非流体方块的处理。
     *
     * @param chunk 区块数据
     * @param x 区块局部X坐标
     * @param y 世界Y坐标
     * @param z 区块局部Z坐标
     * @param neighbors 周围区块
     * @param fluid 流体类型（用于判断同类型流体）
     * @return 实际高度
     */
    [[nodiscard]] static f32 getActualFluidHeight(
        const ChunkData& chunk,
        i32 x, i32 y, i32 z,
        const ChunkData* neighbors[6],
        const fluid::Fluid* fluid);

    /**
     * @brief 计算方块四角的流体高度
     *
     * 用于生成倾斜的水面。四个角的流体高度可能不同，
     * 这会产生流动水的视觉效果。
     *
     * @param chunk 区块数据
     * @param x 方块X坐标
     * @param y 方块Y坐标
     * @param z 方块Z坐标
     * @param neighbors 周围区块
     * @param fluid 流体类型
     * @param[out] h00 西北角高度
     * @param[out] h10 东北角高度
     * @param[out] h01 西南角高度
     * @param[out] h11 东南角高度
     */
    static void getCornerHeights(
        const ChunkData& chunk,
        i32 x, i32 y, i32 z,
        const ChunkData* neighbors[6],
        const fluid::Fluid* fluid,
        f32& h00, f32& h10, f32& h01, f32& h11);

    // ========================================================================
    // 面渲染判断
    // ========================================================================

    /**
     * @brief 检查是否应渲染流体面
     *
     * 渲染条件：
     * 1. 邻居是空气或非固体方块
     * 2. 邻居不是同类型流体
     * 3. 邻居不是完全覆盖的固体方块
     *
     * @param fluidBlock 流体方块状态
     * @param neighborBlock 邻居方块状态
     * @param face 面方向
     * @return 是否应渲染该面
     */
    [[nodiscard]] static bool shouldRenderFluidFace(
        const BlockState* fluidBlock,
        const BlockState* neighborBlock,
        Face face);

    /**
     * @brief 检查方块是否应该显示流体覆盖层
     *
     * 用于水面与透明方块（如玻璃、冰）交界处。
     *
     * @param blockState 方块状态
     * @return 是否显示流体覆盖层
     */
    [[nodiscard]] static bool shouldDisplayFluidOverlay(const BlockState* blockState);

    // ========================================================================
    // 顶点生成
    // ========================================================================

    /**
     * @brief 添加水面顶点
     *
     * 生成倾斜的水面四边形，支持不同高度的四角。
     *
     * @param mesh 输出网格
     * @param x 方块X坐标
     * @param y 方块Y坐标（基础高度）
     * @param z 方块Z坐标
     * @param h00 西北角高度偏移
     * @param h10 东北角高度偏移
     * @param h01 西南角高度偏移
     * @param h11 东南角高度偏移
     * @param skyLight 天空光照
     * @param blockLight 方块光照
     * @param texture 纹理区域
     * @param color 水颜色（RGBA）
     */
    static void addWaterSurface(
        MeshData& mesh,
        f32 x, f32 y, f32 z,
        f32 h00, f32 h10, f32 h01, f32 h11,
        u8 skyLight, u8 blockLight,
        const TextureRegion& texture,
        u32 color);

    /**
     * @brief 添加流体边界面顶点
     *
     * 生成流体侧面的四边形，用于流体与非流体方块的交界。
     *
     * @param mesh 输出网格
     * @param face 面方向
     * @param x 方块X坐标
     * @param y 方块Y坐标
     * @param z 方块Z坐标
     * @param heightTop 顶部高度
     * @param heightBottom 底部高度
     * @param skyLight 天空光照
     * @param blockLight 方块光照
     * @param texture 纹理区域
     * @param color 水颜色
     */
    static void addFluidSide(
        MeshData& mesh,
        Face face,
        f32 x, f32 y, f32 z,
        f32 heightTop, f32 heightBottom,
        u8 skyLight, u8 blockLight,
        const TextureRegion& texture,
        u32 color);

    /**
     * @brief 添加单个面的顶点（使用 BlockAppearance）
     *
     * @param mesh 输出网格
     * @param face 面方向
     * @param x 方块X坐标
     * @param y 方块Y坐标
     * @param z 方块Z坐标
     * @param skyLight 天空光照
     * @param blockLight 方块光照
     * @param appearance 方块外观
     * @param color 颜色调制
     */
    static void addFaceFromAppearance(
        MeshData& mesh,
        Face face,
        f32 x, f32 y, f32 z,
        u8 skyLight, u8 blockLight,
        const BlockAppearance* appearance,
        u32 color);

    // ========================================================================
    // 光照采样
    // ========================================================================

    /**
     * @brief 采样指定坐标的天空光照
     */
    [[nodiscard]] static u8 sampleSkyLight(
        const ChunkData& chunk,
        i32 x, i32 y, i32 z,
        const ChunkData* neighbors[6]);

    /**
     * @brief 采样指定坐标的方块光照
     */
    [[nodiscard]] static u8 sampleBlockLight(
        const ChunkData& chunk,
        i32 x, i32 y, i32 z,
        const ChunkData* neighbors[6]);

    /**
     * @brief 采样指定位置的合成光照（天空光/方块光取最大值）
     */
    [[nodiscard]] static u8 sampleCombinedLight(
        const ChunkData& chunk,
        i32 x, i32 y, i32 z,
        const ChunkData* neighbors[6]);

    // ========================================================================
    // 静态成员
    // ========================================================================

    static BlockModelCache* s_modelCache;

    /// 默认水颜色（ARGB格式）
    static constexpr u32 DEFAULT_WATER_COLOR = 0xFF3F76E4;

    /// 默认岩浆颜色（ARGB格式）
    static constexpr u32 DEFAULT_LAVA_COLOR = 0xFFFF6600;

    /// 水的默认透明度
    static constexpr f32 WATER_ALPHA = 0.6f;

    /// 岩浆的默认透明度
    static constexpr f32 LAVA_ALPHA = 1.0f;
};

} // namespace mc::client::renderer
} // namespace mc
