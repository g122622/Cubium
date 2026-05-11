#include "server/stats/StatRegistry.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/item/Items.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include <mutex>

namespace mc {
namespace server {
namespace stats {

StatRegistry& StatRegistry::instance() {
    static StatRegistry registry;
    return registry;
}

void StatRegistry::clear() {
    m_stats.clear();
}

void StatRegistry::registerBuiltinStats() {
    // 注册自定义统计
    // 注意：方块和物品的统计在相应的方块/物品注册时自动注册
    // 这里只注册自定义统计
    registerAllCustomStats();
}

void StatRegistry::registerAllBlocks() {
    // 注册所有方块的挖掘统计
    // minecraft.mined:{block_id}
    // 注意：需要在 BlockRegistry 有 forEach 方法后实现
    // 当前版本跳过，由游戏在方块注册时调用 registerMinedStat
}

void StatRegistry::registerAllItems() {
    // 注册所有物品的统计
    // 注意：需要在 ItemRegistry 有 forEach 方法后实现
    // 当前版本跳过，由游戏在物品注册时调用相应方法
}

void StatRegistry::registerAllEntities() {
    // 注册所有实体的击杀/被击杀统计
    // minecraft.killed:{entity_id}
    // minecraft.killed_by:{entity_id}
    // 注意：需要在 EntityRegistry 有 forEach 方法后实现
    // 当前版本跳过，由游戏在实体注册时调用相应方法
}

void StatRegistry::registerAllCustomStats() {
    // 注册所有自定义统计
    // 参考 MC 1.16.5: net.minecraft.stats.Stats

    // ========== 时间相关 ==========
    registerCustomStat(ResourceLocation("minecraft:play_one_minute"));
    registerCustomStat(ResourceLocation("minecraft:play_time"));  // 1.17+
    registerCustomStat(ResourceLocation("minecraft:total_world_time"));

    // ========== 距离相关（单位：厘米）==========
    registerCustomStat(ResourceLocation("minecraft:walk_one_cm"));
    registerCustomStat(ResourceLocation("minecraft:sprint_one_cm"));
    registerCustomStat(ResourceLocation("minecraft:swim_one_cm"));
    registerCustomStat(ResourceLocation("minecraft:fall_one_cm"));
    registerCustomStat(ResourceLocation("minecraft:climb_one_cm"));
    registerCustomStat(ResourceLocation("minecraft:fly_one_cm"));
    registerCustomStat(ResourceLocation("minecraft:dive_one_cm"));
    registerCustomStat(ResourceLocation("minecraft:minecart_one_cm"));
    registerCustomStat(ResourceLocation("minecraft:boat_one_cm"));
    registerCustomStat(ResourceLocation("minecraft:pig_one_cm"));
    registerCustomStat(ResourceLocation("minecraft:horse_one_cm"));
    registerCustomStat(ResourceLocation("minecraft:aviate_one_cm"));  // 鞘翅飞行
    registerCustomStat(ResourceLocation("minecraft:llama_one_cm"));
    registerCustomStat(ResourceLocation("minecraft:stride_one_cm"));  // 炽足兽
    registerCustomStat(ResourceLocation("minecraft:crouch_one_cm"));
    registerCustomStat(ResourceLocation("minecraft:sneak_time"));  // 潜行时间（tick）

    // ========== 动作计数 ==========
    registerCustomStat(ResourceLocation("minecraft:jump"));
    registerCustomStat(ResourceLocation("minecraft:drop"));
    registerCustomStat(ResourceLocation("minecraft:damage_dealt"));
    registerCustomStat(ResourceLocation("minecraft:damage_taken"));
    registerCustomStat(ResourceLocation("minecraft:damage_blocked_by_shield"));
    registerCustomStat(ResourceLocation("minecraft:damage_absorbed"));
    registerCustomStat(ResourceLocation("minecraft:damage_resisted"));
    registerCustomStat(ResourceLocation("minecraft:deaths"));
    registerCustomStat(ResourceLocation("minecraft:mob_kills"));
    registerCustomStat(ResourceLocation("minecraft:animals_bred"));
    registerCustomStat(ResourceLocation("minecraft:player_kills"));
    registerCustomStat(ResourceLocation("minecraft:fish_caught"));
    registerCustomStat(ResourceLocation("minecraft:talked_to_villager"));
    registerCustomStat(ResourceLocation("minecraft:traded_with_villager"));
    registerCustomStat(ResourceLocation("minecraft:eat_cake_slice"));
    registerCustomStat(ResourceLocation("minecraft:fill_cauldron"));
    registerCustomStat(ResourceLocation("minecraft:use_cauldron"));
    registerCustomStat(ResourceLocation("minecraft:clean_armor"));
    registerCustomStat(ResourceLocation("minecraft:clean_banner"));
    registerCustomStat(ResourceLocation("minecraft:clean_shulker_box"));
    registerCustomStat(ResourceLocation("minecraft:interact_with_crafting_table"));
    registerCustomStat(ResourceLocation("minecraft:interact_with_furnace"));
    registerCustomStat(ResourceLocation("minecraft:interact_with_blast_furnace"));
    registerCustomStat(ResourceLocation("minecraft:interact_with_smoker"));
    registerCustomStat(ResourceLocation("minecraft:interact_with_brewingstand"));
    registerCustomStat(ResourceLocation("minecraft:interact_with_beacon"));
    registerCustomStat(ResourceLocation("minecraft:interact_with_anvil"));
    registerCustomStat(ResourceLocation("minecraft:interact_with_grindstone"));
    registerCustomStat(ResourceLocation("minecraft:interact_with_cartography_table"));
    registerCustomStat(ResourceLocation("minecraft:interact_with_loom"));
    registerCustomStat(ResourceLocation("minecraft:interact_with_stonecutter"));
    registerCustomStat(ResourceLocation("minecraft:interact_with_smithing_table"));
    registerCustomStat(ResourceLocation("minecraft:bell_ring"));
    registerCustomStat(ResourceLocation("minecraft:raid_trigger"));
    registerCustomStat(ResourceLocation("minecraft:raid_win"));
    registerCustomStat(ResourceLocation("minecraft:sleep_in_bed"));
    registerCustomStat(ResourceLocation("minecraft:open_chest"));
    registerCustomStat(ResourceLocation("minecraft:open_enderchest"));
    registerCustomStat(ResourceLocation("minecraft:open_shulker_box"));
    registerCustomStat(ResourceLocation("minecraft:open_barrel"));
    registerCustomStat(ResourceLocation("minecraft:inspect_dispenser"));
    registerCustomStat(ResourceLocation("minecraft:inspect_dropper"));
    registerCustomStat(ResourceLocation("minecraft:inspect_hopper"));
    registerCustomStat(ResourceLocation("minecraft:ender_pearls_used"));
    registerCustomStat(ResourceLocation("minecraft:firework_rocket_used"));
    registerCustomStat(ResourceLocation("minecraft:record_played"));
    registerCustomStat(ResourceLocation("minecraft:noteblock_played"));
    registerCustomStat(ResourceLocation("minecraft:noteblock_tuned"));
    registerCustomStat(ResourceLocation("minecraft:flower_potted"));
    registerCustomStat(ResourceLocation("minecraft:trapped_chest_triggered"));
    registerCustomStat(ResourceLocation("minecraft:beacon_interaction"));

    // ========== 目标相关 ==========
    registerCustomStat(ResourceLocation("minecraft:target_hit"));

    // ========== 物品交互 ==========
    registerCustomStat(ResourceLocation("minecraft:interact_with_blast_furnace"));
    registerCustomStat(ResourceLocation("minecraft:interact_with_smoker"));
    registerCustomStat(ResourceLocation("minecraft:interact_with_lectern"));
    registerCustomStat(ResourceLocation("minecraft:interact_with_campfire"));
    registerCustomStat(ResourceLocation("minecraft:interact_with_composter"));
    registerCustomStat(ResourceLocation("minecraft:interact_with_barrel"));
    registerCustomStat(ResourceLocation("minecraft:interact_with_smithing_table"));
    registerCustomStat(ResourceLocation("minecraft:interact_with_stonecutter"));

    // ========== 其他 ==========
    registerCustomStat(ResourceLocation("minecraft:leave_game"));
    registerCustomStat(ResourceLocation("minecraft:time_since_death"));
    registerCustomStat(ResourceLocation("minecraft:time_since_rest"));
    registerCustomStat(ResourceLocation("minecraft:sneak_time"));
    registerCustomStat(ResourceLocation("minecraft:walk_under_water_one_cm"));
    registerCustomStat(ResourceLocation("minecraft:walk_on_water_one_cm"));
    registerCustomStat(ResourceLocation("minecraft:boat_one_cm"));
    registerCustomStat(ResourceLocation("minecraft:aviate_one_cm"));
    registerCustomStat(ResourceLocation("minecraft:horse_one_cm"));
    registerCustomStat(ResourceLocation("minecraft:llama_one_cm"));
    registerCustomStat(ResourceLocation("minecraft:pig_one_cm"));
    registerCustomStat(ResourceLocation("minecraft:minecart_one_cm"));
    registerCustomStat(ResourceLocation("minecraft:trigger_trapped_chest"));
    registerCustomStat(ResourceLocation("minecraft:open_barrel"));
    registerCustomStat(ResourceLocation("minecraft:interact_with_beacon"));
    registerCustomStat(ResourceLocation("minecraft:interact_with_brewingstand"));
    registerCustomStat(ResourceLocation("minecraft:interact_with_crafting_table"));
    registerCustomStat(ResourceLocation("minecraft:interact_with_furnace"));
    registerCustomStat(ResourceLocation("minecraft:interact_with_anvil"));
    registerCustomStat(ResourceLocation("minecraft:interact_with_grindstone"));
    registerCustomStat(ResourceLocation("minecraft:interact_with_cartography_table"));
    registerCustomStat(ResourceLocation("minecraft:interact_with_loom"));
    registerCustomStat(ResourceLocation("minecraft:interact_with_stonecutter"));
    registerCustomStat(ResourceLocation("minecraft:interact_with_smithing_table"));
}

void StatRegistry::registerMinedStat(const ResourceLocation& blockId) {
    ResourceLocation fullId = buildStatLocation(StatType::Mined, blockId);
    m_stats[fullId] = {StatType::Mined, blockId};
}

ResourceLocation StatRegistry::getMinedStatId(const ResourceLocation& blockId) const {
    return buildStatLocation(StatType::Mined, blockId);
}

void StatRegistry::registerCraftedStat(const ResourceLocation& itemId) {
    ResourceLocation fullId = buildStatLocation(StatType::Crafted, itemId);
    m_stats[fullId] = {StatType::Crafted, itemId};
}

void StatRegistry::registerUsedStat(const ResourceLocation& itemId) {
    ResourceLocation fullId = buildStatLocation(StatType::Used, itemId);
    m_stats[fullId] = {StatType::Used, itemId};
}

void StatRegistry::registerBrokenStat(const ResourceLocation& itemId) {
    ResourceLocation fullId = buildStatLocation(StatType::Broken, itemId);
    m_stats[fullId] = {StatType::Broken, itemId};
}

void StatRegistry::registerPickedUpStat(const ResourceLocation& itemId) {
    ResourceLocation fullId = buildStatLocation(StatType::PickedUp, itemId);
    m_stats[fullId] = {StatType::PickedUp, itemId};
}

void StatRegistry::registerDroppedStat(const ResourceLocation& itemId) {
    ResourceLocation fullId = buildStatLocation(StatType::Dropped, itemId);
    m_stats[fullId] = {StatType::Dropped, itemId};
}

void StatRegistry::registerKilledStat(const ResourceLocation& entityId) {
    ResourceLocation fullId = buildStatLocation(StatType::Killed, entityId);
    m_stats[fullId] = {StatType::Killed, entityId};
}

void StatRegistry::registerKilledByStat(const ResourceLocation& entityId) {
    ResourceLocation fullId = buildStatLocation(StatType::KilledBy, entityId);
    m_stats[fullId] = {StatType::KilledBy, entityId};
}

void StatRegistry::registerCustomStat(const ResourceLocation& statId) {
    ResourceLocation fullId = buildStatLocation(StatType::Custom, statId);
    m_stats[fullId] = {StatType::Custom, statId};
}

bool StatRegistry::hasStat(StatType type, const ResourceLocation& id) const {
    ResourceLocation fullId = buildStatLocation(type, id);
    return m_stats.find(fullId) != m_stats.end();
}

bool StatRegistry::hasStat(const ResourceLocation& fullId) const {
    return m_stats.find(fullId) != m_stats.end();
}

std::vector<ResourceLocation> StatRegistry::getAllStatIds() const {
    std::vector<ResourceLocation> ids;
    ids.reserve(m_stats.size());
    for (const auto& [id, _] : m_stats) {
        ids.push_back(id);
    }
    return ids;
}

std::vector<ResourceLocation> StatRegistry::getStatIdsByType(StatType type) const {
    std::vector<ResourceLocation> ids;
    for (const auto& [id, pair] : m_stats) {
        if (pair.first == type) {
            ids.push_back(id);
        }
    }
    return ids;
}

} // namespace stats
} // namespace server
} // namespace mc
