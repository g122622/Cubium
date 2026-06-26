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
 * LIABILITY OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "LavaCauldronBlock.hpp"

#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gameevent/GameEvents.hpp"

namespace mc {
namespace blocks {

// ========== 构造函数 ==========

LavaCauldronBlock::LavaCauldronBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 岩浆炼药锅没有水位属性，始终为满状态
    // 不需要额外的状态属性
    auto container = StateContainer<Block, BlockState>::Builder(*this).create(
        [](const Block& block,
            std::vector<size_t> values,
            const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
            const std::vector<BlockState*>* allStates,
            u32 id) { return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id); });
    createBlockState(std::move(container));

    // 炼药锅外部形状（与普通炼药锅完全相同）：
    // 底部: (0, 0, 0) -> (16, 3, 16)
    // 四面墙壁: 2像素厚，内部12x12空间
    CollisionShape base = VoxelShapes::cube(0.0f, 0.0f, 0.0f, 1.0f, 3.0f / 16.0f, 1.0f);
    CollisionShape northWall = VoxelShapes::cube(0.0f, 3.0f / 16.0f, 0.0f, 1.0f, 1.0f, 2.0f / 16.0f);
    CollisionShape southWall = VoxelShapes::cube(0.0f, 3.0f / 16.0f, 14.0f / 16.0f, 1.0f, 1.0f, 1.0f);
    CollisionShape westWall = VoxelShapes::cube(0.0f, 3.0f / 16.0f, 2.0f / 16.0f, 2.0f / 16.0f, 1.0f, 14.0f / 16.0f);
    CollisionShape eastWall = VoxelShapes::cube(14.0f / 16.0f, 3.0f / 16.0f, 2.0f / 16.0f, 1.0f, 1.0f, 14.0f / 16.0f);

    m_outerShape = CollisionShape::combine(base, northWall, CollisionShape::CombineOp::OR);
    m_outerShape = CollisionShape::combine(m_outerShape, southWall, CollisionShape::CombineOp::OR);
    m_outerShape = CollisionShape::combine(m_outerShape, westWall, CollisionShape::CombineOp::OR);
    m_outerShape = CollisionShape::combine(m_outerShape, eastWall, CollisionShape::CombineOp::OR);

    // TODO: 当实现 getEntityInsideCollisionShape 后，添加岩浆内容碰撞形状
    // MC 原版 LavaCauldronBlock 使用 FILLED_SHAPE = Shapes.or(SHAPE, SHAPE_INSIDE)
    // 其中 SHAPE_INSIDE = Block.column(12, 4, 15)，内部岩浆碰撞区域
}

// ========== 交互 ==========

ActionResultType LavaCauldronBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(state);
    MC_UNUSED(hit);

    ItemStack& heldItem = player.getHeldItem(hand);
    const Item* item = heldItem.getItem();
    if (item == nullptr) {
        return ActionResultType::Pass;
    }

    // 空桶：从岩浆炼药锅提取岩浆，替换为空炼药锅
    if (item == Items::BUCKET) {
        if (!world.isClientSide()) {
            // 将岩浆炼药锅替换为空炼药锅
            const BlockState* cauldronState = &block_registry::BuildingBlocks::CAULDRON->defaultState();
            world.setBlockState(pos, cauldronState, 3);

            // 播放岩浆桶填充音效
            world.playSound(SoundEvents::ITEM_BUCKET_FILL_LAVA,
                sound::SoundCategory::Blocks,
                Vector3(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y), static_cast<f32>(pos.z) + 0.5f),
                1.0f,
                1.0f);

            // 触发游戏事件
            world.gameEvent(gameevent::GameEvents::FLUID_PICKUP, pos, cauldronState);

            // 非创造模式：替换为岩浆桶
            if (!player.abilities().creativeMode) {
                heldItem.shrink(1);
                if (heldItem.isEmpty()) {
                    heldItem = ItemStack(Items::LAVA_BUCKET, 1);
                    player.inventory().setChanged();
                } else {
                    ItemStack lavaBucket(Items::LAVA_BUCKET, 1);
                    player.inventory().add(lavaBucket);
                    if (!lavaBucket.isEmpty()) {
                        ItemDropHelper::spawnItemAtEntity(&player, lavaBucket, 0.5f, world.getRandom());
                    }
                }
            }
        }
        return ActionResultType::Success;
    }

    // 岩浆桶：岩浆炼药锅始终满，不能再添加岩浆
    // 水桶/水瓶/其他交互：返回 Pass 让其他系统处理
    return ActionResultType::Pass;
}

// ========== 形状 ==========

const CollisionShape& LavaCauldronBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_outerShape;
}

const CollisionShape& LavaCauldronBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    // 与普通炼药锅相同，碰撞形状仅为外部结构
    // TODO: 当实现 getEntityInsideCollisionShape 后，岩浆内容区域应通过该方法返回
    return m_outerShape;
}

// ========== 实体碰撞 ==========

void LavaCauldronBlock::onEntityCollision(
    const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 对进入岩浆炼药锅的实体造成点燃和岩浆伤害
    // 参考 MC Java: LavaCauldronBlock.entityInside() -> LAVA_IGNITE + lavaHurt
    entity.lavaIgnite();
    entity.lavaHurt();
}

// ========== 红石 ==========

i32 LavaCauldronBlock::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // 岩浆炼药锅始终满，比较器输出 3
    return 3;
}

} // namespace blocks
} // namespace mc
