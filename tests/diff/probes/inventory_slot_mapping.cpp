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

#include "probe.hpp"

#include "common/entity/inventory/InventorySlotMapping.hpp"

#include <nlohmann/json.hpp>

#include <cstdio>
#include <string>
#include <string_view>

namespace mc::diff {

namespace {

using json = nlohmann::json;

/**
 * @file inventory_slot_mapping.cpp
 * @brief 玩家背包内部索引与背包菜单（InventoryMenu）槽位号的互转。
 *
 * 两套索引同时存在，语义完全不同：
 *
 *   - **Inventory 内部索引**（0..40）：玩家背包对象自己的存储顺序；
 *   - **InventoryMenu 菜单槽**（0..45）：`ContainerSetContent` / `ContainerSetSlot`
 *      包里 slot 字段的含义，原版客户端按它摆放物品。
 *
 * 网络同步时按菜单槽逐个反查内部索引再取物品，因此这张映射表**直接决定客户端看到
 * 的背包界面**。错一位的典型表现是「装备穿在错误的格子里」，而且服务端自己看起来
 * 完全自洽——只有和原版逐槽比对才能发现。
 *
 * 单元格式：
 *   - `inv2menu|<内部索引>`：内部索引 → 菜单槽
 *   - `menu2inv|<菜单槽>`：菜单槽 → 内部索引
 *
 * 覆盖全部合法索引（含边界）与若干越界值。
 */

void writeList(std::FILE* out)
{
    // 内部索引：0..40 是合法范围（40 为副手），再补三个越界值。
    for (i32 slot = 0; slot <= 40; ++slot) {
        std::fprintf(out, "inv2menu|%d\n", slot);
    }
    for (const i32 slot : {-1, 41, 100}) {
        std::fprintf(out, "inv2menu|%d\n", slot);
    }

    // 菜单槽：0..45 是合法范围，再补三个越界值。
    for (i32 slot = 0; slot <= 45; ++slot) {
        std::fprintf(out, "menu2inv|%d\n", slot);
    }
    for (const i32 slot : {-1, 46, 100}) {
        std::fprintf(out, "menu2inv|%d\n", slot);
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string requested = input.at("unit").get<std::string>();

    const size_t bar = requested.find('|');
    if (bar == std::string::npos) {
        return json{{"unit", requested}, {"missing", true}}.dump();
    }
    const std::string mode = requested.substr(0, bar);
    const i32 slot = std::stoi(requested.substr(bar + 1));

    if (mode == "inv2menu") {
        return json{{"unit", requested}, {"mapped", InventorySlotMapping::playerInvToMenuSlot(slot)}}.dump();
    }
    if (mode == "menu2inv") {
        return json{{"unit", requested}, {"mapped", InventorySlotMapping::menuSlotToPlayerInvId(slot)}}.dump();
    }
    return json{{"unit", requested}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(inventory_slot_mapping, &writeList, &runLine)

} // namespace mc::diff
