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
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/biome/BiomeTag.hpp"
#include "common/world/gen/feature/DecorationStage.hpp"
#include "common/world/gen/jigsaw/JigsawJunction.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
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
 * Beardifier 使用此信息决定如何平滑结构周围的地形。
 */
enum class TerrainAdaptation : u8 {
    None,       ///< 无地形适配
    Bury,       ///< 埋入地下，顶部留空
    BeardThin,  ///< 薄型胡须（如村庄道路）
    BeardBox,   ///< 方形胡须
    Encapsulate ///< 完全包裹（如试炼密室）
};

/**
 * @brief 结构片段投影类型 — MC 1.21 StructureTemplatePool.Projection
 *
 * Jigsaw 片段有两种投影模式：
 * - Rigid: 刚性放置，位置固定，Beardifier 将其作为 Rigid piece 处理
 * - TerrainMatching: 地形匹配，使用 GravityProcessor 对齐地面，
 *   Beardifier 不将其作为 Rigid piece，只收集其 JigsawJunction
 */
enum class StructurePieceProjection : u8 {
    Rigid,          ///< 刚性放置（MC: "rigid"）
    TerrainMatching ///< 地形匹配（MC: "terrain_matching"）
};

namespace world::gen::structure {
class StructureBoundingBox;
}

namespace world::gen::structure {

// Direction.hpp 中已定义 Direction, Axis, Rotation, Mirror, Directions 等

/**
 * @brief 生物生成覆盖类型
 *
 * 控制结构内生物生成时使用的边界框类型。
 */
enum class SpawnOverrideType : u8 {
    Full, ///< 使用完整结构边界框
    Piece ///< 使用单个结构片段边界框
};

/**
 * @brief 生物生成覆盖条目
 *
 * 描述结构内特定类别生物的生成规则覆盖。
 */
struct SpawnOverrideEntry {
    std::string mobCategory; ///< 生物类别（如 "monster", "creature"）
    i32 minCount;            ///< 最小生成数量
    i32 maxCount;            ///< 最大生成数量
};

/**
 * @brief 结构生物生成覆盖
 *
 * 允许结构覆盖其边界框内的默认生物生成规则。
 * 例如海洋纪念碑覆盖守卫者生成。
 */
struct SpawnOverrides {
    SpawnOverrideType boundingBoxType = SpawnOverrideType::Full; ///< 边界框类型
    std::vector<SpawnOverrideEntry> entries;                     ///< 生成覆盖条目列表
};

/**
 * @brief 数据驱动的结构生物生成覆盖（MC 1.21.11 StructureSpawnOverride）
 *
 * MC 数据包中 spawn_overrides 按 MobCategory 分键，每个类别独立指定 bounding_box 与 spawns 列表。
 * 本结构对应原版 StructureSpawnOverride（单类别覆盖），由 StructureSpawnOverrideMap 按类别聚合。
 * 注意：与上方平铺的 SpawnOverrides 区别——后者是早期硬编码结构（OceanMonument 等）用的单一边界框
 * + 条目列表模型，未按类别分键；本结构为数据驱动按类别分键的规范模型。
 *
 * spawns 列表项当前仅解析 category/minCount/maxCount（与 SpawnOverrideEntry 同字段），
 * 完整 SpawnerData（entity type + weight）解析待后续接入生物生成链路时补全。
 */
struct StructureSpawnOverride {
    SpawnOverrideType boundingBoxType = SpawnOverrideType::Full; ///< 边界框类型（piece/full）
    std::vector<SpawnOverrideEntry> entries;                     ///< 该类别生成覆盖条目
};

/// 数据驱动 spawn_overrides：按生物类别（"monster"/"creature"/...）索引的覆盖表
using StructureSpawnOverrideMap = std::unordered_map<std::string, StructureSpawnOverride>;

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

    // ========== 容器方块放置方法 ==========

    /**
     * @brief 根据周围方块自动确定宝箱朝向
     *
     * 检查宝箱周围四个水平方向的方块状态，确定最佳朝向：
     * 1. 如果相邻位置有宝箱，保持默认朝向（用于双箱合并）
     * 2. 如果恰好有一个方向是不透明完整方块，宝箱朝向该方向的反方向（面向开放空间）
     * 3. 如果没有或有多于一个不透明完整方块，从默认朝向（北）开始寻找非不透明方向
     *
     * @param world 世界接口
     * @param pos 宝箱位置
     * @param defaultState 宝箱默认方块状态
     * @return 自动确定朝向后的方块状态
     */
    [[nodiscard]] static const BlockState* reorientChest(
        IWorld& world, const BlockPos& pos, const BlockState* defaultState);

