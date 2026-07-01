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

#include "ItemTags.hpp"
#include "common/item/core/ItemRegistry.hpp"

namespace mc {
namespace item::tag {

bool ItemTags::s_initialized = false;

std::unordered_map<ResourceLocation, std::unique_ptr<ItemTag>>& ItemTags::tags()
{
    static std::unordered_map<ResourceLocation, std::unique_ptr<ItemTag>> s_tags;
    return s_tags;
}

ItemTag& ItemTags::registerTag(const ResourceLocation& id)
{
    auto& allTags = tags();

    auto it = allTags.find(id);
    if (it != allTags.end()) {
        return *it->second;
    }

    auto inserted = allTags.emplace(id, std::make_unique<ItemTag>(id, false));
    return *inserted.first->second;
}

ItemTag* ItemTags::getTag(const ResourceLocation& id)
{
    auto& allTags = tags();
    auto it = allTags.find(id);
    if (it == allTags.end()) {
        return nullptr;
    }
    return it->second.get();
}

ItemTag* ItemTags::getTag(const std::string& id)
{
    return getTag(ResourceLocation(id));
}

void ItemTags::forEachTag(std::function<void(ItemTag&)> callback)
{
    for (auto& pair : tags()) {
        callback(*pair.second);
    }
}

// ============================================================================
// 内置物品标签
// ============================================================================

ItemTag& ItemTags::FLOWERS()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "flowers"));
    }
    return *tag;
}

ItemTag& ItemTags::CARPETS()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "carpets"));
    }
    return *tag;
}

ItemTag& ItemTags::DAMPENS_VIBRATIONS()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "dampens_vibrations"));
    }
    return *tag;
}

ItemTag& ItemTags::FIRE_RESISTANT()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "fire_resistant"));
    }
    return *tag;
}

ItemTag& ItemTags::DECORATED_POT_SHERDS()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "decorated_pot_sherds"));
    }
    return *tag;
}

ItemTag& ItemTags::DECORATED_POT_INGREDIENTS()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "decorated_pot_ingredients"));
    }
    return *tag;
}

ItemTag& ItemTags::SWORDS()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "swords"));
    }
    return *tag;
}

ItemTag& ItemTags::AXES()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "axes"));
    }
    return *tag;
}

ItemTag& ItemTags::PICKAXES()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "pickaxes"));
    }
    return *tag;
}

ItemTag& ItemTags::SHOVELS()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "shovels"));
    }
    return *tag;
}

ItemTag& ItemTags::HOES()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "hoes"));
    }
    return *tag;
}

ItemTag& ItemTags::BREAKS_DECORATED_POTS()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "breaks_decorated_pots"));
    }
    return *tag;
}

ItemTag& ItemTags::FREEZE_IMMUNE_WEARABLES()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "freeze_immune_wearables"));
    }
    return *tag;
}

