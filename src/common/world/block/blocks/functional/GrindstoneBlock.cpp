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

#include "GrindstoneBlock.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/stats/Stats.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// 使用 BlockStateProperties 中的 AttachFace
using AttachFace = BlockStateProperties::AttachFace;

namespace {

// 八面体群中的单个旋转：输出轴由输入轴经「翻转后置换」得到。
// 原版 Shapes.rotate 把体素下标 (x, z) 按方阵逐位计算，对盒子而言等价于变换两个角点后取 min/max，
// 因此这里只描述「每个输出轴取自哪个输入轴、是否翻转」。
struct _Octahedral {
    i32 axis[3];  // 每个输出轴对应的输入轴下标
    bool flip[3]; // 该输出轴是否取 1 - v
};

/// 绕 Y 轴顺时针 90 度：体素 (x, z) → (1 - z, x)
constexpr _Octahedral _BLOCK_ROT_Y_90 = {{2, 1, 0}, {true, false, false}};
/// 绕 Y 轴 180 度：(x, z) → (1 - x, 1 - z)
constexpr _Octahedral _BLOCK_ROT_Y_180 = {{0, 1, 2}, {true, false, true}};
/// 绕 Y 轴顺时针 270 度：(x, z) → (z, 1 - x)
constexpr _Octahedral _BLOCK_ROT_Y_270 = {{2, 1, 0}, {false, false, true}};
/// BLOCK_ROT_X_90 = ROT_90_X_NEG：(y, z) → (z, 1 - y)
constexpr _Octahedral _BLOCK_ROT_X_90 = {{0, 2, 1}, {false, false, true}};
/// BLOCK_ROT_X_270 = ROT_90_X_POS：(y, z) → (1 - z, y)
constexpr _Octahedral _BLOCK_ROT_X_270 = {{0, 2, 1}, {false, true, false}};

/**
 * @brief 对单个轴对齐盒应用八面体旋转
 * @param box 输入盒子（方块本地坐标）
 * @param rot 旋转
 * @return 旋转后的盒子（仍是轴对齐盒，因为旋转由轴置换与翻转组成）
 */
[[nodiscard]] AxisAlignedBB _rotateBox(const AxisAlignedBB& box, const _Octahedral& rot)
{
    const f32 inMin[3] = {box.minX, box.minY, box.minZ};
    const f32 inMax[3] = {box.maxX, box.maxY, box.maxZ};
    f32 outMin[3] = {0.0f, 0.0f, 0.0f};
    f32 outMax[3] = {0.0f, 0.0f, 0.0f};
    for (i32 i = 0; i < 3; ++i) {
        const i32 src = rot.axis[i];
        outMin[i] = rot.flip[i] ? 1.0f - inMax[src] : inMin[src];
        outMax[i] = rot.flip[i] ? 1.0f - inMin[src] : inMax[src];
    }
    return AxisAlignedBB(outMin[0], outMin[1], outMin[2], outMax[0], outMax[1], outMax[2]);
}

/**
 * @brief 对形状（多盒并集）应用八面体旋转
 *
 * 逐个盒子旋转后再并集。旋转保持各盒之间的相对位置，因此不会引入新盒子或破坏并集语义。
 */
[[nodiscard]] CollisionShape _rotateShape(const CollisionShape& shape, const _Octahedral& rot)
{
    CollisionShape result = CollisionShape::empty();
    for (const auto& b : shape.boxes()) {
        const AxisAlignedBB r = _rotateBox(b, rot);
        result.addBox(r.minX, r.minY, r.minZ, r.maxX, r.maxY, r.maxZ);
    }
    return result;
}

/**
 * @brief 绕 Y 轴顺时针旋转整数个 90 度
 * @param shape 输入形状
 * @param quarterTurns 旋转的 90 度份数（0..3）
 */
[[nodiscard]] CollisionShape _rotateY(const CollisionShape& shape, i32 quarterTurns)
{
    switch (((quarterTurns % 4) + 4) % 4) {
        case 1:
            return _rotateShape(shape, _BLOCK_ROT_Y_90);
        case 2:
            return _rotateShape(shape, _BLOCK_ROT_Y_180);
        case 3:
            return _rotateShape(shape, _BLOCK_ROT_Y_270);
        default:
            return shape;
    }
}

/**
 * @brief 水平朝向对应的绕 Y 轴顺时针 90 度旋转次数
 *
 * baseShape 按 rotateHorizontal 的 NORTH 档构造，NORTH 不需要旋转；
 * 之后每档顺时针 90 度依次得到 EAST / SOUTH / WEST。
 */
[[nodiscard]] i32 _grindstoneQuarterTurns(Direction facing)
{
    switch (facing) {
        case Direction::East:
            return 1;
        case Direction::South:
            return 2;
        case Direction::West:
            return 3;
        default:
            return 0;
    }
}

} // namespace

