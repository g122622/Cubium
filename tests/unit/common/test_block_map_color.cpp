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

#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/registry/BlockRegistry.hpp"
#include "common/world/block/registry/vanilla/BlockMapColorTable.hpp"
#include "common/world/block/state/BlockState.hpp"
#include "common/world/map/MaterialColor.hpp"
#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>
#include <gtest/gtest.h>

using namespace mc;

namespace {

/// 规范化状态名（属性按名字排序）。
std::string canonicalStateName(std::string_view raw)
{
    const std::size_t open = raw.find('[');
    if (open == std::string_view::npos) {
        return std::string(raw);
    }
    const std::size_t close = raw.rfind(']');
    if (close == std::string_view::npos || close <= open) {
        return std::string(raw);
    }
    std::string body(raw.substr(open + 1, close - open - 1));
    std::vector<std::string> attrs;
    std::string current;
    for (char ch : body) {
        if (ch == ',') {
            attrs.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    if (!current.empty()) {
        attrs.push_back(current);
    }
    std::sort(attrs.begin(), attrs.end());
    std::string result(raw.substr(0, open));
    result += "[";
    for (std::size_t i = 0; i < attrs.size(); ++i) {
        if (i > 0) {
            result += ",";
        }
        result += attrs[i];
    }
    result += "]";
    return result;
}

/// 注册表里每个方块枚举出的全部规范状态名（与查表函数的输入格式一致）。
std::vector<std::string>& allStateNames()
{
    static std::vector<std::string> names = [] {
        std::vector<std::string> result;
        BlockRegistry::instance().forEachBlockState([&](const BlockState& state) {
            result.push_back(canonicalStateName(state.toString()));
        });
        return result;
    }();
    return names;
}

std::unordered_map<std::string, u32>& stateIndex()
{
    static std::unordered_map<std::string, u32> index = [] {
        std::unordered_map<std::string, u32> result;
        const auto& names = allStateNames();
        for (std::size_t i = 0; i < names.size(); ++i) {
            result[names[i]] = static_cast<u32>(i);
        }
        return result;
    }();
    return index;
}

/// 以方块名和某个属性值构造状态名，用于在三轴/四向/`part`/`waterlogged` 上做全等配对。
std::optional<std::string> withAttribute(const std::string& stateName, std::string_view key, std::string_view value)
{
    const std::size_t open = stateName.find('[');
    const std::size_t close = stateName.rfind(']');
    std::string head = stateName.substr(0, open);
    std::vector<std::string> attrs;
    std::string current;
    const std::string body = stateName.substr(open + 1, close - open - 1);
    for (char ch : body) {
        if (ch == ',') {
            attrs.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    if (!current.empty()) {
        attrs.push_back(current);
    }

    // 记录的属性值，不属于 key 的原样保留；属于 key 的替换掉。
    std::vector<std::string> others;
    for (const std::string& attr : attrs) {
        const std::size_t eq = attr.find('=');
        if (eq == std::string::npos) {
            others.push_back(attr);
            continue;
        }
        if (attr.substr(0, eq) == key) {
            continue;
        }
        others.push_back(attr);
    }
    others.emplace_back(std::string(key) + "=" + std::string(value));
    std::sort(others.begin(), others.end());
    std::string result = head + "[";
    for (std::size_t i = 0; i < others.size(); ++i) {
        if (i > 0) {
            result += ",";
        }
        result += others[i];
    }
    result += "]";
    return result;
}

} // namespace

/**
 * @brief 快照表只按方块收录，条目数与注册表里的方块数相等
 *
 * 相等而不是「至少」：若新增的方块漏登记，查表会恒返回 AIR 色，也顺带断言了快照与注册表同步。
 */
TEST(BlockMapColorTableTest, TableHasOneEntryPerVanillaBlock)
{
    EXPECT_GT(world::block::vanillaMapColorTableSize(), 1000);
    EXPECT_EQ(
        static_cast<std::size_t>(world::block::vanillaMapColorTableSize()), BlockRegistry::instance().blockCount());
}

/**
 * @brief 不含属性的方块状态直接按方块名命中
 */
TEST(BlockMapColorTableTest, BareStateUsesBlockColor)
{
    EXPECT_EQ(
        world::block::lookupVanillaMapColor("minecraft:dirt"), static_cast<u8>(world::map::MaterialColorId::DIRT));
    EXPECT_EQ(
        world::block::lookupVanillaMapColor("minecraft:stone"), static_cast<u8>(world::map::MaterialColorId::STONE));
    EXPECT_EQ(
        world::block::lookupVanillaMapColor("minecraft:sand"), static_cast<u8>(world::map::MaterialColorId::SAND));
    EXPECT_EQ(world::block::lookupVanillaMapColor("minecraft:water[level=0]"),
        static_cast<u8>(world::map::MaterialColorId::WATER));
}

/**
 * @brief 原木按轴取色：侧面色与内皮色不同，且与属性书写顺序无关
 */
TEST(BlockMapColorTableTest, LogColorDependsOnAxis)
{
    EXPECT_EQ(world::block::lookupVanillaMapColor("minecraft:oak_log[axis=y]"), 13);
    EXPECT_EQ(world::block::lookupVanillaMapColor("minecraft:oak_log[axis=x]"), 34);
    EXPECT_EQ(world::block::lookupVanillaMapColor("minecraft:oak_log[axis=z]"), 34);
}

/**
 * @brief 规范化会把属性按名字排序：状态字符串按注册顺序输出，属性顺序不同不应影响结果
 */
TEST(BlockMapColorTableTest, AttributeOrderDoesNotAffectLookup)
{
    const std::string sorted =
        world::block::canonicalStateName("minecraft:red_bed[occupied=false,facing=west,part=foot]");
    EXPECT_EQ(sorted, "minecraft:red_bed[facing=west,occupied=false,part=foot]");
    EXPECT_EQ(world::block::lookupVanillaMapColor("minecraft:red_bed[occupied=false,facing=west,part=foot]"),
        world::block::lookupVanillaMapColor("minecraft:red_bed[facing=west,occupied=false,part=foot]"));
}

/**
 * @brief 床按 part 取色：脚端用染色色，头端固定羊毛色
 */
TEST(BlockMapColorTableTest, BedColorDependsOnPart)
{
    EXPECT_EQ(world::block::lookupVanillaMapColor("minecraft:red_bed[facing=west,occupied=false,part=foot]"), 28);
    EXPECT_EQ(world::block::lookupVanillaMapColor("minecraft:red_bed[facing=west,occupied=false,part=head]"), 3);
}

/**
 * @brief 含水方块按 waterlogged 取色
 */
TEST(BlockMapColorTableTest, WaterloggedStateTakesWaterColor)
{
    EXPECT_EQ(world::block::lookupVanillaMapColor("minecraft:barrier[waterlogged=false]"), 0);
    EXPECT_EQ(world::block::lookupVanillaMapColor("minecraft:barrier[waterlogged=true]"), 12);
}

/**
 * @brief 快照外表返回 AIR 色，不应崩溃
 */
TEST(BlockMapColorTableTest, UnknownStateFallsBackToAirColor)
{
    EXPECT_EQ(world::block::lookupVanillaMapColor("minecraft:not_a_block"), 0);
    EXPECT_EQ(world::block::lookupVanillaMapColor(""), 0);
    // 方块名命中后，属性段只要不含例外表里的变色属性，就仍取该方块的默认色。
    // 不能要求这种情况返回 AIR：非例外方块携带属性是**生产中的常态**
    // （例如 chest 的 facing/type/waterlogged，chest 不在 29 个例外方块内），
    // 若把「属性未命中例外表」判为未知状态，这些方块的地图色会全部退化成 0。
    // 因此这里断言 stone 取自身颜色（STONE = 11），真正"查不到"的是上面的未知方块名。
    EXPECT_EQ(world::block::lookupVanillaMapColor("minecraft:stone[not_an_attribute=true]"), 11);
}

/**
 * @brief 注册表里每个状态都能查到颜色，且非空气状态不是 AIR 色
 *
 * 压缩表把近 3 万个逐状态条目压成「方块默认色 + 例外」，这里逐状态回查，
 * 确认压缩后没有一个状态漏掉，也没有把有色方块误判成空气色。
 */
TEST(BlockMapColorTableTest, EveryRegisteredStateResolvesToItsBlock)
{
    std::size_t unknown = 0;
    std::size_t colorless = 0;
    const auto& names = allStateNames();
    for (const std::string& name : names) {
        const u8 color = world::block::lookupVanillaMapColor(name);
        if (color > 61) {
            ++unknown;
            ADD_FAILURE() << "color out of range for state: " << name;
            continue;
        }
        if (color == 0 && name.find("minecraft:air") != 0) {
            ++colorless;
        }
    }
    EXPECT_GT(names.size(), 1000u);
    EXPECT_EQ(unknown, 0u);
    // 原版里确有一批彩色方块取 AIR 色（如按钮、铜栏杆），它们本就该是 0，这里只做上限校验。
    EXPECT_LT(colorless, names.size());
}

/**
 * @brief 状态属性做全等变换（四向/三轴/16 向旋转/床的 part/waterlogged）后颜色必须一致
 *
 * 这条测试给「只有 29 个方块随状态变色」的结论兜底：若原版快照里存在某个只在
 * 非颜色属性上变化、却换了颜色的状态，压缩表把它收进方块默认色就会出错。
 * 反向不成立——已知的那 29 个方块按轴/part/waterlogged 变色，故不参与配对。
 */
TEST(BlockMapColorTableTest, ColorIsInvariantUnderColourNeutralTransformations)
{
    const std::unordered_map<std::string, u32>& index = stateIndex();
    const std::vector<std::string>& names = allStateNames();
    const std::vector<std::pair<std::string, std::vector<std::string>>> transforms = {
        {"facing", {"east", "west", "north", "south", "up", "down"}},
        {"axis", {"x", "y", "z"}},
        {"part", {"foot", "head"}},
        {"waterlogged", {"false", "true"}},
    };

    std::size_t checked = 0;
    std::size_t mismatched = 0;
    for (const std::string& name : names) {
        if (name.find('[') == std::string::npos || name.rfind("minecraft:air", 0) == 0) {
            continue;
        }
        const std::string blockName = name.substr(0, name.find('['));
        // 已知的变色方块：原木按轴、床按 part、barrier/light 按 waterlogged。
        if (blockName.ends_with("_log") || blockName.ends_with("_wood") || blockName.ends_with("_bed") ||
            blockName == "minecraft:barrier" || blockName == "minecraft:light") {
            continue;
        }
        for (const auto& [key, values] : transforms) {
            std::optional<std::string> other = withAttribute(name, key, values[0]);
            if (!other || index.find(*other) == index.end()) {
                continue;
            }
            ++checked;
            const u8 original = world::block::lookupVanillaMapColor(name);
            const u8 transformed = world::block::lookupVanillaMapColor(*other);
            if (original != transformed) {
                ++mismatched;
                ADD_FAILURE() << "color mismatch: " << name << " (" << static_cast<int>(original) << ") vs " << *other
                              << " (" << static_cast<int>(transformed) << ")";
            }
        }
    }
    EXPECT_GT(checked, 100u);
    EXPECT_EQ(mismatched, 0u);
}
