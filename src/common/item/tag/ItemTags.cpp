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
#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

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

    // 全部标签来自原版数据包（data/minecraft/tags/item 下的标签 JSON），成员与数据包
    // 逐条对应：`#tag` 成员保留为嵌套引用，其余解析成物品。引用展开与缺失成员判定
    // 由 linkAndExpand 统一处理。
    std::size_t entryCount = 0;
    const VanillaItemTagEntry* table = vanillaItemTagTable(entryCount);
    for (std::size_t i = 0; i < entryCount; ++i) {
        const VanillaItemTagEntry& entry = table[i];
        auto tag = std::make_unique<ItemTag>(ResourceLocation("minecraft", entry.name), false);
        for (std::size_t k = 0; k < entry.count; ++k) {
            const std::string_view value = entry.values[k];
            if (!value.empty() && value.front() == '#') {
                tag->addNestedTag(ResourceLocation("minecraft", std::string(value.substr(1))));
            } else if (const Item* item =
                           ItemRegistry::instance().getItem(ResourceLocation("minecraft", std::string(value)));
                item != nullptr) {
                tag->add(item);
            }
        }
        tags()[tag->getId()] = std::move(tag);
    }

    s_initialized = true;

    // 展开引用。内置表里缺失的物品（方块物品稍后才建立）是预期内的：不接管错误结算，
    // 缺口留在标签上，由 takePendingErrors 交给调用方判定。
    if (auto linked = linkAndExpand(); linked.failed()) {
        spdlog::error("ItemTags: {}", linked.error().message());
    }
}

void ItemTags::clear()
{
    tags().clear();
}

Result<nlohmann::json> ItemTags::mergeTagVersions(
    const std::vector<std::string>& versions, const ResourceLocation& location)
{
    nlohmann::json merged;
    merged["values"] = nlohmann::json::array();
    bool replace = false;
    // versions 按优先级从高到低排列，合并顺序是从低到高。
    for (auto it = versions.rbegin(); it != versions.rend(); ++it) {
        auto compiled = compileTagJson(*it, location);
        if (compiled.failed()) {
            return compiled.error();
        }
        const nlohmann::json& contents = compiled.value();
        if (contents.value("replace", false)) {
            merged["values"] = nlohmann::json::array();
            replace = true;
        }
        for (const auto& value : contents["values"]) {
            merged["values"].push_back(value);
        }
    }
    merged["replace"] = replace;
    return merged;
}

Result<nlohmann::json> ItemTags::compileTagJson(const std::string& json, const ResourceLocation& location)
{
    nlohmann::json parsed;
    try {
        parsed = nlohmann::json::parse(json);
    }
    catch (const nlohmann::json::parse_error& error) {
        return Error(ErrorCode::InvalidData,
            "Failed to parse item tag '" + location.toString() + "': " + std::string(error.what()));
    }

    if (!parsed.contains("values") || !parsed["values"].is_array()) {
        return Error(ErrorCode::InvalidData, "Item tag '" + location.toString() + "' is missing a 'values' array");
    }

    nlohmann::json compiled;
    compiled["replace"] = parsed.contains("replace") && parsed["replace"].is_boolean() && parsed["replace"].get<bool>();
    compiled["values"] = nlohmann::json::array();
    for (const auto& value : parsed["values"]) {
        if (value.is_string()) {
            compiled["values"].push_back(value);
            continue;
        }
        if (value.is_object() && value.contains("id") && value["id"].is_string()) {
            nlohmann::json member;
            member["id"] = value["id"];
            member["required"] =
                !(value.contains("required") && value["required"].is_boolean()) || value["required"].get<bool>();
            compiled["values"].push_back(std::move(member));
            continue;
        }
        spdlog::warn("ItemTags: tag '{}' has a value that is neither an item id nor an object with 'id', skipping",
            location.toString());
    }
    return compiled;
}

Result<void> ItemTags::applyTagJson(const nlohmann::json& contents, const ResourceLocation& location)
{
    ItemTag* existing = getTag(location);
    if (existing == nullptr) {
        static_cast<void>(registerTag(location));
        existing = getTag(location);
    }
    if (existing == nullptr) {
        return Error(ErrorCode::InvalidState, "Could not register item tag '" + location.toString() + "'");
    }
    return writeTagJson(contents, *existing);
}

Result<void> ItemTags::applyTagJson(const nlohmann::json& contents, ItemTag& tag)
{
    return writeTagJson(contents, tag);
}

