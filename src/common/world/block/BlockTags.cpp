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

#include "BlockTags.hpp"
#include "Block.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <algorithm>
#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mc {

// ============================================================================
// BlockTag Implementation
// ============================================================================

BlockTag::BlockTag(ResourceLocation id) noexcept
    : m_id(std::move(id))
{}

void BlockTag::add(const ResourceLocation& blockId)
{
    m_blockIds.insert(blockId);
}

void BlockTag::addAll(const std::vector<ResourceLocation>& blockIds)
{
    for (const auto& id : blockIds) {
        m_blockIds.insert(id);
    }
}

void BlockTag::clear()
{
    m_blockIds.clear();
}

bool BlockTag::contains(const ResourceLocation& blockId) const noexcept
{
    return m_blockIds.find(blockId) != m_blockIds.end();
}

bool BlockTag::contains(const Block* block) const
{
    if (block == nullptr) {
        return false;
    }
    return contains(block->blockLocation());
}

bool BlockTag::contains(const Block& block) const
{
    return contains(block.blockLocation());
}

bool BlockTag::contains(const BlockState& state) const
{
    return contains(state.blockLocation());
}

void BlockTag::addTagReference(const ResourceLocation& tagId)
{
    if (std::find(m_referencedTagIds.begin(), m_referencedTagIds.end(), tagId) == m_referencedTagIds.end()) {
        m_referencedTagIds.push_back(tagId);
    }
}

bool BlockTag::containsIncludingReferences(const ResourceLocation& blockId) const
{
    std::unordered_set<ResourceLocation> visited;
    std::vector<const BlockTag*> stack{this};
    while (!stack.empty()) {
        const BlockTag* current = stack.back();
        stack.pop_back();
        if (!visited.insert(current->m_id).second) {
            continue;
        }
        if (current->m_blockIds.find(blockId) != current->m_blockIds.end()) {
            return true;
        }
        for (const auto& referenced : current->m_referencedTagIds) {
            if (const BlockTag* tag = BlockTags::getTag(referenced); tag != nullptr) {
                stack.push_back(tag);
            }
        }
    }
    return false;
}

bool BlockTag::containsAllOf(const BlockTag& target) const
{
    std::unordered_set<ResourceLocation> blocks;
    std::unordered_set<ResourceLocation> tags;
    std::unordered_set<ResourceLocation> visited;
    collectIncludingReferences(blocks, tags, visited);
    if (blocks.find(target.m_id) != blocks.end() || tags.find(target.m_id) != tags.end()) {
        return true;
    }
    if (!target.m_blockIds.empty() && blocks.size() < target.m_blockIds.size()) {
        return false;
    }
    for (const auto& blockId : target.m_blockIds) {
        if (blocks.find(blockId) == blocks.end()) {
            return false;
        }
    }
    return true;
}

void BlockTag::collectIncludingReferences(std::unordered_set<ResourceLocation>& blocks,
    std::unordered_set<ResourceLocation>& referencedTags,
    std::unordered_set<ResourceLocation>& visited) const
{
    if (!visited.insert(m_id).second) {
        return;
    }
    blocks.insert(m_blockIds.begin(), m_blockIds.end());
    for (const auto& referenced : m_referencedTagIds) {
        referencedTags.insert(referenced);
        if (const BlockTag* tag = BlockTags::getTag(referenced); tag != nullptr) {
            tag->collectIncludingReferences(blocks, referencedTags, visited);
        }
    }
}

// ============================================================================
// BlockTags Implementation
// ============================================================================

bool BlockTags::s_initialized = false;

std::unordered_map<ResourceLocation, std::unique_ptr<BlockTag>>& BlockTags::_getTags()
{
    static std::unordered_map<ResourceLocation, std::unique_ptr<BlockTag>> tags;
    return tags;
}

BlockTag& BlockTags::LOGS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "logs"));
    }
    return *tag;
}

BlockTag& BlockTags::JUNGLE_LOGS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "jungle_logs"));
    }
    return *tag;
}

BlockTag& BlockTags::OAK_LOGS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "oak_logs"));
    }
    return *tag;
}

BlockTag& BlockTags::SPRUCE_LOGS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "spruce_logs"));
    }
    return *tag;
}

BlockTag& BlockTags::BIRCH_LOGS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "birch_logs"));
    }
    return *tag;
}

