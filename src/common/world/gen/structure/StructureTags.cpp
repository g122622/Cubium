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

#include "StructureTags.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/gen/structure/StructureTag.hpp"

#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>
#include <spdlog/spdlog.h>

namespace mc::world::gen::structure {

// ============================================================================
// StructureTags 实现
// ============================================================================

bool StructureTags::s_initialized = false;

std::unordered_map<ResourceLocation, std::unique_ptr<StructureTag>>& StructureTags::_getTags()
{
    static std::unordered_map<ResourceLocation, std::unique_ptr<StructureTag>> tags;
    return tags;
}

// ========== 玩法定位标签 ==========

StructureTag& StructureTags::EYE_OF_ENDER_LOCATED()
{
    static StructureTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "eye_of_ender_located"));
    }
    return *tag;
}

StructureTag& StructureTags::DOLPHIN_LOCATED()
{
    static StructureTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "dolphin_located"));
    }
    return *tag;
}

// ========== 探险地图标签 ==========

StructureTag& StructureTags::ON_WOODLAND_EXPLORER_MAPS()
{
    static StructureTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "on_woodland_explorer_maps"));
    }
    return *tag;
}

StructureTag& StructureTags::ON_OCEAN_EXPLORER_MAPS()
{
    static StructureTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "on_ocean_explorer_maps"));
    }
    return *tag;
}

StructureTag& StructureTags::ON_SAVANNA_VILLAGE_MAPS()
{
    static StructureTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "on_savanna_village_maps"));
    }
    return *tag;
}

StructureTag& StructureTags::ON_DESERT_VILLAGE_MAPS()
{
    static StructureTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "on_desert_village_maps"));
    }
    return *tag;
}

StructureTag& StructureTags::ON_PLAINS_VILLAGE_MAPS()
{
    static StructureTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "on_plains_village_maps"));
    }
    return *tag;
}

StructureTag& StructureTags::ON_TAIGA_VILLAGE_MAPS()
{
    static StructureTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "on_taiga_village_maps"));
    }
    return *tag;
}

StructureTag& StructureTags::ON_SNOWY_VILLAGE_MAPS()
{
    static StructureTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "on_snowy_village_maps"));
    }
    return *tag;
}

StructureTag& StructureTags::ON_JUNGLE_EXPLORER_MAPS()
{
    static StructureTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "on_jungle_explorer_maps"));
    }
    return *tag;
}

StructureTag& StructureTags::ON_SWAMP_EXPLORER_MAPS()
{
    static StructureTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "on_swamp_explorer_maps"));
    }
    return *tag;
}

StructureTag& StructureTags::ON_TREASURE_MAPS()
{
    static StructureTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "on_treasure_maps"));
    }
    return *tag;
}

StructureTag& StructureTags::ON_TRIAL_CHAMBERS_MAPS()
{
    static StructureTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "on_trial_chambers_maps"));
    }
    return *tag;
}

// ========== 猫生成标签 ==========

StructureTag& StructureTags::CATS_SPAWN_IN()
{
    static StructureTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "cats_spawn_in"));
    }
    return *tag;
}

StructureTag& StructureTags::CATS_SPAWN_AS_BLACK()
{
    static StructureTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "cats_spawn_as_black"));
    }
    return *tag;
}

// ========== 结构分组标签 ==========

StructureTag& StructureTags::VILLAGE()
{
    static StructureTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "village"));
    }
    return *tag;
}

StructureTag& StructureTags::MINESHAFT()
{
    static StructureTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "mineshaft"));
    }
    return *tag;
}

StructureTag& StructureTags::SHIPWRECK()
{
    static StructureTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "shipwreck"));
    }
    return *tag;
}

StructureTag& StructureTags::RUINED_PORTAL()
{
    static StructureTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "ruined_portal"));
    }
    return *tag;
}

StructureTag& StructureTags::OCEAN_RUIN()
{
    static StructureTag* tag = nullptr;
    if (tag == nullptr) {
        tag = getTag(ResourceLocation("minecraft", "ocean_ruin"));
    }
    return *tag;
}

// ============================================================================
// 初始化
// ============================================================================

