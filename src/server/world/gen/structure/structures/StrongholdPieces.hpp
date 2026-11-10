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
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include <memory>
#include <optional>
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

    /**
     * @param type 片段类型
     * @param w 抽签权重
     * @param limit 实例上限（0 = 无限制）
     * @param depth 最小深度：只在该深度**之后**才可被抽中（0 = 无门槛），
     *              对应原版 Library（>4）与 PortalRoom（>5）覆写的 `doPlace`
     */
    StrongholdPieceWeight(i32 type, i32 w, i32 limit, i32 depth)
        : pieceType(type)
        , weight(w)
        , instancesSpawned(0)
        , instancesLimit(limit)
        , minDepth(depth)
    {}

    /** 原版 `doPlace`：深度门槛与实例余额，两者都满足才允许放入。 */
    [[nodiscard]] bool canSpawnMoreStructuresOfType(i32 depth) const
    {
        if (minDepth > 0 && depth <= minDepth) {
            return false;
        }
        return instancesLimit == 0 || instancesSpawned < instancesLimit;
    }

    /**
     * @brief 原版 `PieceWeight.isValid()`：只补实例上限，**不含**深度门槛
     *
     * 用尽后该类型要从抽签表里移除；若把深度门槛也并进来，被深度挡下的那次放置会
     * 把类型误判为「已用尽」并从表中删除，后续分支再也抽不到它。
     */
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

    /**
     * @brief 构造要塞片段
     *
     * @param originX/originY/originZ 本片段在**原版坐标系**里的构造原点，即展开包围盒时
     *        `orientBox` 的第一个实参。子构件的落点沿用它做「原版坐标 → 世界坐标」的基准，
     *        因此必须原样传入；见 `StructurePiece` 三参版本构造函数的说明。
     * @param sizeX/sizeZ 本片段在 `orientBox` 里用的局部尺寸。原版子构件落点与
     *        `addChildren` 的偏移翻转都以这两个尺寸为基准（例如五向交叉点的 `8 - i`），
     *        而盒子的世界跨度在 EAST 下会与它们互换，故显式记下来。
     */
    StrongholdPiece(i32 type,
        i32 minX,
        i32 minY,
        i32 minZ,
        i32 maxX,
        i32 maxY,
        i32 maxZ,
        i32 originX,
        i32 originY,
        i32 originZ,
        i32 sizeX,
        i32 sizeZ,
        i32 offsetX,
        i32 offsetZ);

    [[nodiscard]] Door entryDoor() const
    {
        return m_entryDoor;
    }

    void setEntryDoor(Door door)
    {
        m_entryDoor = door;
    }

    /**
     * @brief 获取随机门类型
     */
    [[nodiscard]] static Door getRandomDoor(math::IRandom& rng);

    /**
     * @brief 生成门
     */
    void generateDoor(
        IWorldWriter& world, const StructureBoundingBox& bounds, math::IRandom& rng, Door door, i32 x, i32 y, i32 z);

    /**
     * @brief 检查是否可以继续向下生成
     */
    [[nodiscard]] static bool canStrongholdGoDeeper(const StructureBoundingBox& box);

    /**
     * @brief 原版坐标系里的盒子近角/远角
     *
     * 原版子构件落点写成 `this.boundingBox.minX() + 偏移` 这类形式，而 `minX()` 在原版里
     * 是 `orientBox` 算出的盒角，与构造原点之间只差一个由方向决定的常量。本项目盒子已是
     * 世界坐标，镜像朝向下不能直接拿来当原版盒角用，故由原点加上这两个常量还原。
     *
     * @param direction 本片段的导入口朝向（原版 `getOrientation()`）
     */
    [[nodiscard]] i32 orientedMinX(Direction direction) const noexcept;
    [[nodiscard]] i32 orientedMaxX(Direction direction) const noexcept;
    [[nodiscard]] i32 orientedMinZ(Direction direction) const noexcept;
    [[nodiscard]] i32 orientedMaxZ(Direction direction) const noexcept;

    /** @brief `orientBox` 的局部尺寸 X（原版片段里写死的 WIDTH）。 */
    [[nodiscard]] i32 sizeX() const noexcept
    {
        return m_sizeX;
    }

    /** @brief `orientBox` 的局部尺寸 Z（原版片段里写死的 DEPTH）。 */
    [[nodiscard]] i32 sizeZ() const noexcept
    {
        return m_sizeZ;
    }

    /**
     * @brief 把「沿局部 Z 的偏移」换算成世界偏移
     *
     * 原版若干 `addChildren` 在朝 NORTH/WEST 时把局部偏移取成 `尺寸 - 偏移 - 1`
     * （例如五向交叉点的 `i = 8 - i`）。世界朝向下这段偏移是沿 X 还是沿 Z，随朝向互换。
     */
    [[nodiscard]] static bool flipsLocalZ(Direction direction) noexcept
    {
        return direction == Direction::North || direction == Direction::West;
    }

    /**
     * @brief 获取下一个组件（正向）
     *
     * 在当前片段前方生成下一个连接片段
     */
    StructurePiece* getNextComponentNormal(StrongholdStartStairs* start,
        const StructurePiece& previous,
        std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::IRandom& rng,
        i32 positionOffset,
        i32 verticalOffset);

    /**
     * @brief 获取下一个组件（X方向）
     *
     * 在当前片段左侧或右侧生成连接片段
     */
    StructurePiece* getNextComponentX(StrongholdStartStairs* start,
        const StructurePiece& previous,
        std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::IRandom& rng,
        i32 verticalOffset,
        i32 positionOffset);

    /**
     * @brief 获取下一个组件（Z方向）
     *
     * 在当前片段左侧或右侧生成连接片段
     */
    StructurePiece* getNextComponentZ(StrongholdStartStairs* start,
        const StructurePiece& previous,
        std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::IRandom& rng,
        i32 verticalOffset,
        i32 positionOffset);