    /**
     * @brief 放置带战利品表的宝箱（自动确定朝向）
     *
     * 在指定位置放置宝箱方块并设置战利品表。
     * 宝箱朝向根据周围方块自动确定（调用 reorientChest），
     * 适用于不需要显式指定朝向的结构生成场景（如要塞宝箱）。
     * 还会检查该位置是否已有宝箱，避免重复放置。
     *
     * @param world 世界写入接口
     * @param bounds 结构边界框
     * @param rng 随机数生成器（用于战利品表种子）
     * @param x 相对 X 坐标
     * @param y 相对 Y 坐标
     * @param z 相对 Z 坐标
     * @param lootTable 战利品表资源位置
     */
    void generateChest(IWorldWriter& world,
        const StructureBoundingBox& bounds,
        math::Random& rng,
        i32 x,
        i32 y,
        i32 z,
        const ResourceLocation& lootTable);

    /**
     * @brief 放置带战利品表的宝箱（指定朝向）
     *
     * 在指定位置放置宝箱方块并设置战利品表。放置后通过 IWorld 接口
     * 获取 ChestEntity 并调用 setLootTable() 设置战利品表和种子。
     *
     * @param world 世界写入接口
     * @param bounds 结构边界框
     * @param rng 随机数生成器（用于战利品表种子）
     * @param x 相对 X 坐标
     * @param y 相对 Y 坐标
     * @param z 相对 Z 坐标
     * @param facing 宝箱朝向
     * @param lootTable 战利品表资源位置（如 "minecraft:chests/jungle_temple"）
     */
    void generateChest(IWorldWriter& world,
        const StructureBoundingBox& bounds,
        math::Random& rng,
        i32 x,
        i32 y,
        i32 z,
        Direction facing,
        const ResourceLocation& lootTable);

    /**
     * @brief 放置带战利品表的发射器
     *
     * 在指定位置放置发射器方块并设置战利品表。放置后通过 IWorld 接口
     * 获取 DispenserBlockEntity 并调用 setLootTable() 设置战利品表和种子。
     *
     * @param world 世界写入接口
     * @param bounds 结构边界框
     * @param rng 随机数生成器（用于战利品表种子）
     * @param x 相对 X 坐标
     * @param y 相对 Y 坐标
     * @param z 相对 Z 坐标
     * @param facing 发射器朝向
     * @param lootTable 战利品表资源位置（如 "minecraft:chests/jungle_temple_dispenser"）
     */
    void generateDispenser(IWorldWriter& world,
        const StructureBoundingBox& bounds,
        math::Random& rng,
        i32 x,
        i32 y,
        i32 z,
        Direction facing,
        const ResourceLocation& lootTable);

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

    /**
     * @brief 放置末地传送门框架方块环
     *
     * 在要塞末地传送门房间中放置 12 个末地传送门框架方块。
     * 框架围绕中心位置形成 5×5 的环形（四边各 3 个，不含角落），
     * 凸起朝外（背离传送门中心），与 MC Java 的
     * EndPortalFrameBlock.getOrCreatePortalShape() 图案一致：
     *   ? v v v ?      v = FACING=NORTH（北边框架，z = centerZ - 2）
     *   > P P P <      > = FACING=WEST（西边框架，x = centerX - 2）
     *   > P P P <      P = 末地传送门方块（3×3 内部区域，centerX/Z ± 1）
     *   > P P P <      < = FACING=EAST（东边框架，x = centerX + 2）
     *   ? ^ ^ ^ ?      ^ = FACING=SOUTH（南边框架，z = centerZ + 2）
     *   ? = 角落，不放置方块
     *
     * 使用结构局部坐标和 bounding box 裁剪，与 StructurePiece::setBlockState()
     * 行为一致（自动应用坐标变换、镜像和旋转）。
     *
     * @param world 世界写入接口
     * @param bounds 结构边界框（用于区块裁剪）
     * @param centerX 传送门框架中心 X 坐标（局部坐标）
     * @param y 传送门框架 Y 坐标（局部坐标）
     * @param centerZ 传送门框架中心 Z 坐标（局部坐标）
     * @param eyeStates 12 个末影之眼状态数组，true 表示该框架有眼
     * @param allEyesFilled 是否所有框架都有眼（为 true 时在内部 3×3 区域放置末地传送门方块）
     */
    void placeEndPortalFrames(IWorldWriter& world,
        const StructureBoundingBox& bounds,
        i32 centerX,
        i32 y,
        i32 centerZ,
        const bool eyeStates[12],
        bool allEyesFilled);

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
     * @param chunk 区块数据（FeatureJigsawPiece 等需要放置配置化地物的片段使用，可为 nullptr）
     * @param generator 区块生成器（FeatureJigsawPiece 等需要放置配置化地物的片段使用，可为 nullptr）
     *
     * chunk 与 generator 仅对含 FeatureJigsawPiece 的拼图结构有意义（FeatureJigsawPiece::place 需要调用
     * ConfiguredFeatureBase::place(region, chunk, generator, rng, pos)）。其他结构片段可忽略这两个参数。
     * 通过 placeInChunk → generate → JigsawPlacer::placePiece → JigsawPiece::place 链路传递。
     */
    virtual void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) = 0;

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

    /**
     * @brief 获取片段投影类型 — MC 1.21 PoolElementStructurePiece.getElement().getProjection()
     *
     * Jigsaw 片段可以重写此方法返回实际的投影类型。
     * 非 Jigsaw 片段默认返回 Rigid。
     * Beardifier 使用此信息决定是否将片段作为 Rigid piece 处理：
     * - Rigid: 作为 Rigid piece 添加（影响地形密度）
     * - TerrainMatching: 不作为 Rigid piece（地形自适应），只收集 JigsawJunction
     */
    [[nodiscard]] virtual StructurePieceProjection getProjection() const { return StructurePieceProjection::Rigid; }

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
 * 所有世界结构的基类。结构通过 ResourceLocation 标识，
 * 生物群系判断使用 BiomeTag，放置逻辑由 StructurePlacement 处理。
 */
