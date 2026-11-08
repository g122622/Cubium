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
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include "server/world/gen/structure/base/StructureBoundingBox.hpp"
#include <memory>
#include <vector>

namespace mc::world::gen::structure::nether_fortress {

// ============================================================================
// 常量与类型
// ============================================================================

/// 片段类型编号。200 起独立成段：100-199 已被要塞（StrongholdPieces）占用，
/// 差分探针按类型编号识别要塞片段，共用编号会把下界要塞片段误标成要塞构件。
enum PieceTypeId : i32 {
    PIECE_BRIDGE_CROSSING = 200,
    PIECE_BRIDGE_END_FILLER = 201,
    PIECE_BRIDGE_STRAIGHT = 202,
    PIECE_CASTLE_CORRIDOR_STAIRS = 203,
    PIECE_CASTLE_CORRIDOR_T_BALCONY = 204,
    PIECE_CASTLE_ENTRANCE = 205,
    PIECE_CASTLE_SMALL_CORRIDOR = 206,
    PIECE_CASTLE_SMALL_CORRIDOR_CROSSING = 207,
    PIECE_CASTLE_SMALL_CORRIDOR_LEFT_TURN = 208,
    PIECE_CASTLE_SMALL_CORRIDOR_RIGHT_TURN = 209,
    PIECE_CASTLE_STALK_ROOM = 210,
    PIECE_MONSTER_THRONE = 211,
    PIECE_ROOM_CROSSING = 212,
    PIECE_STAIRS_ROOM = 213,
    PIECE_START = 214,
};

/// 分支展开的最大深度（原版 `MAX_DEPTH`）。
inline constexpr i32 MAX_DEPTH = 30;

/// 生成点允许的最低 Y（原版 `LOWEST_Y_POSITION`，`isOkBox` 的门槛）。
inline constexpr i32 LOWEST_Y_POSITION = 10;

/// 起始片段的魔术高度（原版 `MAGIC_START_Y`）。
inline constexpr i32 MAGIC_START_Y = 64;

/// 水平分支的最远半径（原版 `generateAndAddPiece` 的 112 格限制）。
inline constexpr i32 MAX_HORIZONTAL_RANGE = 112;

/// 整座要塞落位的高度区间（原版 `moveInsideHeights(random, 48, 70)`）。
inline constexpr i32 HEIGHT_RANGE_MIN = 48;
inline constexpr i32 HEIGHT_RANGE_MAX = 70;

/// 片段种类：权重表以种类而不是类型编号区分，避免手写编号表重复。
enum class PieceKind : u8 {
    BridgeStraight,
    BridgeCrossing,
    RoomCrossing,
    StairsRoom,
    MonsterThrone,
    CastleEntrance,
    CastleSmallCorridor,
    CastleSmallCorridorCrossing,
    CastleSmallCorridorRightTurn,
    CastleSmallCorridorLeftTurn,
    CastleCorridorStairs,
    CastleCorridorTBalcony,
    CastleStalkRoom,
    BridgeEndFiller,
};

class StartPiece;
class NetherBridgePiece;

// ============================================================================
// 权重与装配上下文
// ============================================================================

/**
 * @brief 一个片段种类的抽签权重（原版 `NetherFortressPieces.PieceWeight`）
 *
 * `placeCount` 在每次结构生成开始时归零；`maxPlaceCount == 0` 表示不限次数。
 * `allowInRow` 为 true 时允许连续两次抽到同一种片段。
 */
struct PieceWeight {
    PieceKind kind;
    i32 weight;
    i32 placeCount = 0;
    i32 maxPlaceCount;
    bool allowInRow;

    [[nodiscard]] bool doPlace() const
    {
        return maxPlaceCount == 0 || placeCount < maxPlaceCount;
    }

    [[nodiscard]] bool isValid() const
    {
        return maxPlaceCount == 0 || placeCount < maxPlaceCount;
    }
};

/**
 * @brief 装配期访问器：主构件表 + 待展开队列（原版 `StructurePieceAccessor` + `pendingChildren`）
 *
 * 主构件表拥有所有权；待展开队列只存裸指针，元素被弹出后仍由主构件表持有。
 */
struct PieceAccessor {
    std::vector<std::unique_ptr<StructurePiece>>& pieces;
    std::vector<StructurePiece*>& pendingChildren;
};

/**
 * @brief 按种类创建片段，并做包围盒与碰撞校验
 *
 * 与原版 `findAndCreateBridgePieceFactory` 同语义：先检查包围盒高度门槛（>10）与
 * 已有构件的碰撞，两者都通过才真正构造片段（构造函数可能消耗随机数）。
 *
 * @return 新片段；包围盒越界、碰撞或构造条件不满足时返回 nullptr
 */
[[nodiscard]] std::unique_ptr<NetherBridgePiece> createBridgePiece(PieceKind kind,
    PieceAccessor& accessor,
    math::IRandom& rng,
    i32 x,
    i32 y,
    i32 z,
    Direction direction,
    i32 genDepth);

// ============================================================================
// 片段基类
// ============================================================================

/**
 * @brief 下界要塞片段基类（原版 `NetherFortressPieces.NetherBridgePiece`）
 *
 * 提供三个方向的子片段展开函数与权重抽签。三个展开函数都以「世界坐标盒子角」为基准
 * （原版 `boundingBox` 由 `orientBox` 展开，已是世界坐标），与朝向无关；朝向前进方向
 * 决定往哪条边接。
 */
class NetherBridgePiece : public StructurePiece {
public:
    NetherBridgePiece(i32 type, i32 genDepth, const StructureBoundingBox& box, Direction orientation);

