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

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/biome/BiomeTag.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "common/world/generation/base/DecorationStage.hpp"
#include "server/world/gen/jigsaw/JigsawJunction.hpp"
#include "server/world/gen/structure/base/StructureBoundingBox.hpp"
#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
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
class WorldGenRegion;
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
        virtual void selectBlocks(math::IRandom& rng, i32 x, i32 y, i32 z, bool isWall) = 0;

        [[nodiscard]] const BlockState* getBlockState() const
        {
            return m_blockState;
        }

    protected:
        const BlockState* m_blockState;
    };

    StructurePiece(i32 type, i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ);

    /**
     * @brief 带显式构造原点的构造函数
     *
     * 原版的片段定位统一走 `BoundingBox.orientBox(origin, …, direction)`：盒子由
     * 「原版坐标系里的原点 + 局部偏移」一次算成，之后子构件的落点也在**同一套原版坐标**
     * 里按 `盒角 ± 偏移` 推出。本项目把方向拆成了 rotation + mirror，盒子本身已换到世界
     * 坐标，镜像朝向（SOUTH/WEST）下盒角与原版坐标不再对应。
     *
     * 因此凡是需要按原版公式推算子构件落点或使用「原版坐标系里的角坐标」的片段，
     * 必须传入构造原点；`originX()/originY()/originZ()` 给出的就是原版坐标，
     * 它们随 `offset()` 一起平移。
     *
     * @param originX 原版坐标系的原点 X（即 orientBox 的第一个实参）
     * @param originY 原版坐标系的原点 Y
     * @param originZ 原版坐标系的原点 Z
     */
    StructurePiece(
        i32 type, i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ, i32 originX, i32 originY, i32 originZ);

    virtual ~StructurePiece() = default;

    [[nodiscard]] i32 type() const noexcept
    {
        return m_type;
    }

    [[nodiscard]] i32 getComponentType() const noexcept
    {
        return m_type;
    } ///< 别名

    [[nodiscard]] i32 minX() const noexcept
    {
        return m_minX;
    }

    [[nodiscard]] i32 minY() const noexcept
    {
        return m_minY;
    }

    [[nodiscard]] i32 minZ() const noexcept
    {
        return m_minZ;
    }

    /** @brief 构造原点 X（原版坐标系，参见三参版本构造函数的说明）。 */
    [[nodiscard]] i32 originX() const noexcept
    {
        return m_originX;
    }

    /** @brief 构造原点 Y（原版坐标系）。 */
    [[nodiscard]] i32 originY() const noexcept
    {
        return m_originY;
    }

    /** @brief 构造原点 Z（原版坐标系）。 */
    [[nodiscard]] i32 originZ() const noexcept
    {
        return m_originZ;
    }

    [[nodiscard]] i32 maxX() const noexcept
    {
        return m_maxX;
    }

    [[nodiscard]] i32 maxY() const noexcept
    {
        return m_maxY;
    }

    [[nodiscard]] i32 maxZ() const noexcept
    {
        return m_maxZ;
    }

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
     *
     * 虚函数：子类的额外位置状态（模板原点、子节点基准等）必须随之平移，
     * 否则移动后它们仍指向旧位置。
     */
    virtual void offset(i32 dx, i32 dy, i32 dz);

    /** 整体改写包围盒（世界坐标）。构件自身的位置状态由子类按需同步。 */
    void setBoundingBox(const StructureBoundingBox& bounds) noexcept
    {
        m_minX = bounds.minX();
        m_minY = bounds.minY();
        m_minZ = bounds.minZ();
        m_maxX = bounds.maxX();
        m_maxY = bounds.maxY();
        m_maxZ = bounds.maxZ();
    }

    /**
     * @brief 获取基础方向
     */
    [[nodiscard]] Direction getCoordBaseMode() const noexcept
    {
        return m_coordBaseMode;
    }

    /**
     * @brief 设置基础方向（自动设置镜像和旋转）
     */
    void setCoordBaseMode(Direction dir);

    /**
     * @brief 获取镜像
     */
    [[nodiscard]] Mirror getMirror() const noexcept
    {
        return m_mirror;
    }

    /**
     * @brief 设置镜像
     */
    void setMirror(Mirror mirror) noexcept
    {
        m_mirror = mirror;
    }

    /**
     * @brief 获取旋转
     */
    [[nodiscard]] Rotation getRotation() const noexcept
    {
        return m_rotation;
    }

    /**
     * @brief 设置旋转
     */
    void setRotation(Rotation rotation) noexcept
    {
        m_rotation = rotation;
    }

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
     *
     * 虚函数：矿井据此插入「不得覆盖已有木板/原木/栅栏/铁链」的 `canBeReplaced` 判定
     * （原版 `StructurePiece.placeBlock` 会先问 `canBeReplaced`）。
     */
    virtual void setBlockState(
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
        math::IRandom& rng,
        BlockSelector& selector);

    /**
     * @brief 按概率填充区域（原版 `generateMaybeBox`）
     *
     * 每个位置都先抽一次 `nextFloat`，未命中才跳过；两个 require 条件在随机之后判定，
     * 因此不管最终是否放置，随机流都与原版一致。
     *
     * @param probability 每个位置放置方块的概率
     * @param boundaryBlock 边界位置使用的方块
     * @param insideBlock 内部位置使用的方块
     * @param requireNonAir 仅在该位置已有非空气方块时放置
     * @param requireInterior 仅在该位置「上方一格仍在地表之下」时放置（原版 `isInterior`）
     */
    void maybeFillWithBlocks(IWorldWriter& world,
        const StructureBoundingBox& bounds,
        math::IRandom& rng,
        f32 probability,
        i32 minX,
        i32 minY,
        i32 minZ,
        i32 maxX,
        i32 maxY,
        i32 maxZ,
        const BlockState* boundaryBlock,
        const BlockState* insideBlock,
        bool requireNonAir,
        bool requireInterior);

    /**
     * @brief 该位置上方一格是否仍在地表之下（原版 `StructurePiece.isInterior`）
     */
    [[nodiscard]] bool isInterior(IWorld& world, i32 x, i32 y, i32 z, const StructureBoundingBox& bounds) const;

    /**
     * @brief 随机放置单个方块
     */
    void randomlyPlaceBlock(IWorldWriter& world,
        const StructureBoundingBox& bounds,
        math::IRandom& rng,
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
     * @brief 在**世界坐标**放置带战利品表的宝箱（原版 `StructurePiece#createChest`）
     *
     * 与 `generateChest` 的区别：`pos` 已经是世界坐标，不再经过构件的旋转/镜像变换。
     * 越界、该位置已有宝箱或没有可写世界时返回 false 且不消耗随机数。
     *
     * @param world 世界写入接口
     * @param bounds 结构边界框（通常为当前区块的可写区）
     * @param rng 随机数生成器（用于战利品表种子）
     * @param pos 宝箱世界坐标
     * @param lootTable 战利品表资源位置
     * @return 是否放置了宝箱
     */
    [[nodiscard]] bool createChestAt(IWorldWriter& world,
        const StructureBoundingBox& bounds,
        math::IRandom& rng,
        const BlockPos& pos,
        const ResourceLocation& lootTable);

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
        math::IRandom& rng,
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
        math::IRandom& rng,
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
        math::IRandom& rng,
        i32 x,
        i32 y,
        i32 z,
        Direction facing,
        const ResourceLocation& lootTable);

    /**
     * @brief 从给定位置向下填充方块，直到遇到不可替换的方块或世界底部（原版 `StructurePiece#fillColumnDown`）
     *
     * 只替换空气、液体、发光地衣与海草类方块；循环下界是 `getMinBuildHeight() + 1`，
     * 因此最低可写 y 是 `getMinBuildHeight() + 2`。空海洋世界里这会一路填到世界底部，
     * 海洋纪念碑的外围立柱依赖这一行为。
     *
     * @param world 世界接口
     * @param state 要放置的方块状态
     * @param x 相对 X 坐标
     * @param y 起始 Y 坐标（相对坐标）
     * @param z 相对 Z 坐标
     * @param bounds 边界框（只检查起始点是否在内，与原版一致）
     */
    void fillColumnDown(
        IWorld& world, const BlockState* state, i32 x, i32 y, i32 z, const StructureBoundingBox& bounds);

    /**
     * @brief 结构生成可替换的方块判定（原版 `StructurePiece#isReplaceableByStructures`）
     *
     * 空气、液体、发光地衣、海草与高海草都可以被结构生成的填充方块覆盖。
     */
    [[nodiscard]] static bool isReplaceableByStructures(const BlockState& state);

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
        StructurePiece* component, std::vector<std::unique_ptr<StructurePiece>>& pieces, math::IRandom& rng);

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
        math::IRandom& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) = 0;

    /**
     * @brief 处理模板中的数据标记（原版 `TemplateStructurePiece#handleDataMarker`）
     *
     * 模板中 `mode=DATA` 的结构方块不参与方块放置，由所属构件在放好模板后按
     * `metadata` 生成箱子、生物或入口方块。默认实现什么都不做。
     *
     * @param metadata 标记内容（原版结构方块的 `metadata` 字符串）
     * @param pos 标记的世界坐标
     * @param world 世界读写接口
     * @param rng 区块随机源
     * @param chunkBounds 当前区块的放置范围
     */
    virtual void handleDataMarker(const std::string& metadata,
        const BlockPos& pos,
        IWorld& world,
        math::IRandom& rng,
        const StructureBoundingBox& chunkBounds)
    {
        (void)metadata;
        (void)pos;
        (void)world;
        (void)rng;
        (void)chunkBounds;
    }

    // ========== Jigsaw 结构支持 ==========

    /**
     * @brief 获取地面高度偏移
     *
     * 用于地形平滑计算。
     *
     * @return 地面高度偏移，默认返回 0
     */
    [[nodiscard]] virtual i32 getGroundLevelDelta() const
    {
        return 0;
    }

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
    [[nodiscard]] virtual bool isJigsawPiece() const
    {
        return false;
    }

    /**
     * @brief 获取片段的模板资源路径
     *
     * Jigsaw 片段记录其来源模板（如 minecraft:village/plains/houses/plains_small_house_1），
     * 用于诊断与校验"装配出的构件是否与原版一致"——仅凭包围盒无法区分装配选错了模板
     * 还是摆放位置有偏差。非 Jigsaw 片段（或模板未加载成功）返回空字符串。
     */
    [[nodiscard]] virtual std::string_view templateLocation() const noexcept
    {
        return {};
    }

    /**
     * @brief 获取片段投影类型 — MC 1.21 PoolElementStructurePiece.getElement().getProjection()
     *
     * Jigsaw 片段可以重写此方法返回实际的投影类型。
     * 非 Jigsaw 片段默认返回 Rigid。
     * Beardifier 使用此信息决定是否将片段作为 Rigid piece 处理：
     * - Rigid: 作为 Rigid piece 添加（影响地形密度）
     * - TerrainMatching: 不作为 Rigid piece（地形自适应），只收集 JigsawJunction
     */
    [[nodiscard]] virtual StructurePieceProjection getProjection() const
    {
        return StructurePieceProjection::Rigid;
    }

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

    /**
     * @brief 构造原点（原版坐标系）
     *
     * 缺省取盒子的最小角，使未走定向展开的片段仍能给出「与原版等价」的角坐标。
     */
    i32 m_originX = 0;
    i32 m_originY = 0;
    i32 m_originZ = 0;

    /**
     * @brief 本片段在所属分支上的深度（要塞的 50 层上限）
     *
     * 起始片段为 0，每向下一层加一。组装算法按**单条分支**累计深度，而不是数全部片段总数。
     */
    i32 m_expansionDepth = 0;

    /**
     * @brief 生成深度（对应 MC `StructurePiece.genDepth`）
     *
     * 与 m_expansionDepth 语义不同：这个字段是原版用于**重叠判定**的标识。
     * 末地城每次整批提交前用一次 nextInt() 得到的值覆盖它，随后「只与 genDepth
     * 不同的已有构件比对是否相交」；桥段把它设成 -1 表示豁免所有比对。
     * 矿井的起点房间保持原版写死的 50。
     */
    i32 m_genDepth = 0;

