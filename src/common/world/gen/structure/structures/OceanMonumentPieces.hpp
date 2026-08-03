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
#include "common/world/gen/structure/Structure.hpp"
#include <memory>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace structure {

// 前向声明
class OceanMonumentRoomDefinition;

/**
 * @brief 海洋纪念碑房间定义
 *
 * 75 个房间形成 5x5x3 的 3D 网格
 */
class OceanMonumentRoomDefinition {
public:
    explicit OceanMonumentRoomDefinition(i32 index);

    [[nodiscard]] i32 getIndex() const { return m_index; }
    [[nodiscard]] bool isClaimed() const { return m_claimed; }
    void setClaimed(bool claimed) { m_claimed = claimed; }
    [[nodiscard]] bool isSource() const { return m_isSource; }
    void setSource(bool source) { m_isSource = source; }
    [[nodiscard]] i32 getScanIndex() const { return m_scanIndex; }
    void setScanIndex(i32 index) { m_scanIndex = index; }

    /**
     * @brief 设置房间连接
     * @param direction 方向 (0-5: DOWN, UP, NORTH, SOUTH, WEST, EAST)
     * @param room 连接的房间
     */
    void setConnection(i32 direction, OceanMonumentRoomDefinition* room);

    /**
     * @brief 获取房间连接
     * @param direction 方向
     * @return 连接的房间，如果没有则返回 nullptr
     */
    [[nodiscard]] OceanMonumentRoomDefinition* getConnection(i32 direction) const;
    [[nodiscard]] const OceanMonumentRoomDefinition* getConnectionConst(i32 direction) const;

    /**
     * @brief 更新开口状态
     * 根据 connections 数组更新 hasOpening 数组
     */
    void updateOpenings();

    /**
     * @brief 查找到源的路径
     * @param scanIndex 当前扫描索引
     * @return 是否能找到源房间
     */
    [[nodiscard]] bool findSource(i32 scanIndex);

    /**
     * @brief 检查是否是特殊房间（索引 >= 75）
     */
    [[nodiscard]] bool isSpecial() const;

    /**
     * @brief 计算开口数量
     */
    [[nodiscard]] i32 countOpenings() const;

    /**
     * @brief 检查指定方向是否有开口
     */
    [[nodiscard]] bool hasOpening(i32 direction) const;
    void setHasOpening(i32 direction, bool opening);

private:
    i32 m_index;
    OceanMonumentRoomDefinition* m_connections[6] = {};
    bool m_hasOpening[6] = {};
    bool m_claimed = false;
    bool m_isSource = false;
    i32 m_scanIndex = -1;
};

/**
 * @brief 海洋纪念碑片段基类
 */
class OceanMonumentPiece : public StructurePiece {
public:
    OceanMonumentPiece(i32 type, Direction direction);
    OceanMonumentPiece(i32 type, Direction direction, const StructureBoundingBox& bounds);

    /**
     * @brief 生成默认地板
     * @param world 世界写入器
     * @param bounds 边界框
     * @param x X 偏移
     * @param z Z 偏移
     * @param hasOpeningDownwards 是否有向下的开口
     */
    void generateDefaultFloor(
        IWorldWriter& world, const StructureBoundingBox& bounds, i32 x, i32 z, bool hasOpeningDownwards);

    /**
     * @brief 仅在水源块处填充
     */
    void generateBoxOnFillOnly(IWorldWriter& world,
        const StructureBoundingBox& bounds,
        i32 minX,
        i32 minY,
        i32 minZ,
        i32 maxX,
        i32 maxY,
        i32 maxZ,
        const BlockState* state);

    /**
     * @brief 创建开口（清除方块形成通道）
     */
    void makeOpening(
        IWorldWriter& world, const StructureBoundingBox& bounds, i32 x1, i32 y1, i32 z1, i32 x2, i32 y2, i32 z2);

    /**
     * @brief 检查区块是否与范围相交
     */
    [[nodiscard]] bool doesChunkIntersect(
        const StructureBoundingBox& chunkBounds, i32 x1, i32 z1, i32 x2, i32 z2) const;

    /**
     * @brief 生成远古守卫者
     * @return 是否成功生成
     */
    bool spawnElderGuardian(IWorldWriter& world, const StructureBoundingBox& bounds, i32 x, i32 y, i32 z);

    void setRoomDefinition(OceanMonumentRoomDefinition* room) { m_roomDefinition = room; }
    [[nodiscard]] OceanMonumentRoomDefinition* getRoomDefinition() { return m_roomDefinition; }
    [[nodiscard]] const OceanMonumentRoomDefinition* getRoomDefinition() const { return m_roomDefinition; }

protected:
    OceanMonumentRoomDefinition* m_roomDefinition = nullptr;

