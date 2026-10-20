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

#pragma once

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/item/component/AttackRange.hpp"
#include "common/item/component/DataComponentType.hpp"
#include "common/item/component/FireworkExplosion.hpp"
#include "common/item/component/Fireworks.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/item/core/ItemRarity.hpp"
#include "common/item/enchantment/EnchantmentContainer.hpp"
#include "common/util/text/ITextComponent.hpp"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace item {
namespace component {

// 容器 / 充能投射物载荷（元素为 ItemStack 指针），由 ItemStack 头文件在自身定义完成后另行声明
struct ItemContainerContentsPayload;
struct ChargedProjectilesPayload;

/**
 * @brief potion_contents 组件载荷
 *
 * 对应 Java 1.21.11 record PotionContents(Optional<Holder<Potion>> potion,
 * Optional<Integer> customColor, List<MobEffectInstance> customEffects,
 * Optional<String> customName)。
 *
 * potion：药水 holder（vanilla Potions 静态注册表 id）。本项目以资源位置字符串承载
 *   （"minecraft:night_vision"），wire codec 经 JavaPotionIdMap 转 vanilla registry id。
 * customColor：自定义颜色（ARGB），nullopt=无（用效果默认色）。
 * customEffects：自定义效果列表（独立于 potion 自带效果，叠加生效）。
 * customName：自定义药水名（显示用），nullopt=无。
 */
struct PotionContentsPayload {
    std::string potionId; ///< 药水资源位置（空串=无）
    std::optional<i32> customColor;
    std::vector<entity::effect::EffectInstance> customEffects;
    std::optional<std::string> customName;
};

/**
 * @brief instrument 组件载荷
 *
 * 对应 Java 1.21.11 Holder<Instrument>。项目暂无乐器注册表，以乐器注册表键的
 * 资源位置字符串承载，山羊角等乐器物品按注册表键解析音色。
 */
struct InstrumentPayload {
    std::string instrumentId; ///< 乐器的资源位置（如 "minecraft:ponder_goat_horn"）
};

/**
 * @brief tool 组件的单条规则
 *
 * 对应 Java 1.21.11 Tool.Rule：blocks 为方块标签或方块 ID 集合（"#" 前缀已剥离），
 * speed / correctForDrops 为可选的覆盖值。
 */
struct ToolRule {
    std::vector<ResourceLocation> blocks;
    std::optional<f32> speed;
    std::optional<bool> correctForDrops;
};

/**
 * @brief tool 组件载荷
 *
 * 对应 Java 1.21.11 Tool(List<Rule> rules, float defaultMiningSpeed,
 * int damagePerBlock, boolean canDestroyBlocksInCreative)。
 */
struct ToolPayload {
    std::vector<ToolRule> rules;
    f32 defaultMiningSpeed = 1.0f;
    i32 damagePerBlock = 1;
    bool canDestroyBlocksInCreative = true;
};

/**
 * @brief suspicious_stew_effects 组件载荷
 *
 * 对应 Java 1.21.11 的 List<MobEffectInstance>（每个效果带独立持续时间）。
 */
using SuspiciousStewEffectsPayload = std::vector<entity::effect::EffectInstance>;

/**
 * @brief 消耗效果（ConsumeEffect）类型
 *
 * 对应 Java 1.21.11 net.minecraft.world.item.consume_effects.ConsumeEffect 的 5 种实现。
 * death_protection 组件的 death_effects 与食物组件的 consume_effects 共用此结构。
 */
enum class ConsumeEffectKind : u8 {
    ApplyEffects,     ///< apply_effects：按概率施加效果列表
    RemoveEffects,    ///< remove_effects：移除指定效果
    ClearAllEffects,  ///< clear_all_effects：清空全部效果
    TeleportRandomly, ///< teleport_randomly：随机传送
    PlaySound,        ///< play_sound：播放音效
};

/**
 * @brief 单条消耗效果
 *
 * 字段按 kind 取用，未使用字段保持默认值：
 * - ApplyEffects    → effects（条目内 chance 为独立施加概率）
 * - RemoveEffects   → removedEffects
 * - TeleportRandomly→ teleportDiameter（vanilla 默认 16.0）
 * - PlaySound       → soundId（音效资源位置）
 */
struct ConsumeEffectEntry {
    /// apply_effects 的单个效果条目：instance 缺失（数据包条目解析失败）时不施加。
    struct EffectEntry {
        std::optional<entity::effect::EffectInstance> instance;
        f32 chance = 1.0f; ///< 该条目独立施加概率（vanilla MobEffectInstance 条目默认 1.0）
    };