    /**
     * @brief 展开本片段的子分支
     *
     * @param start 整座要塞的起始片段（权重表、上一个片段、待展开队列都挂在它上面）
     * @param accessor 主构件表与待展开队列
     * @param rng 随机源（与结构生成共用，消耗顺序必须与原版一致）
     */
    virtual void addChildren(StartPiece& start, PieceAccessor& accessor, math::IRandom& rng) = 0;

    /// 原版 `isOkBox`：盒子最低点必须高于 LOWEST_Y_POSITION。
    [[nodiscard]] static bool isOkBox(const StructureBoundingBox& box)
    {
        return box.minY() > LOWEST_Y_POSITION;
    }

protected:
    void _generateChildForward(
        StartPiece& start, PieceAccessor& accessor, math::IRandom& rng, i32 offsetX, i32 offsetY, bool isCastle);
    void _generateChildLeft(
        StartPiece& start, PieceAccessor& accessor, math::IRandom& rng, i32 offsetY, i32 offsetZ, bool isCastle);
    void _generateChildRight(
        StartPiece& start, PieceAccessor& accessor, math::IRandom& rng, i32 offsetY, i32 offsetZ, bool isCastle);

    /**
     * @brief 向下用指定方块填补空气/液体（原版 `fillColumnDown`）
     *
     * `IWorldWriter` 不提供读取，写入器在生成期实际是 `IWorld`；转换失败说明调用方
     * 传错了写入器，属编程错误，直接断言。
     */
    void _fillColumnDown(
        IWorldWriter& world, const BlockState* state, i32 x, i32 y, i32 z, const StructureBoundingBox& bounds);

private:
    [[nodiscard]] i32 _updatePieceWeight(const std::vector<std::unique_ptr<PieceWeight>>& weights) const;

    [[nodiscard]] std::unique_ptr<NetherBridgePiece> _generatePiece(StartPiece& start,
        std::vector<std::unique_ptr<PieceWeight>>& weights,
        PieceAccessor& accessor,
        math::IRandom& rng,
        i32 x,
        i32 y,
        i32 z,
        Direction direction,
        i32 depth);

    void _generateAndAddPiece(StartPiece& start,
        PieceAccessor& accessor,
        math::IRandom& rng,
        i32 x,
        i32 y,
        i32 z,
        Direction direction,
        i32 depth,
        bool isCastle);
};

// ============================================================================
// 桥（Bridge*）
// ============================================================================

class BridgeCrossing : public NetherBridgePiece {
public:
    BridgeCrossing(i32 genDepth, const StructureBoundingBox& box, Direction orientation);

    /// 起始片段专用：按原点 + 朝向展开 19×10×19 的盒子。
    BridgeCrossing(i32 x, i32 z, Direction orientation);

protected:
    /// 起始片段专用：类型编号换成 PIECE_START，盒子仍按 19×10×19 展开。
    BridgeCrossing(i32 type, i32 x, i32 z, Direction orientation);

public:
    void addChildren(StartPiece& start, PieceAccessor& accessor, math::IRandom& rng) override;

    void generate(IWorldWriter& world,
        math::IRandom& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk,
        IChunkGenerator* generator) override;

    [[nodiscard]] static std::unique_ptr<BridgeCrossing> createPiece(
        PieceAccessor& accessor, i32 x, i32 y, i32 z, Direction direction, i32 genDepth);
};

class BridgeStraight : public NetherBridgePiece {
public:
    BridgeStraight(i32 genDepth, const StructureBoundingBox& box, Direction orientation);

    void addChildren(StartPiece& start, PieceAccessor& accessor, math::IRandom& rng) override;

    void generate(IWorldWriter& world,
        math::IRandom& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk,
        IChunkGenerator* generator) override;

