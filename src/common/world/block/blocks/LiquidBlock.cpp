#include "LiquidBlock.hpp"
#include "../Block.hpp"
#include "../VanillaBlocks.hpp"
#include "../../../util/property/Properties.hpp"
#include "../../../util/property/FluidProperties.hpp"
#include "../../IWorld.hpp"
#include "../../tick/manager/TickManager.hpp"
#include "../BlockPos.hpp"
#include "../../../util/Direction.hpp"
#include "../../fluid/FluidTags.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "../../../util/math/random/Random.hpp"
#include <functional>

namespace mc {
namespace block {

// ============================================================================
// LiquidBlock 实现
// ============================================================================

// 方块LEVEL属性 (0-15)
namespace {
    const IntegerProperty& LEVEL_0_15() {
        return BlockStateProperties::LEVEL_0_15();
    }
}

LiquidBlock::LiquidBlock(fluid::FlowingFluid& fluid, BlockProperties properties)
    : Block(properties)
    , m_fluid(fluid) {

    // 创建方块状态容器，包含LEVEL属性
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(LEVEL_0_15())
        .create([this](const Block& block, auto values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态（level=0，即源头）
    setDefaultState(defaultState().with(LEVEL_0_15(), 0));

    // 构建流体状态缓存
    buildFluidStateCache();
}

const mc::fluid::FluidState* LiquidBlock::getFluidState(const BlockState& state) const {
    i32 blockLevel = state.get(LEVEL_0_15());
    return &m_fluidStateCache[blockLevel];
}

const CollisionShape& LiquidBlock::getCollisionShape(const BlockState& state) const {
    // 液体方块没有碰撞形状
    (void)state;
    return VoxelShapes::empty();
}

bool LiquidBlock::ticksRandomly() const {
    return m_fluid.ticksRandomly();
}

void LiquidBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state,
                             math::IRandom& random) {
    const fluid::FluidState* fluidState = getFluidState(state);
    if (fluidState != nullptr && !fluidState->isEmpty()) {
        fluid::Fluid& fluidRef = const_cast<fluid::Fluid&>(fluidState->getFluid());
        fluidRef.randomTick(world, pos, *fluidState, random);
    }
}

void LiquidBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) {
    // 参考: net.minecraft.block.FlowingFluidBlock#onBlockAdded
    // 只有当 reactWithNeighbors 返回 true 时才调度流体 tick
    // 如果岩浆放置时旁边有水，会先反应变成石头/黑曜石，返回 false，不再调度 tick
    if (reactWithNeighbors(world, pos, state)) {
        const fluid::FluidState* fluidState = getFluidState(state);
        if (fluidState != nullptr && !fluidState->isEmpty()) {
            fluid::Fluid& fluidRef = const_cast<fluid::Fluid&>(fluidState->getFluid());
            world.tickManager().scheduleFluidTick(pos, fluidRef, fluidRef.getTickDelay(world));
        }
    }
}

void LiquidBlock::neighborChanged(IWorld& world, const BlockPos& pos,
                                   Block& neighborBlock, const BlockPos& neighborPos,
                                   bool isMoving) {
    // 参考: net.minecraft.block.FlowingFluidBlock#neighborChanged
    // 只有当 reactWithNeighbors 返回 true 时才调度流体 tick
    const BlockState* currentState = world.getBlockState(pos);
    if (currentState != nullptr) {
        // 首先检查是否发生了反应（岩浆+水）
        if (reactWithNeighbors(world, pos, *currentState)) {
            const fluid::FluidState* fluidState = currentState->getFluidState();
            if (fluidState != nullptr && !fluidState->isEmpty()) {
                fluid::Fluid& fluidRef = const_cast<fluid::Fluid&>(fluidState->getFluid());
                world.tickManager().scheduleFluidTick(pos, fluidRef, fluidRef.getTickDelay(world));
            }
        }
    }

    (void)neighborBlock;
    (void)neighborPos;
    (void)isMoving;
}

void LiquidBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state) {
    // 获取流体状态并调用流体的tick方法
    // 按当前状态实际归属的流体类型执行tick。
    const fluid::FluidState* fluidState = getFluidState(state);
    if (fluidState != nullptr && !fluidState->isEmpty()) {
        // 创建可变副本进行tick
        fluid::FluidState mutableState = *fluidState;
        fluid::Fluid& fluidRef = const_cast<fluid::Fluid&>(fluidState->getFluid());
        fluidRef.tick(world, pos, mutableState);
    }
}

BlockState LiquidBlock::updatePostPlacement(
    const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos) {
    // 参考: net.minecraft.block.FlowingFluidBlock#updatePostPlacement
    // MC行为：只有当当前流体是源头或邻居流体是源头时才调度tick
    // 如果反应发生则不调度

    (void)facingPos;

    // 检查岩浆水反应
    if (reactWithNeighbors(world, currentPos, state)) {
        // 反应没有发生，检查是否需要调度流体tick
        const fluid::FluidState* fluidState = getFluidState(state);
        if (fluidState != nullptr && !fluidState->isEmpty()) {
            // MC条件：当前流体是源头 或 邻居流体是源头
            bool currentIsSource = fluidState->isSource();
            bool neighborIsSource = false;

            const fluid::FluidState* neighborFluid = facingState.getFluidState();
            if (neighborFluid != nullptr && !neighborFluid->isEmpty()) {
                neighborIsSource = neighborFluid->isSource();
            }

            if (currentIsSource || neighborIsSource) {
                fluid::Fluid& fluidRef = const_cast<fluid::Fluid&>(fluidState->getFluid());
                world.tickManager().scheduleFluidTick(currentPos, fluidRef, fluidRef.getTickDelay(world));
            }
        }
    }

    return state;
}

bool LiquidBlock::reactWithNeighbors(IWorld& world, const BlockPos& pos, const BlockState& state) {
    // 参考: net.minecraft.block.LiquidBlock#reactWithNeighbors
    // 只处理岩浆方块

    const fluid::FluidState* fluidState = getFluidState(state);
    if (fluidState == nullptr || fluidState->isEmpty()) {
        return true;
    }

    // 检查是否是岩浆
    if (!fluidState->getFluid().isIn(fluid::FluidTags::LAVA())) {
        return true;
    }

    // 检查下方是否有灵魂土（用于玄武岩生成）
    const BlockState* belowState = world.getBlockState(pos.x, pos.y - 1, pos.z);
    bool hasSoulSoilBelow = (belowState != nullptr &&
        VanillaBlocks::SOUL_SOIL != nullptr &&
        belowState->is(VanillaBlocks::SOUL_SOIL));

    // 检查所有方向（除了下方）
    for (i32 i = 0; i < 6; ++i) {
        Direction dir = static_cast<Direction>(i);
        if (dir == Direction::Down) {
            continue;
        }

        BlockPos neighborPos = pos.offset(dir);
        const fluid::FluidState* neighborFluid = world.getFluidState(neighborPos);

        // 检查是否是水
        if (neighborFluid != nullptr && !neighborFluid->isEmpty() &&
            neighborFluid->getFluid().isIn(fluid::FluidTags::WATER())) {
            // 岩浆 + 水 -> 生成方块
            Block* resultBlock = nullptr;

            if (fluidState->isSource()) {
                // 源头岩浆 + 水 -> 黑曜石
                resultBlock = VanillaBlocks::OBSIDIAN;
            } else {
                // 流动岩浆 + 水 -> 圆石
                resultBlock = VanillaBlocks::COBBLESTONE;
            }

            if (resultBlock != nullptr) {
                world.setBlockState(pos, &resultBlock->defaultState(), 3);
                triggerMixEffects(world, pos);
                return false;
            }
        }

        // 检查蓝冰 + 灵魂土 -> 玄武岩
        if (hasSoulSoilBelow) {
            const BlockState* neighborBlock = world.getBlockState(neighborPos);
            if (neighborBlock != nullptr &&
                VanillaBlocks::BLUE_ICE != nullptr &&
                neighborBlock->is(VanillaBlocks::BLUE_ICE)) {
                // 岩浆 + 蓝冰 + 灵魂土 -> 玄武岩
                if (VanillaBlocks::BASALT != nullptr) {
                    world.setBlockState(pos, &VanillaBlocks::BASALT->defaultState(), 3);
                    triggerMixEffects(world, pos);
                    return false;
                }
            }
        }
    }

    return true;
}

void LiquidBlock::triggerMixEffects(IWorld& world, const BlockPos& pos) {
    // 参考: net.minecraft.block.LiquidBlock#triggerMixEffects
    // 播放嘶嘶声和烟雾粒子效果

    // 播放嘶嘶声
    world.playSound(
        ResourceLocation("minecraft:block.lava.extinguish"),
        sound::SoundCategory::Blocks,
        Vector3(pos.x + 0.5f, pos.y + 0.5f, pos.z + 0.5f),
        0.5f,  // 音量
        1.0f   // 音调
    );

    // 生成烟雾粒子 - 使用位置和索引派生确定性随机数
    for (i32 i = 0; i < 8; ++i) {
        u64 particleSeed = world.seed() ^ static_cast<u64>(std::hash<BlockPos>{}(pos)) ^ static_cast<u64>(i);
        math::Random random(particleSeed);
        f32 offsetX = random.nextFloat() * 0.6f - 0.3f;
        f32 offsetZ = random.nextFloat() * 0.6f - 0.3f;
        world.addParticle(
            client::renderer::trident::particle::ParticleTypeId::Smoke,
            Vector3(pos.x + 0.5f + offsetX, pos.y + 1.0f, pos.z + 0.5f + offsetZ),
            Vector3(0.0f, 0.1f, 0.0f)
        );
    }
}

fluid::Fluid* LiquidBlock::pickupFluid(IWorld& world, const BlockPos& pos, const BlockState& state) {
    // 参考: net.minecraft.block.LiquidBlock#pickupFluid
    // 只有源头方块可以被舀起

    i32 blockLevel = state.get(LEVEL_0_15());
    if (blockLevel == 0) {  // 源头
        // 移除流体方块
        if (VanillaBlocks::AIR != nullptr) {
            world.setBlockState(pos, &VanillaBlocks::AIR->defaultState(), 11);
        }
        return &m_fluid.getStill();
    }

    return nullptr;  // 非源头，无法舀起
}

i32 LiquidBlock::blockLevelToFluidLevel(i32 blockLevel) {
    // 方块level=0 -> 流体level=8（源头）
    // 方块level=1-7 -> 流体level=8-blockLevel（反向）
    // 方块level=8-15 -> 流体level=8（下落）
    if (blockLevel == 0) {
        return 8;  // 源头
    } else if (blockLevel >= 1 && blockLevel <= 7) {
        return 8 - blockLevel;  // 1->7, 2->6, ..., 7->1
    } else {
        return 8;  // 下落，level>=8
    }
}

i32 LiquidBlock::fluidLevelToBlockLevel(i32 fluidLevel, bool falling) {
    // 流体level=8, falling=false -> 方块level=0（源头）
    // 流体level=1-7 -> 方块level=8-fluidLevel
    // 流体level=8, falling=true -> 方块level=8（下落）
    if (falling) {
        return 8;
    } else if (fluidLevel == 8) {
        return 0;  // 源头
    } else {
        return 8 - fluidLevel;
    }
}

void LiquidBlock::buildFluidStateCache() {
    m_fluidStateCache.clear();
    m_fluidStateCache.reserve(16);

    auto& levelProp = fluid::FluidProperties::LEVEL_1_8();
    auto& fallingProp = fluid::FluidProperties::FALLING();

    for (i32 blockLevel = 0; blockLevel <= 15; ++blockLevel) {
        i32 fluidLevel = blockLevelToFluidLevel(blockLevel);
        bool falling = isFallingLevel(blockLevel);

        if (fluidLevel == 8 && !falling) {
            // 源头状态
            m_fluidStateCache.push_back(m_fluid.getStill().defaultState());
        } else {
            // 流动状态
            m_fluidStateCache.push_back(m_fluid.getFlowing().defaultState()
                .with(levelProp, fluidLevel).with(fallingProp, falling));
        }
    }
}

} // namespace block
} // namespace mc
