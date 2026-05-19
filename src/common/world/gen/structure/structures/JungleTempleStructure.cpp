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
#include "../../../../util/math/random/Random.hpp"
#include "../../../IWorldWriter.hpp"
#include "../../../biome/Biome.hpp"
#include "../../../block/BlockPos.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../StructureBoundingBox.hpp"

namespace mc {
namespace world {
namespace gen {
namespace structure {

using namespace mc::Biomes;

const std::string JungleTempleStructure::m_name = "jungle_temple";

JungleTempleStructure::JungleTempleStructure()
    : Structure(StructureType::Temple)
{
    initializeBiomes();
}

void JungleTempleStructure::initializeBiomes()
{
    m_validBiomes = {Jungle, JungleHills, JungleEdge, ModifiedJungle, ModifiedJungleEdge};
}

bool JungleTempleStructure::canGenerate(
    IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ)
{
    // 检查生物群系是否合适
    return true;
}

std::unique_ptr<StructureStart> JungleTempleStructure::generate(
    IWorldWriter& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const
{
    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    // 丛林神庙尺寸: 12x15 地面部分，高度约 10
    // 计算起始位置
    i32 startX = chunkX * 16 + rng.nextInt(16);
    i32 startZ = chunkZ * 16 + rng.nextInt(16);

    // 获取地表高度
    i32 startY = generator.getHeight(startX, startZ, HeightmapType::WorldSurfaceWG);
    if (startY < 60) startY = 64;

    BlockPos startPos(startX, startY, startZ);

    // 生成丛林神庙
    generateTemple(world, rng, startPos);

    return start;
}

void JungleTempleStructure::generateTemple(IWorldWriter& world, math::Random& rng, const BlockPos& startPos) const
{
    const BlockState* cobblestone = VanillaBlocks::getState(VanillaBlocks::COBBLESTONE);
    const BlockState* mossyCobblestone = VanillaBlocks::getState(VanillaBlocks::MOSSY_COBBLESTONE);
    const BlockState* stoneBricks = VanillaBlocks::getState(VanillaBlocks::STONE_BRICKS);
    const BlockState* mossyStoneBricks = VanillaBlocks::getState(VanillaBlocks::MOSSY_STONE_BRICKS);
    const BlockState* chiseledStoneBricks = VanillaBlocks::getState(VanillaBlocks::CHISELED_STONE_BRICKS);
    const BlockState* vine = VanillaBlocks::getState(VanillaBlocks::VINE);
    const BlockState* air = VanillaBlocks::getState(VanillaBlocks::AIR);

    // 基础参数
    i32 baseX = startPos.x;
    i32 baseY = startPos.y;
    i32 baseZ = startPos.z;
    i32 width = 12;  // X轴宽度
    i32 length = 15; // Z轴深度
    i32 height = 10; // 高度

    // 辅助lambda: 随机选择苔石或普通圆石
    auto randomCobble = [&]() -> const BlockState* { return rng.nextInt(100) < 30 ? mossyCobblestone : cobblestone; };

    // 辅助lambda: 随机选择石砖类型
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
                world.setBlockState(baseX + x, baseY + y, baseZ + z, randomCobble(), 18);
            }
        }
    }

    // 生成外墙
    for (i32 y = 2; y < height; ++y) {
        for (i32 x = 0; x < width; ++x) {
            // 前后墙
            world.setBlockState(baseX + x, baseY + y, baseZ, randomBrick(), 18);
            world.setBlockState(baseX + x, baseY + y, baseZ + length - 1, randomBrick(), 18);
        }
        for (i32 z = 0; z < length; ++z) {
            // 左右墙
            world.setBlockState(baseX, baseY + y, baseZ + z, randomBrick(), 18);
            world.setBlockState(baseX + width - 1, baseY + y, baseZ + z, randomBrick(), 18);
        }
    }

    // 生成入口（南面中央）
    i32 entranceX = width / 2;
    for (i32 y = 2; y < 5; ++y) {
        world.setBlockState(baseX + entranceX - 1, baseY + y, baseZ, air, 18);
        world.setBlockState(baseX + entranceX, baseY + y, baseZ, air, 18);
        world.setBlockState(baseX + entranceX + 1, baseY + y, baseZ, air, 18);
    }