void StructureTags::initialize()
{
    if (s_initialized) {
        return;
    }

    auto& tags = _getTags();

    // 注册内置标签及其默认成员（对应 MC 1.21.11 StructureTagsProvider.addTags()）
    // 数据包加载（StructureTagLoader）可后续追加或替换成员。

    // VILLAGE: 5 个变体
    {
        auto tag = std::make_unique<StructureTag>(ResourceLocation("minecraft", "village"));
        tag->addAll({ResourceLocation("minecraft", "village_plains"),
            ResourceLocation("minecraft", "village_desert"),
            ResourceLocation("minecraft", "village_savanna"),
            ResourceLocation("minecraft", "village_snowy"),
            ResourceLocation("minecraft", "village_taiga")});
        tags[tag->getId()] = std::move(tag);
    }

    // MINESHAFT: 普通矿井 + 恶地矿井
    {
        auto tag = std::make_unique<StructureTag>(ResourceLocation("minecraft", "mineshaft"));
        tag->addAll({ResourceLocation("minecraft", "mineshaft"), ResourceLocation("minecraft", "mineshaft_mesa")});
        tags[tag->getId()] = std::move(tag);
    }

    // OCEAN_RUIN: 冷水 + 温水海底废墟
    {
        auto tag = std::make_unique<StructureTag>(ResourceLocation("minecraft", "ocean_ruin"));
        tag->addAll(
            {ResourceLocation("minecraft", "ocean_ruin_cold"), ResourceLocation("minecraft", "ocean_ruin_warm")});
        tags[tag->getId()] = std::move(tag);
    }

    // SHIPWRECK: 普通 + 搁浅
    {
        auto tag = std::make_unique<StructureTag>(ResourceLocation("minecraft", "shipwreck"));
        tag->addAll({ResourceLocation("minecraft", "shipwreck"), ResourceLocation("minecraft", "shipwreck_beached")});
        tags[tag->getId()] = std::move(tag);
    }

    // RUINED_PORTAL: 7 个变体
    {
        auto tag = std::make_unique<StructureTag>(ResourceLocation("minecraft", "ruined_portal"));
        tag->addAll({ResourceLocation("minecraft", "ruined_portal_desert"),
            ResourceLocation("minecraft", "ruined_portal_jungle"),
            ResourceLocation("minecraft", "ruined_portal_mountain"),
            ResourceLocation("minecraft", "ruined_portal_nether"),
            ResourceLocation("minecraft", "ruined_portal_ocean"),
            ResourceLocation("minecraft", "ruined_portal_standard"),
            ResourceLocation("minecraft", "ruined_portal_swamp")});
        tags[tag->getId()] = std::move(tag);
    }

    // CATS_SPAWN_IN: 沼泽小屋
    {
        auto tag = std::make_unique<StructureTag>(ResourceLocation("minecraft", "cats_spawn_in"));
        tag->add(ResourceLocation("minecraft", "swamp_hut"));
        tags[tag->getId()] = std::move(tag);
    }

    // CATS_SPAWN_AS_BLACK: 沼泽小屋
    {
        auto tag = std::make_unique<StructureTag>(ResourceLocation("minecraft", "cats_spawn_as_black"));
        tag->add(ResourceLocation("minecraft", "swamp_hut"));
        tags[tag->getId()] = std::move(tag);
    }

    // EYE_OF_ENDER_LOCATED: 要塞
    {
        auto tag = std::make_unique<StructureTag>(ResourceLocation("minecraft", "eye_of_ender_located"));
        tag->add(ResourceLocation("minecraft", "stronghold"));
        tags[tag->getId()] = std::move(tag);
    }

    // DOLPHIN_LOCATED: 引用 OCEAN_RUIN + SHIPWRECK
    // 注意：此处直接展开嵌套引用，与 MC Java 的 .addTag() 等价
    // 数据包加载时也会通过 # 引用解析，但内置默认值必须是展开后的叶子结构 ID
    // 以保证数据包未提供 dolphin_located.json 时功能仍正常。
    {
        auto tag = std::make_unique<StructureTag>(ResourceLocation("minecraft", "dolphin_located"));
        tag->addAll({ResourceLocation("minecraft", "ocean_ruin_cold"),
            ResourceLocation("minecraft", "ocean_ruin_warm"),
            ResourceLocation("minecraft", "shipwreck"),
            ResourceLocation("minecraft", "shipwreck_beached")});
        tags[tag->getId()] = std::move(tag);
    }

    // ON_WOODLAND_EXPLORER_MAPS: 林地府邸
    {
        auto tag = std::make_unique<StructureTag>(ResourceLocation("minecraft", "on_woodland_explorer_maps"));
        tag->add(ResourceLocation("minecraft", "mansion"));
        tags[tag->getId()] = std::move(tag);
    }

    // ON_OCEAN_EXPLORER_MAPS: 海底神殿
    {
        auto tag = std::make_unique<StructureTag>(ResourceLocation("minecraft", "on_ocean_explorer_maps"));
        tag->add(ResourceLocation("minecraft", "monument"));
        tags[tag->getId()] = std::move(tag);
    }

    // ON_TREASURE_MAPS: 埋藏宝藏
    {
        auto tag = std::make_unique<StructureTag>(ResourceLocation("minecraft", "on_treasure_maps"));
        tag->add(ResourceLocation("minecraft", "buried_treasure"));
        tags[tag->getId()] = std::move(tag);
    }

    // ON_TRIAL_CHAMBERS_MAPS: 试炼密室
    {
        auto tag = std::make_unique<StructureTag>(ResourceLocation("minecraft", "on_trial_chambers_maps"));
        tag->add(ResourceLocation("minecraft", "trial_chambers"));
        tags[tag->getId()] = std::move(tag);
    }

    // ON_SAVANNA_VILLAGE_MAPS
    {
        auto tag = std::make_unique<StructureTag>(ResourceLocation("minecraft", "on_savanna_village_maps"));
        tag->add(ResourceLocation("minecraft", "village_savanna"));
        tags[tag->getId()] = std::move(tag);
    }

    // ON_DESERT_VILLAGE_MAPS
    {
        auto tag = std::make_unique<StructureTag>(ResourceLocation("minecraft", "on_desert_village_maps"));
        tag->add(ResourceLocation("minecraft", "village_desert"));
        tags[tag->getId()] = std::move(tag);
    }

    // ON_PLAINS_VILLAGE_MAPS
    {
        auto tag = std::make_unique<StructureTag>(ResourceLocation("minecraft", "on_plains_village_maps"));
        tag->add(ResourceLocation("minecraft", "village_plains"));
        tags[tag->getId()] = std::move(tag);
    }

    // ON_TAIGA_VILLAGE_MAPS
    {
        auto tag = std::make_unique<StructureTag>(ResourceLocation("minecraft", "on_taiga_village_maps"));
        tag->add(ResourceLocation("minecraft", "village_taiga"));
        tags[tag->getId()] = std::move(tag);
    }

    // ON_SNOWY_VILLAGE_MAPS
    {
        auto tag = std::make_unique<StructureTag>(ResourceLocation("minecraft", "on_snowy_village_maps"));
        tag->add(ResourceLocation("minecraft", "village_snowy"));
        tags[tag->getId()] = std::move(tag);
    }

    // ON_SWAMP_EXPLORER_MAPS: 沼泽小屋
    {
        auto tag = std::make_unique<StructureTag>(ResourceLocation("minecraft", "on_swamp_explorer_maps"));
        tag->add(ResourceLocation("minecraft", "swamp_hut"));
        tags[tag->getId()] = std::move(tag);
    }

    // ON_JUNGLE_EXPLORER_MAPS: 丛林神庙
    {
        auto tag = std::make_unique<StructureTag>(ResourceLocation("minecraft", "on_jungle_explorer_maps"));
        tag->add(ResourceLocation("minecraft", "jungle_temple"));
        tags[tag->getId()] = std::move(tag);
    }

    s_initialized = true;

    spdlog::info("StructureTags: initialized {} structure tags", tags.size());
}

// ============================================================================
// 查询方法
// ============================================================================

StructureTag* StructureTags::getTag(const ResourceLocation& id)
{
    if (!s_initialized) {
        initialize();
    }
    auto& tags = _getTags();
    auto it = tags.find(id);
    if (it != tags.end()) {
        return it->second.get();
    }
    return nullptr;
}

StructureTag& StructureTags::registerTag(const ResourceLocation& id)
{
    if (!s_initialized) {
        initialize();
    }
    auto& tags = _getTags();
    auto it = tags.find(id);
    if (it != tags.end()) {
        return *it->second;
    }
    auto tag = std::make_unique<StructureTag>(id);
    auto& ref = *tag;
    tags[id] = std::move(tag);
    return ref;
}

void StructureTags::forEachTag(std::function<void(StructureTag&)> callback)
{
    if (!s_initialized) {
        initialize();
    }
    auto& tags = _getTags();
    for (auto& [id, tag] : tags) {
        callback(*tag);
    }
}

} // namespace mc::world::gen::structure
