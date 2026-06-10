/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "StructureBoundingBox.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/gen/feature/DecorationStage.hpp"
#include "common/world/gen/jigsaw/JigsawJunction.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace mc {

// 前向声明
class IWorld;
class IWorldWriter;
namespace world::chunk {
class ChunkPrimer;
}
using world::chunk::ChunkPrimer;
class IChunkGenerator;
class BlockState;
class BlockPos;

/**
 * @brief 地形适配模式
 *
 * 控制结构周围的地形如何调整。
 * MC 1.21 Beardifier 使用此信息决定如何平滑结构周围的地形。
 */
enum class TerrainAdaptation : u8 {
    None,        ///< 无地形适配
    Bury,        ///< 埋入地下，顶部留空
    BeardThin,   ///< 薄型胡须（如村庄道路）
    BeardBox,    ///< 方形胡须
    Encapsulate, ///< 完全包裹（如试炼密室）
    BuryInterior ///< 埋入内部
};

namespace world::gen::structure {
class StructureBoundingBox;
}

namespace world::gen::structure {

// Direction.hpp 中已定义 Direction, Axis, Rotation, Mirror, Directions 等

/**
 * @brief 结构类型枚举
 */
enum class StructureType : u8 {
    Temple,          ///< 神殿/神庙结构（沙漠神殿、丛林神庙等）
    Monument,        ///< 海洋纪念碑
    Stronghold,      ///< 要塞
    Village,         ///< 村庄
    Mineshaft,       ///< 废弃矿井
    RuinedPortal,    ///< 废弃传送门
    BuriedTreasure,  ///< 埋藏宝藏
    Shipwreck,       ///< 沉船
    OceanRuin,       ///< 海洋废墟
    WoodlandMansion, ///< 林地府邸
    Bastion,         ///< 堡垒遗迹
    Fortress,        ///< 下界要塞
    EndCity,         ///< 末地城
    PillagerOutpost, ///< 掠夺者前哨站
    TrialChambers    ///< 试炼密室
};

/**
 * @brief 结构间距设置
 */
struct StructureSeparationSettings {
    i32 spacing;    ///< 平均间距（区块）
    i32 separation; ///< 最小间距（区块）
    i32 salt;       ///< 随机种子盐

    constexpr StructureSeparationSettings(i32 s = 1, i32 sep = 0, i32 st = 0)
        : spacing(s)
        , separation(sep)
        , salt(st)
    {}
};

/**
 * @brief 结构片段基类
 *
 * 提供结构片段的通用功能，包括坐标变换、方块放置等。
 */
class StructurePiece {
public:
    /**
     * @brief 方块选择器抽象基类
     *
     * 用于 fillWithRandomizedBlocks 方法中的随机方块选择。
     */
    class BlockSelector {
    public:
        BlockSelector()
            : m_blockState(nullptr)
        {}
        virtual ~BlockSelector() = default;

        /**
         * @brief 选择方块
         * @param rng 随机数生成器
         * @param x X 坐标
         * @param y Y 坐标
         * @param z Z 坐标
         * @param isWall 是否是墙壁（边界）
         */
        virtual void selectBlocks(math::Random& rng, i32 x, i32 y, i32 z, bool isWall) = 0;

        [[nodiscard]] const BlockState* getBlockState() const { return m_blockState; }

    protected:
        const BlockState* m_blockState;
    };

    StructurePiece(i32 type, i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ);
    virtual ~StructurePiece() = default;

    [[nodiscard]] i32 type() const noexcept { return m_type; }
    [[nodiscard]] i32 getComponentType() const noexcept { return m_type; } ///< 别名
    [[nodiscard]] i32 minX() const noexcept { return m_minX; }
    [[nodiscard]] i32 minY() const noexcept { return m_minY; }
    [[nodiscard]] i32 minZ() const noexcept { return m_minZ; }
    [[nodiscard]] i32 maxX() const noexcept { return m_maxX; }
    [[nodiscard]] i32 maxY() const noexcept { return m_maxY; }
    [[nodiscard]] i32 maxZ() const noexcept { return m_maxZ; }

    /**
     * @brief 获取边界框
     */
    [[nodiscard]] StructureBoundingBox getBoundingBox() const;

    /**
     * @brief 获取边界框（返回值）
     */
    [[nodiscard]] StructureBoundingBox boundingBox() const;

    /**
     * @brief 检查是否与区块相交
     */
    [[nodiscard]] bool intersectsChunk(i32 chunkX, i32 chunkZ) const;

    /**
     * @brief 检查是否与另一个边界框相交
     */
    [[nodiscard]] bool intersects(const StructureBoundingBox& box) const;

