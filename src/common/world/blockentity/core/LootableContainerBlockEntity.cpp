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

#include "world/blockentity/core/LootableContainerBlockEntity.hpp"
#include "entity/entities/player/Player.hpp"
#include "item/loot/LootTable.hpp"
#include "item/loot/LootTableManager.hpp"
#include "item/loot/context/LootContext.hpp"
#include "util/math/random/Random.hpp"
#include "world/IWorld.hpp"

namespace mc {
namespace blockentity {

// ========== 构造函数 ==========

LootableContainerBlockEntity::LootableContainerBlockEntity(BlockEntityType type, const BlockPos& pos)
    : LockableBlockEntity(type, pos)
{}

// ========== 战利品表接口 ==========

void LootableContainerBlockEntity::setLootTable(const ResourceLocation& lootTable, i64 seed)
{
    m_hasLootTable = true;
    m_lootTable = lootTable;
    m_lootTableSeed = seed;
    m_lootFilled = false;
    setChanged();
}

// ========== 容器访问重写 ==========

bool LootableContainerBlockEntity::isEmpty() const
{
    // 在检查前检查战利品表填充状态
    // 注意：这里需要 const_cast 因为 fillWithLoot 不是 const 方法
    if (m_hasLootTable && !m_lootFilled) {
        // 标记为需要填充，等待下次非 const 访问时填充
        // 在实际实现中，isEmpty 会在 fillWithLoot 后立即检查
        // 这里简化处理：返回 false 表示可能有物品
        return false;
    }
    return LockableBlockEntity::isEmpty();
}

void LootableContainerBlockEntity::openContainer(Player* player)
{
    // 观察者模式玩家不能打开有战利品表的容器
    if (m_hasLootTable && player != nullptr) {
        // TODO: 检查玩家是否是观察者模式，需要在 Player 类中实现 isSpectator() 方法
        // if (player->isSpectator()) {
        //     return;
        // }
    }

    // 触发战利品表填充
    fillWithLoot(player);

    LockableBlockEntity::openContainer(player);
}

// ========== 序列化 ==========

bool LootableContainerBlockEntity::load(const nlohmann::json& data)
{
    if (!LockableBlockEntity::load(data)) {
        return false;
    }

    // 加载战利品表
    if (data.contains("LootTable") && data["LootTable"].is_string()) {
        m_lootTable = ResourceLocation(data["LootTable"].get<std::string>());
        m_hasLootTable = true;
        m_lootFilled = false;
    }
    if (data.contains("LootTableSeed") && data["LootTableSeed"].is_number_integer()) {
        m_lootTableSeed = data["LootTableSeed"].get<i64>();
    }

    return true;
}

void LootableContainerBlockEntity::save(nlohmann::json& data) const
{
    LockableBlockEntity::save(data);

    // 保存战利品表（仅在未填充时保存）
    if (m_hasLootTable && !m_lootFilled) {
        data["LootTable"] = m_lootTable.toString();
        if (m_lootTableSeed != 0) {
            data["LootTableSeed"] = m_lootTableSeed;
        }
    }
}

// ========== 战利品填充 ==========

void LootableContainerBlockEntity::fillWithLoot(Player* player)
{
    // 如果没有战利品表或已填充，不执行
    if (!m_hasLootTable || m_lootFilled) {
        return;
    }

    // 需要有世界引用来获取 LootTableManager
    if (m_world == nullptr) {
        return;
    }

    // 获取战利品表管理器
    const loot::LootTableManager* lootTableManager = m_world->lootTableManager();
    if (lootTableManager == nullptr) {
        // 在客户端或未初始化的服务端，无法填充
        return;
    }

    // 调用实际的填充方法
    // 注：const_cast 是安全的，因为 fillWithLootFromTable 不修改管理器
    fillWithLootFromTable(const_cast<loot::LootTableManager&>(*lootTableManager), player);
}

bool LootableContainerBlockEntity::fillWithLootFromTable(loot::LootTableManager& lootTableManager, Player* player)
{
    // 如果没有战利品表或已填充，不执行
    if (!m_hasLootTable || m_lootFilled) {
        return false;
    }

    // 获取战利品表
    const loot::LootTable* table = lootTableManager.getTable(m_lootTable.toString());
    if (table == nullptr) {
        // 战利品表不存在，清除标记
        m_hasLootTable = false;
        m_lootFilled = true;
        return false;
    }

    // 需要有世界引用
    if (m_world == nullptr) {
        return false;
    }

    // 清除战利品表标记（在填充之前，防止递归）
    m_hasLootTable = false;
    m_lootFilled = true;

    // 创建随机数生成器
    math::Random rng;
    if (m_lootTableSeed != 0) {
        rng = math::Random(static_cast<u64>(m_lootTableSeed));
    }

    // 创建战利品上下文
    // 使用 CHEST 参数集，包含位置参数
    loot::LootContextBuilder builder(*m_world);

    // 设置种子
    if (m_lootTableSeed != 0) {
        builder.withSeed(static_cast<u64>(m_lootTableSeed));
    } else {
        builder.withRandom(rng);
    }

    // 设置位置参数
    BlockPos lootPos = m_pos;
    builder.withParameter(loot::LootParams::BLOCK_POS, &lootPos);

    // 设置玩家相关参数（如果有）
    if (player != nullptr) {
        // TODO: 设置玩家幸运值和实体参数
        // builder.withLuck(player->getLuck());
        // builder.withParameter(loot::LootParams::THIS_ENTITY, player);
        MC_UNUSED(player);
    }

    // 设置战利品表解析器（支持嵌套战利品表）
    builder.withLootTableResolver(
        [&lootTableManager](const std::string& id) -> const loot::LootTable* { return lootTableManager.getTable(id); });
    builder.withPredicateResolver([&lootTableManager](const std::string& id) -> const loot::LootCondition* {
        return lootTableManager.getPredicate(id);
    });

    auto context = builder.build(loot::LootParameterSet());

    // 生成物品
    std::vector<ItemStack> items = table->generate(*context);

    // 填充到容器中
    IInventory* inventory = getInventory();
    if (inventory == nullptr) {
        return false;
    }

    i32 containerSize = inventory->getContainerSize();
    for (ItemStack stack : items) {
        if (stack.isEmpty()) {
            continue;
        }

        // 尝试找到可堆叠的槽位
        for (i32 slot = 0; slot < containerSize && !stack.isEmpty(); ++slot) {
            ItemStack existing = inventory->getItem(slot);
            if (!existing.isEmpty() && existing.canMergeWith(stack) &&
                existing.getCount() < existing.getMaxStackSize()) {
                // 尝试堆叠
                i32 space = existing.getMaxStackSize() - existing.getCount();
                i32 toAdd = std::min(space, stack.getCount());
                existing.grow(toAdd);
                inventory->setItem(slot, existing);
                stack.shrink(toAdd);
            }
        }

        // 剩余物品放入空槽位
        for (i32 slot = 0; slot < containerSize && !stack.isEmpty(); ++slot) {
            if (inventory->getItem(slot).isEmpty()) {
                inventory->setItem(slot, stack);
                break;
            }
        }
    }

    setChanged();
    return true;
}

} // namespace blockentity
} // namespace mc