public:
    /**
     * @brief 原版 `getOrientation()`：构件构造时记录的导入口世界朝向
     *
     * 原版用 `setOrientation(Direction)` 存一个独立字段；本项目把同样的信息编码进
     * rotation + mirror（SOUTH=LEFT_RIGHT+NONE、WEST=LEFT_RIGHT+CW90、EAST=NONE+CW90、
     * NORTH=NONE+NONE），这里按同一张表反推。房间一类构件从来没有设过朝向，结果为 None。
     */
    [[nodiscard]] Direction orientation() const noexcept
    {
        // 构造时 setCoordBaseMode 会把「没有朝向」写成 rotation/mirror 双 None，而同一组取值
        // 也对应 NORTH，仅凭旋转无法区分；以 setCoordBaseMode 记下的原始方向为准。
        return m_coordBaseMode;
    }

    [[nodiscard]] i32 expansionDepth() const noexcept
    {
        return m_expansionDepth;
    }

    void setExpansionDepth(i32 depth) noexcept
    {
        m_expansionDepth = depth;
    }

    [[nodiscard]] i32 genDepth() const noexcept
    {
        return m_genDepth;
    }

    void setGenDepth(i32 depth) noexcept
    {
        m_genDepth = depth;
    }
};

/**
 * @brief 结构实例
 *
 * 表示一个结构的起点，包含所有组成片段。
 */