BlockTag& BlockTags::ACACIA_LOGS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "acacia_logs"));
    }
    return *tag;
}

BlockTag& BlockTags::DARK_OAK_LOGS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "dark_oak_logs"));
    }
    return *tag;
}

BlockTag& BlockTags::CRIMSON_STEMS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "crimson_stems"));
    }
    return *tag;
}

BlockTag& BlockTags::WARPED_STEMS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "warped_stems"));
    }
    return *tag;
}

BlockTag& BlockTags::LEAVES()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "leaves"));
    }
    return *tag;
}

BlockTag& BlockTags::PLANKS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "planks"));
    }
    return *tag;
}

BlockTag& BlockTags::DIRT()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "dirt"));
    }
    return *tag;
}

BlockTag& BlockTags::SAND()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "sand"));
    }
    return *tag;
}

BlockTag& BlockTags::TERRACOTTA()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "terracotta"));
    }
    return *tag;
}

BlockTag& BlockTags::DRY_VEGETATION_MAY_PLACE_ON()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "dry_vegetation_may_place_on"));
    }
    return *tag;
}

BlockTag& BlockTags::NYLIUM()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "nylium"));
    }
    return *tag;
}

BlockTag& BlockTags::FIRE()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "fire"));
    }
    return *tag;
}

BlockTag& BlockTags::INVALID_SPAWN_INSIDE()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        // 数据包缺该标签文件时 registerTag 兜底创建空标签（等价原版空集合语义）。
        tag = &registerTag(ResourceLocation("minecraft", "invalid_spawn_inside"));
    }
    return *tag;
}

BlockTag& BlockTags::SOUL_FIRE_BASE_BLOCKS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "soul_fire_base_blocks"));
    }
    return *tag;
}

BlockTag& BlockTags::BLOCKS_WIND_CHARGE_EXPLOSIONS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "blocks_wind_charge_explosions"));
    }
    return *tag;
}

BlockTag& BlockTags::WITHER_SUMMON_BASE_BLOCKS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "wither_summon_base_blocks"));
    }
    return *tag;
}

BlockTag& BlockTags::INFINIBURN_OVERWORLD()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "infiniburn_overworld"));
    }
    return *tag;
}

BlockTag& BlockTags::INFINIBURN_NETHER()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "infiniburn_nether"));
    }
    return *tag;
}

BlockTag& BlockTags::INFINIBURN_END()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "infiniburn_end"));
    }
    return *tag;
}

BlockTag& BlockTags::CAMPFIRES()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "campfires"));
    }
    return *tag;
}

BlockTag& BlockTags::CANDLES()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "candles"));
    }
    return *tag;
}

BlockTag& BlockTags::CANDLE_CAKES()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "candle_cakes"));
    }
    return *tag;
}

BlockTag& BlockTags::WOOL()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "wool"));
    }
    return *tag;
}

BlockTag& BlockTags::WOOL_CARPETS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "wool_carpets"));
    }
    return *tag;
}

BlockTag& BlockTags::BEDS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "beds"));
    }
    return *tag;
}

BlockTag& BlockTags::WOODEN_FENCES()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "wooden_fences"));
    }
    return *tag;
}

BlockTag& BlockTags::FENCES()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "fences"));
    }
    return *tag;
}

BlockTag& BlockTags::FENCE_GATES()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "fence_gates"));
    }
    return *tag;
}

BlockTag& BlockTags::UNSTABLE_BOTTOM_CENTER()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "unstable_bottom_center"));
    }
    return *tag;
}

BlockTag& BlockTags::WOODEN_SHELVES()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "wooden_shelves"));
    }
    return *tag;
}

BlockTag& BlockTags::BAMBOO_PLANTABLE_ON()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "bamboo_plantable_on"));
    }
    return *tag;
}

BlockTag& BlockTags::MUSHROOM_GROW_BLOCK()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "mushroom_grow_block"));
    }
    return *tag;
}

BlockTag& BlockTags::CORALS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "corals"));
    }
    return *tag;
}

BlockTag& BlockTags::WALL_CORALS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "wall_corals"));
    }
    return *tag;
}

BlockTag& BlockTags::UNDERWATER_BONEMEALS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "underwater_bonemeals"));
    }
    return *tag;
}

BlockTag& BlockTags::STRIDER_WARM_BLOCKS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "strider_warm_blocks"));
    }
    return *tag;
}

