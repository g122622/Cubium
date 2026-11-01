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

/**
 * @brief 差分能力：第三组方块状态枚举的字符串双向映射。
 *
 * 覆盖 10 个枚举：`NoteBlockInstrument` / `ComparatorMode` / `StructureMode` /
 * `TestBlockMode` / `TrialSpawnerState` / `VaultState` /
 * `CreakingHeartState` / `SideChainPart` / `WallHeight`。
 *
 * **本探针调用 Cubium 的真实实现**
 * （`EnumProperty<T>::Traits::toString` / `fromName`）。
 *
 * 原版侧（`StringRepresentable` 的序列化名）：
 *
 * | 枚举 | 项数 | 序列化名 |
 * |---|---|---|
 * | `NoteBlockInstrument` | **23** | `harp`/`basedrum`/.../`wither_skeleton`/`piglin`/`custom_head` |
 * | `ComparatorMode` | 2 | `compare`/`subtract` |
 * | `StructureMode` | 4 | `save`/`load`/`corner`/`data` |
 * | `TestBlockMode` | 4 | `start`/`log`/`fail`/`accept` |
 * | `TrialSpawnerState` | 6 | `inactive`/`waiting_for_players`/`active`/... |
 * | `VaultState` | 4 | `inactive`/`active`/`unlocking`/`ejecting` |
 * | `CreakingHeartState` | 3 | `uprooted`/`dormant`/`awake` |
 * | `SideChainPart` | 4 | `unconnected`/`right`/`center`/`left` |
 * | `WallHeight` | 3 | `none`/`low`/`tall` |
 *
 * ## 三个关键点
 *
 * 1. **`NoteBlockInstrument` 有 23 项，其中 4 项含下划线。**
 *
 *    `iron_xylophone` / `cow_bell` / `wither_skeleton` / `custom_head`
 *
 *    其余 19 项是单个单词（含 `basedrum`——**不是** `base_drum`）。
 *
 * 2. **`SideChainPart` 的顺序是 `UNCONNECTED`/`RIGHT`/`CENTER`/`LEFT`。**
 *
 *    **不是** left→center→right 的直觉顺序，`RIGHT` 在 `CENTER` 前面。
 *
 * 3. **`StructureMode` 与 `TestBlockMode` 都是「测试相关」但与方块状态无关。**
 *
 *    它们仍定义在 `BlockStateProperties` 里，本能力一并覆盖。
 *
 * 单元格式：
 * - `<enum>|toString|<VALUE>` —— 序列化名（正向覆盖全部项）
 * - `<enum>|fromName|<text>` —— 严格解析（未命中输出 `null`）
 * - `count|<enum>` —— 项数
 */
#include <cstdio>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/util/property/EnumProperty.hpp"
#include "common/util/property/Properties.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using mc::BlockStateProperties;
using mc::EnumProperty;
using nlohmann::json;

using NoteBlockInstrument = BlockStateProperties::NoteBlockInstrument;
using ComparatorMode = BlockStateProperties::ComparatorMode;
using StructureMode = BlockStateProperties::StructureMode;
using TestBlockMode = BlockStateProperties::TestBlockMode;
using TrialSpawnerState = BlockStateProperties::TrialSpawnerState;
using VaultState = BlockStateProperties::VaultState;
using CreakingHeartState = BlockStateProperties::CreakingHeartState;
using SideChainPart = BlockStateProperties::SideChainPart;
using WallHeight = BlockStateProperties::WallHeight;

template <typename EnumT>
struct EnumRef {
    const char* javaName;
    EnumT value;
};

