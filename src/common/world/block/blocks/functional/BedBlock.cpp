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
#include "common/entity/vehicle/DismountHelper.hpp"
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
#include "common/world/block/BlockUpdateFlags.hpp"
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
                        StateValueIndices valueIndices,
                        size_t propertyCount,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
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

    // 床的形状：原版由三块 OR 而成，再整体 rotateHorizontal：
    //
    //   leg      = Block.box(0, 0, 0, 3, 3, 3);
    //   legRot90 = Shapes.rotate(leg, BLOCK_ROT_Y_90);   // -> box(13, 0, 0, 16, 3, 3)
    //   SHAPES   = rotateHorizontal(Shapes.or(Block.column(16, 3, 9), leg, legRot90));
    //
    // 旋转后两条腿落在 **Z 方向**，实测原版 `forAllBoxes` 得到 4 个盒子：
    //
    //   box(0,     0,     0.8125 -> 0.1875, 0.5625, 1.0     )   左下腿
    //   box(0.8125, 0,    0.8125 -> 1.0,    0.5625, 1.0     )   右下腿
    //   box(0,     0.1875, 0     -> 1.0,    0.5625, 0.8125  )   床垫主体
    //   box(0.1875, 0.1875, 0.8125 -> 0.8125, 0.5625, 1.0   )   床垫延伸段
    //
    // 此前画了四条腿并铺满整个 Z 范围，床底与床尾都比原版大。

    // 四个朝向**不共用**同一形状：原版对每个 facing 做 rotateHorizontal，床腿始终在
    // 与 facing 相反的一侧。逐条按 forAllBoxes 实测（black_bed）：
    //
    //   head_north -> 腿在 z 13..16      head_south -> 腿在 z 0..3
    //   head_east  -> 腿在 x 13..16      head_west  -> 腿在 x 0..3
    //   foot_X     -> 与 head_(X 的反向) 相同
    //
    // 每个朝向都是「两条腿 + 床垫主体 + 床垫延伸段」四个盒子。此前四个朝向套用同一个
    // 「腿在 +z」的形状，床朝向东西时腿会跑到错误的边。
    //
    // 索引布局：`[0..3]` = head(north/south/west/east)，`[4..7]` = foot(同序)。
    // **不能**用 `static_cast<size_t>(Direction::West) + 4`：`Direction` 枚举里
    // Down=0/Up=1 占前两位，West=4，加 4 得 8，直接越过 8 项数组写入——构造期就堆损坏。

    // 以下四个形状逐条照抄原版 forAllBoxes 的实测盒子（black_bed），单位为像素/16。
    // 每个朝向都是「2 条腿 + 床垫主体 + 床垫延伸段」，合计 4 个盒子。
    constexpr f32 A = 0.0f;    // 0
    constexpr f32 B = 0.1875f; // 3 像素
    constexpr f32 C = 0.8125f; // 13 像素
    constexpr f32 D = 1.0f;    // 16 像素
    constexpr f32 Y0 = 0.0f;
    constexpr f32 Y1 = 0.5625f; // 9 像素
    constexpr f32 Y2 = 0.1875f; // 3 像素

    // head_north：腿在 **z 0..3**（原版实测 box z 0..0.1875）。
    const CollisionShape headNorth = CollisionShape::combine(
        CollisionShape::combine(CollisionShape::box(A, Y0, A, B, Y1, B), CollisionShape::box(C, Y0, A, D, Y1, B)),
        CollisionShape::combine(CollisionShape::box(A, Y2, B, D, Y1, D), CollisionShape::box(B, Y2, A, C, Y1, B)));

    // head_south：腿在 z 13..16。
    const CollisionShape headSouth = CollisionShape::combine(
        CollisionShape::combine(CollisionShape::box(A, Y0, C, B, Y1, D), CollisionShape::box(C, Y0, C, D, Y1, D)),
        CollisionShape::combine(CollisionShape::box(A, Y2, A, D, Y1, C), CollisionShape::box(B, Y2, C, C, Y1, D)));

    // head_east：腿在 x 13..16。
    const CollisionShape headEast = CollisionShape::combine(
        CollisionShape::combine(CollisionShape::box(C, Y0, A, D, Y1, B), CollisionShape::box(C, Y0, C, D, Y1, D)),
        CollisionShape::combine(CollisionShape::box(A, Y2, A, C, Y1, D), CollisionShape::box(C, Y2, B, D, Y1, C)));

    // head_west：腿在 x 0..3。原版这块有 **5** 个盒子：床垫延伸段沿 z 被拆成两段。
    const CollisionShape headWest = CollisionShape::combine(
        CollisionShape::combine(CollisionShape::box(A, Y0, A, B, Y1, B), CollisionShape::box(A, Y0, C, B, Y1, D)),
        CollisionShape::combine(
            CollisionShape::combine(CollisionShape::box(A, Y2, B, D, Y1, C), CollisionShape::box(B, Y2, A, D, Y1, B)),
            CollisionShape::box(B, Y2, C, D, Y1, D)));

    m_shapesByFacing[0] = headNorth;
    m_shapesByFacing[1] = headSouth;
    m_shapesByFacing[2] = headWest;
    m_shapesByFacing[3] = headEast;

    // foot_X 恒等于 head_(X 的反向)：床腿落在相反一侧。
    m_shapesByFacing[4] = headSouth;
    m_shapesByFacing[5] = headNorth;
    m_shapesByFacing[6] = headEast;
    m_shapesByFacing[7] = headWest;
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
    const Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());

    // `Direction` 枚举是 Down=0/Up=1/North=2/South=3/West=4/East=5，
    // **不能**直接当 `m_shapesByFacing` 的下标——必须以水平方向序号
    // （north=0, south=1, west=2, east=3）索引，否则 West/East 会越过 8 项数组。
    size_t column = 0;
    switch (facing) {
        case Direction::North:
            column = 0;
            break;
        case Direction::South:
            column = 1;
            break;
        case Direction::West:
            column = 2;
            break;
        case Direction::East:
            column = 3;
            break;
        default:
            column = 0;
            break;
    }

    // head 用 [0..3]，foot 用 [4..7]：两者床腿方向相反，形状不同。
    const bool isFoot = state.get(BlockStateProperties::BED_PART()) == BlockStateProperties::BedPart::Foot;
    const size_t index = column + (isFoot ? 4u : 0u);
    MC_ASSERT(index < m_shapesByFacing.size());
    return m_shapesByFacing[index];
}

