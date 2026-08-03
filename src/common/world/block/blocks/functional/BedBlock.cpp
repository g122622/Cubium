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
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/player/SleepManager.hpp"
#include "common/entity/player/SleepResult.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/color/DyeColor.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/dimension/DimensionType.hpp"
#include "common/world/explosion/ExplosionMode.hpp"
#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ========== BedBlock 实现 ==========

BedBlock::BedBlock(DyeColor color, const BlockProperties& properties)
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

    // 检查头部位置是否可替换（空气、花草等 canBeReplaced=true 的方块）
    const BlockState* headState = context.getWorld().getBlockState(headPos);
    if (headState != nullptr && headState->canBeReplaced()) {
        return defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), facing);
    }

    // 无法放置完整的床，返回默认状态（放置将失败）
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
    // 通过检查 BED_PART 属性判断是否为床方块
    return state->hasProperty(BlockStateProperties::BED_PART());
}

Direction BedBlock::getBedOrientation(IWorld& world, const BlockPos& pos)
{
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr || !state->hasProperty(BlockStateProperties::HORIZONTAL_FACING())) {
        return Direction::None;
    }
    return state->get(BlockStateProperties::HORIZONTAL_FACING());
}

Direction BedBlock::getConnectedDirection(const BlockState& state)
{
    BlockStateProperties::BedPart part = state.get(BlockStateProperties::BED_PART());
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    // 头部指向脚部（与朝向同方向），脚部指向头部（朝向的反方向）
    return (part == BlockStateProperties::BedPart::Head) ? facing : Directions::opposite(facing);
}

Vector3 BedBlock::findStandUpPosition(const IWorld& world, const BlockPos& bedPos, Direction bedFacing, f32 entityYaw)
{
    // 获取床朝向的顺时针方向
    Direction clockwise = Directions::rotateY(bedFacing);
    // 根据实体的偏航角决定优先搜索哪一侧
    Direction sideDir = Directions::isFacingAngle(clockwise, entityYaw) ? Directions::opposite(clockwise) : clockwise;

    // 检查是否为双层床（下方一格也有床）
    bool bunkBed = false;
    BlockPos belowPos = bedPos.down();
    const BlockState* belowState = world.getBlockState(belowPos);
    if (belowState != nullptr && belowState->hasProperty(BlockStateProperties::BED_PART())) {
        bunkBed = true;
    }

    // 预计算方向偏移
    const i32 sdx = Directions::xOffset(sideDir);
    const i32 sdz = Directions::zOffset(sideDir);
    const i32 fdx = Directions::xOffset(bedFacing);
    const i32 fdz = Directions::zOffset(bedFacing);

    // 候选位置偏移量（栈上数组，避免堆分配）
    struct Offset {
        i32 dx;
        i32 dz;
    };

    // 周围 10 个候选位置偏移量
    const std::array<Offset, 10> surroundOffsets = {{
        {sdx, sdz},
        {sdx - fdx, sdz - fdz},
        {sdx - fdx * 2, sdz - fdz * 2},
        {-fdx * 2, -fdz * 2},
        {-sdx - fdx * 2, -sdz - fdz * 2},
        {-sdx - fdx, -sdz - fdz},
        {-sdx, -sdz},
        {-sdx + fdx, -sdz + fdz},
        {fdx, fdz},
        {sdx + fdx, sdz + fdz},
    }};

    // 床上方 2 个候选位置偏移量
    const std::array<Offset, 2> aboveOffsets = {{
        {0, 0},
        {-fdx, -fdz},
    }};

    // 在指定偏移列表中寻找安全位置
    auto findAtOffsets =
        [&](const auto& offsets, const BlockPos& basePos, bool avoidDangerous) -> std::optional<Vector3> {
        for (const auto& off : offsets) {
            BlockPos checkPos(basePos.x + off.dx, basePos.y, basePos.z + off.dz);
            if (avoidDangerous) {
                // 检查该位置和上方位置是否安全（非固体），且下方不是危险方块
                const BlockState* belowCheck = world.getBlockState(checkPos.down());
                if (belowCheck != nullptr && belowCheck->isLiquid()) {
                    continue; // 跳过液体上方的位置
                }
            }
            if (_hasStandingSpaceForStandUp(world, checkPos)) {
                // 返回方块底部中心位置，Y 偏移 0.1
                return Vector3(static_cast<f32>(checkPos.x) + 0.5f,
                    static_cast<f32>(checkPos.y) + 0.1f,
                    static_cast<f32>(checkPos.z) + 0.5f);
            }
        }
        return std::nullopt;
    };

    if (bunkBed) {
        // 双层床：先尝试床层周围（安全），再下层周围（安全），再床上方（安全），
        // 然后回退到不安全检查
        auto result = findAtOffsets(surroundOffsets, bedPos, true);
        if (result.has_value()) return result.value();

        BlockPos lowerPos = bedPos.down();
        result = findAtOffsets(surroundOffsets, lowerPos, true);
        if (result.has_value()) return result.value();

        result = findAtOffsets(aboveOffsets, bedPos, true);
        if (result.has_value()) return result.value();

        // 回退：允许不安全位置
        result = findAtOffsets(surroundOffsets, bedPos, false);
        if (result.has_value()) return result.value();

        result = findAtOffsets(surroundOffsets, lowerPos, false);
        if (result.has_value()) return result.value();

        result = findAtOffsets(aboveOffsets, bedPos, false);
        if (result.has_value()) return result.value();
    } else {
        // 普通床：先尝试周围位置（安全），再回退到不安全检查
        auto result = findAtOffsets(surroundOffsets, bedPos, true);
        if (result.has_value()) return result.value();

        result = findAtOffsets(aboveOffsets, bedPos, true);
        if (result.has_value()) return result.value();

        // 回退：允许不安全位置
        result = findAtOffsets(surroundOffsets, bedPos, false);
        if (result.has_value()) return result.value();

        result = findAtOffsets(aboveOffsets, bedPos, false);
        if (result.has_value()) return result.value();
    }

    // 最终回退：床头正上方
    BlockPos aboveBed = bedPos.up();
    return Vector3(
        static_cast<f32>(aboveBed.x) + 0.5f, static_cast<f32>(aboveBed.y) + 0.1f, static_cast<f32>(aboveBed.z) + 0.5f);
}