class Structure {
public:
    virtual ~Structure() = default;

    /**
     * @brief 获取结构的资源位置 ID
     *
     * 使用 MC 标准命名空间格式，如 minecraft:village_plains。
     */
    [[nodiscard]] const ResourceLocation& id() const noexcept { return m_id; }

    /**
     * @brief 获取结构的名称字符串（兼容旧接口）
     *
     * 返回 id 的完整字符串表示。
     */
    [[nodiscard]] virtual const std::string& name() const = 0;

    /**
     * @brief 获取结构关联的生物群系标签
     *
     * 返回此结构可生成的生物群系标签指针。
     * 数据驱动构造（StructureTypeRegistry 工厂）时，由 setBiomeTag() 注入来自
     * 结构定义 biomes 字段的标签；硬编码构造路径不注入，回退到 defaultBiomeTag()。
     *
     * 本方法为非虚：消费者（如 isValidBiome）经基类指针统一拿到数据驱动覆盖值或子类默认值。
     * 子类改写默认值须覆盖 defaultBiomeTag()。
     *
     * @return 数据驱动注入的标签，或子类默认标签，均未设置则 nullptr
     */
    [[nodiscard]] const biome::BiomeTag* biomeTag() const
    {
        return m_biomeTag != nullptr ? m_biomeTag : defaultBiomeTag();
    }

    /**
     * @brief 子类默认的生物群系标签（硬编码）
     *
     * 数据驱动未注入标签时使用。默认返回 nullptr。
     */
    [[nodiscard]] virtual const biome::BiomeTag* defaultBiomeTag() const { return nullptr; }

    /**
     * @brief 注入数据驱动的生物群系标签
     *
     * 由 StructureTypeRegistry 工厂在从结构定义构造后调用，覆盖子类默认标签。
     * 传入 nullptr 清除注入，回退到 defaultBiomeTag()。
     */
    void setBiomeTag(const biome::BiomeTag* tag) noexcept { m_biomeTag = tag; }

    /**
     * @brief 检查生物群系是否在此结构的有效生物群系中
     *
     * 优先使用 biomeTag() 判断，如果标签未加载则回退到线性搜索。
     *
     * @param biomeId 生物群系 ID
     * @return 是否有效
     */
    [[nodiscard]] bool isValidBiome(BiomeId biomeId) const;

    /**
     * @brief 获取结构的地形适配模式
     *
     * 控制结构周围的地形如何调整。
     * 大多数结构返回 None（无适配），Jigsaw 结构可能返回 Bury/BeardThin/BeardBox/Encapsulate。
     * Beardifier 使用此信息决定如何平滑结构周围的地形。
     *
     * 非虚包装：数据驱动注入（setTerrainAdaptation）优先，否则回退 defaultTerrainAdaptation()。
     */
    [[nodiscard]] TerrainAdaptation terrainAdaptation() const
    {
        return m_terrainAdaptation != TerrainAdaptation::None ? m_terrainAdaptation : defaultTerrainAdaptation();
    }

    /**
     * @brief 子类默认的地形适配模式（硬编码）
     */
    [[nodiscard]] virtual TerrainAdaptation defaultTerrainAdaptation() const { return TerrainAdaptation::None; }

    /**
     * @brief 注入数据驱动的地形适配模式
     */
    void setTerrainAdaptation(TerrainAdaptation adaptation) noexcept { m_terrainAdaptation = adaptation; }