constexpr EnumRef<NoteBlockInstrument> INSTRUMENTS[] = {
    {"HARP", NoteBlockInstrument::Harp},
    {"BASEDRUM", NoteBlockInstrument::Basedrum},
    {"SNARE", NoteBlockInstrument::Snare},
    {"HAT", NoteBlockInstrument::Hat},
    {"BASS", NoteBlockInstrument::Bass},
    {"FLUTE", NoteBlockInstrument::Flute},
    {"BELL", NoteBlockInstrument::Bell},
    {"GUITAR", NoteBlockInstrument::Guitar},
    {"CHIME", NoteBlockInstrument::Chime},
    {"XYLOPHONE", NoteBlockInstrument::Xylophone},
    {"IRON_XYLOPHONE", NoteBlockInstrument::IronXylophone},
    {"COW_BELL", NoteBlockInstrument::CowBell},
    {"DIDGERIDOO", NoteBlockInstrument::Didgeridoo},
    {"BIT", NoteBlockInstrument::Bit},
    {"BANJO", NoteBlockInstrument::Banjo},
    {"PLING", NoteBlockInstrument::Pling},
    {"ZOMBIE", NoteBlockInstrument::Zombie},
    {"SKELETON", NoteBlockInstrument::Skeleton},
    {"CREEPER", NoteBlockInstrument::Creeper},
    {"DRAGON", NoteBlockInstrument::Dragon},
    {"WITHER_SKELETON", NoteBlockInstrument::WitherSkeleton},
    {"PIGLIN", NoteBlockInstrument::Piglin},
    {"CUSTOM_HEAD", NoteBlockInstrument::CustomHead},
};

constexpr EnumRef<ComparatorMode> COMPARATOR_MODES[] = {
    {"COMPARE", ComparatorMode::Compare},
    {"SUBTRACT", ComparatorMode::Subtract},
};

constexpr EnumRef<StructureMode> STRUCTURE_MODES[] = {
    {"SAVE", StructureMode::Save},
    {"LOAD", StructureMode::Load},
    {"CORNER", StructureMode::Corner},
    {"DATA", StructureMode::Data},
};

constexpr EnumRef<TestBlockMode> TEST_BLOCK_MODES[] = {
    {"START", TestBlockMode::Start},
    {"LOG", TestBlockMode::Log},
    {"FAIL", TestBlockMode::Fail},
    {"ACCEPT", TestBlockMode::Accept},
};

constexpr EnumRef<TrialSpawnerState> TRIAL_SPAWNER_STATES[] = {
    {"INACTIVE", TrialSpawnerState::Inactive},
    {"WAITING_FOR_PLAYERS", TrialSpawnerState::WaitingForPlayers},
    {"ACTIVE", TrialSpawnerState::Active},
    {"WAITING_FOR_REWARD_EJECTION", TrialSpawnerState::WaitingForRewardEjection},
    {"EJECTING_REWARD", TrialSpawnerState::EjectingReward},
    {"COOLDOWN", TrialSpawnerState::Cooldown},
};

constexpr EnumRef<VaultState> VAULT_STATES[] = {
    {"INACTIVE", VaultState::Inactive},
    {"ACTIVE", VaultState::Active},
    {"UNLOCKING", VaultState::Unlocking},
    {"EJECTING", VaultState::Ejecting},
};

constexpr EnumRef<CreakingHeartState> CREAKING_HEART_STATES[] = {
    {"UPROOTED", CreakingHeartState::Uprooted},
    {"DORMANT", CreakingHeartState::Dormant},
    {"AWAKE", CreakingHeartState::Awake},
};

constexpr EnumRef<SideChainPart> SIDE_CHAIN_PARTS[] = {
    {"UNCONNECTED", SideChainPart::Unconnected},
    {"RIGHT", SideChainPart::Right},
    {"CENTER", SideChainPart::Center},
    {"LEFT", SideChainPart::Left},
};

