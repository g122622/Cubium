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
 * @brief 差分能力：实体类型的**存档序列化**与**召唤**开关
 *        （原版 `EntityType#canSerialize` / `canSummon`）。
 *
 * `list`：全部已注册实体类型
 * `run` ：一行 -> { "serialize": bool, "summon": bool }
 *
 * ## 两个开关
 *
 * | 开关 | 默认 | 关闭后 | 关闭的实体 |
 * |---|---|---|---|
 * | `serialize` | **true** | 区块保存时被丢弃，重进世界就消失 | 拴绳结、闪电、玩家、浮标 |
 * | `summon` | **true** | `/summon` 与刷怪蛋无法生成 | （原版没有关闭的） |
 *
 * ## 为什么这两个容易抄反
 *
 * - **`serialize` 默认是 true**，要显式 `noSave()` 才关。反过来写的话，
 *   所有未显式声明的实体都不会进存档 —— 玩家造的东西重进世界全没了。
 * - **`summon` 默认也是 true**，原版根本没有实体关它。如果实现里把默认值
 *   写成 false、靠逐个实体显式打开，那么只要漏一个，`/summon` 就会失效，
 *   且只在玩家尝试时才发现。
 *
 * 这两个都是「默认值写反」型缺陷：不会崩溃、不会报错，只是某些实体
 * 悄悄失去存档或召唤能力。
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
        result["serialize"] = type->serializable();
        result["summon"] = type->canSummon();
        return result.dump();
    }

    return json{{"unit", unit}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("entity_serialize_summon", &writeList, &runLine)

} // namespace mc::diff