    static constexpr i32 getRoomIndex(i32 x, i32 y, i32 z) { return y * 25 + z * 5 + x; }

    // 静态方块状态常量
    static const BlockState* s_roughPrismarine;
    static const BlockState* s_bricksPrismarine;
    static const BlockState* s_darkPrismarine;
    static const BlockState* s_seaLantern;
    static const BlockState* s_water;

    // 房间索引常量
    static constexpr i32 GRIDROOM_SOURCE_INDEX = 2;
    static constexpr i32 GRIDROOM_TOP_CONNECT_INDEX = 52;
    static constexpr i32 GRIDROOM_LEFTWING_CONNECT_INDEX = 25;
    static constexpr i32 GRIDROOM_RIGHTWING_CONNECT_INDEX = 29;
};

/**
 * @brief 双 X 方向房间（横向扩展）
 */
class OceanMonumentDoubleXRoom : public OceanMonumentPiece {
public:
    OceanMonumentDoubleXRoom(Direction direction, OceanMonumentRoomDefinition* room);
    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;
};

/**
 * @brief 双 XY 方向房间（横向和纵向扩展）
 */
class OceanMonumentDoubleXYRoom : public OceanMonumentPiece {
public:
    OceanMonumentDoubleXYRoom(Direction direction, OceanMonumentRoomDefinition* room);
    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;
};

/**
 * @brief 双 Y 方向房间（纵向扩展）
 */
class OceanMonumentDoubleYRoom : public OceanMonumentPiece {
public:
    OceanMonumentDoubleYRoom(Direction direction, OceanMonumentRoomDefinition* room);
    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;
};

/**
 * @brief 双 YZ 方向房间（纵向和前后扩展）
 */
class OceanMonumentDoubleYZRoom : public OceanMonumentPiece {
public:
    OceanMonumentDoubleYZRoom(Direction direction, OceanMonumentRoomDefinition* room);
    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;
};

/**
 * @brief 双 Z 方向房间（前后扩展）
 */
class OceanMonumentDoubleZRoom : public OceanMonumentPiece {
public:
    OceanMonumentDoubleZRoom(Direction direction, OceanMonumentRoomDefinition* room);
    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;
};

/**
 * @brief 入口房间
 */
class OceanMonumentEntryRoom : public OceanMonumentPiece {
public:
    OceanMonumentEntryRoom(Direction direction, OceanMonumentRoomDefinition* room);
    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;
};

/**
 * @brief 简单房间
 */
class OceanMonumentSimpleRoom : public OceanMonumentPiece {
public:
    OceanMonumentSimpleRoom(Direction direction, OceanMonumentRoomDefinition* room, math::Random& rng);
    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

private:
    i32 m_mainDesign;
};

/**
 * @brief 简单顶层房间（包含海绵）
 */
class OceanMonumentSimpleTopRoom : public OceanMonumentPiece {
public:
    OceanMonumentSimpleTopRoom(Direction direction, OceanMonumentRoomDefinition* room);
    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;
};

/**
 * @brief 核心房间（金块宝藏室）
 */
class OceanMonumentCoreRoom : public OceanMonumentPiece {
public:
    OceanMonumentCoreRoom(Direction direction, OceanMonumentRoomDefinition* room);
    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;
};

/**
 * @brief 阁楼房间（远古守卫者生成点）
 */
class OceanMonumentPenthouse : public OceanMonumentPiece {
public:
    OceanMonumentPenthouse(Direction direction, const StructureBoundingBox& bounds);
    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;
};

/**
 * @brief 翼楼房间
 */
class OceanMonumentWingRoom : public OceanMonumentPiece {
public:
    OceanMonumentWingRoom(Direction direction, const StructureBoundingBox& bounds, i32 design);
    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

private:
    i32 m_mainDesign;
};

/**
 * @brief 海洋纪念碑主体建筑
 *
 * 包含房间图生成和所有子片段的管理
 */
class OceanMonumentBuilding : public OceanMonumentPiece {
public:
    OceanMonumentBuilding(math::Random& rng, i32 x, i32 z, Direction direction);
    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

private:
    /**
     * @brief 生成房间图
     */
    std::vector<OceanMonumentRoomDefinition*> _generateRoomGraph(math::Random& rng);

    /**
     * @brief 生成翼楼
     */
    void _generateWing(
        bool isLeft, i32 startX, IWorldWriter& world, math::Random& rng, const StructureBoundingBox& chunkBounds);

    /**
     * @brief 生成入口拱门
     */
    void _generateEntranceArchs(IWorldWriter& world, math::Random& rng, const StructureBoundingBox& chunkBounds);

    /**
     * @brief 生成入口墙壁
     */
    void _generateEntranceWall(IWorldWriter& world, math::Random& rng, const StructureBoundingBox& chunkBounds);