    /**
     * @brief 获取结构的装饰阶段
     *
     * 对应 Structure.StructureSettings.generationStep()。
     * 用于在 applyBiomeDecoration 中按装饰阶段交错放置结构。
     *
     * 默认返回 SurfaceStructures。
     * 子类覆盖 defaultDecorationStage() 以返回不同的阶段：
     * - UndergroundStructures: 废弃矿井、埋藏宝藏、试炼密室
     * - UndergroundDecoration: 下界要塞、下界化石
     * - SurfaceStructures: 所有其他结构（村庄、神殿、末地城等）
     *
     * 非虚包装：数据驱动注入（setDecorationStage）优先，否则回退 defaultDecorationStage()。
     */
    [[nodiscard]] DecorationStage decorationStage() const
    {
        return m_decorationStageSet ? m_decorationStage : defaultDecorationStage();
    }

    /**
     * @brief 子类默认的装饰阶段（硬编码）
     */
    [[nodiscard]] virtual DecorationStage defaultDecorationStage() const { return DecorationStage::SurfaceStructures; }

    /**
     * @brief 注入数据驱动的装饰阶段
     */
    void setDecorationStage(DecorationStage stage) noexcept
    {
        m_decorationStage = stage;
        m_decorationStageSet = true;
    }

    /**
     * @brief 获取结构的生物生成覆盖
     *
     * 返回此结构边界框内的生物生成覆盖规则。
     * 例如海洋纪念碑覆盖守卫者生成。
     * 默认返回 nullptr（无覆盖）。
     */
    [[nodiscard]] virtual const SpawnOverrides* spawnOverrides() const { return nullptr; }

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
     * @brief 生成结构起点（仅创建 StructurePiece，禁止写方块）
     *
     * 此方法在 STRUCTURE_STARTS 阶段调用，只能创建 StructureStart 和
     * StructurePiece 对象。所有方块写入必须延迟到 FEATURES 阶段，
     * 由 StructurePiece::generate() 通过 placeInChunk() 执行。
     *
     * @param generator 区块生成器（用于查询高度、生物群系等只读信息）
     * @param rng 随机数生成器
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @return 生成的结构实例
     */
    [[nodiscard]] virtual std::unique_ptr<StructureStart> generate(
        IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const;

    /**
     * @brief 在区块中放置结构片段
     * @param world 世界写入器
     * @param chunk 区块
     * @param start 结构起点
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @param generator 区块生成器（传递给含 FeatureJigsawPiece 的拼图片段用于放置配置化地物，可为 nullptr）
     */
    virtual void placeInChunk(IWorldWriter& world,
        ChunkPrimer& chunk,
        StructureStart& start,
        i32 chunkX,
        i32 chunkZ,
        IChunkGenerator* generator = nullptr) const;

    /**
     * @brief 结构放置完成后的钩子
     *
     * 在所有片段都放置到区块后调用。
     * 用于海洋纪念碑等需要在放置后生成实体的结构。
     *
     * @param world 世界写入器
     * @param start 结构起点
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     */
    virtual void afterPlace(IWorldWriter& world, StructureStart& start, i32 chunkX, i32 chunkZ) const;

protected:
    /**
     * @brief 构造结构
     * @param id 结构资源位置 ID
     */
    explicit Structure(ResourceLocation id)
        : m_id(std::move(id))
    {}

    /**
     * @brief 创建结构随机数生成器
     *
     * 使用世界种子、区块坐标和盐值生成确定性的随机数序列。
     *
     * @param seed 世界种子
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @param salt 盐值
     * @return 随机数生成器
     */
    [[nodiscard]] static math::Random createRandom(i64 seed, i32 chunkX, i32 chunkZ, i32 salt);

    ResourceLocation m_id; ///< 结构资源位置 ID

    // 数据驱动注入字段（StructureTypeRegistry 工厂构造后设置）。
    // 硬编码 initialize() 路径不设置这些字段，非虚访问器回退到 defaultXxx() 子类默认值。
    const biome::BiomeTag* m_biomeTag = nullptr; ///< 数据驱动生物群系标签（覆盖 defaultBiomeTag）
    TerrainAdaptation m_terrainAdaptation =
        TerrainAdaptation::None; ///< 数据驱动地形适配（覆盖 defaultTerrainAdaptation）
    DecorationStage m_decorationStage = DecorationStage::SurfaceStructures; ///< 数据驱动装饰阶段
    bool m_decorationStageSet = false;                                      ///< m_decorationStage 是否被显式注入
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
// 沙漠神殿
constexpr i32 DESERT_PYRAMID = 64;
// 丛林神庙
constexpr i32 JUNGLE_TEMPLE = 65;
// 下界要塞（回退方案）
constexpr i32 FORTRESS_FALLBACK = 66;
} // namespace StructurePieceTypes

} // namespace world::gen::structure
} // namespace mc
