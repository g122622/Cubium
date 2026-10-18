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

#include "common/item/component/DataComponentPatchNbt.hpp"

#include "common/core/Types.hpp"
#include "common/item/component/DataComponentMap.hpp"
#include "common/item/component/DataComponentPayloadCodec.hpp"
#include "common/item/component/DataComponentType.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/EnchantmentContainer.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/nbt/NbtJsonUtils.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace item {
namespace component {

void writePatchToNbt(nbt::tags::compound_tag& out, const DataComponentPatch& patch)
{
    for (const auto& entry : patch.added()) {
        auto type = componentTypeById(entry.typeId);
        if (!type.has_value()) {
            continue; // 未落地组件：跳过
        }
        auto name = componentName(*type);
        if (!name.has_value()) {
            continue;
        }
        auto valueTag = detail::payloadToNbt(*type, entry.value);
        out.value.emplace(std::string(*name), std::move(valueTag));
    }
    for (i32 typeId : patch.removed()) {
        auto type = componentTypeById(typeId);
        if (!type.has_value()) {
            continue;
        }
        auto name = componentName(*type);
        if (!name.has_value()) {
            continue;
        }
        // removed 键以 '!' 前缀；值为占位空 compound
        out.value.emplace(std::string("!") + std::string(*name), std::make_unique<nbt::tags::compound_tag>());
    }
}

DataComponentPatch readPatchFromNbt(const nbt::tags::compound_tag& tag)
{
    DataComponentPatch patch;
    for (const auto& [key, valueTag] : tag.value) {
        if (key.empty()) {
            continue;
        }
        if (key[0] == '!') {
            auto type = componentTypeByName(key.substr(1));
            if (type.has_value()) {
                patch.remove(*type);
            }
            continue;
        }
        auto type = componentTypeByName(key);
        if (!type.has_value()) {
            continue; // 未知组件名：跳过
        }
        // 本实现未落地解码的组件（如 food/equippable/blocks_attacks）跳过；
        // tool/bees/trim/recipes 以载荷形式承载，正常参与往返
        DataComponentPayload payload = detail::nbtToPayload(*type, *valueTag);
        patch.add(*type, std::move(payload));
    }
    return patch;
}

namespace {

/// JSON 字符串是否为 "#namespace:path" 形式的标签引用
bool _isTagReference(const std::string& text)
{
    return !text.empty() && text[0] == '#';
}

/// 资源位置缺失命名空间时补 minecraft:
std::string _normalizeId(const std::string& text)
{
    return text.find(':') == std::string::npos ? "minecraft:" + text : text;
}

/// 方块标签（去掉 '#' 前缀）或方块 ID 列表
bool _readBlockOrTagList(const nlohmann::json& value, std::vector<ResourceLocation>& out)
{
    if (!value.is_array()) {
        return false;
    }
    out.clear();
    for (const auto& entry : value) {
        if (!entry.is_string()) {
            return false;
        }
        const std::string text = entry.get<std::string>();
        out.push_back(ResourceLocation::parse(_isTagReference(text) ? text.substr(1) : text));
    }
    return true;
}

/// 单条 tool 规则（blocks 必填，speed / correct_for_drops 可选）
bool _readToolRule(const nlohmann::json& value, ToolRule& rule)
{
    if (!value.is_object() || !value.contains("blocks")) {
        return false;
    }
    if (!_readBlockOrTagList(value["blocks"], rule.blocks)) {
        return false;
    }
    if (value.contains("speed")) {
        if (!value["speed"].is_number()) {
            return false;
        }
        rule.speed = value["speed"].get<f32>();
    }
    if (value.contains("correct_for_drops")) {
        if (!value["correct_for_drops"].is_boolean()) {
            return false;
        }
        rule.correctForDrops = value["correct_for_drops"].get<bool>();
    }
    return true;
}

/// instrument：注册表键字符串
bool _readInstrument(const nlohmann::json& value, DataComponentPayload& out)
{
    if (!value.is_string()) {
        return false;
    }
    out = DataComponentPayload{std::in_place_index<ComponentPayloadIndex::Instrument>,
        InstrumentPayload{_normalizeId(value.get<std::string>())}};
    return true;
}

/// enchantments / stored_enchantments：{ "levels": { "<id>": level }, "show_in_tooltip": bool }
bool _readEnchantments(const nlohmann::json& value, DataComponentPayload& out)
{
    if (!value.is_object()) {
        return false;
    }
    item::enchant::EnchantmentContainer container;
    if (value.contains("levels")) {
        if (!value["levels"].is_object()) {
            return false;
        }
        for (const auto& [enchantId, level] : value["levels"].items()) {
            if (!level.is_number_integer()) {
                return false;
            }
            container.set(_normalizeId(enchantId), level.get<i32>());
        }
    }
    out = DataComponentPayload{std::in_place_index<ComponentPayloadIndex::Enchantments>, std::move(container)};
    return true;
}

/// potion_contents：{ potion, custom_color, custom_effects... }
bool _readPotionContents(const nlohmann::json& value, DataComponentPayload& out)
{
    if (!value.is_object()) {
        return false;
    }
    PotionContentsPayload pc{};
    if (value.contains("potion")) {
        if (!value["potion"].is_string()) {
            return false;
        }
        pc.potionId = _normalizeId(value["potion"].get<std::string>());
    }
    if (value.contains("custom_color") && value["custom_color"].is_number_integer()) {
        pc.customColor = value["custom_color"].get<i32>();
    }
    if (value.contains("custom_name") && value["custom_name"].is_string()) {
        pc.customName = value["custom_name"].get<std::string>();
    }
    if (value.contains("custom_effects")) {
        if (!value["custom_effects"].is_array()) {
            return false;
        }
        for (const auto& effect : value["custom_effects"]) {
            auto tag = nbt::jsonToNbt(effect);
            if (tag == nullptr || tag->id() != nbt::TagId::Compound) {
                return false;
            }
            pc.customEffects.push_back(
                entity::effect::EffectInstance::fromNbt(dynamic_cast<const nbt::tags::compound_tag&>(*tag)));
        }
    }
    out = DataComponentPayload{std::in_place_index<ComponentPayloadIndex::PotionContents>, std::move(pc)};
    return true;
}

/// 资源位置字符串组件（item_model）
bool _readResourceId(const nlohmann::json& value, DataComponentPayload& out)
{
    if (!value.is_string()) {
        return false;
    }
    out = DataComponentPayload{
        std::in_place_index<ComponentPayloadIndex::Identifier>, _normalizeId(value.get<std::string>())};
    return true;
}

} // namespace

bool jsonToComponentPayload(DataComponentType type, const nlohmann::json& json, DataComponentPayload& out)
{
    // 少数组件在 JSON 中以注册表引用、映射表表达，需在此先行翻译；
    // 其余组件与 NBT Codec 的字段结构一致，走 JSON→NBT 共用解码路径。
    switch (type) {
        case DataComponentType::Instrument:
            return _readInstrument(json, out);
        case DataComponentType::ItemModel:
            return _readResourceId(json, out);
        case DataComponentType::Enchantments:
        case DataComponentType::StoredEnchantments:
            return _readEnchantments(json, out);
        case DataComponentType::PotionContents:
            return _readPotionContents(json, out);
        case DataComponentType::Tool: {
            // tool 的 blocks 列表含 "#tag" 元素：先按 JSON 结构校验，再走通用解码
            if (!json.is_object() || !json.contains("rules") || !json["rules"].is_array()) {
                return false;
            }
            for (const auto& ruleJson : json["rules"]) {
                ToolRule probe{};
                if (!_readToolRule(ruleJson, probe)) {
                    return false;
                }
            }
            break;
        }
        default:
            break;
    }

    // 标量组件值（bool/数值/字符串）直接用单值转换；对象/数组走 jsonToNbtStrictInt
    // （整数不窄化到 byte，与 vanilla Codec.INT 的 NBT 形态一致）
    std::unique_ptr<nbt::tags::tag> tag;
    if (json.is_object() || json.is_array()) {
        tag = nbt::jsonToNbtStrictInt(json);
    } else {
        auto compound = nbt::jsonToNbt(nlohmann::json::object({{"v", json}}));
        if (compound != nullptr) {
            auto it = compound->value.find("v");
            if (it != compound->value.end()) {
                tag = std::move(it->second);
            }
        }
    }
    if (tag == nullptr) {
        return false;
    }
    return detail::tryNbtToPayload(type, *tag, out);
}

} // namespace component
} // namespace item
} // namespace mc
