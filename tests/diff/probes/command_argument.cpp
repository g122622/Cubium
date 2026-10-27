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
 * @file command_argument.cpp
 * @brief 差分能力：命令参数类型对一段输入的解析结果。
 *
 * `list`：`<参数类型>\t<输入>` 形式的单元（输入里的反斜杠拼成两个反斜杠，
 *          制表符拼成 `\t`，两侧用同一套转义还原）
 * `run` ：{ "unit": "<类型>\t<输入>" } → { "ok": true, "value": ... }
 *         或 { "ok": false, "cursor": n }
 *
 * 命令解析是玩家输入直接命中的路径：`/time set 1.5d` 少解析一位小数、
 * `/gamemode sp` 被拒，玩家立刻能感觉到。这些参数类型基本都是纯函数（只有
 * 方块状态要读方块注册表），可以逐条与原版比对。
 *
 * 只比对「接受/拒绝 + 解析出的值 + 出错游标」，不比错误文案：原版报错文案是
 * 面向玩家的翻译键拼装结果，文案不同不影响行为，游标不同则说明两侧的阅读
 * 位置已经分叉。
 */
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/command/StringReader.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/BlockStateArgument.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/command/arguments/ItemArgument.hpp"
#include "common/command/arguments/ItemSlotArgument.hpp"
#include "common/command/arguments/NbtPathArgumentType.hpp"
#include "common/command/arguments/TimeArgument.hpp"
#include "common/command/exceptions/CommandExceptions.hpp"
#include "common/item/core/Item.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

struct Case {
    const char* type;
    std::string input;
};

std::vector<Case> cases()
{
    std::vector<Case> list;
    const auto add = [&list](const char* type, std::initializer_list<const char*> inputs) {
        for (const char* input : inputs) {
            list.push_back(Case{type, input});
        }
    };

    // 整数：符号、前导零、溢出、小数点、非 ASCII 数字、行首空白。
    add("int",
        {"0",
            "1",
            "-1",
            "+1",
            "01",
            "2147483647",
            "2147483648",
            "-2147483648",
            "-2147483649",
            "1.0",
            "",
            " 1",
            "1a",
            "\xd9\xa1"});
    // 双精度：原版读的是坐标语法（数字 + 可选小数点 + 可选尾随字母），不认指数。
    add("double",
        {"0",
            "1.5",
            "-1.5",
            ".5",
            "1.",
            "1e3",
            "1E3",
            "1e",
            "NaN",
            "Infinity",
            "-0.0",
            "",
            "1.5.5",
            "0x10",
            "1d",
            "-",
            "+1",
            " 1"});
    // 布尔
    add("bool", {"true", "false", "1", "0", "True", "", "truex"});
    // 三种字符串模式（贪婪短语读整行剩余内容）
    add("string", {"hello", "\"quoted text\"", "hello world", "", "\"unterminated", "a\"b c\"d"});
    add("word", {"hello", "hello world", "\"quoted\"", "", "a_b-c.d"});
    add("greedy", {"hello", "  padded ", "", "\"unterminated"});
    // 资源位置：原版只接受 [a-z0-9_.-] 的命名空间与路径
    add("resource_location",
        {"stone",
            "minecraft:stone",
            "mine craft",
            "Minecraft:Stone",
            "a:b:c",
            "",
            ":x",
            "x:",
            "a_b-c.d",
            "\"quoted\"",
            "minecraft:/leading_slash",
            "minecraft:a b"});
    // 游戏模式：原版只认全名（1.13 之后 s/c/a/sp 与数字简写都已移除）
    add("gamemode",
        {"survival",
            "creative",
            "adventure",
            "spectator",
            "s",
            "c",
            "a",
            "sp",
            "Survival",
            "nope",
            "",
            "survivalx",
            "0",
            "1"});
    // 时间：数字 + 单位（原版用坐标读取 + 单位后缀）
    add("time",
        {"0",
            "100",
            "100t",
            "5s",
            "1d",
            "1.5d",
            "0.1s",
            "-1",
            "1.5t",
            "1.5s",
            "01",
            "+1",
            "1D",
            "1S",
            "1m",
            "",
            " 5s",
            "5 s",
            "999999999",
            "2147483647",
            "1.9s"});
    // 物品栏槽位：原版槽位名表（weapon/armor.*/hotbar.*/inventory.*/container.*/enderchest.*/horse.*/villager.*）
    add("slot",
        {"weapon",
            "weapon.mainhand",
            "weapon.offhand",
            "armor.head",
            "armor.chest",
            "armor.legs",
            "armor.feet",
            "armor.body",
            "container.0",
            "container.53",
            "container.54",
            "hotbar.0",
            "hotbar.8",
            "hotbar.9",
            "hotbar.17",
            "inventory.0",
            "inventory.26",
            "inventory.27",
            "inventory.007",
            "enderchest.0",
            "enderchest.26",
            "enderchest.27",
            "villager.0",
            "villager.7",
            "villager.8",
            "horse.0",
            "horse.14",
            "horse.chest",
            "player.crafting.0",
            "player.cursor",
            "saddle",
            "weapon.mainhandx",
            "",
            "-1",
            "contents",
            "mob.0",
            "0"});
    // NBT 路径
    // 注意：原版对 "a[" 会抛未捕获的 StringIndexOutOfBoundsException（brigadier peek 越过末尾），
    // 属于原版缺陷，不复刻，也不列入用例。
    add("nbt_path",
        {"a",
            "a.b",
            "a.b[0]",
            "a[0].b",
            "",
            "a..b",
            "{x:1}.a",
            "a[-1]",
            "*",
            "a[]",
            "  a",
            "a:b",
            "Steve:Inventory",
            "a:b.c",
            "{k:1}",
            "{k:1}.a[0]"});

    // 坐标读取的补充档位：相对坐标的前缀、负数、前导零、指数。
    add("double", {"~", "~5", "~-5", "^", "^1", "-0", "007", "1.5e2"});
    return list;
}

