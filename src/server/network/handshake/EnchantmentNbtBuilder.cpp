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

#include "server/network/handshake/EnchantmentNbtBuilder.hpp"

#include "common/core/Types.hpp"
#include "common/entity/tag/EntityTypeTag.hpp"
#include "common/entity/tag/EntityTypeTags.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/tag/ItemTag.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/network/buffer/NbtIo.hpp"
#include "common/network/ir/packets/configuration/ConfigurationPackets.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/nbt/NbtJsonUtils.hpp"
#include "common/world/block/BlockTags.hpp"

#include <spdlog/spdlog.h>

#include <nlohmann/json.hpp>

#include <cstdint>
#include <exception>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>
#include <nlohmann/json_fwd.hpp>

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
std::string _enchantmentIdToResourcePath(std::string_view id)
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
// 同 _enchantmentIdToResourcePath：不含 "data/" 前缀（FolderResourcePack 预置）。
std::string _enchantmentTagRefToResourcePath(std::string_view tagRef)
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

/// 从 datapack 读文本资源；失败返回空串并记 error（调用方据此回退或发 nullopt）。
std::string _readDataPackText(const mc::resource::DataPackRepository& repo, const std::string& resourcePath)
{
    auto r = repo.readTextResource(resourcePath);
    if (!r.success()) {
        spdlog::error("EnchantmentNbtBuilder: readTextResource failed: {} ({})", resourcePath, r.error().toString());
        return {};
    }
    return std::move(r).value();
}

/// 把 HolderSet 引用展平为元素名列表。
/// "namespace:path" 或单元素名 → 直接返回该名字；
/// "#namespace:path" → 优先 ItemTags::getTag（已递归展平嵌套 #），未命中则直读
///   tags/enchantment/*.json 的 values 数组（vanilla exclusive_set 仅含名字，无嵌套 #）。
std::vector<std::string> _flattenHolderSet(
    const mc::resource::DataPackRepository& repo, const std::string& holderSetVal)
{
    std::vector<std::string> result;
    if (holderSetVal.empty()) {
        // 空值：调用方仅在该字段存在且为字符串时调用，空串是数据包异常，明确报错。
        spdlog::error("EnchantmentNbtBuilder: _flattenHolderSet called with empty value; datapack field is blank");
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
        if (result.empty()) {
            spdlog::warn("EnchantmentNbtBuilder: ITEM #tag '{}' exists but resolved ZERO members", holderSetVal);
        }
        return result;
    }

    // 回退：直读 tags/enchantment/<...>.json（enchantment exclusive_set 等未在服务端注册的标签）
    const std::string tagPath = _enchantmentTagRefToResourcePath(holderSetVal);
    const std::string text = _readDataPackText(repo, tagPath);
    if (text.empty()) {
        // 静默回退缺陷源头：既不在 ItemTags 也无数据包文件，下游会拿到空 HolderSet。
        // 明确 error 报告标签名与查找路径，便于定位是"标签名拼错"还是"数据包缺失"。
        spdlog::error("EnchantmentNbtBuilder: #tag '{}' NOT in ItemTags and datapack file missing ({}); "
                      "downstream HolderSet will be empty",
            holderSetVal,
            tagPath);
        return result;
    }
    try {
        const auto j = nlohmann::json::parse(text);
        if (!j.contains("values") || !j["values"].is_array()) {
            spdlog::error("EnchantmentNbtBuilder: tag json {} missing 'values' array", tagPath);
            return result;
        }
        for (const auto& v : j["values"]) {
            if (v.is_string()) {
                const std::string s = v.get<std::string>();
                if (!s.empty() && s[0] == '#') {
                    // 嵌套 # 引用未解析：原样丢弃会让 HolderSet 缺项，明确 error（而非静默 continue）。
                    spdlog::error("EnchantmentNbtBuilder: nested tag ref '{}' in {} not resolved; "
                                  "item will be MISSING from the resulting HolderSet",
                        s,
                        tagPath);
                    continue;
                }
                result.push_back(s);
            } else {
                spdlog::error("EnchantmentNbtBuilder: non-string value in tag json {} values array", tagPath);
            }
        }
    }
    catch (const std::exception& e) {
        spdlog::error("EnchantmentNbtBuilder: parse tag json {} failed: {}", tagPath, e.what());
    }
    return result;
}