// ========== GrindstoneBlock 实现 ==========

GrindstoneBlock::GrindstoneBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .add(BlockStateProperties::ATTACH_FACE())
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
            .with(BlockStateProperties::ATTACH_FACE(), AttachFace::Wall));
    // 原版 makeShapes：
    //   arm   = box(2,6,7, 4,10,16) ∪ box(2,5,3, 4,11,9)
    //   arm2  = rotate(arm, INVERT_X)                       // x → 1 - x
    //   base  = boxZ(8.0, 2.0, 14.0, 0.0, 12.0) ∪ arm ∪ arm2
    //   shapes = rotateAttachFace(base)
    // rotateAttachFace：WALL = rotateHorizontal(base)；
    //   FLOOR = rotateHorizontal(rotate(base, BLOCK_ROT_X_270))，BLOCK_ROT_X_270 = ROT_90_X_POS，
    //   CEILING = rotateHorizontal(rotate(base, BLOCK_ROT_Y_180 ∘ BLOCK_ROT_X_90))。
    // 注意组合顺序：`BLOCK_ROT_Y_180.compose(BLOCK_ROT_X_90)` 是「先 X_90、再 Y_180」，
    // 即对 base 先应用 BLOCK_ROT_X_90，再对结果应用 BLOCK_ROT_Y_180。
    // rotateHorizontal 的 NORTH 档不旋转，EAST/SOUTH/WEST 依次叠加 BLOCK_ROT_Y_90/180/270。
    // boxZ(width, height, y1, y2)：x 居中宽 width、y 居中高 height、z 从 y1 到 y2。
    // 基准形状（WALL + NORTH）：背板 + 左右两根臂。
    //   back = boxZ(8.0, 2.0, 14.0, 0.0, 12.0)：x 4..12、y 6..14、z 0..12 像素。
    //   arm  = box(2,6,7, 4,10,16) ∪ box(2,5,3, 4,11,9)。
    //   arm2 = rotate(arm, INVERT_X)，即 x → 1 - x。（注意 INVERT_X 只翻转 X，不翻转 Z。）
    const CollisionShape arm = CollisionShape::combine(CollisionShape::fromPixelBox(2.0f, 6.0f, 7.0f, 4.0f, 10.0f, 16.0f),
        CollisionShape::fromPixelBox(2.0f, 5.0f, 3.0f, 4.0f, 11.0f, 9.0f));
    const CollisionShape mirrorX = CollisionShape::combine(CollisionShape::fromPixelBox(12.0f, 6.0f, 7.0f, 14.0f, 10.0f, 16.0f),
        CollisionShape::fromPixelBox(12.0f, 5.0f, 3.0f, 14.0f, 11.0f, 9.0f));
    const CollisionShape baseShape = CollisionShape::combine(
        CollisionShape::combine(arm, mirrorX), CollisionShape::fromPixelBox(4.0f, 2.0f, 0.0f, 12.0f, 14.0f, 12.0f));

    // 基准形状即 rotateHorizontal 的 NORTH 档（原版 rotate(NORTH) 不旋转基准）。
    const CollisionShape floorBase = _rotateShape(baseShape, _BLOCK_ROT_X_270);
    const CollisionShape ceilingBase = _rotateShape(_rotateShape(baseShape, _BLOCK_ROT_X_90), _BLOCK_ROT_Y_180);
    for (const Direction facing : {Direction::North, Direction::South, Direction::West, Direction::East}) {
        const i32 turns = _grindstoneQuarterTurns(facing);
        m_shapes[getShapeIndex(AttachFace::Wall, facing)] = _rotateY(baseShape, turns);
        m_shapes[getShapeIndex(AttachFace::Floor, facing)] = _rotateY(floorBase, turns);
        m_shapes[getShapeIndex(AttachFace::Ceiling, facing)] = _rotateY(ceilingBase, turns);
    }
}