    /**
     * @brief 移动边界框
     */
    void offset(i32 dx, i32 dy, i32 dz);

    /**
     * @brief 获取基础方向
     */
    [[nodiscard]] Direction getCoordBaseMode() const noexcept { return m_coordBaseMode; }

    /**
     * @brief 设置基础方向（自动设置镜像和旋转）
     */
    void setCoordBaseMode(Direction dir);

    /**
     * @brief 获取镜像
     */
    [[nodiscard]] Mirror getMirror() const noexcept { return m_mirror; }

    /**
     * @brief 设置镜像
     */
    void setMirror(Mirror mirror) noexcept { m_mirror = mirror; }

    /**
     * @brief 获取旋转
     */
    [[nodiscard]] Rotation getRotation() const noexcept { return m_rotation; }

    /**
     * @brief 设置旋转
     */
    void setRotation(Rotation rotation) noexcept { m_rotation = rotation; }

    // ========== 坐标变换方法 ==========

    /**
     * @brief 根据 X 坐标和相对 Z 坐标计算世界 X 坐标
     */
    [[nodiscard]] i32 getXWithOffset(i32 x, i32 z) const;

    /**
     * @brief 根据 Y 坐标计算世界 Y 坐标
     */
    [[nodiscard]] i32 getYWithOffset(i32 y) const;

    /**
     * @brief 根据 X 坐标和相对 Z 坐标计算世界 Z 坐标
     */
    [[nodiscard]] i32 getZWithOffset(i32 x, i32 z) const;

    // ========== 方块放置方法 ==========

    /**
     * @brief 在指定位置放置方块（应用镜像和旋转）
     */
    void setBlockState(
        IWorldWriter& world, const BlockState* state, i32 x, i32 y, i32 z, const StructureBoundingBox& bounds);

    /**
     * @brief 从位置获取方块状态
     * @param world 世界接口
     * @param x 相对 X 坐标
     * @param y 相对 Y 坐标
     * @param z 相对 Z 坐标
     * @param bounds 边界框
     * @return 方块状态，如果超出边界或位置无效返回 nullptr
     * @note 需要子类提供世界读取能力，默认实现返回 nullptr
     */
    [[nodiscard]] const BlockState* getBlockStateFromPos(
        IWorld& world, i32 x, i32 y, i32 z, const StructureBoundingBox& bounds) const;

    /**
     * @brief 用空气填充区域
     */
    void fillWithAir(IWorldWriter& world,
        const StructureBoundingBox& bounds,
        i32 minX,
        i32 minY,
        i32 minZ,
        i32 maxX,
        i32 maxY,
        i32 maxZ);

    /**
     * @brief 用方块填充区域（边界和内部可以不同）
     */
    void fillWithBlocks(IWorldWriter& world,
        const StructureBoundingBox& bounds,
        i32 minX,
        i32 minY,
        i32 minZ,
        i32 maxX,
        i32 maxY,
        i32 maxZ,
        const BlockState* boundaryBlock,
        const BlockState* insideBlock,
        bool excludeCorners = false);

    /**
     * @brief 用随机选择的方块填充区域
     * @param world 世界写入接口
     * @param bounds 边界框
     * @param minX 最小 X 坐标
     * @param minY 最小 Y 坐标
     * @param minZ 最小 Z 坐标
     * @param maxX 最大 X 坐标
     * @param maxY 最大 Y 坐标
     * @param maxZ 最大 Z 坐标
     * @param alwaysReplace 是否总是替换（true 时只替换非空气方块，false 时无条件填充）
     * @param rng 随机数生成器
     * @param selector 方块选择器
     */
    void fillWithRandomizedBlocks(IWorldWriter& world,
        const StructureBoundingBox& bounds,
        i32 minX,
        i32 minY,
        i32 minZ,
        i32 maxX,
        i32 maxY,
        i32 maxZ,
        bool alwaysReplace,
        math::Random& rng,
        BlockSelector& selector);

    /**
     * @brief 随机放置单个方块
     */
    void randomlyPlaceBlock(IWorldWriter& world,
        const StructureBoundingBox& bounds,
        math::Random& rng,
        f32 chance,
        i32 x,
        i32 y,
        i32 z,
        const BlockState* state);

    /**
     * @brief 球形填充（用于矿井房间等）
     */
    void randomlyRareFillWithBlocks(IWorldWriter& world,
        const StructureBoundingBox& bounds,
        i32 minX,
        i32 minY,
        i32 minZ,
        i32 maxX,
        i32 maxY,
        i32 maxZ,
        const BlockState* state);

