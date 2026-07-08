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

#include "BrushableBlockEntity.hpp"

namespace mc::blockentity {

BrushableBlockEntity::BrushableBlockEntity(const BlockPos& pos)
    : BlockEntity(BlockEntityType::Brushable, pos)
{}

void BrushableBlockEntity::setLootTable(const ResourceLocation& lootTable, i64 seed)
{
    m_hasLootTable = true;
    m_lootTable = lootTable;
    m_lootTableSeed = seed;
    setChanged();
}

bool BrushableBlockEntity::load(const nlohmann::json& data)
{
    BlockEntity::load(data);
    if (data.contains("LootTable") && data["LootTable"].is_string()) {
        m_lootTable = ResourceLocation(data["LootTable"].get<std::string>());
        m_hasLootTable = true;
    }
    if (data.contains("LootTableSeed") && data["LootTableSeed"].is_number_integer()) {
        m_lootTableSeed = data["LootTableSeed"].get<i64>();
    }
    return true;
}

void BrushableBlockEntity::save(nlohmann::json& data) const
{
    BlockEntity::save(data);
    if (m_hasLootTable) {
        data["LootTable"] = m_lootTable.toString();
        if (m_lootTableSeed != 0) {
            data["LootTableSeed"] = m_lootTableSeed;
        }
    }
}

std::unique_ptr<BlockEntity> BrushableBlockEntity::clone() const
{
    auto entity = std::make_unique<BrushableBlockEntity>(m_pos);
    entity->m_hasLootTable = m_hasLootTable;
    entity->m_lootTable = m_lootTable;
    entity->m_lootTableSeed = m_lootTableSeed;
    return entity;
}

} // namespace mc::blockentity