class Structure;

class StructureStart {
public:
    /**
     * @brief 构造一个结构起点
     *
     * @param owner 拥有该起点的结构定义：起点包围盒的最终形状由它的地形适配模式决定
     *              （原版 `StructureStart.getBoundingBox()` 内部调 `structure.adjustBoundingBox`）
     * @param chunkX 起点所在区块 X 坐标
     * @param chunkZ 起点所在区块 Z 坐标
     */
    StructureStart(const Structure& owner, i32 chunkX, i32 chunkZ);
    StructureStart(const StructureStart&) = delete;

    // m_references 是原子量（不可拷贝/移动），故移动语义手写：搬运可变内容，计数按值转移。
    StructureStart(StructureStart&& other) noexcept
        : m_pieces(std::move(other.m_pieces))
        , m_chunkX(other.m_chunkX)
        , m_chunkZ(other.m_chunkZ)
        , m_owner(other.m_owner)
        , m_pieceBox(other.m_pieceBox)
        , m_boundingBox(other.m_boundingBox)
        , m_boundingBoxDirty(other.m_boundingBoxDirty)
    {
        m_references.store(other.m_references.load(std::memory_order::acquire), std::memory_order::release);
    }

    StructureStart& operator=(const StructureStart&) = delete;

    StructureStart& operator=(StructureStart&& other) noexcept
    {
        if (this != &other) {
            m_pieces = std::move(other.m_pieces);
            m_chunkX = other.m_chunkX;
            m_chunkZ = other.m_chunkZ;
            m_owner = other.m_owner;
            m_pieceBox = other.m_pieceBox;
            m_boundingBox = other.m_boundingBox;
            m_boundingBoxDirty = other.m_boundingBoxDirty;
            m_references.store(other.m_references.load(std::memory_order::acquire), std::memory_order::release);
        }
        return *this;
    }