/// effects 树内静态注册表 #tag 引用所属注册表的提示。
///
/// effects 谓词/效果 HolderSet 字段引用 BLOCK/ITEM/ENTITY_TYPE 标签（均静态注册表，客户端无法
/// lookupTag）。所属注册表由**消费字段的 key** 决定（非标签名）：predicate.type→ENTITY_TYPE，
/// predicate.blocks / effect.immune_blocks→BLOCK，predicate.items→ITEM（见 Java
/// EntityTypePredicate/BlockPredicate/ItemPredicate/ExplodeEffect 的 homogeneousList 参数）。
///
/// 关键：标签名可能跨注册表撞名——vanilla `#minecraft:arrows` 同时是 ITEM 标签（arrow/tipped_arrow/
/// spectral_arrow）与 ENTITY_TYPE 标签（arrow/spectral_arrow）。若不按 key 区分而盲目先查 ItemTags，
/// 会把 ITEM 列表（含 tipped_arrow）发给 predicate.type（ENTITY_TYPE HolderSet），客户端按名解码
/// tipped_arrow 在 ENTITY_TYPE 注册表中不存在→"Failed to parse value"（disconnect-...15.46.43）。
/// 故必须按消费字段 key 选定注册表，不能按标签名猜。
enum class EffectsTagRegistry {
    Unknown, ///< 未知字段 key（按安全顺序回退：ENTITY_TYPE→BLOCK→ITEM，绝不让 ITEM 优先）。
    Item,
    EntityType,
    Block,
};

/// 把 key 名映射为注册表提示（仅对已知 HolderSet 字段 key 命中，其余返回 Unknown）。
EffectsTagRegistry _registryHintForKey(std::string_view key)
{
    if (key == "type") {
        return EffectsTagRegistry::EntityType; // EntityTypePredicate.type
    }
    if (key == "blocks" || key == "immune_blocks") {
        return EffectsTagRegistry::Block; // BlockPredicate.blocks / ExplodeEffect.immune_blocks
    }
    if (key == "items") {
        return EffectsTagRegistry::Item; // ItemPredicate.items
    }
    return EffectsTagRegistry::Unknown;
}

