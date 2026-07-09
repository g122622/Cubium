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

#include "world/blockentity/storage/ShulkerBoxEntity.hpp"
#include "common/world/block/blocks/ShulkerBoxBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "entity/core/Entity.hpp"
#include "entity/entities/player/Player.hpp"
#include "item/Items.hpp"
#include "item/items/block/BlockItemRegistry.hpp"
#include "util/AxisAlignedBB.hpp"
#include "util/Direction.hpp"
#include "util/assert/AssertAll.hpp"
#include "util/math/MathConstants.hpp"
#include "util/property/Properties.hpp"
#include "world/IWorld.hpp"
#include "world/block/Block.hpp"

namespace mc {
namespace blockentity {

namespace {

/**
 * @brief 构建潜影盒打开时的实体阻挡检测区域。
 *
 * 与 Java 版行为一致：检测方块正上方 1 格体积内是否有实体。
 */
[[nodiscard]] AxisAlignedBB makeShulkerOpenSpaceBox(const BlockPos& pos)
{
    return AxisAlignedBB(static_cast<f32>(pos.x),
        static_cast<f32>(pos.y + 1),
        static_cast<f32>(pos.z),
        static_cast<f32>(pos.x + 1),
        static_cast<f32>(pos.y + 2),
        static_cast<f32>(pos.z + 1));
}

/**
 * @brief 计算实体推动区域。
 */
[[nodiscard]] AxisAlignedBB getTopBoundingBox(const BlockPos& pos, Direction facing)
{
    // 获取相反方向的边界盒
    Direction opposite = Directions::opposite(facing);

    // 基础碰撞盒
    AxisAlignedBB box(static_cast<f32>(pos.x),
        static_cast<f32>(pos.y),
        static_cast<f32>(pos.z),
        static_cast<f32>(pos.x + 1),
        static_cast<f32>(pos.y + 1),
        static_cast<f32>(pos.z + 1));

    // 向朝向方向扩展后收缩
    box = box.expand(static_cast<f32>(Directions::xOffset(facing)) * 0.5f,
        static_cast<f32>(Directions::yOffset(facing)) * 0.5f,
        static_cast<f32>(Directions::zOffset(facing)) * 0.5f);

    // 收缩朝向反方向的边界（使用 expand 的反向）
    box = box.expand(-static_cast<f32>(Directions::xOffset(opposite)),
        -static_cast<f32>(Directions::yOffset(opposite)),
        -static_cast<f32>(Directions::zOffset(opposite)));

    return box;
}

} // namespace

// ========== ShulkerBoxEntity 实现 ==========

ShulkerBoxEntity::ShulkerBoxEntity(const BlockPos& pos)
    : LootableContainerBlockEntity(BlockEntityType::ShulkerBox, pos)
    , m_inventory(SHULKER_BOX_SIZE)
{}

ShulkerBoxEntity::~ShulkerBoxEntity() noexcept = default;

f32 ShulkerBoxEntity::getProgress(f32 partialTick) const
{
    return m_prevProgress + (m_progress - m_prevProgress) * partialTick;
}

void ShulkerBoxEntity::openContainer(Player* player)
{
    // 先检查是否允许打开（锁定状态和观察者模式检查）
    // 必须在 fillWithLoot 之前检查，防止观察者模式玩家触发战利品生成
    if (player != nullptr && !LootableContainerBlockEntity::canOpen(player, ItemStack())) {
        return;
    }

    // 触发战利品表填充
    fillWithLoot(player);

    // 基类处理计数增加
    LootableContainerBlockEntity::openContainer(player);

    if (m_openCount == 1) {
        m_animationStatus = AnimationStatus::Opening;
    }

    // 同步开合动画到客户端
    if (m_world != nullptr) {
        const BlockState* state = m_world->getBlockState(m_pos);
        if (state != nullptr) {
            m_world->blockEvent(m_pos, state->getBlock(), 1, m_openCount);
        }
    }

    LootableContainerBlockEntity::setChanged();
}

void ShulkerBoxEntity::closeContainer(Player* player)
{
    // 基类已处理观察者检查
    LootableContainerBlockEntity::closeContainer(player);

    if (m_openCount == 0) {
        m_animationStatus = AnimationStatus::Closing;
    }

    // 同步开合动画到客户端
    if (m_world != nullptr) {
        const BlockState* state = m_world->getBlockState(m_pos);
        if (state != nullptr) {
            m_world->blockEvent(m_pos, state->getBlock(), 1, m_openCount);
        }
    }

    LootableContainerBlockEntity::setChanged();
}

bool ShulkerBoxEntity::triggerEvent(i32 id, i32 type)
{
    if (id == 1) {
        m_openCount = type;
        if (type == 0) {
            m_animationStatus = AnimationStatus::Closing;
        }
        if (type == 1) {
            m_animationStatus = AnimationStatus::Opening;
        }
        return true;
    }
    return false;
}

bool ShulkerBoxEntity::canOpen(IWorld& world) const
{
    return _checkCanOpen(world);
}

void ShulkerBoxEntity::tick(IWorld& world)
{
    // 缓存朝向（仅首次或朝向未初始化时）
    if (m_cachedFacing == Direction::None) {
        _cacheFacing(world);
    }

    // 先更新动画状态
    _updateAnimation(0.0f);

    // 在 Opening 或 Closing 状态时推动实体
    if (m_animationStatus == AnimationStatus::Opening || m_animationStatus == AnimationStatus::Closing) {
        _moveCollidedEntities(world, m_cachedFacing);
    }
}

void ShulkerBoxEntity::_updateAnimation(f32 partialTick)
{
    MC_UNUSED(partialTick);

    m_prevProgress = m_progress;

    switch (m_animationStatus) {
        case AnimationStatus::Opening:
            m_progress += 0.1f;
            if (m_progress >= 1.0f) {
                m_progress = 1.0f;
                m_animationStatus = AnimationStatus::Opened;
            }
            break;

        case AnimationStatus::Closing:
            m_progress -= 0.1f;
            if (m_progress <= 0.0f) {
                m_progress = 0.0f;
                m_animationStatus = AnimationStatus::Closed;
            }
            break;

        case AnimationStatus::Opened:
            m_progress = 1.0f;
            break;

        case AnimationStatus::Closed:
        default:
            m_progress = 0.0f;
            break;
    }
}

void ShulkerBoxEntity::_moveCollidedEntities(IWorld& world, Direction facing)
{
    if (facing == Direction::None) {
        return;
    }

    // 获取推动区域
    AxisAlignedBB pushBox = getTopBoundingBox(m_pos, facing);
    std::vector<Entity*> entities = world.getEntitiesInAABB(pushBox, nullptr);

    if (entities.empty()) {
        return;
    }

    // 计算推动方向和距离
    for (Entity* entity : entities) {
        if (entity == nullptr) {
            continue;
        }

        const AxisAlignedBB entityBox = entity->boundingBox();

        // 计算推动距离
        f32 dx = 0.0f;
        f32 dy = 0.0f;
        f32 dz = 0.0f;

        switch (Directions::getAxis(facing)) {
            case Axis::X: {
                if (Directions::getAxisDirection(facing) == AxisDirection::Positive) {
                    dx = pushBox.maxX - entityBox.minX;
                } else {
                    dx = entityBox.maxX - pushBox.minX;
                }
                dx += 0.01f;
                break;
            }
            case Axis::Y: {
                if (Directions::getAxisDirection(facing) == AxisDirection::Positive) {
                    dy = pushBox.maxY - entityBox.minY;
                } else {
                    dy = entityBox.maxY - pushBox.minY;
                }
                dy += 0.01f;
                break;
            }
            case Axis::Z: {
                if (Directions::getAxisDirection(facing) == AxisDirection::Positive) {
                    dz = pushBox.maxZ - entityBox.minZ;
                } else {
                    dz = entityBox.maxZ - pushBox.minZ;
                }
                dz += 0.01f;
                break;
            }
        }

        // 推动实体
        entity->move(dx * static_cast<f32>(Directions::xOffset(facing)),
            dy * static_cast<f32>(Directions::yOffset(facing)),
            dz * static_cast<f32>(Directions::zOffset(facing)));
    }
}

void ShulkerBoxEntity::_cacheFacing(IWorld& world)
{
    const BlockState* state = world.getBlockState(m_pos);
    if (state == nullptr) {
        return;
    }

    if (state->hasProperty(BlockStateProperties::FACING())) {
        m_cachedFacing = state->get(BlockStateProperties::FACING());
    }
}

bool ShulkerBoxEntity::_checkCanOpen(IWorld& world) const
{
    const AxisAlignedBB openSpace = makeShulkerOpenSpaceBox(m_pos);
    const std::vector<Entity*> collidingEntities = world.getEntitiesInAABB(openSpace, nullptr);
    return collidingEntities.empty();
}

bool ShulkerBoxEntity::load(const nlohmann::json& data)
{
    if (!LootableContainerBlockEntity::load(data)) {
        return false;
    }

    // 加载物品
    if (data.contains("items")) {
        m_inventory.load(data["items"]);
    }

    return true;
}

void ShulkerBoxEntity::save(nlohmann::json& data) const
{
    LootableContainerBlockEntity::save(data);

    // 保存物品
    nlohmann::json itemsJson;
    m_inventory.save(itemsJson);
    data["items"] = itemsJson;
}

// ========== NBT 序列化（结构模板 / 客户端同步）==========

bool ShulkerBoxEntity::loadFromNBT(const nbt::CompoundTag& tag)
{
    if (!LootableContainerBlockEntity::loadFromNBT(tag)) {
        return false;
    }

    // 仅在无未解包的战利品表时加载物品，与 MC Java 互斥语义一致
    if (!hasLootTable()) {
        loadItemsFromNBT(tag, m_inventory);
    }

    return true;
}

void ShulkerBoxEntity::saveToNBT(nbt::CompoundTag& tag) const
{
    LootableContainerBlockEntity::saveToNBT(tag);

    // 仅在无未解包的战利品表时保存物品，与 MC Java 互斥语义一致
    if (!hasLootTable()) {
        saveItemsToNBT(tag, m_inventory);
    }
}

std::unique_ptr<BlockEntity> ShulkerBoxEntity::clone() const
{
    auto clone = std::make_unique<ShulkerBoxEntity>(m_pos);
    clone->m_animationStatus = m_animationStatus;
    clone->m_progress = m_progress;
    clone->m_prevProgress = m_prevProgress;
    clone->m_openCount = m_openCount;
    for (i32 slot = 0; slot < SHULKER_BOX_SIZE; ++slot) {
        const ItemStack stack = m_inventory.getItem(slot);
        if (!stack.isEmpty()) {
            clone->m_inventory.setItem(slot, stack.copy());
        }
    }
    return clone;
}

// ========== 红石信号 ==========

i32 ShulkerBoxEntity::getComparatorSignal(IWorld& world) const
{
    MC_UNUSED(world);

    // 比较器信号计算 - 基于填充比例
    // 信号强度 = floor(填充槽位数 / 总槽位数 * 14) + (有物品 ? 1 : 0)
    i32 filledSlots = 0;
    i32 totalCount = 0;

    for (i32 i = 0; i < SHULKER_BOX_SIZE; ++i) {
        const ItemStack stack = m_inventory.getItem(i);
        if (!stack.isEmpty()) {
            filledSlots++;
            totalCount += stack.getCount();
        }
    }

    if (filledSlots == 0) {
        return 0;
    }

    const f32 fillRatio = static_cast<f32>(filledSlots) / static_cast<f32>(SHULKER_BOX_SIZE);
    return static_cast<i32>(fillRatio * 14.0f) + (totalCount > 0 ? 1 : 0);
}

// ========== ISidedInventory 接口实现 ==========

std::vector<i32> ShulkerBoxEntity::getSlotsForFace(Direction side) const
{
    MC_UNUSED(side);
    // 潜影盒可以从任意方向访问所有槽位
    // 使用静态常量避免每次调用都分配 vector
    static const std::vector<i32> allSlots = []() {
        std::vector<i32> slots;
        slots.reserve(SHULKER_BOX_SIZE);
        for (i32 i = 0; i < SHULKER_BOX_SIZE; ++i) {
            slots.push_back(i);
        }
        return slots;
    }();
    return allSlots;
}

bool ShulkerBoxEntity::canInsertItem(i32 slot, const ItemStack& stack, Direction direction) const
{
    MC_UNUSED(slot);
    MC_UNUSED(direction);

    // 潜影盒不能插入另一个潜影盒（防止递归）
    if (stack.isEmpty()) {
        return false;
    }

    // 检查物品是否为潜影盒方块（包括所有染色变体）
    const Item* item = stack.getItem();
    if (item != nullptr) {
        const Block* block = BlockItemRegistry::instance().getBlock(item->itemId());
        if (block != nullptr && blocks::ShulkerBoxBlock::isShulkerBox(*block)) {
            return false;
        }
    }

    return true;
}

bool ShulkerBoxEntity::canExtractItem(i32 slot, const ItemStack& stack, Direction direction) const
{
    MC_UNUSED(slot);
    MC_UNUSED(stack);
    MC_UNUSED(direction);
    // 潜影盒可以从任意方向提取任意物品
    return true;
}

} // namespace blockentity
} // namespace mc