    /**
     * @brief 生成屋顶部分
     */
    void _generateRoofPiece(IWorldWriter& world, math::Random& rng, const StructureBoundingBox& chunkBounds);

    /**
     * @brief 生成下部墙壁
     */
    void _generateLowerWall(IWorldWriter& world, math::Random& rng, const StructureBoundingBox& chunkBounds);

    /**
     * @brief 生成中部墙壁
     */
    void _generateMiddleWall(IWorldWriter& world, math::Random& rng, const StructureBoundingBox& chunkBounds);

    /**
     * @brief 生成上部墙壁
     */
    void _generateUpperWall(IWorldWriter& world, math::Random& rng, const StructureBoundingBox& chunkBounds);

    OceanMonumentRoomDefinition* m_sourceRoom = nullptr;
    OceanMonumentRoomDefinition* m_coreRoom = nullptr;
    std::vector<std::unique_ptr<OceanMonumentPiece>> m_childPieces;
    std::vector<std::unique_ptr<OceanMonumentRoomDefinition>> m_roomDefinitions;
};

/**
 * @brief 房间匹配辅助器接口
 */
class IMonumentRoomFitHelper {
public:
    virtual ~IMonumentRoomFitHelper() = default;

    /**
     * @brief 检查房间定义是否匹配
     */
    [[nodiscard]] virtual bool fits(OceanMonumentRoomDefinition* definition) = 0;

    /**
     * @brief 创建匹配的房间片段
     */
    [[nodiscard]] virtual OceanMonumentPiece* create(
        Direction direction, OceanMonumentRoomDefinition* room, math::Random& rng) = 0;
};

// 房间匹配辅助器实现
class FitSimpleRoomHelper : public IMonumentRoomFitHelper {
public:
    [[nodiscard]] bool fits(OceanMonumentRoomDefinition* definition) override;
    [[nodiscard]] OceanMonumentPiece* create(
        Direction direction, OceanMonumentRoomDefinition* room, math::Random& rng) override;
};

class FitSimpleRoomTopHelper : public IMonumentRoomFitHelper {
public:
    [[nodiscard]] bool fits(OceanMonumentRoomDefinition* definition) override;
    [[nodiscard]] OceanMonumentPiece* create(
        Direction direction, OceanMonumentRoomDefinition* room, math::Random& rng) override;
};

class XDoubleRoomFitHelper : public IMonumentRoomFitHelper {
public:
    [[nodiscard]] bool fits(OceanMonumentRoomDefinition* definition) override;
    [[nodiscard]] OceanMonumentPiece* create(
        Direction direction, OceanMonumentRoomDefinition* room, math::Random& rng) override;
};

class YDoubleRoomFitHelper : public IMonumentRoomFitHelper {
public:
    [[nodiscard]] bool fits(OceanMonumentRoomDefinition* definition) override;
    [[nodiscard]] OceanMonumentPiece* create(
        Direction direction, OceanMonumentRoomDefinition* room, math::Random& rng) override;
};

class ZDoubleRoomFitHelper : public IMonumentRoomFitHelper {
public:
    [[nodiscard]] bool fits(OceanMonumentRoomDefinition* definition) override;
    [[nodiscard]] OceanMonumentPiece* create(
        Direction direction, OceanMonumentRoomDefinition* room, math::Random& rng) override;
};

class XYDoubleRoomFitHelper : public IMonumentRoomFitHelper {
public:
    [[nodiscard]] bool fits(OceanMonumentRoomDefinition* definition) override;
    [[nodiscard]] OceanMonumentPiece* create(
        Direction direction, OceanMonumentRoomDefinition* room, math::Random& rng) override;
};

class YZDoubleRoomFitHelper : public IMonumentRoomFitHelper {
public:
    [[nodiscard]] bool fits(OceanMonumentRoomDefinition* definition) override;
    [[nodiscard]] OceanMonumentPiece* create(
        Direction direction, OceanMonumentRoomDefinition* room, math::Random& rng) override;
};

// 片段类型常量
namespace OceanMonumentPieceTypes {
constexpr i32 DOUBLE_X_ROOM = 200;
constexpr i32 DOUBLE_XY_ROOM = 201;
constexpr i32 DOUBLE_Y_ROOM = 202;
constexpr i32 DOUBLE_YZ_ROOM = 203;
constexpr i32 DOUBLE_Z_ROOM = 204;
constexpr i32 ENTRY_ROOM = 205;
constexpr i32 SIMPLE_ROOM = 206;
constexpr i32 SIMPLE_TOP_ROOM = 207;
constexpr i32 CORE_ROOM = 208;
constexpr i32 PENTHOUSE = 209;
constexpr i32 WING_ROOM = 210;
constexpr i32 MONUMENT_BUILDING = 211;
} // namespace OceanMonumentPieceTypes

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