Result<void> ItemTags::writeTagJson(const nlohmann::json& contents, ItemTag& tag)
{
    // 与 TagLoader 一致：有未注册的必需成员时整条标签不写入（`replace` 也不会清空
    // 已有内容）。缺口记在标签上，由 `linkAndExpand` / `takePendingErrors` 判定。
    std::vector<std::string> missing;
    for (const auto& value : contents["values"]) {
        const bool required = !value.is_object() || value.value("required", true);
        const std::string id = value.is_string() ? value.get<std::string>() : value["id"].get<std::string>();
        if (!required || id.empty() || id.front() == '#') {
            continue;
        }
        if (ItemRegistry::instance().getItem(ResourceLocation::parse(id)) == nullptr) {
            missing.push_back(id);
        }
    }
    if (!missing.empty()) {
        for (auto& id : missing) {
            tag.addMissingItem(id);
        }
        return {};
    }

    if (contents.value("replace", false)) {
        tag.clear();
    }

    for (const auto& value : contents["values"]) {
        const std::string id = value.is_string() ? value.get<std::string>() : value["id"].get<std::string>();
        if (id.empty()) {
            continue;
        }
        if (id.front() == '#') {
            const std::string ref = id.substr(1);
            tag.addNestedTag(
                ref.find(':') == std::string::npos ? ResourceLocation("minecraft", ref) : ResourceLocation::parse(ref));
            continue;
        }
        // 非必需且未注册的成员直接跳过（原版 required=false 的语义）。
        if (const Item* item = ItemRegistry::instance().getItem(ResourceLocation::parse(id)); item != nullptr) {
            tag.add(item);
        }
    }
    return {};
}

Result<void> ItemTags::linkAndExpand()
{
    // 第一遍：引用不存在的标签。未注册的必需成员不在这里判定——内置表初始化时方块
    // 物品还没建立，那些缺口是预期内的，交给 `takePendingErrors`。
    for (const auto& [id, tag] : tags()) {
        for (const auto& nested : tag->getNestedTags()) {
            if (tags().find(nested) == tags().end()) {
                return Error(ErrorCode::NotFound,
                    "Item tag '" + id.toString() + "' references unknown tag '" + nested.toString() + "'");
            }
        }
    }

    // 第二遍：算展开后的成员集合（不动标签本身）。深度优先、按路径检测环——
    // 引用自己的标签是空操作（自身的物品已经在集合里），不算环。
    std::unordered_map<const ItemTag*, std::unordered_set<const Item*>> expanded;
    std::unordered_set<const ItemTag*> visiting;
    const auto expand = [&](const auto& self, ItemTag* tag) -> Result<void> {
        if (expanded.contains(tag)) {
            return {};
        }
        if (visiting.contains(tag)) {
            // 引用自己在路径上（含直接自引用）：自身物品已在集合里，当作空展开。
            return {};
        }
        visiting.insert(tag);
        std::unordered_set<const Item*> members = tag->getItems();
        for (const auto& nested : tag->getNestedTags()) {
            ItemTag* source = tags().at(nested).get();
            if (auto result = self(self, source); result.failed()) {
                return result.error();
            }
            if (visiting.contains(source)) {
                // 被引用方正在展开（环）：它的成员尚未确定，但已有物品已经计入。
                continue;
            }
            const auto& sourceMembers = expanded.at(source);
            members.insert(sourceMembers.begin(), sourceMembers.end());
        }
        visiting.erase(tag);
        expanded[tag] = std::move(members);
        return {};
    };

    for (auto& [unused, tag] : tags()) {
        if (auto result = expand(expand, tag.get()); result.failed()) {
            return result.error();
        }
    }

    // 第三遍：把展开结果并回标签，清掉已消费的引用（重复调用不会重复展开）。
    for (auto& [unused, tag] : tags()) {
        tag->addAll(std::vector<const Item*>(expanded.at(tag.get()).begin(), expanded.at(tag.get()).end()));
        tag->clearNestedTags();
        static_cast<void>(tag->takeMissingItems());
    }
    return {};
}

void ItemTags::takePendingErrors(std::vector<std::string>& errors)
{
    for (auto& [id, tag] : tags()) {
        const auto missing = tag->takeMissingItems();
        if (!missing.empty()) {
            errors.push_back("Item tag '" + id.toString() + "' references unknown item '" + missing.front() + "'");
        }
    }
}

} // namespace item::tag
} // namespace mc
