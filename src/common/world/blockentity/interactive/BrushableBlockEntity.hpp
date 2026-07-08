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

#pragma once

#include "common/resource/ResourceLocation.hpp"
#include "world/block/BlockPos.hpp"
#include "world/blockentity/BlockEntity.hpp"

#include <memory>

namespace mc::blockentity {

/**
 * @brief 可刷方块实体（MC BrushableBlockEntity）
 *
 * 可疑沙 / 可疑沙砾的方块实体，持有考古战利品表与种子。世界生成放置可疑方块时
 * 通过 setLootTable 设置战利品表；玩家用刷子刷方块时按战利品表种子解包生成物品。
 *
 * 字段对应 MC BrushableBlockEntity 的 lootTable / lootTableSeed（非容器，无物品栏）。
 */
class BrushableBlockEntity : public BlockEntity {
public:
    explicit BrushableBlockEntity(const BlockPos& pos);
    ~BrushableBlockEntity() override = default;

    /**
     * @brief 设置考古战利品表（MC setLootTable）
     * @param lootTable 战利品表资源位置（如 minecraft:archaeology/desert_well）
     * @param seed 战利品表种子
     */
    void setLootTable(const ResourceLocation& lootTable, i64 seed);

    [[nodiscard]] const ResourceLocation& getLootTable() const noexcept { return m_lootTable; }
    [[nodiscard]] i64 getLootTableSeed() const noexcept { return m_lootTableSeed; }
    [[nodiscard]] bool hasLootTable() const noexcept { return m_hasLootTable; }

    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

private:
    bool m_hasLootTable = false;
    ResourceLocation m_lootTable;
    i64 m_lootTableSeed = 0;
};

} // namespace mc::blockentity
