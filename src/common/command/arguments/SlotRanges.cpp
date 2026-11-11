#include "SlotRanges.hpp"

#include <algorithm>
#include <array>
#include <unordered_map>

namespace mc {
namespace command {

namespace {

/// 一条槽位区间：名字 + 覆盖的槽位索引。
struct SlotRangeEntry {
    std::string name;
    std::vector<i32> ids;
};

/// 装备槽位编号（以本项目 `ItemSlot` 的编号为准，与 vanilla 有偏移，见头文件表格）。
constexpr i32 SLOT_WEAPON_MAINHAND = 98;
constexpr i32 SLOT_WEAPON_OFFHAND = 99;
constexpr i32 SLOT_ARMOR_FEET = 100;
constexpr i32 SLOT_ARMOR_LEGS = 101;
constexpr i32 SLOT_ARMOR_CHEST = 102;
constexpr i32 SLOT_ARMOR_HEAD = 103;
constexpr i32 SLOT_ARMOR_BODY = 105;
constexpr i32 SLOT_SADDLE = 106;
constexpr i32 SLOT_HORSE_CHEST = 499;
constexpr i32 SLOT_PLAYER_CURSOR = 499;

/// 追加单槽
void addSingle(std::vector<SlotRangeEntry>& out, std::string name, i32 id)
{
    out.push_back({std::move(name), {id}});
}

/// 追加一组不连续的单槽（如 `weapon.*` = {98, 99}）
void addGroup(std::vector<SlotRangeEntry>& out, std::string name, std::initializer_list<i32> ids)
{
    out.push_back({std::move(name), std::vector<i32>(ids)});
}

/// 追加带编号的族：`prefix.N` 各一条，外加 `prefix.*` 覆盖整个区间
void addNumberedRange(std::vector<SlotRangeEntry>& out, const std::string& prefix, i32 base, i32 count)
{
    std::vector<i32> all;
    all.reserve(static_cast<usize>(count));
    for (i32 i = 0; i < count; ++i) {
        const i32 id = base + i;
        addSingle(out, prefix + std::to_string(i), id);
        all.push_back(id);
    }
    out.push_back({prefix + "*", std::move(all)});
}

const std::vector<SlotRangeEntry>& table()
{
    static const std::vector<SlotRangeEntry> entries = [] {
        std::vector<SlotRangeEntry> out;

        // 依次对应 vanilla SlotRanges 的登记顺序，便于对照。
        addSingle(out, "contents", 0);
        addNumberedRange(out, "container.", 0, 54);
        addNumberedRange(out, "hotbar.", 0, 9);
        addNumberedRange(out, "inventory.", 9, 27);
        addNumberedRange(out, "enderchest.", 200, 27);
        addNumberedRange(out, "villager.", 300, 8);
        addNumberedRange(out, "horse.", 500, 15);

        addSingle(out, "weapon", SLOT_WEAPON_MAINHAND);
        addSingle(out, "weapon.mainhand", SLOT_WEAPON_MAINHAND);
        addSingle(out, "weapon.offhand", SLOT_WEAPON_OFFHAND);
        addGroup(out, "weapon.*", {SLOT_WEAPON_MAINHAND, SLOT_WEAPON_OFFHAND});

        addSingle(out, "armor.head", SLOT_ARMOR_HEAD);
        addSingle(out, "armor.chest", SLOT_ARMOR_CHEST);
        addSingle(out, "armor.legs", SLOT_ARMOR_LEGS);
        addSingle(out, "armor.feet", SLOT_ARMOR_FEET);
        addSingle(out, "armor.body", SLOT_ARMOR_BODY);
        addGroup(
            out, "armor.*", {SLOT_ARMOR_HEAD, SLOT_ARMOR_CHEST, SLOT_ARMOR_LEGS, SLOT_ARMOR_FEET, SLOT_ARMOR_BODY});

        addSingle(out, "saddle", SLOT_SADDLE);
        addSingle(out, "horse.chest", SLOT_HORSE_CHEST);
        addSingle(out, "player.cursor", SLOT_PLAYER_CURSOR);
        addNumberedRange(out, "player.crafting.", 500, 4);

        return out;
    }();
    return entries;
}

const std::unordered_map<std::string_view, usize>& nameIndex()
{
    static const std::unordered_map<std::string_view, usize> index = [] {
        std::unordered_map<std::string_view, usize> map;
        const auto& entries = table();
        map.reserve(entries.size());
        for (usize i = 0; i < entries.size(); ++i) {
            map.emplace(entries[i].name, i);
        }
        return map;
    }();
    return index;
}

} // namespace

std::vector<i32> SlotRanges::nameToIds(std::string_view name)
{
    const auto& index = nameIndex();
    const auto it = index.find(name);
    if (it == index.end()) {
        return {};
    }
    return table()[it->second].ids;
}

bool SlotRanges::isKnownName(std::string_view name)
{
    return nameIndex().find(name) != nameIndex().end();
}

const std::vector<std::string>& SlotRanges::allNames()
{
    static const std::vector<std::string> names = [] {
        std::vector<std::string> out;
        const auto& entries = table();
        out.reserve(entries.size());
        for (const auto& entry : entries) {
            out.push_back(entry.name);
        }
        return out;
    }();
    return names;
}

} // namespace command
} // namespace mc
