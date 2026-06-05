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

#include "CauldronBlock.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/armor/DyeableArmorItem.hpp"
#include "common/item/potion/PotionUtils.hpp"
#include "common/item/potion/Potions.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc {
namespace blocks {

using math::IRandom;

// ========== 构造函数 ==========

CauldronBlock::CauldronBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::LEVEL_0_3())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
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

void CauldronBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    // 炼药锅不需要响应邻居更新
    // 水位变化由交互和雨天填充控制
}

void CauldronBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, IRandom& random)
{
    MC_UNUSED(state);
    MC_UNUSED(random);

    // 雨天时填充水
    // 检查是否下雨且该位置可以接收雨水
    if (world.isRaining() && world.canRainAt(pos)) {
        const BlockState* currentState = world.getBlockState(pos);
        if (currentState != nullptr) {
            i32 level = getLevel(*currentState);
            if (level < 3) {
                // 约 1/20 概率在雨天填充（每个随机tick）
                if (random.nextFloat() < 0.05f) {
                    BlockState newState = currentState->with(BlockStateProperties::LEVEL_0_3(), level + 1);
                    world.setBlockState(pos, &newState);
                }
            }
        }
    }
}

// ========== 交互 ==========

ActionResultType CauldronBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{

    MC_UNUSED(hit);

    // 获取手持物品
    ItemStack& heldItem = player.getHeldItem(hand);

    if (heldItem.isEmpty()) {
        return ActionResultType::Pass;
    }

    // 根据物品类型处理不同的交互
    ActionResultType result = ActionResultType::Pass;

    // 水桶交互
    result = _handleBucketInteraction(world, pos, state, player, heldItem);
    if (result != ActionResultType::Pass) {
        return result;
    }

    // 玻璃瓶交互
    result = _handleBottleInteraction(world, pos, state, player, heldItem);
    if (result != ActionResultType::Pass) {
        return result;
    }

    // 皮革盔甲清洗
    result = _handleLeatherArmorCleaning(world, pos, state, player, heldItem);
    if (result != ActionResultType::Pass) {
        return result;
    }

    // 旗帜清洗
    result = _handleBannerCleaning(world, pos, state, player, heldItem);
    if (result != ActionResultType::Pass) {
        return result;
    }

    return ActionResultType::Pass;
}

// ========== 形状 ==========

const CollisionShape& CauldronBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_outerShape;
}

const CollisionShape& CauldronBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_outerShape;
}

const CollisionShape& CauldronBlock::getContentShape(i32 level) const
{
    if (level < 0 || level > 3) {
        return VoxelShapes::empty();
    }
    return m_contentShapes[static_cast<size_t>(level)];
}

// ========== 红石 ==========

i32 CauldronBlock::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
{

    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 比较器信号 = 水位
    return getLevel(state);
}

// ========== 静态工具方法 ==========

i32 CauldronBlock::getLevel(const BlockState& state)
{
    return state.get(BlockStateProperties::LEVEL_0_3());
}

void CauldronBlock::setLevel(IWorld& world, const BlockPos& pos, const BlockState& state, i32 level)
{
    if (level < 0) level = 0;
    if (level > 3) level = 3;

    i32 currentLevel = getLevel(state);
    if (currentLevel != level) {
        BlockState newState = state.with(BlockStateProperties::LEVEL_0_3(), level);
        world.setBlockState(pos, &newState, 3);
    }
}

bool CauldronBlock::isEmpty(const BlockState& state)
{
    return getLevel(state) == 0;
}

bool CauldronBlock::isFull(const BlockState& state)
{
    return getLevel(state) == 3;
}

// ========== 私有方法 ==========

ActionResultType CauldronBlock::_handleBucketInteraction(
    IWorld& world, const BlockPos& pos, const BlockState& state, Player& player, ItemStack& heldItem)
{

    const Item* item = heldItem.getItem();
    if (item == nullptr) {
        return ActionResultType::Pass;
    }

    i32 currentLevel = getLevel(state);

    // 水桶：装水到空的或未满的炼药锅
    if (item == Items::WATER_BUCKET) {
        if (currentLevel < 3 && !world.isClientSide()) {
            // 水桶装水：空炼药锅 -> 满炼药锅
            setLevel(world, pos, state, 3);
            world.playSound(SoundEvents::ITEM_BUCKET_EMPTY,
                sound::SoundCategory::Blocks,
                Vector3(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y), static_cast<f32>(pos.z) + 0.5f),
                1.0f,
                1.0f);

            // 非创造模式：替换为空桶
            if (!player.abilities().creativeMode) {
                heldItem.shrink(1);
                if (heldItem.isEmpty()) {
                    heldItem = ItemStack(Items::BUCKET, 1);
                    player.inventory().setChanged();
                } else {
                    // 尝试添加空桶到背包
                    ItemStack emptyBucket(Items::BUCKET, 1);
                    player.inventory().add(emptyBucket);
                    if (!emptyBucket.isEmpty()) {
                        // 背包满了，在玩家位置掉落物品
                        ItemDropHelper::spawnItemAtEntity(&player, emptyBucket, 0.5f, world.getRandom());
                    }
                }
            }
        }
        return ActionResultType::Success;
    }

    // 空桶：从满的炼药锅取水
    if (item == Items::BUCKET) {
        if (currentLevel == 3 && !world.isClientSide()) {
            // 空桶取水：满炼药锅 -> 空炼药锅
            setLevel(world, pos, state, 0);
            world.playSound(SoundEvents::ITEM_BUCKET_FILL,
                sound::SoundCategory::Blocks,
                Vector3(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y), static_cast<f32>(pos.z) + 0.5f),
                1.0f,
                1.0f);

            // 非创造模式：替换为水桶
            if (!player.abilities().creativeMode) {
                heldItem.shrink(1);
                if (heldItem.isEmpty()) {
                    heldItem = ItemStack(Items::WATER_BUCKET, 1);
                    player.inventory().setChanged();
                } else {
                    // 尝试添加水桶到背包
                    ItemStack waterBucket(Items::WATER_BUCKET, 1);
                    player.inventory().add(waterBucket);
                    if (!waterBucket.isEmpty()) {
                        // 背包满了，在玩家位置掉落物品
                        ItemDropHelper::spawnItemAtEntity(&player, waterBucket, 0.5f, world.getRandom());
                    }
                }
            }
        }
        return ActionResultType::Success;
    }

    return ActionResultType::Pass;
}

