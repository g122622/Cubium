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
 * @file gamerule.cpp
 * @brief 差分能力：游戏规则的分类、类型、默认值与取值范围。
 *
 * `list`：全部规则名（不带命名空间，与 `/gamerule <name>` 的写法一致）
 * `run` ：{ "rule": "mob_griefing" }
 *         → { "category": "mobs", "type": "boolean", "default": true, "min"/"max" }
 *
 * 规则名决定 `/gamerule` 的补全列表与存档里的键名，默认值决定新世界的初始行为。
 * 这两样都在手写表里，写错不会报错：默认值写成反的，玩家只能靠「僵尸怎么不掉装备了」
 * 之类的间接现象察觉；规则名拼错则会让整个规则对存档静默失效。
 */
#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/world/gamerule/GameRule.hpp"
#include "common/world/gamerule/GameRules.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;
using namespace world::gamerule;

/// 分类名：与原版 `GameRuleCategory` 的枚举名（小写）一致。
const char* categoryText(GameRuleCategory category)
{
    switch (category) {
        case GameRuleCategory::Player:
            return "player";
        case GameRuleCategory::Mobs:
            return "mobs";
        case GameRuleCategory::Spawning:
            return "spawning";
        case GameRuleCategory::Drops:
            return "drops";
        case GameRuleCategory::Updates:
            return "updates";
        case GameRuleCategory::Chat:
            return "chat";
        case GameRuleCategory::Misc:
            return "misc";
    }
    return "unknown";
}

/** 一条规则的摘要；布尔与整型共用，靠 `type` 区分。 */
struct RuleSummary {
    std::string name;
    std::string category;
    std::string type;
    json defaultValue;
    json minValue;
    json maxValue;
};

/** 收集全部规则（`visitAll` 的顺序是注册顺序，这里按名字排序保证确定性）。 */
std::vector<RuleSummary> collectRules()
{
    class Collector final : public IGameRuleVisitor {
    public:
        void visitBoolean(const BooleanGameRuleKey& key, const BooleanGameRuleType& type) override
        {
            // 布尔规则在 Cubium 里也带范围（越界常量），原版没有；只报默认值即可。
            rules.push_back({key.getName(), categoryText(key.getCategory()), "boolean", type.getDefaultValue()});
        }

        void visitInteger(const IntegerGameRuleKey& key, const IntegerGameRuleType& type) override
        {
            rules.push_back({key.getName(),
                categoryText(key.getCategory()),
                "integer",
                type.getDefaultValue(),
                type.getMinValue(),
                type.getMaxValue()});
        }

        std::vector<RuleSummary> rules;
    } collector;

    GameRules::visitAll(collector);
    std::sort(collector.rules.begin(), collector.rules.end(), [](const RuleSummary& a, const RuleSummary& b) {
        return a.name < b.name;
    });
    collector.rules.erase(std::unique(collector.rules.begin(),
                              collector.rules.end(),
                              [](const RuleSummary& a, const RuleSummary& b) { return a.name == b.name; }),
        collector.rules.end());
    return collector.rules;
}

const std::vector<RuleSummary>& cachedRules()
{
    static const std::vector<RuleSummary> s_rules = collectRules();
    return s_rules;
}

void writeList(FILE* out)
{
    for (const auto& rule : cachedRules()) {
        std::fprintf(out, "%s\n", rule.name.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string requested = input.at("rule").get<std::string>();

    for (const auto& rule : cachedRules()) {
        if (rule.name != requested) {
            continue;
        }
        json result{{"rule", rule.name}, {"category", rule.category}, {"type", rule.type}};
        result["default"] = rule.defaultValue;
        // 布尔规则没有取值范围（原版的 BoolArgumentType 也不限制），整型才报 min/max。
        if (rule.minValue.is_number()) {
            result["min"] = rule.minValue;
            result["max"] = rule.maxValue;
        }
        return result.dump();
    }
    return json{{"rule", requested}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("gamerule", &writeList, &runLine)

} // namespace mc::diff