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

#include "../../feature/template/Template.hpp"
#include "../Structure.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/biome/BiomeTag.hpp"
#include <memory>
#include <string>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace structure {

// Forward declarations
namespace woodland_mansion {
class SimpleGrid;
class MansionGrid;
class MansionPlacer;
} // namespace woodland_mansion

/**
 * @brief 林地府邸结构
 *
 * 在黑森林生物群系生成的大型府邸结构。
 * 使用递归走廊生成算法创建复杂房间布局。
 * 包含掠夺者和唤魔者。
 */
class WoodlandMansionStructure : public Structure {
public:
    explicit WoodlandMansionStructure(ResourceLocation id);
    ~WoodlandMansionStructure() override = default;

    [[nodiscard]] const std::string& name() const override { return s_name; }

    /**
     * @brief 获取结构关联的生物群系标签
     */
    [[nodiscard]] const biome::BiomeTag* defaultBiomeTag() const override;

    [[nodiscard]] bool canGenerate(
        IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) override;

    [[nodiscard]] std::unique_ptr<StructureStart> generate(
        IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const override;

private:
    static const std::string s_name;
};

/**
 * @brief 林地府邸模板片段
 */
class WoodlandMansionPiece : public StructurePiece {
public:
    WoodlandMansionPiece(const std::string& templateName,
        const BlockPos& pos,
        feature::template_::Rotation rotation,
        feature::template_::Mirror mirror = feature::template_::Mirror::None);

    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

    [[nodiscard]] const std::string& templateName() const { return m_templateName; }
    [[nodiscard]] feature::template_::Rotation rotation() const { return m_rotation; }
    [[nodiscard]] feature::template_::Mirror mirror() const { return m_mirror; }

private:
    std::string m_templateName;
    BlockPos m_templatePosition;
    feature::template_::Rotation m_rotation;
    feature::template_::Mirror m_mirror;
};

namespace woodland_mansion {

/**
 * @brief 简单二维网格
 */
class SimpleGrid {
public:
    SimpleGrid(i32 width, i32 height, i32 valueIfOutside);
    ~SimpleGrid() = default;

    void set(i32 x, i32 y, i32 value);
    void set(i32 x1, i32 y1, i32 x2, i32 y2, i32 value);
    [[nodiscard]] i32 get(i32 x, i32 y) const;
    void setIf(i32 x, i32 y, i32 oldValue, i32 newValue);
    [[nodiscard]] bool edgesTo(i32 x, i32 y, i32 value) const;

    [[nodiscard]] i32 width() const { return m_width; }
    [[nodiscard]] i32 height() const { return m_height; }

private:
    std::vector<std::vector<i32>> m_grid;
    i32 m_width;
    i32 m_height;
    i32 m_valueIfOutside;
};

/**
 * @brief 林地府邸布局网格生成器
 *
 * 使用递归走廊算法生成府邸房间布局。
 */
class MansionGrid {
public:
    explicit MansionGrid(math::Random& rng);
    ~MansionGrid() = default;

    [[nodiscard]] static bool isHouse(const SimpleGrid& grid, i32 x, i32 y);
    [[nodiscard]] bool isRoomId(const SimpleGrid& grid, i32 x, i32 y, i32 floor, i32 roomId) const;
    [[nodiscard]] Direction get1x2RoomDirection(const SimpleGrid& grid, i32 x, i32 y, i32 floor, i32 roomId) const;

    [[nodiscard]] const SimpleGrid& baseGrid() const { return *m_baseGrid; }
    [[nodiscard]] const SimpleGrid& thirdFloorGrid() const { return *m_thirdFloorGrid; }
    [[nodiscard]] const SimpleGrid& floorRoom(i32 floor) const { return *m_floorRooms[floor]; }
    [[nodiscard]] i32 entranceX() const { return m_entranceX; }
    [[nodiscard]] i32 entranceY() const { return m_entranceY; }

private:
    void _recursiveCorridor(SimpleGrid& grid, i32 x, i32 y, Direction dir, i32 depth);
    [[nodiscard]] bool _cleanEdges(SimpleGrid& grid);
    void _identifyRooms(const SimpleGrid& sourceGrid, SimpleGrid& roomGrid);
    void _setupThirdFloor();

    /// 房间网格位标志：0x10000=1x1, 0x20000=1x2, 0x40000=2x2, 0x100000=门位置, 0x200000=走廊入口, 0x400000=楼梯,
    /// 0x800000=楼梯入口

    math::Random& m_rng;
    std::unique_ptr<SimpleGrid> m_baseGrid;
    std::unique_ptr<SimpleGrid> m_thirdFloorGrid;
    std::unique_ptr<SimpleGrid> m_floorRooms[3];
    i32 m_entranceX;
    i32 m_entranceY;
};

/**
 * @brief 林地府邸放置器
 *
 * 根据网格布局放置模板片段。
 */
class MansionPlacer {
public:
    MansionPlacer(math::Random& rng);
    ~MansionPlacer() = default;

