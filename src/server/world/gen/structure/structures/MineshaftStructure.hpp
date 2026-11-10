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

#include <array>

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/biome/BiomeTag.hpp"
#include "common/world/generation/base/DecorationStage.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include <memory>
#include <string>
#include <vector>

namespace mc::world::gen::structure {

class MineshaftPiece;

/**
 * @brief 废弃矿井类型
 */
enum class MineshaftType : u8 {
    Normal, ///< 普通废弃矿井
    Mesa    ///< 恶地废弃矿井（恶地生物群系）
};

/**
 * @brief 废弃矿井配置
 */
struct MineshaftConfig {
    f32 probability = 0.004f; ///< 生成概率
    MineshaftType type = MineshaftType::Normal;
};

// ============================================================================
// 辅助函数声明
// ============================================================================

/**
 * @brief 随机创建矿井片段
 */
[[nodiscard]] std::unique_ptr<MineshaftPiece> createMineshaftPiece(std::vector<std::unique_ptr<MineshaftPiece>>& pieces,
    math::IRandom& rng,
    i32 x,
    i32 y,
    i32 z,
    i32 direction,
    i32 depth,
    MineshaftType type);

/**
 * @brief 生成并添加矿井片段
 *
 * @param parent 发起这次开口的构件（原版 `generateAndAddPiece` 的 p_227707_）
 * @param origin 起始房间，距离限制以它的包围盒为基准；为 nullptr 时不做距离限制
 * @return 新建的构件指针（未创建时为空），对应原版 `generateAndAddPiece` 的返回值；
 *         交叉点要靠它记录四条边的开口盒
 */
[[nodiscard]] MineshaftPiece* addMineshaftPiece(MineshaftPiece* parent,
    const MineshaftPiece* origin,
    std::vector<std::unique_ptr<MineshaftPiece>>& pieces,
    math::IRandom& rng,
    i32 x,
    i32 y,
    i32 z,
    i32 direction,
    i32 depth,
    MineshaftType type);

// ============================================================================
// 废弃矿井片段基类
// ============================================================================

/**
 * @brief 废弃矿井片段基类
 */
class MineshaftPiece : public StructurePiece {
public:
    using StructurePiece::buildComponent;

    MineshaftPiece(i32 type, i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ, MineshaftType mineshaftType);

    [[nodiscard]] MineshaftType mineshaftType() const
    {
        return m_mineshaftType;
    }

    // 生成深度（genDepth / setGenDepth）来自 StructurePiece 基类：
    // 原版 StructurePiece.genDepth 是全局字段，矿井与末地城共用同一份语义。
    // 【与构件类型 ID 是两回事】起点房间的原版初值是写死的 50，每展开一层 +1；
    // addMineshaftPiece 用 `depth > 8` 限制展开层数。此前实现误用 type()（构件类型 ID，
    // ROOM=60/CORRIDOR=61/...）作为深度，导致首次展开即 61 > 8 失败，矿井永远只剩起点房间。

    /**
     * @brief 构建连接片段
     * @param pieces 已有片段列表
     * @param rng 随机数生成器
     * @param maxDepth 最大深度
     */
    /**
     * @brief 朝向：0=北 1=南 2=西 3=东；房间没有朝向，返回 -1
     *
     * 走廊/交叉点/楼梯都按朝向决定从哪条边开口。此前三个子类各自持有一份
     * m_direction 与同名的 direction()，重复且容易漂移，统一上提到基类。
     */
    [[nodiscard]] i32 direction() const noexcept
    {
        return m_direction;
    }

    virtual void buildComponent(
        std::vector<std::unique_ptr<MineshaftPiece>>& pieces, math::IRandom& rng, i32 maxDepth) = 0;

    /// 放置方块前先判 `canBeReplaced`：木板/原木/栅栏/铁链不会被后续填充覆盖
    void setBlockState(
        IWorldWriter& world, const BlockState* state, i32 x, i32 y, i32 z, const StructureBoundingBox& bounds) override;

    /// 木板状态（普通矿井橡木、恶地深色橡木）
    [[nodiscard]] const BlockState* planksState() const noexcept;
    /// 原木状态
    [[nodiscard]] const BlockState* woodState() const noexcept;
    /// 栅栏状态
    [[nodiscard]] const BlockState* fenceState() const noexcept;

protected:
    /**
     * @brief 检查位置是否可以放置片段
     */
    [[nodiscard]] static bool _canPlaceAt(i32 x, i32 y, i32 z);

    // 下列 helper 与原版 StructurePiece / MineShaftPiece 的对应方法一一对应，
    // 局部坐标 -> 世界坐标的换算统一走 getXWithOffset/getYWithOffset/getZWithOffset。

    /// 读取方块（越界或读不到时按空气处理，返回非空）
    [[nodiscard]] const BlockState* _getBlock(
        IWorld& world, i32 x, i32 y, i32 z, const StructureBoundingBox& bounds) const;

