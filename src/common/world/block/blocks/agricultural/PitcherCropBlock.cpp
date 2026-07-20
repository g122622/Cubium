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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN THE EVENT OF THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "PitcherCropBlock.hpp"
#include "common/core/Constants.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/item/Items.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/blocks/agricultural/CropBlock.hpp"
#include "common/world/block/registry/TrailsBlocks.hpp"
#include "common/world/gamerule/GameRules.hpp"

namespace mc {
namespace blocks {

// 使用 BlockStateProperties 中的 DoubleBlockHalf
using DoubleBlockHalf = BlockStateProperties::DoubleBlockHalf;

// ========== 构造函数 ==========

PitcherCropBlock::PitcherCropBlock(const BlockProperties& properties)
    : DoublePlantBlock(properties)
{
    // 创建状态容器：AGE_0_4 + DOUBLE_BLOCK_HALF
    // 状态空间：5 * 2 = 10 种状态
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::AGE_0_4())
            .add(BlockStateProperties::DOUBLE_BLOCK_HALF())
            .create([this](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 默认状态：AGE=0, HALF=Lower
    setDefaultState(defaultState()
            .with(BlockStateProperties::AGE_0_4(), 0)
            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), DoubleBlockHalf::Lower));

    // 预计算形状
    // MC Java: SHAPE_BY_AGE 数组定义了每个年龄的总高度（像素）
    // aint = {0, 9, 11, 22, 26}
    // AGE 0: 半径 6px, 总高 4px (baseHeight=4, aint[0]=0)
    // AGE 1+: 半径 10px, baseHeight=6, aint[i] 额外高度
    constexpr f32 P = 1.0f / 16.0f;

    // 碰撞形状
    // AGE 0（鳞茎阶段）：半径 6px，高 3px
    m_bulbCollisionShape = CollisionShape::box(5.0f * P, 0.0f, 5.0f * P, 11.0f * P, 3.0f * P, 11.0f * P);
    // AGE 1-4（作物阶段）：半径 10px，高 5px
    m_cropCollisionShape = CollisionShape::box(3.0f * P, 0.0f, 3.0f * P, 13.0f * P, 5.0f * P, 13.0f * P);

    // 轮廓形状：按半部分和年龄索引
    // Lower: [age][0], Upper: [age][1]
    constexpr f32 bulbRadius = 6.0f * P;             // AGE 0 的半径
    constexpr f32 cropRadius = 10.0f * P;            // AGE 1-4 的半径
    constexpr f32 bulbMinXZ = 8.0f * P - bulbRadius; // 0.125
    constexpr f32 bulbMaxXZ = 8.0f * P + bulbRadius; // 0.875
    constexpr f32 cropMinXZ = 8.0f * P - cropRadius; // 0.0 + 3/16
    constexpr f32 cropMaxXZ = 8.0f * P + cropRadius; // 1.0 - 3/16

    // MC Java 的轮廓形状计算：
    // 年龄对应的总高度像素值: {0, 9, 11, 22, 26}
    // baseHeight = 4 (AGE 0) 或 6 (AGE 1+)
    // totalHeight = baseHeight + aint[age]
    // Lower 半部分: column(radius, -1, min(16, -1 + totalHeight))
    // Upper 半部分: column(radius, 0, max(0, -1 + totalHeight - 16))
    constexpr i32 aint[] = {0, 9, 11, 22, 26};
    constexpr i32 bulbBaseHeight = 4;
    constexpr i32 cropBaseHeight = 6;

    for (i32 age = 0; age <= MAX_AGE; ++age) {
        i32 baseHeight = (age == 0) ? bulbBaseHeight : cropBaseHeight;
        f32 radius = (age == 0) ? bulbRadius : cropRadius;
        f32 minXZ = (age == 0) ? bulbMinXZ : cropMinXZ;
        f32 maxXZ = (age == 0) ? bulbMaxXZ : cropMaxXZ;

        i32 totalHeight = baseHeight + aint[age];
        i32 lowerMaxY = std::min(16, -1 + totalHeight);
        i32 upperMaxY = std::max(0, -1 + totalHeight - 16);

        // Lower 半部分：y 从 -1 到 lowerMaxY
        // MC Java 使用 column(radius, -1, lowerMaxY)，对应 box 从
        // yMin = (-1)/16, yMax = lowerMaxY/16
        m_shapesByHalfAndAge[0][age] = CollisionShape::box(minXZ, -1.0f * P, minXZ, maxXZ, lowerMaxY * P, maxXZ);

        // Upper 半部分：y 从 0 到 upperMaxY
        // MC Java 使用 column(radius, 0, upperMaxY)
        m_shapesByHalfAndAge[1][age] = CollisionShape::box(minXZ, 0.0f, minXZ, maxXZ, upperMaxY * P, maxXZ);
    }
}

// ========== 状态属性 ==========

i32 PitcherCropBlock::getAge(const BlockState& state) const
{
    return state.get(BlockStateProperties::AGE_0_4());
}

bool PitcherCropBlock::isMaxAge(const BlockState& state) const
{
    return getAge(state) >= MAX_AGE;
}

const BlockState& PitcherCropBlock::withAge(i32 age) const
{
    i32 clampedAge = std::clamp(age, 0, MAX_AGE);
    return defaultState()
        .with(BlockStateProperties::AGE_0_4(), clampedAge)
        .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), DoubleBlockHalf::Lower);
}

