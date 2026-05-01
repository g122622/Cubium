#include "FarmlandBlock.hpp"
#include "CropBlock.hpp"
#include "StemBlock.hpp"
#include "../../VanillaBlocks.hpp"
#include "../../../IWorld.hpp"
#include "../../../tick/manager/TickManager.hpp"
#include "../../../fluid/Fluid.hpp"
#include "../../../fluid/FluidTags.hpp"
#include "../../PlantType.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../util/assert/AssertAll.hpp"
#include "../../../../entity/core/Entity.hpp"

namespace mc {
namespace blocks {

// ========== 构造函数 ==========

FarmlandBlock::FarmlandBlock(const BlockProperties& properties)
    : Block(properties)
    , m_shape(CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 15.0f / 16.0f, 1.0f)) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::MOISTURE_0_7())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::MOISTURE_0_7(), 0));
}

// ========== 放置和更新 ==========

BlockState FarmlandBlock::getStateForPlacement(BlockItemUseContext& context) {
    // 检查是否可以放置
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    // 上方不能有固体方块
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);

    if (aboveState != nullptr && aboveState->hasOpaqueCollisionShape()) {
        // 不能放置，返回泥土状态
        if (VanillaBlocks::DIRT != nullptr) {
            return VanillaBlocks::DIRT->defaultState();
        }
        return defaultState();
    }

    return defaultState();
}

bool FarmlandBlock::isValidPosition(
    const BlockState& state,
    IBlockReader& world,
    const BlockPos& pos) const {

    MC_UNUSED(state);

    // 上方不能有固体方块
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);

    if (aboveState != nullptr && aboveState->hasOpaqueCollisionShape()) {
        return false;
    }

    return true;
}

BlockState FarmlandBlock::updatePostPlacement(
    const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos) {

    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 上方方块更新时检查有效性
    if (facing == Direction::Up) {
        BlockPos abovePos(currentPos.x, currentPos.y + 1, currentPos.z);
        const BlockState* aboveState = world.getBlockState(abovePos);
        if (aboveState != nullptr && aboveState->hasOpaqueCollisionShape()) {
            // 安排下一 tick 转变为泥土
            world.tickManager().scheduleBlockTick(currentPos, *this, 1);
        }
    }

    return state;
}

// ========== Tick ==========

void FarmlandBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state) {
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);
    if (aboveState != nullptr && aboveState->hasOpaqueCollisionShape()) {
        turnToDirt(world, pos, state);
    }
}

void FarmlandBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) {
    MC_UNUSED(random);

    int moisture = state.get(BlockStateProperties::MOISTURE_0_7());

    // 检查附近是否有水
    bool nearWater = hasWater(world, pos);
    const bool raining = world.isRaining() && world.canRainAt(pos.up());

    if (!nearWater && !raining) {
        // 没有水且不下雨，湿润度降低
        if (moisture > 0) {
            world.setBlockState(pos, &state.with(BlockStateProperties::MOISTURE_0_7(), moisture - 1), 2);
        } else if (!hasCrops(world, pos)) {
            // 没有作物且干燥，转变为泥土
            turnToDirt(world, pos, state);
        }
    } else if (moisture < 7) {
        // 有水或下雨，增加湿润度
        world.setBlockState(pos, &state.with(BlockStateProperties::MOISTURE_0_7(), 7), 2);
    }
}

// ========== 形状 ==========

const CollisionShape& FarmlandBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    return m_shape;
}

const CollisionShape& FarmlandBlock::getCollisionShape(const BlockState& state) const {
    // 碰撞箱是完整方块
    MC_UNUSED(state);
    static CollisionShape fullShape = CollisionShape::fullBlock();
    return fullShape;
}

// ========== 其他重写 ==========

bool FarmlandBlock::allowsMovement(
    const BlockState& state,
    IBlockReader& world,
    const BlockPos& pos) const {
    // 参考 MC 1.16.5: FarmlandBlock.allowsMovement
    // 耕地不允许路径寻找（实体不会穿越耕地）
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return false;
}

void FarmlandBlock::onFallenUpon(
    IWorld& world,
    const BlockPos& pos,
    const BlockState& state,
    Entity& entity,
    f32 fallDistance) {

    // 参考 MC 1.16.5: FarmlandBlock.onFallenUpon
    // 如果实体从高处落下，耕地会变成泥土
    // MC 使用 ForgeHooks.onFarmlandTrample 判断是否应该踩踏
    // 踩踏条件：fallDistance > 1.0f 且实体不是飞行或创造模式

    // 简化实现：如果落下距离 > 1.0，则踩踏耕地
    if (!world.isRemote() && fallDistance > 1.0f) {
        turnToDirt(world, pos, state);
    }

    // 调用父类方法处理实体着地
    Block::onFallenUpon(world, pos, state, entity, fallDistance);
}

// ========== 工具方法 ==========

void FarmlandBlock::turnToDirt(IWorld& world, const BlockPos& pos, const BlockState& state) {
    MC_UNUSED(state);
    if (VanillaBlocks::DIRT != nullptr) {
        world.setBlockState(pos, &VanillaBlocks::DIRT->defaultState(), 3);
    }
}

bool FarmlandBlock::hasWater(IWorld& world, const BlockPos& pos) {
    // 参考 MC 1.16.5: FarmlandBlock.hasWater
    // 检查 4 格范围内的水，高度范围 0-1
    // 使用流体标签检测，而不是材质
    for (int dx = -4; dx <= 4; ++dx) {
        for (int dz = -4; dz <= 4; ++dz) {
            for (int dy = 0; dy <= 1; ++dy) {
                BlockPos checkPos(pos.x + dx, pos.y + dy, pos.z + dz);
                const fluid::FluidState* fluidState = world.getFluidState(checkPos);
                if (fluidState != nullptr && !fluidState->isEmpty()) {
                    // 使用流体标签检测水
                    if (fluidState->getFluid().isIn(fluid::FluidTags::WATER())) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool FarmlandBlock::hasCrops(IWorld& world, const BlockPos& pos) {
    // 参考 MC 1.16.5: FarmlandBlock.hasCrops
    // 检查上方是否有作物
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);

    if (aboveState == nullptr) {
        return false;
    }

    const Block& aboveBlock = aboveState->getBlock();

    // 检查方块是否实现了 IPlantable 接口
    const IPlantable* plantable = dynamic_cast<const IPlantable*>(&aboveBlock);
    if (plantable != nullptr) {
        // 耕地可以支撑 IPlantable 类型的植物
        // 注：原始 MC 使用 canSustainPlant 方法，但这里简化处理
        return true;
    }

    // 后备检查：使用 AGE_0_7 属性检测作物
    // 这是为了兼容没有实现 IPlantable 的作物
    return aboveState->hasProperty(BlockStateProperties::AGE_0_7());
}

} // namespace blocks
} // namespace mc
