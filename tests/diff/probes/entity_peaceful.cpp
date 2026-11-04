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
 * @brief 差分能力：实体类型在和平难度下是否允许生成/召唤
 *        （原版 `EntityType#isAllowedInPeaceful`）。
 *
 * `list`：全部已注册实体类型
 * `run` ：一行 -> { "allowedInPeaceful": bool }
 *
 * ## 规则
 *
 * 原版靠两个独立的构建器开关决定，名字容易看反：
 *
 * | 构建器调用 | 含义 | 和平模式 |
 * |---|---|---|
 * | （不调用） | 默认 | **允许** |
 * | `notInPeaceful()` | 标记为「和平模式禁止」 | **禁止** |
 *
 * 也就是说**默认是允许的**，需要显式声明才禁止。反直觉之处在于：
 * 大多数 `MONSTER` 类别实体确实禁止，但并非全部 ——
 * **潜影贝、僵尸猪灵、猪灵、猪灵蛮兵、疣猪兽** 等都不带 `notInPeaceful`
 * （其中僵尸猪灵在和平模式下仍会存在）。
 *
 * ## 写反的后果
 *
 * - 该禁止的没禁止：和平难度下刷出敌对生物，玩家遵守规则却被打
 * - 该允许的被禁止：`/summon` 与刷怪蛋在和平模式下对某些非敌对生物失效
 *
 * 两个方向都是玩家可直接观察到、但不会崩溃的错误，只能靠逐类型对答案发现。
 */

#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/registry/VanillaEntities.hpp"

#include "probe.hpp"
#include "vanilla_datapack.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

void ensureInitialized()
{
    static const bool s_initialized = []() {
        datapack::initializeRegistries();
        entity::VanillaEntities::registerAll();
        return true;
    }();
    (void)s_initialized;
}

/// 实体类型名（原版是裸路径，探针只关心集合本身）。
std::vector<std::string> typeNames()
{
    std::vector<std::string> names;
    for (const entity::EntityType* type : entity::EntityRegistry::instance().getAllTypes()) {
        names.push_back(type->name());
    }
    std::sort(names.begin(), names.end());
    names.erase(std::unique(names.begin(), names.end()), names.end());
    return names;
}

void writeList(FILE* out)
{
    ensureInitialized();
    for (const std::string& name : typeNames()) {
        std::fprintf(out, "%s\n", name.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureInitialized();
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    for (const entity::EntityType* type : entity::EntityRegistry::instance().getAllTypes()) {
        if (type->name() != unit) {
            continue;
        }
        json result{{"unit", unit}};
        result["allowedInPeaceful"] = type->isAllowedInPeaceful();
        return result.dump();
    }

    return json{{"unit", unit}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("entity_peaceful", &writeList, &runLine)

} // namespace mc::diff