// ========== 放置逻辑 ==========

BlockState PitcherCropBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 瓶草作物初始放置时为下半部分、年龄 0
    return defaultState();
}

bool PitcherCropBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    DoubleBlockHalf half = getHalf(state);

    if (half == DoubleBlockHalf::Upper) {
        // 上半部分必须位于下半部分之上
        const BlockPos belowPos = pos.down();
        const BlockState* belowState = world.getBlockState(belowPos);
        if (belowState == nullptr) {
            return false;
        }
        // 下方必须是同类型方块的下半部分
        return belowState->is(this) && getHalf(*belowState) == DoubleBlockHalf::Lower;
    } else {
        // 下半部分：光照检查 + 下方支撑检查
        // 仅下半部分需要检查光照（与 MC Java 对齐）
        // 检查光照：原始亮度 >= 8（存活所需最低光照，与 CropBlock.hasSufficientLight 一致）
        const i32 blockLight = static_cast<i32>(world.getBlockLight(pos));
        const i32 skyLight = static_cast<i32>(world.getSkyLight(pos));
        const i32 rawBrightness = std::max(blockLight, skyLight);
        if (rawBrightness < game::CROP_SURVIVAL_LIGHT_THRESHOLD) {
            return false;
        }
        return BushBlock::isValidPosition(state, world, pos);
    }
}

BlockState PitcherCropBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    if (isDouble(getAge(state))) {
        // 双格状态：委托给 DoublePlantBlock 的逻辑
        return DoublePlantBlock::updatePostPlacement(state, facing, facingState, world, currentPos, facingPos);
    } else {
        // 单格状态：仅检查存活条件
        if (!isValidPosition(state, static_cast<IBlockReader&>(world), currentPos)) {
            if (auto* airState = BlockRegistry::instance().airState()) {
                return *airState;
            }
        }
        return state;
    }
}

// ========== 生长逻辑 ==========

void PitcherCropBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 仅下半部分进行生长检查
    if (getHalf(state) != DoubleBlockHalf::Lower) {
        return;
    }

    // 已达最大年龄，不再生长
    if (isMaxAge(state)) {
        return;
    }

    // 光照检查
    if (!hasSufficientLight(world, pos)) {
        return;
    }

    // 计算生长概率（与普通作物相同）
    const f32 growthChance = std::max(1.0f, CropBlock::getGrowthChance(*this, static_cast<IBlockReader&>(world), pos));
    const i32 randomBound = static_cast<i32>(25.0f / growthChance) + 1;
    if (random.nextInt(randomBound) == 0) {
        const i32 newAge = getAge(state) + 1;
        if (canGrowInto(world, pos, newAge)) {
            // 设置下半部分状态
            const BlockState& lowerState = defaultState()
                                               .with(BlockStateProperties::AGE_0_4(), newAge)
                                               .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), DoubleBlockHalf::Lower);
            world.setBlockState(pos, &lowerState, 2);

            // 如果新年龄使植物变为双格，放置上半部分
            if (isDouble(newAge)) {
                BlockPos abovePos = pos.up();
                const BlockState& upperState =
                    defaultState()
                        .with(BlockStateProperties::AGE_0_4(), newAge)
                        .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), DoubleBlockHalf::Upper);
                world.setBlockState(abovePos, &upperState, 3);
            }
        }
    }
}

// ========== IGrowable 接口实现 ==========

