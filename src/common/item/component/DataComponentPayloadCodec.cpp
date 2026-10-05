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

#include "common/item/component/DataComponentPayloadCodec.hpp"

#include "common/core/Types.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/item/component/AttackRange.hpp"
#include "common/item/component/DataComponentMap.hpp"
#include "common/item/component/DataComponentType.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/item/core/ItemRarity.hpp"
#include "common/item/enchantment/EnchantmentContainer.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/nbt/NbtJsonUtils.hpp"
#include "common/util/text/ITextComponent.hpp"

#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace item {
namespace component {
namespace detail {

using nbt::TagId;
using nbt::tags::compound_tag;
using nbt::tags::float_tag;
using nbt::tags::int_tag;
using nbt::tags::list_tag;
using nbt::tags::short_tag;
using nbt::tags::string_list_tag;
using nbt::tags::string_tag;

std::unique_ptr<nbt::tags::tag> payloadToNbt(DataComponentType type, const DataComponentPayload& payload)
{
    switch (type) {
        case DataComponentType::Damage:
        case DataComponentType::RepairCost:
        case DataComponentType::MaxStackSize:
        case DataComponentType::MaxDamage:
        case DataComponentType::Enchantable: {
            const auto* p = std::get_if<i32>(&payload);
            return std::make_unique<nbt::tags::int_tag>(p ? *p : 0);
        }
        case DataComponentType::Unbreakable: {
            // Unit 组件：NBT 写空 compound（vanilla Unit.CODEC = MapCodec.unit）
            return std::make_unique<compound_tag>();
        }
        case DataComponentType::Rarity: {
            const auto* p = std::get_if<ItemRarity>(&payload);
            const auto name = rarityName(p ? *p : ItemRarity::Common);
            return std::make_unique<string_tag>(std::string(name.value_or("common")));
        }
        case DataComponentType::ItemModel: {
            const auto* p = std::get_if<std::string>(&payload);
            return std::make_unique<string_tag>(p ? *p : std::string{});
        }
        case DataComponentType::CustomName:
        case DataComponentType::ItemName: {
            const auto& p = std::get<std::unique_ptr<text::ITextComponent>>(payload);
            return std::make_unique<nbt::tags::string_tag>(p ? p->toJson().dump() : std::string{});
        }
        case DataComponentType::Lore: {
            const auto& lines = std::get<std::vector<std::unique_ptr<text::ITextComponent>>>(payload);
            auto list = std::make_unique<nbt::tags::string_list_tag>();
            for (const auto& line : lines) {
                list->value.push_back(line ? line->toJson().dump() : std::string{});
            }
            return list;
        }
        case DataComponentType::Enchantments: {
            const auto& ench = std::get<item::enchant::EnchantmentContainer>(payload);
            return ench.toNbt();
        }
        case DataComponentType::AttackRange: {
            const auto& ar = std::get<AttackRange>(payload);
            auto compound = std::make_unique<compound_tag>();
            compound->put("min_reach", static_cast<f32>(ar.minRange));
            compound->put("max_reach", static_cast<f32>(ar.maxRange));
            compound->put("min_creative_reach", static_cast<f32>(ar.minCreativeRange));
            compound->put("max_creative_reach", static_cast<f32>(ar.maxCreativeRange));
            compound->put("hitbox_margin", static_cast<f32>(ar.hitboxMargin));
            compound->put("mob_factor", static_cast<f32>(ar.mobFactor));
            return compound;
        }
        case DataComponentType::PotionContents: {
            const auto& pc = std::get<PotionContentsPayload>(payload);
            auto compound = std::make_unique<compound_tag>();
            if (!pc.potionId.empty()) {
                compound->put("potion", pc.potionId);
            }
            if (pc.customColor.has_value()) {
                compound->put("custom_color", *pc.customColor);
            }
            if (!pc.customEffects.empty()) {
                auto list = std::make_unique<nbt::tags::compound_list_tag>();
                for (const auto& effect : pc.customEffects) {
                    nbt::tags::compound_tag effectCompound;
                    effect.toNbt(effectCompound);
                    list->value.push_back(std::move(effectCompound));
                }
                compound->value.emplace("custom_effects", std::move(list));
            }
            if (pc.customName.has_value()) {
                compound->put("custom_name", *pc.customName);
            }
            return compound;
        }
        case DataComponentType::CanPlaceOn:
        case DataComponentType::CanBreak: {
            const auto& pred = std::get<AdventureModePredicate>(payload);
            auto list = std::make_unique<nbt::tags::string_list_tag>();
            for (const auto& s : pred.getPredicates()) {
                list->value.push_back(s);
            }
            return list;
        }
        case DataComponentType::CustomData: {
            const auto& j = std::get<nlohmann::json>(payload);
            if (j.is_object() && !j.empty()) {
                auto nbt = nbt::jsonToNbt(j);
                if (nbt != nullptr) {
                    return nbt;
                }
            }
            return std::make_unique<compound_tag>();
        }
    }
    return std::make_unique<compound_tag>();
}

DataComponentPayload nbtToPayload(DataComponentType type, const nbt::tags::tag& tag)
{
    switch (type) {
        case DataComponentType::Damage:
        case DataComponentType::RepairCost:
        case DataComponentType::MaxStackSize:
        case DataComponentType::MaxDamage:
        case DataComponentType::Enchantable: {
            if (tag.id() == TagId::Int) {
                return DataComponentPayload{std::in_place_index<1>, dynamic_cast<const int_tag&>(tag).value};
            }
            if (tag.id() == TagId::Short) {
                return DataComponentPayload{
                    std::in_place_index<1>, static_cast<i32>(dynamic_cast<const short_tag&>(tag).value)};
            }
            return DataComponentPayload{std::in_place_index<1>, 0};
        }
        case DataComponentType::Unbreakable: {
            // Unit 组件：存在即不可损坏
            return DataComponentPayload{std::in_place_index<8>, true};
        }
        case DataComponentType::Rarity: {
            ItemRarity rarity = ItemRarity::Common;
            if (tag.id() == TagId::String) {
                const auto& s = dynamic_cast<const string_tag&>(tag).value;
                rarity = rarityFromName(s).value_or(ItemRarity::Common);
            }
            return DataComponentPayload{std::in_place_index<9>, rarity};
        }
        case DataComponentType::ItemModel: {
            std::string model;
            if (tag.id() == TagId::String) {
                model = dynamic_cast<const string_tag&>(tag).value;
            }
            return DataComponentPayload{std::in_place_index<10>, std::move(model)};
        }
        case DataComponentType::AttackRange: {
            AttackRange ar{};
            if (tag.id() == TagId::Compound) {
                const auto& c = dynamic_cast<const compound_tag&>(tag);
                auto readF32 = [&c](const char* key, f32 fallback) -> f32 {
                    auto it = c.value.find(key);
                    if (it != c.value.end() && it->second->id() == TagId::Float) {
                        return dynamic_cast<const float_tag&>(*it->second).value;
                    }
                    return fallback;
                };
                ar.minRange = readF32("min_reach", ar.minRange);
                ar.maxRange = readF32("max_reach", ar.maxRange);
                ar.minCreativeRange = readF32("min_creative_reach", ar.minCreativeRange);
                ar.maxCreativeRange = readF32("max_creative_reach", ar.maxCreativeRange);
                ar.hitboxMargin = readF32("hitbox_margin", ar.hitboxMargin);
                ar.mobFactor = readF32("mob_factor", ar.mobFactor);
            }
            return DataComponentPayload{std::in_place_index<11>, ar};
        }
        case DataComponentType::CustomName:
        case DataComponentType::ItemName: {
            if (tag.id() == TagId::String) {
                const auto& s = dynamic_cast<const string_tag&>(tag).value;
                if (!s.empty()) {
                    // 写侧存的是组件 JSON 字符串（见 payloadToNbt），这里用
                    // ITextComponent::fromJson 解析回组件；TextParser::parse 只认
                    // legacy § 格式，会把 JSON 当纯文本导致 getUnformattedText 退化为 JSON 原文。
                    return DataComponentPayload{
                        std::in_place_index<2>, text::ITextComponent::fromJson(nlohmann::json::parse(s))};
                }
            }
            return DataComponentPayload{std::in_place_index<2>, nullptr};
        }
        case DataComponentType::Lore: {
            std::vector<std::unique_ptr<text::ITextComponent>> lines;
            if (tag.id() == TagId::List) {
                const auto& list = dynamic_cast<const list_tag&>(tag);
                if (list.element_id() == TagId::String) {
                    const auto& sl = dynamic_cast<const string_list_tag&>(list);
                    for (const auto& s : sl.value) {
                        lines.push_back(text::ITextComponent::fromJson(nlohmann::json::parse(s)));
                    }
                }
            }
            return DataComponentPayload{std::in_place_index<3>, std::move(lines)};
        }
        case DataComponentType::Enchantments: {
            if (tag.id() == TagId::List) {
                return DataComponentPayload{std::in_place_index<4>,
                    item::enchant::EnchantmentContainer::fromNbt(dynamic_cast<const list_tag&>(tag))};
            }
            return DataComponentPayload{std::in_place_index<4>, item::enchant::EnchantmentContainer{}};
        }
        case DataComponentType::PotionContents: {
            PotionContentsPayload pc{};
            if (tag.id() == TagId::Compound) {
                const auto& c = dynamic_cast<const compound_tag&>(tag);
                auto it = c.value.find("potion");
                if (it != c.value.end() && it->second->id() == TagId::String) {
                    pc.potionId = dynamic_cast<const string_tag&>(*it->second).value;
                }
                auto colorIt = c.value.find("custom_color");
                if (colorIt != c.value.end() && colorIt->second->id() == TagId::Int) {
                    pc.customColor = dynamic_cast<const int_tag&>(*colorIt->second).value;
                }
                auto effectsIt = c.value.find("custom_effects");
                if (effectsIt != c.value.end() && effectsIt->second->id() == TagId::List) {
                    if (auto* clist = dynamic_cast<const nbt::tags::compound_list_tag*>(effectsIt->second.get())) {
                        for (const auto& elem : clist->value) {
                            pc.customEffects.push_back(entity::effect::EffectInstance::fromNbt(elem));
                        }
                    }
                }
                auto nameIt = c.value.find("custom_name");
                if (nameIt != c.value.end() && nameIt->second->id() == TagId::String) {
                    pc.customName = dynamic_cast<const string_tag&>(*nameIt->second).value;
                }
            }
            return DataComponentPayload{std::in_place_index<5>, std::move(pc)};
        }
        case DataComponentType::CanPlaceOn:
        case DataComponentType::CanBreak: {
            std::vector<std::string> preds;
            if (tag.id() == TagId::List) {
                const auto& list = dynamic_cast<const list_tag&>(tag);
                if (list.element_id() == TagId::String) {
                    const auto& sl = dynamic_cast<const string_list_tag&>(list);
                    for (const auto& s : sl.value) {
                        preds.push_back(s);
                    }
                }
            }
            return DataComponentPayload{std::in_place_index<6>, AdventureModePredicate(std::move(preds))};
        }
        case DataComponentType::CustomData: {
            if (tag.id() == TagId::Compound) {
                return DataComponentPayload{std::in_place_index<7>, nbt::nbtToJson(tag)};
            }
            return DataComponentPayload{std::in_place_index<7>, nlohmann::json::object()};
        }
    }
    return DataComponentPayload{};
}

} // namespace detail
} // namespace component
} // namespace item
} // namespace mc
