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

#include "SmithingTrimRecipe.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/item/armortrim/TrimPatterns.hpp"
#include "common/item/component/DataComponentType.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/crafting/IRecipe.hpp"
#include "common/item/crafting/Ingredient.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace mc {
namespace crafting {

namespace {

/**
 * @brief 从纹饰材料物品反查它声明的 TrimMaterial id
 *
 * 原版 `TrimMaterials.getFromIngredient` 读的是物品的 `provides_trim_material` 数据组件
 * （`EitherHolder<TrimMaterial>`），而不是去 TrimMaterial 注册表里找"材料物品"字段——
 * 该注册表只有 assets / description，没有那个字段。
 *
 * EitherHolder 的文档态有两种：`{"id": "minecraft:gold"}` 或直接 `"minecraft:gold"`，
 * 这里都接受；其余形态视为未声明。
 *
 * @param stack 材料槽的物品堆
 * @return 材质 id；该物品未声明纹饰材质时返回 nullopt
 */
std::optional<ResourceLocation> _trimMaterialFromIngredient(const ItemStack& stack)
{
    const nlohmann::json* component = stack.rawComponent(item::component::DataComponentType::ProvidesTrimMaterial);
    if (component == nullptr) {
        return std::nullopt;
    }

    std::string id;
    if (component->is_string()) {
        id = component->get<std::string>();
    } else if (component->is_object() && component->contains("id") && (*component)["id"].is_string()) {
        id = (*component)["id"].get<std::string>();
    } else {
        return std::nullopt;
    }

    if (id.empty()) {
        return std::nullopt;
    }
    return ResourceLocation(id);
}

} // namespace

const std::string SmithingTrimRecipe::EMPTY_GROUP = "";

SmithingTrimRecipe::SmithingTrimRecipe(const ResourceLocation& id,
    Ingredient templateIngredient,
    Ingredient base,
    Ingredient addition,
    ResourceLocation pattern)
    : m_id(id)
    , m_template(std::move(templateIngredient))
    , m_base(std::move(base))
    , m_addition(std::move(addition))
    , m_pattern(std::move(pattern))
{
    // 缓存原料列表：template + base + addition（顺序对应原版 3 槽）
    m_ingredients.push_back(m_template);
    m_ingredients.push_back(m_base);
    m_ingredients.push_back(m_addition);
}

bool SmithingTrimRecipe::matches(const IInventory& inventory) const
{
    // 槽位语义已与 SmithingMenu.java 核对一致：TEMPLATE_SLOT=0 / BASE_SLOT=1 /
    // ADDITIONAL_SLOT=2，与下方 SLOT_* 常量逐项对应，接入锻造台容器时无需调整。
    // TODO: 项目尚无 SmithingMenu（锻造台容器/界面），本方法目前只被单元测试以
    //   长度≥3 的 inventory 直接驱动；容器落地后应确认实际槽位布局仍与上述一致。
    if (inventory.getContainerSize() < 3) {
        return false;
    }

    ItemStack templateStack = inventory.getItem(SLOT_TEMPLATE);
    ItemStack baseStack = inventory.getItem(SLOT_BASE);
    ItemStack additionStack = inventory.getItem(SLOT_ADDITION);

    return m_template.test(templateStack) && m_base.test(baseStack) && m_addition.test(additionStack);
}

ItemStack SmithingTrimRecipe::assemble(const IInventory& inventory) const
{
    // 对应 MC 1.21.11 SmithingTrimRecipe#assemble -> applyTrim(base, addition, pattern)
    // 原版逻辑：从 addition 解析 TrimMaterial，构造 ArmorTrim(material, pattern)，
    // 若 base 已有相同 trim 则返回 EMPTY（去重），否则返回 base 副本并写入 TRIM 组件；
    // addition 无法解析为 TrimMaterial 时返回 EMPTY。
    //
    // 与地图扩展同口径：数据包只声明配方存在，材料匹配由配方的原料表（模板 / 可纹饰盔甲
    // 标签 / 纹饰材料标签）负责，这里只算结果。
    if (inventory.getContainerSize() <= SLOT_ADDITION) {
        return ItemStack::EMPTY;
    }
    const ItemStack& baseStack = inventory.getItem(SLOT_BASE);
    if (baseStack.isEmpty()) {
        return ItemStack::EMPTY;
    }

    // 由 addition 物品反查纹饰材质：读它的 `provides_trim_material` 组件，而不是去
    // TrimMaterial 注册表里找"材料物品"——该注册表只有 assets / description，没有那个字段。
    const std::optional<ResourceLocation> materialId = _trimMaterialFromIngredient(inventory.getItem(SLOT_ADDITION));
    if (!materialId.has_value()) {
        // addition 不是任何纹饰材料：不产出纹饰，返回空（原版同样返回 EMPTY）。
        return ItemStack::EMPTY;
    }

    // 图案由配方声明（数据包里的 pattern 字段），必须能在注册表中查到，否则是数据包错误。
    if (TrimPatterns::get(m_pattern) == nullptr) {
        spdlog::warn("[Smithing] Trim recipe '{}' references unknown trim pattern '{}', refusing to craft",
            m_id.toString(),
            m_pattern.toString());
        return ItemStack::EMPTY;
    }

    // TRIM 载荷形态与原版 ArmorTrim record 一致：{"material": <id>, "pattern": <id>}。
    // 该组件在项目里以原始 JSON 承载（见 DataComponentPayloadCodec 的 Trim 分支）。
    const nlohmann::json trim = {
        {"material", materialId->toString()},
        {"pattern", m_pattern.toString()},
    };

    // 已有相同纹饰则不重复应用（原版 Objects.equals 后返回 EMPTY）。
    if (const nlohmann::json* existing = baseStack.rawComponent(item::component::DataComponentType::Trim);
        existing != nullptr && *existing == trim) {
        return ItemStack::EMPTY;
    }

    ItemStack result = baseStack.copy();
    result.setCount(1);
    result.setComponent(item::component::DataComponentType::Trim, item::component::DataComponentPayload{trim});
    return result;
}

std::vector<ItemStack> SmithingTrimRecipe::getRemainingItems(const IInventory& inventory) const
{
    return RecipeUtils::getDefaultRemainingItems(inventory);
}

} // namespace crafting
} // namespace mc
