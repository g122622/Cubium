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

#include "common/network/backend/java/mappings/JavaBannerPatternIds.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/blockentity/interactive/BannerPattern.hpp"
#include <array>

#include "common/network/backend/java/mappings/JavaDamageTypeIds.hpp"
#include "server/network/handshake/RegistryDataBuilder.hpp"
#include "server/network/outbound/PacketBuilders.hpp"

#include "common/network/buffer/NbtIo.hpp"
#include "common/network/ir/packets/configuration/ConfigurationPackets.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/nbt/NbtJsonUtils.hpp"
#include "server/network/handshake/EnchantmentNbtBuilder.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <cstdint>
#include <initializer_list>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mc::server::net {

namespace {

constexpr std::array<std::string_view, 3> _dialogIds{
    "minecraft:server_links", "minecraft:custom_options", "minecraft:quick_actions"};

/// 给定一组资源位置 id，构造 data=nullopt 的 RegistryData（声明客户端已知）
mc::network::ir::configuration::RegistryData makeKnownRegistry(
    const char* registryKey, std::initializer_list<const char*> ids)
{
    mc::network::ir::configuration::RegistryData data;
    data.registryKey = registryKey;
    data.entries.reserve(ids.size());
    for (const char* id : ids) {
        data.entries.push_back({std::string(id), std::nullopt});
    }
    return data;
}

/// 进程级 datapack 源（服务器启动期由 setRegistryDatapackSource 注册）。
/// 与 MinecraftServer::m_dataPackList 同生命周期，握手阶段读取安全。
const mc::resource::DataPackRepository* g_registryDatapackRepo = nullptr;

/// 把 "namespace:path" 转成数据包资源路径 "<ns>/<registryDir>/<path>.json"。
/// 不含 "data/" 前缀（与 EnchantmentNbtBuilder::enchantmentIdToResourcePath 同一约定，
/// 误加会双重前缀致 readTextResource 永久 ResourceNotFound）。
std::string registryEntryResourcePath(std::string_view id, std::string_view registryDir)
{
    const auto colon = id.find(':');
    if (colon == std::string_view::npos) {
        return "minecraft/" + std::string(registryDir) + "/" + std::string(id) + ".json";
    }
    return std::string(id.substr(0, colon)) + "/" + std::string(registryDir) + "/" + std::string(id.substr(colon + 1)) +
        ".json";
}

/// 读取并序列化单个注册表条目的 NBT 字节。失败返回 nullopt（调用方回退 data=nullopt）。
std::optional<std::vector<u8>> buildRegistryEntryData(
    const mc::resource::DataPackRepository& repo, std::string_view id, std::string_view registryDir)
{
    const std::string resourcePath = registryEntryResourcePath(id, registryDir);
    auto readResult = repo.readTextResource(resourcePath);
    if (!readResult.success()) {
        spdlog::warn("RegistryDataBuilder: failed to read {}: {}", resourcePath, readResult.error().toString());
        return std::nullopt;
    }

    nlohmann::json j;
    try {
        j = nlohmann::json::parse(std::move(readResult).value());
    }
    catch (const std::exception& e) {
        spdlog::warn("RegistryDataBuilder: failed to parse {}: {}", resourcePath, e.what());
        return std::nullopt;
    }
    if (!j.is_object()) {
        spdlog::warn("RegistryDataBuilder: {} is not a JSON object at top level", resourcePath);
        return std::nullopt;
    }

    auto root = mc::nbt::jsonToNbtStrictInt(j);
    auto* asCompound = dynamic_cast<mc::nbt::tags::compound_tag*>(root.get());
    if (asCompound == nullptr) {
        spdlog::warn("RegistryDataBuilder: {} did not convert to a compound tag", resourcePath);
        return std::nullopt;
    }
    return mc::network::buffer::nbt_io::serializeRootCompoundToBytes(*asCompound);
}

} // namespace

