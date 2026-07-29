/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
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

#include "server/network/EnchantmentNbtBuilder.hpp"

#include "common/item/core/Item.hpp"
#include "common/item/tag/ItemTag.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/network/buffer/NbtIo.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/nbt/NbtJsonUtils.hpp"

#include <spdlog/spdlog.h>

#include <nlohmann/json.hpp>

#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>

namespace mc::server::net {

namespace {

/// 进程级 datapack 源（服务器启动期由 setEnchantmentDatapackSource 注册）。
/// 与 MinecraftServer::m_dataPackList 同生命周期，握手阶段调用安全。
const mc::resource::DataPackRepository* g_datapackRepo = nullptr;

/// enchantment 发送顺序（与原 RegistryDataBuilder 硬编码列表严格一致）。
/// 顺序固定勿乱：enchantment 名字虽按 name 解码，但保留顺序以兼容未来 UpdateTags int id 映射。
const std::vector<std::string_view> kEnchantmentIds = {
    "minecraft:aqua_affinity",
    "minecraft:bane_of_arthropods",
    "minecraft:binding_curse",
    "minecraft:blast_protection",
    "minecraft:breach",
    "minecraft:channeling",
    "minecraft:density",
    "minecraft:depth_strider",
    "minecraft:efficiency",
    "minecraft:feather_falling",
    "minecraft:fire_aspect",
    "minecraft:fire_protection",
    "minecraft:flame",
    "minecraft:fortune",
    "minecraft:frost_walker",
    "minecraft:impaling",
    "minecraft:infinity",
    "minecraft:knockback",
    "minecraft:looting",
    "minecraft:loyalty",
    "minecraft:luck_of_the_sea",
    "minecraft:lure",
    "minecraft:mending",
    "minecraft:multishot",
    "minecraft:piercing",
    "minecraft:power",
    "minecraft:projectile_protection",
    "minecraft:protection",
    "minecraft:punch",
    "minecraft:quick_charge",
    "minecraft:respiration",
    "minecraft:riptide",
    "minecraft:sharpness",
    "minecraft:silk_touch",
    "minecraft:smite",
    "minecraft:soul_speed",
    "minecraft:sweeping_edge",
    "minecraft:swift_sneak",
    "minecraft:thorns",
    "minecraft:unbreaking",
    "minecraft:vanishing_curse",
    "minecraft:wind_burst",
    "minecraft:lunge",
};

/// 把 "minecraft:sharpness" → "minecraft/enchantment/sharpness.json"
//
// 路径约定：readTextResource(PackType::ServerData, path) 经 FolderResourcePack 预置
// packTypeDirectoryName(ServerData)="data" 前缀，最终落盘 root/data/<path>。故此处 path
// 不含 "data/" 前缀（与 ItemTagLoader 用 namespace+"/tags/item" 同一约定，见 ItemTagLoader.cpp）。
// 误加 "data/" 会双重前缀（root/data/data/...）致 readTextResource 永久 ResourceNotFound。
std::string enchantmentIdToResourcePath(std::string_view id)
{
    // id 形如 "namespace:path"
    const auto colon = id.find(':');
    if (colon == std::string_view::npos) {
        return std::string("minecraft/enchantment/") + std::string(id) + ".json";
    }
    std::string ns(id.substr(0, colon));
    std::string path(id.substr(colon + 1));
    return ns + "/enchantment/" + path + ".json";
}

/// 把 "#minecraft:exclusive_set/damage" → "minecraft/tags/enchantment/exclusive_set/damage.json"
// 同 enchantmentIdToResourcePath：不含 "data/" 前缀（FolderResourcePack 预置）。
std::string enchantmentTagRefToResourcePath(std::string_view tagRef)
{
    // tagRef 形如 "#namespace:path"（已剥 # 后调用亦可）
    std::string_view ref = tagRef;
    if (!ref.empty() && ref[0] == '#') {
        ref = ref.substr(1);
    }
    const auto colon = ref.find(':');
    std::string ns = (colon == std::string_view::npos) ? std::string("minecraft") : std::string(ref.substr(0, colon));
    std::string path = (colon == std::string_view::npos) ? std::string(ref) : std::string(ref.substr(colon + 1));
    return ns + "/tags/enchantment/" + path + ".json";
}

/// 从 datapack 读文本资源；失败返回空串并记日志。
std::string readDataPackText(const mc::resource::DataPackRepository& repo, const std::string& resourcePath)
{
    auto r = repo.readTextResource(resourcePath);
    if (!r.success()) {
        spdlog::warn("EnchantmentNbtBuilder: readTextResource failed: {} ({})", resourcePath, r.error().toString());
        return {};
    }
    return std::move(r).value();
}

/// 把 HolderSet 引用展平为元素名列表。
/// "namespace:path" 或单元素名 → 直接返回该名字；
/// "#namespace:path" → 优先 ItemTags::getTag（已递归展平嵌套 #），未命中则直读
///   tags/enchantment/*.json 的 values 数组（vanilla exclusive_set 仅含名字，无嵌套 #）。
std::vector<std::string> flattenHolderSet(const mc::resource::DataPackRepository& repo, const std::string& holderSetVal)
{
    std::vector<std::string> result;
    if (holderSetVal.empty()) {
        return result;
    }
    if (holderSetVal[0] != '#') {
        // 单个元素名（HolderSetCodec Either.right 的单元素列表形式）
        result.push_back(holderSetVal);
        return result;
    }
    // tag 引用：剥 # 得 ResourceLocation
    const std::string refStr = holderSetVal.substr(1);
    const mc::ResourceLocation loc = mc::ResourceLocation::parse(refStr);

    // 优先走 ItemTags（启动期已递归解析嵌套 # 引用，无运行时 IO）
    if (auto* itemTag = mc::item::tag::ItemTags::getTag(loc)) {
        for (const mc::Item* item : itemTag->getItems()) {
            result.push_back(item->itemLocation().toString());
        }
        return result;
    }

    // 回退：直读 tags/enchantment/<...>.json（enchantment exclusive_set 等未在服务端注册的标签）
    const std::string tagPath = enchantmentTagRefToResourcePath(holderSetVal);
    const std::string text = readDataPackText(repo, tagPath);
    if (text.empty()) {
        spdlog::warn(
            "EnchantmentNbtBuilder: tag {} not in ItemTags and datapack file missing ({})", holderSetVal, tagPath);
        return result;
    }
    try {
        const auto j = nlohmann::json::parse(text);
        if (j.contains("values") && j["values"].is_array()) {
            for (const auto& v : j["values"]) {
                if (v.is_string()) {
                    // vanilla exclusive_set 仅含 enchantment 名字；遇嵌套 # 记 warn 跳过
                    // （完整 # 解析需 EnchantmentTagLoader，本任务不做，见 plan 风险 4）
                    const std::string s = v.get<std::string>();
                    if (!s.empty() && s[0] == '#') {
                        spdlog::warn("EnchantmentNbtBuilder: nested tag ref {} in {} not resolved", s, tagPath);
                        continue;
                    }
                    result.push_back(s);
                }
            }
        }
    }
    catch (const std::exception& e) {
        spdlog::warn("EnchantmentNbtBuilder: parse tag json {} failed: {}", tagPath, e.what());
    }
    return result;
}

/// 构造字符串列表 tag（HolderSet 名字列表 / slots 列表的线格式）。
std::unique_ptr<mc::nbt::tags::tag_list_tag> makeStringList(const std::vector<std::string>& names)
{
    auto list = std::make_unique<mc::nbt::tags::tag_list_tag>(mc::nbt::TagId::String);
    for (const auto& name : names) {
        list->value.push_back(std::make_unique<mc::nbt::tags::string_tag>(name));
    }
    return list;
}

/// 构造 Cost 子 compound（base + per_level_above_first 均为 int_tag，对齐 Java Enchantment.Cost CODEC）。
/// 用 put(name, i32) 走 tag_of<int32_t>=int_tag 推断（项目约定，见 test_nbt_io.cpp 注释：
/// 勿用 put<int_tag>，find_of<int_tag> 未特化）。
std::unique_ptr<mc::nbt::tags::compound_tag> makeCostCompound(const nlohmann::json& costJson)
{
    auto cost = std::make_unique<mc::nbt::tags::compound_tag>();
    if (costJson.contains("base")) {
        cost->put("base", static_cast<i32>(costJson["base"].get<std::int32_t>()));
    }
    if (costJson.contains("per_level_above_first")) {
        cost->put("per_level_above_first", static_cast<i32>(costJson["per_level_above_first"].get<std::int32_t>()));
    }
    return cost;
}

/// 构造单个 enchantment 的内联 NBT 字节（Java 根 NBT 线格式）。
/// 失败返回 nullopt（调用方据此回退 data=nullopt 或跳过）。
std::optional<std::vector<u8>> buildEnchantmentEntryData(
    const mc::resource::DataPackRepository& repo, std::string_view id)
{
    const std::string resourcePath = enchantmentIdToResourcePath(id);
    const std::string text = readDataPackText(repo, resourcePath);
    if (text.empty()) {
        return std::nullopt;
    }
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(text);
    }
    catch (const std::exception& e) {
        spdlog::warn("EnchantmentNbtBuilder: parse {} failed: {}", resourcePath, e.what());
        return std::nullopt;
    }