    /**
     * @brief 向下替换空气和液体
     * @param world 世界接口
     * @param state 要放置的方块状态
     * @param x 相对 X 坐标
     * @param y 起始 Y 坐标
     * @param z 相对 Z 坐标
     * @param bounds 边界框
     * @note 需要子类提供世界读取能力
     */
    void replaceAirAndLiquidDownwards(
        IWorld& world, const BlockState* state, i32 x, i32 y, i32 z, const StructureBoundingBox& bounds);

    // ========== 结构构建方法 ==========

    /**
     * @brief 构建组件（由子类覆盖以添加连接组件）
     */
    virtual void buildComponent(
        StructurePiece* component, std::vector<std::unique_ptr<StructurePiece>>& pieces, math::Random& rng);

    /**
     * @brief 在区块中生成片段
     * @param world 世界写入器
     * @param rng 随机数生成器
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @param chunkBounds 区块边界框
     */
    virtual void generate(
        IWorldWriter& world, math::Random& rng, i32 chunkX, i32 chunkZ, const StructureBoundingBox& chunkBounds) = 0;

    // ========== Jigsaw 结构支持 ==========

    /**
     * @brief 获取地面高度偏移
     *
     * 用于地形平滑计算。
     *
     * @return 地面高度偏移，默认返回 0
     */
    [[nodiscard]] virtual i32 getGroundLevelDelta() const { return 0; }

    /**
     * @brief 获取 Jigsaw 连接点列表
     *
     * 用于地形平滑计算。
     *
     * @return JigsawJunction 列表的常量引用，默认返回空列表
     */
    [[nodiscard]] virtual const std::vector<jigsaw::JigsawJunction>& getJunctions() const
    {
        static const std::vector<jigsaw::JigsawJunction> emptyJunctions;
        return emptyJunctions;
    }

    /**
     * @brief 检查是否是 Jigsaw 结构片段
     *
     * @return 是否是 Jigsaw 结构片段
     */
    [[nodiscard]] virtual bool isJigsawPiece() const { return false; }

    // ========== 静态工具方法 ==========

    /**
     * @brief 查找与边界框相交的片段
     */
    [[nodiscard]] static StructurePiece* findIntersecting(
        std::vector<std::unique_ptr<StructurePiece>>& pieces, const StructureBoundingBox& bounds);

protected:
    i32 m_type;
    i32 m_minX, m_minY, m_minZ;
    i32 m_maxX, m_maxY, m_maxZ;
    Direction m_coordBaseMode = Direction::None;
    Mirror m_mirror = Mirror::None;
    Rotation m_rotation = Rotation::None;
};

/**
 * @brief 结构实例
 *
 * 表示一个结构的起点，包含所有组成片段。
 */
class StructureStart {
public:
    StructureStart(i32 chunkX, i32 chunkZ);
    StructureStart(const StructureStart&) = delete;
    StructureStart(StructureStart&&) noexcept = default;
    StructureStart& operator=(const StructureStart&) = delete;
    StructureStart& operator=(StructureStart&&) noexcept = default;
    ~StructureStart() = default;

    void addPiece(std::unique_ptr<StructurePiece> piece);
    [[nodiscard]] const std::vector<std::unique_ptr<StructurePiece>>& pieces() const noexcept { return m_pieces; }
    [[nodiscard]] std::vector<std::unique_ptr<StructurePiece>>& pieces() noexcept { return m_pieces; }
    [[nodiscard]] size_t pieceCount() const noexcept { return m_pieces.size(); }
    [[nodiscard]] bool isValid() const noexcept { return !m_pieces.empty(); }

    [[nodiscard]] i32 chunkX() const noexcept { return m_chunkX; }
    [[nodiscard]] i32 chunkZ() const noexcept { return m_chunkZ; }

    /**
     * @brief 获取边界框
     */
    [[nodiscard]] const StructureBoundingBox& getBoundingBox() const noexcept { return m_boundingBox; }

    /**
     * @brief 重新计算结构大小
     *
     * 根据所有片段的边界框计算整体边界。
     */
    void recalculateStructureSize();

    /**
     * @brief 检查引用计数是否低于最大值
     */
    [[nodiscard]] bool isRefCountBelowMax() const noexcept;

    /**
     * @brief 增加引用计数
     */
    void incrementRefCount() noexcept { ++m_references; }

    /**
     * @brief 获取引用计数
     */
    [[nodiscard]] i32 getRefCount() const noexcept { return m_references; }

    /**
     * @brief 获取最大引用计数
     */
    [[nodiscard]] static constexpr i32 getMaxRefCount() noexcept { return 1; }

