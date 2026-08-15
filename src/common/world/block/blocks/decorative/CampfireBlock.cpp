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

#include "CampfireBlock.hpp"
#include "../../../../entity/entities/player/Player.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../item/core/ActionResult.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../physics/shape/BooleanOp.hpp"
#include "../../../../physics/shape/Shapes.hpp"
#include "../../../../physics/shape/VoxelShape.hpp"
#include "../../../../sound/SoundCategory.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../stats/Stats.hpp"
#include "../../../../util/assert/AssertAll.hpp"
#include "../../../IWorld.hpp"
#include "../../../block/BlockTags.hpp"
#include "../../../blockentity/BlockEntity.hpp"
#include "../../../blockentity/processing/CampfireBlockEntity.hpp"
#include "../../WaterLoggableHelpers.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/EquipmentSlot.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ========== CampfireBlock 实现 ==========

CampfireBlock::CampfireBlock(BlockProperties properties, u8 lightValue)
    : Block(std::move(properties))
    , m_lightValue(lightValue)
{
    // 创建状态容器
    // 营火有 LIT, SIGNAL_FIRE, WATERLOGGED, FACING 四个属性
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::LIT())
            .add(BlockStateProperties::SIGNAL_FIRE())
            .add(BlockStateProperties::WATERLOGGED())
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
            .with(BlockStateProperties::LIT(), true)
            .with(BlockStateProperties::SIGNAL_FIRE(), false)
            .with(BlockStateProperties::WATERLOGGED(), false)
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North));

    // 营火形状（略小于完整方块）
    m_shape = CollisionShape::box(0.0f, 0.0f, 0.0f, 16.0f, 7.0f, 16.0f);
}

BlockState CampfireBlock::getStateForPlacement(BlockItemUseContext& context)
{
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    // 检查是否含水
    bool waterlogged = waterloggable::shouldWaterlogAt(world, pos);

    // 如果在水中，默认不点燃
    bool lit = !waterlogged;

    // 检查下方是否是干草块（信号火）
    bool signalFire = _isHayBlock(const_cast<IWorld&>(world), pos);

    // 获取放置朝向
    Direction facing = context.horizontalDirection();

    return defaultState()
        .with(BlockStateProperties::LIT(), lit)
        .with(BlockStateProperties::SIGNAL_FIRE(), signalFire)
        .with(BlockStateProperties::WATERLOGGED(), waterlogged)
        .with(BlockStateProperties::HORIZONTAL_FACING(), facing);
}

BlockState CampfireBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 处理含水状态
    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

    // 当下方方块变化时，检查是否需要更新信号火状态
    if (facing == Direction::Down) {
        bool signalFire = _isHayBlock(world, currentPos);
        if (state.get(BlockStateProperties::SIGNAL_FIRE()) != signalFire) {
            return state.with(BlockStateProperties::SIGNAL_FIRE(), signalFire);
        }
    }

    return state;
}

void CampfireBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);
    // 如果被水淹没，熄灭
    if (state.get(BlockStateProperties::WATERLOGGED())) {
        extinguish(world, pos, state);
        return;
    }

    // 注意：营火不会因为雨天而熄灭，这是普通火焰(FireBlock)的行为
    // 营火的熄灭方式只有：水接触、铲子右键、喷溅型水瓶

    // 烹饪逻辑由 CampfireBlockEntity.tick() 处理
}

std::unique_ptr<BlockEntity> CampfireBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::CampfireBlockEntity>(pos);
}

const CollisionShape& CampfireBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

u8 CampfireBlock::getLightLevel(const BlockState& state, IWorld* world, const BlockPos* pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 点燃时发出光照，熄灭时不发光
    if (isLit(state)) {
        return m_lightValue;
    }
    return 0;
}

BlockActionResult CampfireBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(state);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    // 玩家右键点击营火时，尝试添加食物进行烹饪

    if (world.isClientSide()) {
        return ActionResultType::Success;
    }

    // 获取营火方块实体
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity == nullptr || blockEntity->getType() != BlockEntityType::Campfire) {
        return ActionResultType::Pass;
    }

    auto* campfire = static_cast<blockentity::CampfireBlockEntity*>(blockEntity);

    // 获取玩家手中的物品
    ItemStack& heldItem = player.getHeldItem(hand);

    // 查找匹配的营火烹饪配方
    auto recipeResult = campfire->findMatchingRecipe(heldItem);
    if (recipeResult.has_value()) {
        const crafting::CampfireCookingRecipe* recipe = recipeResult->first;
        i32 cookTime = recipeResult->second;

        // 添加物品到营火
        // 创造模式传入副本，生存模式传入原物品
        if (campfire->addItem(heldItem, cookTime)) {
            // 成功添加，触发营火交互统计
            player.awardCustomStat(ResourceLocation(stats::INTERACT_WITH_CAMPFIRE), 1);
            return ActionResultType::Success;
        }
    }

    return ActionResultType::Pass;
}