    ConsumeEffectKind kind = ConsumeEffectKind::ApplyEffects;
    std::vector<EffectEntry> effects;                       ///< apply_effects
    std::vector<entity::effect::EffectType> removedEffects; ///< remove_effects
    f32 teleportDiameter = 16.0f;                           ///< teleport_randomly
    std::string soundId;                                    ///< play_sound
};

/**
 * @brief death_protection 组件载荷
 *
 * 对应 Java 1.21.11 DeathProtection(List<ConsumeEffect> deathEffects)：
 * 物品持有者在致死伤害时消耗一件，并把 death_effects 依次施加到持有者身上。
 * 不死图腾的默认值见 item::component::totemOfUndyingDeathProtection()。
 */
struct DeathProtectionPayload {
    std::vector<ConsumeEffectEntry> effects;
};

/// 解析 ConsumeEffect 列表（vanilla ConsumeEffect.CODEC.listOf()）。
/// 条目缺少 type 或条目结构非法时，对应条目被丢弃（对齐 vanilla Codec 的条目级容错）。
[[nodiscard]] std::vector<ConsumeEffectEntry> consumeEffectsFromJson(const nlohmann::json& json);

/// 序列化 ConsumeEffect 列表（与 consumeEffectsFromJson 互逆）。
[[nodiscard]] nlohmann::json consumeEffectsToJson(const std::vector<ConsumeEffectEntry>& effects);

/**
 * @brief 不死图腾的默认 death_protection 组件值
 *
 * 对齐 Java 1.21.11 DeathProtection.TOTEM_OF_UNDYING：
 *   ClearAllStatusEffects + ApplyStatusEffects{regeneration 900/II, absorption 100/II, fire_resistance 800/I}
 */
[[nodiscard]] DeathProtectionPayload totemOfUndyingDeathProtection();

/**
 * @brief 单个数据组件的异构值载荷
 *
 * variant 以 std::monostate 起始，区分"未设置"与"显式设置为某值"。
 * 索引常量集中定义于 ComponentPayloadIndex，供编解码与 ItemStack 覆盖读写统一引用。
 *
 * 注意：容器/充能投射物组件（container / charged_projectiles）不进入本 variant——
 * 它们的元素是 ItemStack 指针，而本头文件先于 ItemStack 定义被包含，
 * 这类组件改由 ItemStack 的独立字段承载（见 ItemStack::getContainerContents 等）。
 */
using DataComponentPayload = std::variant<std::monostate,
    i32,                                                // Damage / RepairCost / MaxStackSize / MaxDamage / Enchantable
    std::unique_ptr<text::ITextComponent>,              // CustomName / ItemName
    std::vector<std::unique_ptr<text::ITextComponent>>, // Lore
    item::enchant::EnchantmentContainer,                // Enchantments / StoredEnchantments
    PotionContentsPayload,                              // PotionContents
    AdventureModePredicate,                             // CanPlaceOn / CanBreak
    nlohmann::json,                                     // CustomData / CustomModelData / Trim / Recipes 等数据驱动组件
    bool,                                               // Unbreakable / EnchantmentGlintOverride
    ItemRarity,                                         // Rarity
    std::string,                                        // ItemModel / WildcardItem 等资源位置
    AttackRange,                                        // AttackRange
    FireworkExplosion,                                  // FireworkExplosion
    Fireworks,                                          // Fireworks
    InstrumentPayload,                                  // Instrument
    std::optional<i32>,                                 // OminousBottleAmplifier / MapId 等可选整数
    ToolPayload,                                        // Tool
    SuspiciousStewEffectsPayload,                       // SuspiciousStewEffects
    DeathProtectionPayload>;                            // DeathProtection

/**
 * @brief DataComponentPayload 各备选项索引
 *
 * 与上面的 variant 声明顺序严格一一对应。
 */
namespace ComponentPayloadIndex {
constexpr usize Monostate = 0;
constexpr usize Int = 1;                // i32
constexpr usize TextComponent = 2;      // unique_ptr<ITextComponent>
constexpr usize TextComponentList = 3;  // vector<unique_ptr<ITextComponent>>
constexpr usize Enchantments = 4;       // EnchantmentContainer
constexpr usize PotionContents = 5;     // PotionContentsPayload
constexpr usize AdventurePredicate = 6; // AdventureModePredicate
constexpr usize Json = 7;               // nlohmann::json
constexpr usize Bool = 8;               // bool
constexpr usize Rarity = 9;             // ItemRarity
constexpr usize Identifier = 10;        // std::string
constexpr usize AttackRange = 11;       // AttackRange
constexpr usize FireworkExplosion = 12; // FireworkExplosion
constexpr usize Fireworks = 13;         // Fireworks
constexpr usize Instrument = 14;        // InstrumentPayload
constexpr usize OptionalInt = 15;       // optional<i32>
constexpr usize Tool = 16;              // ToolPayload
constexpr usize StewEffects = 17;       // SuspiciousStewEffectsPayload
constexpr usize DeathProtection = 18;   // DeathProtectionPayload
} // namespace ComponentPayloadIndex

/**
 * @brief 一个数据组件的 typeId + 值
 */
struct DataComponentEntry {
    i32 typeId = 0; ///< DataComponentType 的整数 id
    DataComponentPayload value;
};

/**
 * @brief 数据组件补丁（1.21.11 DataComponentPatch）
 *
 * NBT 格式：键为组件资源位置名；'!' 前缀 = removed（值为占位），其余 = added。
 * Wire 格式：VarInt(addedCount) + [VarInt(typeId)+value]* + VarInt(removedCount) + [VarInt(typeId)]*。
 */
class DataComponentPatch {
public:
    DataComponentPatch() = default;

    [[nodiscard]] bool isEmpty() const noexcept { return m_added.empty() && m_removed.empty(); }

    [[nodiscard]] const std::vector<DataComponentEntry>& added() const noexcept { return m_added; }
    [[nodiscard]] const std::vector<i32>& removed() const noexcept { return m_removed; }

    /// 增加一个组件值（typeId 必须在落地子集内）
    void add(i32 typeId, DataComponentPayload value) { m_added.push_back({typeId, std::move(value)}); }

    /// 增加一个组件值（枚举入口）
    void add(DataComponentType type, DataComponentPayload value)
    {
        m_added.push_back({componentTypeId(type), std::move(value)});
    }

    /// 标记移除一个组件
    void remove(i32 typeId) { m_removed.push_back(typeId); }
    void remove(DataComponentType type) { m_removed.push_back(componentTypeId(type)); }

    [[nodiscard]] std::vector<DataComponentEntry>& addedMut() noexcept { return m_added; }
    [[nodiscard]] std::vector<i32>& removedMut() noexcept { return m_removed; }

private:
    std::vector<DataComponentEntry> m_added;
    std::vector<i32> m_removed;
};

} // namespace component
} // namespace item
} // namespace mc