    /// 位置上方一格是否在地表高度图之下（原版 `isInterior`）
    [[nodiscard]] bool _isInterior(IWorld& world, i32 x, i32 y, i32 z, const StructureBoundingBox& bounds) const;

    /// 支撑判定：x∈[x1,x2] 的一排、上方一格全部非空气（原版 `isSupportingBox`）
    [[nodiscard]] bool _isSupportingBox(
        IWorld& world, const StructureBoundingBox& bounds, i32 x1, i32 x2, i32 y, i32 z) const;

    /// 结构盒范围内是否浸液或处于禁忌生物群系（原版 `isInInvalidLocation`）
    [[nodiscard]] bool _isInInvalidLocation(IWorld& world, const StructureBoundingBox& bounds) const;

    /// 该位置是否可以被矿井覆盖（木板/原木/栅栏/铁链之外都可）
    [[nodiscard]] bool _canBeReplaced(IWorld& world, i32 x, i32 y, i32 z, const StructureBoundingBox& bounds) const;

    /// 按概率填充盒体（原版 `generateMaybeBox`）
    void _generateMaybeBox(IWorldWriter& world,
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

    /// 按概率放置单个方块（原版 `maybeGenerateBlock`）
    void _maybeGenerateBlock(IWorldWriter& world,
        const StructureBoundingBox& bounds,
        math::IRandom& rng,
        f32 probability,
        i32 x,
        i32 y,
        i32 z,
        const BlockState* state);

    /// 上部半球填充（原版 `generateUpperHalfSphere`）
    void _generateUpperHalfSphere(IWorldWriter& world,
        const StructureBoundingBox& bounds,
        i32 minX,
        i32 minY,
        i32 minZ,
        i32 maxX,
        i32 maxY,
        i32 maxZ,
        const BlockState* state,
        bool requireNonAir);

    /// 地面木板：仅当位置是内部且下方不是满格支撑面时铺板（原版 `setPlanksBlock`）
    void _setPlanksBlock(
        IWorldWriter& world, const StructureBoundingBox& bounds, const BlockState& planks, i32 x, i32 y, i32 z);

    /// 向下填充整列（原版 `fillColumnDown`）
    void _fillColumnDown(
        IWorldWriter& world, const BlockState& state, i32 x, i32 y, i32 z, const StructureBoundingBox& bounds);

    /// 向下找支撑柱 / 向上挂铁链（原版 `fillPillarDownOrChainUp`）
    void _fillPillarDownOrChainUp(
        IWorldWriter& world, const BlockState& state, i32 x, i32 y, i32 z, const StructureBoundingBox& bounds);

    /// 走廊每段的木柱/栅栏支撑（原版 `placeSupport`）
    void _placeSupport(IWorldWriter& world,
        const StructureBoundingBox& bounds,
        i32 x1,
        i32 y,
        i32 z,
        i32 x2,
        i32 topY,
        math::IRandom& rng);

    /// 按概率放蜘蛛网（原版 `maybePlaceCobWeb`）
    void _maybePlaceCobWeb(IWorldWriter& world,
        const StructureBoundingBox& bounds,
        math::IRandom& rng,
        f32 probability,
        i32 x,
        i32 y,
        i32 z);

private:
    /// 原版 `canPlaceColumnOnTopOf`：该方块上面能站柱
    [[nodiscard]] bool _canPlaceColumnOnTopOf(IWorld& world, const BlockPos& pos) const;

    /// 原版 `canHangChainBelow`：下方能挂铁链
    [[nodiscard]] bool _canHangChainBelow(IWorld& world, const BlockPos& pos, const BlockState& state) const;

    /// 在 [fromY, toY) 之间连续填同一方块（原版 `fillColumnBetween`）
    void _fillColumnBetween(IWorldWriter& world, const BlockState& state, i32 x, i32 fromY, i32 toY, i32 z) const;

    /// 原版 `hasSturdyNeighbours`：周围至少 count 个方向有坚固面
    [[nodiscard]] bool _hasSturdyNeighbours(
        IWorld& world, const StructureBoundingBox& bounds, i32 x, i32 y, i32 z, i32 count) const;

protected:
    MineshaftType m_mineshaftType;

    /// 朝向：0=北 1=南 2=西 3=东；房间为 -1
    i32 m_direction = -1;
};

// ============================================================================
// 废弃矿井房间
// ============================================================================

/**
 * @brief 废弃矿井房间
 *
 * 废弃矿井的中央起点房间。
 */
class MineshaftRoom : public MineshaftPiece {
public:
    MineshaftRoom(i32 componentType, math::IRandom& rng, i32 x, i32 y, i32 z, MineshaftType type);

    /** 跨度已抽好的构造：三个 nextInt(6) 由调用方按 X->Y->Z 顺序取出。 */
    MineshaftRoom(
        i32 componentType, math::IRandom& rng, i32 x, i32 y, i32 z, MineshaftType type, std::array<i32, 3> spans);