    [[nodiscard]] static std::unique_ptr<BridgeStraight> createPiece(
        PieceAccessor& accessor, i32 x, i32 y, i32 z, Direction direction, i32 genDepth);
};

class BridgeEndFiller : public NetherBridgePiece {
public:
    BridgeEndFiller(i32 genDepth, math::IRandom& rng, const StructureBoundingBox& box, Direction orientation);

    void addChildren(StartPiece& start, PieceAccessor& accessor, math::IRandom& rng) override;

    void generate(IWorldWriter& world,
        math::IRandom& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk,
        IChunkGenerator* generator) override;

    [[nodiscard]] static std::unique_ptr<BridgeEndFiller> createPiece(
        PieceAccessor& accessor, math::IRandom& rng, i32 x, i32 y, i32 z, Direction direction, i32 genDepth);

private:
    /// 本片段自己的随机种子（原版 `selfSeed`）：生成时用独立随机流开凿封口。
    i32 m_selfSeed;
};

// ============================================================================
// 房间（桥上的节点）
// ============================================================================

class RoomCrossing : public NetherBridgePiece {
public:
    RoomCrossing(i32 genDepth, const StructureBoundingBox& box, Direction orientation);

    void addChildren(StartPiece& start, PieceAccessor& accessor, math::IRandom& rng) override;

    void generate(IWorldWriter& world,
        math::IRandom& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk,
        IChunkGenerator* generator) override;

    [[nodiscard]] static std::unique_ptr<RoomCrossing> createPiece(
        PieceAccessor& accessor, i32 x, i32 y, i32 z, Direction direction, i32 genDepth);
};

class StairsRoom : public NetherBridgePiece {
public:
    StairsRoom(i32 genDepth, const StructureBoundingBox& box, Direction orientation);

    void addChildren(StartPiece& start, PieceAccessor& accessor, math::IRandom& rng) override;

    void generate(IWorldWriter& world,
        math::IRandom& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk,
        IChunkGenerator* generator) override;

    [[nodiscard]] static std::unique_ptr<StairsRoom> createPiece(
        PieceAccessor& accessor, i32 x, i32 y, i32 z, i32 genDepth, Direction direction);
};

class MonsterThrone : public NetherBridgePiece {
public:
    MonsterThrone(i32 genDepth, const StructureBoundingBox& box, Direction orientation);

    void addChildren(StartPiece& start, PieceAccessor& accessor, math::IRandom& rng) override;

    void generate(IWorldWriter& world,
        math::IRandom& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk,
        IChunkGenerator* generator) override;

    [[nodiscard]] static std::unique_ptr<MonsterThrone> createPiece(
        PieceAccessor& accessor, i32 x, i32 y, i32 z, i32 genDepth, Direction direction);

private:
    bool m_hasPlacedSpawner = false;
};

// ============================================================================
// 城堡（Castle*）
// ============================================================================

class CastleEntrance : public NetherBridgePiece {
public:
    CastleEntrance(i32 genDepth, const StructureBoundingBox& box, Direction orientation);

    void addChildren(StartPiece& start, PieceAccessor& accessor, math::IRandom& rng) override;

    void generate(IWorldWriter& world,
        math::IRandom& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk,
        IChunkGenerator* generator) override;

    [[nodiscard]] static std::unique_ptr<CastleEntrance> createPiece(
        PieceAccessor& accessor, math::IRandom& rng, i32 x, i32 y, i32 z, Direction direction, i32 genDepth);
};

class CastleSmallCorridorPiece : public NetherBridgePiece {
public:
    CastleSmallCorridorPiece(i32 genDepth, const StructureBoundingBox& box, Direction orientation);

    void addChildren(StartPiece& start, PieceAccessor& accessor, math::IRandom& rng) override;

    void generate(IWorldWriter& world,
        math::IRandom& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk,
        IChunkGenerator* generator) override;

    [[nodiscard]] static std::unique_ptr<CastleSmallCorridorPiece> createPiece(
        PieceAccessor& accessor, i32 x, i32 y, i32 z, Direction direction, i32 genDepth);
};

class CastleSmallCorridorCrossingPiece : public NetherBridgePiece {
public:
    CastleSmallCorridorCrossingPiece(i32 genDepth, const StructureBoundingBox& box, Direction orientation);

    void addChildren(StartPiece& start, PieceAccessor& accessor, math::IRandom& rng) override;

    void generate(IWorldWriter& world,
        math::IRandom& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk,
        IChunkGenerator* generator) override;

    [[nodiscard]] static std::unique_ptr<CastleSmallCorridorCrossingPiece> createPiece(
        PieceAccessor& accessor, i32 x, i32 y, i32 z, Direction direction, i32 genDepth);
};

class CastleSmallCorridorLeftTurnPiece : public NetherBridgePiece {
public:
    CastleSmallCorridorLeftTurnPiece(
        i32 genDepth, math::IRandom& rng, const StructureBoundingBox& box, Direction orientation);