    auto root = std::make_unique<mc::nbt::tags::compound_tag>();

    // 顶层整数字段：显式 i32 → int_tag 推断（关键）。jsonToNbt 会把 5 推断为 byte_tag，
    // Java Enchantment.intRange/int CODEC 要求 int_tag，拒绝 byte_tag/short_tag。
    // 用 put(name, i32) 走 tag_of<int32_t>=int_tag 推断（勿用 put<int_tag>，见 test_nbt_io 注释）。
    if (j.contains("anvil_cost")) {
        root->put("anvil_cost", static_cast<i32>(j["anvil_cost"].get<std::int32_t>()));
    }
    if (j.contains("max_level")) {
        root->put("max_level", static_cast<i32>(j["max_level"].get<std::int32_t>()));
    }
    if (j.contains("weight")) {
        root->put("weight", static_cast<i32>(j["weight"].get<std::int32_t>()));
    }

    // Cost 子 compound（base/per_level_above_first 均 int_tag）
    if (j.contains("min_cost")) {
        root->value.emplace("min_cost", makeCostCompound(j["min_cost"]));
    }
    if (j.contains("max_cost")) {
        root->value.emplace("max_cost", makeCostCompound(j["max_cost"]));
    }

    // slots：字符串列表
    if (j.contains("slots") && j["slots"].is_array()) {
        std::vector<std::string> slots;
        for (const auto& s : j["slots"]) {
            if (s.is_string()) {
                slots.push_back(s.get<std::string>());
            }
        }
        root->value.emplace("slots", makeStringList(slots));
    }

