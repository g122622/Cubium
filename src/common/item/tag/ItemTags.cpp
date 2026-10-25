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
#include "common/item/tag/ItemTag.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

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

ItemTag& ItemTags::BEDS()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "beds"));
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

ItemTag& ItemTags::SKELETON_PREFERRED_WEAPONS()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "skeleton_preferred_weapons"));
    }
    return *tag;
}

ItemTag& ItemTags::PILLAGER_PREFERRED_WEAPONS()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "pillager_preferred_weapons"));
    }
    return *tag;
}

ItemTag& ItemTags::DROWNED_PREFERRED_WEAPONS()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "drowned_preferred_weapons"));
    }
    return *tag;
}

ItemTag& ItemTags::PIGLIN_PREFERRED_WEAPONS()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "piglin_preferred_weapons"));
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

ItemTag& ItemTags::SPEARS()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "spears"));
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

ItemTag& ItemTags::CHAINS()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "chains"));
    }
    return *tag;
}

ItemTag& ItemTags::BARS()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "bars"));
    }
    return *tag;
}

ItemTag& ItemTags::WOODEN_DOORS()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "wooden_doors"));
    }
    return *tag;
}

ItemTag& ItemTags::DOORS()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "doors"));
    }
    return *tag;
}

ItemTag& ItemTags::WOODEN_TRAPDOORS()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "wooden_trapdoors"));
    }
    return *tag;
}

ItemTag& ItemTags::TRAPDOORS()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "trapdoors"));
    }
    return *tag;
}

ItemTag& ItemTags::NON_FLAMMABLE_WOOD()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "non_flammable_wood"));
    }
    return *tag;
}

ItemTag& ItemTags::SHULKER_BOXES()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "shulker_boxes"));
    }
    return *tag;
}

ItemTag& ItemTags::HARNESSES()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "harnesses"));
    }
    return *tag;
}

ItemTag& ItemTags::BUNDLES()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "bundles"));
    }
    return *tag;
}

ItemTag& ItemTags::REPAIRS_WOLF_ARMOR()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "repairs_wolf_armor"));
    }
    return *tag;
}

ItemTag& ItemTags::DYEABLE()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "dyeable"));
    }
    return *tag;
}

ItemTag& ItemTags::GOLD_ORES()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "gold_ores"));
    }
    return *tag;
}

ItemTag& ItemTags::COPPER_GOLEM_STATUES()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "copper_golem_statues"));
    }
    return *tag;
}

ItemTag& ItemTags::SHEARABLE_FROM_COPPER_GOLEM()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "shearable_from_copper_golem"));
    }
    return *tag;
}

ItemTag& ItemTags::COPPER_CHESTS()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "copper_chests"));
    }
    return *tag;
}

ItemTag& ItemTags::PIGLIN_LOVED()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "piglin_loved"));
    }
    return *tag;
}

ItemTag& ItemTags::VILLAGER_PLANTABLE_SEEDS()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "villager_plantable_seeds"));
    }
    return *tag;
}

ItemTag& ItemTags::SNIFFER_FOOD()
{
    // 首次访问时构建：本标签不加入 initialize() 的内置建表流程，避免在物品尚未注册时
    // 只生成空标签；同时保证访问点必然发生在物品注册之后。
    static std::once_flag once;
    std::call_once(once, [] {
        auto tag = std::make_unique<ItemTag>(ResourceLocation("minecraft", "sniffer_food"), false);
        tag->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "torchflower_seeds")));
        tag->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "pitcher_pod")));
        tags()[tag->getId()] = std::move(tag);
    });
    ItemTag* tag = getTag(ResourceLocation("minecraft", "sniffer_food"));
    MC_ASSERT_RELEASE_MSG(tag != nullptr, "item tag 'sniffer_food' must be built on first access");
    return *tag;
}

ItemTag& ItemTags::BEE_FOOD()
{
    // 首次访问时构建（同 SNIFFER_FOOD 的惰性建表范式），对齐 vanilla
    // ItemTags.BEE_FOOD = dandelion / open_eyeblossom / poppy / torchflower / pitcher_plant / pink_petals ...
    static std::once_flag once;
    std::call_once(once, [] {
        auto tag = std::make_unique<ItemTag>(ResourceLocation("minecraft", "bee_food"), false);
        // 成员与数据包 minecraft:bee_food 完全一致（29 项，含樱花树叶/野花等非花物品）。
        for (const char* id : {"dandelion",
                 "open_eyeblossom",
                 "poppy",
                 "blue_orchid",
                 "allium",
                 "azure_bluet",
                 "red_tulip",
                 "orange_tulip",
                 "white_tulip",
                 "pink_tulip",
                 "oxeye_daisy",
                 "cornflower",
                 "lily_of_the_valley",
                 "wither_rose",
                 "torchflower",
                 "sunflower",
                 "lilac",
                 "peony",
                 "rose_bush",
                 "pitcher_plant",
                 "flowering_azalea_leaves",
                 "flowering_azalea",
                 "mangrove_propagule",
                 "cherry_leaves",
                 "pink_petals",
                 "wildflowers",
                 "chorus_flower",
                 "spore_blossom",
                 "cactus_flower"}) {
            tag->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", id)));
        }
        tags()[tag->getId()] = std::move(tag);
    });
    ItemTag* tag = getTag(ResourceLocation("minecraft", "bee_food"));
    MC_ASSERT_RELEASE_MSG(tag != nullptr, "item tag 'bee_food' must be built on first access");
    return *tag;
}

