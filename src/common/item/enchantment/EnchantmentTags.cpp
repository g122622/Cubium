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

#include "EnchantmentTags.hpp"

#include "common/item/enchantment/EnchantmentRegistry.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/resource/tag/GenericTagLoader.hpp"
#include <string>
#include <vector>

namespace mc {

namespace {

/// 数据包里的标签目录（相对 `data/<ns>/`）。
constexpr std::string_view ENCHANTMENT_TAG_DIRECTORY = "tags/enchantment";

bool s_initialized = false;

/**
 * @brief 加载器用的成员句柄池
 *
 * 通用加载器以 `std::size_t` 标识成员。附魔本身是字符串 ID，这里做一个双向池：
 * 句柄 -> ID 的向量即 `EnchantmentTags::_memberTable()`。
 */
std::vector<ResourceLocation>& memberTable()
{
    return EnchantmentTags::_memberTable();
}

std::size_t poolGetOrCreate(const ResourceLocation& enchantmentId)
{
    auto& table = memberTable();
    for (std::size_t i = 0; i < table.size(); ++i) {
        if (table[i] == enchantmentId) {
            return i;
        }
    }
    table.push_back(enchantmentId);
    return table.size() - 1;
}

} // namespace

std::vector<ResourceLocation>& EnchantmentTags::_memberTable()
{
    static std::vector<ResourceLocation> table;
    return table;
}

std::unordered_map<ResourceLocation, std::unique_ptr<EnchantmentTag>>& EnchantmentTags::_getTags()
{
    static std::unordered_map<ResourceLocation, std::unique_ptr<EnchantmentTag>> tags;
    return tags;
}

void EnchantmentTags::initialize()
{
    if (s_initialized) {
        return;
    }

    // 原版 `EnchantmentTags` 里静态声明的 15 个标签。这里只创建空对象，
    // 成员全部来自数据包——附魔标签没有任何硬编码默认值。
    static constexpr std::string_view TAG_NAMES[] = {
        "minecraft:curse",
        "minecraft:double_trade_price",
        "minecraft:in_enchanting_table",
        "minecraft:non_treasure",
        "minecraft:on_mob_spawn_equipment",
        "minecraft:on_random_loot",
        "minecraft:on_traded_equipment",
        "minecraft:prevents_bee_spawns_when_mining",
        "minecraft:prevents_decorated_pot_shattering",
        "minecraft:prevents_ice_melting",
        "minecraft:prevents_infested_spawns",
        "minecraft:smelts_loot",
        "minecraft:tooltip_order",
        "minecraft:tradeable",
        "minecraft:treasure",
    };
    for (const auto name : TAG_NAMES) {
        registerTag(ResourceLocation(name));
    }

    s_initialized = true;
}

bool EnchantmentTags::isInitialized() noexcept
{
    return s_initialized;
}

EnchantmentTag* EnchantmentTags::getTag(const ResourceLocation& id)
{
    auto& tags = _getTags();
    auto it = tags.find(id);
    return it != tags.end() ? it->second.get() : nullptr;
}

EnchantmentTag& EnchantmentTags::registerTag(const ResourceLocation& id)
{
    auto& tags = _getTags();
    auto it = tags.find(id);
    if (it != tags.end()) {
        return *it->second;
    }
    auto tag = std::make_unique<EnchantmentTag>(id);
    auto& ref = *tag;
    tags[id] = std::move(tag);
    return ref;
}

void EnchantmentTags::forEachTag(const std::function<void(EnchantmentTag&)>& callback)
{
    for (auto& [id, tag] : _getTags()) {
        callback(*tag);
    }
}

Result<size_t> EnchantmentTags::loadFromDataPackRepository(const resource::DataPackRepository& dataPacks)
{
    resource::tag::TagMemberResolver resolveMember = [](std::string_view memberId) -> std::size_t {
        // 成员是附魔 ID。只接受注册表里真实存在的附魔——原版的数据包校验也是这个语义，
        // 引用一个不存在的附魔应当视为配置错误而不是静默忽略。
        const ResourceLocation id(memberId);
        if (!item::enchant::EnchantmentRegistry::has(id.toString())) {
            return resource::tag::TAG_MEMBER_NOT_FOUND;
        }
        return poolGetOrCreate(id);
    };

    resource::tag::TagMemberReader readTag = [](const ResourceLocation& tagId) -> std::vector<std::size_t> {
        std::vector<std::size_t> members;
        const auto* tag = getTag(tagId);
        if (tag == nullptr) {
            return members;
        }
        for (const auto& enchantmentId : tag->getEnchantmentIds()) {
            members.push_back(poolGetOrCreate(enchantmentId));
        }
        return members;
    };

    resource::tag::TagFiller fillTag =
        [](const ResourceLocation& tagId, const std::vector<std::size_t>& members, bool replace) {
            EnchantmentTag& tag = registerTag(tagId);
            if (replace) {
                tag.clear();
            }
            const auto& table = memberTable();
            for (const std::size_t index : members) {
                if (index < table.size()) {
                    tag.add(table[index]);
                }
            }
        };

    return resource::tag::GenericTagLoader::loadFromDataPackRepository(
        dataPacks, ENCHANTMENT_TAG_DIRECTORY, resolveMember, readTag, fillTag);
}

void EnchantmentTags::clearAll()
{
    _getTags().clear();
    _memberTable().clear();
    s_initialized = false;
}

} // namespace mc
