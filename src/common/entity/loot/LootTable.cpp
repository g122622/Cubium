#include "LootTable.hpp"
#include "LootConditions.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include <algorithm>

namespace mc {
namespace loot {

// ============================================================================
// LootTable
// ============================================================================

const LootTable LootTable::EMPTY;

void LootTable::addPool(std::unique_ptr<LootPool> pool) {
    if (pool) {
        m_pools.push_back(std::move(pool));
    }
}

LootPool* LootTable::getPool(const String& name) {
    for (auto& pool : m_pools) {
        if (pool->getName() == name) {
            return pool.get();
        }
    }
    return nullptr;
}

std::unique_ptr<LootPool> LootTable::removePool(const String& name) {
    for (auto it = m_pools.begin(); it != m_pools.end(); ++it) {
        if ((*it)->getName() == name) {
            auto pool = std::move(*it);
            m_pools.erase(it);
            return pool;
        }
    }
    return nullptr;
}

std::vector<ItemStack> LootTable::generate(LootContext& context) const {
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
                            return;  // 完全合并
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

void LootTable::generate(std::function<void(const ItemStack&)> consumer, LootContext& context) const {
    recursiveGenerate(consumer, context);
}

void LootTable::recursiveGenerate(std::function<void(const ItemStack&)> consumer, LootContext& context) const {
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

Result<std::unique_ptr<LootTable>> LootTable::fromJson(const String& /*json*/) {
    // TODO: 实现 JSON 解析
    return Error(ErrorCode::Unsupported, "JSON parsing not yet implemented");
}

String LootTable::toJson() const {
    // TODO: 实现 JSON 序列化
    return "{}";
}

// ============================================================================
// LootTableBuilder
// ============================================================================

std::unique_ptr<LootTable> LootTableBuilder::build() const {
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

void LootTableManager::registerTable(const String& id, std::unique_ptr<LootTable> table) {
    if (table) {
        table->setId(id);
        m_tables[id] = std::move(table);
    }
}

const LootTable* LootTableManager::getTable(const String& id) const {
    auto it = m_tables.find(id);
    if (it != m_tables.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool LootTableManager::hasTable(const String& id) const {
    return m_tables.find(id) != m_tables.end();
}

std::vector<String> LootTableManager::getAllTableIds() const {
    std::vector<String> ids;
    ids.reserve(m_tables.size());
    for (const auto& [id, table] : m_tables) {
        ids.push_back(id);
    }
    return ids;
}

const LootTable& LootTableManager::getEmptyTable() {
    return LootTable::EMPTY;
}

void LootTableManager::initializeDefaultTables() {
    // ========================================================================
    // 实体掉落表
    // ========================================================================

    // 创建猪的掉落表
    {
        auto table = std::make_unique<LootTable>();
        auto pool = std::make_unique<LootPool>(RandomValueRange(1.0f, 3.0f));
        pool->addEntry(std::make_unique<ItemLootEntry>(
            "minecraft:porkchop",
            RandomValueRange(1.0f, 3.0f),
            1, 0
        ));
        table->addPool(std::move(pool));
        registerTable("minecraft:entities/pig", std::move(table));
    }

    // 创建牛的掉落表
    {
        auto table = std::make_unique<LootTable>();
        auto pool = std::make_unique<LootPool>(RandomValueRange(1.0f, 3.0f));
        pool->addEntry(std::make_unique<ItemLootEntry>(
            "minecraft:beef",
            RandomValueRange(1.0f, 3.0f),
            1, 0
        ));
        table->addPool(std::move(pool));
        registerTable("minecraft:entities/cow", std::move(table));
    }

    // 创建羊的掉落表
    {
        auto table = std::make_unique<LootTable>();
        // 羊毛掉落
        auto woolPool = std::make_unique<LootPool>(RandomValueRange(1.0f, 1.0f));
        woolPool->addEntry(std::make_unique<ItemLootEntry>(
            "minecraft:wool",
            RandomValueRange(1.0f, 1.0f),
            1, 0
        ));
        table->addPool(std::move(woolPool));
        // 羊肉掉落
        auto meatPool = std::make_unique<LootPool>(RandomValueRange(1.0f, 2.0f));
        meatPool->addEntry(std::make_unique<ItemLootEntry>(
            "minecraft:mutton",
            RandomValueRange(1.0f, 2.0f),
            1, 0
        ));
        table->addPool(std::move(meatPool));
        registerTable("minecraft:entities/sheep", std::move(table));
    }

    // 创建鸡的掉落表
    {
        auto table = std::make_unique<LootTable>();
        auto pool = std::make_unique<LootPool>(RandomValueRange(1.0f, 2.0f));
        pool->addEntry(std::make_unique<ItemLootEntry>(
            "minecraft:chicken",
            RandomValueRange(1.0f, 2.0f),
            1, 0
        ));
        pool->addEntry(std::make_unique<ItemLootEntry>(
            "minecraft:feather",
            RandomValueRange(0.0f, 2.0f),
            1, 0
        ));
        table->addPool(std::move(pool));
        registerTable("minecraft:entities/chicken", std::move(table));
    }

    // ========================================================================
    // 方块掉落表
    // ========================================================================

    // 钻石矿石掉落表
    // - 精准采集: 掉落钻石矿石
    // - 无精准采集: 掉落钻石（受时运影响）
    // TODO: 需要集成 ApplyBonusFunction 到 LootEntry 以支持时运加成
    // 时运效果目前通过 BlockDropHandler::applyFortuneBonus() 实现
    {
        auto table = std::make_unique<LootTable>();

        // 池1: 精准采集时掉落原矿
        auto silkTouchPool = std::make_unique<LootPool>(RandomValueRange(1.0f, 1.0f));
        auto silkTouchEntry = std::make_unique<ItemLootEntry>(
            "minecraft:diamond_ore",
            RandomValueRange(1.0f, 1.0f),
            1, 0
        );
        silkTouchEntry->addCondition(std::make_unique<SilkTouchCondition>());
        silkTouchPool->addEntry(std::move(silkTouchEntry));
        table->addPool(std::move(silkTouchPool));

        // 池2: 无精准采集时掉落钻石
        auto normalPool = std::make_unique<LootPool>(RandomValueRange(1.0f, 1.0f));
        auto normalEntry = std::make_unique<ItemLootEntry>(
            "minecraft:diamond",
            RandomValueRange(1.0f, 1.0f),
            1, 0
        );
        normalEntry->addCondition(std::make_unique<NotCondition>(std::make_unique<SilkTouchCondition>()));
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
        auto silkTouchEntry = std::make_unique<ItemLootEntry>(
            "minecraft:stone",
            RandomValueRange(1.0f, 1.0f),
            1, 0
        );
        silkTouchEntry->addCondition(std::make_unique<SilkTouchCondition>());
        silkTouchPool->addEntry(std::move(silkTouchEntry));
        table->addPool(std::move(silkTouchPool));

        // 无精准采集时掉落圆石
        auto normalPool = std::make_unique<LootPool>(RandomValueRange(1.0f, 1.0f));
        auto normalEntry = std::make_unique<ItemLootEntry>(
            "minecraft:cobblestone",
            RandomValueRange(1.0f, 1.0f),
            1, 0
        );
        normalEntry->addCondition(std::make_unique<NotCondition>(std::make_unique<SilkTouchCondition>()));
        normalPool->addEntry(std::move(normalEntry));
        table->addPool(std::move(normalPool));

        registerTable("minecraft:blocks/stone", std::move(table));
    }

    // 煤矿掉落表
    // - 精准采集: 掉落煤矿
    // - 无精准采集: 掉落煤炭（受时运影响）
    // TODO: 需要集成 ApplyBonusFunction 到 LootEntry 以支持时运加成
    // 时运效果目前通过 BlockDropHandler::applyFortuneBonus() 实现
    {
        auto table = std::make_unique<LootTable>();

        // 精准采集时掉落煤矿
        auto silkTouchPool = std::make_unique<LootPool>(RandomValueRange(1.0f, 1.0f));
        auto silkTouchEntry = std::make_unique<ItemLootEntry>(
            "minecraft:coal_ore",
            RandomValueRange(1.0f, 1.0f),
            1, 0
        );
        silkTouchEntry->addCondition(std::make_unique<SilkTouchCondition>());
        silkTouchPool->addEntry(std::move(silkTouchEntry));
        table->addPool(std::move(silkTouchPool));

        // 无精准采集时掉落煤炭
        auto normalPool = std::make_unique<LootPool>(RandomValueRange(1.0f, 1.0f));
        auto normalEntry = std::make_unique<ItemLootEntry>(
            "minecraft:coal",
            RandomValueRange(1.0f, 1.0f),
            1, 0
        );
        normalEntry->addCondition(std::make_unique<NotCondition>(std::make_unique<SilkTouchCondition>()));
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
        auto silkTouchEntry = std::make_unique<ItemLootEntry>(
            "minecraft:iron_ore",
            RandomValueRange(1.0f, 1.0f),
            1, 0
        );
        silkTouchEntry->addCondition(std::make_unique<SilkTouchCondition>());
        silkTouchPool->addEntry(std::move(silkTouchEntry));
        table->addPool(std::move(silkTouchPool));

        // 无精准采集时掉落粗铁
        auto normalPool = std::make_unique<LootPool>(RandomValueRange(1.0f, 1.0f));
        auto normalEntry = std::make_unique<ItemLootEntry>(
            "minecraft:raw_iron",
            RandomValueRange(1.0f, 1.0f),
            1, 0
        );
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
        auto silkTouchEntry = std::make_unique<ItemLootEntry>(
            "minecraft:gold_ore",
            RandomValueRange(1.0f, 1.0f),
            1, 0
        );
        silkTouchEntry->addCondition(std::make_unique<SilkTouchCondition>());
        silkTouchPool->addEntry(std::move(silkTouchEntry));
        table->addPool(std::move(silkTouchPool));

        // 无精准采集时掉落粗金
        auto normalPool = std::make_unique<LootPool>(RandomValueRange(1.0f, 1.0f));
        auto normalEntry = std::make_unique<ItemLootEntry>(
            "minecraft:raw_gold",
            RandomValueRange(1.0f, 1.0f),
            1, 0
        );
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
        auto silkTouchEntry = std::make_unique<ItemLootEntry>(
            "minecraft:redstone_ore",
            RandomValueRange(1.0f, 1.0f),
            1, 0
        );
        silkTouchEntry->addCondition(std::make_unique<SilkTouchCondition>());
        silkTouchPool->addEntry(std::move(silkTouchEntry));
        table->addPool(std::move(silkTouchPool));

        // 无精准采集时掉落红石（4-5个）
        auto normalPool = std::make_unique<LootPool>(RandomValueRange(1.0f, 1.0f));
        auto normalEntry = std::make_unique<ItemLootEntry>(
            "minecraft:redstone",
            RandomValueRange(4.0f, 5.0f),
            1, 0
        );
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
        auto silkTouchEntry = std::make_unique<ItemLootEntry>(
            "minecraft:lapis_ore",
            RandomValueRange(1.0f, 1.0f),
            1, 0
        );
        silkTouchEntry->addCondition(std::make_unique<SilkTouchCondition>());
        silkTouchPool->addEntry(std::move(silkTouchEntry));
        table->addPool(std::move(silkTouchPool));

        // 无精准采集时掉落青金石（4-9个）
        auto normalPool = std::make_unique<LootPool>(RandomValueRange(1.0f, 1.0f));
        auto normalEntry = std::make_unique<ItemLootEntry>(
            "minecraft:lapis_lazuli",
            RandomValueRange(4.0f, 9.0f),
            1, 0
        );
        normalEntry->addCondition(std::make_unique<NotCondition>(std::make_unique<SilkTouchCondition>()));
        normalPool->addEntry(std::move(normalEntry));
        table->addPool(std::move(normalPool));

        registerTable("minecraft:blocks/lapis_ore", std::move(table));
    }

    // 圆石掉落表（普通挖掘）
    {
        auto table = std::make_unique<LootTable>();
        auto pool = std::make_unique<LootPool>(RandomValueRange(1.0f, 1.0f));
        pool->addEntry(std::make_unique<ItemLootEntry>(
            "minecraft:cobblestone",
            RandomValueRange(1.0f, 1.0f),
            1, 0
        ));
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
        auto silkTouchEntry = std::make_unique<ItemLootEntry>(
            "minecraft:nether_gold_ore",
            RandomValueRange(1.0f, 1.0f),
            1, 0
        );
        silkTouchEntry->addCondition(std::make_unique<SilkTouchCondition>());
        silkTouchPool->addEntry(std::move(silkTouchEntry));
        table->addPool(std::move(silkTouchPool));

        // 无精准采集时掉落金粒（2-6个）
        auto normalPool = std::make_unique<LootPool>(RandomValueRange(1.0f, 1.0f));
        auto normalEntry = std::make_unique<ItemLootEntry>(
            "minecraft:gold_nugget",
            RandomValueRange(2.0f, 6.0f),
            1, 0
        );
        normalEntry->addCondition(std::make_unique<NotCondition>(std::make_unique<SilkTouchCondition>()));
        normalPool->addEntry(std::move(normalEntry));
        table->addPool(std::move(normalPool));

        registerTable("minecraft:blocks/nether_gold_ore", std::move(table));
    }
}

} // namespace loot
} // namespace mc
