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

#include "world/blockentity/interactive/BrushableBlockEntity.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/LootTable.hpp"
#include "common/item/loot/LootTableManager.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootContextBuilder.hpp"
#include "common/item/loot/context/LootParameterSets.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/functional/TrailsBlocks.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/tick/base/TickPriority.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "util/property/Properties.hpp"
#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace blockentity {

// ============================================================================
// 构造函数
// ============================================================================

BrushableBlockEntity::BrushableBlockEntity(const BlockPos& pos)
    : BlockEntity(BlockEntityType::BrushableBlock, pos)
{}

// ============================================================================
// 核心刷扫逻辑
// ============================================================================

bool BrushableBlockEntity::brush(
    i64 gameTime, IWorld& world, LivingEntity& entity, Direction direction, ItemStack& stack)
{
    // 对齐 MC 1.21.11 BrushableBlockEntity.brush：

    // 1. 若 hitDirection 为空，记录首次命中的方向
    if (!m_hitDirection.has_value()) {
        m_hitDirection = direction;
    }

    // 2. 更新刷扫计数重置 tick（每次 brush 调用都更新，不论是否在冷却期）
    m_brushCountResetsAtTick = gameTime + BRUSH_RESET_TICKS;

    // 3. 冷却期内直接返回 false
    if (gameTime < m_coolDownEndsAtTick) {
        return false;
    }

    // 4. 设置冷却结束 tick
    m_coolDownEndsAtTick = gameTime + BRUSH_COOLDOWN_TICKS;

    // 5. 一次性生成战利品表物品
    unpackLootTable(world, entity, stack);

    // 6. 记录当前 DUSTED 完成度，递增 brushCount
    const i32 oldCompletion = getCompletionState();
    ++m_brushCount;

    // 7. 若达到完成阈值，调用 brushingCompleted 并返回 true
    if (m_brushCount >= REQUIRED_BRUSHES_TO_BREAK) {
        brushingCompleted(world, entity, stack);
        return true;
    }

    // 8. 调度方块 tick（用于 checkReset 与下落检测）
    scheduleBlockTick(world);

    // 9. 若 DUSTED 完成度变化，更新方块状态
    const i32 newCompletion = getCompletionState();
    if (oldCompletion != newCompletion) {
        const BlockState* currentState = world.getBlockState(m_pos);
        if (currentState != nullptr) {
            const BlockState* newState = &currentState->with(BlockStateProperties::DUSTED(), newCompletion);
            world.setBlockState(m_pos, newState, 3);
        }
    }

    setChanged();
    return false;
}

void BrushableBlockEntity::checkReset(IWorld& world)
{
    // 对齐 MC 1.21.11 BrushableBlockEntity.checkReset
    const i64 gameTime = static_cast<i64>(world.getGameTime());

    if (m_brushCount != 0 && gameTime >= m_brushCountResetsAtTick) {
        const i32 oldCompletion = getCompletionState();
        m_brushCount = std::max(0, m_brushCount - 2);
        const i32 newCompletion = getCompletionState();

        if (oldCompletion != newCompletion) {
            const BlockState* currentState = world.getBlockState(m_pos);
            if (currentState != nullptr) {
                const BlockState* newState = &currentState->with(BlockStateProperties::DUSTED(), newCompletion);
                world.setBlockState(m_pos, newState, 3);
            }
        }

        // 重置间隔为 4 tick（对齐 MC 的 4L）
        m_brushCountResetsAtTick = gameTime + BRUSH_RESET_RETRY_TICKS;
    }

    if (m_brushCount == 0) {
        // 完全重置：清空命中方向与计时器
        m_hitDirection.reset();
        m_brushCountResetsAtTick = 0;
        m_coolDownEndsAtTick = 0;
        setChanged();
    } else {
        // 仍有刷扫计数，继续调度 tick 以便下次检查
        scheduleBlockTick(world);
    }
}

