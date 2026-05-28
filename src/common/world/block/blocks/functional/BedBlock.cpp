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

#include "BedBlock.hpp"
#include "../../../../entity/player/SleepManager.hpp"
#include "../../../../entity/player/SleepResult.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../resource/ResourceLocation.hpp"
#include "../../../../sound/SoundCategory.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/assert/AssertAll.hpp"
#include "../../../IWorld.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../../dimension/DimensionType.hpp"
#include "../../../explosion/ExplosionMode.hpp"
#include <unordered_map>

namespace mc {
namespace blocks {

// ========== BedBlock 实现 ==========

BedBlock::BedBlock(u32 color, const BlockProperties& properties)
    : Block(properties)
    , m_color(color)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .add(BlockStateProperties::BED_PART())
            .add(BlockStateProperties::OCCUPIED())
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
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
            .with(BlockStateProperties::BED_PART(), BlockStateProperties::BedPart::Foot)
            .with(BlockStateProperties::OCCUPIED(), false));

    // 预计算各朝向的形状
    // 床的形状：主体 + 四个床腿
    constexpr f32 P = 1.0f / 16.0f;

    // 主体形状 (高度9像素，从Y=3开始)
    CollisionShape baseShape = CollisionShape::box(0.0f, 3.0f * P, 0.0f, 16.0f * P, 9.0f * P, 16.0f * P);

    // 床腿形状
    CollisionShape legNW = CollisionShape::box(0.0f, 0.0f, 0.0f, 3.0f * P, 3.0f * P, 3.0f * P);
    CollisionShape legNE = CollisionShape::box(13.0f * P, 0.0f, 0.0f, 16.0f * P, 3.0f * P, 3.0f * P);
    CollisionShape legSW = CollisionShape::box(0.0f, 0.0f, 13.0f * P, 3.0f * P, 3.0f * P, 16.0f * P);
    CollisionShape legSE = CollisionShape::box(13.0f * P, 0.0f, 13.0f * P, 16.0f * P, 3.0f * P, 16.0f * P);

    // 北朝向形状 (头部在南)
    m_shapesByFacing[static_cast<size_t>(Direction::North)] =
        CollisionShape::combine(CollisionShape::combine(CollisionShape::combine(baseShape, legNW), legNE),
            CollisionShape::combine(legSW, legSE));

    // 南朝向形状 (头部在北)
    m_shapesByFacing[static_cast<size_t>(Direction::South)] =
        CollisionShape::combine(CollisionShape::combine(CollisionShape::combine(baseShape, legNW), legNE),
            CollisionShape::combine(legSW, legSE));

    // 西朝向形状 (头部在东)
    m_shapesByFacing[static_cast<size_t>(Direction::West)] =
        CollisionShape::combine(CollisionShape::combine(CollisionShape::combine(baseShape, legNW), legNE),
            CollisionShape::combine(legSW, legSE));

    // 东朝向形状 (头部在西)
    m_shapesByFacing[static_cast<size_t>(Direction::East)] =
        CollisionShape::combine(CollisionShape::combine(CollisionShape::combine(baseShape, legNW), legNE),
            CollisionShape::combine(legSW, legSE));
}

BlockState BedBlock::getStateForPlacement(BlockItemUseContext& context)
{
    Direction facing = context.horizontalDirection();
    BlockPos pos = context.blockPos();
    BlockPos headPos(pos.x + Directions::xOffset(facing), pos.y, pos.z + Directions::zOffset(facing));

    // 检查头部位置是否可放置
    const BlockState* headState = context.getWorld().getBlockState(headPos);
    if (headState != nullptr && headState->isAir()) {
        return defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), facing);
    }

    // 无法放置完整的床
    return defaultState();
}

BlockState BedBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(world);
    MC_UNUSED(currentPos);
    MC_UNUSED(facingPos);

    BlockStateProperties::BedPart part = state.get(BlockStateProperties::BED_PART());
    Direction bedFacing = state.get(BlockStateProperties::HORIZONTAL_FACING());

    // 计算另一部分的方向
    Direction otherDir = (part == BlockStateProperties::BedPart::Foot) ? bedFacing : Directions::opposite(bedFacing);

    // 配对半床缺失时当前床也必须立刻消失。这里不依赖通知方向，避免测试和实际邻接更新遗漏。
    if (facingState.isAir()) {
        return VanillaBlocks::AIR->defaultState();
    }

    if (facing == otherDir && facingState.hasProperty(BlockStateProperties::OCCUPIED())) {
        return state.with(BlockStateProperties::OCCUPIED(), facingState.get(BlockStateProperties::OCCUPIED()));
    }

    return state;
}

const CollisionShape& BedBlock::getShape(const BlockState& state) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    size_t index = static_cast<size_t>(facing);
    MC_ASSERT(index < Directions::COUNT && Directions::isHorizontal(facing));
    return m_shapesByFacing[index];
}

