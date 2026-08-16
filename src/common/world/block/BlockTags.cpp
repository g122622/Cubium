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

BlockTag& BlockTags::STONE()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "stone"));
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

BlockTag& BlockTags::SOUL_FIRE_BASE_BLOCKS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "soul_fire_base_blocks"));
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

BlockTag& BlockTags::VALID_SWEET_BERRY_BUSH_GROUND()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "valid_sweet_berry_bush_ground"));
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

BlockTag& BlockTags::TALL_FLOWERS()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "tall_flowers"));
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

BlockTag& BlockTags::BASE_STONE_OVERWORLD()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "base_stone_overworld"));
    }
    return *tag;
}

BlockTag& BlockTags::DRIPSTONE_REPLACEABLE()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "dripstone_replaceable"));
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

BlockTag& BlockTags::SNOW()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "snow"));
    }
    return *tag;
}

BlockTag& BlockTags::POWDER_SNOW_WALKABLE_MOVED()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "powder_snow_walkable_moved"));
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

BlockTag& BlockTags::NON_FLAMMABLE_WOOD()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "non_flammable_wood"));
    }
    return *tag;
}

void BlockTags::initialize()
{
    if (s_initialized) {
        return;
    }

    auto& tags = _getTags();

    // 创建 LOGS 标签
    auto logs = std::make_unique<BlockTag>(ResourceLocation("minecraft", "logs"));
    tags[logs->getId()] = std::move(logs);

    // 创建 JUNGLE_LOGS 标签
    auto jungleLogs = std::make_unique<BlockTag>(ResourceLocation("minecraft", "jungle_logs"));
    jungleLogs->addAll({ResourceLocation("minecraft", "jungle_log"),
        ResourceLocation("minecraft", "jungle_wood"),
        ResourceLocation("minecraft", "stripped_jungle_log"),
        ResourceLocation("minecraft", "stripped_jungle_wood")});
    tags[jungleLogs->getId()] = std::move(jungleLogs);

    // 创建 OAK_LOGS 标签
    auto oakLogs = std::make_unique<BlockTag>(ResourceLocation("minecraft", "oak_logs"));
    oakLogs->addAll({ResourceLocation("minecraft", "oak_log"),
        ResourceLocation("minecraft", "oak_wood"),
        ResourceLocation("minecraft", "stripped_oak_log"),
        ResourceLocation("minecraft", "stripped_oak_wood")});
    tags[oakLogs->getId()] = std::move(oakLogs);

    // 创建 SPRUCE_LOGS 标签
    auto spruceLogs = std::make_unique<BlockTag>(ResourceLocation("minecraft", "spruce_logs"));
    spruceLogs->addAll({ResourceLocation("minecraft", "spruce_log"),
        ResourceLocation("minecraft", "spruce_wood"),
        ResourceLocation("minecraft", "stripped_spruce_log"),
        ResourceLocation("minecraft", "stripped_spruce_wood")});
    tags[spruceLogs->getId()] = std::move(spruceLogs);

    // 创建 BIRCH_LOGS 标签
    auto birchLogs = std::make_unique<BlockTag>(ResourceLocation("minecraft", "birch_logs"));
    birchLogs->addAll({ResourceLocation("minecraft", "birch_log"),
        ResourceLocation("minecraft", "birch_wood"),
        ResourceLocation("minecraft", "stripped_birch_log"),
        ResourceLocation("minecraft", "stripped_birch_wood")});
    tags[birchLogs->getId()] = std::move(birchLogs);

    // 创建 ACACIA_LOGS 标签
    auto acaciaLogs = std::make_unique<BlockTag>(ResourceLocation("minecraft", "acacia_logs"));
    acaciaLogs->addAll({ResourceLocation("minecraft", "acacia_log"),
        ResourceLocation("minecraft", "acacia_wood"),
        ResourceLocation("minecraft", "stripped_acacia_log"),
        ResourceLocation("minecraft", "stripped_acacia_wood")});
    tags[acaciaLogs->getId()] = std::move(acaciaLogs);

    // 创建 DARK_OAK_LOGS 标签
    auto darkOakLogs = std::make_unique<BlockTag>(ResourceLocation("minecraft", "dark_oak_logs"));
    darkOakLogs->addAll({ResourceLocation("minecraft", "dark_oak_log"),
        ResourceLocation("minecraft", "dark_oak_wood"),
        ResourceLocation("minecraft", "stripped_dark_oak_log"),
        ResourceLocation("minecraft", "stripped_dark_oak_wood")});
    tags[darkOakLogs->getId()] = std::move(darkOakLogs);

    // 创建 CRIMSON_STEMS 标签
    auto crimsonStems = std::make_unique<BlockTag>(ResourceLocation("minecraft", "crimson_stems"));
    crimsonStems->addAll({ResourceLocation("minecraft", "crimson_stem"),
        ResourceLocation("minecraft", "stripped_crimson_stem"),
        ResourceLocation("minecraft", "crimson_hyphae"),
        ResourceLocation("minecraft", "stripped_crimson_hyphae")});
    tags[crimsonStems->getId()] = std::move(crimsonStems);

    // 创建 WARPED_STEMS 标签
    auto warpedStems = std::make_unique<BlockTag>(ResourceLocation("minecraft", "warped_stems"));
    warpedStems->addAll({ResourceLocation("minecraft", "warped_stem"),
        ResourceLocation("minecraft", "stripped_warped_stem"),
        ResourceLocation("minecraft", "warped_hyphae"),
        ResourceLocation("minecraft", "stripped_warped_hyphae")});
    tags[warpedStems->getId()] = std::move(warpedStems);

    // 创建 LEAVES 标签
    auto leaves = std::make_unique<BlockTag>(ResourceLocation("minecraft", "leaves"));
    leaves->addAll({ResourceLocation("minecraft", "oak_leaves"),
        ResourceLocation("minecraft", "spruce_leaves"),
        ResourceLocation("minecraft", "birch_leaves"),
        ResourceLocation("minecraft", "jungle_leaves"),
        ResourceLocation("minecraft", "acacia_leaves"),
        ResourceLocation("minecraft", "dark_oak_leaves"),
        ResourceLocation("minecraft", "azalea_leaves"),
        ResourceLocation("minecraft", "flowering_azalea_leaves")});
    tags[leaves->getId()] = std::move(leaves);

    // 创建 PLANKS 标签
    auto planks = std::make_unique<BlockTag>(ResourceLocation("minecraft", "planks"));
    planks->addAll({ResourceLocation("minecraft", "oak_planks"),
        ResourceLocation("minecraft", "spruce_planks"),
        ResourceLocation("minecraft", "birch_planks"),
        ResourceLocation("minecraft", "jungle_planks"),
        ResourceLocation("minecraft", "acacia_planks"),
        ResourceLocation("minecraft", "dark_oak_planks"),
        ResourceLocation("minecraft", "crimson_planks"),
        ResourceLocation("minecraft", "warped_planks")});
    tags[planks->getId()] = std::move(planks);

    // 创建 DIRT 标签
    auto dirt = std::make_unique<BlockTag>(ResourceLocation("minecraft", "dirt"));
    dirt->addAll({ResourceLocation("minecraft", "dirt"),
        ResourceLocation("minecraft", "grass_block"),
        ResourceLocation("minecraft", "podzol"),
        ResourceLocation("minecraft", "coarse_dirt"),
        ResourceLocation("minecraft", "mycelium")});
    tags[dirt->getId()] = std::move(dirt);

    // 创建 SAND 标签
    auto sand = std::make_unique<BlockTag>(ResourceLocation("minecraft", "sand"));
    sand->addAll({ResourceLocation("minecraft", "sand"),
        ResourceLocation("minecraft", "red_sand"),
        ResourceLocation("minecraft", "suspicious_sand")});
    tags[sand->getId()] = std::move(sand);

    // 创建 TERRACOTTA 标签（原色陶瓦 + 16 色陶瓦）
    // 成员对齐 datapacks/Vanilla/.../tags/block/terracotta.json
    auto terracotta = std::make_unique<BlockTag>(ResourceLocation("minecraft", "terracotta"));
    terracotta->addAll({ResourceLocation("minecraft", "terracotta"),
        ResourceLocation("minecraft", "white_terracotta"),
        ResourceLocation("minecraft", "orange_terracotta"),
        ResourceLocation("minecraft", "magenta_terracotta"),
        ResourceLocation("minecraft", "light_blue_terracotta"),
        ResourceLocation("minecraft", "yellow_terracotta"),
        ResourceLocation("minecraft", "lime_terracotta"),
        ResourceLocation("minecraft", "pink_terracotta"),
        ResourceLocation("minecraft", "gray_terracotta"),
        ResourceLocation("minecraft", "light_gray_terracotta"),
        ResourceLocation("minecraft", "cyan_terracotta"),
        ResourceLocation("minecraft", "purple_terracotta"),
        ResourceLocation("minecraft", "blue_terracotta"),
        ResourceLocation("minecraft", "brown_terracotta"),
        ResourceLocation("minecraft", "green_terracotta"),
        ResourceLocation("minecraft", "red_terracotta"),
        ResourceLocation("minecraft", "black_terracotta")});
    tags[terracotta->getId()] = std::move(terracotta);

    // 创建 DRY_VEGETATION_MAY_PLACE_ON 标签（干草类可种植标签）
    // vanilla 定义：.addTag(SAND).addTag(TERRACOTTA).addTag(DIRT).add(FARMLAND)
    // BlockTag 是扁平 unordered_set，不支持 #tag 嵌套引用，故手动合并三个已建标签成员
    // （同 lava_pool_stone_cannot_replace 的合并模式），再单独加入 farmland。
    {
        auto dryVegetationMayPlaceOn =
            std::make_unique<BlockTag>(ResourceLocation("minecraft", "dry_vegetation_may_place_on"));
        std::vector<ResourceLocation> merged;
        const auto collect = [&merged](const BlockTag& src) {
            const auto& ids = src.getBlockIds();
            merged.insert(merged.end(), ids.begin(), ids.end());
        };
        collect(*tags.at(ResourceLocation("minecraft", "sand")));
        collect(*tags.at(ResourceLocation("minecraft", "terracotta")));
        collect(*tags.at(ResourceLocation("minecraft", "dirt")));
        merged.push_back(ResourceLocation("minecraft", "farmland"));
        dryVegetationMayPlaceOn->addAll(merged);
        tags[dryVegetationMayPlaceOn->getId()] = std::move(dryVegetationMayPlaceOn);
    }

    // 创建 STONE 标签
    auto stone = std::make_unique<BlockTag>(ResourceLocation("minecraft", "stone"));
    stone->addAll({ResourceLocation("minecraft", "stone"),
        ResourceLocation("minecraft", "granite"),
        ResourceLocation("minecraft", "polished_granite"),
        ResourceLocation("minecraft", "diorite"),
        ResourceLocation("minecraft", "polished_diorite"),
        ResourceLocation("minecraft", "andesite"),
        ResourceLocation("minecraft", "polished_andesite")});
    tags[stone->getId()] = std::move(stone);

    // 创建 NYLIUM 标签（crimson_nylium / warped_nylium）
    auto nylium = std::make_unique<BlockTag>(ResourceLocation("minecraft", "nylium"));
    nylium->addAll({ResourceLocation("minecraft", "crimson_nylium"), ResourceLocation("minecraft", "warped_nylium")});
    tags[nylium->getId()] = std::move(nylium);

    // 创建 FIRE 标签
    auto fire = std::make_unique<BlockTag>(ResourceLocation("minecraft", "fire"));
    fire->addAll({ResourceLocation("minecraft", "fire"), ResourceLocation("minecraft", "soul_fire")});
    tags[fire->getId()] = std::move(fire);

    // 创建 SOUL_FIRE_BASE_BLOCKS 标签（灵魂火基座方块）
    auto soulFireBaseBlocks = std::make_unique<BlockTag>(ResourceLocation("minecraft", "soul_fire_base_blocks"));
    soulFireBaseBlocks->addAll(
        {ResourceLocation("minecraft", "soul_sand"), ResourceLocation("minecraft", "soul_soil")});
    tags[soulFireBaseBlocks->getId()] = std::move(soulFireBaseBlocks);

    // 创建 CAMPFIRES 标签（营火、灵魂营火）
    auto campfires = std::make_unique<BlockTag>(ResourceLocation("minecraft", "campfires"));
    campfires->addAll({ResourceLocation("minecraft", "campfire"), ResourceLocation("minecraft", "soul_campfire")});
    tags[campfires->getId()] = std::move(campfires);

    // 创建 CANDLES 标签（所有蜡烛方块）
    auto candles = std::make_unique<BlockTag>(ResourceLocation("minecraft", "candles"));
    candles->addAll({ResourceLocation("minecraft", "candle"),
        ResourceLocation("minecraft", "white_candle"),
        ResourceLocation("minecraft", "orange_candle"),
        ResourceLocation("minecraft", "magenta_candle"),
        ResourceLocation("minecraft", "light_blue_candle"),
        ResourceLocation("minecraft", "yellow_candle"),
        ResourceLocation("minecraft", "lime_candle"),
        ResourceLocation("minecraft", "pink_candle"),
        ResourceLocation("minecraft", "gray_candle"),
        ResourceLocation("minecraft", "light_gray_candle"),
        ResourceLocation("minecraft", "cyan_candle"),
        ResourceLocation("minecraft", "purple_candle"),
        ResourceLocation("minecraft", "blue_candle"),
        ResourceLocation("minecraft", "brown_candle"),
        ResourceLocation("minecraft", "green_candle"),
        ResourceLocation("minecraft", "red_candle"),
        ResourceLocation("minecraft", "black_candle")});
    tags[candles->getId()] = std::move(candles);

    // 创建 CANDLE_CAKES 标签（所有蜡烛蛋糕方块）
    auto candleCakes = std::make_unique<BlockTag>(ResourceLocation("minecraft", "candle_cakes"));
    candleCakes->addAll({ResourceLocation("minecraft", "candle_cake"),
        ResourceLocation("minecraft", "white_candle_cake"),
        ResourceLocation("minecraft", "orange_candle_cake"),
        ResourceLocation("minecraft", "magenta_candle_cake"),
        ResourceLocation("minecraft", "light_blue_candle_cake"),
        ResourceLocation("minecraft", "yellow_candle_cake"),
        ResourceLocation("minecraft", "lime_candle_cake"),
        ResourceLocation("minecraft", "pink_candle_cake"),
        ResourceLocation("minecraft", "gray_candle_cake"),
        ResourceLocation("minecraft", "light_gray_candle_cake"),
        ResourceLocation("minecraft", "cyan_candle_cake"),
        ResourceLocation("minecraft", "purple_candle_cake"),
        ResourceLocation("minecraft", "blue_candle_cake"),
        ResourceLocation("minecraft", "brown_candle_cake"),
        ResourceLocation("minecraft", "green_candle_cake"),
        ResourceLocation("minecraft", "red_candle_cake"),
        ResourceLocation("minecraft", "black_candle_cake")});
    tags[candleCakes->getId()] = std::move(candleCakes);

    // 创建 WOOL 标签
    auto wool = std::make_unique<BlockTag>(ResourceLocation("minecraft", "wool"));
    wool->addAll({ResourceLocation("minecraft", "white_wool"),
        ResourceLocation("minecraft", "orange_wool"),
        ResourceLocation("minecraft", "magenta_wool"),
        ResourceLocation("minecraft", "light_blue_wool"),
        ResourceLocation("minecraft", "yellow_wool"),
        ResourceLocation("minecraft", "lime_wool"),
        ResourceLocation("minecraft", "pink_wool"),
        ResourceLocation("minecraft", "gray_wool"),
        ResourceLocation("minecraft", "light_gray_wool"),
        ResourceLocation("minecraft", "cyan_wool"),
        ResourceLocation("minecraft", "purple_wool"),
        ResourceLocation("minecraft", "blue_wool"),
        ResourceLocation("minecraft", "brown_wool"),
        ResourceLocation("minecraft", "green_wool"),
        ResourceLocation("minecraft", "red_wool"),
        ResourceLocation("minecraft", "black_wool")});
    tags[wool->getId()] = std::move(wool);

    // 创建 WOOL_CARPETS 标签（所有颜色的地毯方块）
    // 参考: net.minecraft.tags.BlockTags.WOOL_CARPETS
    auto woolCarpets = std::make_unique<BlockTag>(ResourceLocation("minecraft", "wool_carpets"));
    woolCarpets->addAll({ResourceLocation("minecraft", "white_carpet"),
        ResourceLocation("minecraft", "orange_carpet"),
        ResourceLocation("minecraft", "magenta_carpet"),
        ResourceLocation("minecraft", "light_blue_carpet"),
        ResourceLocation("minecraft", "yellow_carpet"),
        ResourceLocation("minecraft", "lime_carpet"),
        ResourceLocation("minecraft", "pink_carpet"),
        ResourceLocation("minecraft", "gray_carpet"),
        ResourceLocation("minecraft", "light_gray_carpet"),
        ResourceLocation("minecraft", "cyan_carpet"),
        ResourceLocation("minecraft", "purple_carpet"),
        ResourceLocation("minecraft", "blue_carpet"),
        ResourceLocation("minecraft", "brown_carpet"),
        ResourceLocation("minecraft", "green_carpet"),
        ResourceLocation("minecraft", "red_carpet"),
        ResourceLocation("minecraft", "black_carpet")});
    tags[woolCarpets->getId()] = std::move(woolCarpets);

    // 创建 BEDS 标签（所有颜色的床方块）
    auto beds = std::make_unique<BlockTag>(ResourceLocation("minecraft", "beds"));
    beds->addAll({ResourceLocation("minecraft", "white_bed"),
        ResourceLocation("minecraft", "orange_bed"),
        ResourceLocation("minecraft", "magenta_bed"),
        ResourceLocation("minecraft", "light_blue_bed"),
        ResourceLocation("minecraft", "yellow_bed"),
        ResourceLocation("minecraft", "lime_bed"),
        ResourceLocation("minecraft", "pink_bed"),
        ResourceLocation("minecraft", "gray_bed"),
        ResourceLocation("minecraft", "light_gray_bed"),
        ResourceLocation("minecraft", "cyan_bed"),
        ResourceLocation("minecraft", "purple_bed"),
        ResourceLocation("minecraft", "blue_bed"),
        ResourceLocation("minecraft", "brown_bed"),
        ResourceLocation("minecraft", "green_bed"),
        ResourceLocation("minecraft", "red_bed"),
        ResourceLocation("minecraft", "black_bed")});
    tags[beds->getId()] = std::move(beds);

    // 创建 WOODEN_FENCES 标签（所有木质栅栏，不含下界砖栅栏）
    auto woodenFences = std::make_unique<BlockTag>(ResourceLocation("minecraft", "wooden_fences"));
    woodenFences->addAll({ResourceLocation("minecraft", "oak_fence"),
        ResourceLocation("minecraft", "spruce_fence"),
        ResourceLocation("minecraft", "birch_fence"),
        ResourceLocation("minecraft", "jungle_fence"),
        ResourceLocation("minecraft", "acacia_fence"),
        ResourceLocation("minecraft", "dark_oak_fence"),
        ResourceLocation("minecraft", "mangrove_fence"),
        ResourceLocation("minecraft", "cherry_fence"),
        ResourceLocation("minecraft", "bamboo_fence"),
        ResourceLocation("minecraft", "pale_oak_fence"),
        ResourceLocation("minecraft", "crimson_fence"),
        ResourceLocation("minecraft", "warped_fence")});
    tags[woodenFences->getId()] = std::move(woodenFences);

    // 创建 FENCES 标签（所有木质栅栏 + 下界砖栅栏）
    auto fences = std::make_unique<BlockTag>(ResourceLocation("minecraft", "fences"));
    fences->addAll({ResourceLocation("minecraft", "oak_fence"),
        ResourceLocation("minecraft", "spruce_fence"),
        ResourceLocation("minecraft", "birch_fence"),
        ResourceLocation("minecraft", "jungle_fence"),
        ResourceLocation("minecraft", "acacia_fence"),
        ResourceLocation("minecraft", "dark_oak_fence"),
        ResourceLocation("minecraft", "mangrove_fence"),
        ResourceLocation("minecraft", "cherry_fence"),
        ResourceLocation("minecraft", "bamboo_fence"),
        ResourceLocation("minecraft", "pale_oak_fence"),
        ResourceLocation("minecraft", "crimson_fence"),
        ResourceLocation("minecraft", "warped_fence"),
        ResourceLocation("minecraft", "nether_brick_fence")});
    tags[fences->getId()] = std::move(fences);

    // 创建 FENCE_GATES 标签
    auto fenceGates = std::make_unique<BlockTag>(ResourceLocation("minecraft", "fence_gates"));
    fenceGates->addAll({ResourceLocation("minecraft", "oak_fence_gate"),
        ResourceLocation("minecraft", "spruce_fence_gate"),
        ResourceLocation("minecraft", "birch_fence_gate"),
        ResourceLocation("minecraft", "jungle_fence_gate"),
        ResourceLocation("minecraft", "acacia_fence_gate"),
        ResourceLocation("minecraft", "dark_oak_fence_gate"),
        ResourceLocation("minecraft", "mangrove_fence_gate"),
        ResourceLocation("minecraft", "cherry_fence_gate"),
        ResourceLocation("minecraft", "bamboo_fence_gate"),
        ResourceLocation("minecraft", "pale_oak_fence_gate"),
        ResourceLocation("minecraft", "crimson_fence_gate"),
        ResourceLocation("minecraft", "warped_fence_gate")});
    tags[fenceGates->getId()] = std::move(fenceGates);

    // 创建 UNSTABLE_BOTTOM_CENTER 标签
    // MC 1.21.11 数据包中本标签内容为 #minecraft:fence_gates
    // 由于项目当前未实现标签到标签的引用，这里直接内联栅栏门列表
    auto unstableBottomCenter = std::make_unique<BlockTag>(ResourceLocation("minecraft", "unstable_bottom_center"));
    unstableBottomCenter->addAll({ResourceLocation("minecraft", "oak_fence_gate"),
        ResourceLocation("minecraft", "spruce_fence_gate"),
        ResourceLocation("minecraft", "birch_fence_gate"),
        ResourceLocation("minecraft", "jungle_fence_gate"),
        ResourceLocation("minecraft", "acacia_fence_gate"),
        ResourceLocation("minecraft", "dark_oak_fence_gate"),
        ResourceLocation("minecraft", "mangrove_fence_gate"),
        ResourceLocation("minecraft", "cherry_fence_gate"),
        ResourceLocation("minecraft", "bamboo_fence_gate"),
        ResourceLocation("minecraft", "pale_oak_fence_gate"),
        ResourceLocation("minecraft", "crimson_fence_gate"),
        ResourceLocation("minecraft", "warped_fence_gate")});
    tags[unstableBottomCenter->getId()] = std::move(unstableBottomCenter);

    // 创建 WOODEN_SHELVES 标签
    auto woodenShelves = std::make_unique<BlockTag>(ResourceLocation("minecraft", "wooden_shelves"));
    woodenShelves->addAll({ResourceLocation("minecraft", "oak_shelf"),
        ResourceLocation("minecraft", "spruce_shelf"),
        ResourceLocation("minecraft", "birch_shelf"),
        ResourceLocation("minecraft", "jungle_shelf"),
        ResourceLocation("minecraft", "acacia_shelf"),
        ResourceLocation("minecraft", "dark_oak_shelf"),
        ResourceLocation("minecraft", "mangrove_shelf"),
        ResourceLocation("minecraft", "cherry_shelf"),
        ResourceLocation("minecraft", "pale_oak_shelf"),
        ResourceLocation("minecraft", "bamboo_shelf"),
        ResourceLocation("minecraft", "crimson_shelf"),
        ResourceLocation("minecraft", "warped_shelf")});
    tags[woodenShelves->getId()] = std::move(woodenShelves);

    // 创建 BAMBOO_PLANTABLE_ON 标签
    // 对齐 vanilla 1.21.11 data/minecraft/tags/block/bamboo_plantable_on.json：
    //   { "#minecraft:sand", "#minecraft:dirt", "minecraft:bamboo",
    //     "minecraft:bamboo_sapling", "minecraft:gravel", "minecraft:suspicious_gravel" }
    // Cubium 标签不支持子标签引用（#sand/#dirt），此处手动展开为完整成员集合：
    //   #dirt（10）+ #sand（3）+ bamboo + bamboo_sapling + gravel + suspicious_gravel = 17 项。
    // 注意：原实现误含 farmland（vanilla 无），已移除——竹子不应能种在耕地上。
    // wiki tech_竹子.txt#生长 列举的可种植方块：草方块/菌丝体/灰化土/泥土/缠根泥土/砂土/泥巴/沾泥的红树根/
    //   苔藓块/苍白苔藓块/沙砾/可疑的沙砾/沙子/红沙/可疑的沙子，外加竹子与竹笋自身。
    auto bambooPlantableOn = std::make_unique<BlockTag>(ResourceLocation("minecraft", "bamboo_plantable_on"));
    bambooPlantableOn->addAll({// #minecraft:dirt 展开（对齐 dirt.json 10 项）
        ResourceLocation("minecraft", "dirt"),
        ResourceLocation("minecraft", "grass_block"),
        ResourceLocation("minecraft", "podzol"),
        ResourceLocation("minecraft", "coarse_dirt"),
        ResourceLocation("minecraft", "mycelium"),
        ResourceLocation("minecraft", "rooted_dirt"),
        ResourceLocation("minecraft", "moss_block"),
        ResourceLocation("minecraft", "pale_moss_block"),
        ResourceLocation("minecraft", "mud"),
        ResourceLocation("minecraft", "muddy_mangrove_roots"),
        // #minecraft:sand 展开（对齐 sand.json 3 项）
        ResourceLocation("minecraft", "sand"),
        ResourceLocation("minecraft", "red_sand"),
        ResourceLocation("minecraft", "suspicious_sand"),
        // 显式成员
        ResourceLocation("minecraft", "bamboo"),
        ResourceLocation("minecraft", "bamboo_sapling"),
        ResourceLocation("minecraft", "gravel"),
        ResourceLocation("minecraft", "suspicious_gravel")});
    tags[bambooPlantableOn->getId()] = std::move(bambooPlantableOn);

    // 创建 VALID_SWEET_BERRY_BUSH_GROUND 标签
    auto sweetBerryBushGround =
        std::make_unique<BlockTag>(ResourceLocation("minecraft", "valid_sweet_berry_bush_ground"));
    sweetBerryBushGround->addAll({ResourceLocation("minecraft", "grass_block"),
        ResourceLocation("minecraft", "dirt"),
        ResourceLocation("minecraft", "coarse_dirt"),
        ResourceLocation("minecraft", "podzol"),
        ResourceLocation("minecraft", "farmland")});
    tags[sweetBerryBushGround->getId()] = std::move(sweetBerryBushGround);

    // 创建 MUSHROOM_GROW_BLOCK 标签（蘑菇可生长方块）
    // 蘑菇在这些方块上放置时不受光照限制
    auto mushroomGrowBlock = std::make_unique<BlockTag>(ResourceLocation("minecraft", "mushroom_grow_block"));
    mushroomGrowBlock->addAll({ResourceLocation("minecraft", "mycelium"),
        ResourceLocation("minecraft", "podzol"),
        ResourceLocation("minecraft", "crimson_nylium"),
        ResourceLocation("minecraft", "warped_nylium")});
    tags[mushroomGrowBlock->getId()] = std::move(mushroomGrowBlock);

    // 创建 WALL_CORALS 标签（墙珊瑚扇）
    // 包含所有活的和死的墙珊瑚扇
    auto wallCorals = std::make_unique<BlockTag>(ResourceLocation("minecraft", "wall_corals"));
    wallCorals->addAll({// 活的墙珊瑚扇
        ResourceLocation("minecraft", "tube_coral_wall_fan"),
        ResourceLocation("minecraft", "brain_coral_wall_fan"),
        ResourceLocation("minecraft", "bubble_coral_wall_fan"),
        ResourceLocation("minecraft", "fire_coral_wall_fan"),
        ResourceLocation("minecraft", "horn_coral_wall_fan"),
        // 死的墙珊瑚扇
        ResourceLocation("minecraft", "dead_tube_coral_wall_fan"),
        ResourceLocation("minecraft", "dead_brain_coral_wall_fan"),
        ResourceLocation("minecraft", "dead_bubble_coral_wall_fan"),
        ResourceLocation("minecraft", "dead_fire_coral_wall_fan"),
        ResourceLocation("minecraft", "dead_horn_coral_wall_fan")});
    tags[wallCorals->getId()] = std::move(wallCorals);

    // 创建 UNDERWATER_BONEMEALS 标签（水下骨粉可催熟方块）
    // 包含海草、海带、各种珊瑚扇（活的）
    auto underwaterBonemeals = std::make_unique<BlockTag>(ResourceLocation("minecraft", "underwater_bonemeals"));
    underwaterBonemeals->addAll({// 海草和海带
        ResourceLocation("minecraft", "seagrass"),
        ResourceLocation("minecraft", "kelp"),
        // 活的珊瑚扇（地面放置）
        ResourceLocation("minecraft", "tube_coral_fan"),
        ResourceLocation("minecraft", "brain_coral_fan"),
        ResourceLocation("minecraft", "bubble_coral_fan"),
        ResourceLocation("minecraft", "fire_coral_fan"),
        ResourceLocation("minecraft", "horn_coral_fan")});
    tags[underwaterBonemeals->getId()] = std::move(underwaterBonemeals);

    // 创建 STRIDER_WARM_BLOCKS 标签（炽足兽温暖方块）
    // 只包含熔岩方块
    auto striderWarmBlocks = std::make_unique<BlockTag>(ResourceLocation("minecraft", "strider_warm_blocks"));
    striderWarmBlocks->addAll({ResourceLocation("minecraft", "lava")});
    tags[striderWarmBlocks->getId()] = std::move(striderWarmBlocks);

    // 创建 HOGLIN_REPELLENTS 标签（疣猪兽排斥物）
    // MC 1.21.11: BlockTags.HOGLIN_REPELLENTS
    // 疣猪兽在这些方块附近会逃跑，getPathWeight 返回 -1.0
    // 包含: 诡异菌(warped_fungus)、盆栽诡异菌(potted_warped_fungus)、下界传送门(nether_portal)、重生锚(respawn_anchor)
    auto hoglinRepellents = std::make_unique<BlockTag>(ResourceLocation("minecraft", "hoglin_repellents"));
    hoglinRepellents->addAll({ResourceLocation("minecraft", "warped_fungus"),
        ResourceLocation("minecraft", "potted_warped_fungus"),
        ResourceLocation("minecraft", "nether_portal"),
        ResourceLocation("minecraft", "respawn_anchor")});
    tags[hoglinRepellents->getId()] = std::move(hoglinRepellents);

    // 创建 PIGLIN_REPELLENTS 标签（猪灵排斥物）
    // MC 1.21.11: BlockTags.PIGLIN_REPELLENTS
    // 猪灵在这些方块附近会逃跑
    // 包含: 灵魂火(soul_fire)、灵魂火把(soul_torch)、灵魂墙火把(soul_wall_torch)、
    //       灵魂灯笼(soul_lantern)、灵魂营火(soul_campfire，需点燃)
    // 注意: 灵魂营火的点燃状态检查已在 PiglinEntity::getPathWeight 和 AvoidBlockGoal 中实现
    //       （对应 MC 原版 PiglinSpecificSensor.isValidRepellent 逻辑）
    // 注意: MC 1.21.11 中 potted_warped_fungus 不在 PIGLIN_REPELLENTS 中，无需添加
    // 注意: MC 1.21.11 中 warped_fungus 不在 PIGLIN_REPELLENTS 中，仅存在于 HOGLIN_REPELLENTS
    auto piglinRepellents = std::make_unique<BlockTag>(ResourceLocation("minecraft", "piglin_repellents"));
    piglinRepellents->addAll({ResourceLocation("minecraft", "soul_fire"),
        ResourceLocation("minecraft", "soul_torch"),
        ResourceLocation("minecraft", "soul_wall_torch"),
        ResourceLocation("minecraft", "soul_lantern"),
        ResourceLocation("minecraft", "soul_campfire")});
    tags[piglinRepellents->getId()] = std::move(piglinRepellents);

    // 创建 SMALL_FLOWERS 标签（小花朵）
    auto smallFlowers = std::make_unique<BlockTag>(ResourceLocation("minecraft", "small_flowers"));
    smallFlowers->addAll({ResourceLocation("minecraft", "dandelion"),
        ResourceLocation("minecraft", "poppy"),
        ResourceLocation("minecraft", "blue_orchid"),
        ResourceLocation("minecraft", "allium"),
        ResourceLocation("minecraft", "azure_bluet"),
        ResourceLocation("minecraft", "red_tulip"),
        ResourceLocation("minecraft", "orange_tulip"),
        ResourceLocation("minecraft", "white_tulip"),
        ResourceLocation("minecraft", "pink_tulip"),
        ResourceLocation("minecraft", "oxeye_daisy"),
        ResourceLocation("minecraft", "cornflower"),
        ResourceLocation("minecraft", "lily_of_the_valley"),
        ResourceLocation("minecraft", "wither_rose")});
    tags[smallFlowers->getId()] = std::move(smallFlowers);

    // 创建 TALL_FLOWERS 标签（高花朵）
    // 注意: MC 1.21.2+ 已移除 tall_flowers 标签，高花朵直接包含在 flowers 标签中
    // 此处保留以兼容旧代码
    auto tallFlowers = std::make_unique<BlockTag>(ResourceLocation("minecraft", "tall_flowers"));
    tallFlowers->addAll({ResourceLocation("minecraft", "sunflower"),
        ResourceLocation("minecraft", "lilac"),
        ResourceLocation("minecraft", "rose_bush"),
        ResourceLocation("minecraft", "peony"),
        ResourceLocation("minecraft", "pitcher_plant")});
    tags[tallFlowers->getId()] = std::move(tallFlowers);

    // 创建 FLOWERS 标签（所有花朵）
    // MC 1.21.11: BlockTags.FLOWERS
    // 包含小花朵标签引用 + 高花朵 + 其他花类方块
    auto flowers = std::make_unique<BlockTag>(ResourceLocation("minecraft", "flowers"));
    flowers->addAll({// 小花朵（内联展开 #minecraft:small_flowers）
        ResourceLocation("minecraft", "dandelion"),
        ResourceLocation("minecraft", "poppy"),
        ResourceLocation("minecraft", "blue_orchid"),
        ResourceLocation("minecraft", "allium"),
        ResourceLocation("minecraft", "azure_bluet"),
        ResourceLocation("minecraft", "red_tulip"),
        ResourceLocation("minecraft", "orange_tulip"),
        ResourceLocation("minecraft", "white_tulip"),
        ResourceLocation("minecraft", "pink_tulip"),
        ResourceLocation("minecraft", "oxeye_daisy"),
        ResourceLocation("minecraft", "cornflower"),
        ResourceLocation("minecraft", "lily_of_the_valley"),
        ResourceLocation("minecraft", "wither_rose"),
        ResourceLocation("minecraft", "torchflower"),
        ResourceLocation("minecraft", "open_eyeblossom"),
        ResourceLocation("minecraft", "closed_eyeblossom"),
        ResourceLocation("minecraft", "cactus_flower"),
        ResourceLocation("minecraft", "wildflowers"),
        // 高花朵
        ResourceLocation("minecraft", "sunflower"),
        ResourceLocation("minecraft", "lilac"),
        ResourceLocation("minecraft", "peony"),
        ResourceLocation("minecraft", "rose_bush"),
        ResourceLocation("minecraft", "pitcher_plant"),
        // 其他花类方块
        ResourceLocation("minecraft", "flowering_azalea_leaves"),
        ResourceLocation("minecraft", "flowering_azalea"),
        ResourceLocation("minecraft", "mangrove_propagule"),
        ResourceLocation("minecraft", "cherry_leaves"),
        ResourceLocation("minecraft", "pink_petals"),
        ResourceLocation("minecraft", "chorus_flower"),
        ResourceLocation("minecraft", "spore_blossom")});
    tags[flowers->getId()] = std::move(flowers);

    // 创建 SAPLINGS 标签（所有树苗）
    // MC 1.21.11: BlockTags.SAPLINGS
    auto saplings = std::make_unique<BlockTag>(ResourceLocation("minecraft", "saplings"));
    saplings->addAll({ResourceLocation("minecraft", "oak_sapling"),
        ResourceLocation("minecraft", "spruce_sapling"),
        ResourceLocation("minecraft", "birch_sapling"),
        ResourceLocation("minecraft", "jungle_sapling"),
        ResourceLocation("minecraft", "acacia_sapling"),
        ResourceLocation("minecraft", "dark_oak_sapling"),
        ResourceLocation("minecraft", "pale_oak_sapling"),
        ResourceLocation("minecraft", "azalea"),
        ResourceLocation("minecraft", "flowering_azalea"),
        ResourceLocation("minecraft", "mangrove_propagule"),
        ResourceLocation("minecraft", "cherry_sapling")});
    tags[saplings->getId()] = std::move(saplings);

    // 创建 FLOWER_POTS 标签（花盆）
    // MC 1.21.11: BlockTags.FLOWER_POTS
    // 包含空花盆 + 所有 potted_* 盆栽方块
    auto flowerPots = std::make_unique<BlockTag>(ResourceLocation("minecraft", "flower_pots"));
    flowerPots->addAll({ResourceLocation("minecraft", "flower_pot"),
        // 树苗系列
        ResourceLocation("minecraft", "potted_oak_sapling"),
        ResourceLocation("minecraft", "potted_spruce_sapling"),
        ResourceLocation("minecraft", "potted_birch_sapling"),
        ResourceLocation("minecraft", "potted_jungle_sapling"),
        ResourceLocation("minecraft", "potted_acacia_sapling"),
        ResourceLocation("minecraft", "potted_dark_oak_sapling"),
        ResourceLocation("minecraft", "potted_cherry_sapling"),
        ResourceLocation("minecraft", "potted_pale_oak_sapling"),
        ResourceLocation("minecraft", "potted_mangrove_propagule"),
        // 花卉系列
        ResourceLocation("minecraft", "potted_dandelion"),
        ResourceLocation("minecraft", "potted_poppy"),
        ResourceLocation("minecraft", "potted_blue_orchid"),
        ResourceLocation("minecraft", "potted_allium"),
        ResourceLocation("minecraft", "potted_azure_bluet"),
        ResourceLocation("minecraft", "potted_red_tulip"),
        ResourceLocation("minecraft", "potted_orange_tulip"),
        ResourceLocation("minecraft", "potted_white_tulip"),
        ResourceLocation("minecraft", "potted_pink_tulip"),
        ResourceLocation("minecraft", "potted_oxeye_daisy"),
        ResourceLocation("minecraft", "potted_cornflower"),
        ResourceLocation("minecraft", "potted_lily_of_the_valley"),
        ResourceLocation("minecraft", "potted_wither_rose"),
        ResourceLocation("minecraft", "potted_torchflower"),
        ResourceLocation("minecraft", "potted_open_eyeblossom"),
        ResourceLocation("minecraft", "potted_closed_eyeblossom"),
        // 蕨/枯草
        ResourceLocation("minecraft", "potted_fern"),
        ResourceLocation("minecraft", "potted_dead_bush"),
        // 蘑菇
        ResourceLocation("minecraft", "potted_red_mushroom"),
        ResourceLocation("minecraft", "potted_brown_mushroom"),
        // 仙人掌/竹子
        ResourceLocation("minecraft", "potted_cactus"),
        ResourceLocation("minecraft", "potted_bamboo"),
        // 下界菌/菌索
        ResourceLocation("minecraft", "potted_crimson_fungus"),
        ResourceLocation("minecraft", "potted_warped_fungus"),
        ResourceLocation("minecraft", "potted_crimson_roots"),
        ResourceLocation("minecraft", "potted_warped_roots"),
        // 杜鹃花
        ResourceLocation("minecraft", "potted_azalea_bush"),
        ResourceLocation("minecraft", "potted_flowering_azalea_bush")});
    tags[flowerPots->getId()] = std::move(flowerPots);

    // 创建 BEEHIVES 标签（蜂巢/蜂箱）
    auto beehives = std::make_unique<BlockTag>(ResourceLocation("minecraft", "beehives"));
    beehives->addAll({ResourceLocation("minecraft", "beehive"), ResourceLocation("minecraft", "bee_nest")});
    tags[beehives->getId()] = std::move(beehives);

    // 创建 BEE_ATTRACTIVE 标签（蜜蜂吸引物）
    // MC 1.21.11: BlockTags.BEE_ATTRACTIVE
    // 蜜蜂被这些方块吸引（用于授粉目标判定、眼眸花中毒触发等）。
    // 含水的水合花朵与向日葵下半部分由 BeeEntity::attractsBees 工具函数特判排除。
    // 注意：闭合眼眸花 (closed_eyeblossom) 不在此标签中，与 MC 1.21.11 数据包一致。
    auto beeAttractive = std::make_unique<BlockTag>(ResourceLocation("minecraft", "bee_attractive"));
    beeAttractive->addAll({// 小花朵（与 BlockItemTagsProvider.ablock 一致）
        ResourceLocation("minecraft", "dandelion"),
        ResourceLocation("minecraft", "open_eyeblossom"),
        ResourceLocation("minecraft", "poppy"),
        ResourceLocation("minecraft", "blue_orchid"),
        ResourceLocation("minecraft", "allium"),
        ResourceLocation("minecraft", "azure_bluet"),
        ResourceLocation("minecraft", "red_tulip"),
        ResourceLocation("minecraft", "orange_tulip"),
        ResourceLocation("minecraft", "white_tulip"),
        ResourceLocation("minecraft", "pink_tulip"),
        ResourceLocation("minecraft", "oxeye_daisy"),
        ResourceLocation("minecraft", "cornflower"),
        ResourceLocation("minecraft", "lily_of_the_valley"),
        ResourceLocation("minecraft", "wither_rose"),
        ResourceLocation("minecraft", "torchflower"),
        // 高花朵与其他花类方块（与 BlockItemTagsProvider.ablock1 一致）
        ResourceLocation("minecraft", "sunflower"),
        ResourceLocation("minecraft", "lilac"),
        ResourceLocation("minecraft", "peony"),
        ResourceLocation("minecraft", "rose_bush"),
        ResourceLocation("minecraft", "pitcher_plant"),
        ResourceLocation("minecraft", "flowering_azalea_leaves"),
        ResourceLocation("minecraft", "flowering_azalea"),
        ResourceLocation("minecraft", "mangrove_propagule"),
        ResourceLocation("minecraft", "cherry_leaves"),
        ResourceLocation("minecraft", "pink_petals"),
        ResourceLocation("minecraft", "wildflowers"),
        ResourceLocation("minecraft", "chorus_flower"),
        ResourceLocation("minecraft", "spore_blossom"),
        ResourceLocation("minecraft", "cactus_flower")});
    tags[beeAttractive->getId()] = std::move(beeAttractive);

    // 创建 DOES_NOT_BLOCK_HOPPERS 标签（漏斗不阻挡方块）
    // MC 1.21.11: BlockTags.DOES_NOT_BLOCK_HOPPERS
    // 即使碰撞形状为完整方块，漏斗仍可从中吸取物品的方块。
    // MC Java 中此标签引用 BEEHIVES 标签，包含蜂巢(bee_nest)和蜂箱(beehive)。
    // 蜂巢/蜂箱碰撞形状为完整方块，但漏斗应能与之交互（吸取蜂蜜瓶/空瓶）。
    auto doesNotBlockHoppers = std::make_unique<BlockTag>(ResourceLocation("minecraft", "does_not_block_hoppers"));
    doesNotBlockHoppers->addAll({ResourceLocation("minecraft", "beehive"), ResourceLocation("minecraft", "bee_nest")});
    tags[doesNotBlockHoppers->getId()] = std::move(doesNotBlockHoppers);

    // 创建 BEE_GROWABLES 标签（蜜蜂可授粉作物）
    auto beeGrowables = std::make_unique<BlockTag>(ResourceLocation("minecraft", "bee_growables"));
    beeGrowables->addAll({// 农作物
        ResourceLocation("minecraft", "wheat"),
        ResourceLocation("minecraft", "carrots"),
        ResourceLocation("minecraft", "potatoes"),
        ResourceLocation("minecraft", "beetroots"),
        // 瓜果茎
        ResourceLocation("minecraft", "melon_stem"),
        ResourceLocation("minecraft", "pumpkin_stem"),
        // 甜浆果丛
        ResourceLocation("minecraft", "sweet_berry_bush"),
        // 洞穴藤蔓（发光浆果）— MC 原版 bee_growables 包含 cave_vines 和 cave_vines_plant
        ResourceLocation("minecraft", "cave_vines"),
        ResourceLocation("minecraft", "cave_vines_plant")});
    tags[beeGrowables->getId()] = std::move(beeGrowables);

    // 创建 ENDERMAN_HOLDABLE 标签（末影人可拾取方块）
    // 包含：泥土类、沙子类、蘑菇、花、仙人掌、南瓜/西瓜、TNT、下界方块
    auto endermanHoldable = std::make_unique<BlockTag>(ResourceLocation("minecraft", "enderman_holdable"));
    endermanHoldable->addAll({// 泥土类
        ResourceLocation("minecraft", "grass_block"),
        ResourceLocation("minecraft", "dirt"),
        ResourceLocation("minecraft", "coarse_dirt"),
        ResourceLocation("minecraft", "podzol"),
        // 沙子类
        ResourceLocation("minecraft", "sand"),
        ResourceLocation("minecraft", "red_sand"),
        // 沙砾
        ResourceLocation("minecraft", "gravel"),
        // 蘑菇
        ResourceLocation("minecraft", "brown_mushroom"),
        ResourceLocation("minecraft", "red_mushroom"),
        // TNT
        ResourceLocation("minecraft", "tnt"),
        // 仙人掌
        ResourceLocation("minecraft", "cactus"),
        // 黏土块
        ResourceLocation("minecraft", "clay"),
        // 南瓜和西瓜
        ResourceLocation("minecraft", "pumpkin"),
        ResourceLocation("minecraft", "carved_pumpkin"),
        ResourceLocation("minecraft", "melon"),
        // 菌丝体
        ResourceLocation("minecraft", "mycelium"),
        // 下界方块（1.16新增）
        ResourceLocation("minecraft", "crimson_fungus"),
        ResourceLocation("minecraft", "crimson_nylium"),
        ResourceLocation("minecraft", "crimson_roots"),
        ResourceLocation("minecraft", "warped_fungus"),
        ResourceLocation("minecraft", "warped_nylium"),
        ResourceLocation("minecraft", "warped_roots")});
    tags[endermanHoldable->getId()] = std::move(endermanHoldable);

    // 创建小花朵标签并添加到末影人可拾取
    // 小花朵通过 SMALL_FLOWERS 标签也被末影人可拾取
    // 将小花朵添加到 ENDERMAN_HOLDABLE
    BlockTag* endermanTag = tags.at(ResourceLocation("minecraft", "enderman_holdable")).get();
    if (endermanTag) {
        // 添加所有小花朵
        endermanTag->addAll({ResourceLocation("minecraft", "dandelion"),
            ResourceLocation("minecraft", "poppy"),
            ResourceLocation("minecraft", "blue_orchid"),
            ResourceLocation("minecraft", "allium"),
            ResourceLocation("minecraft", "azure_bluet"),
            ResourceLocation("minecraft", "red_tulip"),
            ResourceLocation("minecraft", "orange_tulip"),
            ResourceLocation("minecraft", "white_tulip"),
            ResourceLocation("minecraft", "pink_tulip"),
            ResourceLocation("minecraft", "oxeye_daisy"),
            ResourceLocation("minecraft", "cornflower"),
            ResourceLocation("minecraft", "lily_of_the_valley"),
            ResourceLocation("minecraft", "wither_rose")});
    }

    // 将所有原木类型添加到 LOGS 标签
    BlockTag& logsTag = *tags.at(ResourceLocation("minecraft", "logs"));
    logsTag.addAll({ResourceLocation("minecraft", "jungle_log"),
        ResourceLocation("minecraft", "jungle_wood"),
        ResourceLocation("minecraft", "stripped_jungle_log"),
        ResourceLocation("minecraft", "stripped_jungle_wood"),
        ResourceLocation("minecraft", "oak_log"),
        ResourceLocation("minecraft", "oak_wood"),
        ResourceLocation("minecraft", "stripped_oak_log"),
        ResourceLocation("minecraft", "stripped_oak_wood"),
        ResourceLocation("minecraft", "spruce_log"),
        ResourceLocation("minecraft", "spruce_wood"),
        ResourceLocation("minecraft", "stripped_spruce_log"),
        ResourceLocation("minecraft", "stripped_spruce_wood"),
        ResourceLocation("minecraft", "birch_log"),
        ResourceLocation("minecraft", "birch_wood"),
        ResourceLocation("minecraft", "stripped_birch_log"),
        ResourceLocation("minecraft", "stripped_birch_wood"),
        ResourceLocation("minecraft", "acacia_log"),
        ResourceLocation("minecraft", "acacia_wood"),
        ResourceLocation("minecraft", "stripped_acacia_log"),
        ResourceLocation("minecraft", "stripped_acacia_wood"),
        ResourceLocation("minecraft", "dark_oak_log"),
        ResourceLocation("minecraft", "dark_oak_wood"),
        ResourceLocation("minecraft", "stripped_dark_oak_log"),
        ResourceLocation("minecraft", "stripped_dark_oak_wood"),
        ResourceLocation("minecraft", "crimson_stem"),
        ResourceLocation("minecraft", "stripped_crimson_stem"),
        ResourceLocation("minecraft", "warped_stem"),
        ResourceLocation("minecraft", "stripped_warped_stem"),
        // 1.19 红树林原木
        ResourceLocation("minecraft", "mangrove_log"),
        ResourceLocation("minecraft", "mangrove_wood"),
        ResourceLocation("minecraft", "stripped_mangrove_log"),
        ResourceLocation("minecraft", "stripped_mangrove_wood"),
        // 1.20 樱花原木
        ResourceLocation("minecraft", "cherry_log"),
        ResourceLocation("minecraft", "cherry_wood"),
        ResourceLocation("minecraft", "stripped_cherry_log"),
        ResourceLocation("minecraft", "stripped_cherry_wood"),
        // 1.21.2 苍白橡木原木
        ResourceLocation("minecraft", "pale_oak_log"),
        ResourceLocation("minecraft", "pale_oak_wood"),
        ResourceLocation("minecraft", "stripped_pale_oak_log"),
        ResourceLocation("minecraft", "stripped_pale_oak_wood")});

    // 更新 LEAVES 标签，添加 1.17-1.21 新叶子
    BlockTag& leavesTag = *tags.at(ResourceLocation("minecraft", "leaves"));
    leavesTag.addAll({// 1.19 红树林树叶
        ResourceLocation("minecraft", "mangrove_leaves"),
        // 1.20 樱花树叶
        ResourceLocation("minecraft", "cherry_leaves"),
        // 1.21.2 苍白橡木树叶
        ResourceLocation("minecraft", "pale_oak_leaves")});

    // 更新 PLANKS 标签，添加 1.19-1.21 新木板
    BlockTag& planksTag = *tags.at(ResourceLocation("minecraft", "planks"));
    planksTag.addAll({// 1.19 红树林木板
        ResourceLocation("minecraft", "mangrove_planks"),
        // 1.20 樱花木板
        ResourceLocation("minecraft", "cherry_planks"),
        // 1.20 竹木板
        ResourceLocation("minecraft", "bamboo_planks"),
        // 1.21.2 苍白橡木木板
        ResourceLocation("minecraft", "pale_oak_planks")});

    // 更新 DIRT 标签，添加 1.17+ 新泥土类方块
    BlockTag& dirtTag = *tags.at(ResourceLocation("minecraft", "dirt"));
    dirtTag.addAll({// 1.17 缠根泥土
        ResourceLocation("minecraft", "rooted_dirt"),
        // 1.17 苔藓块
        ResourceLocation("minecraft", "moss_block"),
        // 1.19 泥巴
        ResourceLocation("minecraft", "mud"),
        // 1.19 泥泞的红树根
        ResourceLocation("minecraft", "muddy_mangrove_roots"),
        // 1.21.2 苍白苔藓块
        ResourceLocation("minecraft", "pale_moss_block")});

    // 更新 STONE 标签，添加深板岩和凝灰岩
    BlockTag& stoneTag = *tags.at(ResourceLocation("minecraft", "stone"));
    stoneTag.addAll({// 1.17 深板岩和凝灰岩
        ResourceLocation("minecraft", "deepslate"),
        ResourceLocation("minecraft", "tuff"),
        // 1.16 黑石变种
        ResourceLocation("minecraft", "blackstone"),
        ResourceLocation("minecraft", "polished_blackstone"),
        ResourceLocation("minecraft", "polished_blackstone_bricks"),
        ResourceLocation("minecraft", "chiseled_polished_blackstone"),
        ResourceLocation("minecraft", "cracked_polished_blackstone_bricks"),
        ResourceLocation("minecraft", "gilded_blackstone"),
        // 1.17 方解石
        ResourceLocation("minecraft", "calcite"),
        // 1.17 滴水石块
        ResourceLocation("minecraft", "dripstone_block"),
        // 1.17 安山岩/花岗岩/闪长岩变种
        ResourceLocation("minecraft", "granite"),
        ResourceLocation("minecraft", "diorite"),
        ResourceLocation("minecraft", "andesite")});

    // 更新 SAND 标签，添加细雪和红沙
    // (red_sand already included)

    // 更新 ENDERMAN_HOLDABLE 标签，添加 1.17+ 新方块
    BlockTag& endermanTag2 = *tags.at(ResourceLocation("minecraft", "enderman_holdable"));
    endermanTag2.addAll({// 1.17 深板岩和凝灰岩
        ResourceLocation("minecraft", "deepslate"),
        ResourceLocation("minecraft", "cobbled_deepslate"),
        ResourceLocation("minecraft", "tuff"),
        // 1.17 苔藓块和苔藓地毯
        ResourceLocation("minecraft", "moss_block"),
        ResourceLocation("minecraft", "moss_carpet"),
        // 1.17 方解石
        ResourceLocation("minecraft", "calcite"),
        // 1.19 泥巴和泥砖
        ResourceLocation("minecraft", "mud"),
        ResourceLocation("minecraft", "muddy_mangrove_roots"),
        // 1.17 杜鹃花丛
        ResourceLocation("minecraft", "azalea"),
        ResourceLocation("minecraft", "flowering_azalea"),
        // 1.17 点滴杜鹃
        ResourceLocation("minecraft", "small_dripleaf"),
        // 1.17 缠根泥土
        ResourceLocation("minecraft", "rooted_dirt"),
        // 1.17 浆果
        ResourceLocation("minecraft", "glow_lichen"),
        // 1.21.2 苍白苔藓
        ResourceLocation("minecraft", "pale_moss_block"),
        ResourceLocation("minecraft", "pale_moss_carpet"),
        // 1.21.4 仙人掌花
        ResourceLocation("minecraft", "cactus_flower")});

    // 更新 SMALL_FLOWERS 标签，添加 1.17+ 新花
    BlockTag& smallFlowersTag = *tags.at(ResourceLocation("minecraft", "small_flowers"));
    smallFlowersTag.addAll({// 1.20 火把花
        ResourceLocation("minecraft", "torchflower"),
        // 1.21.2 睁眼花和闭眼花
        ResourceLocation("minecraft", "open_eyeblossom"),
        ResourceLocation("minecraft", "closed_eyeblossom"),
        // 1.21.4 仙人掌花和野花
        ResourceLocation("minecraft", "cactus_flower"),
        ResourceLocation("minecraft", "wildflowers")});

    // 更新 BEE_GROWABLES 标签，添加 1.20 火把花
    BlockTag& beeGrowablesTag = *tags.at(ResourceLocation("minecraft", "bee_growables"));
    beeGrowablesTag.addAll(
        {ResourceLocation("minecraft", "torchflower_crop"), ResourceLocation("minecraft", "pitcher_crop")});

    // ============================================================================
    // 1.17 Caves & Cliffs - 新标签
    // ============================================================================

    // 铜矿石标签
    auto copperOres = std::make_unique<BlockTag>(ResourceLocation("minecraft", "copper_ores"));
    copperOres->addAll(
        {ResourceLocation("minecraft", "copper_ore"), ResourceLocation("minecraft", "deepslate_copper_ore")});
    tags[copperOres->getId()] = std::move(copperOres);

    // 深板岩矿石可替换方块
    auto deepslateOreReplaceables =
        std::make_unique<BlockTag>(ResourceLocation("minecraft", "deepslate_ore_replaceables"));
    deepslateOreReplaceables->addAll(
        {ResourceLocation("minecraft", "deepslate"), ResourceLocation("minecraft", "tuff")});
    tags[deepslateOreReplaceables->getId()] = std::move(deepslateOreReplaceables);

    // 主世界基础石头
    auto baseStoneOverworld = std::make_unique<BlockTag>(ResourceLocation("minecraft", "base_stone_overworld"));
    baseStoneOverworld->addAll({ResourceLocation("minecraft", "stone"),
        ResourceLocation("minecraft", "granite"),
        ResourceLocation("minecraft", "diorite"),
        ResourceLocation("minecraft", "andesite"),
        ResourceLocation("minecraft", "tuff"),
        ResourceLocation("minecraft", "deepslate")});
    tags[baseStoneOverworld->getId()] = std::move(baseStoneOverworld);

    // 可被滴水石块替换的方块（DripstoneUtils.placeDripstoneBlockIfPossible / isDripstoneBase 依赖）
    auto dripstoneReplaceable = std::make_unique<BlockTag>(ResourceLocation("minecraft", "dripstone_replaceable"));
    dripstoneReplaceable->addAll({ResourceLocation("minecraft", "dirt"),
        ResourceLocation("minecraft", "grass_block"),
        ResourceLocation("minecraft", "podzol"),
        ResourceLocation("minecraft", "coarse_dirt"),
        ResourceLocation("minecraft", "mycelium"),
        ResourceLocation("minecraft", "rooted_dirt"),
        ResourceLocation("minecraft", "moss_block"),
        ResourceLocation("minecraft", "pale_moss_block"),
        ResourceLocation("minecraft", "deepslate"),
        ResourceLocation("minecraft", "tuff"),
        ResourceLocation("minecraft", "stone"),
        ResourceLocation("minecraft", "granite"),
        ResourceLocation("minecraft", "diorite"),
        ResourceLocation("minecraft", "andesite"),
        ResourceLocation("minecraft", "terracotta"),
        ResourceLocation("minecraft", "sandstone"),
        ResourceLocation("minecraft", "red_sandstone"),
        ResourceLocation("minecraft", "calcite"),
        ResourceLocation("minecraft", "dripstone_block")});
    tags[dripstoneReplaceable->getId()] = std::move(dripstoneReplaceable);

    // 水晶声音方块
    auto crystalSoundBlocks = std::make_unique<BlockTag>(ResourceLocation("minecraft", "crystal_sound_blocks"));
    crystalSoundBlocks->addAll(
        {ResourceLocation("minecraft", "amethyst_block"), ResourceLocation("minecraft", "budding_amethyst")});
    tags[crystalSoundBlocks->getId()] = std::move(crystalSoundBlocks);

    // 组合脚步声方块（踩在上面时同时播放自身步声和下方方块沉闷步声）
    // 参考: net.minecraft.data.tags.VanillaBlockTagsProvider - COMBINATION_STEP_SOUND_BLOCKS
    // 包含: #minecraft:wool_carpets + moss_carpet + pale_moss_carpet + snow + nether_sprouts + warped_roots +
    // crimson_roots + resin_clump
    auto combinationStepSoundBlocks =
        std::make_unique<BlockTag>(ResourceLocation("minecraft", "combination_step_sound_blocks"));
    combinationStepSoundBlocks->addAll({// 羊毛地毯（16色）
        ResourceLocation("minecraft", "white_carpet"),
        ResourceLocation("minecraft", "orange_carpet"),
        ResourceLocation("minecraft", "magenta_carpet"),
        ResourceLocation("minecraft", "light_blue_carpet"),
        ResourceLocation("minecraft", "yellow_carpet"),
        ResourceLocation("minecraft", "lime_carpet"),
        ResourceLocation("minecraft", "pink_carpet"),
        ResourceLocation("minecraft", "gray_carpet"),
        ResourceLocation("minecraft", "light_gray_carpet"),
        ResourceLocation("minecraft", "cyan_carpet"),
        ResourceLocation("minecraft", "purple_carpet"),
        ResourceLocation("minecraft", "blue_carpet"),
        ResourceLocation("minecraft", "brown_carpet"),
        ResourceLocation("minecraft", "green_carpet"),
        ResourceLocation("minecraft", "red_carpet"),
        ResourceLocation("minecraft", "black_carpet"),
        // 苔藓地毯
        ResourceLocation("minecraft", "moss_carpet"),
        // 苍白苔藓地毯
        ResourceLocation("minecraft", "pale_moss_carpet"),
        // 雪层（注意：不是雪块 snow_block）
        ResourceLocation("minecraft", "snow"),
        // 下界苗
        ResourceLocation("minecraft", "nether_sprouts"),
        // 诡异菌索
        ResourceLocation("minecraft", "warped_roots"),
        // 绯红菌索
        ResourceLocation("minecraft", "crimson_roots"),
        // 树脂团
        ResourceLocation("minecraft", "resin_clump")});
    tags[combinationStepSoundBlocks->getId()] = std::move(combinationStepSoundBlocks);

    // 内部脚步声方块（踩在上面时只播放自身步声，替代脚下方块的步声）
    // 参考: net.minecraft.data.tags.VanillaBlockTagsProvider - INSIDE_STEP_SOUND_BLOCKS
    auto insideStepSoundBlocks = std::make_unique<BlockTag>(ResourceLocation("minecraft", "inside_step_sound_blocks"));
    insideStepSoundBlocks->addAll({// 细雪
        ResourceLocation("minecraft", "powder_snow"),
        // 幽匿脉络
        ResourceLocation("minecraft", "sculk_vein"),
        // 发光地衣
        ResourceLocation("minecraft", "glow_lichen"),
        // 睡莲
        ResourceLocation("minecraft", "lily_pad"),
        // 小型紫水晶芽
        ResourceLocation("minecraft", "small_amethyst_bud"),
        // 粉红色花瓣
        ResourceLocation("minecraft", "pink_petals"),
        // 野花
        ResourceLocation("minecraft", "wildflowers"),
        // 落叶层
        ResourceLocation("minecraft", "leaf_litter")});
    tags[insideStepSoundBlocks->getId()] = std::move(insideStepSoundBlocks);

    // 洞穴藤蔓
    auto caveVines = std::make_unique<BlockTag>(ResourceLocation("minecraft", "cave_vines"));
    caveVines->addAll({ResourceLocation("minecraft", "cave_vines"), ResourceLocation("minecraft", "cave_vines_plant")});
    tags[caveVines->getId()] = std::move(caveVines);

    // 苔藓可替换方块
    auto mossReplaceable = std::make_unique<BlockTag>(ResourceLocation("minecraft", "moss_replaceable"));
    mossReplaceable->addAll({// 基础石头类
        ResourceLocation("minecraft", "stone"),
        ResourceLocation("minecraft", "granite"),
        ResourceLocation("minecraft", "diorite"),
        ResourceLocation("minecraft", "andesite"),
        ResourceLocation("minecraft", "tuff"),
        ResourceLocation("minecraft", "deepslate"),
        // 洞穴藤蔓
        ResourceLocation("minecraft", "cave_vines"),
        ResourceLocation("minecraft", "cave_vines_plant"),
        // 泥土类
        ResourceLocation("minecraft", "dirt"),
        ResourceLocation("minecraft", "grass_block"),
        ResourceLocation("minecraft", "podzol"),
        ResourceLocation("minecraft", "coarse_dirt"),
        ResourceLocation("minecraft", "mycelium"),
        ResourceLocation("minecraft", "farmland")});
    tags[mossReplaceable->getId()] = std::move(mossReplaceable);

    // 繁茂洞穴地面可替换方块（黏土特征替换）
    // 参考: net.minecraft.tags.BlockTags.LUSH_GROUND_REPLACEABLE
    auto lushGroundReplaceable = std::make_unique<BlockTag>(ResourceLocation("minecraft", "lush_ground_replaceable"));
    lushGroundReplaceable->addAll({// 基础石头类
        ResourceLocation("minecraft", "stone"),
        ResourceLocation("minecraft", "granite"),
        ResourceLocation("minecraft", "diorite"),
        ResourceLocation("minecraft", "andesite"),
        ResourceLocation("minecraft", "tuff"),
        ResourceLocation("minecraft", "deepslate"),
        // 泥土类
        ResourceLocation("minecraft", "dirt"),
        ResourceLocation("minecraft", "grass_block"),
        ResourceLocation("minecraft", "podzol"),
        ResourceLocation("minecraft", "coarse_dirt"),
        ResourceLocation("minecraft", "mycelium"),
        ResourceLocation("minecraft", "farmland"),
        // 沙砾和沙子
        ResourceLocation("minecraft", "gravel"),
        ResourceLocation("minecraft", "sand"),
        // 黏土
        ResourceLocation("minecraft", "clay"),
        // 苔藓
        ResourceLocation("minecraft", "moss_block")});
    tags[lushGroundReplaceable->getId()] = std::move(lushGroundReplaceable);

    // 杜鹃根系可替换方块
    // 参考: net.minecraft.tags.BlockTags.AZALEA_ROOT_REPLACEABLE
    auto azaleaRootReplaceable = std::make_unique<BlockTag>(ResourceLocation("minecraft", "azalea_root_replaceable"));
    azaleaRootReplaceable->addAll({// 基础石头类
        ResourceLocation("minecraft", "stone"),
        ResourceLocation("minecraft", "granite"),
        ResourceLocation("minecraft", "diorite"),
        ResourceLocation("minecraft", "andesite"),
        ResourceLocation("minecraft", "tuff"),
        ResourceLocation("minecraft", "deepslate"),
        // 泥土类
        ResourceLocation("minecraft", "dirt"),
        ResourceLocation("minecraft", "grass_block"),
        ResourceLocation("minecraft", "podzol"),
        ResourceLocation("minecraft", "coarse_dirt"),
        ResourceLocation("minecraft", "mycelium"),
        // 沙砾和沙子
        ResourceLocation("minecraft", "gravel"),
        ResourceLocation("minecraft", "sand"),
        // 黏土
        ResourceLocation("minecraft", "clay"),
        // 苔藓
        ResourceLocation("minecraft", "moss_block"),
        // 基岩（不能替换）
        // 矿石类
        ResourceLocation("minecraft", "coal_ore"),
        ResourceLocation("minecraft", "iron_ore"),
        ResourceLocation("minecraft", "gold_ore"),
        ResourceLocation("minecraft", "diamond_ore"),
        ResourceLocation("minecraft", "lapis_ore"),
        ResourceLocation("minecraft", "redstone_ore"),
        ResourceLocation("minecraft", "copper_ore"),
        ResourceLocation("minecraft", "emerald_ore"),
        // 深板岩矿石
        ResourceLocation("minecraft", "deepslate_coal_ore"),
        ResourceLocation("minecraft", "deepslate_iron_ore"),
        ResourceLocation("minecraft", "deepslate_gold_ore"),
        ResourceLocation("minecraft", "deepslate_diamond_ore"),
        ResourceLocation("minecraft", "deepslate_lapis_ore"),
        ResourceLocation("minecraft", "deepslate_redstone_ore"),
        ResourceLocation("minecraft", "deepslate_copper_ore"),
        ResourceLocation("minecraft", "deepslate_emerald_ore")});
    tags[azaleaRootReplaceable->getId()] = std::move(azaleaRootReplaceable);

    // 铜块标签（所有铜质方块）
    auto copper = std::make_unique<BlockTag>(ResourceLocation("minecraft", "copper"));
    copper->addAll({ResourceLocation("minecraft", "copper_block"),
        ResourceLocation("minecraft", "exposed_copper"),
        ResourceLocation("minecraft", "weathered_copper"),
        ResourceLocation("minecraft", "oxidized_copper"),
        ResourceLocation("minecraft", "waxed_copper_block"),
        ResourceLocation("minecraft", "waxed_exposed_copper"),
        ResourceLocation("minecraft", "waxed_weathered_copper"),
        ResourceLocation("minecraft", "waxed_oxidized_copper"),
        ResourceLocation("minecraft", "cut_copper"),
        ResourceLocation("minecraft", "exposed_cut_copper"),
        ResourceLocation("minecraft", "weathered_cut_copper"),
        ResourceLocation("minecraft", "oxidized_cut_copper"),
        ResourceLocation("minecraft", "waxed_cut_copper"),
        ResourceLocation("minecraft", "waxed_exposed_cut_copper"),
        ResourceLocation("minecraft", "waxed_weathered_cut_copper"),
        ResourceLocation("minecraft", "waxed_oxidized_cut_copper"),
        ResourceLocation("minecraft", "chiseled_copper"),
        ResourceLocation("minecraft", "waxed_chiseled_copper"),
        ResourceLocation("minecraft", "copper_grate"),
        ResourceLocation("minecraft", "exposed_copper_grate"),
        ResourceLocation("minecraft", "weathered_copper_grate"),
        ResourceLocation("minecraft", "oxidized_copper_grate"),
        ResourceLocation("minecraft", "waxed_copper_grate"),
        ResourceLocation("minecraft", "waxed_exposed_copper_grate"),
        ResourceLocation("minecraft", "waxed_weathered_copper_grate"),
        ResourceLocation("minecraft", "waxed_oxidized_copper_grate"),
        ResourceLocation("minecraft", "copper_bulb"),
        ResourceLocation("minecraft", "exposed_copper_bulb"),
        ResourceLocation("minecraft", "weathered_copper_bulb"),
        ResourceLocation("minecraft", "oxidized_copper_bulb"),
        ResourceLocation("minecraft", "waxed_copper_bulb"),
        ResourceLocation("minecraft", "waxed_exposed_copper_bulb"),
        ResourceLocation("minecraft", "waxed_weathered_copper_bulb"),
        ResourceLocation("minecraft", "waxed_oxidized_copper_bulb"),
        ResourceLocation("minecraft", "copper_door"),
        ResourceLocation("minecraft", "exposed_copper_door"),
        ResourceLocation("minecraft", "weathered_copper_door"),
        ResourceLocation("minecraft", "oxidized_copper_door"),
        ResourceLocation("minecraft", "waxed_copper_door"),
        ResourceLocation("minecraft", "waxed_exposed_copper_door"),
        ResourceLocation("minecraft", "waxed_weathered_copper_door"),
        ResourceLocation("minecraft", "waxed_oxidized_copper_door"),
        ResourceLocation("minecraft", "copper_trapdoor"),
        ResourceLocation("minecraft", "exposed_copper_trapdoor"),
        ResourceLocation("minecraft", "weathered_copper_trapdoor"),
        ResourceLocation("minecraft", "oxidized_copper_trapdoor"),
        ResourceLocation("minecraft", "waxed_copper_trapdoor"),
        ResourceLocation("minecraft", "waxed_exposed_copper_trapdoor"),
        ResourceLocation("minecraft", "waxed_weathered_copper_trapdoor"),
        ResourceLocation("minecraft", "waxed_oxidized_copper_trapdoor"),
        // 铜傀儡雕像（8 个变体）：MC 1.21.11 中通过 #copper_golem_statues 标签加入 #copper
        ResourceLocation("minecraft", "copper_golem_statue"),
        ResourceLocation("minecraft", "exposed_copper_golem_statue"),
        ResourceLocation("minecraft", "weathered_copper_golem_statue"),
        ResourceLocation("minecraft", "oxidized_copper_golem_statue"),
        ResourceLocation("minecraft", "waxed_copper_golem_statue"),
        ResourceLocation("minecraft", "waxed_exposed_copper_golem_statue"),
        ResourceLocation("minecraft", "waxed_weathered_copper_golem_statue"),
        ResourceLocation("minecraft", "waxed_oxidized_copper_golem_statue"),
        // 铜箱子（8 个变体）：MC 1.21.11 中通过 #copper_chests 标签加入 #copper
        ResourceLocation("minecraft", "copper_chest"),
        ResourceLocation("minecraft", "exposed_copper_chest"),
        ResourceLocation("minecraft", "weathered_copper_chest"),
        ResourceLocation("minecraft", "oxidized_copper_chest"),
        ResourceLocation("minecraft", "waxed_copper_chest"),
        ResourceLocation("minecraft", "waxed_exposed_copper_chest"),
        ResourceLocation("minecraft", "waxed_weathered_copper_chest"),
        ResourceLocation("minecraft", "waxed_oxidized_copper_chest")});
    tags[copper->getId()] = std::move(copper);

    // 铜傀儡雕像标签（8 个变体）
    // 参考: net.minecraft.tags.BlockTags.COPPER_GOLEM_STATUES (MC 1.21.11)
    // 用于 CopperGolemStatueBlock.shouldChangedStateKeepBlockEntity 判断：
    // 当斧头刮削/去蜡导致方块状态变化时，保留方块实体（CUSTOM_NAME 等数据不丢失）
    auto copperGolemStatues = std::make_unique<BlockTag>(ResourceLocation("minecraft", "copper_golem_statues"));
    copperGolemStatues->addAll({ResourceLocation("minecraft", "copper_golem_statue"),
        ResourceLocation("minecraft", "exposed_copper_golem_statue"),
        ResourceLocation("minecraft", "weathered_copper_golem_statue"),
        ResourceLocation("minecraft", "oxidized_copper_golem_statue"),
        ResourceLocation("minecraft", "waxed_copper_golem_statue"),
        ResourceLocation("minecraft", "waxed_exposed_copper_golem_statue"),
        ResourceLocation("minecraft", "waxed_weathered_copper_golem_statue"),
        ResourceLocation("minecraft", "waxed_oxidized_copper_golem_statue")});
    tags[copperGolemStatues->getId()] = std::move(copperGolemStatues);

    // 铜箱子标签（8 个变体）
    // 参考: net.minecraft.tags.BlockTags.COPPER_CHESTS (MC 1.21.11)
    // 用于 CopperChestBlock.chestCanConnectTo 判断：双箱合并允许跨氧化等级与涂蜡状态连接
    // 以及 shouldChangedStateKeepBlockEntity 判断：斧头刮削/去蜡时保留方块实体（物品不丢失）
    auto copperChests = std::make_unique<BlockTag>(ResourceLocation("minecraft", "copper_chests"));
    copperChests->addAll({ResourceLocation("minecraft", "copper_chest"),
        ResourceLocation("minecraft", "exposed_copper_chest"),
        ResourceLocation("minecraft", "weathered_copper_chest"),
        ResourceLocation("minecraft", "oxidized_copper_chest"),
        ResourceLocation("minecraft", "waxed_copper_chest"),
        ResourceLocation("minecraft", "waxed_exposed_copper_chest"),
        ResourceLocation("minecraft", "waxed_weathered_copper_chest"),
        ResourceLocation("minecraft", "waxed_oxidized_copper_chest")});
    tags[copperChests->getId()] = std::move(copperChests);

    // 避雷针标签（包含所有氧化和涂蜡变种）
    auto lightningRods = std::make_unique<BlockTag>(ResourceLocation("minecraft", "lightning_rods"));
    lightningRods->addAll({ResourceLocation("minecraft", "lightning_rod"),
        ResourceLocation("minecraft", "exposed_lightning_rod"),
        ResourceLocation("minecraft", "weathered_lightning_rod"),
        ResourceLocation("minecraft", "oxidized_lightning_rod"),
        ResourceLocation("minecraft", "waxed_lightning_rod"),
        ResourceLocation("minecraft", "waxed_exposed_lightning_rod"),
        ResourceLocation("minecraft", "waxed_weathered_lightning_rod"),
        ResourceLocation("minecraft", "waxed_oxidized_lightning_rod")});
    tags[lightningRods->getId()] = std::move(lightningRods);

    // 减振方块标签（羊毛和地毯）
    // 参考: net.minecraft.data.tags.BlockItemTagsProvider - DAMPENS_VIBRATIONS 包含 wool 和 wool_carpets
    auto dampensVibrations = std::make_unique<BlockTag>(ResourceLocation("minecraft", "dampens_vibrations"));
    dampensVibrations->addAll({// 所有羊毛颜色
        ResourceLocation("minecraft", "white_wool"),
        ResourceLocation("minecraft", "orange_wool"),
        ResourceLocation("minecraft", "magenta_wool"),
        ResourceLocation("minecraft", "light_blue_wool"),
        ResourceLocation("minecraft", "yellow_wool"),
        ResourceLocation("minecraft", "lime_wool"),
        ResourceLocation("minecraft", "pink_wool"),
        ResourceLocation("minecraft", "gray_wool"),
        ResourceLocation("minecraft", "light_gray_wool"),
        ResourceLocation("minecraft", "cyan_wool"),
        ResourceLocation("minecraft", "purple_wool"),
        ResourceLocation("minecraft", "blue_wool"),
        ResourceLocation("minecraft", "brown_wool"),
        ResourceLocation("minecraft", "green_wool"),
        ResourceLocation("minecraft", "red_wool"),
        ResourceLocation("minecraft", "black_wool"),
        // 所有地毯颜色
        ResourceLocation("minecraft", "white_carpet"),
        ResourceLocation("minecraft", "orange_carpet"),
        ResourceLocation("minecraft", "magenta_carpet"),
        ResourceLocation("minecraft", "light_blue_carpet"),
        ResourceLocation("minecraft", "yellow_carpet"),
        ResourceLocation("minecraft", "lime_carpet"),
        ResourceLocation("minecraft", "pink_carpet"),
        ResourceLocation("minecraft", "gray_carpet"),
        ResourceLocation("minecraft", "light_gray_carpet"),
        ResourceLocation("minecraft", "cyan_carpet"),
        ResourceLocation("minecraft", "purple_carpet"),
        ResourceLocation("minecraft", "blue_carpet"),
        ResourceLocation("minecraft", "brown_carpet"),
        ResourceLocation("minecraft", "green_carpet"),
        ResourceLocation("minecraft", "red_carpet"),
        ResourceLocation("minecraft", "black_carpet")});
    tags[dampensVibrations->getId()] = std::move(dampensVibrations);

    // 遮挡振动信号方块标签
    auto occludesVibrationSignals =
        std::make_unique<BlockTag>(ResourceLocation("minecraft", "occludes_vibration_signals"));
    occludesVibrationSignals->addAll({// 所有羊毛颜色
        ResourceLocation("minecraft", "white_wool"),
        ResourceLocation("minecraft", "orange_wool"),
        ResourceLocation("minecraft", "magenta_wool"),
        ResourceLocation("minecraft", "light_blue_wool"),
        ResourceLocation("minecraft", "yellow_wool"),
        ResourceLocation("minecraft", "lime_wool"),
        ResourceLocation("minecraft", "pink_wool"),
        ResourceLocation("minecraft", "gray_wool"),
        ResourceLocation("minecraft", "light_gray_wool"),
        ResourceLocation("minecraft", "cyan_wool"),
        ResourceLocation("minecraft", "purple_wool"),
        ResourceLocation("minecraft", "blue_wool"),
        ResourceLocation("minecraft", "brown_wool"),
        ResourceLocation("minecraft", "green_wool"),
        ResourceLocation("minecraft", "red_wool"),
        ResourceLocation("minecraft", "black_wool")});
    tags[occludesVibrationSignals->getId()] = std::move(occludesVibrationSignals);

    // 主世界自然原木标签
    auto overworldNaturalLogs = std::make_unique<BlockTag>(ResourceLocation("minecraft", "overworld_natural_logs"));
    overworldNaturalLogs->addAll({ResourceLocation("minecraft", "oak_log"),
        ResourceLocation("minecraft", "spruce_log"),
        ResourceLocation("minecraft", "birch_log"),
        ResourceLocation("minecraft", "jungle_log"),
        ResourceLocation("minecraft", "acacia_log"),
        ResourceLocation("minecraft", "dark_oak_log"),
        ResourceLocation("minecraft", "mangrove_log"),
        ResourceLocation("minecraft", "cherry_log"),
        ResourceLocation("minecraft", "pale_oak_log")});
    tags[overworldNaturalLogs->getId()] = std::move(overworldNaturalLogs);

    // 雪标签
    auto snow = std::make_unique<BlockTag>(ResourceLocation("minecraft", "snow"));
    snow->addAll({ResourceLocation("minecraft", "snow"),
        ResourceLocation("minecraft", "snow_block"),
        ResourceLocation("minecraft", "powder_snow")});
    tags[snow->getId()] = std::move(snow);

    // 细雪可放置标签（皮革靴子可行走的方块）
    auto powderSnowWalkableMoved =
        std::make_unique<BlockTag>(ResourceLocation("minecraft", "powder_snow_walkable_moved"));
    powderSnowWalkableMoved->addAll({ResourceLocation("minecraft", "powder_snow")});
    tags[powderSnowWalkableMoved->getId()] = std::move(powderSnowWalkableMoved);

    // ============================================================================
    // 1.19 Wild Update - 新标签
    // ============================================================================

    // 幽匿可替换方块
    auto sculkReplaceable = std::make_unique<BlockTag>(ResourceLocation("minecraft", "sculk_replaceable"));
    sculkReplaceable->addAll({// 基础石头
        ResourceLocation("minecraft", "stone"),
        ResourceLocation("minecraft", "granite"),
        ResourceLocation("minecraft", "diorite"),
        ResourceLocation("minecraft", "andesite"),
        ResourceLocation("minecraft", "tuff"),
        ResourceLocation("minecraft", "deepslate"),
        // 泥土类
        ResourceLocation("minecraft", "dirt"),
        ResourceLocation("minecraft", "grass_block"),
        ResourceLocation("minecraft", "podzol"),
        ResourceLocation("minecraft", "coarse_dirt"),
        ResourceLocation("minecraft", "mycelium"),
        ResourceLocation("minecraft", "farmland"),
        // 陶瓦
        ResourceLocation("minecraft", "terracotta"),
        // 下界
        ResourceLocation("minecraft", "crimson_nylium"),
        ResourceLocation("minecraft", "warped_nylium"),
        ResourceLocation("minecraft", "netherrack"),
        ResourceLocation("minecraft", "basalt"),
        ResourceLocation("minecraft", "blackstone"),
        // 沙子和沙砾
        ResourceLocation("minecraft", "sand"),
        ResourceLocation("minecraft", "red_sand"),
        ResourceLocation("minecraft", "gravel"),
        ResourceLocation("minecraft", "soul_sand"),
        ResourceLocation("minecraft", "soul_soil"),
        // 其他
        ResourceLocation("minecraft", "calcite"),
        ResourceLocation("minecraft", "smooth_basalt"),
        ResourceLocation("minecraft", "clay"),
        ResourceLocation("minecraft", "dripstone_block"),
        ResourceLocation("minecraft", "end_stone"),
        ResourceLocation("minecraft", "red_sandstone"),
        ResourceLocation("minecraft", "sandstone")});
    tags[sculkReplaceable->getId()] = std::move(sculkReplaceable);

    // 幽匿世界生成可替换方块
    auto sculkReplaceableWorldGen =
        std::make_unique<BlockTag>(ResourceLocation("minecraft", "sculk_replaceable_world_gen"));
    sculkReplaceableWorldGen->addAll({// 包含 sculk_replaceable 中的所有方块
        ResourceLocation("minecraft", "stone"),
        ResourceLocation("minecraft", "granite"),
        ResourceLocation("minecraft", "diorite"),
        ResourceLocation("minecraft", "andesite"),
        ResourceLocation("minecraft", "tuff"),
        ResourceLocation("minecraft", "deepslate"),
        ResourceLocation("minecraft", "dirt"),
        ResourceLocation("minecraft", "grass_block"),
        ResourceLocation("minecraft", "podzol"),
        ResourceLocation("minecraft", "coarse_dirt"),
        ResourceLocation("minecraft", "mycelium"),
        ResourceLocation("minecraft", "farmland"),
        ResourceLocation("minecraft", "terracotta"),
        ResourceLocation("minecraft", "crimson_nylium"),
        ResourceLocation("minecraft", "warped_nylium"),
        ResourceLocation("minecraft", "netherrack"),
        ResourceLocation("minecraft", "basalt"),
        ResourceLocation("minecraft", "blackstone"),
        ResourceLocation("minecraft", "sand"),
        ResourceLocation("minecraft", "red_sand"),
        ResourceLocation("minecraft", "gravel"),
        ResourceLocation("minecraft", "soul_sand"),
        ResourceLocation("minecraft", "soul_soil"),
        ResourceLocation("minecraft", "calcite"),
        ResourceLocation("minecraft", "smooth_basalt"),
        ResourceLocation("minecraft", "clay"),
        ResourceLocation("minecraft", "dripstone_block"),
        ResourceLocation("minecraft", "end_stone"),
        ResourceLocation("minecraft", "red_sandstone"),
        ResourceLocation("minecraft", "sandstone"),
        // 额外的深板岩变种
        ResourceLocation("minecraft", "deepslate_bricks"),
        ResourceLocation("minecraft", "deepslate_tiles"),
        ResourceLocation("minecraft", "cobbled_deepslate"),
        ResourceLocation("minecraft", "cracked_deepslate_bricks"),
        ResourceLocation("minecraft", "cracked_deepslate_tiles"),
        ResourceLocation("minecraft", "polished_deepslate")});
    tags[sculkReplaceableWorldGen->getId()] = std::move(sculkReplaceableWorldGen);

    // 远古城市可替换方块
    auto ancientCityReplaceable = std::make_unique<BlockTag>(ResourceLocation("minecraft", "ancient_city_replaceable"));
    ancientCityReplaceable->addAll({ResourceLocation("minecraft", "deepslate"),
        ResourceLocation("minecraft", "deepslate_bricks"),
        ResourceLocation("minecraft", "deepslate_tiles"),
        ResourceLocation("minecraft", "deepslate_brick_slab"),
        ResourceLocation("minecraft", "deepslate_tile_slab"),
        ResourceLocation("minecraft", "deepslate_brick_stairs"),
        ResourceLocation("minecraft", "deepslate_tile_wall"),
        ResourceLocation("minecraft", "deepslate_brick_wall"),
        ResourceLocation("minecraft", "cobbled_deepslate"),
        ResourceLocation("minecraft", "cracked_deepslate_bricks"),
        ResourceLocation("minecraft", "cracked_deepslate_tiles"),
        ResourceLocation("minecraft", "gray_wool")});
    tags[ancientCityReplaceable->getId()] = std::move(ancientCityReplaceable);

    // 振动共振方块标签
    auto vibrationResonators = std::make_unique<BlockTag>(ResourceLocation("minecraft", "vibration_resonators"));
    vibrationResonators->addAll({ResourceLocation("minecraft", "amethyst_block")});
    tags[vibrationResonators->getId()] = std::move(vibrationResonators);

    // 青蛙可生成标签
    auto frogsSpawnableOn = std::make_unique<BlockTag>(ResourceLocation("minecraft", "frogs_spawnable_on"));
    frogsSpawnableOn->addAll({ResourceLocation("minecraft", "grass_block"),
        ResourceLocation("minecraft", "mud"),
        ResourceLocation("minecraft", "mangrove_roots"),
        ResourceLocation("minecraft", "muddy_mangrove_roots")});
    tags[frogsSpawnableOn->getId()] = std::move(frogsSpawnableOn);

    // 可转化为泥巴方块标签
    auto convertableToMud = std::make_unique<BlockTag>(ResourceLocation("minecraft", "convertable_to_mud"));
    convertableToMud->addAll({ResourceLocation("minecraft", "dirt"),
        ResourceLocation("minecraft", "coarse_dirt"),
        ResourceLocation("minecraft", "rooted_dirt")});
    tags[convertableToMud->getId()] = std::move(convertableToMud);

    // 红树林原木可生长标签
    auto mangroveLogsCanGrowThrough =
        std::make_unique<BlockTag>(ResourceLocation("minecraft", "mangrove_logs_can_grow_through"));
    mangroveLogsCanGrowThrough->addAll({ResourceLocation("minecraft", "mud"),
        ResourceLocation("minecraft", "muddy_mangrove_roots"),
        ResourceLocation("minecraft", "mangrove_roots"),
        ResourceLocation("minecraft", "mangrove_leaves"),
        ResourceLocation("minecraft", "mangrove_log"),
        ResourceLocation("minecraft", "mangrove_propagule"),
        ResourceLocation("minecraft", "moss_carpet"),
        ResourceLocation("minecraft", "vine")});
    tags[mangroveLogsCanGrowThrough->getId()] = std::move(mangroveLogsCanGrowThrough);

    // 红树林根可生长标签
    auto mangroveRootsCanGrowThrough =
        std::make_unique<BlockTag>(ResourceLocation("minecraft", "mangrove_roots_can_grow_through"));
    mangroveRootsCanGrowThrough->addAll({ResourceLocation("minecraft", "mud"),
        ResourceLocation("minecraft", "muddy_mangrove_roots"),
        ResourceLocation("minecraft", "mangrove_roots"),
        ResourceLocation("minecraft", "moss_carpet"),
        ResourceLocation("minecraft", "vine"),
        ResourceLocation("minecraft", "mangrove_propagule"),
        ResourceLocation("minecraft", "snow")});
    tags[mangroveRootsCanGrowThrough->getId()] = std::move(mangroveRootsCanGrowThrough);

    // 红树林原木标签
    auto mangroveLogs = std::make_unique<BlockTag>(ResourceLocation("minecraft", "mangrove_logs"));
    mangroveLogs->addAll({ResourceLocation("minecraft", "mangrove_log"),
        ResourceLocation("minecraft", "mangrove_wood"),
        ResourceLocation("minecraft", "stripped_mangrove_log"),
        ResourceLocation("minecraft", "stripped_mangrove_wood")});
    tags[mangroveLogs->getId()] = std::move(mangroveLogs);

    // ============================================================================
    // 1.20 Trails & Tales - 新标签
    // ============================================================================

    // 樱花原木标签
    auto cherryLogs = std::make_unique<BlockTag>(ResourceLocation("minecraft", "cherry_logs"));
    cherryLogs->addAll({ResourceLocation("minecraft", "cherry_log"),
        ResourceLocation("minecraft", "cherry_wood"),
        ResourceLocation("minecraft", "stripped_cherry_log"),
        ResourceLocation("minecraft", "stripped_cherry_wood")});
    tags[cherryLogs->getId()] = std::move(cherryLogs);

    // 竹木方块标签
    auto bambooBlocks = std::make_unique<BlockTag>(ResourceLocation("minecraft", "bamboo_blocks"));
    bambooBlocks->addAll(
        {ResourceLocation("minecraft", "bamboo_block"), ResourceLocation("minecraft", "stripped_bamboo_block")});
    tags[bambooBlocks->getId()] = std::move(bambooBlocks);

    // 嗅探兽可挖掘方块标签
    auto snifferDiggableBlock = std::make_unique<BlockTag>(ResourceLocation("minecraft", "sniffer_diggable_block"));
    snifferDiggableBlock->addAll({ResourceLocation("minecraft", "dirt"),
        ResourceLocation("minecraft", "grass_block"),
        ResourceLocation("minecraft", "podzol"),
        ResourceLocation("minecraft", "coarse_dirt"),
        ResourceLocation("minecraft", "rooted_dirt"),
        ResourceLocation("minecraft", "moss_block"),
        ResourceLocation("minecraft", "pale_moss_block"),
        ResourceLocation("minecraft", "mud"),
        ResourceLocation("minecraft", "muddy_mangrove_roots")});
    tags[snifferDiggableBlock->getId()] = std::move(snifferDiggableBlock);

    // 嗅探兽蛋孵化加速方块标签（蛋下方为此标签方块时孵化时间 24000→12000 tick）
    auto snifferEggHatchBoost = std::make_unique<BlockTag>(ResourceLocation("minecraft", "sniffer_egg_hatch_boost"));
    snifferEggHatchBoost->addAll({ResourceLocation("minecraft", "moss_block")});
    tags[snifferEggHatchBoost->getId()] = std::move(snifferEggHatchBoost);

    // ============================================================================
    // 1.21 Tricky Trials - 新标签
    // ============================================================================

    // 不可被特性替换方块
    auto featuresCannotReplace = std::make_unique<BlockTag>(ResourceLocation("minecraft", "features_cannot_replace"));
    featuresCannotReplace->addAll({ResourceLocation("minecraft", "bedrock"),
        ResourceLocation("minecraft", "spawner"),
        ResourceLocation("minecraft", "chest"),
        ResourceLocation("minecraft", "end_portal_frame"),
        ResourceLocation("minecraft", "reinforced_deepslate"),
        ResourceLocation("minecraft", "trial_spawner"),
        ResourceLocation("minecraft", "vault")});
    tags[featuresCannotReplace->getId()] = std::move(featuresCannotReplace);

    // 晶洞无效方块标签（geode_invalid_blocks）：晶洞分布点命中这些方块计入 invalidCount。
    // 对齐数据包 data/minecraft/tags/blocks/geode_invalid_blocks.json。
    auto geodeInvalidBlocks = std::make_unique<BlockTag>(ResourceLocation("minecraft", "geode_invalid_blocks"));
    geodeInvalidBlocks->addAll({ResourceLocation("minecraft", "bedrock"),
        ResourceLocation("minecraft", "water"),
        ResourceLocation("minecraft", "lava"),
        ResourceLocation("minecraft", "ice"),
        ResourceLocation("minecraft", "packed_ice"),
        ResourceLocation("minecraft", "blue_ice")});
    tags[geodeInvalidBlocks->getId()] = std::move(geodeInvalidBlocks);

    // 附魔力量提供者标签
    auto enchantmentPowerProvider =
        std::make_unique<BlockTag>(ResourceLocation("minecraft", "enchantment_power_provider"));
    enchantmentPowerProvider->addAll({ResourceLocation("minecraft", "bookshelf")});
    tags[enchantmentPowerProvider->getId()] = std::move(enchantmentPowerProvider);

    // 附魔力量传输者标签（允许附魔力量穿过的方块）
    // MC中此标签包含#minecraft:replaceable（即canBeReplaced()==true的所有方块）
    // 此处仅注册标签以支持数据包兼容，实际附魔验证逻辑使用BlockState::canBeReplaced()
    auto enchantmentPowerTransmitter =
        std::make_unique<BlockTag>(ResourceLocation("minecraft", "enchantment_power_transmitter"));
    tags[enchantmentPowerTransmitter->getId()] = std::move(enchantmentPowerTransmitter);

    // 维持耕地标签
    auto maintainsFarmland = std::make_unique<BlockTag>(ResourceLocation("minecraft", "maintains_farmland"));
    maintainsFarmland->addAll({ResourceLocation("minecraft", "pumpkin_stem"),
        ResourceLocation("minecraft", "attached_pumpkin_stem"),
        ResourceLocation("minecraft", "melon_stem"),
        ResourceLocation("minecraft", "attached_melon_stem"),
        ResourceLocation("minecraft", "beetroots"),
        ResourceLocation("minecraft", "carrots"),
        ResourceLocation("minecraft", "potatoes"),
        ResourceLocation("minecraft", "torchflower_crop"),
        ResourceLocation("minecraft", "torchflower"),
        ResourceLocation("minecraft", "pitcher_crop"),
        ResourceLocation("minecraft", "wheat")});
    tags[maintainsFarmland->getId()] = std::move(maintainsFarmland);

    // ============================================================================
    // 1.21.2+ Garden Awakens - 新标签
    // ============================================================================

    // 苍白橡木原木标签
    auto paleOakLogs = std::make_unique<BlockTag>(ResourceLocation("minecraft", "pale_oak_logs"));
    paleOakLogs->addAll({ResourceLocation("minecraft", "pale_oak_log"),
        ResourceLocation("minecraft", "pale_oak_wood"),
        ResourceLocation("minecraft", "stripped_pale_oak_log"),
        ResourceLocation("minecraft", "stripped_pale_oak_wood")});
    tags[paleOakLogs->getId()] = std::move(paleOakLogs);

    // 可被树替换方块标签
    auto replaceableByTrees = std::make_unique<BlockTag>(ResourceLocation("minecraft", "replaceable_by_trees"));
    replaceableByTrees->addAll({// 叶子
        ResourceLocation("minecraft", "oak_leaves"),
        ResourceLocation("minecraft", "spruce_leaves"),
        ResourceLocation("minecraft", "birch_leaves"),
        ResourceLocation("minecraft", "jungle_leaves"),
        ResourceLocation("minecraft", "acacia_leaves"),
        ResourceLocation("minecraft", "dark_oak_leaves"),
        ResourceLocation("minecraft", "azalea_leaves"),
        ResourceLocation("minecraft", "flowering_azalea_leaves"),
        ResourceLocation("minecraft", "mangrove_leaves"),
        ResourceLocation("minecraft", "cherry_leaves"),
        ResourceLocation("minecraft", "pale_oak_leaves"),
        // 小花朵
        ResourceLocation("minecraft", "dandelion"),
        ResourceLocation("minecraft", "poppy"),
        ResourceLocation("minecraft", "blue_orchid"),
        ResourceLocation("minecraft", "allium"),
        ResourceLocation("minecraft", "azure_bluet"),
        ResourceLocation("minecraft", "red_tulip"),
        ResourceLocation("minecraft", "orange_tulip"),
        ResourceLocation("minecraft", "white_tulip"),
        ResourceLocation("minecraft", "pink_tulip"),
        ResourceLocation("minecraft", "oxeye_daisy"),
        ResourceLocation("minecraft", "cornflower"),
        ResourceLocation("minecraft", "lily_of_the_valley"),
        ResourceLocation("minecraft", "wither_rose"),
        ResourceLocation("minecraft", "torchflower"),
        ResourceLocation("minecraft", "open_eyeblossom"),
        ResourceLocation("minecraft", "closed_eyeblossom"),
        ResourceLocation("minecraft", "cactus_flower"),
        ResourceLocation("minecraft", "wildflowers"),
        // 苍白苔藓地毯
        ResourceLocation("minecraft", "pale_moss_carpet"),
        // 植物
        ResourceLocation("minecraft", "short_grass"),
        ResourceLocation("minecraft", "fern"),
        ResourceLocation("minecraft", "dead_bush"),
        ResourceLocation("minecraft", "vine"),
        ResourceLocation("minecraft", "glow_lichen"),
        ResourceLocation("minecraft", "hanging_roots"),
        ResourceLocation("minecraft", "pitcher_plant"),
        // 水
        ResourceLocation("minecraft", "water"),
        ResourceLocation("minecraft", "seagrass"),
        ResourceLocation("minecraft", "tall_seagrass"),
        // 1.21.2 苍白花园植物
        ResourceLocation("minecraft", "bush"),
        ResourceLocation("minecraft", "firefly_bush"),
        ResourceLocation("minecraft", "warped_roots"),
        ResourceLocation("minecraft", "nether_sprouts"),
        ResourceLocation("minecraft", "crimson_roots"),
        ResourceLocation("minecraft", "leaf_litter"),
        ResourceLocation("minecraft", "short_dry_grass"),
        ResourceLocation("minecraft", "tall_dry_grass")});
    tags[replaceableByTrees->getId()] = std::move(replaceableByTrees);

    // 创建 WITHER_IMMUNE 标签（凋灵免疫方块）
    // 凋灵无法破坏这些方块（基岩、屏障、末地传送门、命令方块等）
    auto witherImmune = std::make_unique<BlockTag>(ResourceLocation("minecraft", "wither_immune"));
    witherImmune->addAll({// 屏障方块
        ResourceLocation("minecraft", "barrier"),
        // 基岩
        ResourceLocation("minecraft", "bedrock"),
        // 末地传送门
        ResourceLocation("minecraft", "end_portal"),
        ResourceLocation("minecraft", "end_portal_frame"),
        ResourceLocation("minecraft", "end_gateway"),
        // 命令方块
        ResourceLocation("minecraft", "command_block"),
        ResourceLocation("minecraft", "repeating_command_block"),
        ResourceLocation("minecraft", "chain_command_block"),
        // 结构方块
        ResourceLocation("minecraft", "structure_block"),
        ResourceLocation("minecraft", "jigsaw"),
        // 活塞移动中的方块
        ResourceLocation("minecraft", "moving_piston"),
        // 光源方块
        ResourceLocation("minecraft", "light"),
        // 1.17 加固深板岩
        ResourceLocation("minecraft", "reinforced_deepslate"),
        // 1.21 试炼刷怪笼和宝库
        ResourceLocation("minecraft", "trial_spawner"),
        ResourceLocation("minecraft", "vault")});
    tags[witherImmune->getId()] = std::move(witherImmune);

    // 创建 DRAGON_IMMUNE 标签（末影龙免疫方块）
    // 末影龙无法破坏这些方块，碰到后标记为"碰墙"状态影响飞行行为
    auto dragonImmune = std::make_unique<BlockTag>(ResourceLocation("minecraft", "dragon_immune"));
    dragonImmune->addAll({// 屏障方块
        ResourceLocation("minecraft", "barrier"),
        // 基岩
        ResourceLocation("minecraft", "bedrock"),
        // 末地传送门和传送门框架
        ResourceLocation("minecraft", "end_portal"),
        ResourceLocation("minecraft", "end_portal_frame"),
        ResourceLocation("minecraft", "end_gateway"),
        // 命令方块
        ResourceLocation("minecraft", "command_block"),
        ResourceLocation("minecraft", "repeating_command_block"),
        ResourceLocation("minecraft", "chain_command_block"),
        // 结构方块
        ResourceLocation("minecraft", "structure_block"),
        ResourceLocation("minecraft", "jigsaw"),
        // 活塞移动中的方块
        ResourceLocation("minecraft", "moving_piston"),
        // 黑曜石和哭泣黑曜石
        ResourceLocation("minecraft", "obsidian"),
        ResourceLocation("minecraft", "crying_obsidian"),
        // 末地石
        ResourceLocation("minecraft", "end_stone"),
        // 铁栏杆
        ResourceLocation("minecraft", "iron_bars"),
        // 重生锚
        ResourceLocation("minecraft", "respawn_anchor"),
        // 加固深板岩
        ResourceLocation("minecraft", "reinforced_deepslate"),
        // 1.21 试炼刷怪笼和宝库
        ResourceLocation("minecraft", "trial_spawner"),
        ResourceLocation("minecraft", "vault")});
    tags[dragonImmune->getId()] = std::move(dragonImmune);

    // 创建 DRAGON_TRANSPARENT 标签（末影龙透明方块）
    // 末影龙穿过这些方块时不破坏它们
    auto dragonTransparent = std::make_unique<BlockTag>(ResourceLocation("minecraft", "dragon_transparent"));
    dragonTransparent->addAll({// 光照方块
        ResourceLocation("minecraft", "light")});
    tags[dragonTransparent->getId()] = std::move(dragonTransparent);

    // ============================================================================
    // 额外标签更新 - 1.16+ 黑石和下界方块
    // ============================================================================

    // 黑石方块标签
    auto blackstoneTag = std::make_unique<BlockTag>(ResourceLocation("minecraft", "blackstone"));
    blackstoneTag->addAll({ResourceLocation("minecraft", "blackstone"),
        ResourceLocation("minecraft", "blackstone_stairs"),
        ResourceLocation("minecraft", "blackstone_slab"),
        ResourceLocation("minecraft", "blackstone_wall"),
        ResourceLocation("minecraft", "polished_blackstone"),
        ResourceLocation("minecraft", "polished_blackstone_stairs"),
        ResourceLocation("minecraft", "polished_blackstone_slab"),
        ResourceLocation("minecraft", "polished_blackstone_wall"),
        ResourceLocation("minecraft", "polished_blackstone_bricks"),
        ResourceLocation("minecraft", "polished_blackstone_brick_stairs"),
        ResourceLocation("minecraft", "polished_blackstone_brick_slab"),
        ResourceLocation("minecraft", "polished_blackstone_brick_wall"),
        ResourceLocation("minecraft", "chiseled_polished_blackstone"),
        ResourceLocation("minecraft", "cracked_polished_blackstone_bricks"),
        ResourceLocation("minecraft", "gilded_blackstone")});
    tags[blackstoneTag->getId()] = std::move(blackstoneTag);

    // 磁石标签
    auto lodestoneTag = std::make_unique<BlockTag>(ResourceLocation("minecraft", "lodestone"));
    lodestoneTag->addAll({ResourceLocation("minecraft", "lodestone")});
    tags[lodestoneTag->getId()] = std::move(lodestoneTag);

    // 主世界可雕刻方块标签
    auto overworldCarverReplaceables =
        std::make_unique<BlockTag>(ResourceLocation("minecraft", "overworld_carver_replaceables"));
    overworldCarverReplaceables->addAll({
        ResourceLocation("minecraft", "stone"),
        ResourceLocation("minecraft", "granite"),
        ResourceLocation("minecraft", "diorite"),
        ResourceLocation("minecraft", "andesite"),
        ResourceLocation("minecraft", "deepslate"),
        ResourceLocation("minecraft", "tuff"),
        ResourceLocation("minecraft", "calcite"),
        ResourceLocation("minecraft", "dirt"),
        ResourceLocation("minecraft", "coarse_dirt"),
        ResourceLocation("minecraft", "podzol"),
        ResourceLocation("minecraft", "grass_block"),
        ResourceLocation("minecraft", "moss_block"),
        ResourceLocation("minecraft", "terracotta"),
        ResourceLocation("minecraft", "white_terracotta"),
        ResourceLocation("minecraft", "orange_terracotta"),
        ResourceLocation("minecraft", "magenta_terracotta"),
        ResourceLocation("minecraft", "light_blue_terracotta"),
        ResourceLocation("minecraft", "yellow_terracotta"),
        ResourceLocation("minecraft", "lime_terracotta"),
        ResourceLocation("minecraft", "pink_terracotta"),
        ResourceLocation("minecraft", "gray_terracotta"),
        ResourceLocation("minecraft", "light_gray_terracotta"),
        ResourceLocation("minecraft", "cyan_terracotta"),
        ResourceLocation("minecraft", "purple_terracotta"),
        ResourceLocation("minecraft", "blue_terracotta"),
        ResourceLocation("minecraft", "brown_terracotta"),
        ResourceLocation("minecraft", "green_terracotta"),
        ResourceLocation("minecraft", "red_terracotta"),
        ResourceLocation("minecraft", "black_terracotta"),
        ResourceLocation("minecraft", "sand"),
        ResourceLocation("minecraft", "red_sand"),
        ResourceLocation("minecraft", "sandstone"),
        ResourceLocation("minecraft", "red_sandstone"),
        ResourceLocation("minecraft", "gravel"),
        ResourceLocation("minecraft", "packed_ice"),
        ResourceLocation("minecraft", "mycelium"),
        ResourceLocation("minecraft", "snow_block"),
        ResourceLocation("minecraft", "clay"),
        ResourceLocation("minecraft", "dripstone_block"),
        ResourceLocation("minecraft", "pointed_dripstone"),
    });
    tags[overworldCarverReplaceables->getId()] = std::move(overworldCarverReplaceables);

    // 下界可雕刻方块标签
    auto netherCarverReplaceables =
        std::make_unique<BlockTag>(ResourceLocation("minecraft", "nether_carver_replaceables"));
    netherCarverReplaceables->addAll({
        ResourceLocation("minecraft", "netherrack"),
        ResourceLocation("minecraft", "soul_sand"),
        ResourceLocation("minecraft", "soul_soil"),
        ResourceLocation("minecraft", "crimson_nylium"),
        ResourceLocation("minecraft", "warped_nylium"),
        ResourceLocation("minecraft", "nether_wart_block"),
        ResourceLocation("minecraft", "warped_wart_block"),
        ResourceLocation("minecraft", "basalt"),
        ResourceLocation("minecraft", "blackstone"),
        ResourceLocation("minecraft", "gravel"),
        ResourceLocation("minecraft", "magma_block"),
    });
    tags[netherCarverReplaceables->getId()] = std::move(netherCarverReplaceables);

    // 创建 ANVIL 标签（铁砧系列方块）
    auto anvilTag = std::make_unique<BlockTag>(ResourceLocation("minecraft", "anvil"));
    anvilTag->addAll({ResourceLocation("minecraft", "anvil"),
        ResourceLocation("minecraft", "chipped_anvil"),
        ResourceLocation("minecraft", "damaged_anvil")});
    tags[anvilTag->getId()] = std::move(anvilTag);

    // 创建 SNOW_LAYER_CANNOT_SURVIVE_ON 标签（雪层不可放置方块）
    // 雪层不能在这些方块上方存活（即使它们有完整的上表面碰撞箱）
    auto snowLayerCannotSurviveOn =
        std::make_unique<BlockTag>(ResourceLocation("minecraft", "snow_layer_cannot_survive_on"));
    snowLayerCannotSurviveOn->addAll({ResourceLocation("minecraft", "ice"),
        ResourceLocation("minecraft", "packed_ice"),
        ResourceLocation("minecraft", "barrier")});
    tags[snowLayerCannotSurviveOn->getId()] = std::move(snowLayerCannotSurviveOn);

    // 创建 SNOW_LAYER_CAN_SURVIVE_ON 标签（雪层可放置方块）
    // 雪层可以在这些方块上方存活（即使它们没有完整的上表面碰撞箱）
    auto snowLayerCanSurviveOn = std::make_unique<BlockTag>(ResourceLocation("minecraft", "snow_layer_can_survive_on"));
    snowLayerCanSurviveOn->addAll({ResourceLocation("minecraft", "honey_block"),
        ResourceLocation("minecraft", "soul_sand"),
        ResourceLocation("minecraft", "mud")});
    tags[snowLayerCanSurviveOn->getId()] = std::move(snowLayerCanSurviveOn);

    // 创建 SMALL_DRIPLEAF_PLACEABLE 标签（小滴叶可放置方块）
    // 参考: net.minecraft.tags.BlockTags.SMALL_DRIPLEAF_PLACEABLE
    auto smallDripleafPlaceable = std::make_unique<BlockTag>(ResourceLocation("minecraft", "small_dripleaf_placeable"));
    smallDripleafPlaceable->addAll(
        {ResourceLocation("minecraft", "clay"), ResourceLocation("minecraft", "moss_block")});
    tags[smallDripleafPlaceable->getId()] = std::move(smallDripleafPlaceable);

    // 创建 BIG_DRIPLEAF_PLACEABLE 标签（大滴叶可放置方块）
    // 参考: net.minecraft.tags.BlockTags.BIG_DRIPLEAF_PLACEABLE
    // 包含: 黏土、泥土、砂土、灰化土、耕地、苔藓块、缠根泥土、泥巴、泥泞红树林根、草方块、菌丝、沙子、小滴叶
    auto bigDripleafPlaceable = std::make_unique<BlockTag>(ResourceLocation("minecraft", "big_dripleaf_placeable"));
    bigDripleafPlaceable->addAll({ResourceLocation("minecraft", "clay"),
        ResourceLocation("minecraft", "dirt"),
        ResourceLocation("minecraft", "coarse_dirt"),
        ResourceLocation("minecraft", "podzol"),
        ResourceLocation("minecraft", "farmland"),
        ResourceLocation("minecraft", "moss_block"),
        ResourceLocation("minecraft", "rooted_dirt"),
        ResourceLocation("minecraft", "mud"),
        ResourceLocation("minecraft", "muddy_mangrove_roots"),
        ResourceLocation("minecraft", "grass_block"),
        ResourceLocation("minecraft", "mycelium"),
        ResourceLocation("minecraft", "sand"),
        ResourceLocation("minecraft", "small_dripleaf")});
    tags[bigDripleafPlaceable->getId()] = std::move(bigDripleafPlaceable);

    // ============================================================================
    // 建筑方块形状标签
    // ============================================================================

    // 楼梯方块标签（所有楼梯方块）
    auto stairs = std::make_unique<BlockTag>(ResourceLocation("minecraft", "stairs"));
    stairs->addAll({// 木质楼梯
        ResourceLocation("minecraft", "oak_stairs"),
        ResourceLocation("minecraft", "spruce_stairs"),
        ResourceLocation("minecraft", "birch_stairs"),
        ResourceLocation("minecraft", "jungle_stairs"),
        ResourceLocation("minecraft", "acacia_stairs"),
        ResourceLocation("minecraft", "dark_oak_stairs"),
        ResourceLocation("minecraft", "mangrove_stairs"),
        ResourceLocation("minecraft", "cherry_stairs"),
        ResourceLocation("minecraft", "bamboo_stairs"),
        ResourceLocation("minecraft", "bamboo_mosaic_stairs"),
        ResourceLocation("minecraft", "pale_oak_stairs"),
        ResourceLocation("minecraft", "crimson_stairs"),
        ResourceLocation("minecraft", "warped_stairs"),
        // 石质楼梯
        ResourceLocation("minecraft", "stone_stairs"),
        ResourceLocation("minecraft", "cobblestone_stairs"),
        ResourceLocation("minecraft", "mossy_cobblestone_stairs"),
        ResourceLocation("minecraft", "stone_brick_stairs"),
        ResourceLocation("minecraft", "mossy_stone_brick_stairs"),
        ResourceLocation("minecraft", "sandstone_stairs"),
        ResourceLocation("minecraft", "smooth_sandstone_stairs"),
        ResourceLocation("minecraft", "granite_stairs"),
        ResourceLocation("minecraft", "polished_granite_stairs"),
        ResourceLocation("minecraft", "diorite_stairs"),
        ResourceLocation("minecraft", "polished_diorite_stairs"),
        ResourceLocation("minecraft", "andesite_stairs"),
        ResourceLocation("minecraft", "polished_andesite_stairs"),
        ResourceLocation("minecraft", "brick_stairs"),
        ResourceLocation("minecraft", "prismarine_stairs"),
        ResourceLocation("minecraft", "prismarine_brick_stairs"),
        ResourceLocation("minecraft", "dark_prismarine_stairs"),
        ResourceLocation("minecraft", "nether_brick_stairs"),
        ResourceLocation("minecraft", "red_nether_brick_stairs"),
        ResourceLocation("minecraft", "quartz_stairs"),
        ResourceLocation("minecraft", "smooth_quartz_stairs"),
        ResourceLocation("minecraft", "purpur_stairs"),
        ResourceLocation("minecraft", "end_stone_brick_stairs"),
        ResourceLocation("minecraft", "red_sandstone_stairs"),
        ResourceLocation("minecraft", "smooth_red_sandstone_stairs"),
        // 深板岩楼梯
        ResourceLocation("minecraft", "cobbled_deepslate_stairs"),
        ResourceLocation("minecraft", "polished_deepslate_stairs"),
        ResourceLocation("minecraft", "deepslate_brick_stairs"),
        ResourceLocation("minecraft", "deepslate_tile_stairs"),
        // 黑石楼梯
        ResourceLocation("minecraft", "blackstone_stairs"),
        ResourceLocation("minecraft", "polished_blackstone_stairs"),
        ResourceLocation("minecraft", "polished_blackstone_brick_stairs"),
        // 凝灰岩楼梯
        ResourceLocation("minecraft", "tuff_stairs"),
        ResourceLocation("minecraft", "polished_tuff_stairs"),
        ResourceLocation("minecraft", "tuff_brick_stairs"),
        // 泥砖楼梯
        ResourceLocation("minecraft", "mud_brick_stairs"),
        // 树脂砖楼梯
        ResourceLocation("minecraft", "resin_brick_stairs"),
        // 铜楼梯
        ResourceLocation("minecraft", "cut_copper_stairs"),
        ResourceLocation("minecraft", "exposed_cut_copper_stairs"),
        ResourceLocation("minecraft", "weathered_cut_copper_stairs"),
        ResourceLocation("minecraft", "oxidized_cut_copper_stairs"),
        ResourceLocation("minecraft", "waxed_cut_copper_stairs"),
        ResourceLocation("minecraft", "waxed_exposed_cut_copper_stairs"),
        ResourceLocation("minecraft", "waxed_weathered_cut_copper_stairs"),
        ResourceLocation("minecraft", "waxed_oxidized_cut_copper_stairs")});
    tags[stairs->getId()] = std::move(stairs);

    // 台阶方块标签（所有台阶方块）
    auto slabs = std::make_unique<BlockTag>(ResourceLocation("minecraft", "slabs"));
    slabs->addAll({// 木质台阶
        ResourceLocation("minecraft", "oak_slab"),
        ResourceLocation("minecraft", "spruce_slab"),
        ResourceLocation("minecraft", "birch_slab"),
        ResourceLocation("minecraft", "jungle_slab"),
        ResourceLocation("minecraft", "acacia_slab"),
        ResourceLocation("minecraft", "dark_oak_slab"),
        ResourceLocation("minecraft", "mangrove_slab"),
        ResourceLocation("minecraft", "cherry_slab"),
        ResourceLocation("minecraft", "bamboo_slab"),
        ResourceLocation("minecraft", "bamboo_mosaic_slab"),
        ResourceLocation("minecraft", "pale_oak_slab"),
        ResourceLocation("minecraft", "crimson_slab"),
        ResourceLocation("minecraft", "warped_slab"),
        // 石质台阶
        ResourceLocation("minecraft", "stone_slab"),
        ResourceLocation("minecraft", "smooth_stone_slab"),
        ResourceLocation("minecraft", "cobblestone_slab"),
        ResourceLocation("minecraft", "mossy_cobblestone_slab"),
        ResourceLocation("minecraft", "stone_brick_slab"),
        ResourceLocation("minecraft", "mossy_stone_brick_slab"),
        ResourceLocation("minecraft", "sandstone_slab"),
        ResourceLocation("minecraft", "smooth_sandstone_slab"),
        ResourceLocation("minecraft", "cut_sandstone_slab"),
        ResourceLocation("minecraft", "granite_slab"),
        ResourceLocation("minecraft", "polished_granite_slab"),
        ResourceLocation("minecraft", "diorite_slab"),
        ResourceLocation("minecraft", "polished_diorite_slab"),
        ResourceLocation("minecraft", "andesite_slab"),
        ResourceLocation("minecraft", "polished_andesite_slab"),
        ResourceLocation("minecraft", "brick_slab"),
        ResourceLocation("minecraft", "prismarine_slab"),
        ResourceLocation("minecraft", "prismarine_brick_slab"),
        ResourceLocation("minecraft", "dark_prismarine_slab"),
        ResourceLocation("minecraft", "nether_brick_slab"),
        ResourceLocation("minecraft", "red_nether_brick_slab"),
        ResourceLocation("minecraft", "quartz_slab"),
        ResourceLocation("minecraft", "smooth_quartz_slab"),
        ResourceLocation("minecraft", "purpur_slab"),
        ResourceLocation("minecraft", "end_stone_brick_slab"),
        ResourceLocation("minecraft", "red_sandstone_slab"),
        ResourceLocation("minecraft", "smooth_red_sandstone_slab"),
        ResourceLocation("minecraft", "cut_red_sandstone_slab"),
        // 深板岩台阶
        ResourceLocation("minecraft", "cobbled_deepslate_slab"),
        ResourceLocation("minecraft", "polished_deepslate_slab"),
        ResourceLocation("minecraft", "deepslate_brick_slab"),
        ResourceLocation("minecraft", "deepslate_tile_slab"),
        // 黑石台阶
        ResourceLocation("minecraft", "blackstone_slab"),
        ResourceLocation("minecraft", "polished_blackstone_slab"),
        ResourceLocation("minecraft", "polished_blackstone_brick_slab"),
        // 凝灰岩台阶
        ResourceLocation("minecraft", "tuff_slab"),
        ResourceLocation("minecraft", "polished_tuff_slab"),
        ResourceLocation("minecraft", "tuff_brick_slab"),
        // 泥砖台阶
        ResourceLocation("minecraft", "mud_brick_slab"),
        // 树脂砖台阶
        ResourceLocation("minecraft", "resin_brick_slab"),
        // 铜台阶
        ResourceLocation("minecraft", "cut_copper_slab"),
        ResourceLocation("minecraft", "exposed_cut_copper_slab"),
        ResourceLocation("minecraft", "weathered_cut_copper_slab"),
        ResourceLocation("minecraft", "oxidized_cut_copper_slab"),
        ResourceLocation("minecraft", "waxed_cut_copper_slab"),
        ResourceLocation("minecraft", "waxed_exposed_cut_copper_slab"),
        ResourceLocation("minecraft", "waxed_weathered_cut_copper_slab"),
        ResourceLocation("minecraft", "waxed_oxidized_cut_copper_slab"),
        // 切石台阶（petrified_oak_slab 在新版本中归入此类）
        ResourceLocation("minecraft", "petrified_oak_slab")});
    tags[slabs->getId()] = std::move(slabs);

    // 墙壁方块标签（所有 WallBlock 类型的墙壁方块）
    auto walls = std::make_unique<BlockTag>(ResourceLocation("minecraft", "walls"));
    walls->addAll({ResourceLocation("minecraft", "cobblestone_wall"),
        ResourceLocation("minecraft", "mossy_cobblestone_wall"),
        ResourceLocation("minecraft", "stone_brick_wall"),
        ResourceLocation("minecraft", "mossy_stone_brick_wall"),
        ResourceLocation("minecraft", "granite_wall"),
        ResourceLocation("minecraft", "diorite_wall"),
        ResourceLocation("minecraft", "andesite_wall"),
        ResourceLocation("minecraft", "brick_wall"),
        ResourceLocation("minecraft", "prismarine_wall"),
        ResourceLocation("minecraft", "nether_brick_wall"),
        ResourceLocation("minecraft", "red_nether_brick_wall"),
        ResourceLocation("minecraft", "sandstone_wall"),
        ResourceLocation("minecraft", "red_sandstone_wall"),
        ResourceLocation("minecraft", "end_stone_brick_wall"),
        // 深板岩墙
        ResourceLocation("minecraft", "cobbled_deepslate_wall"),
        ResourceLocation("minecraft", "polished_deepslate_wall"),
        ResourceLocation("minecraft", "deepslate_brick_wall"),
        ResourceLocation("minecraft", "deepslate_tile_wall"),
        // 黑石墙
        ResourceLocation("minecraft", "blackstone_wall"),
        ResourceLocation("minecraft", "polished_blackstone_wall"),
        ResourceLocation("minecraft", "polished_blackstone_brick_wall"),
        // 凝灰岩墙
        ResourceLocation("minecraft", "tuff_wall"),
        ResourceLocation("minecraft", "polished_tuff_wall"),
        ResourceLocation("minecraft", "tuff_brick_wall"),
        // 泥砖墙
        ResourceLocation("minecraft", "mud_brick_wall"),
        // 树脂砖墙
        ResourceLocation("minecraft", "resin_brick_wall")});
    tags[walls->getId()] = std::move(walls);

    // 创建 GUARDED_BY_PIGLINS 标签（猪灵守护的方块，破坏时激怒附近猪灵）
    auto guardedByPiglins = std::make_unique<BlockTag>(ResourceLocation("minecraft", "guarded_by_piglins"));
    guardedByPiglins->addAll({// 箱子
        ResourceLocation("minecraft", "chest"),
        // 末影箱
        ResourceLocation("minecraft", "ender_chest"),
        // 木桶
        ResourceLocation("minecraft", "barrel"),
        // 金块相关
        ResourceLocation("minecraft", "gold_block"),
        ResourceLocation("minecraft", "raw_gold_block"),
        // 金矿石
        ResourceLocation("minecraft", "gold_ore"),
        ResourceLocation("minecraft", "deepslate_gold_ore"),
        // 下界金矿石
        ResourceLocation("minecraft", "nether_gold_ore"),
        // 猪灵所需方块
        ResourceLocation("minecraft", "gilded_blackstone")});
    tags[guardedByPiglins->getId()] = std::move(guardedByPiglins);

    // 创建 BARS 标签（铁栏杆、铜栏杆等，用于墙壁和玻璃板连接判断）
    // 参考: datapacks/Vanilla/data/minecraft/tags/block/bars.json
    auto bars = std::make_unique<BlockTag>(ResourceLocation("minecraft", "bars"));
    bars->addAll({ResourceLocation("minecraft", "iron_bars"),
        ResourceLocation("minecraft", "copper_bars"),
        ResourceLocation("minecraft", "waxed_copper_bars"),
        ResourceLocation("minecraft", "exposed_copper_bars"),
        ResourceLocation("minecraft", "waxed_exposed_copper_bars"),
        ResourceLocation("minecraft", "weathered_copper_bars"),
        ResourceLocation("minecraft", "waxed_weathered_copper_bars"),
        ResourceLocation("minecraft", "oxidized_copper_bars"),
        ResourceLocation("minecraft", "waxed_oxidized_copper_bars")});
    tags[bars->getId()] = std::move(bars);

    // 创建 CHAINS 标签（铁锁链和铜锁链，含氧化和涂蜡变种）
    // 参考: datapacks/Vanilla/data/minecraft/tags/block/chains.json
    auto chains = std::make_unique<BlockTag>(ResourceLocation("minecraft", "chains"));
    chains->addAll({ResourceLocation("minecraft", "iron_chain"),
        ResourceLocation("minecraft", "copper_chain"),
        ResourceLocation("minecraft", "waxed_copper_chain"),
        ResourceLocation("minecraft", "exposed_copper_chain"),
        ResourceLocation("minecraft", "waxed_exposed_copper_chain"),
        ResourceLocation("minecraft", "weathered_copper_chain"),
        ResourceLocation("minecraft", "waxed_weathered_copper_chain"),
        ResourceLocation("minecraft", "oxidized_copper_chain"),
        ResourceLocation("minecraft", "waxed_oxidized_copper_chain")});
    tags[chains->getId()] = std::move(chains);

    // 创建 SHULKER_BOXES 标签（所有潜影盒变体，用于连接例外判断）
    auto shulkerBoxes = std::make_unique<BlockTag>(ResourceLocation("minecraft", "shulker_boxes"));
    shulkerBoxes->addAll({ResourceLocation("minecraft", "shulker_box"),
        ResourceLocation("minecraft", "white_shulker_box"),
        ResourceLocation("minecraft", "orange_shulker_box"),
        ResourceLocation("minecraft", "magenta_shulker_box"),
        ResourceLocation("minecraft", "light_blue_shulker_box"),
        ResourceLocation("minecraft", "yellow_shulker_box"),
        ResourceLocation("minecraft", "lime_shulker_box"),
        ResourceLocation("minecraft", "pink_shulker_box"),
        ResourceLocation("minecraft", "gray_shulker_box"),
        ResourceLocation("minecraft", "light_gray_shulker_box"),
        ResourceLocation("minecraft", "cyan_shulker_box"),
        ResourceLocation("minecraft", "purple_shulker_box"),
        ResourceLocation("minecraft", "blue_shulker_box"),
        ResourceLocation("minecraft", "brown_shulker_box"),
        ResourceLocation("minecraft", "green_shulker_box"),
        ResourceLocation("minecraft", "red_shulker_box"),
        ResourceLocation("minecraft", "black_shulker_box")});
    tags[shulkerBoxes->getId()] = std::move(shulkerBoxes);

    // 创建 WALL_POST_OVERRIDE 标签（放置在墙上时强制显示墙柱的方块）
    // 参考: net.minecraft.data.tags.VanillaBlockTagsProvider - WALL_POST_OVERRIDE
    // 参考: datapacks/Vanilla/data/minecraft/tags/block/wall_post_override.json
    // 注意: JSON中使用标签引用(#minecraft:signs等)，此处展开为具体方块ID
    auto wallPostOverride = std::make_unique<BlockTag>(ResourceLocation("minecraft", "wall_post_override"));
    wallPostOverride->addAll({ResourceLocation("minecraft", "torch"),
        ResourceLocation("minecraft", "soul_torch"),
        ResourceLocation("minecraft", "redstone_torch"),
        ResourceLocation("minecraft", "copper_torch"),
        ResourceLocation("minecraft", "tripwire"),
        // 告示牌（所有变体）
        ResourceLocation("minecraft", "oak_sign"),
        ResourceLocation("minecraft", "spruce_sign"),
        ResourceLocation("minecraft", "birch_sign"),
        ResourceLocation("minecraft", "jungle_sign"),
        ResourceLocation("minecraft", "acacia_sign"),
        ResourceLocation("minecraft", "dark_oak_sign"),
        ResourceLocation("minecraft", "mangrove_sign"),
        ResourceLocation("minecraft", "cherry_sign"),
        ResourceLocation("minecraft", "bamboo_sign"),
        ResourceLocation("minecraft", "pale_oak_sign"),
        ResourceLocation("minecraft", "crimson_sign"),
        ResourceLocation("minecraft", "warped_sign"),
        ResourceLocation("minecraft", "oak_wall_sign"),
        ResourceLocation("minecraft", "spruce_wall_sign"),
        ResourceLocation("minecraft", "birch_wall_sign"),
        ResourceLocation("minecraft", "jungle_wall_sign"),
        ResourceLocation("minecraft", "acacia_wall_sign"),
        ResourceLocation("minecraft", "dark_oak_wall_sign"),
        ResourceLocation("minecraft", "mangrove_wall_sign"),
        ResourceLocation("minecraft", "cherry_wall_sign"),
        ResourceLocation("minecraft", "bamboo_wall_sign"),
        ResourceLocation("minecraft", "pale_oak_wall_sign"),
        ResourceLocation("minecraft", "crimson_wall_sign"),
        ResourceLocation("minecraft", "warped_wall_sign"),
        // 旗帜（所有颜色）
        ResourceLocation("minecraft", "white_banner"),
        ResourceLocation("minecraft", "orange_banner"),
        ResourceLocation("minecraft", "magenta_banner"),
        ResourceLocation("minecraft", "light_blue_banner"),
        ResourceLocation("minecraft", "yellow_banner"),
        ResourceLocation("minecraft", "lime_banner"),
        ResourceLocation("minecraft", "pink_banner"),
        ResourceLocation("minecraft", "gray_banner"),
        ResourceLocation("minecraft", "light_gray_banner"),
        ResourceLocation("minecraft", "cyan_banner"),
        ResourceLocation("minecraft", "purple_banner"),
        ResourceLocation("minecraft", "blue_banner"),
        ResourceLocation("minecraft", "brown_banner"),
        ResourceLocation("minecraft", "green_banner"),
        ResourceLocation("minecraft", "red_banner"),
        ResourceLocation("minecraft", "black_banner"),
        ResourceLocation("minecraft", "white_wall_banner"),
        ResourceLocation("minecraft", "orange_wall_banner"),
        ResourceLocation("minecraft", "magenta_wall_banner"),
        ResourceLocation("minecraft", "light_blue_wall_banner"),
        ResourceLocation("minecraft", "yellow_wall_banner"),
        ResourceLocation("minecraft", "lime_wall_banner"),
        ResourceLocation("minecraft", "pink_wall_banner"),
        ResourceLocation("minecraft", "gray_wall_banner"),
        ResourceLocation("minecraft", "light_gray_wall_banner"),
        ResourceLocation("minecraft", "cyan_wall_banner"),
        ResourceLocation("minecraft", "purple_wall_banner"),
        ResourceLocation("minecraft", "blue_wall_banner"),
        ResourceLocation("minecraft", "brown_wall_banner"),
        ResourceLocation("minecraft", "green_wall_banner"),
        ResourceLocation("minecraft", "red_wall_banner"),
        ResourceLocation("minecraft", "black_wall_banner"),
        // 压力板（所有变体）
        ResourceLocation("minecraft", "oak_pressure_plate"),
        ResourceLocation("minecraft", "spruce_pressure_plate"),
        ResourceLocation("minecraft", "birch_pressure_plate"),
        ResourceLocation("minecraft", "jungle_pressure_plate"),
        ResourceLocation("minecraft", "acacia_pressure_plate"),
        ResourceLocation("minecraft", "dark_oak_pressure_plate"),
        ResourceLocation("minecraft", "mangrove_pressure_plate"),
        ResourceLocation("minecraft", "cherry_pressure_plate"),
        ResourceLocation("minecraft", "bamboo_pressure_plate"),
        ResourceLocation("minecraft", "pale_oak_pressure_plate"),
        ResourceLocation("minecraft", "crimson_pressure_plate"),
        ResourceLocation("minecraft", "warped_pressure_plate"),
        ResourceLocation("minecraft", "stone_pressure_plate"),
        ResourceLocation("minecraft", "polished_blackstone_pressure_plate"),
        ResourceLocation("minecraft", "heavy_weighted_pressure_plate"),
        ResourceLocation("minecraft", "light_weighted_pressure_plate"),
        // 仙人掌花
        ResourceLocation("minecraft", "cactus_flower")});
    tags[wallPostOverride->getId()] = std::move(wallPostOverride);

    // 创建 CAULDRONS 标签（所有炼药锅变体）
    // 参考: net.minecraft.data.tags.VanillaBlockTagsProvider - CAULDRONS
    // 参考: datapacks/Vanilla/data/minecraft/tags/block/cauldrons.json
    auto cauldrons = std::make_unique<BlockTag>(ResourceLocation("minecraft", "cauldrons"));
    cauldrons->addAll({ResourceLocation("minecraft", "cauldron"),
        ResourceLocation("minecraft", "water_cauldron"),
        ResourceLocation("minecraft", "lava_cauldron"),
        ResourceLocation("minecraft", "powder_snow_cauldron")});
    tags[cauldrons->getId()] = std::move(cauldrons);

    // 创建 WOODEN_DOORS 标签（所有木门方块）
    // 参考: net.minecraft.tags.BlockTags.WOODEN_DOORS
    auto woodenDoors = std::make_unique<BlockTag>(ResourceLocation("minecraft", "wooden_doors"));
    woodenDoors->addAll({ResourceLocation("minecraft", "oak_door"),
        ResourceLocation("minecraft", "spruce_door"),
        ResourceLocation("minecraft", "birch_door"),
        ResourceLocation("minecraft", "jungle_door"),
        ResourceLocation("minecraft", "acacia_door"),
        ResourceLocation("minecraft", "dark_oak_door"),
        ResourceLocation("minecraft", "mangrove_door"),
        ResourceLocation("minecraft", "cherry_door"),
        ResourceLocation("minecraft", "bamboo_door"),
        ResourceLocation("minecraft", "pale_oak_door"),
        ResourceLocation("minecraft", "crimson_door"),
        ResourceLocation("minecraft", "warped_door")});
    tags[woodenDoors->getId()] = std::move(woodenDoors);

    // 创建 DOORS 标签（所有门方块）
    // 参考: net.minecraft.tags.BlockTags.DOORS
    // 包含所有木门 + 铁门 + 铜门（含氧化和涂蜡变种）
    auto doors = std::make_unique<BlockTag>(ResourceLocation("minecraft", "doors"));
    doors->addAll({// 木质门
        ResourceLocation("minecraft", "oak_door"),
        ResourceLocation("minecraft", "spruce_door"),
        ResourceLocation("minecraft", "birch_door"),
        ResourceLocation("minecraft", "jungle_door"),
        ResourceLocation("minecraft", "acacia_door"),
        ResourceLocation("minecraft", "dark_oak_door"),
        ResourceLocation("minecraft", "mangrove_door"),
        ResourceLocation("minecraft", "cherry_door"),
        ResourceLocation("minecraft", "bamboo_door"),
        ResourceLocation("minecraft", "pale_oak_door"),
        ResourceLocation("minecraft", "crimson_door"),
        ResourceLocation("minecraft", "warped_door"),
        // 铁门
        ResourceLocation("minecraft", "iron_door"),
        // 铜门（含氧化和涂蜡变种）
        ResourceLocation("minecraft", "copper_door"),
        ResourceLocation("minecraft", "exposed_copper_door"),
        ResourceLocation("minecraft", "weathered_copper_door"),
        ResourceLocation("minecraft", "oxidized_copper_door"),
        ResourceLocation("minecraft", "waxed_copper_door"),
        ResourceLocation("minecraft", "waxed_exposed_copper_door"),
        ResourceLocation("minecraft", "waxed_weathered_copper_door"),
        ResourceLocation("minecraft", "waxed_oxidized_copper_door")});
    tags[doors->getId()] = std::move(doors);

    // 创建 WOODEN_TRAPDOORS 标签（所有木活板门方块）
    // 参考: net.minecraft.tags.BlockTags.WOODEN_TRAPDOORS
    auto woodenTrapdoors = std::make_unique<BlockTag>(ResourceLocation("minecraft", "wooden_trapdoors"));
    woodenTrapdoors->addAll({ResourceLocation("minecraft", "oak_trapdoor"),
        ResourceLocation("minecraft", "spruce_trapdoor"),
        ResourceLocation("minecraft", "birch_trapdoor"),
        ResourceLocation("minecraft", "jungle_trapdoor"),
        ResourceLocation("minecraft", "acacia_trapdoor"),
        ResourceLocation("minecraft", "dark_oak_trapdoor"),
        ResourceLocation("minecraft", "mangrove_trapdoor"),
        ResourceLocation("minecraft", "cherry_trapdoor"),
        ResourceLocation("minecraft", "bamboo_trapdoor"),
        ResourceLocation("minecraft", "pale_oak_trapdoor"),
        ResourceLocation("minecraft", "crimson_trapdoor"),
        ResourceLocation("minecraft", "warped_trapdoor")});
    tags[woodenTrapdoors->getId()] = std::move(woodenTrapdoors);

    // 创建 TRAPDOORS 标签（所有活板门方块）
    // 参考: net.minecraft.tags.BlockTags.TRAPDOORS
    // 包含所有木活板门 + 铁活板门 + 铜活板门（含氧化和涂蜡变种）
    auto trapdoors = std::make_unique<BlockTag>(ResourceLocation("minecraft", "trapdoors"));
    trapdoors->addAll({// 木质活板门
        ResourceLocation("minecraft", "oak_trapdoor"),
        ResourceLocation("minecraft", "spruce_trapdoor"),
        ResourceLocation("minecraft", "birch_trapdoor"),
        ResourceLocation("minecraft", "jungle_trapdoor"),
        ResourceLocation("minecraft", "acacia_trapdoor"),
        ResourceLocation("minecraft", "dark_oak_trapdoor"),
        ResourceLocation("minecraft", "mangrove_trapdoor"),
        ResourceLocation("minecraft", "cherry_trapdoor"),
        ResourceLocation("minecraft", "bamboo_trapdoor"),
        ResourceLocation("minecraft", "pale_oak_trapdoor"),
        ResourceLocation("minecraft", "crimson_trapdoor"),
        ResourceLocation("minecraft", "warped_trapdoor"),
        // 铁活板门
        ResourceLocation("minecraft", "iron_trapdoor"),
        // 铜活板门（含氧化和涂蜡变种）
        ResourceLocation("minecraft", "copper_trapdoor"),
        ResourceLocation("minecraft", "exposed_copper_trapdoor"),
        ResourceLocation("minecraft", "weathered_copper_trapdoor"),
        ResourceLocation("minecraft", "oxidized_copper_trapdoor"),
        ResourceLocation("minecraft", "waxed_copper_trapdoor"),
        ResourceLocation("minecraft", "waxed_exposed_copper_trapdoor"),
        ResourceLocation("minecraft", "waxed_weathered_copper_trapdoor"),
        ResourceLocation("minecraft", "waxed_oxidized_copper_trapdoor")});
    tags[trapdoors->getId()] = std::move(trapdoors);

    // 创建 NON_FLAMMABLE_WOOD 标签（不可燃木材方块）
    // 参考: net.minecraft.tags.BlockTags.NON_FLAMMABLE_WOOD
    // 包含绯红木和诡异木系列的所有方块
    auto nonFlammableWood = std::make_unique<BlockTag>(ResourceLocation("minecraft", "non_flammable_wood"));
    nonFlammableWood->addAll({// 绯红木系列
        ResourceLocation("minecraft", "crimson_stem"),
        ResourceLocation("minecraft", "stripped_crimson_stem"),
        ResourceLocation("minecraft", "crimson_hyphae"),
        ResourceLocation("minecraft", "stripped_crimson_hyphae"),
        ResourceLocation("minecraft", "crimson_planks"),
        ResourceLocation("minecraft", "crimson_slab"),
        ResourceLocation("minecraft", "crimson_stairs"),
        ResourceLocation("minecraft", "crimson_fence"),
        ResourceLocation("minecraft", "crimson_fence_gate"),
        ResourceLocation("minecraft", "crimson_door"),
        ResourceLocation("minecraft", "crimson_trapdoor"),
        ResourceLocation("minecraft", "crimson_button"),
        ResourceLocation("minecraft", "crimson_pressure_plate"),
        ResourceLocation("minecraft", "crimson_sign"),
        ResourceLocation("minecraft", "crimson_wall_sign"),
        ResourceLocation("minecraft", "crimson_hanging_sign"),
        ResourceLocation("minecraft", "crimson_wall_hanging_sign"),
        ResourceLocation("minecraft", "crimson_shelf"),
        // 诡异木系列
        ResourceLocation("minecraft", "warped_stem"),
        ResourceLocation("minecraft", "stripped_warped_stem"),
        ResourceLocation("minecraft", "warped_hyphae"),
        ResourceLocation("minecraft", "stripped_warped_hyphae"),
        ResourceLocation("minecraft", "warped_planks"),
        ResourceLocation("minecraft", "warped_slab"),
        ResourceLocation("minecraft", "warped_stairs"),
        ResourceLocation("minecraft", "warped_fence"),
        ResourceLocation("minecraft", "warped_fence_gate"),
        ResourceLocation("minecraft", "warped_door"),
        ResourceLocation("minecraft", "warped_trapdoor"),
        ResourceLocation("minecraft", "warped_button"),
        ResourceLocation("minecraft", "warped_pressure_plate"),
        ResourceLocation("minecraft", "warped_sign"),
        ResourceLocation("minecraft", "warped_wall_sign"),
        ResourceLocation("minecraft", "warped_hanging_sign"),
        ResourceLocation("minecraft", "warped_wall_hanging_sign"),
        ResourceLocation("minecraft", "warped_shelf")});
    tags[nonFlammableWood->getId()] = std::move(nonFlammableWood);

    // 熔岩湖边界石不可替换标签。
    // 数据包 data/minecraft/tags/block/lava_pool_stone_cannot_replace.json 引用三个子标签：
    //   ["#minecraft:features_cannot_replace", "#minecraft:leaves", "#minecraft:logs"]
    // BlockTag 体系是扁平的（unordered_set<ResourceLocation>，不支持 #tag 嵌套引用），故在此把三个
    // 已完整填充的标签内容合并到 lava_pool_stone_cannot_replace。features_cannot_replace（基岩/刷怪笼/
    // 箱子等）、leaves、logs 在上方均已 addAll 完毕，合并顺序无依赖。LakeFeature 边界方块放置时用它
    // 判定哪些固体方块不被熔岩湖边界石覆盖。
    {
        auto lavaPoolStoneCannotReplace =
            std::make_unique<BlockTag>(ResourceLocation("minecraft", "lava_pool_stone_cannot_replace"));
        std::vector<ResourceLocation> merged;
        const auto collect = [&merged](const BlockTag& src) {
            const auto& ids = src.getBlockIds();
            merged.insert(merged.end(), ids.begin(), ids.end());
        };
        collect(*tags.at(ResourceLocation("minecraft", "features_cannot_replace")));
        collect(*tags.at(ResourceLocation("minecraft", "leaves")));
        collect(*tags.at(ResourceLocation("minecraft", "logs")));
        lavaPoolStoneCannotReplace->addAll(merged);
        tags[lavaPoolStoneCannotReplace->getId()] = std::move(lavaPoolStoneCannotReplace);
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

void BlockTags::forEachTag(std::function<void(BlockTag&)> callback)
{
    auto& tags = _getTags();
    for (auto& [id, tag] : tags) {
        callback(*tag);
    }
}

} // namespace mc