void CampfireBlock::onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    MC_UNUSED(pos);

    // 熄灭的营火不造成伤害
    if (!isLit(state)) {
        return;
    }

    // 仅服务端执行伤害（避免客户端重复伤害）
    if (world.isClientSide()) {
        return;
    }

    // 只对 LivingEntity 生效（掉落物、经验球、投射物等非生物不受伤害）
    auto* livingEntity = dynamic_cast<LivingEntity*>(&entity);
    if (livingEntity == nullptr) {
        return;
    }

    // 冰霜行者靴子免疫营火伤害（对齐 wiki）。
    // 检查靴子槽（EquipmentSlot::Feet）单件物品是否带 frost_walker 附魔。
    const ItemStack& boots = livingEntity->getEquipment(EquipmentSlot::Feet);
    if (item::enchant::EnchantmentHelper::hasFrostWalker(boots)) {
        return;
    }

    // 伤害量：灵魂营火（m_lightValue==10）hp2，普通营火（m_lightValue==15）hp1。
    // 对齐 wiki：灵魂营火伤害为营火的两倍（tech_灵魂营火.txt 历史 20w22a）。
    // 伤害源类型 Campfire 是火焰伤害（DamageSource::isFire()==true），不绕过无敌帧
    // （bypassesInvulnerability()==false），故 LivingEntity::hurt 内的受击免疫逻辑
    // 会使每 tick 的 hurt 调用节流为约每 10 tick（半秒）实际生效一次，与 wiki 一致。
    // 注意：1.19.60+ 营火不再引燃实体（移除 setOnFire），仅造成即时火焰伤害。
    const f32 damageAmount = (m_lightValue == 10) ? 2.0f : 1.0f;
    auto damageSource = DamageSources::campfire();
    livingEntity->hurt(damageSource, damageAmount);
}

void CampfireBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 方块被移除时，掉落所有烹饪中的物品

    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity != nullptr && blockEntity->getType() == BlockEntityType::Campfire) {
        auto* campfire = static_cast<blockentity::CampfireBlockEntity*>(blockEntity);
        campfire->dropAllItems(world);
    }

    Block::onBlockRemoved(world, pos, state);
}

const BlockState& CampfireBlock::rotate(const BlockState& state, Rotation rotation) const
{
    // 根据旋转改变朝向
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction newFacing = facing;

    switch (rotation) {
        case Rotation::Clockwise90:
            newFacing = Directions::rotateY(facing);
            break;
        case Rotation::Clockwise180:
            newFacing = Directions::opposite(facing);
            break;
        case Rotation::CounterClockwise90:
            newFacing = Directions::rotateYCCW(facing);
            break;
        case Rotation::None:
        default:
            break;
    }

    return state.with(BlockStateProperties::HORIZONTAL_FACING(), newFacing);
}

const BlockState& CampfireBlock::mirror(const BlockState& state, Mirror mirror) const
{
    // 根据镜像改变朝向
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction newFacing = facing;

    switch (mirror) {
        case Mirror::LeftRight:
            // 南北镜像：东西互换
            if (facing == Direction::East) {
                newFacing = Direction::West;
            } else if (facing == Direction::West) {
                newFacing = Direction::East;
            }
            break;
        case Mirror::FrontBack:
            // 前后镜像：南北互换
            if (facing == Direction::North) {
                newFacing = Direction::South;
            } else if (facing == Direction::South) {
                newFacing = Direction::North;
            }
            break;
        case Mirror::None:
        default:
            break;
    }

    return state.with(BlockStateProperties::HORIZONTAL_FACING(), newFacing);
}