    void addChildren(StartPiece& start, PieceAccessor& accessor, math::IRandom& rng) override;

    void generate(IWorldWriter& world,
        math::IRandom& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk,
        IChunkGenerator* generator) override;

    [[nodiscard]] static std::unique_ptr<CastleSmallCorridorLeftTurnPiece> createPiece(
        PieceAccessor& accessor, math::IRandom& rng, i32 x, i32 y, i32 z, Direction direction, i32 genDepth);

private:
    bool m_isNeedingChest;
};

class CastleSmallCorridorRightTurnPiece : public NetherBridgePiece {
public:
    CastleSmallCorridorRightTurnPiece(
        i32 genDepth, math::IRandom& rng, const StructureBoundingBox& box, Direction orientation);

    void addChildren(StartPiece& start, PieceAccessor& accessor, math::IRandom& rng) override;

    void generate(IWorldWriter& world,
        math::IRandom& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk,
        IChunkGenerator* generator) override;

    [[nodiscard]] static std::unique_ptr<CastleSmallCorridorRightTurnPiece> createPiece(
        PieceAccessor& accessor, math::IRandom& rng, i32 x, i32 y, i32 z, Direction direction, i32 genDepth);

private:
    bool m_isNeedingChest;
};

class CastleCorridorStairsPiece : public NetherBridgePiece {
public:
    CastleCorridorStairsPiece(i32 genDepth, const StructureBoundingBox& box, Direction orientation);

    void addChildren(StartPiece& start, PieceAccessor& accessor, math::IRandom& rng) override;

    void generate(IWorldWriter& world,
        math::IRandom& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk,
        IChunkGenerator* generator) override;

    [[nodiscard]] static std::unique_ptr<CastleCorridorStairsPiece> createPiece(
        PieceAccessor& accessor, i32 x, i32 y, i32 z, Direction direction, i32 genDepth);
};

class CastleCorridorTBalconyPiece : public NetherBridgePiece {
public:
    CastleCorridorTBalconyPiece(i32 genDepth, const StructureBoundingBox& box, Direction orientation);

    void addChildren(StartPiece& start, PieceAccessor& accessor, math::IRandom& rng) override;

    void generate(IWorldWriter& world,
        math::IRandom& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk,
        IChunkGenerator* generator) override;

    [[nodiscard]] static std::unique_ptr<CastleCorridorTBalconyPiece> createPiece(
        PieceAccessor& accessor, i32 x, i32 y, i32 z, Direction direction, i32 genDepth);
};

class CastleStalkRoom : public NetherBridgePiece {
public:
    CastleStalkRoom(i32 genDepth, const StructureBoundingBox& box, Direction orientation);

    void addChildren(StartPiece& start, PieceAccessor& accessor, math::IRandom& rng) override;

    void generate(IWorldWriter& world,
        math::IRandom& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk,
        IChunkGenerator* generator) override;

    [[nodiscard]] static std::unique_ptr<CastleStalkRoom> createPiece(
        PieceAccessor& accessor, i32 x, i32 y, i32 z, Direction direction, i32 genDepth);
};

// ============================================================================
// 起始片段
// ============================================================================

/**
 * @brief 起始片段（原版 `NetherFortressPieces.StartPiece`）
 *
 * 自身是一个 BridgeCrossing（19×10×19），另外持有两份权重表、上一个片段指针与待展开队列。
 */
class StartPiece : public BridgeCrossing {
public:
    StartPiece(math::IRandom& rng, i32 x, i32 z);

    [[nodiscard]] std::vector<std::unique_ptr<PieceWeight>>& availableBridgePieces()
    {
        return m_availableBridgePieces;
    }

    [[nodiscard]] std::vector<std::unique_ptr<PieceWeight>>& availableCastlePieces()
    {
        return m_availableCastlePieces;
    }

    [[nodiscard]] PieceWeight* previousPiece() const
    {
        return m_previousPiece;
    }

    void setPreviousPiece(PieceWeight* weight)
    {
        m_previousPiece = weight;
    }

    [[nodiscard]] std::vector<StructurePiece*>& pendingChildren()
    {
        return m_pendingChildren;
    }

private:
    PieceWeight* m_previousPiece = nullptr;
    std::vector<std::unique_ptr<PieceWeight>> m_availableBridgePieces;
    std::vector<std::unique_ptr<PieceWeight>> m_availableCastlePieces;
    std::vector<StructurePiece*> m_pendingChildren;
};

} // namespace mc::world::gen::structure::nether_fortress