    // 入口台阶
    for (i32 step = 0; step < 3; ++step) {
        for (i32 x = entranceX - 1; x <= entranceX + 1; ++x) {
            world.setBlockState(baseX + x, baseY + 2 - step, baseZ - step - 1, cobblestone, 18);
        }
    }

    // 内部地板
    for (i32 x = 1; x < width - 1; ++x) {
        for (i32 z = 1; z < length - 1; ++z) {
            world.setBlockState(baseX + x, baseY + 2, baseZ + z, stoneBricks, 18);
        }
    }

    // 中央走廊 - 被墙分隔
    i32 corridorZ = length / 2;
    for (i32 x = 1; x < width - 1; ++x) {
        world.setBlockState(baseX + x, baseY + 3, baseZ + corridorZ, stoneBricks, 18);
        world.setBlockState(baseX + x, baseY + 4, baseZ + corridorZ, stoneBricks, 18);
    }

    // 走廊门洞
    world.setBlockState(baseX + entranceX, baseY + 3, baseZ + corridorZ, air, 18);
    world.setBlockState(baseX + entranceX, baseY + 4, baseZ + corridorZ, air, 18);

    // 拉杆谜题房间（西侧）
    i32 puzzleX = 2;
    i32 puzzleZ = 3;
    // 隐藏机关墙
    for (i32 y = 3; y < 6; ++y) {
        world.setBlockState(baseX + puzzleX, baseY + y, baseZ + puzzleZ, chiseledStoneBricks, 18);
        world.setBlockState(baseX + puzzleX + 1, baseY + y, baseZ + puzzleZ, chiseledStoneBricks, 18);
    }

    // 箭矢陷阱走廊（东侧）
    // 陷阱房间
    i32 trapX = width - 3;
    for (i32 z = 2; z < 6; ++z) {
        world.setBlockState(baseX + trapX, baseY + 3, baseZ + z, air, 18);
        world.setBlockState(baseX + trapX + 1, baseY + 3, baseZ + z, air, 18);
    }

    // 宝箱房间（北侧，隐藏）
    i32 chestX = width / 2;
    i32 chestZ = length - 3;

    // 宝箱房间入口（需要机关开启）
    for (i32 y = 3; y < 6; ++y) {
        world.setBlockState(baseX + chestX - 1, baseY + y, baseZ + chestZ, mossyStoneBricks, 18);
        world.setBlockState(baseX + chestX, baseY + y, baseZ + chestZ, mossyStoneBricks, 18);
        world.setBlockState(baseX + chestX + 1, baseY + y, baseZ + chestZ, mossyStoneBricks, 18);
    }

    // 宝箱房间内部
    for (i32 x = chestX - 1; x <= chestX + 1; ++x) {
        for (i32 z = chestZ + 1; z < length - 1; ++z) {
            world.setBlockState(baseX + x, baseY + 3, baseZ + z, stoneBricks, 18);
        }
    }

    // 宝箱（高概率生成）
    if (rng.nextInt(100) < 70) {
        // 宝箱位置暂时用金块标记（实际应该放置宝箱）
        const BlockState* goldBlock = VanillaBlocks::getState(VanillaBlocks::GOLD_BLOCK);
        world.setBlockState(baseX + chestX, baseY + 3, baseZ + chestZ + 2, goldBlock, 18);
    }

    // 三层塔楼（四角）
    for (i32 corner = 0; corner < 4; ++corner) {
        i32 cx = (corner % 2 == 0) ? 0 : width - 1;
        i32 cz = (corner < 2) ? 0 : length - 1;

        // 塔楼高度额外增加 4 格
        for (i32 y = height; y < height + 4; ++y) {
            world.setBlockState(baseX + cx, baseY + y, baseZ + cz, randomBrick(), 18);
        }

        // 塔楼顶部装饰
        world.setBlockState(baseX + cx, baseY + height + 4, baseZ + cz, chiseledStoneBricks, 18);
    }

    // 屋顶平台
    for (i32 x = 1; x < width - 1; ++x) {
        for (i32 z = 1; z < length - 1; ++z) {
            world.setBlockState(baseX + x, baseY + height, baseZ + z, stoneBricks, 18);
        }
    }

    // 添加藤蔓装饰（外墙）
    for (i32 i = 0; i < 20; ++i) {
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
            world.setBlockState(baseX + wx, baseY + vy, baseZ + wz, vine, 18);
        }
    }
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