    ~StructureStart() = default;

    void addPiece(std::unique_ptr<StructurePiece> piece);

    [[nodiscard]] const std::vector<std::unique_ptr<StructurePiece>>& pieces() const noexcept
    {
        return m_pieces;
    }

    [[nodiscard]] std::vector<std::unique_ptr<StructurePiece>>& pieces() noexcept
    {
        return m_pieces;
    }

    [[nodiscard]] size_t pieceCount() const noexcept
    {
        return m_pieces.size();
    }

    [[nodiscard]] bool isValid() const noexcept
    {
        return !m_pieces.empty();
    }

    [[nodiscard]] i32 chunkX() const noexcept
    {
        return m_chunkX;
    }

    [[nodiscard]] i32 chunkZ() const noexcept
    {
        return m_chunkZ;
    }

    /**
     * @brief 获取边界框（片段并集 + 地形适配外扩）
     *
     * 惰性重算并缓存，与原版 `StructureStart.getBoundingBox()` 一致：片段增删后第一次读取
     * 才会重新求并集并套用 `Structure.adjustBoundingBox`。返回引用在下一次片段变动前有效。
     */
    [[nodiscard]] const StructureBoundingBox& getBoundingBox() const noexcept;

    /**
     * @brief 片段并集包围盒（**不含**地形适配外扩）
     *
     * 对应原版 `StructurePiecesBuilder.getBoundingBox()`（= `PiecesContainer.calculateBoundingBox()`）。
     * 生成过程中需要"当前片段实际占多大"的地方（例如要塞/矿井的 `moveBelowSeaLevel`）必须用这个，
     * 而不是 `getBoundingBox()`：后者已按地形适配外扩 12 格，会把整体高度多算 24，下沉量随之偏移。
     *
     * @return 片段并集；没有片段时返回无效包围盒
     */
    [[nodiscard]] StructureBoundingBox piecesBoundingBox() const noexcept
    {
        return m_pieceBox;
    }

