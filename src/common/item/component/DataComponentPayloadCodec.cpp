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
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/EnchantmentContainer.hpp"
#include "common/item/instrument/InstrumentDefinition.hpp"
#include "common/item/instrument/InstrumentRegistry.hpp"
#include "common/item/items/special/bundle/BundleContents.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/nbt/NbtJsonUtils.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/blockentity/interactive/BannerPattern.hpp"
#include <algorithm>
#include <array>

#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>
#include <nlohmann/json_fwd.hpp>
#include <spdlog/spdlog.h>

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

namespace {

constexpr std::array<std::string_view, 16> _colorNames{"white",
    "orange",
    "magenta",
    "light_blue",
    "yellow",
    "lime",
    "pink",
    "gray",
    "light_gray",
    "cyan",
    "purple",
    "blue",
    "brown",
    "green",
    "red",
    "black"};

/// 读 compound 中名为 key 的整数列表（缺省/类型不符返回空列表）。
///
/// 三种列表类型都要认：`[I; ...]`、`[L; ...]` 与逐元素的通用 `list_tag`。早先只认
/// `int_list_tag`，别的形态一律当作空列表——`firework_explosion.colors` 一旦落成其它
/// 整数宽度就会被静默清空，而原版读的是 `List<Integer>`，只在宽度不符时报错。
[[nodiscard]] std::vector<i32> _readIntList(const compound_tag& compound, const char* key)
{
    std::vector<i32> result;
    auto it = compound.value.find(key);
    if (it == compound.value.end() || it->second->id() != TagId::List) {
        return result;
    }
    const auto& tag = *it->second;
    if (const auto* ints = dynamic_cast<const nbt::tags::int_list_tag*>(&tag); ints != nullptr) {
        return ints->value;
    }
    if (const auto* longs = dynamic_cast<const nbt::tags::long_list_tag*>(&tag); longs != nullptr) {
        result.reserve(longs->value.size());
        for (i64 value : longs->value) {
            result.push_back(static_cast<i32>(value));
        }
        return result;
    }
    if (const auto* generic = dynamic_cast<const nbt::tags::tag_list_tag*>(&tag); generic != nullptr) {
        for (const auto& element : generic->value) {
            if (element == nullptr) {
                continue;
            }
            switch (element->id()) {
                case TagId::Int:
                    result.push_back(dynamic_cast<const int_tag&>(*element).value);
                    break;
                case TagId::Long:
                    result.push_back(static_cast<i32>(dynamic_cast<const nbt::tags::long_tag&>(*element).value));
                    break;
                case TagId::Short:
                    result.push_back(dynamic_cast<const short_tag&>(*element).value);
                    break;
                case TagId::Byte:
                    result.push_back(dynamic_cast<const nbt::tags::byte_tag&>(*element).value);
                    break;
                default:
                    break;
            }
        }
    }
    return result;
}

/// 读 compound 中名为 key 的布尔值（Byte 语义：非 0 即真；缺省/类型不符返回 false）。
[[nodiscard]] bool _readBool(const compound_tag& compound, const char* key)
{
    auto it = compound.value.find(key);
    if (it == compound.value.end()) {
        return false;
    }
    if (it->second->id() == TagId::Byte) {
        return dynamic_cast<const nbt::tags::byte_tag&>(*it->second).value != 0;
    }
    return false;
}

} // namespace

