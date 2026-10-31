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

#include "RecipeBook.hpp"

#include "RecipeManager.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "util/assert/AssertAll.hpp"
#include "util/nbt/Nbt.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

namespace mc {
namespace crafting {

// ========== RecipeBookType ==========

const char* recipeBookTypeToString(RecipeBookType type) noexcept
{
    switch (type) {
        case RecipeBookType::Crafting:
            return "crafting";
        case RecipeBookType::Furnace:
            return "furnace";
        case RecipeBookType::BlastFurnace:
            return "blast_furnace";
        case RecipeBookType::Smoker:
            return "smoker";
        default:
            return "unknown";
    }
}

std::optional<RecipeBookType> recipeBookTypeFromString(const std::string& str) noexcept
{
    // 数据包里这两种写法都出现过：NBT 用裸名，资源位置带命名空间。
    if (str == "crafting" || str == "minecraft:crafting") {
        return RecipeBookType::Crafting;
    }
    if (str == "furnace" || str == "minecraft:furnace") {
        return RecipeBookType::Furnace;
    }
    if (str == "blast_furnace" || str == "minecraft:blast_furnace") {
        return RecipeBookType::BlastFurnace;
    }
    if (str == "smoker" || str == "minecraft:smoker") {
        return RecipeBookType::Smoker;
    }
    return std::nullopt;
}

// ========== RecipeBookSettings ==========

namespace {

/// 类型的下标；越界时按 Crafting 处理并断言。
[[nodiscard]] size_t typeIndex(RecipeBookType type) noexcept
{
    const size_t index = static_cast<size_t>(type);
    MC_ASSERT_RELEASE(index < static_cast<size_t>(RecipeBookType::Count));
    return index < static_cast<size_t>(RecipeBookType::Count) ? index : 0;
}

} // namespace

bool RecipeBookSettings::isOpen(RecipeBookType type) const noexcept
{
    return m_settings[typeIndex(type)].open;
}

bool RecipeBookSettings::isFiltering(RecipeBookType type) const noexcept
{
    return m_settings[typeIndex(type)].filtering;
}

void RecipeBookSettings::setOpen(RecipeBookType type, bool open) noexcept
{
    m_settings[typeIndex(type)].open = open;
}

void RecipeBookSettings::setFiltering(RecipeBookType type, bool filtering) noexcept
{
    m_settings[typeIndex(type)].filtering = filtering;
}

void RecipeBookSettings::set(RecipeBookType type, bool open, bool filtering) noexcept
{
    TypeSettings& settings = m_settings[typeIndex(type)];
    settings.open = open;
    settings.filtering = filtering;
}

RecipeBookSettings::TypeSettings RecipeBookSettings::get(RecipeBookType type) const noexcept
{
    return m_settings[typeIndex(type)];
}

void RecipeBookSettings::replaceFrom(const RecipeBookSettings& other) noexcept
{
    m_settings = other.m_settings;
}

RecipeBookSettings RecipeBookSettings::copy() const noexcept
{
    return *this;
}

bool RecipeBookSettings::operator==(const RecipeBookSettings& other) const noexcept
{
    for (size_t index = 0; index < m_settings.size(); ++index) {
        if (!(m_settings[index] == other.m_settings[index])) {
            return false;
        }
    }
    return true;
}

// ========== ServerRecipeBook ==========

void ServerRecipeBook::add(const ResourceLocation& recipeId)
{
    // 原版 `addRecipes`：新解锁的同时插入 highlight，两集合同步增长。
    m_known.insert(recipeId);
    m_highlight.insert(recipeId);
}

bool ServerRecipeBook::contains(const ResourceLocation& recipeId) const noexcept
{
    return m_known.contains(recipeId);
}

void ServerRecipeBook::remove(const ResourceLocation& recipeId)
{
    // 原版 `remove` 同时清两个集合：配方要是还在高亮里，界面上会残留一个点不了的页签。
    m_known.erase(recipeId);
    m_highlight.erase(recipeId);
}

void ServerRecipeBook::removeHighlight(const ResourceLocation& recipeId)
{
    m_highlight.erase(recipeId);
}

bool ServerRecipeBook::isHighlighted(const ResourceLocation& recipeId) const noexcept
{
    return m_highlight.contains(recipeId);
}

bool ServerRecipeBook::isSpecialRecipe(const ResourceLocation& recipeId)
{
    const CraftingRecipe* recipe = RecipeManager::instance().getRecipe(recipeId);
    return recipe != nullptr && recipe->isDynamic();
}

void ServerRecipeBook::clearRecipes() noexcept
{
    m_known.clear();
    m_highlight.clear();
}

ServerRecipeBook::Packed ServerRecipeBook::pack() const
{
    Packed packed;
    packed.settings = m_settings.copy();
    packed.known.assign(m_known.begin(), m_known.end());
    packed.highlight.assign(m_highlight.begin(), m_highlight.end());
    // 集合是无序的，打出来的顺序必须稳定：存档里的列表顺序变化会让「保存后内容不变」
    // 的判定失效（读写往返比对会凭空多出差异）。
    std::sort(packed.known.begin(), packed.known.end());
    std::sort(packed.highlight.begin(), packed.highlight.end());
    return packed;
}

void ServerRecipeBook::apply(const Packed& packed)
{
    m_settings.replaceFrom(packed.settings);
    m_known.clear();
    m_highlight.clear();
    m_known.insert(packed.known.begin(), packed.known.end());
    m_highlight.insert(packed.highlight.begin(), packed.highlight.end());
}

namespace {

/// 读一个布尔字段：缺省或类型不符时按原版 `optionalFieldOf(name, false)` 取 false。
[[nodiscard]] bool readBool(const nbt::tags::compound_tag& tag, const std::string& key) noexcept
{
    const auto it = tag.value.find(key);
    if (it == tag.value.end()) {
        return false;
    }
    const auto* byteTag = dynamic_cast<const nbt::tags::byte_tag*>(it->second.get());
    return byteTag != nullptr && byteTag->value != 0;
}

} // namespace

nbt::tags::compound_tag ServerRecipeBook::write() const
{
    nbt::tags::compound_tag tag;

    // 四个类型的设置平铺在顶层，键名与原版 `RecipeBookSettings` 的四个 MAP_CODEC 一致。
    const auto writeType = [&tag, this](RecipeBookType type, const char* openKey, const char* filteringKey) {
        tag.put(openKey, static_cast<i8>(m_settings.isOpen(type)));
        tag.put(filteringKey, static_cast<i8>(m_settings.isFiltering(type)));
    };
    writeType(RecipeBookType::Crafting, "isGuiOpen", "isFilteringCraftable");
    writeType(RecipeBookType::Furnace, "isFurnaceGuiOpen", "isFurnaceFilteringCraftable");
    writeType(RecipeBookType::BlastFurnace, "isBlastingFurnaceGuiOpen", "isBlastingFurnaceFilteringCraftable");
    writeType(RecipeBookType::Smoker, "isSmokerGuiOpen", "isSmokerFilteringCraftable");

    const Packed packed = pack();
    const auto writeList = [&tag](const char* key, const std::vector<ResourceLocation>& ids) {
        auto list = std::make_unique<nbt::tags::string_list_tag>();
        list->value.reserve(ids.size());
        for (const auto& id : ids) {
            list->value.push_back(id.toString());
        }
        tag.value[key] = std::move(list);
    };
    // `recipes` 是已解锁集合，`toBeDisplayed` 是尚未看过的高亮集合——原版
    // `Packed.CODEC` 里 `known` → `recipes`、`highlight` → `toBeDisplayed`。
    writeList("recipes", packed.known);
    writeList("toBeDisplayed", packed.highlight);

    return tag;
}

void ServerRecipeBook::read(
    const nbt::tags::compound_tag& tag, const std::function<bool(const ResourceLocation&)>& known)
{
    m_settings.set(RecipeBookType::Crafting, readBool(tag, "isGuiOpen"), readBool(tag, "isFilteringCraftable"));
    m_settings.set(
        RecipeBookType::Furnace, readBool(tag, "isFurnaceGuiOpen"), readBool(tag, "isFurnaceFilteringCraftable"));
    m_settings.set(RecipeBookType::BlastFurnace,
        readBool(tag, "isBlastingFurnaceGuiOpen"),
        readBool(tag, "isBlastingFurnaceFilteringCraftable"));
    m_settings.set(
        RecipeBookType::Smoker, readBool(tag, "isSmokerGuiOpen"), readBool(tag, "isSmokerFilteringCraftable"));

    m_known.clear();
    m_highlight.clear();

    // 原版 `loadUntrusted` 会丢弃「不认识的配方」并记日志；这里把判定交给调用方，
    // 因为「认识」取决于各自的配方注册表（探针与游戏拥有的配方并不相同）。
    const auto readList = [&tag, &known](const char* key, std::unordered_set<ResourceLocation>& into) {
        const auto it = tag.value.find(key);
        if (it == tag.value.end()) {
            return;
        }
        const auto* list = dynamic_cast<const nbt::tags::string_list_tag*>(it->second.get());
        if (list == nullptr) {
            return;
        }
        for (const auto& recipeStr : list->value) {
            ResourceLocation recipeId(recipeStr);
            if (known(recipeId)) {
                into.insert(std::move(recipeId));
            }
        }
    };
    readList("recipes", m_known);
    readList("toBeDisplayed", m_highlight);
}

} // namespace crafting
} // namespace mc
