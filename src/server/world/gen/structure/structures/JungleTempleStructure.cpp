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

#include "JungleTempleStructure.hpp"

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorldWriter.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeTag.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include <memory>
#include <string>
#include <utility>

namespace mc {
namespace world {
namespace gen {
namespace structure {

// 匿名命名空间：文件内部常量和辅助函数
namespace {

using namespace mc::Biomes;

// 丛林神庙尺寸常量
constexpr i32 TEMPLE_WIDTH = 12;      // X轴宽度
constexpr i32 TEMPLE_LENGTH = 15;     // Z轴深度
constexpr i32 TEMPLE_HEIGHT = 10;     // 主结构高度
constexpr i32 TOWER_EXTRA_HEIGHT = 4; // 塔楼额外高度
constexpr i32 ENTRANCE_STEPS = 3;     // 入口台阶级数（每级向外、向下一格）

// 最低生成高度（低于此高度时不生成）
constexpr i32 MIN_GENERATION_HEIGHT = 60;
constexpr i32 FALLBACK_HEIGHT = 64;

// 藤蔓装饰数量
constexpr i32 VINE_DECORATION_COUNT = 20;

} // namespace

// ============================================================================
// JungleTemplePiece
// ============================================================================

JungleTemplePiece::JungleTemplePiece(const BlockPos& pos, Direction direction)
    : StructurePiece(StructurePieceTypes::JUNGLE_TEMPLE, pos.x, pos.y, pos.z, pos.x, pos.y, pos.z)
    , m_startPos(pos)
{
    // 主体脚印沿 Z 时是 12x15、沿 X 时展开成 15x12，必须按朝向展开。
    const i32 width = TEMPLE_WIDTH;
    const i32 height = TEMPLE_HEIGHT;
    const i32 depth = TEMPLE_LENGTH;
    const bool alongZ = direction == Direction::North || direction == Direction::South;

    // 记下朝向：_mapLocalX/_mapLocalZ 与包围盒都按它决定局部坐标的映射方式。
    setCoordBaseMode(direction);

    // 布局用结构内局部坐标书写，世界坐标由 _generateTemple 顶部的 place() 按朝向映射得到；
    // 包围盒在这里用同一套映射反推，所以两者不可能脱节。
    constexpr i32 localMinX = 0;
    const i32 localMaxX = width - 1;
    constexpr i32 localMinZ = -ENTRANCE_STEPS;
    const i32 localMaxZ = depth - 1;
    constexpr i32 localMinY = -ENTRANCE_STEPS;
    const i32 localMaxY = height - 1 + TOWER_EXTRA_HEIGHT + 1;

    // 布局「正面」是局部 -Z 一侧。各朝向把局部 (x,z) 映射到世界的规则：
    //   North: (pos.x + x,           pos.z - z)
    //   South: (pos.x + x,           pos.z + z)
    //   West:  (pos.x + (depth-1-z), pos.z + x)
    //   East:  (pos.x + z,           pos.z + x)
    // 用这四个角求包围盒，正好覆盖布局可能写到的全部世界坐标。

    i32 minX = _mapLocalX(localMinX, localMinZ);
    i32 maxX = minX;
    i32 minZ = _mapLocalZ(localMinX, localMinZ);
    i32 maxZ = minZ;
    for (const auto& [localX, localZ] : std::array<std::pair<i32, i32>, 4>{std::pair{localMinX, localMinZ},
             std::pair{localMinX, localMaxZ},
             std::pair{localMaxX, localMinZ},
             std::pair{localMaxX, localMaxZ}}) {
        minX = std::min(minX, _mapLocalX(localX, localZ));
        maxX = std::max(maxX, _mapLocalX(localX, localZ));
        minZ = std::min(minZ, _mapLocalZ(localX, localZ));
        maxZ = std::max(maxZ, _mapLocalZ(localX, localZ));
    }

    m_minX = minX;
    m_minY = pos.y + localMinY;
    m_minZ = minZ;
    m_maxX = maxX;
    m_maxY = pos.y + localMaxY;
    m_maxZ = maxZ;
}

i32 JungleTemplePiece::_mapLocalX(i32 localX, i32 localZ) const
{
    switch (getCoordBaseMode()) {
        case Direction::West:
            // 局部 Z 走世界 X 轴，且反向（正面朝 -X）。
            return m_startPos.x + (TEMPLE_LENGTH - 1) - localZ;
        case Direction::East:
            return m_startPos.x + localZ;
        default:
            return m_startPos.x + localX;
    }
}

i32 JungleTemplePiece::_mapLocalZ(i32 localX, i32 localZ) const
{
    switch (getCoordBaseMode()) {
        case Direction::North:
            // 正面朝 -Z：局部 Z 越大越靠近 -Z。
            return m_startPos.z - localZ;
        case Direction::West:
        case Direction::East:
            return m_startPos.z + localX;
        default:
            return m_startPos.z + localZ;
    }
}

void JungleTemplePiece::generate(IWorldWriter& world,
    math::IRandom& rng,
    i32 /*chunkX*/,
    i32 /*chunkZ*/,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    if (!getBoundingBox().intersects(chunkBounds)) {
        return;
    }
    _generateTemple(world, rng, chunkBounds);
}

void JungleTemplePiece::_generateTemple(IWorldWriter& world, math::IRandom& rng, const StructureBoundingBox& bounds)
{
    // 布局按局部坐标书写，这里按朝向映射成世界坐标后直接写入。
    // 不用 StructurePiece::setBlockState：它对局部坐标再做一次 getXWithOffset/getZWithOffset，
    // 会让包围盒与真实写入位置脱节；映射只应发生在一处。
    const auto place = [&](const BlockState* state, i32 localX, i32 localY, i32 localZ) {
        if (state == nullptr) {
            return;
        }
        const i32 worldX = _mapLocalX(localX, localZ);
        const i32 worldY = m_startPos.y + localY;
        const i32 worldZ = _mapLocalZ(localX, localZ);
        if (!bounds.contains(worldX, worldY, worldZ)) {
            return;
        }
        world.setBlockState(worldX, worldY, worldZ, state, world::BlockUpdateFlags::UPDATE_CLIENTS);
    };

    const BlockState* cobblestone = VanillaBlocks::getState(VanillaBlocks::COBBLESTONE);
    const BlockState* mossyCobblestone = VanillaBlocks::getState(VanillaBlocks::MOSSY_COBBLESTONE);
    const BlockState* stoneBricks = VanillaBlocks::getState(VanillaBlocks::STONE_BRICKS);
    const BlockState* mossyStoneBricks = VanillaBlocks::getState(VanillaBlocks::MOSSY_STONE_BRICKS);
    const BlockState* chiseledStoneBricks = VanillaBlocks::getState(VanillaBlocks::CHISELED_STONE_BRICKS);
    const BlockState* vine = VanillaBlocks::getState(VanillaBlocks::VINE);
    const BlockState* air = VanillaBlocks::getState(VanillaBlocks::AIR);
    const BlockState* tripwire = VanillaBlocks::getState(VanillaBlocks::TRIPWIRE);
    const BlockState* tripwireHook = VanillaBlocks::getState(VanillaBlocks::TRIPWIRE_HOOK);

    // 布局一律用**结构内局部坐标**书写（x 沿宽度、z 沿长度、y 自地板起算），
    // 由 setBlockState 叠加 getXWithOffset/getZWithOffset 的镜像与旋转映射到世界坐标。
    // 此前这里直接写 baseX + x / baseZ + z，绕过了朝向变换，朝向为东西向时整座神庙
    // 仍按 +X/+Z 展开，与包围盒错位。
    i32 width = TEMPLE_WIDTH;
    i32 length = TEMPLE_LENGTH;
    i32 height = TEMPLE_HEIGHT;

    // 随机选择苔石或普通圆石
    auto randomCobble = [&]() -> const BlockState* {
        return rng.nextInt(100) < 30 ? mossyCobblestone : cobblestone;
    };

    // 随机选择石砖类型
    auto randomBrick = [&]() -> const BlockState* {
        i32 r = rng.nextInt(100);
        if (r < 50) {
            return stoneBricks;
        }
        if (r < 80) {
            return mossyStoneBricks;
        }
        return chiseledStoneBricks;
    };

    // 生成地板（两层）
    for (i32 y = 0; y < 2; ++y) {
        for (i32 x = 0; x < width; ++x) {
            for (i32 z = 0; z < length; ++z) {
                place(randomCobble(), x, y, z);
            }
        }
    }

    // 生成外墙
    for (i32 y = 2; y < height; ++y) {
        for (i32 x = 0; x < width; ++x) {
            // 前后墙
            {
                place(randomBrick(), x, y, 0);
            }
            {
                place(randomBrick(), x, y, length - 1);
            }
        }
        for (i32 z = 0; z < length; ++z) {
            // 左右墙
            {
                place(randomBrick(), 0, y, z);
            }
            {
                place(randomBrick(), width - 1, y, z);
            }
        }
    }

    // 生成入口（南面中央）
    i32 entranceX = width / 2;
    for (i32 y = 2; y < 5; ++y) {
        {
            place(air, entranceX - 1, y, 0);
        }
        {
            place(air, entranceX, y, 0);
        }
        {
            place(air, entranceX + 1, y, 0);
        }
    }

    // 入口台阶
    for (i32 step = 0; step < 3; ++step) {
        for (i32 x = entranceX - 1; x <= entranceX + 1; ++x) {
            place(cobblestone, x, 2 - step, 0 - step - 1);
        }
    }

    // 内部地板
    for (i32 x = 1; x < width - 1; ++x) {
        for (i32 z = 1; z < length - 1; ++z) {
            place(stoneBricks, x, 2, z);
        }
    }

    // 中央走廊 - 被墙分隔
    i32 corridorZ = length / 2;
    for (i32 x = 1; x < width - 1; ++x) {
        {
            place(stoneBricks, x, 3, corridorZ);
        }
        {
            place(stoneBricks, x, 4, corridorZ);
        }
    }

    // 走廊门洞
    {
        place(air, entranceX, 3, corridorZ);
    }
    {
        place(air, entranceX, 4, corridorZ);
    }

    // 拉杆谜题房间（西侧）
    i32 puzzleX = 2;
    i32 puzzleZ = 3;
    // 隐藏机关墙
    for (i32 y = 3; y < 6; ++y) {
        {
            place(chiseledStoneBricks, puzzleX, y, puzzleZ);
        }
        {
            place(chiseledStoneBricks, puzzleX + 1, y, puzzleZ);
        }
    }

    // 箭矢陷阱走廊（东侧）
    i32 trapX = width - 3;
    for (i32 z = 2; z < 6; ++z) {
        {
            place(air, trapX, 3, z);
        }
        {
            place(air, trapX + 1, 3, z);
        }
    }

    // 绊线陷阱
    if (tripwire && tripwireHook) {
        // 绊线钩在墙上
        {
            place(tripwireHook, trapX + 2, 3, 3);
        }
        {
            place(tripwireHook, trapX + 2, 3, 4);
        }

        // 绊线连接
        for (i32 z = 3; z <= 4; ++z) {
            {
                place(tripwire, trapX, 3, z);
            }
            {
                place(tripwire, trapX + 1, 3, z);
            }
        }
    }

    // 发射器陷阱（天花板）
    // 丛林神庙有两个发射器陷阱，朝下射击箭矢
    // 战利品表 minecraft:chests/jungle_temple_dispenser 包含 2-7 支箭
    // generateDispenser 会同时放置方块和设置战利品表
    {
        // 第一组发射器（走廊上方前侧）
        generateDispenser(world,
            bounds,
            rng,
            trapX,
            5,
            3,
            Direction::Down,
            ResourceLocation("minecraft", "chests/jungle_temple_dispenser"));
        // 第二组发射器（走廊上方后侧）
        generateDispenser(world,
            bounds,
            rng,
            trapX,
            5,
            4,
            Direction::Down,
            ResourceLocation("minecraft", "chests/jungle_temple_dispenser"));
    }

    // 宝箱房间（北侧，隐藏）
    i32 chestX = width / 2;
    i32 chestZ = length - 3;

    // 宝箱房间入口（需要机关开启）
    for (i32 y = 3; y < 6; ++y) {
        {
            place(mossyStoneBricks, chestX - 1, y, chestZ);
        }
        {
            place(mossyStoneBricks, chestX, y, chestZ);
        }
        {
            place(mossyStoneBricks, chestX + 1, y, chestZ);
        }
    }

    // 宝箱房间内部
    for (i32 x = chestX - 1; x <= chestX + 1; ++x) {
        for (i32 z = chestZ + 1; z < length - 1; ++z) {
            place(stoneBricks, x, 3, z);
        }
    }

    // 宝箱（高概率生成）
    // 丛林神庙宝箱使用 minecraft:chests/jungle_temple 战利品表
    if (rng.nextInt(100) < 70) {
        // 宝箱朝向南面（入口方向）
        generateChest(world,
            bounds,
            rng,
            chestX,
            3,
            chestZ + 2,
            Direction::South,
            ResourceLocation("minecraft", "chests/jungle_temple"));
    }

    // 三层塔楼（四角）
    for (i32 corner = 0; corner < 4; ++corner) {
        i32 cx = (corner % 2 == 0) ? 0 : width - 1;
        i32 cz = (corner < 2) ? 0 : length - 1;

        // 塔楼高度额外增加
        for (i32 y = height; y < height + TOWER_EXTRA_HEIGHT; ++y) {
            place(randomBrick(), cx, y, cz);
        }

        // 塔楼顶部装饰
        {
            place(chiseledStoneBricks, cx, height + TOWER_EXTRA_HEIGHT, cz);
        }
    }

    // 屋顶平台
    for (i32 x = 1; x < width - 1; ++x) {
        for (i32 z = 1; z < length - 1; ++z) {
            place(stoneBricks, x, height, z);
        }
    }

    // 添加藤蔓装饰（外墙）
    for (i32 i = 0; i < VINE_DECORATION_COUNT; ++i) {
        i32 vx = rng.nextInt(width);
        i32 vz = rng.nextInt(length);
        i32 vy = rng.nextInt(height - 2) + 2;

        // 选择一面墙
        i32 side = rng.nextInt(4);
        i32 wx, wz;
        if (side == 0) {
            wx = 0;
            wz = vz;
        } else if (side == 1) {
            wx = width - 1;
            wz = vz;
        } else if (side == 2) {
            wx = vx;
            wz = 0;
        } else {
            wx = vx;
            wz = length - 1;
        }

        // 放置藤蔓
        if (rng.nextInt(100) < 40) {
            place(vine, wx, vy, wz);
        }
    }
}

// ============================================================================
// JungleTempleStructure
// ============================================================================

const std::string JungleTempleStructure::m_name = "jungle_temple";

JungleTempleStructure::JungleTempleStructure(ResourceLocation id) noexcept
    : Structure(std::move(id))
{}

const biome::BiomeTag* JungleTempleStructure::defaultBiomeTag() const
{
    return &biome::BiomeTags::HAS_STRUCTURE_JUNGLE_PYRAMID();
}

std::unique_ptr<StructureStart> JungleTempleStructure::generate(
    IChunkGenerator& generator, math::IRandom& rng, i32 chunkX, i32 chunkZ) const
{
    auto start = makeStructureStart(chunkX, chunkZ);

    // `SinglePieceStructure.findGenerationPoint` 的三步，顺序不可换：
    //   1. 以区块最小角展开 12x15 方框取四角最低地表，低于海平面直接放弃；
    //   2. 在候选生成点（区块中心 + 世界表面高度）校验生物群系；
    //   3. `generatePieces` 只做一件事：抽一次水平朝向并建构件（原点 = 区块最小角、Y=64）。
    const i32 minX = chunkX * CHUNK_WIDTH;
    const i32 minZ = chunkZ * CHUNK_WIDTH;
    const i32 lowest = std::min({generator.getHeight(minX, minZ, HeightmapType::WorldSurfaceWG),
        generator.getHeight(minX + TEMPLE_WIDTH, minZ, HeightmapType::WorldSurfaceWG),
        generator.getHeight(minX, minZ + TEMPLE_LENGTH, HeightmapType::WorldSurfaceWG),
        generator.getHeight(minX + TEMPLE_WIDTH, minZ + TEMPLE_LENGTH, HeightmapType::WorldSurfaceWG)});
    if (lowest < generator.seaLevel()) {
        return start;
    }
    if (!isValidBiomeAt(generator, onTopOfChunkCenter(generator, chunkX, chunkZ, HeightmapType::WorldSurfaceWG))) {
        return start;
    }

    // 原版 `Direction.Plane.HORIZONTAL.getRandomDirection` 的候选顺序是
    // NORTH, EAST, SOUTH, WEST（与本项目 Direction 的枚举值不同，映射必须显式写）。
    constexpr Direction kHorizontalFaces[] = {Direction::North, Direction::East, Direction::South, Direction::West};
    const Direction direction = kHorizontalFaces[rng.nextInt(4)];

    auto piece = std::make_unique<JungleTemplePiece>(BlockPos(minX, 64, minZ), direction);
    start->addPiece(std::move(piece));
    start->recalculateStructureSize();

    return start;
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
