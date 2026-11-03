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
 * @brief 差分能力：发射器行为表（每个物品被归为哪一类行为）。
 *
 * `list`：全部物品资源名
 * `run` ：一行 → { "registered": bool, "kind": 标签, "successDefault": bool }
 *
 * ## 为什么比对「标签」而不是类名
 *
 * 原版 169 条注册里，109 条是 `DispenserBlock` 静态块中的**匿名子类**
 * （`new DefaultDispenseItemBehavior() { ... }`），类名是 `DispenserBlock$1` 这类序号，
 * 跨实现毫无可比性；Cubium 侧的类拆得也比原版细。因此两侧都归一到少数几个语义标签，
 * 只比对「这个物品被当作哪一类处理」这个真正影响游戏行为的结论。
 *
 * ## successDefault
 *
 * 原版 `OptionalDispenseItemBehavior.isSuccess()` 初始为 false，只有子类真正做成事才置
 * true——它决定**是否播发射音效、是否消耗物品**。普通行为恒为 true。这个布尔值是最容易
 * 被漏抄的一位语义。
 *
 * ## 标签口径（必须与 `DispenseBehaviorOracle.kindOf` 严格一致）
 *
 * | 标签 | 含义 |
 * |---|---|
 * | `none` | 未注册专用行为，走默认投掷 |
 * | `projectile` | 投掷物（箭、雪球、药水、烟花…）|
 * | `boat` | 放置船 |
 * | `minecart` | 放置矿车 |
 * | `shears` | 剪刀 |
 * | `shulker_box` | 放置潜影盒 |
 * | `equipment` | 给实体穿戴装备 |
 * | `custom` | 其余具名 / 匿名定制行为 |
 */

#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/world/block/dispense/DispenseItemBehaviorRegistry.hpp"
#include "common/world/block/dispense/IDispenseItemBehavior.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 物品与发射行为都是重型全局注册，探针是单次进程，静态标志足够。
void ensureItems()
{
    static bool s_initialized = false;
    if (!s_initialized) {
        Items::initialize();
        blocks::DispenseItemBehaviorRegistry::instance().initDefaultBehaviors();
        s_initialized = true;
    }
}

void writeList(FILE* out)
{
    ensureItems();
    std::vector<std::string> names;
    ItemRegistry::instance().forEachItem([&names](Item& item) {
        names.push_back(item.itemLocation().toString());
    });
    std::sort(names.begin(), names.end());
    names.erase(std::unique(names.begin(), names.end()), names.end());
    for (const std::string& name : names) {
        std::fprintf(out, "%s\n", name.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureItems();
    const json input = json::parse(inputLine);
    const std::string requested = input.at("unit").get<std::string>();

    const ResourceLocation location = ResourceLocation::parse(requested);
    const Item* item = ItemRegistry::instance().getItem(location);
    if (item == nullptr) {
        return json{{"unit", requested}, {"missing", true}}.dump();
    }

    const blocks::DispenseItemBehaviorRegistry& registry = blocks::DispenseItemBehaviorRegistry::instance();
    const bool registered = registry.hasBehavior(requested);
    const blocks::IDispenseItemBehavior* behavior = registry.getBehavior(requested);

    // 与 oracle 侧同口径：未注册 => none；注册了但拿不到实例也按 none 处理（不应发生）。
    std::string kind = "none";
    bool successDefault = true;
    if (registered && behavior != nullptr) {
        // 按 Cubium 自己的类层次归类，映射到与 oracle 相同的语义标签。
        // 顺序重要：子类必须先于父类判断（Projectile/Boat 都继承自 Default）。
        if (dynamic_cast<const blocks::ProjectileDispenseBehavior*>(behavior) != nullptr) {
            kind = "projectile";
        } else if (dynamic_cast<const blocks::BoatDispenseBehavior*>(behavior) != nullptr) {
            kind = "boat";
        } else {
            // 其余（桶、打火石、骨粉、TNT 及各物品定制行为）在原版侧同样归入 custom。
            kind = "custom";
        }
        successDefault = behavior->isSuccess();
    }

    return json{{"unit", requested}, {"registered", registered}, {"kind", kind}, {"successDefault", successDefault}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("dispense_behavior", &writeList, &runLine)

} // namespace mc::diff