ItemTag& ItemTags::FROG_FOOD()
{
    // 首次访问时构建：本标签不加入 initialize() 的内置建表流程，避免在物品尚未注册时
    // 只生成空标签；同时保证访问点必然发生在物品注册之后。
    static std::once_flag once;
    std::call_once(once, [] {
        auto tag = std::make_unique<ItemTag>(ResourceLocation("minecraft", "frog_food"), false);
        tag->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "slime_ball")));
        tags()[tag->getId()] = std::move(tag);
    });
    ItemTag* tag = getTag(ResourceLocation("minecraft", "frog_food"));
    MC_ASSERT_RELEASE_MSG(tag != nullptr, "item tag 'frog_food' must be built on first access");
    return *tag;
}

ItemTag& ItemTags::CAMEL_FOOD()
{
    // 首次访问时构建：本标签不加入 initialize() 的内置建表流程，避免在物品尚未注册时
    // 只生成空标签；同时保证访问点必然发生在物品注册之后。
    static std::once_flag once;
    std::call_once(once, [] {
        auto tag = std::make_unique<ItemTag>(ResourceLocation("minecraft", "camel_food"), false);
        tag->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "cactus")));
        tags()[tag->getId()] = std::move(tag);
    });
    ItemTag* tag = getTag(ResourceLocation("minecraft", "camel_food"));
    MC_ASSERT_RELEASE_MSG(tag != nullptr, "item tag 'camel_food' must be built on first access");
    return *tag;
}

ItemTag& ItemTags::CAMEL_HUSK_FOOD()
{
    // 首次访问时构建：本标签不加入 initialize() 的内置建表流程，避免在物品尚未注册时
    // 只生成空标签；同时保证访问点必然发生在物品注册之后。
    static std::once_flag once;
    std::call_once(once, [] {
        auto tag = std::make_unique<ItemTag>(ResourceLocation("minecraft", "camel_husk_food"), false);
        tag->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "rabbit_foot")));
        tags()[tag->getId()] = std::move(tag);
    });
    ItemTag* tag = getTag(ResourceLocation("minecraft", "camel_husk_food"));
    MC_ASSERT_RELEASE_MSG(tag != nullptr, "item tag 'camel_husk_food' must be built on first access");
    return *tag;
}

ItemTag& ItemTags::HAPPY_GHAST_FOOD()
{
    // 首次访问时构建：本标签不加入 initialize() 的内置建表流程，避免在物品尚未注册时
    // 只生成空标签；同时保证访问点必然发生在物品注册之后。
    static std::once_flag once;
    std::call_once(once, [] {
        auto tag = std::make_unique<ItemTag>(ResourceLocation("minecraft", "happy_ghast_food"), false);
        tag->add(ItemRegistry::instance().getItem(ResourceLocation("minecraft", "snowball")));
        tags()[tag->getId()] = std::move(tag);
    });
    ItemTag* tag = getTag(ResourceLocation("minecraft", "happy_ghast_food"));
    MC_ASSERT_RELEASE_MSG(tag != nullptr, "item tag 'happy_ghast_food' must be built on first access");
    return *tag;
}

ItemTag& ItemTags::HAPPY_GHAST_TEMPT_ITEMS()
{
    // 首次访问时构建：原版内容为 #happy_ghast_food + #harnesses，此处展开为具体物品成员。
    static std::once_flag once;
    std::call_once(once, [] {
        auto tag = std::make_unique<ItemTag>(ResourceLocation("minecraft", "happy_ghast_tempt_items"), false);
        for (const auto* item : ItemTags::HAPPY_GHAST_FOOD().getItems()) {
            tag->add(item);
        }
        if (const ItemTag* harnesses = getTag(ResourceLocation("minecraft", "harnesses")); harnesses != nullptr) {
            for (const auto* item : harnesses->getItems()) {
                tag->add(item);
            }
        }
        tags()[tag->getId()] = std::move(tag);
    });
    ItemTag* tag = getTag(ResourceLocation("minecraft", "happy_ghast_tempt_items"));
    MC_ASSERT_RELEASE_MSG(tag != nullptr, "item tag 'happy_ghast_tempt_items' must be built on first access");
    return *tag;
}

ItemTag& ItemTags::CREEPER_IGNITERS()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "creeper_igniters"));
    }
    return *tag;
}

ItemTag& ItemTags::WOODEN_SHELVES()
{
    static ItemTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "wooden_shelves"));
    }
    return *tag;
}

ItemTag& ItemTags::BEACON_PAYMENT_ITEMS()
{
    return registerTag(ResourceLocation("minecraft:beacon_payment_items"));
}

ItemTag& ItemTags::NOTE_BLOCK_TOP_INSTRUMENTS()
{
    return registerTag(ResourceLocation("minecraft:noteblock_top_instruments"));
}

void ItemTags::initialize()
{
    if (s_initialized) {
        return;
    }

    // 全部标签来自原版数据包（data/minecraft/tags/item 下的标签 JSON），成员已在
    // 生成期展开 #tag 引用，等价原版 Tag#getValues() 的口径。以前这里逐个手写
    // item->add(...)，与数据包之间会悄悄漂移。
    std::size_t entryCount = 0;
    const VanillaItemTagEntry* table = vanillaItemTagTable(entryCount);
    for (std::size_t i = 0; i < entryCount; ++i) {
        const VanillaItemTagEntry& entry = table[i];
        auto tag = std::make_unique<ItemTag>(ResourceLocation("minecraft", entry.name), false);
        for (std::size_t k = 0; k < entry.count; ++k) {
            if (const Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", entry.values[k]));
                item != nullptr) {
                tag->add(item);
            }
        }
        tags()[tag->getId()] = std::move(tag);
    }

    s_initialized = true;
}

} // namespace item::tag
} // namespace mc