void CampfireBlock::light(IWorld& world, const BlockPos& pos, BlockState& state)
{
    if (!isLit(state) && !state.get(BlockStateProperties::WATERLOGGED())) {
        BlockState newState = state.with(BlockStateProperties::LIT(), true);
        world.setBlockState(pos, &newState, 3);
        // 点燃音效使用通用的火焰点燃声，此处不播放特定音效
        // 粒子效果由客户端渲染器处理
    }
}

void CampfireBlock::extinguish(IWorld& world, const BlockPos& pos, BlockState& state)
{
    if (isLit(state)) {
        BlockState newState = state.with(BlockStateProperties::LIT(), false);
        world.setBlockState(pos, &newState, 3);

        // 熄灭时播放音效
        if (!world.isClientSide()) {
            world.playSound(
                SoundEvents::ENTITY_GENERIC_EXTINGUISH_FIRE, sound::SoundCategory::Blocks, pos.center(), 1.0f, 1.0f);
        }
    }
}

bool CampfireBlock::_isHayBlock(IWorld& world, const BlockPos& pos) const
{
    // 检查下方是否是干草块
    BlockPos belowPos = pos.down();
    const BlockState* belowState = world.getBlockState(belowPos);
    if (belowState == nullptr) {
        return false;
    }

    // 检查是否是干草块
    return &belowState->getBlock() == VanillaBlocks::HAY_BLOCK;
}

// ========== IWaterLoggable 接口实现 ==========

const fluid::FluidState* CampfireBlock::getFluidState(const BlockState& state) const
{
    const fluid::FluidState* waterState = waterloggable::getWaterFluidState(state);
    return waterState != nullptr ? waterState : Block::getFluidState(state);
}

const VoxelShape& CampfireBlock::getVirtualPostShape()
{
    // 对应 MC Java 的 CampfireBlock.SHAPE_VIRTUAL_POST = Block.column(4.0, 0.0, 16.0)
    // Block.column(4.0, 0.0, 16.0) -> column(4.0, 4.0, 0.0, 16.0) -> box(8-2, 0, 8-2, 8+2, 16, 8+2)
    // = box(6, 0, 6, 10, 16, 10) in pixel coordinates
    // = box(0.375, 0.0, 0.375, 0.625, 1.0, 0.625) in block-local coordinates
    // 这是一个 4x16x4 像素的中心柱，用于检测烟雾是否被方块碰撞形状阻挡
    static const VoxelShape s_virtualPost = Shapes::box(0.375, 0.0, 0.375, 0.625, 1.0, 0.625);
    return s_virtualPost;
}

bool CampfireBlock::isSmokeyPos(IWorld& world, const BlockPos& pos)
{
    const VoxelShape& virtualPost = getVirtualPostShape();

    for (i32 i = 1; i <= 5; ++i) {
        BlockPos checkPos(pos.x, pos.y - i, pos.z);
        const BlockState* state = world.getBlockState(checkPos);
        if (state && isLitCampfire(*state)) {
            return true;
        }

        // 使用虚拟烟雾柱与方块碰撞形状的交集检测，对齐 MC Java 原版逻辑：
        // Shapes.joinIsNotEmpty(SHAPE_VIRTUAL_POST, blockstate.getCollisionShape(...), BooleanOp.AND)
        // 当虚拟柱与方块碰撞形状有交集时，表示烟雾被该方块阻挡
        if (state) {
            const CollisionShape& collisionShape = state->getCollisionShape();
            VoxelShape blockShape = Shapes::fromCollisionShape(collisionShape);
            bool blocked = Shapes::joinIsNotEmpty(virtualPost, blockShape, BooleanOps::And());
            if (blocked) {
                // 烟雾被阻挡，检查阻挡方块下方是否有点燃的营火
                BlockPos belowPos(pos.x, pos.y - i - 1, pos.z);
                const BlockState* belowState = world.getBlockState(belowPos);
                return belowState && isLitCampfire(*belowState);
            }
        }
    }
    return false;
}

bool CampfireBlock::isLitCampfire(const BlockState& state)
{
    return BlockTags::CAMPFIRES().contains(state) && state.hasProperty(BlockStateProperties::LIT()) &&
        state.get(BlockStateProperties::LIT());
}

// ========== SoulCampfireBlock 实现 ==========

SoulCampfireBlock::SoulCampfireBlock(BlockProperties properties)
    : CampfireBlock(std::move(properties), 10) // 灵魂营火光照等级为10
{}

} // namespace blocks
} // namespace mc