protected:
    Door m_entryDoor = Door::Opening;
    i32 m_sizeX = 0;
    i32 m_sizeZ = 0;
    /** orientBox 的局部偏移（各片段不同：多数是 -1，五向交叉点是 -4）。 */
    i32 m_offsetX = 0;
    i32 m_offsetZ = 0;
};

// ============================================================================
// 要塞石砖选择器
// ============================================================================

/**
 * @brief 要塞石砖选择器
 */
class StrongholdStonesSelector : public StructurePiece::BlockSelector {
public:
    void selectBlocks(math::IRandom& rng, i32 x, i32 y, i32 z, bool isWall) override;
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
        math::IRandom& rng,
        i32 minX,
        i32 minY,
        i32 minZ,
        i32 maxX,
        i32 maxY,
        i32 maxZ,
        i32 originX,
        i32 originY,
        i32 originZ,
        i32 sizeX,
        i32 sizeZ,
        i32 offsetX,
        i32 offsetZ,
        Direction direction);

    void generate(IWorldWriter& world,
        math::IRandom& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

    void buildComponent(
        StructurePiece* component, std::vector<std::unique_ptr<StructurePiece>>& pieces, math::IRandom& rng) override;

    [[nodiscard]] static std::unique_ptr<StrongholdStraight> createPiece(
        std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::IRandom& rng,
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
        math::IRandom& rng,
        i32 minX,
        i32 minY,
        i32 minZ,
        i32 maxX,
        i32 maxY,
        i32 maxZ,
        i32 originX,
        i32 originY,
        i32 originZ,
        i32 sizeX,
        i32 sizeZ,
        i32 offsetX,
        i32 offsetZ,
        Direction direction);

    void generate(IWorldWriter& world,
        math::IRandom& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

    void buildComponent(
        StructurePiece* component, std::vector<std::unique_ptr<StructurePiece>>& pieces, math::IRandom& rng) override;

    [[nodiscard]] static std::unique_ptr<StrongholdPrison> createPiece(
        std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::IRandom& rng,
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
        math::IRandom& rng,
        i32 minX,
        i32 minY,
        i32 minZ,
        i32 maxX,
        i32 maxY,
        i32 maxZ,
        i32 originX,
        i32 originY,
        i32 originZ,
        i32 sizeX,
        i32 sizeZ,
        i32 offsetX,
        i32 offsetZ,
        Direction direction);

    void generate(IWorldWriter& world,
        math::IRandom& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

    void buildComponent(
        StructurePiece* component, std::vector<std::unique_ptr<StructurePiece>>& pieces, math::IRandom& rng) override;

    [[nodiscard]] static std::unique_ptr<StrongholdLeftTurn> createPiece(
        std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::IRandom& rng,
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
        math::IRandom& rng,
        i32 minX,
        i32 minY,
        i32 minZ,
        i32 maxX,
        i32 maxY,
        i32 maxZ,
        i32 originX,
        i32 originY,
        i32 originZ,
        i32 sizeX,
        i32 sizeZ,
        i32 offsetX,
        i32 offsetZ,
        Direction direction);

    void generate(IWorldWriter& world,
        math::IRandom& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

    void buildComponent(
        StructurePiece* component, std::vector<std::unique_ptr<StructurePiece>>& pieces, math::IRandom& rng) override;

    [[nodiscard]] static std::unique_ptr<StrongholdRightTurn> createPiece(
        std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::IRandom& rng,
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
        math::IRandom& rng,
        i32 minX,
        i32 minY,
        i32 minZ,
        i32 maxX,
        i32 maxY,
        i32 maxZ,
        i32 originX,
        i32 originY,
        i32 originZ,
        i32 sizeX,
        i32 sizeZ,
        i32 offsetX,
        i32 offsetZ,
        Direction direction);

    void generate(IWorldWriter& world,
        math::IRandom& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

    void buildComponent(
        StructurePiece* component, std::vector<std::unique_ptr<StructurePiece>>& pieces, math::IRandom& rng) override;

    [[nodiscard]] static std::unique_ptr<StrongholdRoomCrossing> createPiece(
        std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::IRandom& rng,
        i32 x,
        i32 y,
        i32 z,
        Direction direction,
        i32 depth);

    /** 房间内景类型：构造时由 `nextInt(5)` 抽出，决定墙饰与宝箱等细节。 */
    [[nodiscard]] i32 roomType() const noexcept
    {
        return m_roomType;
    }

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
        math::IRandom& rng,
        i32 minX,
        i32 minY,
        i32 minZ,
        i32 maxX,
        i32 maxY,
        i32 maxZ,
        i32 originX,
        i32 originY,
        i32 originZ,
        i32 sizeX,
        i32 sizeZ,
        i32 offsetX,
        i32 offsetZ,
        Direction direction);

    void generate(IWorldWriter& world,
        math::IRandom& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

    void buildComponent(
        StructurePiece* component, std::vector<std::unique_ptr<StructurePiece>>& pieces, math::IRandom& rng) override;

    [[nodiscard]] static std::unique_ptr<StrongholdStairsStraight> createPiece(
        std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::IRandom& rng,
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
    /**
     * @brief 构造楼梯
     *
     * @param box 已按朝向展开好的世界包围盒
     * @param originX/originY/originZ 展开该盒时 `orientBox` 用的原点（原版坐标系）
     * @param fixedDoor 非空时使用该入口门且**不消耗随机数**；为空时按原版
     *                  `randomSmallDoor` 抽一次。起始楼梯必须传 `Door::Opening`。
     * @param sizeX/sizeZ `orientBox` 的局部尺寸
     */
    StrongholdStairs(i32 componentType,
        math::IRandom& rng,
        i32 minX,
        i32 minY,
        i32 minZ,
        i32 maxX,
        i32 maxY,
        i32 maxZ,
        i32 originX,
        i32 originY,
        i32 originZ,
        Direction direction,
        std::optional<Door> fixedDoor,
        i32 sizeX,
        i32 sizeZ,
        i32 offsetX,
        i32 offsetZ);

    void generate(IWorldWriter& world,
        math::IRandom& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

    void buildComponent(
        StructurePiece* component, std::vector<std::unique_ptr<StructurePiece>>& pieces, math::IRandom& rng) override;

    [[nodiscard]] static std::unique_ptr<StrongholdStairs> createPiece(
        std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::IRandom& rng,
        i32 x,
        i32 y,
        i32 z,
        Direction direction,
        i32 depth);

protected:
    bool m_isSource = false;                   ///< 是否是起始楼梯
    Direction m_orientation = Direction::None; ///< 起始楼梯的导入口朝向
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
    /**
     * @brief 构造起始楼梯
     *
     * @param direction 朝向。必须由调用方用 `nextInt(4)` 抽取后映射，
     *                  原版候选顺序为 NORTH/EAST/SOUTH/WEST。
     */
    StrongholdStartStairs(math::IRandom& rng, i32 x, i32 z, Direction direction);

    /** 原版起始楼梯的固定起始高度（包围盒从这里向下伸展 11 格）。 */
    static constexpr i32 MAGIC_START_Y = 64;

    /**
     * @brief 获取原版 `getOrientation()`：起始楼梯的导入口朝向
     *
     * 其余构件把朝向编码进 rotation + mirror，由 `generateSmallDoorChild*` 内部换算，
     * 只有起始楼梯显式记录了这个朝向。
     */
    [[nodiscard]] Direction orientation() const
    {
        return m_orientation;
    }

    void buildComponent(
        StructurePiece* component, std::vector<std::unique_ptr<StructurePiece>>& pieces, math::IRandom& rng) override;

    /**
     * @brief 上一次放下的片段类型（对应原版 `StartPiece.previousPiece`）在抽签表中的下标
     *
     * 用下标而不是指针：类型用尽时要从表里删除，指针会悬垂。
     * -1 表示尚未放置过任何片段。
     */
    [[nodiscard]] i32& lastPlacedIndex()
    {
        return m_lastPlacedIndex;
    }

    [[nodiscard]] StrongholdPiece* portalRoom() const
    {
        return m_portalRoom;
    }

    void setPortalRoom(StrongholdPiece* room)
    {
        m_portalRoom = room;
    }

    [[nodiscard]] const std::vector<StructurePiece*>& pendingChildren() const
    {
        return m_pendingChildren;
    }

    /** 待扩展队列的可写视图：装配循环要按原版「随机下标取出并移除」消费它。 */
    [[nodiscard]] std::vector<StructurePiece*>& mutablePendingChildren()
    {
        return m_pendingChildren;
    }

    void addPendingChild(StructurePiece* piece)
    {
        m_pendingChildren.push_back(piece);
    }

    [[nodiscard]] std::vector<StrongholdPieceWeight>& weights()
    {
        return m_weights;
    }

    [[nodiscard]] const std::vector<StrongholdPieceWeight>& weights() const
    {
        return m_weights;
    }

    /**
     * @brief 获取强制片段类型
     * 对应 MC Java 的 imposedPiece 字段，当非空时下一个片段必须为指定类型
     */
    [[nodiscard]] i32 imposedPieceType() const
    {
        return m_imposedPieceType;
    }

    /**
     * @brief 设置强制片段类型并清除
     * @param type 片段类型（设为 -1 表示无强制）
     */
    void setImposedPieceType(i32 type)
    {
        m_imposedPieceType = type;
    }

private:
    std::vector<StrongholdPieceWeight> m_weights;   ///< 片段权重列表（用尽的类型会被移出）
    i32 m_lastPlacedIndex = -1;                     ///< 上一次放下的类型下标（-1 = 无）
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
        math::IRandom& rng,
        i32 minX,
        i32 minY,
        i32 minZ,
        i32 maxX,
        i32 maxY,
        i32 maxZ,
        i32 originX,
        i32 originY,
        i32 originZ,
        i32 sizeX,
        i32 sizeZ,
        i32 offsetX,
        i32 offsetZ,
        Direction direction);

    void generate(IWorldWriter& world,
        math::IRandom& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

    void buildComponent(
        StructurePiece* component, std::vector<std::unique_ptr<StructurePiece>>& pieces, math::IRandom& rng) override;

    [[nodiscard]] static std::unique_ptr<StrongholdCrossing> createPiece(
        std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::IRandom& rng,
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
        math::IRandom& rng,
        i32 minX,
        i32 minY,
        i32 minZ,
        i32 maxX,
        i32 maxY,
        i32 maxZ,
        i32 originX,
        i32 originY,
        i32 originZ,
        i32 sizeX,
        i32 sizeZ,
        i32 offsetX,
        i32 offsetZ,
        Direction direction);

    void generate(IWorldWriter& world,
        math::IRandom& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

    void buildComponent(
        StructurePiece* component, std::vector<std::unique_ptr<StructurePiece>>& pieces, math::IRandom& rng) override;

    [[nodiscard]] static std::unique_ptr<StrongholdChestCorridor> createPiece(
        std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::IRandom& rng,
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
        math::IRandom& rng,
        i32 minX,
        i32 minY,
        i32 minZ,
        i32 maxX,
        i32 maxY,
        i32 maxZ,
        i32 originX,
        i32 originY,
        i32 originZ,
        i32 sizeX,
        i32 sizeZ,
        i32 offsetX,
        i32 offsetZ,
        Direction direction);

    void generate(IWorldWriter& world,
        math::IRandom& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

    [[nodiscard]] static std::unique_ptr<StrongholdLibrary> createPiece(
        std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::IRandom& rng,
        i32 x,
        i32 y,
        i32 z,
        Direction direction,
        i32 depth);

    [[nodiscard]] bool isLargeRoom() const
    {
        return m_isLargeRoom;
    }

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
    StrongholdPortalRoom(i32 componentType,
        math::IRandom& rng,
        i32 minX,
        i32 minY,
        i32 minZ,
        i32 maxX,
        i32 maxY,
        i32 maxZ,
        i32 originX,
        i32 originY,
        i32 originZ,
        i32 sizeX,
        i32 sizeZ,
        i32 offsetX,
        i32 offsetZ,
        Direction direction);

    void generate(IWorldWriter& world,
        math::IRandom& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

    void buildComponent(
        StructurePiece* component, std::vector<std::unique_ptr<StructurePiece>>& pieces, math::IRandom& rng) override;

    [[nodiscard]] static std::unique_ptr<StrongholdPortalRoom> createPiece(
        std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::IRandom& rng,
        i32 x,
        i32 y,
        i32 z,
        Direction direction,
        i32 depth);

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
    StrongholdCorridor(i32 componentType,
        i32 steps,
        math::IRandom& rng,
        i32 minX,
        i32 minY,
        i32 minZ,
        i32 maxX,
        i32 maxY,
        i32 maxZ,
        i32 originX,
        i32 originY,
        i32 originZ,
        i32 sizeX,
        i32 sizeZ,
        i32 offsetX,
        i32 offsetZ,
        Direction direction);

    void generate(IWorldWriter& world,
        math::IRandom& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

    [[nodiscard]] static std::unique_ptr<StrongholdCorridor> createPiece(
        std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::IRandom& rng,
        i32 x,
        i32 y,
        i32 z,
        Direction direction,
        i32 depth);

    [[nodiscard]] static StructureBoundingBox findPieceBox(std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::IRandom& rng,
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
[[nodiscard]] std::unique_ptr<StrongholdPiece> createStrongholdPiece(i32 pieceType,
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::IRandom& rng,
    i32 x,
    i32 y,
    i32 z,
    Direction direction,
    i32 depth);

/**
 * @brief 重算抽签总权重，并报告是否还有受限类型可放置（原版 `updatePieceWeight`）
 */
[[nodiscard]] bool updatePieceWeight(std::vector<StrongholdPieceWeight>& weights, i32& outTotalWeight);

/**
 * @brief 从小门生成要塞片段
 */
[[nodiscard]] std::unique_ptr<StrongholdPiece> generatePieceFromSmallDoor(StrongholdStartStairs* start,
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::IRandom& rng,
    i32 x,
    i32 y,
    i32 z,
    Direction direction,
    i32 depth,
    std::vector<StrongholdPieceWeight>& weights,
    i32& lastPlacedIndex);

/**
 * @brief 生成并添加片段
 *
 * 新片段的所有权立即交给 @p pieces（最终落入 StructureStart::m_pieces），
 * 返回值是**非拥有观察指针**，仅便于调用方继续传递，调用方不得释放。
 */
[[nodiscard]] StructurePiece* generateAndAddPiece(StrongholdStartStairs* start,
    const StructurePiece& previous,
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::IRandom& rng,
    i32 x,
    i32 y,
    i32 z,
    Direction direction,
    i32 depth);

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