std::unique_ptr<nbt::tags::tag> payloadToNbt(DataComponentType type, const DataComponentPayload& payload)
{
    switch (type) {
        case DataComponentType::Damage:
        case DataComponentType::RepairCost:
        case DataComponentType::MaxStackSize:
        case DataComponentType::MaxDamage: {
            const auto* p = std::get_if<i32>(&payload);
            return std::make_unique<nbt::tags::int_tag>(p ? *p : 0);
        }
        case DataComponentType::Enchantable: {
            // vanilla Enchantable.CODEC 是 record {value: int}，持久形态必须是复合对象
            // （裸 int 读回来会被判定为「不是 map」，组件整体丢失）。
            // 注意：网络协议上它仍是裸 VarInt，见 DataComponentPatchWire。
            const auto* p = std::get_if<i32>(&payload);
            auto compound = std::make_unique<compound_tag>();
            compound->put("value", p ? *p : 0);
            return compound;
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
        case DataComponentType::StoredEnchantments:
        case DataComponentType::Enchantments: {
            const auto& ench = std::get<item::enchant::EnchantmentContainer>(payload);
            return ench.toNbt();
        }
        case DataComponentType::AttackRange: {
            // 六个字段在原版都是 optionalFieldOf，**等于默认值就省略**（0 / 3 / 0 / 5 / 0.3 / 1）。
            // 注意：读回时 max_creative_reach 缺省取 5.0，所以「输入 5.0」与「不写」等价。
            const auto& ar = std::get<AttackRange>(payload);
            auto compound = std::make_unique<compound_tag>();
            // 六个字段在原版都是 optionalFieldOf，**等于默认值就省略**（0 / 3 / 0 / 5 / 0.3 / 1）。
            const auto putIfNotDefault = [&compound](const char* key, f32 value, f32 defaultValue) {
                if (value != defaultValue) {
                    compound->put(key, value);
                }
            };
            putIfNotDefault("min_reach", static_cast<f32>(ar.minRange), 0.0f);
            putIfNotDefault("max_reach", static_cast<f32>(ar.maxRange), 3.0f);
            putIfNotDefault("min_creative_reach", static_cast<f32>(ar.minCreativeRange), 0.0f);
            putIfNotDefault("max_creative_reach", static_cast<f32>(ar.maxCreativeRange), 5.0f);
            putIfNotDefault("hitbox_margin", static_cast<f32>(ar.hitboxMargin), 0.3f);
            putIfNotDefault("mob_factor", static_cast<f32>(ar.mobFactor), 1.0f);
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
        case DataComponentType::BundleContents: {
            auto list = std::make_unique<nbt::tags::compound_list_tag>();
            const auto contents = items::BundleContents::fromJson(std::get<nlohmann::json>(payload));
            for (const auto& stack : contents.items()) {
                compound_tag value;
                stack.toNbt(value);
                list->value.push_back(std::move(value));
            }
            return list;
        }
        case DataComponentType::WritableBookContent:
        case DataComponentType::WrittenBookContent:
            return nbt::jsonToNbt(std::get<nlohmann::json>(payload));
        case DataComponentType::BannerPatterns: {
            auto layers = nlohmann::json::array();
            for (const auto& layer : std::get<nlohmann::json>(payload)) {
                const auto type = blockentity::BannerPatterns::byHash(layer.at("Pattern").get<std::string>());
                layers.push_back(
                    {{"pattern",
                         layer.contains("DirectPattern")
                             ? layer.at("DirectPattern")
                             : nlohmann::json("minecraft:" + blockentity::BannerPatterns::getFileName(type))},
                        {"color", _colorNames.at(static_cast<usize>(layer.at("Color").get<i32>()))}});
            }
            auto list = std::make_unique<nbt::tags::compound_list_tag>();
            for (const auto& layer : layers) {
                auto compound = nbt::jsonToNbt(layer);
                list->value.push_back(std::move(*compound));
            }
            return list;
        }
        case DataComponentType::CustomData: {
            const auto& j = std::get<nlohmann::json>(payload);
            if (j.is_object() && !j.empty()) {
                // 整数必须落 Int：`jsonToNbt` 按值域把 3 缩成 Byte（`3f` 的意图是 Float，
                // 但 `{"level": 3}` 是整数），原版的 custom_data 是**原样**的 NBT，
                // 存档里写的是 `3`（Int）而不是 `3b`。用严格整数转换保持宽度。
                auto nbt = nbt::jsonToNbtStrictInt(j);
                if (nbt != nullptr && nbt->id() == nbt::TagId::Compound) {
                    return nbt;
                }
            }
            return std::make_unique<compound_tag>();
        }
        case DataComponentType::FireworkExplosion: {
            // vanilla FireworkExplosion.CODEC：shape(默认 small_ball) + colors/fade_colors
            // (int 列表，默认空) + has_trail/has_twinkle(默认 false)，全部 optionalFieldOf。
            // 四个可选字段（colors / fade_colors / has_trail / has_twinkle）原版都是
            // optionalFieldOf：**等于默认值就省略**。全写出来会让字节与原版不同（语义相同，
            // 但存档/网络逐字节比对会判不一致），因此只写非默认值。
            const auto& explosion = std::get<FireworkExplosion>(payload);
            auto compound = std::make_unique<compound_tag>();
            compound->put("shape", std::string(fireworkShapeName(explosion.shape)));
            if (!explosion.colors.empty()) {
                auto colors = std::make_unique<nbt::tags::int_list_tag>(explosion.colors);
                compound->value.emplace("colors", std::move(colors));
            }
            if (!explosion.fadeColors.empty()) {
                auto fadeColors = std::make_unique<nbt::tags::int_list_tag>(explosion.fadeColors);
                compound->value.emplace("fade_colors", std::move(fadeColors));
            }
            if (explosion.hasTrail) {
                compound->put("has_trail", static_cast<i8>(1));
            }
            if (explosion.hasTwinkle) {
                compound->put("has_twinkle", static_cast<i8>(1));
            }
            return compound;
        }
        case DataComponentType::CustomModelData: {
            // vanilla CustomModelData.CODEC：floats 可选（默认空）/ strings 可选 / flags 可选（默认空）。
            // 未设置的子字段不写出，与表驱动组件的省略语义一致。
            const auto& modelData = std::get<nlohmann::json>(payload);
            auto compound = std::make_unique<compound_tag>();
            for (const char* key : {"floats", "colors"}) {
                auto it = modelData.find(key);
                if (it == modelData.end() || it->is_null()) {
                    continue;
                }
                // floats/colors 是数值列表（Float / RGB 整数），保持元素原类型。
                auto encoded = nbt::jsonToNbtStrictInt(*it);
                if (encoded != nullptr) {
                    compound->value.emplace(key, std::move(encoded));
                }
            }
            // flags 是 BOOL 列表：原版 codec 读 `List<Boolean>`，NBT 上必须是逐元素 byte
            // 的字节列表。jsonToNbtStrictInt 对布尔数组会走「通用列表」，元素是 byte_tag，
            // 但外层是 tag_list_tag——读回时类型对不上，原版整组读空。这里显式构造字节列表。
            if (auto flagsIt = modelData.find("flags"); flagsIt != modelData.end() && flagsIt->is_array()) {
                auto flags = std::make_unique<nbt::tags::byte_list_tag>();
                for (const auto& flag : *flagsIt) {
                    flags->value.push_back(flag.is_boolean() && flag.get<bool>() ? 1 : 0);
                }
                compound->value.emplace("flags", std::move(flags));
            }
            if (auto stringsIt = modelData.find("strings"); stringsIt != modelData.end() && stringsIt->is_array()) {
                auto strings = std::make_unique<nbt::tags::string_list_tag>();
                for (const auto& text : *stringsIt) {
                    strings->value.push_back(text.is_string() ? text.get<std::string>() : std::string{});
                }
                compound->value.emplace("strings", std::move(strings));
            }
            return compound;
        }
        case DataComponentType::EnchantmentGlintOverride:
        case DataComponentType::IntangibleProjectile: {
            const auto* value = std::get_if<bool>(&payload);
            return std::make_unique<nbt::tags::byte_tag>(static_cast<i8>((value != nullptr && *value) ? 1 : 0));
        }
        case DataComponentType::Instrument: {
            // 持久形态是 Holder<Instrument> 的完整定义（sound_event / use_duration / range /
            // description），不是资源位置字符串。写成字符串能被写进存档，但读回来
            // EitherHolder 会当成「只有注册名、没有内联定义」，与原版存档互不兼容。
            const auto& instrument = std::get<InstrumentPayload>(payload);
            const item::instrument::InstrumentDefinition* definition =
                item::instrument::InstrumentRegistry::instance().get(ResourceLocation::parse(instrument.instrumentId));
            if (definition == nullptr) {
                // 注册表里没有该乐器：只能退回资源位置字符串，避免丢掉「引用了哪个乐器」。
                // TODO: 数据包把乐器定义内联进组件时（EitherHolder 的左值分支），需要在这里
                //       序列化内联定义，而不是依赖注册表命中。
                return std::make_unique<string_tag>(instrument.instrumentId);
            }
            // 四个字段的类型逐一对照 vanilla Instrument.DIRECT_CODEC：
            //   sound_event -> SoundEvent.CODEC，持久形态是 {sound_id: "..."} 复合对象
            //                  （不是裸字符串）；
            //   description -> ComponentSerialization，文本组件对象（不是 JSON 字符串）；
            //   use_duration / range -> 裸 float。
            auto compound = std::make_unique<compound_tag>();
            auto soundEvent = std::make_unique<compound_tag>();
            soundEvent->put("sound_id", definition->soundEvent().toString());
            compound->value.emplace("sound_event", std::move(soundEvent));
            compound->put("use_duration", definition->useDuration());
            compound->put("range", definition->range());
            if (definition->description() != nullptr) {
                compound->value.emplace("description", nbt::jsonToNbtStrictInt(definition->description()->toJson()));
            }
            return compound;
        }
        case DataComponentType::Tool: {
            // vanilla Tool.CODEC：rules 必填；default_mining_speed(默认 1.0) /
            // damage_per_block(默认 1) / can_destroy_blocks_in_creative(默认 true) 可省略。
            // Rule.CODEC：blocks 为方块标签或方块 ID 列表；speed / correct_for_drops 可省略。
            const auto& tool = std::get<ToolPayload>(payload);
            auto compound = std::make_unique<compound_tag>();
            auto rules = std::make_unique<nbt::tags::compound_list_tag>();
            for (const auto& rule : tool.rules) {
                auto ruleTag = std::make_unique<compound_tag>();
                auto blocks = std::make_unique<string_list_tag>();
                for (const auto& id : rule.blocks) {
                    // 解码时不保留 "#" 前缀，编码时按"是否为已注册标签"恢复：
                    // 标签在 BlockTags 中登记，普通方块 ID 不会与之同名
                    const std::string formatted = id.toString();
                    blocks->value.push_back(BlockTags::getTag(id) != nullptr ? "#" + formatted : formatted);
                }
                ruleTag->value.emplace("blocks", std::move(blocks));
                if (rule.speed.has_value()) {
                    ruleTag->put("speed", *rule.speed);
                }
                if (rule.correctForDrops.has_value()) {
                    ruleTag->put("correct_for_drops", static_cast<i8>(*rule.correctForDrops ? 1 : 0));
                }
                rules->value.push_back(std::move(*ruleTag));
            }
            compound->value.emplace("rules", std::move(rules));
            compound->put("default_mining_speed", tool.defaultMiningSpeed);
            compound->put("damage_per_block", tool.damagePerBlock);
            compound->put("can_destroy_blocks_in_creative", static_cast<i8>(tool.canDestroyBlocksInCreative ? 1 : 0));
            return compound;
        }
        case DataComponentType::SuspiciousStewEffects: {
            // SuspiciousStewEffects.Entry.CODEC：{ id: 效果注册表键, duration: 默认 160 }
            auto list = std::make_unique<nbt::tags::compound_list_tag>();
            for (const auto& effect : std::get<SuspiciousStewEffectsPayload>(payload)) {
                nbt::tags::compound_tag effectTag;
                effectTag.put("id", entity::effect::getEffectResourceLocation(effect.type()).toString());
                effectTag.put("duration", effect.duration());
                list->value.push_back(std::move(effectTag));
            }
            return list;
        }
        case DataComponentType::DeathProtection: {
            // DeathProtection.CODEC：{ death_effects: [ConsumeEffect...] }（可选，默认空列表）
            const auto& deathProtection = std::get<DeathProtectionPayload>(payload);
            auto compound = std::make_unique<compound_tag>();
            auto encoded = nbt::jsonToNbtStrictInt(consumeEffectsToJson(deathProtection.effects));
            if (encoded != nullptr) {
                compound->value.emplace("death_effects", std::move(encoded));
            }
            return compound;
        }
        case DataComponentType::OminousBottleAmplifier:
        case DataComponentType::MapId: {
            // 可选整数：未设置时写空 compound（vanilla optionalFieldOf 的空值表示）
            const auto& value = std::get<std::optional<i32>>(payload);
            if (!value.has_value()) {
                return std::make_unique<compound_tag>();
            }
            return std::make_unique<nbt::tags::int_tag>(*value);
        }
        case DataComponentType::Bees:
        case DataComponentType::Trim:
        case DataComponentType::Recipes: {
            // 未落地结构化载荷的组件：以原始 NBT（JSON 形式）承载，保证 copy_components /
            // set_components 能原样复制数据包组件而不丢失内容
            const auto& raw = std::get<nlohmann::json>(payload);
            auto encoded = nbt::jsonToNbt(raw);
            return encoded != nullptr ? std::move(encoded) : std::make_unique<compound_tag>();
        }
        case DataComponentType::Fireworks: {
            // vanilla Fireworks.CODEC：flight_duration(UNSIGNED_BYTE，默认 0，NBT 上是 byte)
            // + explosions(FireworkExplosion 列表，上限 256，默认空)。
            // flight_duration 与 explosions 同样是 optionalFieldOf：0 与空列表省略。
            const auto& fireworks = std::get<Fireworks>(payload);
            auto compound = std::make_unique<compound_tag>();
            if (fireworks.flightDuration != 0) {
                compound->put("flight_duration", static_cast<i8>(fireworks.flightDuration));
            }
            if (!fireworks.explosions.empty()) {
                auto explosions = std::make_unique<nbt::tags::compound_list_tag>();
                for (const auto& explosion : fireworks.explosions) {
                    auto element = std::make_unique<nbt::tags::compound_tag>();
                    auto encoded = payloadToNbt(DataComponentType::FireworkExplosion,
                        DataComponentPayload{std::in_place_index<ComponentPayloadIndex::FireworkExplosion>, explosion});
                    if (encoded != nullptr && encoded->id() == TagId::Compound) {
                        *element = dynamic_cast<const compound_tag&>(*encoded);
                    }
                    explosions->value.push_back(std::move(*element));
                }
                compound->value.emplace("explosions", std::move(explosions));
            }
            return compound;
        }
        default:
            // 未落地结构化载荷的组件
            break;
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
            // Enchantable 的持久形态是 {value: n}，其它四个是裸整数；两者都接受。
            if (tag.id() == TagId::Compound) {
                const auto& compound = dynamic_cast<const compound_tag&>(tag);
                auto it = compound.value.find("value");
                if (it != compound.value.end() && it->second->id() == TagId::Int) {
                    return DataComponentPayload{std::in_place_index<ComponentPayloadIndex::Int>,
                        dynamic_cast<const int_tag&>(*it->second).value};
                }
            }
            if (tag.id() == TagId::Int) {
                return DataComponentPayload{
                    std::in_place_index<ComponentPayloadIndex::Int>, dynamic_cast<const int_tag&>(tag).value};
            }
            if (tag.id() == TagId::Short) {
                return DataComponentPayload{std::in_place_index<ComponentPayloadIndex::Int>,
                    static_cast<i32>(dynamic_cast<const short_tag&>(tag).value)};
            }
            return DataComponentPayload{std::in_place_index<ComponentPayloadIndex::Int>, 0};
        }
        case DataComponentType::CustomModelData: {
            if (tag.id() == TagId::Compound) {
                auto decoded = nbt::nbtToJson(tag);
                // flags 在 NBT 上是字节列表，nbtToJson 会把它解成 [1,0]；原版语义是布尔，
                // 载荷里必须还原成 [true,false]，否则读回后再编码会把「true」写成 0。
                if (auto flagsIt = decoded.find("flags"); flagsIt != decoded.end() && flagsIt->is_array()) {
                    for (auto& flag : *flagsIt) {
                        flag = flag.is_number() && flag.get<double>() != 0;
                    }
                }
                return DataComponentPayload{std::in_place_index<ComponentPayloadIndex::Json>, std::move(decoded)};
            }
            return DataComponentPayload{std::in_place_index<ComponentPayloadIndex::Json>, nlohmann::json::object()};
        }
        case DataComponentType::EnchantmentGlintOverride:
        case DataComponentType::IntangibleProjectile: {
            // NBT 中 Codec.BOOL 落为 byte，但数据包 JSON 经 jsonToNbtStrictInt/jsonToNbt
            // 可能落为 int；两者都接受
            bool value = false;
            if (tag.id() == TagId::Byte) {
                value = dynamic_cast<const nbt::tags::byte_tag&>(tag).value != 0;
            } else if (tag.id() == TagId::Int) {
                value = dynamic_cast<const int_tag&>(tag).value != 0;
            }
            return DataComponentPayload{std::in_place_index<ComponentPayloadIndex::Bool>, value};
        }
        case DataComponentType::Instrument: {
            // 两种形态都要认：字符串是「只给注册名」，compound 是内联的完整定义
            // （sound_event / use_duration / range / description），见 payloadToNbt。
            std::string instrument;
            if (tag.id() == TagId::String) {
                instrument = dynamic_cast<const string_tag&>(tag).value;
            } else if (tag.id() == TagId::Compound) {
                // TODO: 内联定义目前无法反查到注册名——InstrumentPayload 只承载资源位置。
                //       要支持内联定义，需要让载荷也能持有完整定义（而不是只记一个 id）。
                //       在此之前按「空引用」处理，至少不会把 player 侧的随机记忆带进结果。
                const auto& c = dynamic_cast<const compound_tag&>(tag);
                auto sound = c.value.find("sound_event");
                static_cast<void>(sound);
            }
            return DataComponentPayload{
                std::in_place_index<ComponentPayloadIndex::Instrument>, InstrumentPayload{std::move(instrument)}};
        }
        case DataComponentType::Tool: {
            ToolPayload tool{};
            if (tag.id() == TagId::Compound) {
                const auto& c = dynamic_cast<const compound_tag&>(tag);
                auto rulesIt = c.value.find("rules");
                if (rulesIt != c.value.end() && rulesIt->second->id() == TagId::List) {
                    if (const auto* list = dynamic_cast<const nbt::tags::compound_list_tag*>(rulesIt->second.get())) {
                        for (const auto& ruleTag : list->value) {
                            ToolRule rule{};
                            auto blocksIt = ruleTag.value.find("blocks");
                            if (blocksIt != ruleTag.value.end() && blocksIt->second->id() == TagId::List) {
                                const auto& sl = dynamic_cast<const string_list_tag&>(*blocksIt->second);
                                for (const auto& entry : sl.value) {
                                    // "#namespace:path" 为标签引用，否则为方块 ID
                                    if (!entry.empty() && entry[0] == '#') {
                                        rule.blocks.push_back(ResourceLocation::parse(entry.substr(1)));
                                    } else {
                                        rule.blocks.push_back(ResourceLocation::parse(entry));
                                    }
                                }
                            }
                            auto speedIt = ruleTag.value.find("speed");
                            if (speedIt != ruleTag.value.end() && speedIt->second->id() == TagId::Float) {
                                rule.speed = dynamic_cast<const float_tag&>(*speedIt->second).value;
                            }
                            auto dropsIt = ruleTag.value.find("correct_for_drops");
                            if (dropsIt != ruleTag.value.end() && dropsIt->second->id() == TagId::Byte) {
                                rule.correctForDrops =
                                    dynamic_cast<const nbt::tags::byte_tag&>(*dropsIt->second).value != 0;
                            }
                            tool.rules.push_back(std::move(rule));
                        }
                    }
                }
                auto speedIt = c.value.find("default_mining_speed");
                if (speedIt != c.value.end()) {
                    // 数据包写整数（`default_mining_speed: 1`）时落成 IntTag，只认 Float 会
                    // 把非默认值静默退回 1.0。
                    if (speedIt->second->id() == TagId::Float) {
                        tool.defaultMiningSpeed = dynamic_cast<const float_tag&>(*speedIt->second).value;
                    } else if (speedIt->second->id() == TagId::Double) {
                        tool.defaultMiningSpeed =
                            static_cast<f32>(dynamic_cast<const nbt::tags::double_tag&>(*speedIt->second).value);
                    } else if (speedIt->second->id() == TagId::Int) {
                        tool.defaultMiningSpeed =
                            static_cast<f32>(dynamic_cast<const int_tag&>(*speedIt->second).value);
                    }
                }
                auto damageIt = c.value.find("damage_per_block");
                if (damageIt != c.value.end() && damageIt->second->id() == TagId::Int) {
                    tool.damagePerBlock = dynamic_cast<const int_tag&>(*damageIt->second).value;
                }
                auto creativeIt = c.value.find("can_destroy_blocks_in_creative");
                if (creativeIt != c.value.end() && creativeIt->second->id() == TagId::Byte) {
                    tool.canDestroyBlocksInCreative =
                        dynamic_cast<const nbt::tags::byte_tag&>(*creativeIt->second).value != 0;
                }
            }
            return DataComponentPayload{std::in_place_index<ComponentPayloadIndex::Tool>, std::move(tool)};
        }
        case DataComponentType::SuspiciousStewEffects: {
            // SuspiciousStewEffects.Entry.CODEC：{ id: 效果注册表键, duration: 默认 160 }
            SuspiciousStewEffectsPayload effects;
            if (const auto* list = dynamic_cast<const nbt::tags::compound_list_tag*>(&tag)) {
                for (const auto& value : list->value) {
                    auto idIt = value.value.find("id");
                    if (idIt == value.value.end() || idIt->second->id() != TagId::String) {
                        continue;
                    }
                    const auto& effectId = dynamic_cast<const string_tag&>(*idIt->second).value;
                    auto effectType = entity::effect::getEffectByResourceLocation(ResourceLocation::parse(effectId));
                    if (!effectType.has_value()) {
                        spdlog::warn("Ignoring suspicious stew effect with unknown id '{}'", effectId);
                        continue;
                    }
                    i32 duration = 160;
                    auto durationIt = value.value.find("duration");
                    if (durationIt != value.value.end() && durationIt->second->id() == TagId::Int) {
                        duration = dynamic_cast<const int_tag&>(*durationIt->second).value;
                    }
                    effects.push_back(
                        entity::effect::EffectInstance(effectType.value(), duration, 0, false, true, true));
                }
            }
            return DataComponentPayload{std::in_place_index<ComponentPayloadIndex::StewEffects>, std::move(effects)};
        }
        case DataComponentType::DeathProtection: {
            // DeathProtection.CODEC：{ death_effects: [ConsumeEffect...] }；缺省或非 compound 时为空列表。
            DeathProtectionPayload deathProtection;
            if (tag.id() == TagId::Compound) {
                const auto& c = dynamic_cast<const compound_tag&>(tag);
                auto effectsIt = c.value.find("death_effects");
                if (effectsIt != c.value.end()) {
                    deathProtection.effects = consumeEffectsFromJson(nbt::nbtToJson(*effectsIt->second));
                }
            }
            return DataComponentPayload{
                std::in_place_index<ComponentPayloadIndex::DeathProtection>, std::move(deathProtection)};
        }
        case DataComponentType::OminousBottleAmplifier:
        case DataComponentType::MapId: {
            if (tag.id() == TagId::Int) {
                return DataComponentPayload{std::in_place_index<ComponentPayloadIndex::OptionalInt>,
                    std::optional<i32>(dynamic_cast<const int_tag&>(tag).value)};
            }
            if (tag.id() == TagId::Byte) {
                return DataComponentPayload{std::in_place_index<ComponentPayloadIndex::OptionalInt>,
                    std::optional<i32>(static_cast<i32>(dynamic_cast<const nbt::tags::byte_tag&>(tag).value))};
            }
            return DataComponentPayload{std::in_place_index<ComponentPayloadIndex::OptionalInt>, std::nullopt};
        }
        case DataComponentType::Bees:
        case DataComponentType::Trim:
        case DataComponentType::Recipes:
            return DataComponentPayload{std::in_place_index<ComponentPayloadIndex::Json>, nbt::nbtToJson(tag)};
        case DataComponentType::Unbreakable: {
            // Unit 组件：存在即不可损坏
            return DataComponentPayload{std::in_place_index<ComponentPayloadIndex::Bool>, true};
        }
        case DataComponentType::Rarity: {
            ItemRarity rarity = ItemRarity::Common;
            if (tag.id() == TagId::String) {
                const auto& s = dynamic_cast<const string_tag&>(tag).value;
                rarity = rarityFromName(s).value_or(ItemRarity::Common);
            }
            return DataComponentPayload{std::in_place_index<ComponentPayloadIndex::Rarity>, rarity};
        }
        case DataComponentType::ItemModel: {
            std::string model;
            if (tag.id() == TagId::String) {
                model = dynamic_cast<const string_tag&>(tag).value;
            }
            return DataComponentPayload{std::in_place_index<ComponentPayloadIndex::Identifier>, std::move(model)};
        }
        case DataComponentType::AttackRange: {
            AttackRange ar{};
            if (tag.id() == TagId::Compound) {
                const auto& c = dynamic_cast<const compound_tag&>(tag);
                // 三种数值宽度都接受：数据包写 `max_reach: 4`（整数字面量）时落成 IntTag，
                // 只认 Float 会让这个字段静默回到默认值。
                auto readF32 = [&c](const char* key, f32 fallback) -> f32 {
                    auto it = c.value.find(key);
                    if (it == c.value.end()) {
                        return fallback;
                    }
                    switch (it->second->id()) {
                        case TagId::Float:
                            return dynamic_cast<const float_tag&>(*it->second).value;
                        case TagId::Double:
                            return static_cast<f32>(dynamic_cast<const nbt::tags::double_tag&>(*it->second).value);
                        case TagId::Int:
                            return static_cast<f32>(dynamic_cast<const int_tag&>(*it->second).value);
                        default:
                            return fallback;
                    }
                };
                ar.minRange = readF32("min_reach", ar.minRange);
                ar.maxRange = readF32("max_reach", ar.maxRange);
                ar.minCreativeRange = readF32("min_creative_reach", ar.minCreativeRange);
                ar.maxCreativeRange = readF32("max_creative_reach", ar.maxCreativeRange);
                ar.hitboxMargin = readF32("hitbox_margin", ar.hitboxMargin);
                ar.mobFactor = readF32("mob_factor", ar.mobFactor);
            }
            return DataComponentPayload{std::in_place_index<ComponentPayloadIndex::AttackRange>, ar};
        }
        case DataComponentType::CustomName:
        case DataComponentType::ItemName: {
            if (tag.id() == TagId::String) {
                const auto& s = dynamic_cast<const string_tag&>(tag).value;
                if (!s.empty()) {
                    // 写侧存的是组件 JSON 字符串（见 payloadToNbt），这里用
                    // ITextComponent::fromJson 解析回组件；TextParser::parse 只认
                    // legacy § 格式，会把 JSON 当纯文本导致 getUnformattedText 退化为 JSON 原文。
                    return DataComponentPayload{std::in_place_index<ComponentPayloadIndex::TextComponent>,
                        text::ITextComponent::fromJson(nlohmann::json::parse(s))};
                }
            }
            return DataComponentPayload{std::in_place_index<ComponentPayloadIndex::TextComponent>, nullptr};
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
            return DataComponentPayload{
                std::in_place_index<ComponentPayloadIndex::TextComponentList>, std::move(lines)};
        }
        case DataComponentType::StoredEnchantments:
        case DataComponentType::Enchantments: {
            if (tag.id() == TagId::List) {
                return DataComponentPayload{std::in_place_index<ComponentPayloadIndex::Enchantments>,
                    item::enchant::EnchantmentContainer::fromNbt(dynamic_cast<const list_tag&>(tag))};
            }
            return DataComponentPayload{
                std::in_place_index<ComponentPayloadIndex::Enchantments>, item::enchant::EnchantmentContainer{}};
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
            return DataComponentPayload{std::in_place_index<ComponentPayloadIndex::PotionContents>, std::move(pc)};
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
            return DataComponentPayload{std::in_place_index<ComponentPayloadIndex::AdventurePredicate>,
                AdventureModePredicate(std::move(preds))};
        }
        case DataComponentType::BundleContents: {
            std::vector<ItemStack> items;
            if (const auto* list = dynamic_cast<const nbt::tags::compound_list_tag*>(&tag)) {
                for (const auto& value : list->value) {
                    auto stack = ItemStack::fromNbt(value);
                    if (stack.success()) {
                        items.push_back(std::move(stack).value());
                    }
                }
            }
            return DataComponentPayload{std::in_place_index<ComponentPayloadIndex::Json>,
                item::items::BundleContents(std::move(items)).toJson()};
        }
        case DataComponentType::WritableBookContent:
        case DataComponentType::WrittenBookContent:
            return DataComponentPayload{std::in_place_index<ComponentPayloadIndex::Json>, nbt::nbtToJson(tag)};
        case DataComponentType::BannerPatterns: {
            auto layers = nlohmann::json::array();
            const auto saved = nbt::nbtToJson(tag);
            if (!saved.is_array()) {
                spdlog::warn("Ignoring banner patterns that are not an NBT list");
                return DataComponentPayload{std::in_place_index<ComponentPayloadIndex::Json>, std::move(layers)};
            }
            for (const auto& layer : saved) {
                if (!layer.is_object() || !layer.contains("pattern") || !layer.contains("color") ||
                    !layer["color"].is_string()) {
                    spdlog::warn("Ignoring malformed saved banner layer");
                    continue;
                }
                auto value = nlohmann::json::object();
                const auto& pattern = layer.at("pattern");
                if (pattern.is_object()) {
                    if (!pattern.contains("asset_id") || !pattern["asset_id"].is_string() ||
                        !pattern.contains("translation_key") || !pattern["translation_key"].is_string()) {
                        spdlog::warn("Ignoring malformed direct banner pattern");
                        continue;
                    }
                    value["DirectPattern"] = pattern;
                    value["Pattern"] = "b";
                } else if (pattern.is_string()) {
                    for (i32 type = 0; type < static_cast<i32>(blockentity::BannerPatternType::Count); ++type) {
                        const auto entry = static_cast<blockentity::BannerPatternType>(type);
                        if ("minecraft:" + blockentity::BannerPatterns::getFileName(entry) ==
                            pattern.get<std::string>()) {
                            value["Pattern"] = blockentity::BannerPatterns::getHashName(entry);
                        }
                    }
                }
                const auto color = layer.at("color").get<std::string>();
                const auto it = std::find(_colorNames.begin(), _colorNames.end(), color);
                if (!value.contains("Pattern") || it == _colorNames.end()) {
                    spdlog::warn("Ignoring saved banner layer with an unknown pattern or color");
                    continue;
                }
                value["Color"] = static_cast<i32>(std::distance(_colorNames.begin(), it));
                layers.push_back(std::move(value));
            }
            return DataComponentPayload{std::in_place_index<ComponentPayloadIndex::Json>, std::move(layers)};
        }
        case DataComponentType::CustomData: {
            if (tag.id() == TagId::Compound) {
                return DataComponentPayload{std::in_place_index<ComponentPayloadIndex::Json>, nbt::nbtToJson(tag)};
            }
            return DataComponentPayload{std::in_place_index<ComponentPayloadIndex::Json>, nlohmann::json::object()};
        }
        case DataComponentType::FireworkExplosion: {
            FireworkExplosion explosion{};
            if (tag.id() == TagId::Compound) {
                const auto& c = dynamic_cast<const compound_tag&>(tag);
                auto shapeIt = c.value.find("shape");
                if (shapeIt != c.value.end() && shapeIt->second->id() == TagId::String) {
                    const auto name = dynamic_cast<const string_tag&>(*shapeIt->second).value;
                    explosion.shape = fireworkShapeFromName(name).value_or(FireworkShape::SmallBall);
                }
                explosion.colors = _readIntList(c, "colors");
                explosion.fadeColors = _readIntList(c, "fade_colors");
                explosion.hasTrail = _readBool(c, "has_trail");
                explosion.hasTwinkle = _readBool(c, "has_twinkle");
            }
            return DataComponentPayload{
                std::in_place_index<ComponentPayloadIndex::FireworkExplosion>, std::move(explosion)};
        }
        case DataComponentType::Fireworks: {
            Fireworks fireworks{};
            if (tag.id() == TagId::Compound) {
                const auto& c = dynamic_cast<const compound_tag&>(tag);
                auto flightIt = c.value.find("flight_duration");
                if (flightIt != c.value.end()) {
                    // vanilla UNSIGNED_BYTE：NBT 上是 byte（0..255 无符号解读）
                    if (flightIt->second->id() == TagId::Byte) {
                        fireworks.flightDuration =
                            static_cast<u8>(dynamic_cast<const nbt::tags::byte_tag&>(*flightIt->second).value);
                    } else if (flightIt->second->id() == TagId::Int) {
                        fireworks.flightDuration = dynamic_cast<const int_tag&>(*flightIt->second).value;
                    }
                }
                auto explosionsIt = c.value.find("explosions");
                if (explosionsIt != c.value.end() && explosionsIt->second->id() == TagId::List) {
                    const auto* list = dynamic_cast<const nbt::tags::compound_list_tag*>(explosionsIt->second.get());
                    if (list != nullptr) {
                        for (const auto& element : list->value) {
                            if (fireworks.explosions.size() >= static_cast<usize>(Fireworks::MAX_EXPLOSIONS)) {
                                spdlog::warn(
                                    "Ignoring firework explosions beyond the maximum of {}", Fireworks::MAX_EXPLOSIONS);
                                break;
                            }
                            auto payload = nbtToPayload(DataComponentType::FireworkExplosion, element);
                            fireworks.explosions.push_back(std::get<FireworkExplosion>(payload));
                        }
                    }
                }
            }
            return DataComponentPayload{std::in_place_index<ComponentPayloadIndex::Fireworks>, std::move(fireworks)};
        }
        default:
            // 未落地结构化载荷的组件（如 weapon / equippable / food 等）
            return DataComponentPayload{};
    }
}

bool tryNbtToPayload(DataComponentType type, const nbt::tags::tag& tag, DataComponentPayload& out)
{
    // nbtToPayload 未落地的分支返回 monostate 空载荷，这里据此判定"无法解码"
    DataComponentPayload payload = nbtToPayload(type, tag);
    if (std::holds_alternative<std::monostate>(payload)) {
        return false;
    }
    out = std::move(payload);
    return true;
}

} // namespace detail
} // namespace component
} // namespace item
} // namespace mc
