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
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/structure/Structure.hpp"
#include <memory>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace structure {

// 前向声明
class StrongholdPiece;
class StrongholdStartStairs;

/**
 * @brief 要塞片段权重
 *
 * 用于要塞片段的随机选择
 */
struct StrongholdPieceWeight {
    i32 pieceType;        ///< 片段类型
    i32 weight;           ///< 选择权重
    i32 instancesSpawned; ///< 已生成数量
    i32 instancesLimit;   ///< 最大数量 (0 = 无限制)
    i32 minDepth;         ///< 最小深度条件 (0 = 无条件)

    StrongholdPieceWeight(i32 type, i32 w, i32 limit, i32 depth = 0)
        : pieceType(type)
        , weight(w)
        , instancesSpawned(0)
        , instancesLimit(limit)
        , minDepth(depth)
    {}

    [[nodiscard]] bool canSpawnMoreStructuresOfType(i32 depth) const
    {
        // 检查深度条件
        if (minDepth > 0 && depth <= minDepth) {
            return false;
        }
        return instancesLimit == 0 || instancesSpawned < instancesLimit;
    }

    [[nodiscard]] bool canSpawnMoreStructures() const
    {
        return instancesLimit == 0 || instancesSpawned < instancesLimit;
    }
};

// ============================================================================
// 要塞片段基类
// ============================================================================

/**
 * @brief 要塞片段基类
 */
class StrongholdPiece : public StructurePiece {
public:
    /**
     * @brief 门类型枚举
     */
    enum class Door : u8 {
        Opening,  ///< 开口
        WoodDoor, ///< 木门
        Grates,   ///< 铁栏杆
        IronDoor  ///< 铁门
    };

    StrongholdPiece(i32 type, i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ);

    [[nodiscard]] Door entryDoor() const { return m_entryDoor; }
    void setEntryDoor(Door door) { m_entryDoor = door; }

    /**
     * @brief 获取随机门类型
     */
    [[nodiscard]] static Door getRandomDoor(math::Random& rng);

    /**
     * @brief 生成门
     */
    void generateDoor(
        IWorldWriter& world, const StructureBoundingBox& bounds, math::Random& rng, Door door, i32 x, i32 y, i32 z);

    /**
     * @brief 检查是否可以继续向下生成
     */
    [[nodiscard]] static bool canStrongholdGoDeeper(const StructureBoundingBox& box);

    /**
     * @brief 获取下一个组件（正向）
     *
     * 在当前片段前方生成下一个连接片段
     */
    StructurePiece* getNextComponentNormal(StrongholdStartStairs* start,
        std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::Random& rng,
        i32 offsetX,
        i32 offsetY);

    /**
     * @brief 获取下一个组件（X方向）
     *
     * 在当前片段左侧或右侧生成连接片段
     */
    StructurePiece* getNextComponentX(StrongholdStartStairs* start,
        std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::Random& rng,
        i32 offsetX,
        i32 offsetY);

    /**
     * @brief 获取下一个组件（Z方向）
     *
     * 在当前片段左侧或右侧生成连接片段
     */
    StructurePiece* getNextComponentZ(StrongholdStartStairs* start,
        std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::Random& rng,
        i32 offsetX,
        i32 offsetY);

protected:
    Door m_entryDoor = Door::Opening;
};

// ============================================================================
// 要塞石砖选择器
// ============================================================================

/**
 * @brief 要塞石砖选择器
 */
class StrongholdStonesSelector : public StructurePiece::BlockSelector {
public:
    void selectBlocks(math::Random& rng, i32 x, i32 y, i32 z, bool isWall) override;
};

// ============================================================================
// 要塞直走廊
// ============================================================================

/**
 * @brief 要塞直走廊
 */
class StrongholdStraight : public StrongholdPiece {
public:
    StrongholdStraight(i32 componentType,
        math::Random& rng,
        i32 minX,
        i32 minY,
        i32 minZ,
        i32 maxX,
        i32 maxY,
        i32 maxZ,
        Direction direction);

    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

    void buildComponent(
        StructurePiece* component, std::vector<std::unique_ptr<StructurePiece>>& pieces, math::Random& rng) override;

    [[nodiscard]] static StrongholdStraight* createPiece(std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::Random& rng,
        i32 x,
        i32 y,
        i32 z,
        Direction direction,
        i32 depth);

private:
    bool m_expandsLeft;
    bool m_expandsRight;
};

