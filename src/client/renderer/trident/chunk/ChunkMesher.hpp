#pragma once

#include "../../../../common/core/Types.hpp"
#include "../../../../common/world/chunk/ChunkData.hpp"
#include "../../../../common/world/block/Block.hpp"
#include "../../MeshTypes.hpp"
#include "../../../settings/ClientSettings.hpp"
#include "../../../world/color/blend/blend.hpp"
#include <array>
#include <memory>
#include <functional>

namespace mc {

// 前向声明
class BlockModelCache;
struct BlockAppearance;

// ============================================================================
// 光照模式
// ============================================================================

/**
 * @brief 光照模式枚举
 *
 * 参考: net.minecraft.client.settings.AmbientOcclusionStatus
 */
enum class LightingMode : u8 {
    Flat = 0,     ///< 平面光照（每个面使用统一光照）
    Smooth = 1,   ///< 平滑光照（逐顶点AO）
};

// ============================================================================
// 区块网格生成器
// ============================================================================

/**
 * @brief 区块网格生成器
 *
 * 负责将 ChunkData 转换为可渲染的 MeshData。
 * 使用 BlockModelCache 获取方块外观信息。
 *
 * 支持两种光照模式:
 * - Flat: 平面光照，每个面的所有顶点使用相同的光照值
 * - Smooth: 平滑光照，使用环境光遮蔽(AO)计算逐顶点光照
 *
 * 使用示例:
 * @code
 * // 初始化时设置模型缓存
 * ChunkMesher::setModelCache(&modelCache);
 *
 * // 启用平滑光照
 * ChunkMesher::setLightingMode(LightingMode::Smooth);
 *
 * // 生成网格
 * MeshData mesh;
 * ChunkMesher::generateMesh(chunk, mesh, neighbors);
 *
 * // 清理
 * ChunkMesher::setModelCache(nullptr);
 * @endcode
 */
class ChunkMesher {
public:
    // ========================================================================
    // 网格生成
    // ========================================================================

    /**
     * @brief 生成区块网格
     *
     * @param chunk 区块数据
     * @param outMesh 输出网格
     * @param neighbors 周围6个区块 (用于边界面的剔除)
     *                  顺序: -X, +X, -Z, +Z, -Y, +Y (可以是nullptr)
     */
    static void generateMesh(
        const ChunkData& chunk,
        MeshData& outMesh,
        const ChunkData* neighbors[6] = nullptr
    );

    /**
     * @brief 生成分层区块网格（实心层 + 半透明层）
     *
     * 用于水、玻璃等需要延后混合渲染的方块。
     * 
     * @param chunk 区块数据
     * @param outSolidMesh 输出实心网格
     * @param outTransparentMesh 输出半透明网格
     * @param neighbors 周围6个区块 (用于边界面的剔除)
     */
    static void generateSplitMesh(
        const ChunkData& chunk,
        MeshData& outSolidMesh,
        MeshData& outTransparentMesh,
        const ChunkData* neighbors[6] = nullptr
    );

    /**
     * @brief 生成单个区块段的网格 (用于渐进加载)
     *
     * @param chunk 区块数据
     * @param sectionIndex 区段索引 (0-15)
     * @param outMesh 输出网格
     * @param neighborChunks 周围区块
     */
    static void generateSectionMesh(
        const ChunkData& chunk,
        i32 sectionIndex,
        MeshData& outMesh,
        const ChunkData* neighborChunks[6] = nullptr
    );

    // ========================================================================
    // 配置
    // ========================================================================

    /**
     * @brief 设置 BlockModelCache
     *
     * 必须在使用 ChunkMesher 之前调用。
     * BlockModelCache 用于获取方块的外观信息（模型、纹理）。
     *
     * @param cache 模型缓存指针（可以为 nullptr 禁用渲染）
     */
    static void setModelCache(BlockModelCache* cache);

    /**
     * @brief 获取 BlockModelCache
     */
    static BlockModelCache* modelCache() { return s_modelCache; }

    /**
     * @brief 设置是否使用贪婪网格合并
     */
    static void setGreedyMeshing(bool enabled) { s_useGreedyMeshing = enabled; }
    static bool isGreedyMeshingEnabled() { return s_useGreedyMeshing; }

