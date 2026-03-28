#include "CactusBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../BlockRegistry.hpp"
#include "../../../../item/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/math/random/Random.hpp"

namespace mc {
namespace blocks {

// ========== 构造函数 ==========

CactusBlock::CactusBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::AGE_0_15())
        .create([this](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::AGE_0_15(), 0));

    // 仙人掌形状：稍小的方块
    CollisionShape cactusShape = CollisionShape::box(0.0625f, 0.0f, 0.0625f, 0.9375f, 1.0f, 0.9375f);
    for (int i = 0; i < 16; ++i) {
        m_shapesByAge[i] = cactusShape;
    }
}

// ========== 状态属性 ==========

i32 CactusBlock::getAge(const BlockState& state) const {
    return state.get(BlockStateProperties::AGE_0_15());
}

BlockState CactusBlock::withAge(i32 age) const {
    return defaultState().with(BlockStateProperties::AGE_0_15(), std::min(age, 15));
}

// ========== 放置逻辑 ==========

BlockState CactusBlock::getStateForPlacement(BlockItemUseContext& context) {
    return defaultState();
}

bool CactusBlock::isValidPosition(
    const BlockState& state,
    IBlockReader& world,
    const BlockPos& pos) const {

    MC_UNUSED(state);

    // 检查下方方块
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos.x, belowPos.y, belowPos.z);

    if (belowState == nullptr) {
        return false;
    }

    // 仙人掌可以放置在：仙人掌上、沙子上、红沙上
    // TODO: 使用更精确的方块检查
    const Material& material = belowState->getMaterial();
    if (material.isSolid() || belowState->is(this)) {
        // 检查周围是否有水或其他非空气方块
        // 仙人掌周围不能有固体方块
        for (Direction dir : {Direction::North, Direction::South, Direction::East, Direction::West}) {
            BlockPos adjPos = pos.offset(dir);
            const BlockState* adjState = world.getBlockState(adjPos.x, adjPos.y, adjPos.z);
            if (adjState != nullptr && !adjState->isAir() && adjState->getMaterial().isSolid()) {
                return false;
            }
        }
        return true;
    }

    return false;
}

BlockState CactusBlock::updatePostPlacement(
    const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos) {

    MC_UNUSED(world);
    MC_UNUSED(currentPos);
    MC_UNUSED(facingPos);

    // 检查周围是否有固体方块
    if (facing != Direction::Up && facing != Direction::Down) {
        if (facingState.getMaterial().isSolid()) {
            // 周围有固体方块，仙人掌应该被破坏 - 返回空气状态
            if (auto* airState = BlockRegistry::instance().airState()) {
                return *airState;
            }
        }
    }

    // 检查下方支撑
    if (facing == Direction::Down) {
        IBlockReader& blockReader = static_cast<IBlockReader&>(world);
        if (!isValidPosition(state, blockReader, currentPos)) {
            if (auto* airState = BlockRegistry::instance().airState()) {
                return *airState;
            }
        }
    }

    return state;
}

// ========== 生长逻辑 ==========

void CactusBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) {
    // 检查上方是否有空间
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos.x, abovePos.y, abovePos.z);

    if (aboveState != nullptr && !aboveState->isAir()) {
        return;
    }

    // 检查高度限制（最高3格）
    int height = 1;
    for (int i = 1; i < 3; ++i) {
        BlockPos checkPos(pos.x, pos.y - i, pos.z);
        const BlockState* checkState = world.getBlockState(checkPos.x, checkPos.y, checkPos.z);
        if (checkState == nullptr || !checkState->is(this)) {
            break;
        }
        height++;
    }

    if (height >= 3) {
        return;  // 已达到最高高度
    }

    // 随机生长
    if (random.nextInt(16) == 0) {
        i32 age = getAge(state);
        if (age >= 15) {
            // 生长新的仙人掌
            world.setBlockState(abovePos.x, abovePos.y, abovePos.z, &defaultState(), 2);
            world.setBlockState(pos.x, pos.y, pos.z, &withAge(0), 2);
        } else {
            // 增加年龄
            world.setBlockState(pos.x, pos.y, pos.z, &withAge(age + 1), 2);
        }
    }
}

// ========== 形状 ==========

const CollisionShape& CactusBlock::getShape(const BlockState& state) const {
    i32 age = getAge(state);
    return m_shapesByAge[std::min(age, 15)];
}

const CollisionShape& CactusBlock::getCollisionShape(const BlockState& state) const {
    return getShape(state);
}

// ========== 实体交互 ==========

void CactusBlock::onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) {
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(entity);

    // TODO: 对实体造成伤害
    // entity.hurt(DamageSource::CACTUS, 1.0f);
}

} // namespace blocks
} // namespace mc