void BedBlock::setOccupied(IWorld& world, const BlockPos& pos, BlockState& state, bool occupied)
{
    if (state.hasProperty(BlockStateProperties::OCCUPIED())) {
        world.setBlockState(
            pos, &state.with(BlockStateProperties::OCCUPIED(), occupied), world::BlockUpdateFlags::UPDATE_CLIENTS);
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

namespace {

/// 水平候选偏移（相对床方块）
struct BedOffset {
    i32 dx;
    i32 dz;
};

/**
 * @brief 围绕床的 10 个候选偏移
 *
 * 从"优先侧"开始沿两床方块外圈环绕，包含床头正前方一格。
 */
[[nodiscard]] std::array<BedOffset, 10> _bedSurroundStandUpOffsets(Direction bedFacing, Direction sideDir)
{
    const i32 sdx = Directions::xOffset(sideDir);
    const i32 sdz = Directions::zOffset(sideDir);
    const i32 fdx = Directions::xOffset(bedFacing);
    const i32 fdz = Directions::zOffset(bedFacing);
    return {{
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
}

/// 床方块自身与床边（朝向反方向）两个"床上"候选
[[nodiscard]] std::array<BedOffset, 2> _bedAboveStandUpOffsets(Direction bedFacing)
{
    return {{
        {0, 0},
        {-Directions::xOffset(bedFacing), -Directions::zOffset(bedFacing)},
    }};
}

} // namespace

std::optional<Vector3> BedBlock::findStandUpPosition(const IWorld& world,
    const BlockPos& bedPos,
    Direction bedFacing,
    f32 entityYaw,
    f32 width,
    f32 height,
    bool isPlayer,
    bool fireImmune)
{
    // 以顺时针方向为基准；实体朝向对着该方向时改从相反侧开始搜索。
    const Direction clockwise = Directions::rotateY(bedFacing);
    const Direction sideDir =
        Directions::isFacingAngle(clockwise, entityYaw) ? Directions::opposite(clockwise) : clockwise;

    const auto surroundOffsets = _bedSurroundStandUpOffsets(bedFacing, sideDir);
    const auto aboveOffsets = _bedAboveStandUpOffsets(bedFacing);

    const auto findAtOffsets =
        [&](const auto& offsets, const BlockPos& basePos, bool safeMode) -> std::optional<Vector3> {
        for (const auto& off : offsets) {
            const BlockPos checkPos(basePos.x + off.dx, basePos.y, basePos.z + off.dz);
            const auto position = DismountHelper::findSafeDismountLocation(
                world, checkPos, width, height, isPlayer, fireImmune, safeMode);
            if (position.has_value()) {
                return position;
            }
        }
        return std::nullopt;
    };

    const auto trySurroundThenAbove = [&](const BlockPos& basePos, bool safeMode) -> std::optional<Vector3> {
        auto result = findAtOffsets(surroundOffsets, basePos, safeMode);
        if (result.has_value()) {
            return result;
        }
        return findAtOffsets(aboveOffsets, basePos, safeMode);
    };

    // 下方一格仍是床 → 双层床：先搜床上层，再搜下层，最后允许不安全位置重来一遍。
    const BlockState* belowState = world.getBlockState(bedPos.down());
    if (belowState != nullptr && belowState->hasProperty(BlockStateProperties::BED_PART())) {
        if (auto result = trySurroundThenAbove(bedPos, true); result.has_value()) {
            return result;
        }
        if (auto result = trySurroundThenAbove(bedPos.down(), true); result.has_value()) {
            return result;
        }
        if (auto result = trySurroundThenAbove(bedPos, false); result.has_value()) {
            return result;
        }
        return trySurroundThenAbove(bedPos.down(), false);
    }

    if (auto result = trySurroundThenAbove(bedPos, true); result.has_value()) {
        return result;
    }
    return trySurroundThenAbove(bedPos, false);
}

BlockActionResult BedBlock::useWithoutItem(
    const BlockState& state, IWorld& world, const BlockPos& pos, Player& player, const BlockRaycastResult& hit)
{

    MC_UNUSED(hit);

    // 获取维度信息
    DimensionType dimType = DimensionType::fromId(world.dimension());

    // 检查床是否可用（主世界可用，下界和末地会爆炸）
    if (!dimType.bedWorks()) {
        // 在下界或末地使用床会爆炸
        // 移除床方块
        world.setBlockState(pos, nullptr, world::BlockUpdateFlags::UPDATE_ALL_IMMEDIATE);

        // 检查是否为床的头部，如果是脚部则同时移除头部
        BlockStateProperties::BedPart part = state.get(BlockStateProperties::BED_PART());
        if (part == BlockStateProperties::BedPart::Foot) {
            Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
            BlockPos headPos = pos.offset(facing);
            const BlockState* headState = world.getBlockState(headPos);
            if (headState && headState->hasProperty(BlockStateProperties::BED_PART()) &&
                headState->get(BlockStateProperties::BED_PART()) == BlockStateProperties::BedPart::Head) {
                world.setBlockState(headPos, nullptr, world::BlockUpdateFlags::UPDATE_ALL_IMMEDIATE);
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
        world.setBlockState(bedHeadPos, &newHeadState, world::BlockUpdateFlags::UPDATE_CLIENTS);

        // 如果交互的是脚部，也标记脚部
        if (part == BlockStateProperties::BedPart::Foot) {
            BlockState newFootState = state.with(BlockStateProperties::OCCUPIED(), true);
            world.setBlockState(pos, &newFootState, world::BlockUpdateFlags::UPDATE_CLIENTS);
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

void BedBlock::onBlockPlacedBy(
    IWorld& world, const BlockPos& pos, const BlockState& state, const Entity* placer, const ItemStack& stack)
{
    MC_UNUSED(placer);
    MC_UNUSED(stack);

    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    BlockPos headPos = pos.offset(facing);

    // 在脚部前方放置头部方块
    BlockState headState = state.with(BlockStateProperties::BED_PART(), BlockStateProperties::BedPart::Head);
    world.setBlockState(headPos, &headState, world::BlockUpdateFlags::UPDATE_ALL);
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
                world.setBlockState(headPos,
                    airState,
                    world::BlockUpdateFlags::UPDATE_ALL | world::BlockUpdateFlags::UPDATE_SUPPRESS_DROPS);
            }
        }
    }

    Block::playerWillDestroy(world, pos, state, player);
}

} // namespace blocks
} // namespace mc
