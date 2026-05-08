#include "FortressStructure.hpp"
#include "../../jigsaw/JigsawManager.hpp"
#include "../../jigsaw/JigsawPattern.hpp"
#include "../../../biome/Biome.hpp"
#include "../../../biome/Biomes.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../block/BlockPos.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../../IWorldWriter.hpp"
#include "../StructureBoundingBox.hpp"
#include <cmath>

namespace mc {
namespace world {
namespace gen {
namespace structure {

using namespace mc::Biomes;

const std::string FortressStructure::m_name = "fortress";

FortressStructure::FortressStructure()
    : Structure(StructureType::Fortress)
{
    initializeBiomes();
}

FortressStructure::FortressStructure(const Config& config)
    : Structure(StructureType::Fortress)
    , m_config(config)
{
    initializeBiomes();
}

void FortressStructure::initializeBiomes() {
    // 下界要塞只生成在下界荒地和灵魂沙谷
    m_validBiomes = {
        NetherWastes,       // 下界荒地
        SoulSandValley      // 灵魂沙谷
    };
}

bool FortressStructure::canGenerate(
    IWorld& world,
    IChunkGenerator& generator,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ)
{
    // MC 1.16.5: 下界要塞只检查概率，不检查生物群系
    // 生物群系检查由维度的 BiomeGenerationSettings 决定
    // 参考: FortressStructure.func_230363_a_ -> 只调用 rng.nextInt(5) < 2

    // 40% 概率生成 (参考 MC: nextInt(5) < 2)
    return rng.nextInt(5) < 2;
}

std::unique_ptr<StructureStart> FortressStructure::generate(
    IWorldWriter& world,
    IChunkGenerator& generator,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ) const
{
    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    // 计算起始位置
    i32 startX = chunkX * 16 + 2;
    i32 startZ = chunkZ * 16 + 2;

    // Y 坐标在 64-128 之间随机选择
    i32 startY = m_config.minY + rng.nextInt(m_config.maxY - m_config.minY);

    BlockPos startPos(startX, startY, startZ);

    // 使用 Jigsaw 系统生成要塞
    ResourceLocation startPoolLocation("minecraft", "nether_fortress/start");
    auto& patternRegistry = jigsaw::JigsawPatternRegistry::instance();
    const jigsaw::JigsawPattern* startPool = patternRegistry.getPattern(startPoolLocation);

    if (startPool && !startPool->isEmpty()) {
        // 使用 Jigsaw 系统生成
        jigsaw::JigsawManager::assembleAndPlace(
            world,
            patternRegistry,
            *startPool,
            10,  // 要塞深度
            startPos,
            rng
        );
    } else {
        // 回退：生成简单的要塞结构
        generateFallbackFortress(world, rng, startPos);
    }

    return start;
}

void FortressStructure::generateFallbackFortress(
    IWorldWriter& world,
    math::Random& rng,
    const BlockPos& startPos) const
{
    // 获取方块状态
    const BlockState* netherBricks = VanillaBlocks::getState(VanillaBlocks::NETHERRACK); // 使用下界岩替代
    const BlockState* netherrack = VanillaBlocks::getState(VanillaBlocks::NETHERRACK);
    const BlockState* soulSand = VanillaBlocks::getState(VanillaBlocks::SOUL_SAND);
    const BlockState* netherWart = VanillaBlocks::getState(VanillaBlocks::NETHER_WART);
    const BlockState* netherWartBlock = VanillaBlocks::getState(VanillaBlocks::NETHER_WART_BLOCK);
    const BlockState* air = VanillaBlocks::getState(VanillaBlocks::AIR);
    const BlockState* lava = VanillaBlocks::getState(VanillaBlocks::LAVA);

    i32 baseX = startPos.x;
    i32 baseY = startPos.y;
    i32 baseZ = startPos.z;

    // 生成主桥 (直线段)
    // 尺寸: 5x10x19
    for (i32 z = 0; z < 19; ++z) {
        // 桥面
        for (i32 x = 0; x < 5; ++x) {
            world.setBlockState(baseX + x, baseY, baseZ + z, netherBricks, 18);
        }
        // 围栏
        world.setBlockState(baseX, baseY + 1, baseZ + z, netherBricks, 18);
        world.setBlockState(baseX + 4, baseY + 1, baseZ + z, netherBricks, 18);
        world.setBlockState(baseX, baseY + 2, baseZ + z, netherBricks, 18);
        world.setBlockState(baseX + 4, baseY + 2, baseZ + z, netherBricks, 18);

        // 中间是空气
        for (i32 y = 1; y <= 8; ++y) {
            world.setBlockState(baseX + 1, baseY + y, baseZ + z, air, 18);
            world.setBlockState(baseX + 2, baseY + y, baseZ + z, air, 18);
            world.setBlockState(baseX + 3, baseY + y, baseZ + z, air, 18);
        }
    }

    // 生成王座房间（带烈焰人刷怪笼）
    // 尺寸: 7x8x9
    i32 throneX = baseX + 10;
    i32 throneY = baseY;
    i32 throneZ = baseZ + 20;

    // 地板
    for (i32 x = 0; x < 7; ++x) {
        for (i32 z = 0; z < 9; ++z) {
            world.setBlockState(throneX + x, throneY, throneZ + z, netherBricks, 18);
        }
    }

    // 墙壁
    for (i32 y = 1; y <= 7; ++y) {
        for (i32 x = 0; x < 7; ++x) {
            world.setBlockState(throneX + x, throneY + y, throneZ, netherBricks, 18);
            world.setBlockState(throneX + x, throneY + y, throneZ + 8, netherBricks, 18);
        }
        for (i32 z = 0; z < 9; ++z) {
            world.setBlockState(throneX, throneY + y, throneZ + z, netherBricks, 18);
            world.setBlockState(throneX + 6, throneY + y, throneZ + z, netherBricks, 18);
        }
    }

    // 天花板
    for (i32 x = 0; x < 7; ++x) {
        for (i32 z = 0; z < 9; ++z) {
            world.setBlockState(throneX + x, throneY + 8, throneZ + z, netherBricks, 18);
        }
    }

    // 烈焰人刷怪点标记（中心偏移）
    BlockPos spawnerPos(throneX + 3, throneY + 5, throneZ + 4);
    world.setBlockState(
        spawnerPos.x,
        spawnerPos.y,
        spawnerPos.z,
        netherWartBlock ? netherWartBlock : netherBricks,
        18);

    // 生成地狱疣房间
    // 尺寸: 13x14x13
    i32 wartRoomX = baseX + 25;
    i32 wartRoomY = baseY;
    i32 wartRoomZ = baseZ + 10;

    // 地板（灵魂沙）
    for (i32 x = 0; x < 13; ++x) {
        for (i32 z = 0; z < 13; ++z) {
            world.setBlockState(wartRoomX + x, wartRoomY, wartRoomZ + z, soulSand, 18);
        }
    }

    // 墙壁
    for (i32 y = 1; y <= 13; ++y) {
        for (i32 x = 0; x < 13; ++x) {
            world.setBlockState(wartRoomX + x, wartRoomY + y, wartRoomZ, netherBricks, 18);
            world.setBlockState(wartRoomX + x, wartRoomY + y, wartRoomZ + 12, netherBricks, 18);
        }
        for (i32 z = 0; z < 13; ++z) {
            world.setBlockState(wartRoomX, wartRoomY + y, wartRoomZ + z, netherBricks, 18);
            world.setBlockState(wartRoomX + 12, wartRoomY + y, wartRoomZ + z, netherBricks, 18);
        }
    }

    // 天花板
    for (i32 x = 0; x < 13; ++x) {
        for (i32 z = 0; z < 13; ++z) {
            world.setBlockState(wartRoomX + x, wartRoomY + 14, wartRoomZ + z, netherBricks, 18);
        }
    }

    // 灵魂沙平台上的地狱疣
    // 平台位置: (3-4, 4, 4-8) 和 (8-9, 4, 4-8)
    for (i32 x = 3; x <= 4; ++x) {
        for (i32 z = 4; z <= 8; ++z) {
            if (netherWart) {
                world.setBlockState(wartRoomX + x, wartRoomY + 1, wartRoomZ + z, netherWart, 18);
            }
        }
    }
    for (i32 x = 8; x <= 9; ++x) {
        for (i32 z = 4; z <= 8; ++z) {
            if (netherWart) {
                world.setBlockState(wartRoomX + x, wartRoomY + 1, wartRoomZ + z, netherWart, 18);
            }
        }
    }

    // 入口（带岩浆）
    // 尺寸: 13x14x13
    i32 entranceX = baseX - 8;
    i32 entranceY = baseY;
    i32 entranceZ = baseZ - 8;

    // 简化入口：岩浆坑
    for (i32 x = 0; x < 5; ++x) {
        for (i32 z = 0; z < 5; ++z) {
            world.setBlockState(entranceX + x, entranceY - 1, entranceZ + z, lava, 18);
        }
    }
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
