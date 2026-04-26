#include "UnderwaterCarver.hpp"
#include "../../block/VanillaBlocks.hpp"
#include "../../chunk/ChunkPrimer.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../core/Constants.hpp"
#include <unordered_set>

namespace mc::world::gen::carver {

// ============================================================================
// 水下可雕刻方块集合
// ============================================================================

static const std::unordered_set<u32>& getUnderwaterCarvableBlocks()
{
    static std::unordered_set<u32> blocks = {
        // 标准可雕刻方块
        VanillaBlocks::STONE->blockId(),
        VanillaBlocks::GRANITE->blockId(),
        VanillaBlocks::DIORITE->blockId(),
        VanillaBlocks::ANDESITE->blockId(),
        VanillaBlocks::DIRT->blockId(),
        VanillaBlocks::COARSE_DIRT->blockId(),
        VanillaBlocks::PODZOL->blockId(),
        VanillaBlocks::GRASS_BLOCK->blockId(),
        // 陶瓦（包括染色陶瓦）
        VanillaBlocks::TERRACOTTA->blockId(),
        VanillaBlocks::WHITE_TERRACOTTA->blockId(),
        VanillaBlocks::ORANGE_TERRACOTTA->blockId(),
        VanillaBlocks::MAGENTA_TERRACOTTA->blockId(),
        VanillaBlocks::LIGHT_BLUE_TERRACOTTA->blockId(),
        VanillaBlocks::YELLOW_TERRACOTTA->blockId(),
        VanillaBlocks::LIME_TERRACOTTA->blockId(),
        VanillaBlocks::PINK_TERRACOTTA->blockId(),
        VanillaBlocks::GRAY_TERRACOTTA->blockId(),
        VanillaBlocks::LIGHT_GRAY_TERRACOTTA->blockId(),
        VanillaBlocks::CYAN_TERRACOTTA->blockId(),
        VanillaBlocks::PURPLE_TERRACOTTA->blockId(),
        VanillaBlocks::BLUE_TERRACOTTA->blockId(),
        VanillaBlocks::BROWN_TERRACOTTA->blockId(),
        VanillaBlocks::GREEN_TERRACOTTA->blockId(),
        VanillaBlocks::RED_TERRACOTTA->blockId(),
        VanillaBlocks::BLACK_TERRACOTTA->blockId(),
        // 沙子和砂岩
        VanillaBlocks::SANDSTONE->blockId(),
        VanillaBlocks::RED_SANDSTONE->blockId(),
        VanillaBlocks::MYCELIUM->blockId(),
        VanillaBlocks::SNOW->blockId(),
        // 水下特有的可雕刻方块
        VanillaBlocks::SAND->blockId(),
        VanillaBlocks::GRAVEL->blockId(),
        VanillaBlocks::WATER->blockId(),
        VanillaBlocks::LAVA->blockId(),
        VanillaBlocks::OBSIDIAN->blockId(),
        // AIR 由 isAir() 检查
        // CAVE_AIR 暂未实现
        VanillaBlocks::PACKED_ICE->blockId()
    };
    return blocks;
}

// ============================================================================
// UnderwaterCaveCarver 实现
// ============================================================================

UnderwaterCaveCarver::UnderwaterCaveCarver()
    : CaveCarver(world::MAX_BUILD_HEIGHT)
{
}

bool UnderwaterCaveCarver::shouldSkipEllipsoidPosition(f32 dx, f32 dy, f32 dz, i32 y) const
{
    // 水下洞穴使用与普通洞穴相同的椭球检测
    // 参考 MC: return p_222708_3_ <= -0.7D || dx * dx + dy * dy + dz * dz >= 1.0D;
    (void)y;
    return dy <= -0.7f || dx * dx + dy * dy + dz * dz >= 1.0f;
}

bool UnderwaterCaveCarver::isUnderwaterCarvable(const BlockState& state)
{
    // 检查是否为空气
    if (state.isAir()) {
        return true;
    }

    // 检查是否在水下可雕刻方块列表中
    const auto& blocks = getUnderwaterCarvableBlocks();
    return blocks.find(state.blockId()) != blocks.end();
}

// ============================================================================
// UnderwaterCanyonCarver 实现
// ============================================================================

UnderwaterCanyonCarver::UnderwaterCanyonCarver()
    : CanyonCarver(world::MAX_BUILD_HEIGHT)
{
}

bool UnderwaterCanyonCarver::shouldSkipEllipsoidPosition(f32 dx, f32 dy, f32 dz, i32 y) const
{
    // 水下峡谷使用与普通峡谷相同的厚度检测
    return CanyonCarver::shouldSkipEllipsoidPosition(dx, dy, dz, y);
}

// ============================================================================
// 工厂函数
// ============================================================================

std::unique_ptr<UnderwaterCaveCarver> createUnderwaterCaveCarver()
{
    return std::make_unique<UnderwaterCaveCarver>();
}

std::unique_ptr<UnderwaterCanyonCarver> createUnderwaterCanyonCarver()
{
    return std::make_unique<UnderwaterCanyonCarver>();
}

} // namespace mc::world::gen::carver
