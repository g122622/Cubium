#include "BoneMealItem.hpp"
#include "../../context/ItemUseContext.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/IGrowable.hpp"
#include "../../../world/block/Block.hpp"
#include "../../../world/block/BlockPos.hpp"
#include "../../../world/block/BlockTags.hpp"
#include "../../../world/block/BlockRegistry.hpp"
#include "../../../world/block/VanillaBlocks.hpp"
#include "../../../world/biome/Biome.hpp"
#include "../../../world/biome/Biomes.hpp"
#include "../../../world/biome/BiomeRegistry.hpp"
#include "../../../world/chunk/ChunkData.hpp"
#include "../../../world/fluid/Fluid.hpp"
#include "../../../world/fluid/FluidTags.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../util/Direction.hpp"
#include "../../../util/property/Properties.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"

#include <iterator>

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
    // 在水下使用骨粉生成海草的逻辑

    // 检查是否为完整水源方块（level == 8）
    const BlockState* blockState = world.getBlockState(pos);
    if (blockState == nullptr || !blockState->is(VanillaBlocks::WATER)) {
        return false;
    }

    const fluid::FluidState* fluidState = world.getFluidState(pos);
    if (fluidState == nullptr || fluidState->isEmpty()) {
        return false;
    }

    // 必须是水且为完整水源方块（level == 8）
    if (!fluidState->getFluid().isIn(fluid::FluidTags::WATER())) {
        return false;
    }

    if (fluidState->getLevel() != 8) {
        return false;
    }

    // 检查 VanillaBlocks 是否已初始化
    if (VanillaBlocks::SEAGRASS == nullptr) {
        return false;
    }

    // MC 1.16.5: 循环 128 次，随机偏移位置
    bool placedAny = false;
    BlockPosMutable currentPos;

    for (i32 i = 0; i < 128; ++i) {
        // 从初始位置开始
        currentPos.set(pos.x, pos.y, pos.z);

        // 随机偏移位置
        // MC: blockpos = blockpos.add(random.nextInt(3) - 1, (random.nextInt(3) - 1) * random.nextInt(3) / 2, random.nextInt(3) - 1);
        for (i32 j = 0; j < i / 16; ++j) {
            const i32 dx = random.nextInt(3) - 1;  // -1, 0, 或 1
            const i32 dy = (random.nextInt(3) - 1) * random.nextInt(3) / 2;  // -1, 0, 或 1
            const i32 dz = random.nextInt(3) - 1;  // -1, 0, 或 1
            currentPos.move(dx, dy, dz);
        }

        // 检查当前位置是否有固体碰撞（跳过）
        const BlockState* currentState = world.getBlockState(currentPos);
        if (currentState == nullptr) {
            continue;
        }

        // MC: if (worldIn.getBlockState(blockpos).hasOpaqueCollisionShape(worldIn, blockpos)) continue;
        // 如果当前位置有固体方块，跳过
        if (currentState->isSolid()) {
            continue;
        }

        // 检查生物群系，在温暖海洋可能有珊瑚
        const BlockState* stateToPlace = &VanillaBlocks::SEAGRASS->defaultState();

        // 获取当前位置的生物群系
        const ChunkData* chunk = world.getChunk(currentPos.chunkX(), currentPos.chunkZ());
        if (chunk != nullptr) {
            const BiomeId biomeId = chunk->getBiomeAtBlock(currentPos.localX(), currentPos.y, currentPos.localZ());
            const bool isWarmOcean = (biomeId == Biomes::WarmOcean || biomeId == Biomes::DeepWarmOcean);

            if (isWarmOcean) {
                // 在温暖海洋中，有机会生成珊瑚
                if (BlockTags::WALL_CORALS().getBlockIds().size() > 0 &&
                    BlockTags::UNDERWATER_BONEMEALS().getBlockIds().size() > 0) {
                    // i == 0 且有水平方向时，放置墙珊瑚
                    if (i == 0) {
                        // 获取随机墙珊瑚方向
                        const auto& horizontalDirs = Directions::horizontal();
                        const Direction dir = horizontalDirs[static_cast<size_t>(random.nextInt(4))];

                        // 从 WALL_CORALS 标签中随机选择一个
                        const auto& wallCoralIds = BlockTags::WALL_CORALS().getBlockIds();
                        if (!wallCoralIds.empty()) {
                            // 随机选择一个墙珊瑚
                            auto it = wallCoralIds.begin();
                            std::advance(it, random.nextInt(static_cast<i32>(wallCoralIds.size())));
                            const ResourceLocation& coralId = *it;

                            // 检查是否可以放置墙珊瑚
                            // 墙珊瑚需要有墙面支撑
                            const Block* coralBlock = BlockRegistry::instance().getBlock(coralId);
                            if (coralBlock != nullptr) {
                                // 获取有 FACING 属性的默认状态
                                const BlockState* coralState = &coralBlock->defaultState();
                                if (coralState->hasProperty(BlockStateProperties::FACING())) {
                                    coralState = &coralState->with(BlockStateProperties::FACING(), dir);
                                    stateToPlace = coralState;
                                }
                            }
                        }
                    } else if (random.nextInt(4) == 0) {
                        // 25% 概率放置水下骨粉方块（珊瑚扇、海带等）
                        const auto& underwaterIds = BlockTags::UNDERWATER_BONEMEALS().getBlockIds();
                        if (!underwaterIds.empty()) {
                            auto it = underwaterIds.begin();
                            std::advance(it, random.nextInt(static_cast<i32>(underwaterIds.size())));
                            const ResourceLocation& blockId = *it;

                            const Block* block = BlockRegistry::instance().getBlock(blockId);
                            if (block != nullptr) {
                                stateToPlace = &block->defaultState();
                            }
                        }
                    }
                }
            }
        }

        // 检查是否可以放置
        // MC: if (blockstate.isValidPosition(worldIn, blockpos))
        const Block& blockToPlace = stateToPlace->owner();
        if (!blockToPlace.isValidPosition(*stateToPlace, static_cast<IBlockReader&>(world), currentPos)) {
            continue;
        }

        // 检查目标位置是否为水源方块
        const BlockState* targetState = world.getBlockState(currentPos);
        const fluid::FluidState* targetFluid = world.getFluidState(currentPos);

        if (targetState != nullptr && targetState->is(VanillaBlocks::WATER)) {
            if (targetFluid != nullptr && !targetFluid->isEmpty() &&
                targetFluid->getFluid().isIn(fluid::FluidTags::WATER()) &&
                targetFluid->getLevel() == 8) {
                // 放置方块
                world.setBlockState(currentPos, stateToPlace, 3);
                placedAny = true;
            }
        } else if (targetState != nullptr && targetState->is(VanillaBlocks::SEAGRASS)) {
            // 如果当前位置已经是海草，有 10% 概率让它生长
            if (random.nextInt(10) == 0) {
                // 检查海草是否可以生长（实现 IGrowable 接口）
                IGrowable* growable = const_cast<IGrowable*>(dynamic_cast<const IGrowable*>(&targetState->owner()));
                if (growable != nullptr && growable->canGrow(static_cast<IBlockReader&>(world), currentPos, *targetState, false)) {
                    growable->grow(world, random, currentPos, *targetState);
                    placedAny = true;
                }
            }
        }
    }

    return placedAny;
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
