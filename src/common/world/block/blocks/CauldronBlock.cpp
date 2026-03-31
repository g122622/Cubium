#include "CauldronBlock.hpp"
#include "../../IWorld.hpp"
#include "../VanillaBlocks.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../entity/core/LivingEntity.hpp"
#include "../../../item/core/ItemStack.hpp"
#include "../../../item/Items.hpp"
#include "../../../util/math/random/IRandom.hpp"
#include "../../../util/assert/AssertAll.hpp"

namespace mc {
namespace blocks {

using math::IRandom;

// ========== 构造函数 ==========

CauldronBlock::CauldronBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::LEVEL_0_3())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::LEVEL_0_3(), 0));

    // 炼药锅外部形状：
    // 底部: (0, 0, 0) -> (16, 3, 16)
    // 壁: 4像素厚，内部12x12空间
    // 顶部边缘: 2像素宽

    // 底部
    CollisionShape base = VoxelShapes::cube(0.0f, 0.0f, 0.0f, 1.0f, 3.0f / 16.0f, 1.0f);

    // 四面墙壁
    CollisionShape northWall = VoxelShapes::cube(0.0f, 3.0f / 16.0f, 0.0f, 1.0f, 1.0f, 2.0f / 16.0f);
    CollisionShape southWall = VoxelShapes::cube(0.0f, 3.0f / 16.0f, 14.0f / 16.0f, 1.0f, 1.0f, 1.0f);
    CollisionShape westWall = VoxelShapes::cube(0.0f, 3.0f / 16.0f, 2.0f / 16.0f, 2.0f / 16.0f, 1.0f, 14.0f / 16.0f);
    CollisionShape eastWall = VoxelShapes::cube(14.0f / 16.0f, 3.0f / 16.0f, 2.0f / 16.0f, 1.0f, 1.0f, 14.0f / 16.0f);

    // 合并所有部分
    m_outerShape = CollisionShape::combine(base, northWall, CollisionShape::CombineOp::OR);
    m_outerShape = CollisionShape::combine(m_outerShape, southWall, CollisionShape::CombineOp::OR);
    m_outerShape = CollisionShape::combine(m_outerShape, westWall, CollisionShape::CombineOp::OR);
    m_outerShape = CollisionShape::combine(m_outerShape, eastWall, CollisionShape::CombineOp::OR);

    // 内容形状（水位）
    // 水从底部3像素开始，最高到顶部边缘
    // 0: 空
    // 1: 1/3满 (高度约3像素)
    // 2: 2/3满 (高度约6像素)
    // 3: 满 (高度约9像素)
    f32 innerMinY = 3.0f / 16.0f;
    f32 innerMaxX1 = 2.0f / 16.0f;
    f32 innerMaxX2 = 14.0f / 16.0f;
    f32 innerMaxZ1 = 2.0f / 16.0f;
    f32 innerMaxZ2 = 14.0f / 16.0f;

    // 水位0：空
    m_contentShapes[0] = VoxelShapes::empty();

    // 水位1：约3像素高
    m_contentShapes[1] = VoxelShapes::cube(innerMaxX1, innerMinY, innerMaxZ1, innerMaxX2, innerMinY + 0.2f, innerMaxZ2);

    // 水位2：约6像素高
    m_contentShapes[2] = VoxelShapes::cube(innerMaxX1, innerMinY, innerMaxZ1, innerMaxX2, innerMinY + 0.4f, innerMaxZ2);

    // 水位3：约9像素高
    m_contentShapes[3] = VoxelShapes::cube(innerMaxX1, innerMinY, innerMaxZ1, innerMaxX2, innerMinY + 0.6f, innerMaxZ2);
}

// ========== 放置和更新 ==========

void CauldronBlock::neighborChanged(IWorld& world, const BlockPos& pos,
                                     Block& neighborBlock, const BlockPos& neighborPos,
                                     bool isMoving) {
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    // 炼药锅不需要响应邻居更新
    // 水位变化由交互和雨天填充控制
}

void CauldronBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, IRandom& random) {
    MC_UNUSED(state);
    MC_UNUSED(random);

    // 雨天时填充水
    // 检查是否下雨且该位置可以接收雨水
    if (world.isRaining() && world.canRainAt(pos)) {
        const BlockState* currentState = world.getBlockState(pos.x, pos.y, pos.z);
        if (currentState != nullptr) {
            i32 level = getLevel(*currentState);
            if (level < 3) {
                // 约 1/20 概率在雨天填充（每个随机tick）
                if (random.nextFloat() < 0.05f) {
                    BlockState newState = currentState->with(BlockStateProperties::LEVEL_0_3(), level + 1);
                    world.setBlock(pos.x, pos.y, pos.z, &newState);
                }
            }
        }
    }
}

// ========== 交互 ==========

ActionResult CauldronBlock::onBlockActivated(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit) {

    MC_UNUSED(hit);

    // 获取手持物品
    ItemStack& heldItem = player.getHeldItem(hand);

    if (heldItem.isEmpty()) {
        return ActionResult::Pass;
    }

    // 根据物品类型处理不同的交互
    ActionResult result = ActionResult::Pass;

    // 水桶交互
    result = handleBucketInteraction(world, pos, state, player, heldItem);
    if (result != ActionResult::Pass) {
        return result;
    }

    // 玻璃瓶交互
    result = handleBottleInteraction(world, pos, state, player, heldItem);
    if (result != ActionResult::Pass) {
        return result;
    }

    // 皮革盔甲清洗
    result = handleLeatherArmorCleaning(world, pos, state, player, heldItem);
    if (result != ActionResult::Pass) {
        return result;
    }

    // 旗帜清洗
    result = handleBannerCleaning(world, pos, state, player, heldItem);
    if (result != ActionResult::Pass) {
        return result;
    }

    return ActionResult::Pass;
}

// ========== 形状 ==========

const CollisionShape& CauldronBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    return m_outerShape;
}

const CollisionShape& CauldronBlock::getCollisionShape(const BlockState& state) const {
    MC_UNUSED(state);
    return m_outerShape;
}

const CollisionShape& CauldronBlock::getContentShape(i32 level) const {
    if (level < 0 || level > 3) {
        return VoxelShapes::empty();
    }
    return m_contentShapes[static_cast<size_t>(level)];
}

// ========== 红石 ==========

i32 CauldronBlock::getComparatorInputOverride(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos) const {

    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 比较器信号 = 水位
    return getLevel(state);
}

// ========== 静态工具方法 ==========

i32 CauldronBlock::getLevel(const BlockState& state) {
    return state.get(BlockStateProperties::LEVEL_0_3());
}

void CauldronBlock::setLevel(IWorld& world, const BlockPos& pos, const BlockState& state, i32 level) {
    if (level < 0) level = 0;
    if (level > 3) level = 3;

    i32 currentLevel = getLevel(state);
    if (currentLevel != level) {
        BlockState newState = state.with(BlockStateProperties::LEVEL_0_3(), level);
        world.setBlockState(pos.x, pos.y, pos.z, &newState, 3);
    }
}

bool CauldronBlock::isEmpty(const BlockState& state) {
    return getLevel(state) == 0;
}

bool CauldronBlock::isFull(const BlockState& state) {
    return getLevel(state) == 3;
}

// ========== 私有方法 ==========