    /**
     * @brief 设置光照模式
     *
     * @param mode 光照模式 (Flat: 平面, Smooth: 平滑AO)
     */
    static void setLightingMode(LightingMode mode) { s_lightingMode = mode; }
    static LightingMode lightingMode() { return s_lightingMode; }

    /**
     * @brief 设置光照计算是否启用
     */
    static void setLightingEnabled(bool enabled) { s_lightingEnabled = enabled; }
    static bool isLightingEnabled() { return s_lightingEnabled; }

    /**
     * @brief 从客户端设置同步光照模式
     *
     * 将 AmbientOcclusionMode 转换为内部 LightingMode：
     * - Off -> Flat（平面光照）
     * - Min/Max -> Smooth（平滑光照）
     *
     * @param aoMode 客户端的 AO 模式设置
     */
    static void syncFromSettings(client::AmbientOcclusionMode aoMode) {
        using client::AmbientOcclusionMode;
        if (aoMode == AmbientOcclusionMode::Off) {
            s_lightingMode = LightingMode::Flat;
        } else {
            s_lightingMode = LightingMode::Smooth;
        }
    }

    /**
     * @brief 设置生物群系颜色混合半径
     *
     * @param radius 混合半径 (0-7)，默认为 2（5x5 混合区域）
     *               0 表示禁用混合，直接使用当前生物群系颜色
     */
    static void setBiomeBlendRadius(i32 radius);

    /**
     * @brief 获取当前生物群系颜色混合半径
     */
    [[nodiscard]] static i32 biomeBlendRadius();

    /**
     * @brief 获取生物群系颜色混合器
     */
    [[nodiscard]] static client::BiomeColorBlender& biomeColorBlender() { return s_biomeColorBlender; }

    /**
     * @brief 使区块的颜色缓存失效
     *
     * 当区块卸载或生物群系变化时调用。
     *
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     */
    static void invalidateBiomeColorCache(ChunkCoord chunkX, ChunkCoord chunkZ);

    /**
     * @brief 清空所有生物群系颜色缓存
     */
    static void clearBiomeColorCache();

    /**
     * @brief 采样指定坐标的合成光照（天空光/方块光取最大值）
     *
     * 用于区块网格构建阶段的光照查询。
     * 当采样位置越过当前区块边界时，会尝试从邻居区块读取。
     *
     * @param chunk 当前区块
     * @param x 区块局部 X（可越界，用于采样邻接面）
     * @param y 世界 Y
     * @param z 区块局部 Z（可越界，用于采样邻接面）
     * @param neighborChunks 周围区块，顺序: -X, +X, -Z, +Z, -Y, +Y
     */
    [[nodiscard]] static u8 sampleCombinedLight(
        const ChunkData& chunk,
        i32 x,
        i32 y,
        i32 z,
        const ChunkData* neighborChunks[6] = nullptr
    );

private:
    // 检查方块是否应该渲染
    static bool shouldRenderBlock(const BlockState* state);

    // 检查面是否应该渲染
    static bool shouldRenderFace(
        const BlockState* block,
        const BlockState* neighbor
    );

    // 添加单个面的顶点（使用 BlockAppearance）- 平面光照版本
    static void addFaceFromAppearance(
        MeshData& mesh,
        Face face,
        f64 x, f64 y, f64 z,
        const ChunkData& chunk,
        i32 blockX,
        i32 blockY,
        i32 blockZ,
        u8 skyLight,
        u8 blockLight,
        const BlockState* block,
        const BlockAppearance* appearance,
        const ChunkData* neighborChunks[6]
    );

    // 添加单个面的顶点（使用 BlockAppearance）- 平滑光照版本
    static void addFaceFromAppearanceSmooth(
        MeshData& mesh,
        Face face,
        f64 x, f64 y, f64 z,
        const ChunkData& chunk,
        i32 blockX, i32 blockY, i32 blockZ,
        const BlockState* block,
        const BlockAppearance* appearance,
        const ChunkData* neighborChunks[6]
    );

    [[nodiscard]] static u32 resolveTintColor(
        const ChunkData& chunk,
        i32 blockX,
        i32 blockY,
        i32 blockZ,
        const BlockState* block,
        i32 tintIndex
    );