// ============================================================================
// 战利品表接口
// ============================================================================

void BrushableBlockEntity::setLootTable(const ResourceLocation& lootTable, i64 seed)
{
    m_hasLootTable = true;
    m_lootTable = lootTable;
    m_lootTableSeed = seed;
    m_item = ItemStack();
    setChanged();
}

// ============================================================================
// 访问器
// ============================================================================

i32 BrushableBlockEntity::getCompletionState() const
{
    // 对齐 MC 1.21.11 BrushableBlockEntity.getCompletionState
    if (m_brushCount == 0) {
        return 0;
    } else if (m_brushCount < 3) {
        return 1;
    } else if (m_brushCount < 6) {
        return 2;
    } else {
        return 3;
    }
}

// ============================================================================
// BlockEntity 重写
// ============================================================================

void BrushableBlockEntity::tick(IWorld& world)
{
    // BlockEntity::tick 在本项目中由 needsTick() 控制是否调用。
    // BrushableBlockEntity 的 checkReset 由 BrushableBlock::tick（方块计划刻）触发，
    // 因此 BlockEntity::tick 此处不做任何事，避免重复调度。
    MC_UNUSED(world);
}

// ============================================================================
// 私有辅助
// ============================================================================

void BrushableBlockEntity::unpackLootTable(IWorld& world, LivingEntity& entity, ItemStack& tool)
{
    if (!m_hasLootTable) {
        return;
    }

    const loot::LootTableManager* lootTableManager = world.lootTableManager();
    if (lootTableManager == nullptr) {
        // 客户端或未初始化的服务端，无法生成战利品
        return;
    }

    const loot::LootTable* table = lootTableManager->getTable(m_lootTable.toString());
    if (table == nullptr) {
        // 战利品表不存在，清除标记并返回空物品
        m_hasLootTable = false;
        return;
    }

    // 清除标记，防止递归生成
    m_hasLootTable = false;

    // 构建战利品上下文
    // 对齐 MC 1.21.11 BrushableBlockEntity.unpackLootTable 的 LootParams：
    //   ORIGIN = Vec3.atCenterOf(worldPosition)
    //   LUCK = entity.getLuck()
    //   THIS_ENTITY = entity
    //   TOOL = tool
    // 本项目使用 BLOCK_POS 代替 ORIGIN（项目约定）。
    loot::LootContextBuilder builder(world);

    if (m_lootTableSeed != 0) {
        builder.withSeed(static_cast<u64>(m_lootTableSeed));
    }

    // 位置参数
    BlockPos lootPos = m_pos;
    builder.withParameter(loot::LootParams::BLOCK_POS, &lootPos);

    // 实体参数（LivingEntity* -> Entity*）
    Entity* entityPtr = static_cast<Entity*>(&entity);
    builder.withParameter(loot::LootParams::THIS_ENTITY, entityPtr);

    // 工具参数
    builder.withParameter(loot::LootParams::TOOL, &tool);

    // 方块实体参数（this 指针，供 CopyNbtFunction 等使用）
    BlockEntity* blockEntityPtr = this;
    builder.withNullableParameter(loot::LootParams::BLOCK_ENTITY, blockEntityPtr);

    // 幸运值
    builder.withLuck(0.0f);

    // 战利品表解析器（支持嵌套战利品表）
    builder.withLootTableResolver([&lootTableManager](const std::string& id) -> const loot::LootTable* {
        return lootTableManager->getTable(id);
    });
    builder.withPredicateResolver([&lootTableManager](const std::string& id) -> const loot::LootCondition* {
        return lootTableManager->getPredicate(id);
    });

    auto context = builder.build(loot::LootParameterSets::archaeology());

    // 生成物品
    std::vector<ItemStack> items = table->generate(*context);

    // 对齐 MC：取第一个物品（空列表则 m_item 为空）
    if (items.empty()) {
        m_item = ItemStack();
    } else {
        m_item = items.front();
    }

    setChanged();
}