ActionResult CauldronBlock::handleBucketInteraction(
    IWorld& world,
    const BlockPos& pos,
    const BlockState& state,
    Player& player,
    ItemStack& heldItem) {

    MC_UNUSED(player);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    MC_UNUSED(heldItem);

    // TODO: 检查物品类型
    // if (heldItem.getItem() == Items::WATER_BUCKET) {
    //     // 水桶装水：空炼药锅 -> 满炼药锅
    //     if (isEmpty(state)) {
    //         setLevel(world, pos, state, 3);
    //         playFillSound(world, pos);
    //         // TODO: 替换为空桶
    //         // heldItem = new ItemStack(Items::BUCKET);
    //         return ActionResult::Success;
    //     }
    // } else if (heldItem.getItem() == Items::BUCKET) {
    //     // 空桶取水：满炼药锅 -> 空炼药锅
    //     if (isFull(state)) {
    //         setLevel(world, pos, state, 0);
    //         playEmptySound(world, pos);
    //         // TODO: 替换为水桶
    //         // heldItem = new ItemStack(Items::WATER_BUCKET);
    //         return ActionResult::Success;
    //     }
    // }

    return ActionResult::Pass;
}

ActionResult CauldronBlock::handleBottleInteraction(
    IWorld& world,
    const BlockPos& pos,
    const BlockState& state,
    Player& player,
    ItemStack& heldItem) {

    MC_UNUSED(player);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    MC_UNUSED(heldItem);

    // TODO: 检查物品类型
    // if (heldItem.getItem() == Items::GLASS_BOTTLE) {
    //     // 空瓶取水：水位-1
    //     if (!isEmpty(state)) {
    //         setLevel(world, pos, state, getLevel(state) - 1);
    //         playEmptySound(world, pos);
    //         // TODO: 替换为水瓶
    //         // heldItem = new ItemStack(Items::POTION);
    //         return ActionResult::Success;
    //     }
    // } else if (heldItem.getItem() == Items::POTION && heldItem.getMetadata() == 0) {
    //     // 水瓶倒水：水位+1
    //     if (!isFull(state)) {
    //         setLevel(world, pos, state, getLevel(state) + 1);
    //         playFillSound(world, pos);
    //         // TODO: 替换为玻璃瓶
    //         // heldItem = new ItemStack(Items::GLASS_BOTTLE);
    //         return ActionResult::Success;
    //     }
    // }

    return ActionResult::Pass;
}

ActionResult CauldronBlock::handleLeatherArmorCleaning(
    IWorld& world,
    const BlockPos& pos,
    const BlockState& state,
    Player& player,
    ItemStack& heldItem) {

    MC_UNUSED(player);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    MC_UNUSED(heldItem);

    // TODO: 检查是否为皮革盔甲
    // if (heldItem.getItem() instanceof LeatherArmorItem) {
    //     if (heldItem.hasColor() && !isEmpty(state)) {
    //         setLevel(world, pos, state, getLevel(state) - 1);
    //         // TODO: 移除颜色
    //         // heldItem.removeColor();
    //         playEmptySound(world, pos);
    //         return ActionResult::Success;
    //     }
    // }

    return ActionResult::Pass;
}

ActionResult CauldronBlock::handleBannerCleaning(
    IWorld& world,
    const BlockPos& pos,
    const BlockState& state,
    Player& player,
    ItemStack& heldItem) {

    MC_UNUSED(player);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    MC_UNUSED(heldItem);

    // TODO: 检查是否为旗帜
    // if (heldItem.getItem() == Items::BANNER) {
    //     if (heldItem.hasBannerPattern() && !isEmpty(state)) {
    //         setLevel(world, pos, state, getLevel(state) - 1);
    //         // TODO: 移除最顶层的图案
    //         // BannerBlock.removeTopPattern(heldItem);
    //         playEmptySound(world, pos);
    //         return ActionResult::Success;
    //     }
    // }

    return ActionResult::Pass;
}

void CauldronBlock::playFillSound(IWorld& world, const BlockPos& pos) {
    // TODO: 实现音效系统
    // world.playEvent(nullptr, 1040, pos, 0);
    MC_UNUSED(world);
    MC_UNUSED(pos);
}

void CauldronBlock::playEmptySound(IWorld& world, const BlockPos& pos) {
    // TODO: 实现音效系统
    // world.playEvent(nullptr, 1041, pos, 0);
    MC_UNUSED(world);
    MC_UNUSED(pos);
}

} // namespace blocks
} // namespace mc