std::vector<mc::network::ir::configuration::RegistryData> buildConfigurationRegistryData()
{
    // 对齐 Java 1.21.11 RegistryDataLoader.SYNCHRONIZED_REGISTRIES：Configuration 阶段须推送
    // 这 23 个动态注册表。条目 id 严格匹配 1.21.11 vanilla 数据包（C:\Users\Administrator\
    // minecraft_reborn\datapacks\Vanilla），均属 minecraft:core 包。
    //
    // 所有条目 data=nullopt：客户端 SelectKnownPacks 命中 minecraft:core 后，对命中 core 的
    // 条目按 RegistrySynchronization.packRegistry 规则视为"客户端已知"，从本地 core 包加载
    // 完整 NBT，无需服务端内联（RegistrySynchronization.java:52-62）。故本项目无需实现各注册表
    // 的 NBT codec 即可与真 Java 客户端互通，前提是 id 与客户端 core 包完全一致。
    //
    // 整数 id（UpdateTags 的 elementId）= 条目在此处的发送顺序索引（0-based）。客户端
    // loadContentsFromNetwork 按 RegistryData 条目顺序 register() 自增分配 id
    // （RegistryDataLoader.java:343-364）。timeline 等需发标签的注册表，顺序须与
    // buildConfigurationUpdateTags() 的索引计算保持一致——故各注册表条目顺序固定勿乱序。
    std::vector<mc::network::ir::configuration::RegistryData> registries;
    registries.reserve(23);

    // 1. dimension_type（4）。requiredNonEmpty=false。
    registries.push_back(makeKnownRegistry("minecraft:dimension_type",
        {"minecraft:overworld", "minecraft:overworld_caves", "minecraft:the_nether", "minecraft:the_end"}));

    // 2. biome（worldgen/biome，65）。registryKey 须为 vanilla Registries.BIOME 的完整 key
    // "minecraft:worldgen/biome"（Registries.java:251 createRegistryKey("worldgen/biome")）。
    // 客户端 RegistryDataLoader.loadContentsFromNetwork 按 SYNCHRONIZED_REGISTRIES 里 biome
    // 的 key（即 worldgen/biome）去 RegistryDataCollector 的 map 查收到的 registry；若发
    // "minecraft:biome" 则 key 不匹配→biome 注册表整个被跳过、从不填充→客户端 biome 注册表
    // 为空→Play 阶段构造 ClientLevel 时 ClientChunkCache 硬编码 getOrThrow(Biomes.PLAINS) 抛
    // "Missing element ResourceKey[minecraft:worldgen/biome / minecraft:plains]"
    // （disconnect-2026-07-29_16.41.16-client.txt）。23 个同步注册表里仅 biome 带 worldgen/
    // 前缀，其余 22 个 vanilla key 均无此前缀。
    registries.push_back(makeKnownRegistry("minecraft:worldgen/biome",
        {"minecraft:badlands",
            "minecraft:bamboo_jungle",
            "minecraft:basalt_deltas",
            "minecraft:beach",
            "minecraft:birch_forest",
            "minecraft:cherry_grove",
            "minecraft:cold_ocean",
            "minecraft:crimson_forest",
            "minecraft:dark_forest",
            "minecraft:deep_cold_ocean",
            "minecraft:deep_dark",
            "minecraft:deep_frozen_ocean",
            "minecraft:deep_lukewarm_ocean",
            "minecraft:deep_ocean",
            "minecraft:desert",
            "minecraft:dripstone_caves",
            "minecraft:end_barrens",
            "minecraft:end_highlands",
            "minecraft:end_midlands",
            "minecraft:eroded_badlands",
            "minecraft:flower_forest",
            "minecraft:forest",
            "minecraft:frozen_ocean",
            "minecraft:frozen_peaks",
            "minecraft:frozen_river",
            "minecraft:grove",
            "minecraft:ice_spikes",
            "minecraft:jagged_peaks",
            "minecraft:jungle",
            "minecraft:lukewarm_ocean",
            "minecraft:lush_caves",
            "minecraft:mangrove_swamp",
            "minecraft:meadow",
            "minecraft:mushroom_fields",
            "minecraft:nether_wastes",
            "minecraft:ocean",
            "minecraft:old_growth_birch_forest",
            "minecraft:old_growth_pine_taiga",
            "minecraft:old_growth_spruce_taiga",
            "minecraft:pale_garden",
            "minecraft:plains",
            "minecraft:river",
            "minecraft:savanna",
            "minecraft:savanna_plateau",
            "minecraft:small_end_islands",
            "minecraft:snowy_beach",
            "minecraft:snowy_plains",
            "minecraft:snowy_slopes",
            "minecraft:snowy_taiga",
            "minecraft:soul_sand_valley",
            "minecraft:sparse_jungle",
            "minecraft:stony_peaks",
            "minecraft:stony_shore",
            "minecraft:sunflower_plains",
            "minecraft:swamp",
            "minecraft:taiga",
            "minecraft:the_end",
            "minecraft:the_void",
            "minecraft:warm_ocean",
            "minecraft:warped_forest",
            "minecraft:windswept_forest",
            "minecraft:windswept_gravelly_hills",
            "minecraft:windswept_hills",
            "minecraft:windswept_savanna",
            "minecraft:wooded_badlands"}));

    // 3. chat_type（7）。1.21.11 ChatType.bootstrap 仅注册 7 个（无 system/narration，旧版残留已删）。
    registries.push_back(makeKnownRegistry("minecraft:chat_type",
        {"minecraft:chat",
            "minecraft:say_command",
            "minecraft:msg_command_incoming",
            "minecraft:msg_command_outgoing",
            "minecraft:team_msg_command_incoming",
            "minecraft:team_msg_command_outgoing",
            "minecraft:emote_command"}));

    // 4. trim_material（11）。1.21.11 新增 resin（旧版 10 个无 resin）。
    registries.push_back(makeKnownRegistry("minecraft:trim_material",
        {"minecraft:quartz",
            "minecraft:iron",
            "minecraft:netherite",
            "minecraft:redstone",
            "minecraft:copper",
            "minecraft:gold",
            "minecraft:emerald",
            "minecraft:diamond",
            "minecraft:lapis",
            "minecraft:amethyst",
            "minecraft:resin"}));

    // 5. trim_pattern（18）。1.21.11 新增 flow（旧版 17 个无 flow）。
    registries.push_back(makeKnownRegistry("minecraft:trim_pattern",
        {"minecraft:bolt",
            "minecraft:coast",
            "minecraft:dune",
            "minecraft:eye",
            "minecraft:flow",
            "minecraft:host",
            "minecraft:raiser",
            "minecraft:rib",
            "minecraft:sentry",
            "minecraft:shaper",
            "minecraft:silence",
            "minecraft:snout",
            "minecraft:spire",
            "minecraft:tide",
            "minecraft:vex",
            "minecraft:ward",
            "minecraft:wayfinder",
            "minecraft:wild"}));

    // 6. wolf_variant（9）。requiredNonEmpty=true。
    registries.push_back(makeKnownRegistry("minecraft:wolf_variant",
        {"minecraft:pale",
            "minecraft:woods",
            "minecraft:ashen",
            "minecraft:black",
            "minecraft:chestnut",
            "minecraft:rusty",
            "minecraft:snowy",
            "minecraft:spotted",
            "minecraft:striped"}));

    // 7. wolf_sound_variant（7）。requiredNonEmpty=true。1.21.11 新增同步注册表。
    registries.push_back(makeKnownRegistry("minecraft:wolf_sound_variant",
        {"minecraft:classic",
            "minecraft:big",
            "minecraft:cute",
            "minecraft:angry",
            "minecraft:grumpy",
            "minecraft:sad",
            "minecraft:puglin"}));

    // 8. pig_variant（3）。requiredNonEmpty=true。
    registries.push_back(
        makeKnownRegistry("minecraft:pig_variant", {"minecraft:temperate", "minecraft:cold", "minecraft:warm"}));

    // 9. frog_variant（3）。requiredNonEmpty=true。
    registries.push_back(
        makeKnownRegistry("minecraft:frog_variant", {"minecraft:temperate", "minecraft:cold", "minecraft:warm"}));

    // 10. cat_variant（11）。requiredNonEmpty=true。
    registries.push_back(makeKnownRegistry("minecraft:cat_variant",
        {"minecraft:tabby",
            "minecraft:black",
            "minecraft:red",
            "minecraft:siamese",
            "minecraft:british_shorthair",
            "minecraft:calico",
            "minecraft:persian",
            "minecraft:ragdoll",
            "minecraft:white",
            "minecraft:jellie",
            "minecraft:all_black"}));

    // 11. cow_variant（3）。requiredNonEmpty=true。
    registries.push_back(
        makeKnownRegistry("minecraft:cow_variant", {"minecraft:temperate", "minecraft:cold", "minecraft:warm"}));

    // 12. chicken_variant（3）。requiredNonEmpty=true。
    registries.push_back(
        makeKnownRegistry("minecraft:chicken_variant", {"minecraft:temperate", "minecraft:cold", "minecraft:warm"}));

    // 13. zombie_nautilus_variant（2）。requiredNonEmpty=true。
    registries.push_back(
        makeKnownRegistry("minecraft:zombie_nautilus_variant", {"minecraft:temperate", "minecraft:warm"}));

    // 14. painting_variant（51）。requiredNonEmpty=true。
    // 1.21.11 无 graal（旧版残留已删）；skull_and_roses 仅一次（旧版重复已修）。
    // 严格按数据包 51 个文件名（排序后）。
    registries.push_back(makeKnownRegistry("minecraft:painting_variant",
        {"minecraft:alban",
            "minecraft:aztec",
            "minecraft:aztec2",
            "minecraft:backyard",
            "minecraft:baroque",
            "minecraft:bomb",
            "minecraft:bouquet",
            "minecraft:burning_skull",
            "minecraft:bust",
            "minecraft:cavebird",
            "minecraft:changing",
            "minecraft:cotan",
            "minecraft:courbet",
            "minecraft:creebet",
            "minecraft:dennis",
            "minecraft:donkey_kong",
            "minecraft:earth",
            "minecraft:endboss",
            "minecraft:fern",
            "minecraft:fighters",
            "minecraft:finding",
            "minecraft:fire",
            "minecraft:graham",
            "minecraft:humble",
            "minecraft:kebab",
            "minecraft:lowmist",
            "minecraft:match",
            "minecraft:meditative",
            "minecraft:orb",
            "minecraft:owlemons",
            "minecraft:passage",
            "minecraft:pigscene",
            "minecraft:plant",
            "minecraft:pointer",
            "minecraft:pond",
            "minecraft:pool",
            "minecraft:prairie_ride",
            "minecraft:sea",
            "minecraft:skeleton",
            "minecraft:skull_and_roses",
            "minecraft:stage",
            "minecraft:sunflowers",
            "minecraft:sunset",
            "minecraft:tides",
            "minecraft:unpacked",
            "minecraft:void",
            "minecraft:wanderer",
            "minecraft:wasteland",
            "minecraft:water",
            "minecraft:wind",
            "minecraft:wither"}));

    // 15. damage_type（50）。1.21.11 无 avoid/merging/thistle（旧版残留已删）；新增多个。
    mc::network::ir::configuration::RegistryData damageTypes{};
    damageTypes.registryKey = "minecraft:damage_type";
    for (const auto name : mc::network::backend::java::JavaDamageTypeIds::ENTRIES) {
        damageTypes.entries.push_back({std::string(name), std::nullopt});
    }
    registries.push_back(std::move(damageTypes));

    // 16. banner_pattern（43）。
    mc::network::ir::configuration::RegistryData bannerPatterns{};
    bannerPatterns.registryKey = "minecraft:banner_pattern";
    for (const auto name : mc::network::backend::java::JavaBannerPatternIds::ENTRIES) {
        bannerPatterns.entries.push_back({std::string(name), std::nullopt});
    }
    registries.push_back(std::move(bannerPatterns));

    // 17. enchantment（43）。发【内联 NBT】而非 data=nullopt。
    // 根因：enchantment JSON 的 supported_items/primary_items 引用 ITEM 标签（静态注册表），
    // 服务端无法发 ITEM UpdateTags（ITEM 整数 id 由客户端 bootstrap 决定、不可复制），故
    // data=nullopt 路径下客户端解码时 HolderSetCodec.lookupTag 因 ITEM 标签未绑定报
    // "Missing tag" → "Failed to parse local data"（全部 43 个）。改为内联 NBT 并把
    // supported_items/primary_items/exclusive_set 的 #tag 展平为显式名字列表（HolderSetCodec
    // 走 Either.right → HolderSet.direct → 按 name 解码，绕开 lookupTag 与未绑定 Named）。
    // id 顺序与原硬编码列表严格一致（见 EnchantmentNbtBuilder::kEnchantmentIds）。
    {
        mc::network::ir::configuration::RegistryData data;
        data.registryKey = "minecraft:enchantment";
        data.entries = buildEnchantmentRegistryEntries();
        registries.push_back(std::move(data));
    }

    // 18. jukebox_song（21）。
    registries.push_back(makeKnownRegistry("minecraft:jukebox_song",
        {"minecraft:13",
            "minecraft:cat",
            "minecraft:blocks",
            "minecraft:chirp",
            "minecraft:far",
            "minecraft:mall",
            "minecraft:mellohi",
            "minecraft:stal",
            "minecraft:strad",
            "minecraft:ward",
            "minecraft:11",
            "minecraft:wait",
            "minecraft:otherside",
            "minecraft:5",
            "minecraft:pigstep",
            "minecraft:relic",
            "minecraft:precipice",
            "minecraft:creator",
            "minecraft:creator_music_box",
            "minecraft:tears",
            "minecraft:lava_chicken"}));

    // 19. instrument（8）。
    registries.push_back(makeKnownRegistry("minecraft:instrument",
        {"minecraft:admire_goat_horn",
            "minecraft:call_goat_horn",
            "minecraft:dream_goat_horn",
            "minecraft:feel_goat_horn",
            "minecraft:ponder_goat_horn",
            "minecraft:seek_goat_horn",
            "minecraft:sing_goat_horn",
            "minecraft:yearn_goat_horn"}));

    // 20. test_environment（1）。
    registries.push_back(makeKnownRegistry("minecraft:test_environment", {"minecraft:default"}));

    // 21. test_instance（1）。
    registries.push_back(makeKnownRegistry("minecraft:test_instance", {"minecraft:always_pass"}));

    // 22. dialog（3）。
    mc::network::ir::configuration::RegistryData dialogs{};
    dialogs.registryKey = "minecraft:dialog";
    for (const auto name : _dialogIds)
        dialogs.entries.push_back({std::string(name), std::nullopt});
    registries.push_back(std::move(dialogs));

    // 23. timeline（4）。顺序固定对齐 Timelines.bootstrap：DAY, MOON, VILLAGER_SCHEDULE,
    // EARLY_GAME（id 0/1/2/3）。buildConfigurationUpdateTags() 的 timeline tag 索引据此计算。
    registries.push_back(makeKnownRegistry("minecraft:timeline",
        {"minecraft:day", "minecraft:moon", "minecraft:villager_schedule", "minecraft:early_game"}));

    return registries;
}

