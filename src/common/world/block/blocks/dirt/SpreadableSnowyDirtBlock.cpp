#include "SpreadableSnowyDirtBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../../fluid/Fluid.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/math/random/IRandom.hpp"
#include "../../../../util/property/StateContainer.hpp"
#include "../../../../core/Constants.hpp"
#include "../ice/SnowBlock.hpp"

namespace mc::blocks {

// ============================================================================
// SpreadableSnowyDirtBlock 实现
// ============================================================================

SpreadableSnowyDirtBlock::SpreadableSnowyDirtBlock(BlockProperties properties)
    : Block(std::move(properties)) {

    // 创建状态容器，添加 SNOWY 属性
    // 参考 MC 1.16.5 SnowyDirtBlock.fillStateContainer()
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(SNOWY())
        .create([this](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态：无雪
    // 参考 MC 1.16.5 SnowyDirtBlock 构造函数
    setDefaultState(defaultState().with(SNOWY(), false));
}

void SpreadableSnowyDirtBlock::randomTick(
    IWorld& world,
    const BlockPos& pos,
    BlockState& state,
    math::IRandom& random) {

    // 参考: MC 1.16.5 SpreadableSnowyDirtBlock.randomTick()

    // 检查是否满足蔓延条件
    if (!isSnowyConditions(world, pos, state)) {
        // 不满足条件，退化成泥土
        const BlockState* dirtState = &VanillaBlocks::DIRT->defaultState();
        if (dirtState != nullptr) {
            world.setBlockState(pos, dirtState);
        }
    } else {
        // 满足条件，尝试向周围蔓延
        // 需要 pos.up() 的光照 >= 9
        u8 skyLight = world.getSkyLight(pos.x, pos.y + 1, pos.z);
        u8 blockLight = world.getBlockLight(pos.x, pos.y + 1, pos.z);
        u8 lightLevel = std::max(skyLight, blockLight);

        if (lightLevel >= 9) {
            const BlockState* defaultState = &getDefaultState();
            if (defaultState == nullptr) {
                return;
            }

            // 尝试向4个随机位置的泥土蔓延
            for (i32 i = 0; i < 4; ++i) {
                i32 dx = random.nextInt(3) - 1;  // -1, 0, 1
                i32 dy = random.nextInt(5) - 3;  // -3, -2, -1, 0, 1
                i32 dz = random.nextInt(3) - 1;  // -1, 0, 1

                BlockPos targetPos(pos.x + dx, pos.y + dy, pos.z + dz);

                // 检查目标位置是否为泥土
                const BlockState* targetState = world.getBlockState(targetPos);
                if (targetState == nullptr || targetState->blockId() != VanillaBlocks::DIRT->blockId()) {
                    continue;
                }

                // 检查目标位置是否满足蔓延条件
                if (isSnowyAndNotUnderwater(world, targetPos, *defaultState)) {
                    // 检查目标位置上方是否有雪
                    // 参考 MC 1.16.5: 蔓延时只检查 SNOW（雪层），不检查 SNOW_BLOCK（雪块）
                    BlockPos abovePos(targetPos.x, targetPos.y + 1, targetPos.z);
                    const BlockState* aboveState = world.getBlockState(abovePos);
                    bool hasSnow = aboveState != nullptr &&
                                   aboveState->is(VanillaBlocks::SNOW);

                    // 设置新方块状态，包含 SNOWY 属性
                    const BlockState* newState = &defaultState->with(SNOWY(), hasSnow);
                    world.setBlockState(targetPos, newState);
                }
            }
        }
    }
}

BlockState SpreadableSnowyDirtBlock::getStateForPlacement(BlockItemUseContext& context) {
    // 参考 MC 1.16.5 SnowyDirtBlock.getStateForPlacement()
    // 检查放置位置上方是否有雪块或雪层
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);

    bool hasSnow = false;
    if (aboveState != nullptr) {
        // MC 1.16.5: 检查 SNOW_BLOCK 或 SNOW（任意层数）
        hasSnow = aboveState->is(VanillaBlocks::SNOW_BLOCK) ||
                  aboveState->is(VanillaBlocks::SNOW);
    }

    return defaultState().with(SNOWY(), hasSnow);
}

BlockState SpreadableSnowyDirtBlock::updatePostPlacement(
    const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos) {

    // 参考 MC 1.16.5 SnowyDirtBlock.updatePostPlacement()
    // 只有上方方块变化时才更新 SNOWY 状态
    if (facing == Direction::Up) {
        // 检查上方是否为雪块或雪层
        bool hasSnow = facingState.is(VanillaBlocks::SNOW_BLOCK) ||
                       facingState.is(VanillaBlocks::SNOW);
        return state.with(SNOWY(), hasSnow);
    }

    return state;
}

bool SpreadableSnowyDirtBlock::isSnowyConditions(
    IWorld& world,
    const BlockPos& pos,
    const BlockState& state) {

    // 参考: MC 1.16.5 SpreadableSnowyDirtBlock.isSnowyConditions()
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);

    if (aboveState == nullptr || aboveState->isAir()) {
        // 上方为空气，检查光照
        u8 skyLight = world.getSkyLight(abovePos);
        u8 blockLight = world.getBlockLight(abovePos);
        u8 lightLevel = std::max(skyLight, blockLight);
        return lightLevel < 15;  // 不是完全被阻挡就满足条件
    }

    // 检查是否为雪层且层数为1
    // MC 1.16.5: blockstate.isIn(Blocks.SNOW) && blockstate.get(SnowBlock.LAYERS) == 1
    if (aboveState->is(VanillaBlocks::SNOW)) {
        // 检查 LAYERS 属性是否为 1
        // 使用 getOptional 安全获取，因为 SNOWY 状态会在这里被检查
        std::optional<i32> layers = aboveState->getOptional(SnowBlock::LAYERS());
        if (layers.has_value() && layers.value() == 1) {
            return true;  // 只有1层雪时满足条件
        }
        // 多层雪会进入下面的光照检查逻辑
    }

    // 检查上方是否有完整水源
    const fluid::FluidState* fluidState = aboveState->getFluidState();
    if (fluidState != nullptr && !fluidState->isEmpty() && fluidState->getLevel() == 8) {
        return false;  // 上方有完整水源，不满足条件
    }

    // 检查光照
    u8 skyLight = world.getSkyLight(abovePos);
    u8 blockLight = world.getBlockLight(abovePos);
    u8 lightLevel = std::max(skyLight, blockLight);

    return lightLevel < 15;  // 不是完全被阻挡就满足条件
}

bool SpreadableSnowyDirtBlock::isSnowyAndNotUnderwater(
    IWorld& world,
    const BlockPos& pos,
    const BlockState& state) {

    // 参考: MC 1.16.5 SpreadableSnowyDirtBlock.isSnowyAndNotUnderwater()

    if (!isSnowyConditions(world, pos, state)) {
        return false;
    }

    // 检查上方是否有水
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);
    if (aboveState != nullptr) {
        const fluid::FluidState* fluidState = aboveState->getFluidState();
        if (fluidState != nullptr && !fluidState->isEmpty()) {
            return false;  // 上方有流体
        }
    }

    return true;
}

// ============================================================================
// GrassBlock 实现
// ============================================================================

GrassBlock::GrassBlock(BlockProperties properties)
    : SpreadableSnowyDirtBlock(std::move(properties)) {
}

// ============================================================================
// MyceliumBlock 实现
// ============================================================================

MyceliumBlock::MyceliumBlock(BlockProperties properties)
    : SpreadableSnowyDirtBlock(std::move(properties)) {
}

} // namespace mc::blocks
