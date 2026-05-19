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
#include "BlockRegistry.hpp"

namespace mc {

// ============================================================================
// BlockTag Implementation
// ============================================================================

BlockTag::BlockTag(ResourceLocation id)
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

bool BlockTag::contains(const ResourceLocation& blockId) const
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

std::unordered_map<ResourceLocation, std::unique_ptr<BlockTag>>& BlockTags::getTags()
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

BlockTag& BlockTags::WOOL()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "wool"));
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

BlockTag& BlockTags::BEEHIVES()
{
    static BlockTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "beehives"));
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

void BlockTags::initialize()
{
    if (s_initialized) {
        return;
    }

    auto& tags = getTags();

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
    crimsonStems->addAll(
        {ResourceLocation("minecraft", "crimson_stem"), ResourceLocation("minecraft", "stripped_crimson_stem")});
    tags[crimsonStems->getId()] = std::move(crimsonStems);

    // 创建 WARPED_STEMS 标签
    auto warpedStems = std::make_unique<BlockTag>(ResourceLocation("minecraft", "warped_stems"));
    warpedStems->addAll(
        {ResourceLocation("minecraft", "warped_stem"), ResourceLocation("minecraft", "stripped_warped_stem")});
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
        ResourceLocation("minecraft", "mycelium"),
        ResourceLocation("minecraft", "farmland")});
    tags[dirt->getId()] = std::move(dirt);

    // 创建 SAND 标签
    auto sand = std::make_unique<BlockTag>(ResourceLocation("minecraft", "sand"));
    sand->addAll({ResourceLocation("minecraft", "sand"),
        ResourceLocation("minecraft", "red_sand"),
        ResourceLocation("minecraft", "soul_sand")});
    tags[sand->getId()] = std::move(sand);

    // 创建 STONE 标签（MC 1.16.5 stone 标签仅包含 stone 方块）
    auto stone = std::make_unique<BlockTag>(ResourceLocation("minecraft", "stone"));
    stone->addAll({ResourceLocation("minecraft", "stone"),
        ResourceLocation("minecraft", "granite"),
        ResourceLocation("minecraft", "polished_granite"),
        ResourceLocation("minecraft", "diorite"),
        ResourceLocation("minecraft", "polished_diorite"),
        ResourceLocation("minecraft", "andesite"),
        ResourceLocation("minecraft", "polished_andesite")});
    tags[stone->getId()] = std::move(stone);

    // 创建 FIRE 标签
    auto fire = std::make_unique<BlockTag>(ResourceLocation("minecraft", "fire"));
    fire->addAll({ResourceLocation("minecraft", "fire"), ResourceLocation("minecraft", "soul_fire")});
    tags[fire->getId()] = std::move(fire);

    // 创建 SOUL_FIRE_BASE_BLOCKS 标签（灵魂火基座方块）
    // 参考 MC 1.16.5: Blocks.SOUL_SAND, Blocks.SOUL_SOIL
    auto soulFireBaseBlocks = std::make_unique<BlockTag>(ResourceLocation("minecraft", "soul_fire_base_blocks"));
    soulFireBaseBlocks->addAll(
        {ResourceLocation("minecraft", "soul_sand"), ResourceLocation("minecraft", "soul_soil")});
    tags[soulFireBaseBlocks->getId()] = std::move(soulFireBaseBlocks);

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

    // 创建 BAMBOO_PLANTABLE_ON 标签
    auto bambooPlantableOn = std::make_unique<BlockTag>(ResourceLocation("minecraft", "bamboo_plantable_on"));
    bambooPlantableOn->addAll({ResourceLocation("minecraft", "grass_block"),
        ResourceLocation("minecraft", "dirt"),
        ResourceLocation("minecraft", "coarse_dirt"),
        ResourceLocation("minecraft", "podzol"),
        ResourceLocation("minecraft", "farmland"),
        ResourceLocation("minecraft", "sand"),
        ResourceLocation("minecraft", "red_sand"),
        ResourceLocation("minecraft", "gravel"),
        ResourceLocation("minecraft", "bamboo"),
        ResourceLocation("minecraft", "bamboo_sapling")});
    tags[bambooPlantableOn->getId()] = std::move(bambooPlantableOn);

    // 创建 VALID_SWEET_BERRY_BUSH_GROUND 标签
    // 参考 MC 1.16.5 SweetBerryBushBlock.isValidGround()
    // Blocks.GRASS_BLOCK, Blocks.DIRT, Blocks.COARSE_DIRT, Blocks.PODZOL, Blocks.FARMLAND
    auto sweetBerryBushGround =
        std::make_unique<BlockTag>(ResourceLocation("minecraft", "valid_sweet_berry_bush_ground"));
    sweetBerryBushGround->addAll({ResourceLocation("minecraft", "grass_block"),
        ResourceLocation("minecraft", "dirt"),
        ResourceLocation("minecraft", "coarse_dirt"),
        ResourceLocation("minecraft", "podzol"),
        ResourceLocation("minecraft", "farmland")});
    tags[sweetBerryBushGround->getId()] = std::move(sweetBerryBushGround);

    // 创建 WALL_CORALS 标签（墙珊瑚扇）
    // 参考 MC 1.16.5 BlockTags.WALL_CORALS
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
    // 参考 MC 1.16.5 BlockTags.UNDERWATER_BONEMEALS
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
    // 参考 MC 1.16.5 BlockTags.STRIDER_WARM_BLOCKS
    // 只包含熔岩方块
    auto striderWarmBlocks = std::make_unique<BlockTag>(ResourceLocation("minecraft", "strider_warm_blocks"));
    striderWarmBlocks->addAll({ResourceLocation("minecraft", "lava")});
    tags[striderWarmBlocks->getId()] = std::move(striderWarmBlocks);

    // 创建 SMALL_FLOWERS 标签（小花朵）
    // 参考 MC 1.16.5: BlockTags.SMALL_FLOWERS
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
    // 参考 MC 1.16.5: BlockTags.TALL_FLOWERS
    auto tallFlowers = std::make_unique<BlockTag>(ResourceLocation("minecraft", "tall_flowers"));
    tallFlowers->addAll({ResourceLocation("minecraft", "sunflower"),
        ResourceLocation("minecraft", "lilac"),
        ResourceLocation("minecraft", "rose_bush"),
        ResourceLocation("minecraft", "peony")});
    tags[tallFlowers->getId()] = std::move(tallFlowers);

    // 创建 BEEHIVES 标签（蜂巢/蜂箱）
    // 参考 MC 1.16.5: BlockTags.BEEHIVES
    auto beehives = std::make_unique<BlockTag>(ResourceLocation("minecraft", "beehives"));
    beehives->addAll({ResourceLocation("minecraft", "beehive"), ResourceLocation("minecraft", "bee_nest")});
    tags[beehives->getId()] = std::move(beehives);

    // 创建 BEE_GROWABLES 标签（蜜蜂可授粉作物）
    // 参考 MC 1.16.5: BlockTags.BEE_GROWABLES
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
        ResourceLocation("minecraft", "sweet_berry_bush")});
    tags[beeGrowables->getId()] = std::move(beeGrowables);

    // 创建 ENDERMAN_HOLDABLE 标签（末影人可拾取方块）
    // 参考 MC 1.16.5: BlockTags.ENDERMAN_HOLDABLE
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
        ResourceLocation("minecraft", "stripped_warped_stem")});

    // 创建 WITHER_IMMUNE 标签（凋灵免疫方块）
    // 参考 MC 1.16.5: BlockTags.WITHER_IMMUNE
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
        ResourceLocation("minecraft", "light")});
    tags[witherImmune->getId()] = std::move(witherImmune);

    s_initialized = true;
}

BlockTag* BlockTags::getTag(const ResourceLocation& id)
{
    auto& tags = getTags();
    auto it = tags.find(id);
    if (it != tags.end()) {
        return it->second.get();
    }
    return nullptr;
}

void BlockTags::forEachTag(std::function<void(BlockTag&)> callback)
{
    auto& tags = getTags();
    for (auto& [id, tag] : tags) {
        callback(*tag);
    }
}

} // namespace mc