// ============================================================================
// 要塞监狱
// ============================================================================

/**
 * @brief 要塞监狱
 */
class StrongholdPrison : public StrongholdPiece {
public:
    StrongholdPrison(i32 componentType,
        math::Random& rng,
        i32 minX,
        i32 minY,
        i32 minZ,
        i32 maxX,
        i32 maxY,
        i32 maxZ,
        Direction direction);

    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

    void buildComponent(
        StructurePiece* component, std::vector<std::unique_ptr<StructurePiece>>& pieces, math::Random& rng) override;

    [[nodiscard]] static StrongholdPrison* createPiece(std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::Random& rng,
        i32 x,
        i32 y,
        i32 z,
        Direction direction,
        i32 depth);
};

// ============================================================================
// 要塞左转
// ============================================================================

/**
 * @brief 要塞左转
 */
class StrongholdLeftTurn : public StrongholdPiece {
public:
    StrongholdLeftTurn(i32 componentType,
        math::Random& rng,
        i32 minX,
        i32 minY,
        i32 minZ,
        i32 maxX,
        i32 maxY,
        i32 maxZ,
        Direction direction);

    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

    void buildComponent(
        StructurePiece* component, std::vector<std::unique_ptr<StructurePiece>>& pieces, math::Random& rng) override;

    [[nodiscard]] static StrongholdLeftTurn* createPiece(std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::Random& rng,
        i32 x,
        i32 y,
        i32 z,
        Direction direction,
        i32 depth);
};

// ============================================================================
// 要塞右转
// ============================================================================

/**
 * @brief 要塞右转
 */
class StrongholdRightTurn : public StrongholdPiece {
public:
    StrongholdRightTurn(i32 componentType,
        math::Random& rng,
        i32 minX,
        i32 minY,
        i32 minZ,
        i32 maxX,
        i32 maxY,
        i32 maxZ,
        Direction direction);

    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

    void buildComponent(
        StructurePiece* component, std::vector<std::unique_ptr<StructurePiece>>& pieces, math::Random& rng) override;

    [[nodiscard]] static StrongholdRightTurn* createPiece(std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::Random& rng,
        i32 x,
        i32 y,
        i32 z,
        Direction direction,
        i32 depth);
};

// ============================================================================
// 要塞房间交叉点
// ============================================================================

/**
 * @brief 要塞房间交叉点
 */
class StrongholdRoomCrossing : public StrongholdPiece {
public:
    StrongholdRoomCrossing(i32 componentType,
        math::Random& rng,
        i32 minX,
        i32 minY,
        i32 minZ,
        i32 maxX,
        i32 maxY,
        i32 maxZ,
        Direction direction);

    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

    void buildComponent(
        StructurePiece* component, std::vector<std::unique_ptr<StructurePiece>>& pieces, math::Random& rng) override;

    [[nodiscard]] static StrongholdRoomCrossing* createPiece(std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::Random& rng,
        i32 x,
        i32 y,
        i32 z,
        Direction direction,
        i32 depth);

private:
    i32 m_roomType; ///< 0=喷泉, 1=火把柱, 2=宝箱房间
};

// ============================================================================
// 要塞直楼梯
// ============================================================================

/**
 * @brief 要塞直楼梯
 */
class StrongholdStairsStraight : public StrongholdPiece {
public:
    StrongholdStairsStraight(i32 componentType,
        math::Random& rng,
        i32 minX,
        i32 minY,
        i32 minZ,
        i32 maxX,
        i32 maxY,
        i32 maxZ,
        Direction direction);

    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

    void buildComponent(
        StructurePiece* component, std::vector<std::unique_ptr<StructurePiece>>& pieces, math::Random& rng) override;

    [[nodiscard]] static StrongholdStairsStraight* createPiece(std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::Random& rng,
        i32 x,
        i32 y,
        i32 z,
        Direction direction,
        i32 depth);
};

// ============================================================================
// 要塞螺旋楼梯
// ============================================================================

/**
 * @brief 要塞螺旋楼梯
 */
class StrongholdStairs : public StrongholdPiece {
public:
    StrongholdStairs(i32 componentType,
        math::Random& rng,
        i32 minX,
        i32 minY,
        i32 minZ,
        i32 maxX,
        i32 maxY,
        i32 maxZ,
        Direction direction);

    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

