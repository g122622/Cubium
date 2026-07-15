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

#include "../../../../core/Constants.hpp"
#include "../../../../resource/ResourceLocation.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../IWorldWriter.hpp"
#include "../../../biome/BiomeIds.hpp"
#include "../../../biome/BiomeTags.hpp"
#include "../../../block/BlockPos.hpp"
#include "../../../block/registry/VanillaBlocks.hpp"
#include "../Structure.hpp"

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

// 最低生成高度（低于此高度时不生成）
constexpr i32 MIN_GENERATION_HEIGHT = 60;
constexpr i32 FALLBACK_HEIGHT = 64;

// 藤蔓装饰数量
constexpr i32 VINE_DECORATION_COUNT = 20;

} // namespace

// ============================================================================
// JungleTemplePiece
// ============================================================================

JungleTemplePiece::JungleTemplePiece(const BlockPos& pos)
    : StructurePiece(StructurePieceTypes::JUNGLE_TEMPLE, pos.x, pos.y, pos.z, pos.x + 11, pos.y + 13, pos.z + 14)
    , m_startPos(pos)
{}

void JungleTemplePiece::generate(IWorldWriter& world,
    math::Random& rng,
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

void JungleTemplePiece::_generateTemple(IWorldWriter& world, math::Random& rng, const StructureBoundingBox& bounds)
{
    const BlockState* cobblestone = VanillaBlocks::getState(VanillaBlocks::COBBLESTONE);
    const BlockState* mossyCobblestone = VanillaBlocks::getState(VanillaBlocks::MOSSY_COBBLESTONE);
    const BlockState* stoneBricks = VanillaBlocks::getState(VanillaBlocks::STONE_BRICKS);
    const BlockState* mossyStoneBricks = VanillaBlocks::getState(VanillaBlocks::MOSSY_STONE_BRICKS);
    const BlockState* chiseledStoneBricks = VanillaBlocks::getState(VanillaBlocks::CHISELED_STONE_BRICKS);
    const BlockState* vine = VanillaBlocks::getState(VanillaBlocks::VINE);
    const BlockState* air = VanillaBlocks::getState(VanillaBlocks::AIR);
    const BlockState* tripwire = VanillaBlocks::getState(VanillaBlocks::TRIPWIRE);
    const BlockState* tripwireHook = VanillaBlocks::getState(VanillaBlocks::TRIPWIRE_HOOK);

    // 基础参数
    i32 baseX = m_startPos.x;
    i32 baseY = m_startPos.y;
    i32 baseZ = m_startPos.z;
    i32 width = TEMPLE_WIDTH;
    i32 length = TEMPLE_LENGTH;
    i32 height = TEMPLE_HEIGHT;

    // 随机选择苔石或普通圆石
    auto randomCobble = [&]() -> const BlockState* { return rng.nextInt(100) < 30 ? mossyCobblestone : cobblestone; };

    // 随机选择石砖类型
    auto randomBrick = [&]() -> const BlockState* {
        i32 r = rng.nextInt(100);
        if (r < 50) return stoneBricks;
        if (r < 80) return mossyStoneBricks;
        return chiseledStoneBricks;
    };

    // 生成地板（两层）
    for (i32 y = 0; y < 2; ++y) {
        for (i32 x = 0; x < width; ++x) {
            for (i32 z = 0; z < length; ++z) {
                i32 wx = baseX + x, wy = baseY + y, wz = baseZ + z;
                if (bounds.contains(wx, wy, wz)) {
                    world.setBlockState(wx, wy, wz, randomCobble(), 18);
                }
            }
        }
    }

    // 生成外墙
    for (i32 y = 2; y < height; ++y) {
        for (i32 x = 0; x < width; ++x) {
            // 前后墙
            {
                i32 wx = baseX + x, wy = baseY + y, wz = baseZ;
                if (bounds.contains(wx, wy, wz)) {
                    world.setBlockState(wx, wy, wz, randomBrick(), 18);
                }
            }
            {
                i32 wx = baseX + x, wy = baseY + y, wz = baseZ + length - 1;
                if (bounds.contains(wx, wy, wz)) {
                    world.setBlockState(wx, wy, wz, randomBrick(), 18);
                }
            }
        }
        for (i32 z = 0; z < length; ++z) {
            // 左右墙
            {
                i32 wx = baseX, wy = baseY + y, wz = baseZ + z;
                if (bounds.contains(wx, wy, wz)) {
                    world.setBlockState(wx, wy, wz, randomBrick(), 18);
                }
            }
            {
                i32 wx = baseX + width - 1, wy = baseY + y, wz = baseZ + z;
                if (bounds.contains(wx, wy, wz)) {
                    world.setBlockState(wx, wy, wz, randomBrick(), 18);
                }
            }
        }
    }

    // 生成入口（南面中央）
    i32 entranceX = width / 2;
    for (i32 y = 2; y < 5; ++y) {
        {
            i32 wx = baseX + entranceX - 1, wy = baseY + y, wz = baseZ;
            if (bounds.contains(wx, wy, wz)) {
                world.setBlockState(wx, wy, wz, air, 18);
            }
        }
        {
            i32 wx = baseX + entranceX, wy = baseY + y, wz = baseZ;
            if (bounds.contains(wx, wy, wz)) {
                world.setBlockState(wx, wy, wz, air, 18);
            }
        }
        {
            i32 wx = baseX + entranceX + 1, wy = baseY + y, wz = baseZ;
            if (bounds.contains(wx, wy, wz)) {
                world.setBlockState(wx, wy, wz, air, 18);
            }
        }
    }

    // 入口台阶
    for (i32 step = 0; step < 3; ++step) {
        for (i32 x = entranceX - 1; x <= entranceX + 1; ++x) {
            i32 wx = baseX + x, wy = baseY + 2 - step, wz = baseZ - step - 1;
            if (bounds.contains(wx, wy, wz)) {
                world.setBlockState(wx, wy, wz, cobblestone, 18);
            }
        }
    }

    // 内部地板
    for (i32 x = 1; x < width - 1; ++x) {
        for (i32 z = 1; z < length - 1; ++z) {
            i32 wx = baseX + x, wy = baseY + 2, wz = baseZ + z;
            if (bounds.contains(wx, wy, wz)) {
                world.setBlockState(wx, wy, wz, stoneBricks, 18);
            }
        }
    }

    // 中央走廊 - 被墙分隔
    i32 corridorZ = length / 2;
    for (i32 x = 1; x < width - 1; ++x) {
        {
            i32 wx = baseX + x, wy = baseY + 3, wz = baseZ + corridorZ;
            if (bounds.contains(wx, wy, wz)) {
                world.setBlockState(wx, wy, wz, stoneBricks, 18);
            }
        }
        {
            i32 wx = baseX + x, wy = baseY + 4, wz = baseZ + corridorZ;
            if (bounds.contains(wx, wy, wz)) {
                world.setBlockState(wx, wy, wz, stoneBricks, 18);
            }
        }
    }

    // 走廊门洞
    {
        i32 wx = baseX + entranceX, wy = baseY + 3, wz = baseZ + corridorZ;
        if (bounds.contains(wx, wy, wz)) {
            world.setBlockState(wx, wy, wz, air, 18);
        }
    }
    {
        i32 wx = baseX + entranceX, wy = baseY + 4, wz = baseZ + corridorZ;
        if (bounds.contains(wx, wy, wz)) {
            world.setBlockState(wx, wy, wz, air, 18);
        }
    }

    // 拉杆谜题房间（西侧）
    i32 puzzleX = 2;
    i32 puzzleZ = 3;
    // 隐藏机关墙
    for (i32 y = 3; y < 6; ++y) {
        {
            i32 wx = baseX + puzzleX, wy = baseY + y, wz = baseZ + puzzleZ;
            if (bounds.contains(wx, wy, wz)) {
                world.setBlockState(wx, wy, wz, chiseledStoneBricks, 18);
            }
        }
        {
            i32 wx = baseX + puzzleX + 1, wy = baseY + y, wz = baseZ + puzzleZ;
            if (bounds.contains(wx, wy, wz)) {
                world.setBlockState(wx, wy, wz, chiseledStoneBricks, 18);
            }
        }
    }

    // 箭矢陷阱走廊（东侧）
    i32 trapX = width - 3;
    for (i32 z = 2; z < 6; ++z) {
        {
            i32 wx = baseX + trapX, wy = baseY + 3, wz = baseZ + z;
            if (bounds.contains(wx, wy, wz)) {
                world.setBlockState(wx, wy, wz, air, 18);
            }
        }
        {
            i32 wx = baseX + trapX + 1, wy = baseY + 3, wz = baseZ + z;
            if (bounds.contains(wx, wy, wz)) {
                world.setBlockState(wx, wy, wz, air, 18);
            }
        }
    }

    // 绊线陷阱
    if (tripwire && tripwireHook) {
        // 绊线钩在墙上
        {
            i32 wx = baseX + trapX + 2, wy = baseY + 3, wz = baseZ + 3;
            if (bounds.contains(wx, wy, wz)) {
                world.setBlockState(wx, wy, wz, tripwireHook, 18);
            }
        }
        {
            i32 wx = baseX + trapX + 2, wy = baseY + 3, wz = baseZ + 4;
            if (bounds.contains(wx, wy, wz)) {
                world.setBlockState(wx, wy, wz, tripwireHook, 18);
            }
        }

        // 绊线连接
        for (i32 z = 3; z <= 4; ++z) {
            {
                i32 wx = baseX + trapX, wy = baseY + 3, wz = baseZ + z;
                if (bounds.contains(wx, wy, wz)) {
                    world.setBlockState(wx, wy, wz, tripwire, 18);
                }
            }
            {
                i32 wx = baseX + trapX + 1, wy = baseY + 3, wz = baseZ + z;
                if (bounds.contains(wx, wy, wz)) {
                    world.setBlockState(wx, wy, wz, tripwire, 18);
                }
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
            i32 wx = baseX + chestX - 1, wy = baseY + y, wz = baseZ + chestZ;
            if (bounds.contains(wx, wy, wz)) {
                world.setBlockState(wx, wy, wz, mossyStoneBricks, 18);
            }
        }
        {
            i32 wx = baseX + chestX, wy = baseY + y, wz = baseZ + chestZ;
            if (bounds.contains(wx, wy, wz)) {
                world.setBlockState(wx, wy, wz, mossyStoneBricks, 18);
            }
        }
        {
            i32 wx = baseX + chestX + 1, wy = baseY + y, wz = baseZ + chestZ;
            if (bounds.contains(wx, wy, wz)) {
                world.setBlockState(wx, wy, wz, mossyStoneBricks, 18);
            }
        }
    }

    // 宝箱房间内部
    for (i32 x = chestX - 1; x <= chestX + 1; ++x) {
        for (i32 z = chestZ + 1; z < length - 1; ++z) {
            i32 wx = baseX + x, wy = baseY + 3, wz = baseZ + z;
            if (bounds.contains(wx, wy, wz)) {
                world.setBlockState(wx, wy, wz, stoneBricks, 18);
            }
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
            i32 wx = baseX + cx, wy = baseY + y, wz = baseZ + cz;
            if (bounds.contains(wx, wy, wz)) {
                world.setBlockState(wx, wy, wz, randomBrick(), 18);
            }
        }

        // 塔楼顶部装饰
        {
            i32 wx = baseX + cx, wy = baseY + height + TOWER_EXTRA_HEIGHT, wz = baseZ + cz;
            if (bounds.contains(wx, wy, wz)) {
                world.setBlockState(wx, wy, wz, chiseledStoneBricks, 18);
            }
        }
    }

    // 屋顶平台
    for (i32 x = 1; x < width - 1; ++x) {
        for (i32 z = 1; z < length - 1; ++z) {
            i32 wx = baseX + x, wy = baseY + height, wz = baseZ + z;
            if (bounds.contains(wx, wy, wz)) {
                world.setBlockState(wx, wy, wz, stoneBricks, 18);
            }
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
            i32 wwx = baseX + wx, wwy = baseY + vy, wwz = baseZ + wz;
            if (bounds.contains(wwx, wwy, wwz)) {
                world.setBlockState(wwx, wwy, wwz, vine, 18);
            }
        }
    }
}

// ============================================================================
// JungleTempleStructure
// ============================================================================

const std::string JungleTempleStructure::m_name = "jungle_temple";

JungleTempleStructure::JungleTempleStructure() noexcept
    : Structure(ResourceLocation("minecraft", "jungle_pyramid"))
{}

const biome::BiomeTag* JungleTempleStructure::defaultBiomeTag() const
{
    return &biome::BiomeTags::HAS_STRUCTURE_JUNGLE_PYRAMID();
}

bool JungleTempleStructure::canGenerate(
    IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ)
{
    MC_UNUSED(world);
    MC_UNUSED(rng);
    // 检查区块中心位置的生物群系是否属于丛林神庙可生成的生物群系
    const BiomeId biome = generator.getBiome(chunkX * CHUNK_WIDTH + 8, 64, chunkZ * CHUNK_WIDTH + 8);
    return isValidBiome(biome);
}

std::unique_ptr<StructureStart> JungleTempleStructure::generate(
    IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const
{
    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    // 计算起始位置
    i32 startX = chunkX * CHUNK_WIDTH + rng.nextInt(CHUNK_WIDTH);
    i32 startZ = chunkZ * CHUNK_WIDTH + rng.nextInt(CHUNK_WIDTH);

    // 获取地表高度
    i32 startY = generator.getHeight(startX, startZ, HeightmapType::WorldSurfaceWG);
    if (startY < MIN_GENERATION_HEIGHT) startY = FALLBACK_HEIGHT;

    BlockPos startPos(startX, startY, startZ);

    // 创建片段（方块写入延迟到 FEATURES 阶段由 placeInChunk() 执行）
    auto piece = std::make_unique<JungleTemplePiece>(startPos);
    start->addPiece(std::move(piece));
    start->recalculateStructureSize();

    return start;
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