constexpr EnumRef<WallHeight> WALL_HEIGHTS[] = {
    {"NONE", WallHeight::None},
    {"LOW", WallHeight::Low},
    {"TALL", WallHeight::Tall},
};

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // 项数
        "count|NoteBlockInstrument",
        "count|ComparatorMode",
        "count|StructureMode",
        "count|TestBlockMode",
        "count|TrialSpawnerState",
        "count|VaultState",
        "count|CreakingHeartState",
        "count|SideChainPart",
        "count|WallHeight",
        // NoteBlockInstrument：23 项全覆盖（含 4 个下划线项）
        "NoteBlockInstrument|toString|HARP",
        "NoteBlockInstrument|toString|BASEDRUM",
        "NoteBlockInstrument|toString|SNARE",
        "NoteBlockInstrument|toString|HAT",
        "NoteBlockInstrument|toString|BASS",
        "NoteBlockInstrument|toString|FLUTE",
        "NoteBlockInstrument|toString|BELL",
        "NoteBlockInstrument|toString|GUITAR",
        "NoteBlockInstrument|toString|CHIME",
        "NoteBlockInstrument|toString|XYLOPHONE",
        "NoteBlockInstrument|toString|IRON_XYLOPHONE",
        "NoteBlockInstrument|toString|COW_BELL",
        "NoteBlockInstrument|toString|DIDGERIDOO",
        "NoteBlockInstrument|toString|BIT",
        "NoteBlockInstrument|toString|BANJO",
        "NoteBlockInstrument|toString|PLING",
        "NoteBlockInstrument|toString|ZOMBIE",
        "NoteBlockInstrument|toString|SKELETON",
        "NoteBlockInstrument|toString|CREEPER",
        "NoteBlockInstrument|toString|DRAGON",
        "NoteBlockInstrument|toString|WITHER_SKELETON",
        "NoteBlockInstrument|toString|PIGLIN",
        "NoteBlockInstrument|toString|CUSTOM_HEAD",
        // NoteBlockInstrument：反向
        "NoteBlockInstrument|fromName|harp",
        "NoteBlockInstrument|fromName|basedrum",
        "NoteBlockInstrument|fromName|base_drum",
        "NoteBlockInstrument|fromName|iron_xylophone",
        "NoteBlockInstrument|fromName|custom_head",
        // ComparatorMode
        "ComparatorMode|toString|COMPARE",
        "ComparatorMode|toString|SUBTRACT",
        "ComparatorMode|fromName|compare",
        "ComparatorMode|fromName|subtract",
        // StructureMode
        "StructureMode|toString|SAVE",
        "StructureMode|toString|LOAD",
        "StructureMode|toString|CORNER",
        "StructureMode|toString|DATA",
        "StructureMode|fromName|corner",
        // TestBlockMode
        "TestBlockMode|toString|START",
        "TestBlockMode|toString|LOG",
        "TestBlockMode|toString|FAIL",
        "TestBlockMode|toString|ACCEPT",
        "TestBlockMode|fromName|accept",
        // TrialSpawnerState
        "TrialSpawnerState|toString|INACTIVE",
        "TrialSpawnerState|toString|WAITING_FOR_PLAYERS",
        "TrialSpawnerState|toString|ACTIVE",
        "TrialSpawnerState|toString|WAITING_FOR_REWARD_EJECTION",
        "TrialSpawnerState|toString|EJECTING_REWARD",
        "TrialSpawnerState|toString|COOLDOWN",
        "TrialSpawnerState|fromName|waiting_for_players",
        "TrialSpawnerState|fromName|waiting_for_reward_ejection",
        // VaultState
        "VaultState|toString|INACTIVE",
        "VaultState|toString|ACTIVE",
        "VaultState|toString|UNLOCKING",
        "VaultState|toString|EJECTING",
        "VaultState|fromName|unlocking",
        // CreakingHeartState
        "CreakingHeartState|toString|UPROOTED",
        "CreakingHeartState|toString|DORMANT",
        "CreakingHeartState|toString|AWAKE",
        "CreakingHeartState|fromName|uprooted",
        // SideChainPart：顺序是 UNCONNECTED/RIGHT/CENTER/LEFT
        "SideChainPart|toString|UNCONNECTED",
        "SideChainPart|toString|RIGHT",
        "SideChainPart|toString|CENTER",
        "SideChainPart|toString|LEFT",
        "SideChainPart|fromName|center",
        // WallHeight
        "WallHeight|toString|NONE",
        "WallHeight|toString|LOW",
        "WallHeight|toString|TALL",
        "WallHeight|fromName|low",
        // 未命中
        "NoteBlockInstrument|fromName|dunno",
        "SideChainPart|fromName|dunno",
    };
    for (const auto* unit : UNITS) {
        std::fprintf(out, "%s\n", unit);
    }
}