    void createMansion(const BlockPos& startPos,
        feature::template_::Rotation rotation,
        std::vector<std::unique_ptr<StructurePiece>>& pieces,
        const MansionGrid& grid);

private:
    void _entrance(
        std::vector<std::unique_ptr<StructurePiece>>& pieces, feature::template_::Rotation& rotation, BlockPos& pos);

    void _traverseOuterWalls(std::vector<std::unique_ptr<StructurePiece>>& pieces,
        const SimpleGrid& grid,
        Direction startDir,
        i32 startX,
        i32 startY,
        i32 targetX,
        i32 targetY,
        BlockPos& pos,
        feature::template_::Rotation& rotation,
        const std::string& wallType);

    void _traverseWallPiece(std::vector<std::unique_ptr<StructurePiece>>& pieces,
        BlockPos& pos,
        feature::template_::Rotation rotation,
        const std::string& wallType);

    void _traverseTurn(
        std::vector<std::unique_ptr<StructurePiece>>& pieces, BlockPos& pos, feature::template_::Rotation& rotation);

    void _traverseInnerTurn(
        std::vector<std::unique_ptr<StructurePiece>>& pieces, BlockPos& pos, feature::template_::Rotation& rotation);

    void _createRoof(std::vector<std::unique_ptr<StructurePiece>>& pieces,
        const BlockPos& basePos,
        feature::template_::Rotation rotation,
        const SimpleGrid& grid,
        const SimpleGrid* upperGrid);

    void _addRoom1x1(std::vector<std::unique_ptr<StructurePiece>>& pieces,
        const BlockPos& pos,
        feature::template_::Rotation rotation,
        Direction doorDir,
        i32 floor);

    void _addRoom1x2(std::vector<std::unique_ptr<StructurePiece>>& pieces,
        const BlockPos& pos,
        feature::template_::Rotation rotation,
        Direction roomDir,
        Direction doorDir,
        i32 floor,
        bool isStairs);

    void _addRoom2x2(std::vector<std::unique_ptr<StructurePiece>>& pieces,
        const BlockPos& pos,
        feature::template_::Rotation rotation,
        Direction roomDir,
        Direction doorDir,
        i32 floor);

    void _addRoom2x2Secret(std::vector<std::unique_ptr<StructurePiece>>& pieces,
        const BlockPos& pos,
        feature::template_::Rotation rotation,
        i32 floor);

    math::Random& m_rng;
    i32 m_startX;
    i32 m_startY;
};

/**
 * @brief 房间模板选择器
 *
 * 根据楼层和房间类型选择合适的模板。
 */
class RoomCollection {
public:
    virtual ~RoomCollection() = default;

    [[nodiscard]] virtual std::string get1x1(math::Random& rng) const = 0;
    [[nodiscard]] virtual std::string get1x1Secret(math::Random& rng) const = 0;
    [[nodiscard]] virtual std::string get1x2SideEntrance(math::Random& rng, bool isStairs) const = 0;
    [[nodiscard]] virtual std::string get1x2FrontEntrance(math::Random& rng, bool isStairs) const = 0;
    [[nodiscard]] virtual std::string get1x2Secret(math::Random& rng) const = 0;
    [[nodiscard]] virtual std::string get2x2(math::Random& rng) const = 0;
    [[nodiscard]] virtual std::string get2x2Secret(math::Random& rng) const = 0;
};

/**
 * @brief 一楼房间模板
 */
class FirstFloorRoomCollection : public RoomCollection {
public:
    [[nodiscard]] std::string get1x1(math::Random& rng) const override;
    [[nodiscard]] std::string get1x1Secret(math::Random& rng) const override;
    [[nodiscard]] std::string get1x2SideEntrance(math::Random& rng, bool isStairs) const override;
    [[nodiscard]] std::string get1x2FrontEntrance(math::Random& rng, bool isStairs) const override;
    [[nodiscard]] std::string get1x2Secret(math::Random& rng) const override;
    [[nodiscard]] std::string get2x2(math::Random& rng) const override;
    [[nodiscard]] std::string get2x2Secret(math::Random& rng) const override;
};

/**
 * @brief 二楼房间模板
 */
class SecondFloorRoomCollection : public RoomCollection {
public:
    [[nodiscard]] std::string get1x1(math::Random& rng) const override;
    [[nodiscard]] std::string get1x1Secret(math::Random& rng) const override;
    [[nodiscard]] std::string get1x2SideEntrance(math::Random& rng, bool isStairs) const override;
    [[nodiscard]] std::string get1x2FrontEntrance(math::Random& rng, bool isStairs) const override;
    [[nodiscard]] std::string get1x2Secret(math::Random& rng) const override;
    [[nodiscard]] std::string get2x2(math::Random& rng) const override;
    [[nodiscard]] std::string get2x2Secret(math::Random& rng) const override;
};

/**
 * @brief 三楼房间模板（与二楼相同）
 */
class ThirdFloorRoomCollection : public SecondFloorRoomCollection {
    // 三楼房间与二楼相同，继承即可
};

} // namespace woodland_mansion

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
