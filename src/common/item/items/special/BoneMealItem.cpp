#include "BoneMealItem.hpp"
#include "../../context/ItemUseContext.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/IGrowable.hpp"
#include "../../../world/block/Block.hpp"
#include "../../../world/block/BlockPos.hpp"
#include "../../../world/block/VanillaBlocks.hpp"
#include "../../../world/block/WaterLoggableHelpers.hpp"
#include "../../../world/fluid/FluidTags.hpp"
#include "../../../world/fluid/FluidRegistry.hpp"
#include "../../../world/chunk/IChunk.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../util/Direction.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"

namespace mc {
namespace item::items {

BoneMealItem::BoneMealItem(ItemProperties properties)
    : Item(std::move(properties)) {
}

ActionResultType BoneMealItem::onItemUse(ItemUseContext& context) {
    const BlockPos& pos = context.blockPos();
    IWorld& world = const_cast<IWorld&>(context.world());
    const BlockState* statePtr = world.getBlockState(pos);

    // 首先尝试对 IGrowable 方块使用骨粉
    if (statePtr != nullptr) {
        const Block& block = statePtr->owner();
        IGrowable* growable = const_cast<IGrowable*>(dynamic_cast<const IGrowable*>(&block));

        if (growable != nullptr) {
            // 检查是否可以生长
            if (growable->canGrow(static_cast<IBlockReader&>(world), pos, *statePtr, false)) {
                const u64 seed = world.seed() ^ static_cast<u64>(std::hash<BlockPos>{}(pos));
                math::Random random(seed);

                if (growable->canUseBonemeal(world, random, pos, *statePtr)) {
                    // 执行生长
                    growable->grow(world, random, pos, *statePtr);

                    // 减少物品数量（非创造模式）
                    if (context.itemStack().getCount() > 0) {
                        const_cast<ItemStack&>(context.itemStack()).shrink(1);
                    }

                    // 生成快乐村民粒子
                    spawnBonemealParticles(world, pos);

                    return ActionResultType::Success;
                }
            }
        }
    }

    // 如果对 IGrowable 使用失败，尝试在水中生成海草
    // MC 1.16.5: 如果目标位置是水源方块，尝试生成海草
    const fluid::FluidState* fluidState = world.getFluidState(pos);
    if (fluidState != nullptr &&
        !fluidState->isEmpty() &&
        fluidState->getFluid().isIn(fluid::FluidTags::WATER()) &&
        fluidState->isSource()) {

        const u64 seed = world.seed() ^ static_cast<u64>(std::hash<BlockPos>{}(pos));
        math::Random random(seed);

        if (growSeagrass(world, pos, random)) {
            // 减少物品数量（非创造模式）
            if (context.itemStack().getCount() > 0) {
                const_cast<ItemStack&>(context.itemStack()).shrink(1);
            }

            // 生成快乐村民粒子
            spawnBonemealParticles(world, pos);

            return ActionResultType::Success;
        }
    }

    return ActionResultType::Fail;
}

bool BoneMealItem::applyBonemeal(ItemStack& stack, IWorld& world, const BlockPos& pos, Player* player) {
    MC_UNUSED(player);

    // 获取方块状态
    const BlockState* statePtr = world.getBlockState(pos);
    if (statePtr == nullptr) {
        return false;
    }

    const BlockState& state = *statePtr;
    const Block& block = state.owner();

    // 检查方块是否实现 IGrowable 接口
    // 注意：grow() 是非 const 方法，所以不能用 const 指针
    IGrowable* growable = const_cast<IGrowable*>(dynamic_cast<const IGrowable*>(&block));
    if (growable == nullptr) {
        return false;
    }

    // 检查是否可以生长
    // IGrowable::canGrow 需要 IBlockReader，需要从 IWorld 转换
    if (!growable->canGrow(static_cast<IBlockReader&>(world), pos, state, false)) {
        return false;
    }

    // 检查是否可以使用骨粉
    // 使用世界种子和位置派生随机数，确保确定性
    const u64 seed = world.seed() ^ static_cast<u64>(std::hash<BlockPos>{}(pos));
    math::Random random(seed);

    if (growable->canUseBonemeal(world, random, pos, state)) {
        // 执行生长
        growable->grow(world, random, pos, state);

        // 减少物品数量（非创造模式）
        if (stack.getCount() > 0) {
            stack.shrink(1);
        }

        // 生成快乐村民粒子
        spawnBonemealParticles(world, pos);

        return true;
    }

    return false;
}

bool BoneMealItem::growSeagrass(IWorld& world, const BlockPos& pos, math::IRandom& random) {
    // 参考: net.minecraft.item.BoneMealItem#growSeagrass
    // MC 1.16.5: 在水下生成海草的逻辑

    // 1. 检查目标位置是否为水源方块（流体等级=8）
    const BlockState* blockState = world.getBlockState(pos);
    if (blockState == nullptr) {
        return false;
    }

    // 检查是否为水方块
    if (!blockState->isLiquid()) {
        return false;
    }

    // 检查流体等级是否为8（完整水源）
    const fluid::FluidState* fluidState = world.getFluidState(pos);
    if (fluidState == nullptr || fluidState->isEmpty()) {
        return false;
    }

    if (!fluidState->getFluid().isIn(fluid::FluidTags::WATER())) {
        return false;
    }

    if (!fluidState->isSource()) {
        return false;  // 只在完整水源方块中生成
    }

    // 获取海草和高海草方块
    Block* seagrassBlock = VanillaBlocks::SEAGRASS;
    Block* tallSeagrassBlock = VanillaBlocks::TALL_SEAGRASS;

    if (seagrassBlock == nullptr || tallSeagrassBlock == nullptr) {
        return false;
    }

    // 2. 128次循环尝试生成
    // MC 1.16.5: for(int i = 0; i < 128; ++i)
    for (i32 i = 0; i < 128; ++i) {
        BlockPos currentPos = pos;

        // 扩散范围：i/16 决定扩散距离
        // MC 1.16.5: for(int j = 0; j < i / 16; ++j)
        for (i32 j = 0; j < i / 16; ++j) {
            // 随机偏移位置
            // MC 1.16.5: blockpos.add(random.nextInt(3) - 1,
            //                          (random.nextInt(3) - 1) * random.nextInt(3) / 2,
            //                          random.nextInt(3) - 1)
            currentPos = BlockPos(
                currentPos.x + random.nextInt(3) - 1,
                currentPos.y + (random.nextInt(3) - 1) * random.nextInt(3) / 2,
                currentPos.z + random.nextInt(3) - 1
            );

            // 检查位置是否有效
            const BlockState* currentState = world.getBlockState(currentPos);
            if (currentState == nullptr) {
                continue;
            }

            // 跳过有碰撞的方块
            // MC 1.16.5: if (worldIn.getBlockState(blockpos).hasOpaqueCollisionShape(...))
            if (!currentState->isAir() && currentState->isSolid()) {
                goto next_iteration;
            }
        }

        // 3. 尝试在当前位置生成海草
        {
            const BlockState* currentState = world.getBlockState(currentPos);
            const fluid::FluidState* currentFluid = world.getFluidState(currentPos);

            if (currentState == nullptr || currentFluid == nullptr) {
                goto next_iteration;
            }

            // 检查是否为海草（可升级为高海草）
            if (currentState->is(seagrassBlock)) {
                // 10% 概率将海草变成高海草
                // MC 1.16.5: if (blockstate1.isIn(Blocks.SEAGRASS) && random.nextInt(10) == 0)
                if (random.nextInt(10) == 0) {
                    // 检查上方是否有水源
                    BlockPos abovePos(currentPos.x, currentPos.y + 1, currentPos.z);
                    const fluid::FluidState* aboveFluid = world.getFluidState(abovePos);

                    if (aboveFluid != nullptr &&
                        !aboveFluid->isEmpty() &&
                        aboveFluid->getFluid().isIn(fluid::FluidTags::WATER()) &&
                        aboveFluid->isSource()) {

                        // 设置高海草
                        const BlockState& lowerState = tallSeagrassBlock->defaultState()
                            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower)
                            .with(BlockStateProperties::WATERLOGGED(), true);

                        const BlockState& upperState = tallSeagrassBlock->defaultState()
                            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Upper)
                            .with(BlockStateProperties::WATERLOGGED(), true);

                        world.setBlockState(currentPos, &lowerState, 3);
                        world.setBlockState(abovePos, &upperState, 3);
                    }
                }
            }
            // 检查是否为水源方块，可以放置海草
            else if (currentState->isLiquid() &&
                     currentFluid->getFluid().isIn(fluid::FluidTags::WATER()) &&
                     currentFluid->isSource()) {

                // 检查下方是否有固体支撑
                BlockPos belowPos(currentPos.x, currentPos.y - 1, currentPos.z);
                const BlockState* belowState = world.getBlockState(belowPos);

                if (belowState != nullptr && belowState->isSolid()) {
                    // 放置海草
                    // MC 1.16.5: worldIn.setBlockState(blockpos, blockstate, 3);
                    const BlockState& seagrassState = seagrassBlock->defaultState();
                    world.setBlockState(currentPos, &seagrassState, 3);
                }
            }
        }

        next_iteration:;
    }

    return true;
}

void BoneMealItem::spawnBonemealParticles(IWorld& world, const BlockPos& pos) {
    // 在方块周围生成快乐村民粒子
    // 粒子在方块上方随机分布

    constexpr f32 offsetX = 0.5f;
    constexpr f32 offsetY = 0.5f;
    constexpr f32 offsetZ = 0.5f;

    // 生成 15 个粒子
    // 参考 MC 1.16.5: BoneMealItem 生成 15 个 happy_villager 粒子
    constexpr u32 particleCount = 15;

    world.addParticle(
        client::renderer::trident::particle::ParticleTypeId::HappyVillager,
        Vector3(static_cast<f32>(pos.x) + offsetX,
                static_cast<f32>(pos.y) + offsetY,
                static_cast<f32>(pos.z) + offsetZ),
        Vector3(0.0f, 0.0f, 0.0f),  // 速度为0
        Vector3(1.0f, 1.0f, 1.0f),   // 偏移范围
        particleCount
    );
}

} // namespace item::items
} // namespace mc