    /**
     * @brief 重新计算结构大小
     *
     * 根据所有片段的边界框求并集，再交给拥有者结构的 `adjustBoundingBox` 做地形适配外扩，
     * 与原版 `StructureStart.getBoundingBox()`（`structure.adjustBoundingBox(pieces.calculateBoundingBox())`）
     * 同义。外扩每次都从并集重算，因此不会被后续 `addPiece` 覆盖掉，也不需要调用方参与。
     */
    void recalculateStructureSize();

    /**
     * @brief 检查引用计数是否低于最大值
     */
    [[nodiscard]] bool isRefCountBelowMax() const noexcept;

    /**
     * @brief 增加引用计数
     *
     * 【必须原子】`StructureStart` 通过 shared_ptr 被邻居区块共享，而 STRUCTURE_REFERENCES
     * 阶段在多个工作线程上并行跑，同一份起点的计数会被不同线程各加一次。relaxed 足够：
     * 计数只用于"是否已达上限"的判定，不承担发布其它数据的作用。
     */
    void incrementRefCount() noexcept
    {
        m_references.fetch_add(1, std::memory_order::relaxed);
    }

    /**
     * @brief 获取引用计数
     */
    [[nodiscard]] i32 getRefCount() const noexcept
    {
        return m_references.load(std::memory_order::acquire);
    }

    /**
     * @brief 获取最大引用计数
     */
    [[nodiscard]] static constexpr i32 getMaxRefCount() noexcept
    {
        return 1;
    }

    /**
     * @brief 移动结构
     */
    void offset(i32 dx, i32 dy, i32 dz);

private:
    std::vector<std::unique_ptr<StructurePiece>> m_pieces;
    i32 m_chunkX;
    i32 m_chunkZ;
    /// 引用计数：被多少个区块的 STRUCTURE_REFERENCES 引用过（多线程并发递增，故为原子量）
    std::atomic<i32> m_references{0};
    const Structure* m_owner;                   ///< 拥有该起点的结构定义（决定地形适配外扩）
    StructureBoundingBox m_pieceBox;            ///< 片段并集（未外扩）
    mutable StructureBoundingBox m_boundingBox; ///< 外扩后的缓存，失效时按需重算
    mutable bool m_boundingBoxDirty = true;     ///< 缓存是否失效
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
    [[nodiscard]] const ResourceLocation& id() const noexcept
    {
        return m_id;
    }

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
    [[nodiscard]] virtual const biome::BiomeTag* defaultBiomeTag() const
    {
        return nullptr;
    }

    /**
     * @brief 注入数据驱动的生物群系标签
     *
     * 由 StructureTypeRegistry 工厂在从结构定义构造后调用，覆盖子类默认标签。
     * 传入 nullptr 清除注入，回退到 defaultBiomeTag()。
     */
    void setBiomeTag(const biome::BiomeTag* tag) noexcept
    {
        m_biomeTag = tag;
    }

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
     * @brief 判定某位置处的噪声生物群系是否在此结构的有效生物群系中
     *
     * 采样点是**候选生成点**（对应 MC 1.21 Structure.isValidBiome(GenerationStub, ...)），
     * 逐维换算四分坐标（QuartPos.fromBlock，即算术右移 2 位）后取噪声生物群系。
     * 高度参与采样，故用区块中心的 y=0 代替候选点高度会采到洞穴/深板岩群系，
     * 在群系边界处得出与原版相反的结果。
     *
     * @param generator 区块生成器（取自噪声生物群系源）
     * @param pos 候选生成点（世界坐标）
     * @return 该点的生物群系是否有效
     */
    [[nodiscard]] bool isValidBiomeAt(IChunkGenerator& generator, const BlockPos& pos) const;