std::vector<mc::network::ir::configuration::TagRegistry> buildConfigurationUpdateTags()
{
    // Configuration 阶段 UpdateTags。对齐 Java SynchronizeRegistriesTask：网络同步的 registry
    // 完全替换客户端本地 registry（RegistryDataLoader.loadFromNetwork 新建空 MappedRegistry），
    // 本地 core 包的 tags/ 目录在网络路径不被读取（TagLoader.loadTagsForRegistry 仅用于
    // loadContentsFromManager 资源路径）。故标签必须由 UpdateTags 显式 bindTag。
    //
    // 关键：timeline 的 in_overworld/in_nether/in_end tag 在 dimension_type 解码时因
    // HolderSetCodec.lookupTag → getOrCreateTagForRegistration 被创建为【未绑定】
    // （MappedRegistry.java:237-243），freeze() 校验未绑定 tag 抛 "Unbound tags"
    // （MappedRegistry.java:290-301）。必须由 UpdateTags 提供非空 payload 才能 bindTag。
    //
    // elementId = 条目在 buildConfigurationRegistryData() 对应 registry 中的发送顺序索引
    // （0-based）。timeline 顺序：day=0, moon=1, villager_schedule=2, early_game=3。
    // tag 定义（数据包 tags/timeline/*.json）：
    //   universal        = [villager_schedule]                       → [2]
    //   in_overworld     = [#universal, day, moon, early_game]       → [2, 0, 1, 3]
    //   in_nether        = [#universal]                              → [2]
    //   in_end           = [#universal]                              → [2]
    // tag 引用其他 tag（#前缀）在网络层已展平为具体元素 id 列表（TagNetworkSerialization 按
    // registry.getId 序列化最终 Holder 集合），故此处填展平后的 id。
    //
    // dialog：quick_actions.json / custom_options.json 引用 #minecraft:quick_actions /
    // #minecraft:pause_screen_additions（DIALOG 标签，networkable）。两者在 datapack 中均
    // 为空（values:[]），但 HolderSetCodec.lookupTag 在解码时经 getOrCreateTagForRegistration
    // 创建未绑定 Named，freeze() 校验未绑定 tag 抛 "Unbound tags"。UpdateTags 即使发空 id
    // 列表也会触发 bindTag(空列表) → bound（HolderSet.Named.bind 空列表即 bound），故发空 tag。
    // dialog i32 id（发送顺序 0-2）对空 tag 无关（无元素 id 可编码）。
    std::vector<mc::network::ir::configuration::TagRegistry> registries;
    registries.reserve(2);

    mc::network::ir::configuration::TagRegistry timeline;
    timeline.registryKey = "minecraft:timeline";
    timeline.tags = {
        {"minecraft:universal", {2}},
        {"minecraft:in_overworld", {2, 0, 1, 3}},
        {"minecraft:in_nether", {2}},
        {"minecraft:in_end", {2}},
    };
    registries.push_back(std::move(timeline));

    mc::network::ir::configuration::TagRegistry dialog;
    dialog.registryKey = "minecraft:dialog";
    dialog.tags = {
        {"minecraft:pause_screen_additions", {}}, // 空 id 列表（datapack 中该 tag 为空）
        {"minecraft:quick_actions", {}},          // 空 id 列表
    };
    registries.push_back(std::move(dialog));

    mc::network::ir::configuration::TagRegistry bannerTags{};
    bannerTags.registryKey = "minecraft:banner_pattern";
    mc::network::ir::configuration::TagListEntry basic{"minecraft:no_item_required", {}};
    for (const auto type : mc::blockentity::BannerPatterns::getLoomPatterns()) {
        const auto id = mc::network::backend::java::JavaBannerPatternIds::toWireId(
            "minecraft:" + mc::blockentity::BannerPatterns::getFileName(type));
        MC_ASSERT_RELEASE(id.has_value());
        basic.elementIds.push_back(*id);
    }
    bannerTags.tags.push_back(std::move(basic));
    constexpr std::array<std::pair<std::string_view, std::string_view>, 10> patternTags{
        {{"minecraft:pattern_item/bordure_indented", "minecraft:curly_border"},
            {"minecraft:pattern_item/creeper", "minecraft:creeper"},
            {"minecraft:pattern_item/field_masoned", "minecraft:bricks"},
            {"minecraft:pattern_item/flow", "minecraft:flow"},
            {"minecraft:pattern_item/flower", "minecraft:flower"},
            {"minecraft:pattern_item/globe", "minecraft:globe"},
            {"minecraft:pattern_item/guster", "minecraft:guster"},
            {"minecraft:pattern_item/mojang", "minecraft:mojang"},
            {"minecraft:pattern_item/piglin", "minecraft:piglin"},
            {"minecraft:pattern_item/skull", "minecraft:skull"}}};
    for (const auto& [tag, pattern] : patternTags) {
        const auto id = mc::network::backend::java::JavaBannerPatternIds::toWireId(pattern);
        MC_ASSERT_RELEASE(id.has_value());
        bannerTags.tags.push_back({std::string(tag), {*id}});
    }
    registries.push_back(std::move(bannerTags));

    return registries;
}