void ensureBlocks()
{
    static bool s_initialized = false;
    if (!s_initialized) {
        VanillaBlocks::initialize();
        s_initialized = true;
    }
}

/** 浮点按位输出，避免两侧 toString 格式不同掩盖最后一位差异。 */
std::string f64Bits(f64 value)
{
    if (std::isnan(value)) {
        return "nan";
    }
    std::uint64_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    char buffer[24];
    std::snprintf(buffer, sizeof(buffer), "0x%016llx", static_cast<unsigned long long>(bits));
    return buffer;
}

/** 单元里的转义：反斜杠写成两个，制表符写成 `\t`。 */
std::string escape(std::string_view text)
{
    std::string result;
    for (char ch : text) {
        if (ch == '\\') {
            result += "\\\\";
        } else if (ch == '\t') {
            result += "\\t";
        } else {
            result += ch;
        }
    }
    return result;
}

std::string unescape(std::string_view text)
{
    std::string result;
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\\' && i + 1 < text.size()) {
            const char next = text[++i];
            result += (next == 't') ? '\t' : next;
        } else {
            result += text[i];
        }
    }
    return result;
}

void writeList(FILE* out)
{
    for (const auto& c : cases()) {
        std::fprintf(out, "%s\t%s\n", c.type, escape(c.input).c_str());
    }
}

json parseValue(const std::string& type, command::StringReader& reader)
{
    using namespace command;
    if (type == "time") {
        // 原版 TimeArgument 的默认最小值为 Integer.MIN_VALUE；minimum=0 的那几个
        // 变体（/weather 等）留给单测覆盖，这里对齐默认构造。
        TimeArgumentType argument(INT32_MIN);
        return argument.parse(reader);
    }
    if (type == "gamemode") {
        GameModeArgumentType argument;
        return static_cast<i32>(argument.parse(reader));
    }
    if (type == "item") {
        ItemArgumentType argument;
        const ItemInput inputValue = argument.parse(reader);
        const Item* parsedItem = inputValue.getItem();
        return parsedItem != nullptr ? parsedItem->itemLocation().toString() : std::string("<unknown>");
    }
    if (type == "slot") {
        ItemSlotArgumentType argument;
        const auto slot = argument.parse(reader);
        return slot.slotIndex();
    }
    if (type == "int") {
        IntegerArgumentType argument;
        return argument.parse(reader);
    }
    if (type == "double") {
        // 原版 DoubleArgumentType 与坐标读取共用同一套「数字 + 小数点 + 尾随字母」语法。
        FloatArgumentType argument;
        return f64Bits(static_cast<f64>(argument.parse(reader)));
    }
    if (type == "bool") {
        BoolArgumentType argument;
        return argument.parse(reader);
    }
    if (type == "string" || type == "word" || type == "greedy") {
        const auto mode = (type == "word") ? StringArgumentType::StringType::SingleWord
            : (type == "greedy")           ? StringArgumentType::StringType::GreedyPhrase
                                           : StringArgumentType::StringType::QuotablePhrase;
        StringArgumentType argument(mode);
        return argument.parse(reader);
    }
    if (type == "resource_location") {
        ResourceLocationArgumentType argument;
        return argument.parse(reader).toString();
    }
    if (type == "nbt_path") {
        ensureBlocks();
        NbtPathArgumentType argument;
        return argument.parse(reader).toString();
    }
    if (type == "block_state") {
        ensureBlocks();
        BlockStateArgumentType argument;
        const BlockStateInput parsed = argument.parse(reader);
        return parsed.isValid() ? json(parsed.state()->stateId()) : json(nullptr);
    }
    return json{{"unsupported", type}};
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const size_t tab = unit.find('\t');
    if (tab == std::string::npos) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }
    const std::string type = unit.substr(0, tab);
    const std::string text = unescape(unit.substr(tab + 1));

    command::StringReader reader(text);
    try {
        json value = parseValue(type, reader);
        return json{{"unit", unit}, {"ok", true}, {"value", value}}.dump();
    }
    catch (const command::CommandException& e) {
        return json{{"unit", unit}, {"ok", false}, {"cursor", e.cursor()}}.dump();
    }
}

} // namespace

MC_DIFF_DEFINE_PROBE("command_argument", &writeList, &runLine)

} // namespace mc::diff