    /**
     * @brief 区块中心处的候选生成点（Y 取指定高度图）
     *
     * 等价原版 `Structure.onTopOfChunkCenter`：X/Z 是区块中心定值、Y 取高度图，
     * 全程不消耗随机数。生物群系校验必须落在候选生成点上（原版
     * `findValidGenerationPoint` 就是在 Stub 位置上采样的），用区块角落或固定 Y
     * 采样会在群系边界与洞穴上方判出相反结论。
     *
     * @param generator 区块生成器
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @param type 高度图类型（原版各结构用 WORLD_SURFACE_WG 或 OCEAN_FLOOR_WG）
     * @return 候选生成点
     */
    [[nodiscard]] BlockPos onTopOfChunkCenter(
        IChunkGenerator& generator, i32 chunkX, i32 chunkZ, HeightmapType type) const;

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
    [[nodiscard]] virtual TerrainAdaptation defaultTerrainAdaptation() const
    {
        return TerrainAdaptation::None;
    }

    /**
     * @brief 把起点包围盒按其地形适配模式外扩
     *
     * 对应原版 `Structure.adjustBoundingBox`：地形适配非 NONE 时每侧外扩 12 格
     * （Beardifier 的核作用范围），NONE 时原样返回。起点包围盒必须经这里得到，
     * 否则所有带地形适配的结构（村庄、试炼密室、前哨站……）的覆盖范围都会比原版小 24 格。
     *
     * @param box 片段的并集包围盒
     * @return 外扩后的包围盒
     */
    [[nodiscard]] StructureBoundingBox adjustBoundingBox(const StructureBoundingBox& box) const;

    /**
     * @brief 注入数据驱动的地形适配模式
     */
    void setTerrainAdaptation(TerrainAdaptation adaptation) noexcept
    {
        m_terrainAdaptation = adaptation;
    }

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
    [[nodiscard]] virtual DecorationStage defaultDecorationStage() const
    {
        return DecorationStage::SurfaceStructures;
    }

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
    [[nodiscard]] virtual const SpawnOverrides* spawnOverrides() const
    {
        return nullptr;
    }

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
        IChunkGenerator& generator, math::IRandom& rng, i32 chunkX, i32 chunkZ) const;

    /**
     * @brief 生成结构起点，并可读取生成区域中已放置结构的引用
     *
     * 落点依赖「已被结构整平过的地形」的结构（如废墟传送门，原版经 RandomState 的
     * NoiseChunk 缓存看到 Beardifier 贡献）需要 region 才能得到与原版一致的高度。
     *
     * 默认实现丢弃 region 并转发到四参数版本；需要结构地形贡献的子类覆写本方法。
     *
     * @param region 生成区域（可能为 nullptr，表示无法获取结构引用）
     */
    [[nodiscard]] virtual std::unique_ptr<StructureStart> generate(
        IChunkGenerator& generator, WorldGenRegion* region, math::IRandom& rng, i32 chunkX, i32 chunkZ) const
    {
        MC_UNUSED(region);
        return generate(generator, rng, chunkX, chunkZ);
    }

    /**
     * @brief 在区块中放置结构片段
     * @param world 世界写入器
     * @param chunk 区块
     * @param start 结构起点
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @param rng 世界生成随机源（调用方已按 `setFeatureSeed` 设定本结构在本区块的种子）
     * @param generator 区块生成器（传递给含 FeatureJigsawPiece 的拼图片段用于放置配置化地物，可为 nullptr）
     */
    virtual void placeInChunk(IWorldWriter& world,
        ChunkPrimer& chunk,
        StructureStart& start,
        i32 chunkX,
        i32 chunkZ,
        math::IRandom& rng,
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
     * @brief 创建一个归属于本结构的起点
     *
     * 起点包围盒的最终形状由拥有者的地形适配模式决定（`adjustBoundingBox`），因此子类
     * 生成起点时必须经此创建，不能直接构造 `StructureStart`——少了拥有者，起点盒永远不会
     * 按地形适配外扩，STRUCTURE_REFERENCES 与区块覆盖都会偏小。
     *
     * @param chunkX 起点所在区块 X 坐标
     * @param chunkZ 起点所在区块 Z 坐标
     * @return 空的结构起点
     */
    [[nodiscard]] std::unique_ptr<StructureStart> makeStructureStart(i32 chunkX, i32 chunkZ) const;

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