    /**
     * @brief 解析方块着色颜色（带生物群系混合）
     *
     * @param accessor 生物群系访问器
     * @param worldX 方块世界X坐标
     * @param worldY 方块世界Y坐标
     * @param worldZ 方块世界Z坐标
     * @param block 方块状态
     * @param tintIndex 着色索引
     * @return 打包的 RGBA 颜色值
     */
    [[nodiscard]] static u32 resolveTintColorBlended(
        const client::ChunkBiomeAccessor& accessor,
        i32 worldX,
        i32 worldY,
        i32 worldZ,
        const BlockState* block,
        i32 tintIndex
    );

    [[nodiscard]] static bool tryLoadColorMap(
        StringView path,
        std::array<u32, 65536>& outColorMap
    );

    static void refreshBiomeColorMaps();

    // 获取天空光照
    [[nodiscard]] static u8 sampleSkyLight(
        const ChunkData& chunk,
        i32 x,
        i32 y,
        i32 z,
        const ChunkData* neighborChunks[6]
    );

    // 获取方块光照
    [[nodiscard]] static u8 sampleBlockLight(
        const ChunkData& chunk,
        i32 x,
        i32 y,
        i32 z,
        const ChunkData* neighborChunks[6]
    );

    // 贪婪网格合并
    static void greedyMeshSection(
        const ChunkData& chunk,
        i32 sectionIndex,
        MeshData& outMesh,
        const ChunkData* neighborChunks[6]
    );

    // 简单网格生成 (逐面生成)
    static void simpleMeshSection(
        const ChunkData& chunk,
        i32 sectionIndex,
        MeshData& outMesh,
        const ChunkData* neighborChunks[6]
    );

    static BlockModelCache* s_modelCache;
    static bool s_useGreedyMeshing;
    static bool s_lightingEnabled;
    static LightingMode s_lightingMode;
    static std::array<u32, 65536> s_grassColorMap;
    static std::array<u32, 65536> s_foliageColorMap;
    static bool s_grassColorMapLoaded;
    static bool s_foliageColorMapLoaded;
    static client::BiomeColorBlender s_biomeColorBlender;
};

// ============================================================================
// 区块渲染数据
// ============================================================================

struct ChunkRenderData {
    ChunkId chunkId;
    MeshData solidMesh;       ///< 实心方块网格
    MeshData transparentMesh; ///< 透明方块网格 (水、玻璃等)

    // 渲染状态
    bool needsUpdate = true;
    bool isDirty = false;
    u32 renderVersion = 0;

    // 统计
    u32 vertexCount = 0;
    u32 indexCount = 0;

    void clear() {
        solidMesh.clear();
        transparentMesh.clear();
        vertexCount = 0;
        indexCount = 0;
    }

    void markDirty() {
        isDirty = true;
        needsUpdate = true;
    }

    void markClean() {
        isDirty = false;
        needsUpdate = false;
    }
};

// ============================================================================
// 区块网格缓存
// ============================================================================

class ChunkMeshCache {
public:
    explicit ChunkMeshCache(size_t maxCachedChunks = 256);

    // 获取或创建区块渲染数据
    [[nodiscard]] ChunkRenderData* getOrCreate(const ChunkId& id);
    [[nodiscard]] ChunkRenderData* get(const ChunkId& id);
    [[nodiscard]] const ChunkRenderData* get(const ChunkId& id) const;

    // 标记区块需要更新
    void markDirty(const ChunkId& id);

    // 移除区块
    void remove(const ChunkId& id);

    // 清除所有缓存
    void clear();

    // 获取需要更新的区块数量
    [[nodiscard]] size_t dirtyCount() const { return m_dirtyCount; }

    // 获取缓存大小
    [[nodiscard]] size_t size() const { return m_cache.size(); }

    // 遍历所有缓存项
    template<typename Func>
    void forEach(Func&& func) {
        for (auto& [id, data] : m_cache) {
            func(id, data);
        }
    }

private:
    std::unordered_map<ChunkId, ChunkRenderData> m_cache;
    size_t m_maxCachedChunks;
    size_t m_dirtyCount = 0;
};

// ============================================================================
// 区块网格构建任务
// ============================================================================

struct MeshBuildTask {
    ChunkId chunkId;
    const ChunkData* chunkData = nullptr;
    std::array<const ChunkData*, 6> neighbors = {};

    // 回调函数 (构建完成后调用)
    std::function<void(const ChunkId&, const MeshData& solid, const MeshData& transparent)> onComplete;

    MeshBuildTask() = default;
    explicit MeshBuildTask(ChunkId id) : chunkId(id) {}
};

} // namespace mc