void BrushableBlockEntity::brushingCompleted(IWorld& world, LivingEntity& entity, ItemStack& tool)
{
    // 对齐 MC 1.21.11 BrushableBlockEntity.brushingCompleted：

    // 1. 掉落物品
    dropContent(world, entity, tool);

    // 2. 触发 BRUSH_BLOCK_COMPLETE 世界事件（data = Block.getId(blockstate)）
    const BlockState* state = world.getBlockState(m_pos);
    const i32 blockId = (state != nullptr) ? static_cast<i32>(state->blockId()) : 0;
    world.playEvent(world::WorldEvents::BRUSH_BLOCK_COMPLETE, m_pos, blockId);

    // 3. 将方块替换为 BrushableBlock::getTurnsInto() 的默认状态
    const Block* turnsInto = nullptr;
    if (state != nullptr) {
        const auto* brushableBlock = dynamic_cast<const blocks::BrushableBlock*>(&state->getBlock());
        if (brushableBlock != nullptr) {
            turnsInto = brushableBlock->getTurnsInto();
        }
    }

    const BlockState* newState = nullptr;
    if (turnsInto != nullptr) {
        newState = BlockRegistry::instance().get(turnsInto->blockLocation());
    }
    if (newState == nullptr) {
        // 回退到空气（对齐 MC 的 Blocks.AIR）
        newState = BlockRegistry::instance().airState();
    }

    if (newState != nullptr) {
        world.setBlockState(m_pos, newState, 3);
    }
}

void BrushableBlockEntity::dropContent(IWorld& world, LivingEntity& entity, ItemStack& tool)
{
    // 对齐 MC 1.21.11 BrushableBlockEntity.dropContent：

    // 1. 确保物品已生成
    unpackLootTable(world, entity, tool);

    if (m_item.isEmpty()) {
        return;
    }

    // 2. 计算掉落位置（命中方向的相邻方块中心，按 ItemEntity 宽度缩放）
    const Direction direction = m_hitDirection.value_or(Direction::Up);
    // 对齐 MC: blockpos = worldPosition.relative(direction, 1)
    // 本项目 BlockPos 无 relative 方法，手动按方向偏移
    const BlockPos adjacentPos(m_pos.x + Directions::xOffset(direction),
        m_pos.y + Directions::yOffset(direction),
        m_pos.z + Directions::zOffset(direction));

    // MC: d0 = EntityType.ITEM.getWidth(); d1 = 1.0 - d0; d2 = d0 / 2.0;
    // ItemEntity 宽度 = 0.25（参考 ItemEntity 构造默认碰撞箱）
    constexpr f64 itemWidth = 0.25;
    constexpr f64 d1 = 1.0 - itemWidth;
    constexpr f64 d2 = itemWidth / 2.0;

    // MC: d3 = blockpos.getX() + 0.5 * d1 + d2;
    const f64 d3 = static_cast<f64>(adjacentPos.x) + 0.5 * d1 + d2;
    // MC: d4 = blockpos.getY() + 0.5 + EntityType.ITEM.getHeight() / 2.0F;
    // ItemEntity 高度 = 0.25
    constexpr f64 itemHeight = 0.25;
    const f64 d4 = static_cast<f64>(adjacentPos.y) + 0.5 + itemHeight / 2.0;
    // MC: d5 = blockpos.getZ() + 0.5 * d1 + d2;
    const f64 d5 = static_cast<f64>(adjacentPos.z) + 0.5 * d1 + d2;

    // 3. 分裂物品（split(random.nextInt(21) + 10)，即 10-30 个）
    // MC: this.item.split(p_373112_.random.nextInt(21) + 10)
    // nextInt(21) 返回 [0, 20]，+10 后为 [10, 30]
    math::Random& rng = world.getRandom();
    const i32 splitCount = rng.nextInt(21) + 10;
    ItemStack dropStack = m_item.split(splitCount);

    // 4. 生成物品实体（速度为零，对齐 MC setDeltaMovement(Vec3.ZERO)）
    ItemDropHelper::spawnItemEntity(&world, dropStack, d3, d4, d5, 0.0f, 0.0f, 0.0f);

    // 5. 清空缓存物品
    m_item = ItemStack();
    setChanged();
}