void BedBlock::setOccupied(IWorld& world, const BlockPos& pos, BlockState& state, bool occupied)
{
    if (state.hasProperty(BlockStateProperties::OCCUPIED())) {
        world.setBlockState(pos, &state.with(BlockStateProperties::OCCUPIED(), occupied), 2);
    }
}

bool BedBlock::isBed(IWorld& world, const BlockPos& pos)
{
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return false;
    }
    // 检查是否为床方块：通过检查 BED_PART 属性判断
    // 只有床方块才有 BED_PART 属性，因此这种方式是可靠的
    // 参考 SpawnPointValidator::isBed() 和 POITypeHelper::isBed()
    return state->hasProperty(BlockStateProperties::BED_PART());
}

ActionResultType BedBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{

    MC_UNUSED(hand);
    MC_UNUSED(hit);

    // MC Java: 床的交互逻辑
    // 参考 MC 1.16.5 BedBlock.onBlockActivated()
    // 1. 检查维度 - 在下界和末地床会爆炸
    // 2. 在主世界尝试睡眠（验证距离、阻挡、时间、怪物等）

    // 获取维度信息
    DimensionType dimType = DimensionType::fromId(world.dimension());

    // 检查床是否可用（主世界可用，下界和末地会爆炸）
    if (!dimType.bedWorks()) {
        // 在下界或末地使用床会爆炸
        // 移除床方块
        world.setBlockState(pos, nullptr, 11);

        // 检查是否为床的头部，如果是脚部则同时移除头部
        BlockStateProperties::BedPart part = state.get(BlockStateProperties::BED_PART());
        if (part == BlockStateProperties::BedPart::Foot) {
            Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
            BlockPos headPos = pos.offset(facing);
            const BlockState* headState = world.getBlockState(headPos);
            if (headState && headState->hasProperty(BlockStateProperties::BED_PART()) &&
                headState->get(BlockStateProperties::BED_PART()) == BlockStateProperties::BedPart::Head) {
                world.setBlockState(headPos, nullptr, 11);
            }
        }

        // MC 1.16.5: 床爆炸强度为 5.0，破坏方块并生成火焰
        // 参考: net.minecraft.block.BedBlock.onBlockActivated
        world.createExplosion(pos.center(),
            5.0f, // 爆炸半径
            world::explosion::ExplosionMode::Destroy,
            true // 生成火焰
        );

        return ActionResultType::Success;
    }

    // 获取床头位置（如果是脚部，则计算头部位置）
    BlockPos bedHeadPos = pos;
    Direction bedFacing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    BlockStateProperties::BedPart part = state.get(BlockStateProperties::BED_PART());
    if (part == BlockStateProperties::BedPart::Foot) {
        bedHeadPos = pos.offset(bedFacing);
    }

    // 获取床头状态
    const BlockState* headState = world.getBlockState(bedHeadPos);
    if (headState == nullptr || !headState->hasProperty(BlockStateProperties::OCCUPIED())) {
        // 床头不存在或不是有效的床
        return ActionResultType::Pass;
    }

    // 检查床是否被占用
    if (headState->get(BlockStateProperties::OCCUPIED())) {
        // 床已被占用，显示消息
        // 参考 MC 1.16.5 BedBlock.onBlockActivated() 行96-100
        player.sendStatusMessage("block.minecraft.bed.occupied", true);
        return ActionResultType::Success;
    }

    // 使用 Player::tryStartSleeping() 进行睡眠验证
    // ServerPlayer 会重写此方法进行完整验证
    // Player 基类实现为简单成功（直接睡眠）
    entity::SleepResult result = player.tryStartSleeping(bedHeadPos);

    if (result == entity::SleepResult::OK) {
        // 睡眠成功，标记床为占用状态
        BlockState newHeadState = headState->with(BlockStateProperties::OCCUPIED(), true);
        world.setBlockState(bedHeadPos, &newHeadState, 2);

        // 如果交互的是脚部，也标记脚部
        if (part == BlockStateProperties::BedPart::Foot) {
            BlockState newFootState = state.with(BlockStateProperties::OCCUPIED(), true);
            world.setBlockState(pos, &newFootState, 2);
        }

        // 播放睡眠音效
        world.playSound(
            ResourceLocation("minecraft:block.bed.use"), sound::SoundCategory::Blocks, pos.center(), 1.0f, 1.0f);

        return ActionResultType::Success;
    } else {
        // 睡眠失败，显示错误消息
        const char* message = entity::getSleepResultMessage(result);
        if (message != nullptr) {
            player.sendStatusMessage(message, true);
        }
        return ActionResultType::Success;
    }
}

} // namespace blocks
} // namespace mc