/// 把 effects 树内的静态注册表 #tag 引用展平为元素名列表（HolderSetCodec 名字列表形式）。
///
/// effects 谓词/效果字段引用 BLOCK/ITEM/ENTITY_TYPE 标签（均静态注册表，客户端无法 lookupTag）。
/// 三类底层全 HolderSetCodec，#tag 走 Either.left→lookupTag 会失败；展平为名字列表走
/// Either.right→HolderSet.direct→RegistryFixedCodec 按 name 解码，无 lookupTag、无未绑定 Named。
///
/// registryHint 指定标签所属注册表（由消费字段 key 决定，见 EffectsTagRegistry）。命中时**仅**从
/// 该注册表解析，避免跨注册表撞名（如 #arrows 同名 ITEM/ENTITY_TYPE 标签）误选。Unknown 时按安全
/// 顺序 ENTITY_TYPE→BLOCK→ITEM 回退（绝不让 ITEM 优先，因 predicate.type 字段最易与 ITEM 撞名）。
/// 全失败返回空（调用方保留原 # 串 + 记 warn，让客户端报错定位）。
std::vector<std::string> _flattenTagToNames(
    const mc::resource::DataPackRepository& repo, const std::string& tagRef, EffectsTagRegistry registryHint)
{
    std::vector<std::string> result;
    if (tagRef.empty() || tagRef[0] != '#') {
        // 非 #tag 引用：调用方按 key 值判定后才调用本函数，走到这里说明调用方逻辑有误，明确报错。
        spdlog::error("EnchantmentNbtBuilder: _flattenTagToNames called with non-tag value '{}'; caller bug", tagRef);
        return result;
    }
    const std::string refStr = tagRef.substr(1);
    const mc::ResourceLocation loc = mc::ResourceLocation::parse(refStr);

    // Unknown hint 意味着"该字段 key 不在已知 HolderSet 字段表内"，只能按安全顺序猜测注册表——
    // 这正是跨注册表撞名（如 #arrows 同名 ITEM/ENTITY_TYPE）的隐患点，明确告警而非静默猜。
    if (registryHint == EffectsTagRegistry::Unknown) {
        spdlog::warn("EnchantmentNbtBuilder: effects #tag '{}' has unknown registry hint (field key not recognized); "
                     "guessing registries in order ENTITY_TYPE→BLOCK→ITEM",
            tagRef);
    }

    // 按注册表尝试顺序：hint 已知则只查该注册表；Unknown 按 ENTITY_TYPE→BLOCK→ITEM 安全回退
    // （不让 ITEM 优先，规避 #arrows 跨注册表撞名）。
    const bool tryItem = (registryHint == EffectsTagRegistry::Item || registryHint == EffectsTagRegistry::Unknown);
    const bool tryEntity =
        (registryHint == EffectsTagRegistry::EntityType || registryHint == EffectsTagRegistry::Unknown);
    const bool tryBlock = (registryHint == EffectsTagRegistry::Block || registryHint == EffectsTagRegistry::Unknown);

    // 每个注册表分支的命中判定记录，供全部落空时给出可定位的错误信息（哪个注册表查过、
    // 该注册表当前是否初始化、标签名是否撞名）。
    bool entityLookupAttempted = false;
    bool blockLookupAttempted = false;
    bool itemLookupAttempted = false;

    // ENTITY_TYPE 标签（启动期 EntityTypeTagLoader 已递归解析嵌套 #）
    if (tryEntity) {
        entityLookupAttempted = true;
        if (auto* entityTag = mc::EntityTypeTags::getTag(loc)) {
            for (const mc::ResourceLocation& id : entityTag->getEntityTypeIds()) {
                result.push_back(id.toString());
            }
            if (result.empty()) {
                // 标签存在但成员为空：数据包定义/加载顺序异常，静默返回空会让下游拿到空列表，
                // 明确告警以便定位（空标签在 vanilla 里几乎不存在）。
                spdlog::warn("EnchantmentNbtBuilder: ENTITY_TYPE #tag '{}' exists but resolved ZERO members", tagRef);
            }
            return result;
        }
    }
    // BLOCK 标签（启动期 BlockTagLoader 已递归解析嵌套 #，含数据包定义的新标签）
    if (tryBlock) {
        blockLookupAttempted = true;
        if (auto* blockTag = mc::BlockTags::getTag(loc)) {
            for (const mc::ResourceLocation& id : blockTag->getBlockIds()) {
                result.push_back(id.toString());
            }
            if (result.empty()) {
                spdlog::warn("EnchantmentNbtBuilder: BLOCK #tag '{}' exists but resolved ZERO members", tagRef);
            }
            return result;
        }
    }
    // ITEM 标签（启动期 ItemTagLoader 已递归解析嵌套 #）
    if (tryItem) {
        itemLookupAttempted = true;
        if (auto* itemTag = mc::item::tag::ItemTags::getTag(loc)) {
            for (const mc::Item* item : itemTag->getItems()) {
                result.push_back(item->itemLocation().toString());
            }
            if (result.empty()) {
                spdlog::warn("EnchantmentNbtBuilder: ITEM #tag '{}' exists but resolved ZERO members", tagRef);
            }
            return result;
        }
    }

    // 全部落空：这是"静默回退"型缺陷的源头——effects 内 #tag 展平失败会原样保留 "#..." 串发给客户端，
    // 客户端按 HolderSetCodec 解码失败并 disconnect，而服务端日志此前只有一条 warn 无法定位。
    // 此处升级为 error，并逐项报告查询过的注册表、各注册表是否已初始化、以及跨注册表撞名诊断。
    spdlog::error("EnchantmentNbtBuilder: effects #tag '{}' (hint={}) unresolved in ALL candidate registries; "
                  "the raw '#' string will be sent to the client and cause a disconnect",
        tagRef,
        static_cast<int>(registryHint));
    if (entityLookupAttempted) {
        spdlog::error("  - ENTITY_TYPE registry queried: EntityTypeTags::isInitialized()={}",
            mc::EntityTypeTags::isInitialized());
    }
    if (blockLookupAttempted) {
        spdlog::error("  - BLOCK registry queried: BlockTags::getTag('{}')={}",
            loc.toString(),
            mc::BlockTags::getTag(loc) != nullptr ? "present" : "absent");
    }
    if (itemLookupAttempted) {
        spdlog::error(
            "  - ITEM registry queried: ItemTags::isInitialized()={}", mc::item::tag::ItemTags::isInitialized());
    }
    return result;
}