    // HolderSet 字段：展平 #tag → 显式名字列表（绕开 lookupTag 与未绑定 Named）
    if (j.contains("supported_items") && j["supported_items"].is_string()) {
        const auto names = flattenHolderSet(repo, j["supported_items"].get<std::string>());
        root->value.emplace("supported_items", makeStringList(names));
    }
    if (j.contains("primary_items") && j["primary_items"].is_string()) {
        const auto names = flattenHolderSet(repo, j["primary_items"].get<std::string>());
        root->value.emplace("primary_items", makeStringList(names));
    }
    if (j.contains("exclusive_set") && j["exclusive_set"].is_string()) {
        const auto names = flattenHolderSet(repo, j["exclusive_set"].get<std::string>());
        root->value.emplace("exclusive_set", makeStringList(names));
    }

    // description（Component）：jsonToNbt 透传。{"translate":"..."} → compound{translate:string}
    if (j.contains("description")) {
        auto descTag = mc::nbt::jsonToNbt(j["description"]);
        if (descTag) {
            root->value.emplace("description", std::move(descTag));
        }
    }

    // effects 树：jsonToNbt 透传。该树无 enchantment/dialog HolderSet #tag 引用（仅含
    // block/damage_type 谓词标签 TagPredicate.id，存为 TagKey 运行时判定，不创建未绑定
    // Named）。Java FloatCodec/NumberProvider 接受任意数值 tag，类型安全。
    if (j.contains("effects")) {
        auto effectsTag = mc::nbt::jsonToNbt(j["effects"]);
        if (effectsTag) {
            root->value.emplace("effects", std::move(effectsTag));
        }
    }

    return mc::network::buffer::nbt_io::serializeRootCompoundToBytes(*root);
}

} // namespace

void setEnchantmentDatapackSource(const mc::resource::DataPackRepository& repo)
{
    g_datapackRepo = &repo;
}

std::vector<mc::network::ir::configuration::RegistryEntry> buildEnchantmentRegistryEntriesUncached(
    const mc::resource::DataPackRepository& repo)
{
    std::vector<mc::network::ir::configuration::RegistryEntry> entries;
    entries.reserve(kEnchantmentIds.size());
    for (const auto id : kEnchantmentIds) {
        mc::network::ir::configuration::RegistryEntry entry;
        entry.id = std::string(id);
        entry.data = buildEnchantmentEntryData(repo, id);
        if (!entry.data.has_value()) {
            spdlog::warn("EnchantmentNbtBuilder: enchantment {} inline NBT build failed, sending nullopt", id);
        }
        entries.push_back(std::move(entry));
    }
    return entries;
}

namespace {
struct EnchantmentCache {
    std::once_flag flag;
    std::vector<mc::network::ir::configuration::RegistryEntry> entries;
};
EnchantmentCache& enchantmentCache()
{
    static EnchantmentCache inst;
    return inst;
}
} // namespace

std::vector<mc::network::ir::configuration::RegistryEntry> buildEnchantmentRegistryEntries()
{
    auto& c = enchantmentCache();
    std::call_once(c.flag, [&c] {
        if (g_datapackRepo == nullptr) {
            spdlog::error("EnchantmentNbtBuilder: datapack source not registered "
                          "(setEnchantmentDatapackSource not called); enchantment entries will be empty");
            return;
        }
        c.entries = buildEnchantmentRegistryEntriesUncached(*g_datapackRepo);
        spdlog::info("EnchantmentNbtBuilder: built {} enchantment entries with inline NBT", c.entries.size());
    });
    return c.entries;
}

} // namespace mc::server::net
