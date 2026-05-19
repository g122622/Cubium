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

#include "LootTable.hpp"
#include "LootConditions.hpp"
#include "LootFunctions.hpp"
#include "LootSerializers.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include <algorithm>

namespace mc {
namespace loot {

// ============================================================================
// LootTable
// ============================================================================

const LootTable LootTable::EMPTY;

void LootTable::addPool(std::unique_ptr<LootPool> pool)
{
    if (pool) {
        m_pools.push_back(std::move(pool));
    }
}

LootPool* LootTable::getPool(const std::string& name)
{
    for (auto& pool : m_pools) {
        if (pool->getName() == name) {
            return pool.get();
        }
    }
    return nullptr;
}

std::unique_ptr<LootPool> LootTable::removePool(const std::string& name)
{
    for (auto it = m_pools.begin(); it != m_pools.end(); ++it) {
        if ((*it)->getName() == name) {
            auto pool = std::move(*it);
            m_pools.erase(it);
            return pool;
        }
    }
    return nullptr;
}

std::vector<ItemStack> LootTable::generate(LootContext& context) const
{
    std::vector<ItemStack> items;

    // 处理物品堆叠
    auto consumer = [&items](const ItemStack& stack) {
        if (!stack.isEmpty()) {
            // 尝试合并到现有堆
            for (auto& existing : items) {
                if (existing.canMergeWith(stack)) {
                    i32 space = existing.getMaxStackSize() - existing.getCount();
                    if (space > 0) {
                        i32 toAdd = std::min(space, stack.getCount());
                        existing.grow(toAdd);
                        if (toAdd >= stack.getCount()) {
                            return; // 完全合并
                        }
                        // 部分合并，创建新堆
                        ItemStack remaining(stack.copy());
                        remaining.shrink(toAdd);
                        items.push_back(remaining);
                        return;
                    }
                }
            }
            // 无法合并，添加新堆
            items.push_back(stack);
        }
    };

    recursiveGenerate(consumer, context);
    return items;
}

void LootTable::generate(std::function<void(const ItemStack&)> consumer, LootContext& context) const
{
    recursiveGenerate(consumer, context);
}

void LootTable::recursiveGenerate(std::function<void(const ItemStack&)> consumer, LootContext& context) const
{
    // 循环检测
    if (!context.pushLootTable(this)) {
        // 检测到循环引用，跳过
        return;
    }

    // 执行所有池
    for (auto& pool : m_pools) {
        pool->generate(consumer, context);
    }

    context.popLootTable(this);
}

Result<std::unique_ptr<LootTable>> LootTable::fromJson(const std::string& json)
{
    return LootSerializers::parseLootTable(json);
}

std::string LootTable::toJson() const
{
    return LootSerializers::toJsonString(*this, 2);
}

// ============================================================================
// LootTableBuilder
// ============================================================================

std::unique_ptr<LootTable> LootTableBuilder::build() const
{
    auto table = std::make_unique<LootTable>();
    table->setId(m_id);
    table->setParameterSet(m_paramSet);

    for (const auto& pool : m_pools) {
        table->addPool(pool->clone());
    }

    return table;
}

// ============================================================================
// LootTableManager
// ============================================================================

void LootTableManager::registerTable(const std::string& id, std::unique_ptr<LootTable> table)
{
    if (table) {
        table->setId(id);
        m_tables[id] = std::move(table);
    }
}

const LootTable* LootTableManager::getTable(const std::string& id) const
{
    auto it = m_tables.find(id);
    if (it != m_tables.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool LootTableManager::hasTable(const std::string& id) const
{
    return m_tables.find(id) != m_tables.end();
}

std::vector<std::string> LootTableManager::getAllTableIds() const
{
    std::vector<std::string> ids;
    ids.reserve(m_tables.size());
    for (const auto& [id, table] : m_tables) {
        ids.push_back(id);
    }
    return ids;
}

const LootTable& LootTableManager::getEmptyTable()
{
    return LootTable::EMPTY;
}

void LootTableManager::initializeDefaultTables()
{
    // ========================================================================
    // 实体掉落表
    // ========================================================================

    // 创建猪的掉落表
    {
        auto table = std::make_unique<LootTable>();
        auto pool = std::make_unique<LootPool>(RandomValueRange(1.0f, 3.0f));
        pool->addEntry(std::make_unique<ItemLootEntry>("minecraft:porkchop", RandomValueRange(1.0f, 3.0f), 1, 0));
        table->addPool(std::move(pool));
        registerTable("minecraft:entities/pig", std::move(table));
    }

    // 创建牛的掉落表
    {
        auto table = std::make_unique<LootTable>();
        auto pool = std::make_unique<LootPool>(RandomValueRange(1.0f, 3.0f));
        pool->addEntry(std::make_unique<ItemLootEntry>("minecraft:beef", RandomValueRange(1.0f, 3.0f), 1, 0));
        table->addPool(std::move(pool));
        registerTable("minecraft:entities/cow", std::move(table));
    }

    // 创建羊的掉落表
    {
        auto table = std::make_unique<LootTable>();
        // 羊毛掉落
        auto woolPool = std::make_unique<LootPool>(RandomValueRange(1.0f, 1.0f));
        woolPool->addEntry(std::make_unique<ItemLootEntry>("minecraft:wool", RandomValueRange(1.0f, 1.0f), 1, 0));
        table->addPool(std::move(woolPool));
        // 羊肉掉落
        auto meatPool = std::make_unique<LootPool>(RandomValueRange(1.0f, 2.0f));
        meatPool->addEntry(std::make_unique<ItemLootEntry>("minecraft:mutton", RandomValueRange(1.0f, 2.0f), 1, 0));
        table->addPool(std::move(meatPool));
        registerTable("minecraft:entities/sheep", std::move(table));
    }

    // 创建鸡的掉落表
    {
        auto table = std::make_unique<LootTable>();
        auto pool = std::make_unique<LootPool>(RandomValueRange(1.0f, 2.0f));
        pool->addEntry(std::make_unique<ItemLootEntry>("minecraft:chicken", RandomValueRange(1.0f, 2.0f), 1, 0));
        pool->addEntry(std::make_unique<ItemLootEntry>("minecraft:feather", RandomValueRange(0.0f, 2.0f), 1, 0));
        table->addPool(std::move(pool));
        registerTable("minecraft:entities/chicken", std::move(table));
    }

    // ========================================================================
    // 方块掉落表
    // ========================================================================

    // 钻石矿石掉落表
    // - 精准采集: 掉落钻石矿石
    // - 无精准采集: 掉落钻石（受时运影响）
    {
        auto table = std::make_unique<LootTable>();

        // 池1: 精准采集时掉落原矿
        auto silkTouchPool = std::make_unique<LootPool>(RandomValueRange(1.0f, 1.0f));
        auto silkTouchEntry =
            std::make_unique<ItemLootEntry>("minecraft:diamond_ore", RandomValueRange(1.0f, 1.0f), 1, 0);
        silkTouchEntry->addCondition(std::make_unique<SilkTouchCondition>());
        silkTouchPool->addEntry(std::move(silkTouchEntry));
        table->addPool(std::move(silkTouchPool));

        // 池2: 无精准采集时掉落钻石（受时运影响）
        auto normalPool = std::make_unique<LootPool>(RandomValueRange(1.0f, 1.0f));
        auto normalEntry = std::make_unique<ItemLootEntry>("minecraft:diamond", RandomValueRange(1.0f, 1.0f), 1, 0);
        normalEntry->addCondition(std::make_unique<NotCondition>(std::make_unique<SilkTouchCondition>()));
        // 添加时运加成函数（OreDrops 公式）
        normalEntry->addFunction(std::make_unique<ApplyBonusFunction>(ApplyBonusFunction::BonusType::OreDrops));
        normalPool->addEntry(std::move(normalEntry));
        table->addPool(std::move(normalPool));

        registerTable("minecraft:blocks/diamond_ore", std::move(table));
    }

    // 石头掉落表
    // - 精准采集: 掉落石头
    // - 无精准采集: 掉落圆石
    {
        auto table = std::make_unique<LootTable>();

        // 精准采集时掉落石头
        auto silkTouchPool = std::make_unique<LootPool>(RandomValueRange(1.0f, 1.0f));
        auto silkTouchEntry = std::make_unique<ItemLootEntry>("minecraft:stone", RandomValueRange(1.0f, 1.0f), 1, 0);
        silkTouchEntry->addCondition(std::make_unique<SilkTouchCondition>());
        silkTouchPool->addEntry(std::move(silkTouchEntry));
        table->addPool(std::move(silkTouchPool));

        // 无精准采集时掉落圆石
        auto normalPool = std::make_unique<LootPool>(RandomValueRange(1.0f, 1.0f));
        auto normalEntry = std::make_unique<ItemLootEntry>("minecraft:cobblestone", RandomValueRange(1.0f, 1.0f), 1, 0);
        normalEntry->addCondition(std::make_unique<NotCondition>(std::make_unique<SilkTouchCondition>()));
        normalPool->addEntry(std::move(normalEntry));
        table->addPool(std::move(normalPool));

        registerTable("minecraft:blocks/stone", std::move(table));
    }

    // 煤矿掉落表
    // - 精准采集: 掉落煤矿
    // - 无精准采集: 掉落煤炭（受时运影响）
    {
        auto table = std::make_unique<LootTable>();

        // 精准采集时掉落煤矿
        auto silkTouchPool = std::make_unique<LootPool>(RandomValueRange(1.0f, 1.0f));
        auto silkTouchEntry = std::make_unique<ItemLootEntry>("minecraft:coal_ore", RandomValueRange(1.0f, 1.0f), 1, 0);
        silkTouchEntry->addCondition(std::make_unique<SilkTouchCondition>());
        silkTouchPool->addEntry(std::move(silkTouchEntry));
        table->addPool(std::move(silkTouchPool));

        // 无精准采集时掉落煤炭（受时运影响）
        auto normalPool = std::make_unique<LootPool>(RandomValueRange(1.0f, 1.0f));
        auto normalEntry = std::make_unique<ItemLootEntry>("minecraft:coal", RandomValueRange(1.0f, 1.0f), 1, 0);
        normalEntry->addCondition(std::make_unique<NotCondition>(std::make_unique<SilkTouchCondition>()));
        // 添加时运加成函数（OreDrops 公式）
        normalEntry->addFunction(std::make_unique<ApplyBonusFunction>(ApplyBonusFunction::BonusType::OreDrops));
        normalPool->addEntry(std::move(normalEntry));
        table->addPool(std::move(normalPool));

        registerTable("minecraft:blocks/coal_ore", std::move(table));
    }

    // 铁矿掉落表
    // - 精准采集: 掉落铁矿
    // - 无精准采集: 掉落粗铁 (raw_iron)
    {
        auto table = std::make_unique<LootTable>();

        // 精准采集时掉落铁矿
        auto silkTouchPool = std::make_unique<LootPool>(RandomValueRange(1.0f, 1.0f));
        auto silkTouchEntry = std::make_unique<ItemLootEntry>("minecraft:iron_ore", RandomValueRange(1.0f, 1.0f), 1, 0);
        silkTouchEntry->addCondition(std::make_unique<SilkTouchCondition>());
        silkTouchPool->addEntry(std::move(silkTouchEntry));
        table->addPool(std::move(silkTouchPool));

        // 无精准采集时掉落粗铁
        auto normalPool = std::make_unique<LootPool>(RandomValueRange(1.0f, 1.0f));
        auto normalEntry = std::make_unique<ItemLootEntry>("minecraft:raw_iron", RandomValueRange(1.0f, 1.0f), 1, 0);
        normalEntry->addCondition(std::make_unique<NotCondition>(std::make_unique<SilkTouchCondition>()));
        normalPool->addEntry(std::move(normalEntry));
        table->addPool(std::move(normalPool));

        registerTable("minecraft:blocks/iron_ore", std::move(table));
    }

    // 金矿掉落表
    // - 精准采集: 掉落金矿
    // - 无精准采集: 掉落粗金 (raw_gold)
    {
        auto table = std::make_unique<LootTable>();

        // 精准采集时掉落金矿
        auto silkTouchPool = std::make_unique<LootPool>(RandomValueRange(1.0f, 1.0f));
        auto silkTouchEntry = std::make_unique<ItemLootEntry>("minecraft:gold_ore", RandomValueRange(1.0f, 1.0f), 1, 0);
        silkTouchEntry->addCondition(std::make_unique<SilkTouchCondition>());
        silkTouchPool->addEntry(std::move(silkTouchEntry));
        table->addPool(std::move(silkTouchPool));

        // 无精准采集时掉落粗金
        auto normalPool = std::make_unique<LootPool>(RandomValueRange(1.0f, 1.0f));
        auto normalEntry = std::make_unique<ItemLootEntry>("minecraft:raw_gold", RandomValueRange(1.0f, 1.0f), 1, 0);
        normalEntry->addCondition(std::make_unique<NotCondition>(std::make_unique<SilkTouchCondition>()));
        normalPool->addEntry(std::move(normalEntry));
        table->addPool(std::move(normalPool));

        registerTable("minecraft:blocks/gold_ore", std::move(table));
    }

    // 红石矿掉落表
    // - 精准采集: 掉落红石矿
    // - 无精准采集: 掉落红石（4-5个，受时运影响）
    {
        auto table = std::make_unique<LootTable>();

        // 精准采集时掉落红石矿
        auto silkTouchPool = std::make_unique<LootPool>(RandomValueRange(1.0f, 1.0f));
        auto silkTouchEntry =
            std::make_unique<ItemLootEntry>("minecraft:redstone_ore", RandomValueRange(1.0f, 1.0f), 1, 0);
        silkTouchEntry->addCondition(std::make_unique<SilkTouchCondition>());
        silkTouchPool->addEntry(std::move(silkTouchEntry));
        table->addPool(std::move(silkTouchPool));

        // 无精准采集时掉落红石（4-5个）
        auto normalPool = std::make_unique<LootPool>(RandomValueRange(1.0f, 1.0f));
        auto normalEntry = std::make_unique<ItemLootEntry>("minecraft:redstone", RandomValueRange(4.0f, 5.0f), 1, 0);
        normalEntry->addCondition(std::make_unique<NotCondition>(std::make_unique<SilkTouchCondition>()));
        normalPool->addEntry(std::move(normalEntry));
        table->addPool(std::move(normalPool));

        registerTable("minecraft:blocks/redstone_ore", std::move(table));
    }

    // 青金石矿掉落表
    // - 精准采集: 掉落青金石矿
    // - 无精准采集: 掉落青金石（4-9个，受时运影响）
    {
        auto table = std::make_unique<LootTable>();

        // 精准采集时掉落青金石矿
        auto silkTouchPool = std::make_unique<LootPool>(RandomValueRange(1.0f, 1.0f));
        auto silkTouchEntry =
            std::make_unique<ItemLootEntry>("minecraft:lapis_ore", RandomValueRange(1.0f, 1.0f), 1, 0);
        silkTouchEntry->addCondition(std::make_unique<SilkTouchCondition>());
        silkTouchPool->addEntry(std::move(silkTouchEntry));
        table->addPool(std::move(silkTouchPool));

        // 无精准采集时掉落青金石（4-9个）
        auto normalPool = std::make_unique<LootPool>(RandomValueRange(1.0f, 1.0f));
        auto normalEntry =
            std::make_unique<ItemLootEntry>("minecraft:lapis_lazuli", RandomValueRange(4.0f, 9.0f), 1, 0);
        normalEntry->addCondition(std::make_unique<NotCondition>(std::make_unique<SilkTouchCondition>()));
        normalPool->addEntry(std::move(normalEntry));
        table->addPool(std::move(normalPool));

        registerTable("minecraft:blocks/lapis_ore", std::move(table));
    }

    // 圆石掉落表（普通挖掘）
    {
        auto table = std::make_unique<LootTable>();
        auto pool = std::make_unique<LootPool>(RandomValueRange(1.0f, 1.0f));
        pool->addEntry(std::make_unique<ItemLootEntry>("minecraft:cobblestone", RandomValueRange(1.0f, 1.0f), 1, 0));
        table->addPool(std::move(pool));
        registerTable("minecraft:blocks/cobblestone", std::move(table));
    }

    // 下界金矿掉落表
    // - 精准采集: 掉落下界金矿
    // - 无精准采集: 掉落金粒（2-6个，受时运影响）
    {
        auto table = std::make_unique<LootTable>();

        // 精准采集时掉落下界金矿
        auto silkTouchPool = std::make_unique<LootPool>(RandomValueRange(1.0f, 1.0f));
        auto silkTouchEntry =
            std::make_unique<ItemLootEntry>("minecraft:nether_gold_ore", RandomValueRange(1.0f, 1.0f), 1, 0);
        silkTouchEntry->addCondition(std::make_unique<SilkTouchCondition>());
        silkTouchPool->addEntry(std::move(silkTouchEntry));
        table->addPool(std::move(silkTouchPool));

        // 无精准采集时掉落金粒（2-6个）
        auto normalPool = std::make_unique<LootPool>(RandomValueRange(1.0f, 1.0f));
        auto normalEntry = std::make_unique<ItemLootEntry>("minecraft:gold_nugget", RandomValueRange(2.0f, 6.0f), 1, 0);
        normalEntry->addCondition(std::make_unique<NotCondition>(std::make_unique<SilkTouchCondition>()));
        normalPool->addEntry(std::move(normalEntry));
        table->addPool(std::move(normalPool));

        registerTable("minecraft:blocks/nether_gold_ore", std::move(table));
    }

    // ========================================================================
    // 钓鱼掉落表
    // ========================================================================

    // 鱼掉落表 (minecraft:gameplay/fishing/fish)
    // 参考 MC 1.16.5 FishingLootTables
    {
        auto table = std::make_unique<LootTable>();
        auto pool = std::make_unique<LootPool>(RandomValueRange(1.0f, 1.0f));

        // 鳕鱼 - 权重 60 (60%)
        pool->addEntry(std::make_unique<ItemLootEntry>("minecraft:cod", RandomValueRange(1.0f, 1.0f), 60, 0));

        // 鲑鱼 - 权重 25 (25%)
        pool->addEntry(std::make_unique<ItemLootEntry>("minecraft:salmon", RandomValueRange(1.0f, 1.0f), 25, 0));

        // 热带鱼 - 权重 2 (2%)
        pool->addEntry(std::make_unique<ItemLootEntry>("minecraft:tropical_fish", RandomValueRange(1.0f, 1.0f), 2, 0));

        // 河豚 - 权重 13 (13%)
        pool->addEntry(std::make_unique<ItemLootEntry>("minecraft:pufferfish", RandomValueRange(1.0f, 1.0f), 13, 0));

        table->addPool(std::move(pool));
        registerTable("minecraft:gameplay/fishing/fish", std::move(table));
    }

    // 垃圾掉落表 (minecraft:gameplay/fishing/junk)
    // 参考 MC 1.16.5 FishingLootTables
    // 权重总和: 10 (受 quality -2 影响)
    {
        auto table = std::make_unique<LootTable>();
        auto pool = std::make_unique<LootPool>(RandomValueRange(1.0f, 1.0f));

        // 睡莲 - 权重 17 (项目中暂未定义，暂时跳过)
        // pool->addEntry(std::make_unique<ItemLootEntry>("minecraft:lily_pad", RandomValueRange(1.0f, 1.0f), 17, -2));

        // 皮革靴子 - 权重 10 (需要设置损坏)
        pool->addEntry(
            std::make_unique<ItemLootEntry>("minecraft:leather_boots", RandomValueRange(1.0f, 1.0f), 10, -2));

        // 皮革 - 权重 10
        pool->addEntry(std::make_unique<ItemLootEntry>("minecraft:leather", RandomValueRange(1.0f, 1.0f), 10, -2));

        // 骨头 - 权重 10
        pool->addEntry(std::make_unique<ItemLootEntry>("minecraft:bone", RandomValueRange(1.0f, 1.0f), 10, -2));

        // 水瓶 - 权重 10 (项目中暂未定义，暂时跳过)
        // pool->addEntry(std::make_unique<ItemLootEntry>("minecraft:potion", RandomValueRange(1.0f, 1.0f), 10, -2));

        // 线 - 权重 5
        pool->addEntry(std::make_unique<ItemLootEntry>("minecraft:string", RandomValueRange(1.0f, 1.0f), 5, -2));

        // 钓鱼竿（损坏）- 权重 2 (需要设置损坏)
        pool->addEntry(std::make_unique<ItemLootEntry>("minecraft:fishing_rod", RandomValueRange(1.0f, 1.0f), 2, -2));

        // 碗 - 权重 10
        pool->addEntry(std::make_unique<ItemLootEntry>("minecraft:bowl", RandomValueRange(1.0f, 1.0f), 10, -2));

        // 木棍 - 权重 5
        pool->addEntry(std::make_unique<ItemLootEntry>("minecraft:stick", RandomValueRange(1.0f, 1.0f), 5, -2));

        // 墨囊 x10 - 权重 1
        pool->addEntry(std::make_unique<ItemLootEntry>("minecraft:ink_sac", RandomValueRange(10.0f, 10.0f), 1, -2));

        // 绊线钩 - 权重 10 (项目中暂未定义，暂时跳过)
        // pool->addEntry(std::make_unique<ItemLootEntry>("minecraft:tripwire_hook", RandomValueRange(1.0f, 1.0f), 10,
        // -2));

        // 腐肉 - 权重 10
        pool->addEntry(std::make_unique<ItemLootEntry>("minecraft:rotten_flesh", RandomValueRange(1.0f, 1.0f), 10, -2));

        // 竹子 - 权重 10 (仅在丛林群系，简化处理)
        // pool->addEntry(std::make_unique<ItemLootEntry>("minecraft:bamboo", RandomValueRange(1.0f, 1.0f), 10, -2));

        table->addPool(std::move(pool));
        registerTable("minecraft:gameplay/fishing/junk", std::move(table));
    }

    // 宝藏掉落表 (minecraft:gameplay/fishing/treasure)
    // 参考 MC 1.16.5 FishingLootTables
    // 权重总和: 6 (受 quality +2 影响)
    // 注意：宝藏只在开放水域出现
    {
        auto table = std::make_unique<LootTable>();
        auto pool = std::make_unique<LootPool>(RandomValueRange(1.0f, 1.0f));

        // 命名牌 - 权重 1
        auto nameTagEntry = std::make_unique<ItemLootEntry>("minecraft:name_tag", RandomValueRange(1.0f, 1.0f), 1, 2);
        nameTagEntry->addCondition(std::make_unique<FishingOpenWaterCondition>(true));
        pool->addEntry(std::move(nameTagEntry));

        // 鞍 - 权重 1
        auto saddleEntry = std::make_unique<ItemLootEntry>("minecraft:saddle", RandomValueRange(1.0f, 1.0f), 1, 2);
        saddleEntry->addCondition(std::make_unique<FishingOpenWaterCondition>(true));
        pool->addEntry(std::move(saddleEntry));

        // 弓（附魔）- 权重 1 (需要附魔函数)
        auto bowEntry = std::make_unique<ItemLootEntry>("minecraft:bow", RandomValueRange(1.0f, 1.0f), 1, 2);
        bowEntry->addCondition(std::make_unique<FishingOpenWaterCondition>(true));
        // TODO: 添加附魔函数 EnchantWithLevelsFunction
        pool->addEntry(std::move(bowEntry));

        // 钓鱼竿（附魔）- 权重 1 (需要附魔函数)
        auto fishingRodEntry =
            std::make_unique<ItemLootEntry>("minecraft:fishing_rod", RandomValueRange(1.0f, 1.0f), 1, 2);
        fishingRodEntry->addCondition(std::make_unique<FishingOpenWaterCondition>(true));
        // TODO: 添加附魔函数 EnchantWithLevelsFunction
        pool->addEntry(std::move(fishingRodEntry));

        // 书（附魔）- 权重 1 (需要附魔函数)
        auto bookEntry = std::make_unique<ItemLootEntry>("minecraft:book", RandomValueRange(1.0f, 1.0f), 1, 2);
        bookEntry->addCondition(std::make_unique<FishingOpenWaterCondition>(true));
        // TODO: 添加附魔函数 EnchantWithLevelsFunction
        pool->addEntry(std::move(bookEntry));

        // 鹦鹉螺壳 - 权重 1
        auto nautilusEntry =
            std::make_unique<ItemLootEntry>("minecraft:nautilus_shell", RandomValueRange(1.0f, 1.0f), 1, 2);
        nautilusEntry->addCondition(std::make_unique<FishingOpenWaterCondition>(true));
        pool->addEntry(std::move(nautilusEntry));

        table->addPool(std::move(pool));
        registerTable("minecraft:gameplay/fishing/treasure", std::move(table));
    }

    // 主钓鱼掉落表 (minecraft:gameplay/fishing)
    // 参考 MC 1.16.5 FishingLootTables
    // 使用 TableLootEntry 引用子表
    {
        auto table = std::make_unique<LootTable>();
        auto pool = std::make_unique<LootPool>(RandomValueRange(1.0f, 1.0f));

        // 垃圾 - 权重 10, quality -2
        auto junkEntry = std::make_unique<TableLootEntry>("minecraft:gameplay/fishing/junk", 10, -2);
        pool->addEntry(std::move(junkEntry));

        // 宝藏 - 权重 5, quality +2 (需要开放水域条件)
        auto treasureEntry = std::make_unique<TableLootEntry>("minecraft:gameplay/fishing/treasure", 5, 2);
        treasureEntry->addCondition(std::make_unique<FishingOpenWaterCondition>(true));
        pool->addEntry(std::move(treasureEntry));

        // 鱼 - 权重 85, quality -1
        auto fishEntry = std::make_unique<TableLootEntry>("minecraft:gameplay/fishing/fish", 85, -1);
        pool->addEntry(std::move(fishEntry));

        table->addPool(std::move(pool));
        registerTable("minecraft:gameplay/fishing", std::move(table));
    }
}

} // namespace loot
} // namespace mc