    void generate(IWorldWriter& world,
        math::IRandom& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

    void buildComponent(
        std::vector<std::unique_ptr<MineshaftPiece>>& pieces, math::IRandom& rng, i32 maxDepth) override;

    /// 开口盒是房间的额外位置状态，必须随整体平移一起移动（原版 `move`）
    void offset(i32 dx, i32 dy, i32 dz) override;

private:
    /// 四条边开口盒（原版 `childEntranceBoxes`）：Room 生成时按它把每条支路的入口挖通
    std::vector<StructureBoundingBox> m_childEntranceBoxes;
};

// ============================================================================
// 废弃矿井走廊
// ============================================================================

/**
 * @brief 废弃矿井走廊
 *
 * 水平的矿道走廊，带有支撑柱和铁轨。
 */
class MineshaftCorridor : public MineshaftPiece {
public:
    MineshaftCorridor(i32 componentType,
        math::IRandom& rng,
        i32 minX,
        i32 minY,
        i32 minZ,
        i32 maxX,
        i32 maxY,
        i32 maxZ,
        i32 direction,
        MineshaftType type);

    void generate(IWorldWriter& world,
        math::IRandom& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

    void buildComponent(
        std::vector<std::unique_ptr<MineshaftPiece>>& pieces, math::IRandom& rng, i32 maxDepth) override;

private:
    /// 走廊两端的木柱/铁链：位置是木板才继续（原版 `placeDoubleLowerOrUpperSupport`）
    void _placeDoubleLowerOrUpperSupport(
        IWorldWriter& world, IWorld& reader, const StructureBoundingBox& chunkBounds, i32 x, i32 y, i32 z);

    /// 1% 概率的宝箱矿车（原版覆盖的 `createChest`）
    void _createChest(IWorldWriter& world,
        IWorld& reader,
        math::IRandom& rng,
        i32 x,
        i32 y,
        i32 z,
        const StructureBoundingBox& chunkBounds);

    bool m_hasRails;
    bool m_hasSpiders;
    bool m_spawnerPlaced = false;
    i32 m_sectionCount;
};

// ============================================================================
// 废弃矿井交叉点
// ============================================================================

/**
 * @brief 废弃矿井交叉点
 *
 * 两条走廊的交叉点。
 */
class MineshaftCross : public MineshaftPiece {
public:
    MineshaftCross(i32 componentType,
        i32 minX,
        i32 minY,
        i32 minZ,
        i32 maxX,
        i32 maxY,
        i32 maxZ,
        i32 direction,
        MineshaftType type);

    void generate(IWorldWriter& world,
        math::IRandom& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

    void buildComponent(
        std::vector<std::unique_ptr<MineshaftPiece>>& pieces, math::IRandom& rng, i32 maxDepth) override;

private:
    /// 包围盒高度超过 3 时是双层交叉点（原版 `isTwoFloored`）
    bool m_isTwoFloored = false;
};

// ============================================================================
// 废弃矿井楼梯
// ============================================================================

/**
 * @brief 废弃矿井楼梯
 *
 * 连接不同高度层的楼梯。
 */
class MineshaftStairs : public MineshaftPiece {
public:
    MineshaftStairs(i32 componentType,
        i32 minX,
        i32 minY,
        i32 minZ,
        i32 maxX,
        i32 maxY,
        i32 maxZ,
        i32 direction,
        MineshaftType type);

    void generate(IWorldWriter& world,
        math::IRandom& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

    void buildComponent(
        std::vector<std::unique_ptr<MineshaftPiece>>& pieces, math::IRandom& rng, i32 maxDepth) override;

private:
};

// ============================================================================
// 废弃矿井结构
// ============================================================================

/**
 * @brief 废弃矿井结构
 *
 * 地下生成的复杂矿井结构，包含走廊、交叉点和楼梯。
 */
class MineshaftStructure : public Structure {
public:
    explicit MineshaftStructure(ResourceLocation id, MineshaftType type = MineshaftType::Normal);

    [[nodiscard]] const std::string& name() const override
    {
        return m_name;
    }

    [[nodiscard]] DecorationStage defaultDecorationStage() const override
    {
        return DecorationStage::UndergroundStructures;
    }

    /**
     * @brief 获取结构关联的生物群系标签
     *
     * 返回 minecraft:has_structure/mineshaft 标签，用于 O(1) 生物群系查找。
     */
    [[nodiscard]] const biome::BiomeTag* defaultBiomeTag() const override;

    /**
     * @brief 生成废弃矿井
     */
    [[nodiscard]] std::unique_ptr<StructureStart> generate(
        IChunkGenerator& generator, math::IRandom& rng, i32 chunkX, i32 chunkZ) const override;

private:
    static const std::string m_name;
    static const std::vector<BiomeId> m_mesaBiomes;

    MineshaftConfig m_config;
};

// 片段类型常量
namespace MineshaftPieceTypes {
constexpr i32 ROOM = 60;
constexpr i32 CORRIDOR = 61;
constexpr i32 CROSS = 62;
constexpr i32 STAIRS = 63;
} // namespace MineshaftPieceTypes

} // namespace mc::world::gen::structure