void BrushableBlockEntity::scheduleBlockTick(IWorld& world)
{
    // 客户端不调度方块 tick
    if (world.isClientSide()) {
        return;
    }

    const BlockState* state = world.getBlockState(m_pos);
    if (state == nullptr) {
        return;
    }

    world.tickManager().scheduleBlockTick(m_pos, state->getBlock(), TICK_DELAY, world::tick::TickPriority::Normal);
}

// ============================================================================
// 序列化 - JSON（区块存档）
// ============================================================================

bool BrushableBlockEntity::load(const nlohmann::json& data)
{
    if (!BlockEntity::load(data)) {
        return false;
    }

    // 战利品表
    m_hasLootTable = false;
    m_lootTable = ResourceLocation();
    m_lootTableSeed = 0;
    if (data.contains("LootTable") && data["LootTable"].is_string()) {
        m_lootTable = ResourceLocation(data["LootTable"].get<std::string>());
        m_hasLootTable = true;
        if (data.contains("LootTableSeed") && data["LootTableSeed"].is_number_integer()) {
            m_lootTableSeed = data["LootTableSeed"].get<i64>();
        }
        m_item = ItemStack();
    } else if (data.contains("item") && data["item"].is_object()) {
        // 无战利品表时加载已生成的物品
        auto result = ItemStack::fromJson(data["item"]);
        if (result.success()) {
            m_item = result.value();
        } else {
            m_item = ItemStack();
        }
    } else {
        m_item = ItemStack();
    }

    // 命中方向
    m_hitDirection.reset();
    if (data.contains("hit_direction") && data["hit_direction"].is_number_integer()) {
        const i32 dir = data["hit_direction"].get<i32>();
        if (dir >= 0 && dir <= 5) {
            m_hitDirection = static_cast<Direction>(dir);
        }
    }

    // 运行时状态（持久化以支持重载后继续 checkReset）
    m_brushCount = 0;
    if (data.contains("brush_count") && data["brush_count"].is_number_integer()) {
        m_brushCount = data["brush_count"].get<i32>();
    }
    m_brushCountResetsAtTick = 0;
    if (data.contains("brush_count_resets_at_tick") && data["brush_count_resets_at_tick"].is_number_integer()) {
        m_brushCountResetsAtTick = data["brush_count_resets_at_tick"].get<i64>();
    }
    m_coolDownEndsAtTick = 0;
    if (data.contains("cooldown_ends_at_tick") && data["cooldown_ends_at_tick"].is_number_integer()) {
        m_coolDownEndsAtTick = data["cooldown_ends_at_tick"].get<i64>();
    }

    return true;
}

void BrushableBlockEntity::save(nlohmann::json& data) const
{
    BlockEntity::save(data);

    // 战利品表（仅在未生成物品时保存）
    if (m_hasLootTable) {
        data["LootTable"] = m_lootTable.toString();
        if (m_lootTableSeed != 0) {
            data["LootTableSeed"] = m_lootTableSeed;
        }
    } else if (!m_item.isEmpty()) {
        data["item"] = m_item.toJson();
    }

    if (m_hitDirection.has_value()) {
        data["hit_direction"] = static_cast<i32>(m_hitDirection.value());
    }

    // 运行时状态
    data["brush_count"] = m_brushCount;
    data["brush_count_resets_at_tick"] = m_brushCountResetsAtTick;
    data["cooldown_ends_at_tick"] = m_coolDownEndsAtTick;
}

// ============================================================================
// 序列化 - NBT（结构模板 / 客户端同步）
// ============================================================================