/// 递归遍历 effects JSON 子树，原地展平所有静态注册表 #tag 引用为元素名列表。
///
/// 判据：值以 '#' 开头的字符串即静态注册表 HolderSet 标签引用（effect type/sound/entity/attribute
/// 等非标签字段用纯 ResourceLocation 串，从不以 # 开头，自然排除）。所属注册表由**父 key** 决定
/// （type→ENTITY_TYPE，blocks/immune_blocks→BLOCK，items→ITEM），避免跨注册表撞名误选。展平后由
/// jsonToNbt 把名字数组转为 string_list_tag（HolderSetCodec 名字列表线格式）。
///
/// 仅作用于 effects 子树——顶层 supported_items/primary_items/exclusive_set（含 networkable ENCHANTMENT
/// 标签）由现有 _flattenHolderSet 处理，本函数不触及。visited 防标签自环（vanilla 无，兜底）。
void _flattenEffectsTagRefsInPlace(const mc::resource::DataPackRepository& repo,
    nlohmann::json& node,
    EffectsTagRegistry parentHint,
    std::unordered_set<std::string>& visited)
{
    if (node.is_object()) {
        // 先收集 key（迭代中改 value 不触发增删 key，但用 key 索引重赋值最稳）。
        std::vector<std::string> keys;
        keys.reserve(node.size());
        for (auto& [key, value] : node.items()) {
            keys.push_back(key);
        }
        for (const auto& key : keys) {
            auto& value = node[key];
            // 子节点的注册表提示由当前 key 决定（消费字段 key → 注册表）。
            const EffectsTagRegistry childHint = _registryHintForKey(key);
            if (value.is_string()) {
                const std::string s = value.get<std::string>();
                if (!s.empty() && s[0] == '#') {
                    if (visited.count(s) != 0) {
                        spdlog::error("EnchantmentNbtBuilder: tag ref cycle detected at '{}'; "
                                      "aborting this branch to avoid infinite recursion",
                            s);
                        continue;
                    }
                    visited.insert(s);
                    auto names = _flattenTagToNames(repo, s, childHint);
                    visited.erase(s);
                    if (!names.empty()) {
                        nlohmann::json arr = nlohmann::json::array();
                        for (auto& name : names) {
                            arr.push_back(std::move(name));
                        }
                        value = std::move(arr);
                    } else {
                        // 展平失败却原样保留 "#..." 串：客户端 HolderSetCodec 会拒绝该值并 disconnect。
                        // 明确 error（具体注册表诊断已在 _flattenTagToNames 内打印）。
                        spdlog::error("EnchantmentNbtBuilder: effects #tag '{}' flatten returned EMPTY; "
                                      "leaving raw '#' string which will cause a client disconnect",
                            s);
                    }
                }
            } else {
                _flattenEffectsTagRefsInPlace(repo, value, childHint, visited);
            }
        }
    } else if (node.is_array()) {
        for (auto& elem : node) {
            // 数组元素继承父 hint（如 terms[]/predicate 链下，子元素仍属同一注册表上下文）。
            _flattenEffectsTagRefsInPlace(repo, elem, parentHint, visited);
        }
    }
}

/// 构造字符串列表 tag（HolderSet 名字列表 / slots 列表的线格式）。
std::unique_ptr<mc::nbt::tags::tag_list_tag> _makeStringList(const std::vector<std::string>& names)
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
std::unique_ptr<mc::nbt::tags::compound_tag> _makeCostCompound(const nlohmann::json& costJson)
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

/// 缺失可选字段的累计记录（字段名 → 缺失该字段的附魔 id 列表）。
///
/// primary_items / exclusive_set / effects 在原版 Enchantment CODEC 中均为 optionalFieldOf
/// （分别缺省回落 Optional.empty() / HolderSet.direct() / DataComponentMap.EMPTY），故数据包不写
/// 这些字段是合法形态，不应逐条报错。此处累计后由调用方汇总输出一行 info，避免 43 条附魔各报
/// 数条 error 淹没真正缺陷（文件缺失 / JSON 语法错 / 标签展平失败）。
struct MissingOptionalFieldTally {
    std::vector<std::string> primaryItems; ///< 未声明 primary_items 的附魔 id
    std::vector<std::string> exclusiveSet; ///< 未声明 exclusive_set 的附魔 id
    std::vector<std::string> effects;      ///< 未声明 effects 的附魔 id
};