std::vector<mc::network::ir::configuration::KnownPack> buildServerKnownPacks()
{
    return {mc::network::ir::configuration::KnownPack{"minecraft", "core", "1.21.11"}};
}

void setRegistryDatapackSource(const mc::resource::DataPackRepository& repo)
{
    g_registryDatapackRepo = &repo;
}

std::vector<mc::network::ir::configuration::RegistryData> buildConfigurationRegistryDataForUnknownClient()
{
    std::vector<mc::network::ir::configuration::RegistryData> registries;

    if (g_registryDatapackRepo == nullptr) {
        spdlog::error("RegistryDataBuilder: datapack source not registered; cannot build registry NBT for a client "
                      "that declared no minecraft:core");
        return registries;
    }

    // 以 buildConfigurationRegistryData() 的条目与顺序为基准——该顺序与 UpdateTags 的
    // elementId 计算绑定（客户端按 RegistryData 的收到顺序自增分配 registry id），
    // 不可变更。此处只做「逐条目补齐 NBT」这一件事。
    registries = buildConfigurationRegistryData();

    std::size_t filledCount = 0;
    std::size_t skippedCount = 0;

    for (auto& registry : registries) {
        // registryKey 形如 "minecraft:dimension_type" / "minecraft:worldgen/biome"，
        // 去掉 "minecraft:" 前缀后即为数据包内的目录名。
        const std::string& key = registry.registryKey;
        const auto colonPos = key.find(':');
        const std::string registryDir = (colonPos == std::string::npos) ? key : key.substr(colonPos + 1);

        std::vector<mc::network::ir::configuration::RegistryEntry> filledEntries;
        filledEntries.reserve(registry.entries.size());
        for (auto& entry : registry.entries) {
            if (entry.data.has_value()) {
                // 已带内联 NBT 的条目（如 enchantment，由 EnchantmentNbtBuilder 构造）原样保留。
                filledEntries.push_back(std::move(entry));
                continue;
            }
            auto data = buildRegistryEntryData(*g_registryDatapackRepo, entry.id, registryDir);
            if (!data.has_value()) {
                // 构建失败时**跳过该条目**，而不是回退到「有 key 无 value」——后者会让客户端
                // 解析时抛异常（prismarine-registry 对 undefined value 调 nbt.simplify）。
                // 跳过至少不会使对端崩溃，且下面的统计日志会暴露覆盖率缺口。
                ++skippedCount;
                continue;
            }
            entry.data = std::move(data);
            filledEntries.push_back(std::move(entry));
            ++filledCount;
        }
        registry.entries = std::move(filledEntries);
    }

    spdlog::info("RegistryDataBuilder: client declared no minecraft:core; sending inline NBT for {} registries"
                 " ({} entries built, {} skipped)",
        registries.size(),
        filledCount,
        skippedCount);
    if (skippedCount > 0) {
        spdlog::warn("RegistryDataBuilder: {} entries failed NBT construction and were skipped;"
                     "the peer registry will be incomplete (see the read/parse warnings above)",
            skippedCount);
    }

    return registries;
}

Result<nlohmann::json> loadDialogDefinition(std::string_view id)
{
    if (g_registryDatapackRepo == nullptr)
        return Error(ErrorCode::InvalidState, "Dialog datapack source is not configured");
    auto text = g_registryDatapackRepo->readTextResource(registryEntryResourcePath(id, "dialog"));
    if (text.failed()) return text.error();
    auto definition = nlohmann::json::parse(text.value(), nullptr, false);
    if (definition.is_discarded()) return Error(ErrorCode::InvalidData, "Invalid dialog JSON");
    return definition;
}

Result<mc::network::ir::IrPacket> resolveDialogIr(const ResourceLocation& name)
{
    for (i32 i = 0; i < static_cast<i32>(_dialogIds.size()); ++i)
        if (name.toString() == _dialogIds[i])
            return mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play,
                mc::network::ir::PlayPacket{mc::network::ir::play::ShowDialog{i}}};
    auto definition = loadDialogDefinition(name.toString());
    if (definition.failed()) return definition.error();
    return buildShowDialogIr(definition.value());
}

} // namespace mc::server::net