bool PitcherCropBlock::canGrow(
    IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const
{
    MC_UNUSED(isClientSide);

    // 查找下半部分
    auto [lowerPos, lowerState] = getLowerHalf(static_cast<IWorld&>(const_cast<IBlockReader&>(world)), pos, state);
    if (lowerState == nullptr) {
        return false;
    }

    // 检查下半部分是否未成熟，且可以生长到下一阶段
    i32 currentAge = getAge(*lowerState);
    if (currentAge >= MAX_AGE) {
        return false;
    }

    i32 newAge = currentAge + 1;
    return canGrowInto(static_cast<IWorld&>(const_cast<IBlockReader&>(world)), lowerPos, newAge);
}

bool PitcherCropBlock::canUseBonemeal(
    IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const
{
    MC_UNUSED(world);
    MC_UNUSED(random);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    // 骨粉总是有效
    return true;
}

void PitcherCropBlock::grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(random);

    // 查找下半部分
    auto [lowerPos, lowerState] = getLowerHalf(world, pos, state);
    if (lowerState == nullptr) {
        return;
    }

    i32 currentAge = getAge(*lowerState);
    i32 newAge = std::min(currentAge + 1, MAX_AGE);

    if (!canGrowInto(world, lowerPos, newAge)) {
        return;
    }

    // 设置下半部分状态
    const BlockState& newLowerState = defaultState()
                                          .with(BlockStateProperties::AGE_0_4(), newAge)
                                          .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), DoubleBlockHalf::Lower);
    world.setBlockState(lowerPos, &newLowerState, 2);

    // 如果新年龄使植物变为双格，放置上半部分
    if (isDouble(newAge)) {
        BlockPos abovePos = lowerPos.up();
        const BlockState& upperState = defaultState()
                                           .with(BlockStateProperties::AGE_0_4(), newAge)
                                           .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), DoubleBlockHalf::Upper);
        world.setBlockState(abovePos, &upperState, 3);
    }
}

// ========== 形状 ==========

const CollisionShape& PitcherCropBlock::getShape(const BlockState& state) const
{
    i32 age = getAge(state);
    DoubleBlockHalf half = getHalf(state);
    size_t halfIdx = (half == DoubleBlockHalf::Upper) ? 1 : 0;

    MC_ASSERT_RELEASE(age >= 0 && age <= MAX_AGE);
    return m_shapesByHalfAndAge[halfIdx][age];
}

const CollisionShape& PitcherCropBlock::getCollisionShape(const BlockState& state) const
{
    // 上半部分无碰撞
    if (getHalf(state) == DoubleBlockHalf::Upper) {
        return VoxelShapes::empty();
    }

    // 下半部分：AGE 0 用鳞茎形状，AGE 1-4 用作物形状
    i32 age = getAge(state);
    MC_ASSERT_RELEASE(age >= 0 && age <= MAX_AGE);
    return (age == 0) ? m_bulbCollisionShape : m_cropCollisionShape;
}

// ========== 掉落物 ==========

u32 PitcherCropBlock::getCropItem() const
{
    // 成熟时掉落瓶草植物
    return Items::PITCHER_PLANT->itemId();
}

u32 PitcherCropBlock::getSeedItem() const
{
    // 种子为瓶草荚果
    return Items::PITCHER_POD->itemId();
}

// ========== 其他 ==========

bool PitcherCropBlock::canSustain(const BlockState& groundState, IWorld& world, const BlockPos& groundPos) const
{
    MC_UNUSED(world);
    MC_UNUSED(groundPos);
    // 瓶草作物只能放置在耕地上
    // 委托给下方方块的 canSustainPlant 方法，与 CropBlock 一致
    // getPlantType() 返回 PlantType::Crop，Block::canSustainPlant 的 Crop 分支只接受 Farmland
    const Block& groundBlock = groundState.getBlock();
    return groundBlock.canSustainPlant(groundState, static_cast<IBlockReader&>(world), groundPos, Direction::Up, *this);
}

void PitcherCropBlock::playerWillDestroy(IWorld& world, const BlockPos& pos, const BlockState& state, Player& player)
{
    DoubleBlockHalf half = getHalf(state);

    if (half == DoubleBlockHalf::Upper) {
        // 破坏上半部分时，同时清除下半部分
        BlockPos belowPos = pos.down();
        const BlockState* belowState = world.getBlockState(belowPos);
        if (belowState != nullptr && belowState->is(this) && getHalf(*belowState) == DoubleBlockHalf::Lower) {
            // 创造模式下直接清除下半部分（不掉落）
            if (player.isCreative()) {
                world.setBlockState(belowPos, BlockRegistry::instance().airState(), 35);
            } else {
                // 生存模式：清除下半部分（触发掉落）
                world.setBlockState(belowPos, BlockRegistry::instance().airState(), 3);
            }
        }
    } else {
        // 破坏下半部分时，同时清除上半部分
        if (isDouble(getAge(state))) {
            BlockPos abovePos = pos.up();
            const BlockState* aboveState = world.getBlockState(abovePos);
            if (aboveState != nullptr && aboveState->is(this) &&
                aboveState->get(BlockStateProperties::DOUBLE_BLOCK_HALF()) == DoubleBlockHalf::Upper) {
                world.setBlockState(abovePos, BlockRegistry::instance().airState(), 35);
            }
        }
    }

    // 调用基类处理
    DoublePlantBlock::playerWillDestroy(world, pos, state, player);
}