/// 把缺失某可选字段的附魔 id 列表格式化为 "N=[a, b, ...]"（空列表→"0"），供汇总行拼装。
std::string _formatMissingFieldIds(const std::vector<std::string>& ids)
{
    if (ids.empty()) {
        return "0";
    }
    std::string s = std::to_string(ids.size());
    s += "=[";
    for (usize i = 0; i < ids.size(); ++i) {
        if (i != 0) {
            s += ", ";
        }
        s += ids[i];
    }
    s += "]";
    return s;
}

/// 汇总输出缺失可选字段的一行 info（无任何缺失时静默）。
void _logMissingOptionalFields(const MissingOptionalFieldTally& tally)
{
    if (tally.primaryItems.empty() && tally.exclusiveSet.empty() && tally.effects.empty()) {
        return;
    }
    spdlog::info("EnchantmentNbtBuilder: optional enchantment fields absent in datapack "
                 "(defaults applied per vanilla CODEC optionalFieldOf): primary_items={}; exclusive_set={}; effects={}",
        _formatMissingFieldIds(tally.primaryItems),
        _formatMissingFieldIds(tally.exclusiveSet),
        _formatMissingFieldIds(tally.effects));
}

/// 构造单个 enchantment 的内联 NBT 字节（Java 根 NBT 线格式）。
/// 失败返回 nullopt（调用方据此回退 data=nullopt 或跳过）。
/// tally 累计本条目缺失的可选字段（primary_items/exclusive_set/effects），供调用方汇总一行 info。
std::optional<std::vector<u8>> _buildEnchantmentEntryData(
    const mc::resource::DataPackRepository& repo, std::string_view id, MissingOptionalFieldTally& tally)
{
    const std::string resourcePath = _enchantmentIdToResourcePath(id);
    const std::string text = _readDataPackText(repo, resourcePath);
    if (text.empty()) {
        // 资源文件缺失：调用方据此发 nullopt 给客户端。已由 _readDataPackText 记 error，
        // 此处补一条指明是哪个 enchantment，便于定位。
        spdlog::error(
            "EnchantmentNbtBuilder: enchantment '{}' resource missing ({}); sending nullopt", id, resourcePath);
        return std::nullopt;
    }
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(text);
    }
    catch (const std::exception& e) {
        spdlog::error("EnchantmentNbtBuilder: parse {} failed: {}; sending nullopt", resourcePath, e.what());
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
        root->value.emplace("min_cost", _makeCostCompound(j["min_cost"]));
    }
    if (j.contains("max_cost")) {
        root->value.emplace("max_cost", _makeCostCompound(j["max_cost"]));
    }

    // slots：字符串列表
    if (j.contains("slots") && j["slots"].is_array()) {
        std::vector<std::string> slots;
        for (const auto& s : j["slots"]) {
            if (s.is_string()) {
                slots.push_back(s.get<std::string>());
            } else {
                // 非字符串槽位名：静默跳过会让 slots 缺项，客户端按 EquipmentSlot 名解码时缺项，明确报错。
                spdlog::error("EnchantmentNbtBuilder: enchantment '{}' slots contains non-string entry, skipped", id);
            }
        }
        root->value.emplace("slots", _makeStringList(slots));
    } else if (j.contains("slots")) {
        spdlog::error("EnchantmentNbtBuilder: enchantment '{}' slots field is not an array, omitted", id);
    }

    // HolderSet 字段：展平 #tag → 显式名字列表（绕开 lookupTag 与未绑定 Named）。
    // supported_items 在 vanilla Enchantment CODEC 中为必需（fieldOf），缺失会导致客户端解码失败，明确报错；
    // primary_items / exclusive_set 为可选（optionalFieldOf），缺失属合法形态，仅累计后汇总一行 info。
    const bool hasSupportedItems = j.contains("supported_items") && j["supported_items"].is_string();
    if (hasSupportedItems) {
        const auto names = _flattenHolderSet(repo, j["supported_items"].get<std::string>());
        root->value.emplace("supported_items", _makeStringList(names));
    } else {
        spdlog::error("EnchantmentNbtBuilder: enchantment '{}' missing/invalid 'supported_items' (required)", id);
    }
    if (j.contains("primary_items") && j["primary_items"].is_string()) {
        const auto names = _flattenHolderSet(repo, j["primary_items"].get<std::string>());
        root->value.emplace("primary_items", _makeStringList(names));
    } else if (j.contains("primary_items")) {
        // 字段存在但类型非字符串：数据包格式异常（非"合法缺省"），明确报错。
        spdlog::error("EnchantmentNbtBuilder: enchantment '{}' 'primary_items' present but not a string", id);
    } else {
        // 合法缺省：原版 optionalFieldOf 回落 Optional.empty()（该附魔无 primary 限制）。
        tally.primaryItems.emplace_back(id);
    }
    if (j.contains("exclusive_set") && j["exclusive_set"].is_string()) {
        const auto names = _flattenHolderSet(repo, j["exclusive_set"].get<std::string>());
        root->value.emplace("exclusive_set", _makeStringList(names));
    } else if (j.contains("exclusive_set")) {
        spdlog::error("EnchantmentNbtBuilder: enchantment '{}' 'exclusive_set' present but not a string", id);
    } else {
        // 合法缺省：原版 optionalFieldOf 回落 HolderSet.direct()（该附魔不属于任何互斥组）。
        tally.exclusiveSet.emplace_back(id);
    }

    // description（Component）：jsonToNbt 透传。{"translate":"..."} → compound{translate:string}
    if (j.contains("description")) {
        auto descTag = mc::nbt::jsonToNbt(j["description"]);
        if (descTag) {
            root->value.emplace("description", std::move(descTag));
        } else {
            // jsonToNbt 失败会让 description 缺失，客户端 Enchantment CODEC 拒绝（Component 必需）。
            spdlog::error("EnchantmentNbtBuilder: enchantment '{}' description jsonToNbt failed, omitted", id);
        }
    } else {
        spdlog::error("EnchantmentNbtBuilder: enchantment '{}' missing 'description' (required)", id);
    }

    // effects 树：先把静态注册表 #tag 引用（predicate.type/blocks/items、effect.immune_blocks 等
    // HolderSetCodec 字段）原地展平为名字列表，再 jsonToNbt 透传。BLOCK/ITEM/ENTITY_TYPE 标签客户端
    // 无法 lookupTag（静态注册表），不展平会被 Enchantment.DIRECT_CODEC 拒收（"Failed to parse value"，
    // disconnect-2026-07-29_14.13.16-client.txt）。展平后名字数组 → string_list_tag（HolderSetCodec
    // 名字列表线格式），effects 数值子树仍走 jsonToNbt（Java FloatCodec/NumberProvider 接受任意数值 tag）。
    if (j.contains("effects")) {
        std::unordered_set<std::string> visited;
        _flattenEffectsTagRefsInPlace(repo, j["effects"], EffectsTagRegistry::Unknown, visited);
        auto effectsTag = mc::nbt::jsonToNbt(j["effects"]);
        if (effectsTag) {
            root->value.emplace("effects", std::move(effectsTag));
        } else {
            spdlog::error("EnchantmentNbtBuilder: enchantment '{}' effects jsonToNbt failed, omitted", id);
        }
    } else {
        // 合法缺省：原版 optionalFieldOf 回落 DataComponentMap.EMPTY（该附魔无效果组件，
        // 如 fortune 的效果走战利品表修改器而非附魔效果）。
        tally.effects.emplace_back(id);
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
    // 累计各附魔缺失的可选字段（primary_items/exclusive_set/effects），循环结束后汇总一行 info，
    // 避免 43 条附魔逐条报 error 淹没真正缺陷（文件缺失 / JSON 语法错 / 标签展平失败）。
    MissingOptionalFieldTally tally;
    for (const auto id : kEnchantmentIds) {
        mc::network::ir::configuration::RegistryEntry entry;
        entry.id = std::string(id);
        entry.data = _buildEnchantmentEntryData(repo, id, tally);
        if (!entry.data.has_value()) {
            // 内联 NBT 构建失败 → 发 nullopt，客户端该 enchantment 注册表条目为空，明确 error。
            spdlog::error("EnchantmentNbtBuilder: enchantment '{}' inline NBT build failed, sending nullopt", id);
        }
        entries.push_back(std::move(entry));
    }
    _logMissingOptionalFields(tally);
    return entries;
}

namespace {
struct EnchantmentCache {
    std::once_flag flag;
    std::vector<mc::network::ir::configuration::RegistryEntry> entries;
};
EnchantmentCache& _enchantmentCache()
{
    static EnchantmentCache inst;
    return inst;
}
} // namespace

std::vector<mc::network::ir::configuration::RegistryEntry> buildEnchantmentRegistryEntries()
{
    auto& c = _enchantmentCache();
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