BlockTag& BlockTags::HOGLIN_REPELLENTS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "hoglin_repellents"));
    }
    return *tag;
}

BlockTag& BlockTags::PIGLIN_REPELLENTS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "piglin_repellents"));
    }
    return *tag;
}

BlockTag& BlockTags::SMALL_FLOWERS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "small_flowers"));
    }
    return *tag;
}

BlockTag& BlockTags::FLOWERS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "flowers"));
    }
    return *tag;
}

BlockTag& BlockTags::FLOWER_POTS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "flower_pots"));
    }
    return *tag;
}

BlockTag& BlockTags::SAPLINGS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "saplings"));
    }
    return *tag;
}

BlockTag& BlockTags::BEEHIVES()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "beehives"));
    }
    return *tag;
}

BlockTag& BlockTags::BEE_ATTRACTIVE()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "bee_attractive"));
    }
    return *tag;
}

BlockTag& BlockTags::DOES_NOT_BLOCK_HOPPERS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "does_not_block_hoppers"));
    }
    return *tag;
}

BlockTag& BlockTags::BEE_GROWABLES()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "bee_growables"));
    }
    return *tag;
}

BlockTag& BlockTags::ENDERMAN_HOLDABLE()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "enderman_holdable"));
    }
    return *tag;
}

BlockTag& BlockTags::WITHER_IMMUNE()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "wither_immune"));
    }
    return *tag;
}

// ============================================================================
// 末影龙标签
// ============================================================================

BlockTag& BlockTags::DRAGON_IMMUNE()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "dragon_immune"));
    }
    return *tag;
}

BlockTag& BlockTags::DRAGON_TRANSPARENT()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "dragon_transparent"));
    }
    return *tag;
}

// ============================================================================
// 1.17 Caves & Cliffs
// ============================================================================

BlockTag& BlockTags::COPPER_ORES()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "copper_ores"));
    }
    return *tag;
}

BlockTag& BlockTags::DEEPSLATE_ORE_REPLACEABLES()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "deepslate_ore_replaceables"));
    }
    return *tag;
}

BlockTag& BlockTags::STONE_ORE_REPLACEABLES()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "stone_ore_replaceables"));
    }
    return *tag;
}

BlockTag& BlockTags::BASE_STONE_OVERWORLD()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "base_stone_overworld"));
    }
    return *tag;
}

BlockTag& BlockTags::BASE_STONE_NETHER()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "base_stone_nether"));
    }
    return *tag;
}

BlockTag& BlockTags::TRAIL_RUINS_REPLACEABLE()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "trail_ruins_replaceable"));
    }
    return *tag;
}

BlockTag& BlockTags::AZALEA_GROWS_ON()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "azalea_grows_on"));
    }
    return *tag;
}

BlockTag& BlockTags::CRYSTAL_SOUND_BLOCKS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "crystal_sound_blocks"));
    }
    return *tag;
}

BlockTag& BlockTags::COMBINATION_STEP_SOUND_BLOCKS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "combination_step_sound_blocks"));
    }
    return *tag;
}

BlockTag& BlockTags::INSIDE_STEP_SOUND_BLOCKS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "inside_step_sound_blocks"));
    }
    return *tag;
}

BlockTag& BlockTags::CAVE_VINES()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "cave_vines"));
    }
    return *tag;
}

BlockTag& BlockTags::MOSS_REPLACEABLE()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "moss_replaceable"));
    }
    return *tag;
}

BlockTag& BlockTags::DRIPSTONE_REPLACEABLE_BLOCKS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "dripstone_replaceable_blocks"));
    }
    return *tag;
}

BlockTag& BlockTags::LUSH_GROUND_REPLACEABLE()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "lush_ground_replaceable"));
    }
    return *tag;
}

BlockTag& BlockTags::AZALEA_ROOT_REPLACEABLE()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "azalea_root_replaceable"));
    }
    return *tag;
}

BlockTag& BlockTags::COPPER()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "copper"));
    }
    return *tag;
}

BlockTag& BlockTags::COPPER_GOLEM_STATUES()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "copper_golem_statues"));
    }
    return *tag;
}

BlockTag& BlockTags::COPPER_CHESTS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "copper_chests"));
    }
    return *tag;
}

BlockTag& BlockTags::LIGHTNING_RODS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "lightning_rods"));
    }
    return *tag;
}