    /**
     * @brief 移动结构
     */
    void offset(i32 dx, i32 dy, i32 dz);

private:
    std::vector<std::unique_ptr<StructurePiece>> m_pieces;
    StructureBoundingBox m_boundingBox;
    i32 m_chunkX;
    i32 m_chunkZ;
    i32 m_references = 0; ///< 引用计数，用于追踪多少个区块引用此结构
};

/**
 * @brief 结构基类
 *
 * 所有世界结构的基类。
 */
class Structure {
public:
    virtual ~Structure() = default;

    [[nodiscard]] virtual const std::string& name() const = 0;
    [[nodiscard]] virtual StructureSeparationSettings separationSettings() const = 0;
    [[nodiscard]] virtual const std::vector<BiomeId>& validBiomes() const = 0;

    /**
     * @brief 是否使用均匀间距分布
     *
     * 大多数结构返回 true（均匀分布）。
     * 废弃矿井等结构返回 false，使用两次随机平均值作为偏移，
     * 产生更集中的分布。
     */
    [[nodiscard]] virtual bool useUniformSpacing() const { return true; }

    /**
     * @brief 获取结构的地形适配模式
     *
     * MC 1.21: 控制结构周围的地形如何调整。
     * 大多数结构返回 None（无适配），Jigsaw 结构可能返回 Bury/BeardThin/BeardBox/Encapsulate。
     * Beardifier 使用此信息决定如何平滑结构周围的地形。
     */
    [[nodiscard]] virtual TerrainAdaptation terrainAdaptation() const { return TerrainAdaptation::None; }

    /**
     * @brief 获取结构的装饰阶段
     *
     * MC 1.21: 对应 Structure.StructureSettings.generationStep()。
     * 用于在 applyBiomeDecoration 中按装饰阶段交错放置结构。
     *
     * 默认返回 SurfaceStructures（MC 1.21 默认值）。
     * 子类覆盖以返回不同的阶段：
     * - UndergroundStructures: 废弃矿井、埋藏宝藏、试炼密室
     * - UndergroundDecoration: 下界要塞、下界化石
     * - SurfaceStructures: 所有其他结构（村庄、神殿、末地城等）
     */
    [[nodiscard]] virtual DecorationStage decorationStage() const { return DecorationStage::SurfaceStructures; }

    [[nodiscard]] StructureType structureType() const noexcept { return m_type; }
    [[nodiscard]] bool isValidBiome(BiomeId biomeId) const;

    /**
     * @brief 检查是否可以在指定位置生成结构
     * @param world 世界引用
     * @param generator 区块生成器
     * @param rng 随机数生成器
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @return 是否可以生成
     */
    [[nodiscard]] virtual bool canGenerate(
        IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ);

    /**
     * @brief 生成结构
     * @param world 世界写入器
     * @param generator 区块生成器
     * @param rng 随机数生成器
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @return 生成的结构实例，如果无法生成则返回 nullptr
     */
    [[nodiscard]] virtual std::unique_ptr<StructureStart> generate(
        IWorldWriter& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const;

    /**
     * @brief 在区块中放置结构片段
     * @param world 世界写入器
     * @param chunk 区块
     * @param start 结构起点
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     */
    virtual void placeInChunk(
        IWorldWriter& world, ChunkPrimer& chunk, StructureStart& start, i32 chunkX, i32 chunkZ) const;

    [[nodiscard]] static bool findStructureStart(i64 seed,
        i32 chunkX,
        i32 chunkZ,
        const StructureSeparationSettings& settings,
        i32& outStartX,
        i32& outStartZ,
        bool useUniformSpacing = true);

protected:
    explicit Structure(StructureType type)
        : m_type(type)
    {}

    [[nodiscard]] static math::Random createRandom(i64 seed, i32 chunkX, i32 chunkZ, i32 salt);

    StructureType m_type;
};

// 片段类型常量
namespace StructurePieceTypes {
constexpr i32 RUINED_PORTAL = 50;
constexpr i32 BURIED_TREASURE = 53;
// Igloo
constexpr i32 IGLOO = 54;
// Swamp Hut
constexpr i32 SWAMP_HUT = 55;
// Nether Fossil
constexpr i32 NETHER_FOSSIL = 56;
// End City
constexpr i32 END_CITY = 57;
// Woodland Mansion
constexpr i32 WOODLAND_MANSION = 58;
// Bastion Remnant
constexpr i32 BASTION_REMNANT = 59;
// 废弃矿井片段
constexpr i32 MINESHAFT_ROOM = 60;
constexpr i32 MINESHAFT_CORRIDOR = 61;
constexpr i32 MINESHAFT_CROSS = 62;
constexpr i32 MINESHAFT_STAIRS = 63;
} // namespace StructurePieceTypes

} // namespace world::gen::structure
} // namespace mc