    void buildComponent(
        StructurePiece* component, std::vector<std::unique_ptr<StructurePiece>>& pieces, math::Random& rng) override;

    [[nodiscard]] static StrongholdStairs* createPiece(std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::Random& rng,
        i32 x,
        i32 y,
        i32 z,
        Direction direction,
        i32 depth);

protected:
    bool m_isSource = false; ///< 是否是起始楼梯
};

// ============================================================================
// 要塞起始楼梯
// ============================================================================

/**
 * @brief 要塞起始楼梯
 *
 * 存储要塞生成的全局状态（权重列表、lastPlaced等）
 */
class StrongholdStartStairs : public StrongholdStairs {
public:
    explicit StrongholdStartStairs(math::Random& rng, i32 x, i32 z);

    void buildComponent(
        StructurePiece* component, std::vector<std::unique_ptr<StructurePiece>>& pieces, math::Random& rng) override;

    [[nodiscard]] StrongholdPieceWeight* lastPlaced() const { return m_lastPlaced; }
    void setLastPlaced(StrongholdPieceWeight* weight) { m_lastPlaced = weight; }
    StrongholdPieceWeight*& lastPlacedRef() { return m_lastPlaced; }

    [[nodiscard]] StrongholdPiece* portalRoom() const { return m_portalRoom; }
    void setPortalRoom(StrongholdPiece* room) { m_portalRoom = room; }

    [[nodiscard]] const std::vector<StructurePiece*>& pendingChildren() const { return m_pendingChildren; }
    void addPendingChild(StructurePiece* piece) { m_pendingChildren.push_back(piece); }

    [[nodiscard]] std::vector<StrongholdPieceWeight>& weights() { return m_weights; }
    [[nodiscard]] const std::vector<StrongholdPieceWeight>& weights() const { return m_weights; }

    /**
     * @brief 获取强制片段类型
     * 对应 MC Java 的 imposedPiece 字段，当非空时下一个片段必须为指定类型
     */
    [[nodiscard]] i32 imposedPieceType() const { return m_imposedPieceType; }

    /**
     * @brief 设置强制片段类型并清除
     * @param type 片段类型（设为 -1 表示无强制）
     */
    void setImposedPieceType(i32 type) { m_imposedPieceType = type; }

private:
    std::vector<StrongholdPieceWeight> m_weights;   ///< 片段权重列表
    StrongholdPieceWeight* m_lastPlaced = nullptr;  ///< 上一个放置的片段权重
    StrongholdPiece* m_portalRoom = nullptr;        ///< 传送门房间引用
    std::vector<StructurePiece*> m_pendingChildren; ///< 待处理的子片段
    i32 m_imposedPieceType = -1;                    ///< 强制片段类型（-1 表示无强制）
};

// ============================================================================
// 要塞交叉点
// ============================================================================

/**
 * @brief 要塞交叉点
 */
class StrongholdCrossing : public StrongholdPiece {
public:
    StrongholdCrossing(i32 componentType,
        math::Random& rng,
        i32 minX,
        i32 minY,
        i32 minZ,
        i32 maxX,
        i32 maxY,
        i32 maxZ,
        Direction direction);

    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

    void buildComponent(
        StructurePiece* component, std::vector<std::unique_ptr<StructurePiece>>& pieces, math::Random& rng) override;

    [[nodiscard]] static StrongholdCrossing* createPiece(std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::Random& rng,
        i32 x,
        i32 y,
        i32 z,
        Direction direction,
        i32 depth);

private:
    bool m_leftLow;
    bool m_leftHigh;
    bool m_rightLow;
    bool m_rightHigh;
};

// ============================================================================
// 要塞宝箱走廊
// ============================================================================

/**
 * @brief 要塞宝箱走廊
 */
class StrongholdChestCorridor : public StrongholdPiece {
public:
    StrongholdChestCorridor(i32 componentType,
        math::Random& rng,
        i32 minX,
        i32 minY,
        i32 minZ,
        i32 maxX,
        i32 maxY,
        i32 maxZ,
        Direction direction);

    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

    void buildComponent(
        StructurePiece* component, std::vector<std::unique_ptr<StructurePiece>>& pieces, math::Random& rng) override;

