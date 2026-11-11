#pragma once

#include "common/core/Types.hpp"

#include <string_view>
#include <vector>

namespace mc {
namespace command {

/**
 * @brief 槽位区间名到槽位索引集合的映射（对应 vanilla `SlotRanges`）
 *
 * 命令里的槽位参数（`/execute if items ... <slots> ...`、`/item replace ...`）用一组
 * 具名区间表达"要检查哪些槽位"，例如 `container.*`、`weapon.mainhand`、`hotbar.3`。
 * 本类负责把这类名字解析成具体的槽位索引列表。
 *
 * ## 命名规则（与 vanilla 的 `SlotRanges` 同名）
 *
 * 单槽：`contents`、`weapon`、`weapon.mainhand`、`weapon.offhand`、`armor.head`、
 * `armor.chest`、`armor.legs`、`armor.feet`、`armor.body`、`saddle`、`horse.chest`、
 * `player.cursor`。
 *
 * 带编号的族（同时提供 `前缀.N` 与 `前缀.*`）：
 * `container.`（0..53）、`hotbar.`（0..8）、`inventory.`（9..35）、
 * `enderchest.`（200..226）、`villager.`（300..307）、`horse.`（500..514）、
 * `player.crafting.`（500..503）。
 *
 * ## 与 vanilla 1.21.11 的差异（刻意保留）
 *
 * 装备槽位的编号以**本项目**的 `ItemSlot` 为准，与 vanilla 有偏移：
 *
 * | 名字 | 本项目 | vanilla |
 * |---|---|---|
 * | `weapon` / `weapon.mainhand` | 98 | 98 |
 * | `weapon.offhand` | 99 | 99 |
 * | `armor.feet` | 100 | 101 |
 * | `armor.legs` | 101 | 102 |
 * | `armor.chest` | 102 | 103 |
 * | `armor.head` | 103 | 104 |
 * | `armor.body` | 105 | 111 |
 * | `saddle` | 106 | 113 |
 *
 * 项目的 `ItemSlot::toPlayerInventorySlot` 已按上表把装备槽映射到 `PlayerInventory`，
 * 若在此改成 vanilla 编号，会让 `/execute if items` 与 `/item replace` 对同一槽位
 * 给出不同结果。因此这里跟随项目编号，差异在此记录，不静默处理。
 */
class SlotRanges {
public:
    /**
     * @brief 把槽位区间名解析为槽位索引列表
     *
     * 未知名字返回空列表（调用方据此报"未知槽位"，不要当成"空集合恰好不匹配"）。
     *
     * @param name 槽位区间名（区分大小写，与命令输入一致）
     * @return 该名字覆盖的槽位索引；未知名字为空
     */
    [[nodiscard]] static std::vector<i32> nameToIds(std::string_view name);

    /**
     * @brief 该名字是否是已知的槽位区间名
     *
     * @param name 槽位区间名
     * @return 已知返回 true
     */
    [[nodiscard]] static bool isKnownName(std::string_view name);

    /**
     * @brief 全部已知槽位区间名（供命令补全）
     *
     * @return 名字列表，顺序稳定
     */
    [[nodiscard]] static const std::vector<std::string>& allNames();
};

} // namespace command
} // namespace mc