BlockTag& BlockTags::DAMPENS_VIBRATIONS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "dampens_vibrations"));
    }
    return *tag;
}

BlockTag& BlockTags::OCCLUDES_VIBRATION_SIGNALS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "occludes_vibration_signals"));
    }
    return *tag;
}

BlockTag& BlockTags::OVERWORLD_NATURAL_LOGS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "overworld_natural_logs"));
    }
    return *tag;
}

BlockTag& BlockTags::SNAPS_GOAT_HORN()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "snaps_goat_horn"));
    }
    return *tag;
}

BlockTag& BlockTags::SNOW()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "snow"));
    }
    return *tag;
}

BlockTag& BlockTags::CLIMBABLE()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "climbable"));
    }
    return *tag;
}

BlockTag& BlockTags::CAN_GLIDE_THROUGH()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "can_glide_through"));
    }
    return *tag;
}

BlockTag& BlockTags::SOUL_SPEED_BLOCKS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "soul_speed_blocks"));
    }
    return *tag;
}

BlockTag& BlockTags::FALL_DAMAGE_RESETTING()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "fall_damage_resetting"));
    }
    return *tag;
}

// ============================================================================
// 1.19 Wild Update
// ============================================================================

BlockTag& BlockTags::SCULK_REPLACEABLE()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "sculk_replaceable"));
    }
    return *tag;
}

BlockTag& BlockTags::SCULK_REPLACEABLE_WORLD_GEN()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "sculk_replaceable_world_gen"));
    }
    return *tag;
}

BlockTag& BlockTags::ANCIENT_CITY_REPLACEABLE()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "ancient_city_replaceable"));
    }
    return *tag;
}

BlockTag& BlockTags::VIBRATION_RESONATORS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "vibration_resonators"));
    }
    return *tag;
}

BlockTag& BlockTags::FROGS_SPAWNABLE_ON()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "frogs_spawnable_on"));
    }
    return *tag;
}

BlockTag& BlockTags::CONVERTABLE_TO_MUD()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "convertable_to_mud"));
    }
    return *tag;
}

BlockTag& BlockTags::MANGROVE_LOGS_CAN_GROW_THROUGH()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "mangrove_logs_can_grow_through"));
    }
    return *tag;
}

BlockTag& BlockTags::MANGROVE_ROOTS_CAN_GROW_THROUGH()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "mangrove_roots_can_grow_through"));
    }
    return *tag;
}

BlockTag& BlockTags::MANGROVE_LOGS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "mangrove_logs"));
    }
    return *tag;
}

// ============================================================================
// 1.20 Trails & Tales
// ============================================================================

BlockTag& BlockTags::CHERRY_LOGS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "cherry_logs"));
    }
    return *tag;
}

BlockTag& BlockTags::BAMBOO_BLOCKS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "bamboo_blocks"));
    }
    return *tag;
}

BlockTag& BlockTags::SNIFFER_DIGGABLE_BLOCK()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "sniffer_diggable_block"));
    }
    return *tag;
}

BlockTag& BlockTags::SNIFFER_EGG_HATCH_BOOST()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "sniffer_egg_hatch_boost"));
    }
    return *tag;
}

// ============================================================================
// 1.21 Tricky Trials
// ============================================================================

BlockTag& BlockTags::FEATURES_CANNOT_REPLACE()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "features_cannot_replace"));
    }
    return *tag;
}

BlockTag& BlockTags::LAVA_POOL_STONE_CANNOT_REPLACE()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "lava_pool_stone_cannot_replace"));
    }
    return *tag;
}

BlockTag& BlockTags::ENCHANTMENT_POWER_PROVIDER()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "enchantment_power_provider"));
    }
    return *tag;
}

BlockTag& BlockTags::ENCHANTMENT_POWER_TRANSMITTER()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "enchantment_power_transmitter"));
    }
    return *tag;
}

BlockTag& BlockTags::MAINTAINS_FARMLAND()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "maintains_farmland"));
    }
    return *tag;
}

// ============================================================================
// 1.21.2+ Garden Awakens
// ============================================================================

BlockTag& BlockTags::PALE_OAK_LOGS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "pale_oak_logs"));
    }
    return *tag;
}

BlockTag& BlockTags::REPLACEABLE_BY_TREES()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "replaceable_by_trees"));
    }
    return *tag;
}