ActionResultType CauldronBlock::_handleBottleInteraction(
    IWorld& world, const BlockPos& pos, const BlockState& state, Player& player, ItemStack& heldItem)
{

    const Item* item = heldItem.getItem();
    if (item == nullptr) {
        return ActionResultType::Pass;
    }

    i32 currentLevel = getLevel(state);

    // 玻璃瓶：从炼药锅取水
    if (item == Items::GLASS_BOTTLE) {
        if (currentLevel > 0 && !world.isClientSide()) {
            // 创建水瓶
            ItemStack waterBottle = potion::PotionUtils::createPotionItem(potion::Potions::WATER);

            // 降低水位
            setLevel(world, pos, state, currentLevel - 1);

            world.playSound(SoundEvents::ITEM_BOTTLE_FILL,
                sound::SoundCategory::Blocks,
                Vector3(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y), static_cast<f32>(pos.z) + 0.5f),
                1.0f,
                1.0f);

            // 非创造模式：替换为水瓶
            if (!player.abilities().creativeMode) {
                heldItem.shrink(1);
                if (heldItem.isEmpty()) {
                    heldItem = waterBottle;
                    player.inventory().setChanged();
                } else {
                    // 尝试添加水瓶到背包
                    player.inventory().add(waterBottle);
                    if (!waterBottle.isEmpty()) {
                        // 背包满了，在玩家位置掉落物品
                        ItemDropHelper::spawnItemAtEntity(&player, waterBottle, 0.5f, world.getRandom());
                    }
                }
            }
        }
        return ActionResultType::Success;
    }

    // 水瓶：向炼药锅倒水
    if (item == Items::POTION && potion::PotionUtils::isWaterBottle(heldItem)) {
        if (currentLevel < 3 && !world.isClientSide()) {
            // 增加水位
            setLevel(world, pos, state, currentLevel + 1);

            world.playSound(SoundEvents::ITEM_BOTTLE_EMPTY,
                sound::SoundCategory::Blocks,
                Vector3(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y), static_cast<f32>(pos.z) + 0.5f),
                1.0f,
                1.0f);

            // 非创造模式：替换为玻璃瓶
            if (!player.abilities().creativeMode) {
                ItemStack glassBottle(Items::GLASS_BOTTLE, 1);
                heldItem.shrink(1);
                if (heldItem.isEmpty()) {
                    heldItem = glassBottle;
                    player.inventory().setChanged();
                } else {
                    // 尝试添加玻璃瓶到背包
                    player.inventory().add(glassBottle);
                    if (!glassBottle.isEmpty()) {
                        // 背包满了，在玩家位置掉落物品
                        ItemDropHelper::spawnItemAtEntity(&player, glassBottle, 0.5f, world.getRandom());
                    }
                }
            }
        }
        return ActionResultType::Success;
    }

    return ActionResultType::Pass;
}

ActionResultType CauldronBlock::_handleLeatherArmorCleaning(
    IWorld& world, const BlockPos& pos, const BlockState& state, Player& player, ItemStack& heldItem)
{

    const Item* item = heldItem.getItem();
    if (item == nullptr) {
        return ActionResultType::Pass;
    }

    // 检查是否为皮革盔甲且有颜色
    const auto* dyeableArmor = dynamic_cast<const item::items::DyeableArmorItem*>(item);
    if (dyeableArmor != nullptr) {
        i32 currentLevel = getLevel(state);

        // 检查是否有自定义颜色且炼药锅有水
        if (currentLevel > 0 && item::items::DyeableArmorItem::hasColor(heldItem)) {
            if (!world.isClientSide()) {
                // 清除颜色
                item::items::DyeableArmorItem::clearColor(heldItem);

                // 降低水位
                setLevel(world, pos, state, currentLevel - 1);
            }
            return ActionResultType::Success;
        }
    }

    return ActionResultType::Pass;
}

ActionResultType CauldronBlock::_handleBannerCleaning(
    IWorld& world, const BlockPos& pos, const BlockState& state, Player& player, ItemStack& heldItem)
{

    MC_UNUSED(player);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    MC_UNUSED(heldItem);

    // TODO: 旗帜系统尚未实现，需要实现旗帜清洗功能
    // 检查物品是否为 BannerItem 并有图案层，如果有则移除最顶层图案并降低水位

    return ActionResultType::Pass;
}

void CauldronBlock::_playFillSound(IWorld& world, const BlockPos& pos)
{
    world.playSound(SoundEvents::ITEM_BUCKET_FILL,
        sound::SoundCategory::Blocks,
        Vector3(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y), static_cast<f32>(pos.z) + 0.5f),
        1.0f,
        1.0f);
}

void CauldronBlock::_playEmptySound(IWorld& world, const BlockPos& pos)
{
    world.playSound(SoundEvents::ITEM_BUCKET_EMPTY,
        sound::SoundCategory::Blocks,
        Vector3(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y), static_cast<f32>(pos.z) + 0.5f),
        1.0f,
        1.0f);
}

} // namespace blocks
} // namespace mc