size_t GrindstoneBlock::getShapeIndex(AttachFace attachFace, Direction facing)
{
    size_t faceIndex = static_cast<size_t>(attachFace);
    size_t facingIndex = 0;
    switch (facing) {
        // 槽位顺序沿用原版 rotate 的枚举顺序（NORTH, WEST, EAST, SOUTH 落 0..3），
        // 因此原版 EAST 档（= rotateHorizontal 中的 BLOCK_ROT_Y_90）落在槽位 2。
        case Direction::North:
            facingIndex = 0;
            break;
        case Direction::West:
            facingIndex = 1;
            break;
        case Direction::East:
            facingIndex = 2;
            break;
        case Direction::South:
            facingIndex = 3;
            break;
        default:
            facingIndex = 0;
            break;
    }
    return faceIndex * 4 + facingIndex;
}

BlockState GrindstoneBlock::getStateForPlacement(BlockItemUseContext& context)
{
    Direction clickedFace = context.getClickedFace();
    Direction horizontalFacing = context.horizontalDirection();

    AttachFace attachFace;
    Direction finalFacing = horizontalFacing;

    if (clickedFace == Direction::Up) {
        // 点击地面 -> 地面附着
        attachFace = AttachFace::Floor;
    } else if (clickedFace == Direction::Down) {
        // 点击天花板 -> 天花板附着
        attachFace = AttachFace::Ceiling;
    } else {
        // 点击墙面 -> 墙面附着
        attachFace = AttachFace::Wall;
        finalFacing = clickedFace;
    }

    return defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), finalFacing)
        .with(BlockStateProperties::ATTACH_FACE(), attachFace);
}

const BlockState& GrindstoneBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction rotated = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), rotated);
}

const BlockState& GrindstoneBlock::mirror(const BlockState& state, Mirror mirror) const
{
    if (mirror == Mirror::None) {
        return state;
    }

    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    return rotate(state, rotation);
}

const CollisionShape& GrindstoneBlock::getShape(const BlockState& state) const
{
    AttachFace attachFace = state.get(BlockStateProperties::ATTACH_FACE());
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    size_t index = getShapeIndex(attachFace, facing);
    MC_ASSERT(index < 12);
    return m_shapes[index];
}

const CollisionShape& GrindstoneBlock::getCollisionShape(const BlockState& state) const
{
    // 碰撞形状与渲染形状相同
    return getShape(state);
}

BlockActionResult GrindstoneBlock::useWithoutItem(
    const BlockState& state, IWorld& world, const BlockPos& pos, Player& player, const BlockRaycastResult& hit)
{
    MC_UNUSED(state);
    MC_UNUSED(hit);

    if (world.isClientSide()) {
        return ActionResultType::Success;
    }

    if (world.openContainer(ContainerType::Grindstone, pos, player)) {
        player.awardCustomStat(ResourceLocation(stats::INTERACT_WITH_GRINDSTONE), 1);
        return ActionResultType::Consume;
    }

    return ActionResultType::Pass;
}

std::optional<ContainerType> GrindstoneBlock::getMenuProvider(
    const BlockState& state, IWorld& world, const BlockPos& pos) const
{
    // 有界面方块声明自身菜单类型，服务端旁观者右键据此统一打开（对齐 vanilla MenuProvider）
    (void)state;
    (void)world;
    (void)pos;
    return ContainerType::Grindstone;
}

} // namespace blocks
} // namespace mc