bool BedBlock::_hasStandingSpaceForStandUp(const IWorld& world, const BlockPos& pos)
{
    // 检查 pos 和 pos.up() 是否都是非固体方块
    const BlockState* state1 = world.getBlockState(pos);
    const BlockState* state2 = world.getBlockState(pos.up());

    bool canStand1 = (state1 == nullptr || state1->isAir() || !state1->blocksMovement());
    bool canStand2 = (state2 == nullptr || state2->isAir() || !state2->blocksMovement());

    return canStand1 && canStand2;
}

BlockActionResult BedBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{

    MC_UNUSED(hand);
    MC_UNUSED(hit);

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

        // 床爆炸，破坏方块并生成火焰
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

void BedBlock::onBlockPlacedBy(IWorld& world, const BlockPos& pos, const BlockState& state, const ItemStack& stack)
{
    MC_UNUSED(stack);

    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    BlockPos headPos = pos.offset(facing);

    // 在脚部前方放置头部方块
    BlockState headState = state.with(BlockStateProperties::BED_PART(), BlockStateProperties::BedPart::Head);
    world.setBlockState(headPos, &headState, 3);
}

void BedBlock::playerWillDestroy(IWorld& world, const BlockPos& pos, const BlockState& state, Player& player)
{
    BlockStateProperties::BedPart part = state.get(BlockStateProperties::BED_PART());

    // 创造模式玩家破坏脚部时，同时移除头部方块（防止产生掉落物）
    // 参考 MC Java: BedBlock.playerWillDestroy — player.preventsBlockDrops() 对应创造模式
    if (part == BlockStateProperties::BedPart::Foot && player.isCreative()) {
        Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
        BlockPos headPos = pos.offset(facing);
        const BlockState* headState = world.getBlockState(headPos);
        if (headState != nullptr && headState->hasProperty(BlockStateProperties::BED_PART()) &&
            headState->get(BlockStateProperties::BED_PART()) == BlockStateProperties::BedPart::Head) {
            // 创造模式：销毁头部方块但不产生掉落物
            if (auto* airState = BlockRegistry::instance().airState()) {
                world.setBlockState(headPos, airState, 35);
            }
        }
    }

    Block::playerWillDestroy(world, pos, state, player);
}

} // namespace blocks
} // namespace mc