void PitcherCropBlock::onEntityCollision(
    const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    // 仅服务端执行（对应 MC Java: world instanceof ServerLevel）
    if (world.isClientSide()) {
        return;
    }

    // 仅 Ravager 触发破坏（对应 MC Java: entity instanceof Ravager）
    if (entity.entityType() != entity::VanillaEntityTypeKeys::RAVAGER) {
        return;
    }

    // 检查 mobGriefing 游戏规则（对应 MC Java: serverLevel.getGameRules().getBoolean(MOB_GRIEFING)）
    if (!world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING)) {
        return;
    }

    // 破坏方块（对应 MC Java: serverLevel.destroyBlock(pos, true, entity)）
    // 项目无 IWorld::destroyBlock，采用 setBlockState(air) + spawnAfterBreak 的等价模式，
    // 与 RavagerEntity::_breakLeavesOnCollision / EnderDragonEntity::_destroyBlocksInAABB 一致：
    // - setBlockState(pos, air, 3)：将方块设为空气并触发邻居更新
    // - spawnAfterBreak：触发方块掉落物（由战利品表控制），第二个参数 true 表示掉落物品
    const BlockState* airState = BlockRegistry::instance().airState();
    if (airState == nullptr) {
        return;
    }

    // 保存方块对象引用，因为 setBlockState 后 state 可能失效
    const Block& brokenBlock = state.getBlock();

    world.setBlockState(pos, airState, 3);
    brokenBlock.spawnAfterBreak(world, pos, state, nullptr, true);
}

bool PitcherCropBlock::isReplaceable(const BlockState& state, const BlockItemUseContext& context) const
{
    // 瓶草作物不可被替换放置
    MC_UNUSED(context);
    MC_UNUSED(state);
    return false;
}

PlantType PitcherCropBlock::getPlantType(IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return PlantType::Crop;
}

// ========== 静态方法 ==========

bool PitcherCropBlock::isDouble(i32 age)
{
    return age >= DOUBLE_PLANT_AGE_INTERSECTION;
}

bool PitcherCropBlock::placeAt(IWorld& world, const BlockPos& pos, i32 age, i32 flags)
{
    // 创建下半部分状态
    const BlockState& lowerState = block_registry::TrailsBlocks::PITCHER_CROP->defaultState()
                                       .with(BlockStateProperties::AGE_0_4(), age)
                                       .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), DoubleBlockHalf::Lower);

    // 设置下半部分
    if (!world.setBlockState(pos, &lowerState, flags)) {
        return false;
    }

    // 如果是双格状态，放置上半部分
    if (isDouble(age)) {
        BlockPos abovePos = pos.up();
        const BlockState& upperState = block_registry::TrailsBlocks::PITCHER_CROP->defaultState()
                                           .with(BlockStateProperties::AGE_0_4(), age)
                                           .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), DoubleBlockHalf::Upper);
        world.setBlockState(abovePos, &upperState, flags);
    }

    return true;
}

// ========== 私有方法 ==========

bool PitcherCropBlock::canGrowInto(IWorld& world, const BlockPos& pos, i32 newAge) const
{
    if (isDouble(newAge)) {
        // 双格状态需要检查上方是否有空间
        BlockPos abovePos = pos.up();
        const BlockState* aboveState = world.getBlockState(abovePos);
        // 上方可以是空气或是同类型的瓶草作物
        return aboveState == nullptr || aboveState->isAir() || aboveState->is(this);
    }
    return true;
}

bool PitcherCropBlock::hasSufficientLight(IWorld& world, const BlockPos& pos)
{
    // 光照检查：原始亮度 >= 8
    return world.getLightSubtracted(pos, 0) >= 8;
}

std::pair<BlockPos, const BlockState*> PitcherCropBlock::getLowerHalf(
    IWorld& world, const BlockPos& pos, const BlockState& state)
{
    DoubleBlockHalf half = state.get(BlockStateProperties::DOUBLE_BLOCK_HALF());

    if (half == DoubleBlockHalf::Lower) {
        return {pos, &state};
    }

    // 上半部分：查找下方
    BlockPos belowPos = pos.down();
    const BlockState* belowState = world.getBlockState(belowPos);
    if (belowState != nullptr && belowState->is(block_registry::TrailsBlocks::PITCHER_CROP) &&
        belowState->get(BlockStateProperties::DOUBLE_BLOCK_HALF()) == DoubleBlockHalf::Lower) {
        return {belowPos, belowState};
    }

    return {pos, nullptr};
}

} // namespace blocks
} // namespace mc