    [[nodiscard]] static StrongholdChestCorridor* createPiece(std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::Random& rng,
        i32 x,
        i32 y,
        i32 z,
        Direction direction,
        i32 depth);

private:
    bool m_hasChest = false;
};

// ============================================================================
// 要塞图书馆
// ============================================================================

/**
 * @brief 要塞图书馆
 */
class StrongholdLibrary : public StrongholdPiece {
public:
    StrongholdLibrary(i32 componentType,
        math::Random& rng,
        i32 minX,
        i32 minY,
        i32 minZ,
        i32 maxX,
        i32 maxY,
        i32 maxZ,
        Direction direction);

    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

    [[nodiscard]] static StrongholdLibrary* createPiece(std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::Random& rng,
        i32 x,
        i32 y,
        i32 z,
        Direction direction,
        i32 depth);

    [[nodiscard]] bool isLargeRoom() const { return m_isLargeRoom; }

private:
    bool m_isLargeRoom;
};

// ============================================================================
// 要塞传送门房间
// ============================================================================

/**
 * @brief 要塞传送门房间
 */
class StrongholdPortalRoom : public StrongholdPiece {
public:
    StrongholdPortalRoom(
        i32 componentType, i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ, Direction direction);

    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

    void buildComponent(
        StructurePiece* component, std::vector<std::unique_ptr<StructurePiece>>& pieces, math::Random& rng) override;

    [[nodiscard]] static StrongholdPortalRoom* createPiece(
        std::vector<std::unique_ptr<StructurePiece>>& pieces, i32 x, i32 y, i32 z, Direction direction, i32 depth);

private:
    bool m_hasSpawner = false;
};

// ============================================================================
// 要塞填充走廊
// ============================================================================

/**
 * @brief 要塞填充走廊
 */
class StrongholdCorridor : public StrongholdPiece {
public:
    StrongholdCorridor(
        i32 componentType, i32 steps, i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ, Direction direction);

    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

    [[nodiscard]] static StrongholdCorridor* createPiece(std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::Random& rng,
        i32 x,
        i32 y,
        i32 z,
        Direction direction,
        i32 depth);

    [[nodiscard]] static StructureBoundingBox findPieceBox(std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::Random& rng,
        i32 x,
        i32 y,
        i32 z,
        Direction direction);

private:
    i32 m_steps; ///< 走廊长度（步数）
};

// ============================================================================
// 片段类型常量
// ============================================================================

namespace StrongholdPieceTypes {
constexpr i32 STRAIGHT = 100;
constexpr i32 PRISON = 101;
constexpr i32 LEFT_TURN = 102;
constexpr i32 RIGHT_TURN = 103;
constexpr i32 ROOM_CROSSING = 104;
constexpr i32 STAIRS_STRAIGHT = 105;
constexpr i32 STAIRS = 106;
constexpr i32 START_STAIRS = 107;
constexpr i32 CROSSING = 108;
constexpr i32 CHEST_CORRIDOR = 109;
constexpr i32 LIBRARY = 110;
constexpr i32 PORTAL_ROOM = 111;
constexpr i32 CORRIDOR = 112;
} // namespace StrongholdPieceTypes

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * @brief 初始化要塞片段权重列表
 */
void initializeStrongholdPieceWeights(std::vector<StrongholdPieceWeight>& weights);

/**
 * @brief 创建要塞片段
 */
[[nodiscard]] StrongholdPiece* createStrongholdPiece(i32 pieceType,
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng,
    i32 x,
    i32 y,
    i32 z,
    Direction direction,
    i32 depth);

/**
 * @brief 检查是否可以添加更多片段
 */
[[nodiscard]] bool canAddStructurePieces(std::vector<StrongholdPieceWeight>& weights, i32& outTotalWeight);

/**
 * @brief 从小门生成要塞片段
 */
[[nodiscard]] StrongholdPiece* generatePieceFromSmallDoor(StrongholdStartStairs* start,
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng,
    i32 x,
    i32 y,
    i32 z,
    Direction direction,
    i32 depth,
    std::vector<StrongholdPieceWeight>& weights,
    StrongholdPieceWeight*& lastPlaced);

/**
 * @brief 生成并添加片段
 */
[[nodiscard]] StructurePiece* generateAndAddPiece(StrongholdStartStairs* start,
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng,
    i32 x,
    i32 y,
    i32 z,
    Direction direction,
    i32 depth);

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