void ItemTags::initialize()
{
    if (s_initialized) {
        return;
    }

    // 物品标签的成员列表从数据包动态加载（ItemTagLoader），此处注册内置标签的默认值。
    // 当数据包可用时，ItemTagLoader 会追加或替换（replace=true）这些默认值；
    // 当数据包不可用时（如单元测试环境），这些硬编码默认值作为回退。
    // 数据包加载路径: data/<namespace>/tags/item/<path>.json
    // 参见 ItemTagLoader.hpp

    auto& allTags = tags();

    // 创建 FLOWERS 标签
    // 包含所有小型花朵和大型花朵
    auto flowers = std::make_unique<ItemTag>(ResourceLocation("minecraft", "flowers"), false);

    // 小型花朵（单格）
    flowers->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "dandelion")));
    flowers->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "poppy")));
    flowers->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "blue_orchid")));
    flowers->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "allium")));
    flowers->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "azure_bluet")));
    flowers->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "red_tulip")));
    flowers->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "orange_tulip")));
    flowers->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "white_tulip")));
    flowers->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "pink_tulip")));
    flowers->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "oxeye_daisy")));
    flowers->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "lily_of_the_valley")));
    flowers->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "cornflower")));
    flowers->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "wither_rose")));

    // 大型花朵（双格）
    flowers->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "sunflower")));
    flowers->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "lilac")));
    flowers->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "rose_bush")));
    flowers->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "peony")));

    allTags[flowers->getId()] = std::move(flowers);

    // 创建 CARPETS 标签
    // 包含所有颜色的地毯物品
    auto carpets = std::make_unique<ItemTag>(ResourceLocation("minecraft", "carpets"), false);

    // 16 色地毯
    carpets->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "white_carpet")));
    carpets->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "orange_carpet")));
    carpets->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "magenta_carpet")));
    carpets->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "light_blue_carpet")));
    carpets->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "yellow_carpet")));
    carpets->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "lime_carpet")));
    carpets->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "pink_carpet")));
    carpets->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "gray_carpet")));
    carpets->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "light_gray_carpet")));
    carpets->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "cyan_carpet")));
    carpets->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "purple_carpet")));
    carpets->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "blue_carpet")));
    carpets->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "brown_carpet")));
    carpets->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "green_carpet")));
    carpets->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "red_carpet")));
    carpets->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "black_carpet")));

    allTags[carpets->getId()] = std::move(carpets);

    // 创建 DAMPENS_VIBRATIONS 标签
    // 包含所有羊毛物品和地毯物品
    auto dampensVibrations = std::make_unique<ItemTag>(ResourceLocation("minecraft", "dampens_vibrations"), false);

    // 16 色羊毛物品
    dampensVibrations->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "white_wool")));
    dampensVibrations->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "orange_wool")));
    dampensVibrations->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "magenta_wool")));
    dampensVibrations->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "light_blue_wool")));
    dampensVibrations->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "yellow_wool")));
    dampensVibrations->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "lime_wool")));
    dampensVibrations->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "pink_wool")));
    dampensVibrations->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "gray_wool")));
    dampensVibrations->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "light_gray_wool")));
    dampensVibrations->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "cyan_wool")));
    dampensVibrations->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "purple_wool")));
    dampensVibrations->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "blue_wool")));
    dampensVibrations->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "brown_wool")));
    dampensVibrations->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "green_wool")));
    dampensVibrations->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "red_wool")));
    dampensVibrations->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "black_wool")));

    // 16 色地毯物品
    dampensVibrations->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "white_carpet")));
    dampensVibrations->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "orange_carpet")));
    dampensVibrations->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "magenta_carpet")));
    dampensVibrations->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "light_blue_carpet")));
    dampensVibrations->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "yellow_carpet")));
    dampensVibrations->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "lime_carpet")));
    dampensVibrations->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "pink_carpet")));
    dampensVibrations->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "gray_carpet")));
    dampensVibrations->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "light_gray_carpet")));
    dampensVibrations->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "cyan_carpet")));
    dampensVibrations->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "purple_carpet")));
    dampensVibrations->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "blue_carpet")));
    dampensVibrations->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "brown_carpet")));
    dampensVibrations->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "green_carpet")));
    dampensVibrations->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "red_carpet")));
    dampensVibrations->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "black_carpet")));

    allTags[dampensVibrations->getId()] = std::move(dampensVibrations);

    // 创建 FIRE_RESISTANT 标签
    // 包含所有防火物品：下界合金锭、下界合金碎片、远古残骸、下界星等
    // 防火物品通过此标签显式列出
    auto fireResistant = std::make_unique<ItemTag>(ResourceLocation("minecraft", "fire_resistant"), false);

    // 下界合金相关物品
    fireResistant->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_ingot")));
    fireResistant->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_scrap")));
    fireResistant->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "ancient_debris")));

    // 下界合金工具
    fireResistant->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_sword")));
    fireResistant->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_shovel")));
    fireResistant->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_pickaxe")));
    fireResistant->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_axe")));
    fireResistant->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_hoe")));

    // 下界合金盔甲
    fireResistant->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_helmet")));
    fireResistant->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_chestplate")));
    fireResistant->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_leggings")));
    fireResistant->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_boots")));

    // 下界星
    fireResistant->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "nether_star")));

    // 下界合金块
    fireResistant->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_block")));

    // 下界合金马铠
    fireResistant->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_horse_armor")));

    allTags[fireResistant->getId()] = std::move(fireResistant);

    // 创建 DECORATED_POT_SHERDS 标签
    // 包含所有陶片物品（1.20 考古学陶片 + 1.21 试炼密室陶片）
    auto sherds = std::make_unique<ItemTag>(ResourceLocation("minecraft", "decorated_pot_sherds"), false);

    // 1.20 考古学陶片（20种）
    sherds->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "angler_pottery_sherd")));
    sherds->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "archer_pottery_sherd")));
    sherds->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "arms_up_pottery_sherd")));
    sherds->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "blade_pottery_sherd")));
    sherds->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "brewer_pottery_sherd")));
    sherds->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "burn_pottery_sherd")));
    sherds->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "danger_pottery_sherd")));
    sherds->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "explorer_pottery_sherd")));
    sherds->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "friend_pottery_sherd")));
    sherds->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "heart_pottery_sherd")));
    sherds->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "heartbreak_pottery_sherd")));
    sherds->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "howl_pottery_sherd")));
    sherds->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "miner_pottery_sherd")));
    sherds->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "mourner_pottery_sherd")));
    sherds->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "plenty_pottery_sherd")));
    sherds->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "prize_pottery_sherd")));
    sherds->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "sheaf_pottery_sherd")));
    sherds->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "shelter_pottery_sherd")));
    sherds->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "skull_pottery_sherd")));
    sherds->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "snort_pottery_sherd")));

    // 1.21 试炼密室陶片（3种）
    sherds->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "flow_pottery_sherd")));
    sherds->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "guster_pottery_sherd")));
    sherds->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "scrape_pottery_sherd")));

    allTags[sherds->getId()] = std::move(sherds);

    // 创建 DECORATED_POT_INGREDIENTS 标签
    // 包含所有陶片 + 砖块，用于饰纹陶罐合成配方
    auto ingredients = std::make_unique<ItemTag>(ResourceLocation("minecraft", "decorated_pot_ingredients"), false);

    // 砖块（作为空白面使用）
    ingredients->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "bricks")));

    // 所有陶片（复用 sherds 标签的内容）
    // 1.20 考古学陶片
    ingredients->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "angler_pottery_sherd")));
    ingredients->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "archer_pottery_sherd")));
    ingredients->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "arms_up_pottery_sherd")));
    ingredients->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "blade_pottery_sherd")));
    ingredients->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "brewer_pottery_sherd")));
    ingredients->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "burn_pottery_sherd")));
    ingredients->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "danger_pottery_sherd")));
    ingredients->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "explorer_pottery_sherd")));
    ingredients->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "friend_pottery_sherd")));
    ingredients->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "heart_pottery_sherd")));
    ingredients->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "heartbreak_pottery_sherd")));
    ingredients->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "howl_pottery_sherd")));
    ingredients->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "miner_pottery_sherd")));
    ingredients->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "mourner_pottery_sherd")));
    ingredients->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "plenty_pottery_sherd")));
    ingredients->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "prize_pottery_sherd")));
    ingredients->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "sheaf_pottery_sherd")));
    ingredients->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "shelter_pottery_sherd")));
    ingredients->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "skull_pottery_sherd")));
    ingredients->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "snort_pottery_sherd")));

    // 1.21 试炼密室陶片
    ingredients->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "flow_pottery_sherd")));
    ingredients->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "guster_pottery_sherd")));
    ingredients->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "scrape_pottery_sherd")));

    allTags[ingredients->getId()] = std::move(ingredients);

    // 创建 SWORDS 标签
    // 包含所有材质的剑物品
    auto swords = std::make_unique<ItemTag>(ResourceLocation("minecraft", "swords"), false);
    swords->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "wooden_sword")));
    swords->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone_sword")));
    swords->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "copper_sword")));
    swords->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "iron_sword")));
    swords->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "golden_sword")));
    swords->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond_sword")));
    swords->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_sword")));
    allTags[swords->getId()] = std::move(swords);

    // 创建 AXES 标签
    // 包含所有材质的斧物品
    auto axes = std::make_unique<ItemTag>(ResourceLocation("minecraft", "axes"), false);
    axes->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "wooden_axe")));
    axes->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone_axe")));
    axes->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "copper_axe")));
    axes->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "iron_axe")));
    axes->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "golden_axe")));
    axes->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond_axe")));
    axes->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_axe")));
    allTags[axes->getId()] = std::move(axes);

    // 创建 PICKAXES 标签
    // 包含所有材质的镐物品
    auto pickaxes = std::make_unique<ItemTag>(ResourceLocation("minecraft", "pickaxes"), false);
    pickaxes->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "wooden_pickaxe")));
    pickaxes->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone_pickaxe")));
    pickaxes->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "copper_pickaxe")));
    pickaxes->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "iron_pickaxe")));
    pickaxes->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "golden_pickaxe")));
    pickaxes->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond_pickaxe")));
    pickaxes->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_pickaxe")));
    allTags[pickaxes->getId()] = std::move(pickaxes);

    // 创建 SHOVELS 标签
    // 包含所有材质的铲物品
    auto shovels = std::make_unique<ItemTag>(ResourceLocation("minecraft", "shovels"), false);
    shovels->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "wooden_shovel")));
    shovels->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone_shovel")));
    shovels->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "copper_shovel")));
    shovels->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "iron_shovel")));
    shovels->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "golden_shovel")));
    shovels->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond_shovel")));
    shovels->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_shovel")));
    allTags[shovels->getId()] = std::move(shovels);

    // 创建 HOES 标签
    // 包含所有材质的锄物品
    auto hoes = std::make_unique<ItemTag>(ResourceLocation("minecraft", "hoes"), false);
    hoes->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "wooden_hoe")));
    hoes->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone_hoe")));
    hoes->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "copper_hoe")));
    hoes->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "iron_hoe")));
    hoes->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "golden_hoe")));
    hoes->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond_hoe")));
    hoes->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_hoe")));
    allTags[hoes->getId()] = std::move(hoes);

    // 创建 BREAKS_DECORATED_POTS 标签
    // 包含所有会破坏饰纹陶罐的物品：工具类型标签 + 三叉戟 + 重锤
    // 手持这些物品破坏陶罐时，陶罐被设为 CRACKED 状态并掉落陶片而非完整陶罐。
    // 精准采集附魔可阻止陶罐碎裂（PREVENTS_DECORATED_POT_SHATTERING 标签）。
    auto breaksPots = std::make_unique<ItemTag>(ResourceLocation("minecraft", "breaks_decorated_pots"), false);

    // 引用工具类型标签
    breaksPots->addAll(SWORDS().getItemsList());
    breaksPots->addAll(AXES().getItemsList());
    breaksPots->addAll(PICKAXES().getItemsList());
    breaksPots->addAll(SHOVELS().getItemsList());
    breaksPots->addAll(HOES().getItemsList());

    // 三叉戟和重锤
    breaksPots->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "trident")));
    breaksPots->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "mace")));

    // 刷子 - 刷扫也会碎裂陶罐
    breaksPots->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "brush")));

    allTags[breaksPots->getId()] = std::move(breaksPots);

    // 创建 FREEZE_IMMUNE_WEARABLES 标签
    // 包含所有使穿戴者免疫冰冻效果的物品。
    // 对应 MC 原版标签 minecraft:freeze_immune_wearables。
    // 穿戴任意一件皮革护甲即可免疫细雪冰冻。
    auto freezeImmuneWearables =
        std::make_unique<ItemTag>(ResourceLocation("minecraft", "freeze_immune_wearables"), false);

    // 皮革护甲
    freezeImmuneWearables->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "leather_helmet")));
    freezeImmuneWearables->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "leather_chestplate")));
    freezeImmuneWearables->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "leather_leggings")));
    freezeImmuneWearables->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "leather_boots")));
    freezeImmuneWearables->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "leather_horse_armor")));

    allTags[freezeImmuneWearables->getId()] = std::move(freezeImmuneWearables);

    s_initialized = true;
}

} // namespace item::tag
} // namespace mc