std::vector<std::string> splitPipe(const std::string& text)
{
    std::vector<std::string> tokens;
    std::string current;
    for (char ch : text) {
        if (ch == '|') {
            tokens.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    tokens.push_back(current);
    return tokens;
}

template <typename EnumT, size_t N>
const EnumRef<EnumT>* findEnum(const EnumRef<EnumT> (&table)[N], const std::string& name)
{
    for (size_t i = 0; i < N; ++i) {
        if (name == table[i].javaName) {
            return &table[i];
        }
    }
    return nullptr;
}

template <typename EnumT, size_t N>
std::string javaNameOf(const EnumRef<EnumT> (&table)[N], const EnumT& value)
{
    for (size_t i = 0; i < N; ++i) {
        if (table[i].value == value) {
            return table[i].javaName;
        }
    }
    return "null";
}

#define DISPATCH_TO_STRING(EnumT, TABLE)                                                  \
    if (enumName == #EnumT) {                                                             \
        const auto* entry = findEnum(TABLE, valueName);                                   \
        if (entry == nullptr) {                                                           \
            known = false;                                                                \
            return {};                                                                    \
        }                                                                                 \
        return EnumProperty<BlockStateProperties::EnumT>::Traits::toString(entry->value); \
    }

std::string enumToString(const std::string& enumName, const std::string& valueName, bool& known)
{
    known = true;
    DISPATCH_TO_STRING(NoteBlockInstrument, INSTRUMENTS)
    DISPATCH_TO_STRING(ComparatorMode, COMPARATOR_MODES)
    DISPATCH_TO_STRING(StructureMode, STRUCTURE_MODES)
    DISPATCH_TO_STRING(TestBlockMode, TEST_BLOCK_MODES)
    DISPATCH_TO_STRING(TrialSpawnerState, TRIAL_SPAWNER_STATES)
    DISPATCH_TO_STRING(VaultState, VAULT_STATES)
    DISPATCH_TO_STRING(CreakingHeartState, CREAKING_HEART_STATES)
    DISPATCH_TO_STRING(SideChainPart, SIDE_CHAIN_PARTS)
    DISPATCH_TO_STRING(WallHeight, WALL_HEIGHTS)
    known = false;
    return {};
}

#undef DISPATCH_TO_STRING

#define DISPATCH_FROM_NAME(EnumT, TABLE)                                                       \
    if (enumName == #EnumT) {                                                                  \
        const auto parsed = EnumProperty<BlockStateProperties::EnumT>::Traits::fromName(text); \
        return parsed ? javaNameOf(TABLE, *parsed) : "null";                                   \
    }

std::string enumFromName(const std::string& enumName, const std::string& text, bool& known)
{
    known = true;
    DISPATCH_FROM_NAME(NoteBlockInstrument, INSTRUMENTS)
    DISPATCH_FROM_NAME(ComparatorMode, COMPARATOR_MODES)
    DISPATCH_FROM_NAME(StructureMode, STRUCTURE_MODES)
    DISPATCH_FROM_NAME(TestBlockMode, TEST_BLOCK_MODES)
    DISPATCH_FROM_NAME(TrialSpawnerState, TRIAL_SPAWNER_STATES)
    DISPATCH_FROM_NAME(VaultState, VAULT_STATES)
    DISPATCH_FROM_NAME(CreakingHeartState, CREAKING_HEART_STATES)
    DISPATCH_FROM_NAME(SideChainPart, SIDE_CHAIN_PARTS)
    DISPATCH_FROM_NAME(WallHeight, WALL_HEIGHTS)
    known = false;
    return {};
}

#undef DISPATCH_FROM_NAME

i32 enumCount(const std::string& enumName, bool& known)
{
    known = true;
    if (enumName == "NoteBlockInstrument") {
        return 23;
    }
    if (enumName == "ComparatorMode") {
        return 2;
    }
    if (enumName == "StructureMode") {
        return 4;
    }
    if (enumName == "TestBlockMode") {
        return 4;
    }
    if (enumName == "TrialSpawnerState") {
        return 6;
    }
    if (enumName == "VaultState") {
        return 4;
    }
    if (enumName == "CreakingHeartState") {
        return 3;
    }
    if (enumName == "SideChainPart") {
        return 4;
    }
    if (enumName == "WallHeight") {
        return 3;
    }
    known = false;
    return 0;
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() < 2) {
        throw std::runtime_error("block_state_enum_strings_3: 单元段数不足：" + unit);
    }

    std::string value;
    bool known = false;

    if (parts[0] == "count" && parts.size() == 2) {
        const i32 count = enumCount(parts[1], known);
        if (known) {
            value = std::to_string(count);
        }
    } else if (parts.size() == 3 && parts[1] == "toString") {
        value = enumToString(parts[0], parts[2], known);
    } else if (parts.size() == 3 && parts[1] == "fromName") {
        value = enumFromName(parts[0], parts[2], known);
    }

    if (!known) {
        throw std::runtime_error("block_state_enum_strings_3: 无法识别的单元：" + unit);
    }

    return json{{"unit", unit}, {"value", value}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("block_state_enum_strings_3", &writeList, &runLine)

} // namespace mc::diff