bool BrushableBlockEntity::loadFromNBT(const nbt::tags::compound_tag& tag)
{
    if (!BlockEntity::loadFromNBT(tag)) {
        return false;
    }

    namespace nbt_helper = mc::entity::serialization::nbt_helper;

    // 战利品表
    m_hasLootTable = false;
    m_lootTable = ResourceLocation();
    m_lootTableSeed = 0;
    auto lootTableOpt = nbt_helper::tryGetString(tag, LOOT_TABLE_TAG);
    if (lootTableOpt.has_value()) {
        m_lootTable = ResourceLocation(lootTableOpt.value());
        m_hasLootTable = true;
        auto seedOpt = nbt_helper::tryGetLong(tag, LOOT_TABLE_SEED_TAG);
        if (seedOpt.has_value()) {
            m_lootTableSeed = seedOpt.value();
        }
        m_item = ItemStack();
    } else {
        const nbt::tags::compound_tag* itemTag = nbt_helper::tryGetCompound(tag, ITEM_TAG);
        if (itemTag != nullptr) {
            auto result = ItemStack::fromNbt(*itemTag);
            if (result.success()) {
                m_item = result.value();
            } else {
                m_item = ItemStack();
            }
        } else {
            m_item = ItemStack();
        }
    }

    // 命中方向（存储为 i8 的 legacy id）
    m_hitDirection.reset();
    auto dirOpt = nbt_helper::tryGetByte(tag, HIT_DIRECTION_TAG);
    if (dirOpt.has_value()) {
        const i32 dir = static_cast<i32>(dirOpt.value());
        if (dir >= 0 && dir <= 5) {
            m_hitDirection = static_cast<Direction>(dir);
        }
    }

    // 运行时状态
    m_brushCount = nbt_helper::tryGetInt(tag, "brush_count").value_or(0);
    m_brushCountResetsAtTick = nbt_helper::tryGetLong(tag, "brush_count_resets_at_tick").value_or(0);
    m_coolDownEndsAtTick = nbt_helper::tryGetLong(tag, "cooldown_ends_at_tick").value_or(0);

    return true;
}

void BrushableBlockEntity::saveToNBT(nbt::tags::compound_tag& tag) const
{
    BlockEntity::saveToNBT(tag);

    // 战利品表
    if (m_hasLootTable) {
        tag.put(LOOT_TABLE_TAG, m_lootTable.toString());
        if (m_lootTableSeed != 0) {
            tag.put(LOOT_TABLE_SEED_TAG, m_lootTableSeed);
        }
    } else if (!m_item.isEmpty()) {
        auto itemTag = std::make_unique<nbt::tags::compound_tag>();
        m_item.toNbt(*itemTag);
        tag.value.emplace(ITEM_TAG, std::move(itemTag));
    }

    // 命中方向
    if (m_hitDirection.has_value()) {
        tag.put(HIT_DIRECTION_TAG, static_cast<i8>(m_hitDirection.value()));
    }

    // 运行时状态
    tag.put("brush_count", m_brushCount);
    tag.put("brush_count_resets_at_tick", m_brushCountResetsAtTick);
    tag.put("cooldown_ends_at_tick", m_coolDownEndsAtTick);
}

std::unique_ptr<BlockEntity> BrushableBlockEntity::clone() const
{
    auto clone = std::make_unique<BrushableBlockEntity>(m_pos);
    clone->m_brushCount = m_brushCount;
    clone->m_brushCountResetsAtTick = m_brushCountResetsAtTick;
    clone->m_coolDownEndsAtTick = m_coolDownEndsAtTick;
    clone->m_item = m_item;
    clone->m_hitDirection = m_hitDirection;
    clone->m_lootTable = m_lootTable;
    clone->m_lootTableSeed = m_lootTableSeed;
    clone->m_hasLootTable = m_hasLootTable;
    return clone;
}

} // namespace blockentity
} // namespace mc