BlockTag& BlockTags::OVERWORLD_CARVER_REPLACEABLES()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "overworld_carver_replaceables"));
    }
    return *tag;
}

BlockTag& BlockTags::NETHER_CARVER_REPLACEABLES()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "nether_carver_replaceables"));
    }
    return *tag;
}

BlockTag& BlockTags::ANVIL()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "anvil"));
    }
    return *tag;
}

BlockTag& BlockTags::SNOW_LAYER_CANNOT_SURVIVE_ON()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "snow_layer_cannot_survive_on"));
    }
    return *tag;
}

BlockTag& BlockTags::SNOW_LAYER_CAN_SURVIVE_ON()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "snow_layer_can_survive_on"));
    }
    return *tag;
}

BlockTag& BlockTags::SMALL_DRIPLEAF_PLACEABLE()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "small_dripleaf_placeable"));
    }
    return *tag;
}

BlockTag& BlockTags::BIG_DRIPLEAF_PLACEABLE()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "big_dripleaf_placeable"));
    }
    return *tag;
}

// ============================================================================
// 建筑方块形状标签
// ============================================================================

BlockTag& BlockTags::STAIRS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "stairs"));
    }
    return *tag;
}

BlockTag& BlockTags::SLABS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "slabs"));
    }
    return *tag;
}

BlockTag& BlockTags::WALLS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "walls"));
    }
    return *tag;
}

BlockTag& BlockTags::GUARDED_BY_PIGLINS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "guarded_by_piglins"));
    }
    return *tag;
}

BlockTag& BlockTags::BARS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "bars"));
    }
    return *tag;
}

BlockTag& BlockTags::CHAINS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "chains"));
    }
    return *tag;
}

BlockTag& BlockTags::SHULKER_BOXES()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "shulker_boxes"));
    }
    return *tag;
}

BlockTag& BlockTags::WALL_POST_OVERRIDE()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "wall_post_override"));
    }
    return *tag;
}

BlockTag& BlockTags::CAULDRONS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "cauldrons"));
    }
    return *tag;
}

BlockTag& BlockTags::WOODEN_DOORS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "wooden_doors"));
    }
    return *tag;
}

BlockTag& BlockTags::DOORS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "doors"));
    }
    return *tag;
}

BlockTag& BlockTags::WOODEN_TRAPDOORS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "wooden_trapdoors"));
    }
    return *tag;
}

BlockTag& BlockTags::TRAPDOORS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "trapdoors"));
    }
    return *tag;
}

void BlockTags::initialize()
{
    if (s_initialized) {
        return;
    }

    auto& tags = _getTags();

    // 全部标签来自原版数据包（data/minecraft/tags/block/*.json），成员已在生成期
    // 展开 #tag 引用，等价原版 Tag#getValues() 的口径。以前这里是逐个手写
    // addAll(...)，与数据包之间会悄悄漂移（首次接入 block_tags 差分时查出 82 个
    // 标签缺失、22 个标签成员不全）。
    size_t entryCount = 0;
    const VanillaBlockTagEntry* table = vanillaBlockTagTable(entryCount);
    for (size_t i = 0; i < entryCount; ++i) {
        const VanillaBlockTagEntry& entry = table[i];
        auto tag = std::make_unique<BlockTag>(ResourceLocation("minecraft", entry.name));
        std::vector<ResourceLocation> values;
        values.reserve(entry.count);
        for (std::size_t k = 0; k < entry.count; ++k) {
            values.emplace_back("minecraft", entry.values[k]);
        }
        tag->addAll(values);
        tags[tag->getId()] = std::move(tag);
    }

    s_initialized = true;
}

BlockTag* BlockTags::getTag(const ResourceLocation& id)
{
    auto& tags = _getTags();
    auto it = tags.find(id);
    if (it != tags.end()) {
        return it->second.get();
    }
    return nullptr;
}

BlockTag& BlockTags::registerTag(const ResourceLocation& id)
{
    auto& tags = _getTags();
    auto it = tags.find(id);
    if (it != tags.end()) {
        return *it->second;
    }
    auto inserted = tags.emplace(id, std::make_unique<BlockTag>(id));
    return *inserted.first->second;
}

void BlockTags::forEachTag(std::function<void(BlockTag&)> callback)
{
    auto& tags = _getTags();
    for (auto& [id, tag] : tags) {
        callback(*tag);
    }
}

} // namespace mc
