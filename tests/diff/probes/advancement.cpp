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
 * @file advancement.cpp
 * @brief 差分能力：进度（advancement）定义的加载结果。
 *
 * `list`：数据包里全部进度 ID（`minecraft:story/mine_stone`）
 * `run` ：{ "advancement": "minecraft:story/mine_stone" } → 该进度的结构化摘要
 *
 * 比对的是**定义本身**（父进度、显示信息、条件、需求、奖励、遥测开关），不是玩家的
 * 完成状态：后者取决于运行时事件，不在静态数据的可比范围内。
 */

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

#include <nlohmann/json.hpp>

#include "common/advancement/Advancement.hpp"
#include "common/advancement/AdvancementDisplay.hpp"
#include "common/advancement/AdvancementLoader.hpp"
#include "common/advancement/AdvancementManager.hpp"
#include "common/advancement/AdvancementRewards.hpp"
#include "common/advancement/Criterion.hpp"
#include "common/advancement/trigger/CriterionTriggers.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/TranslationTextComponent.hpp"
#include "vanilla_datapack.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 加载后的进度表：ID → 定义。
std::map<std::string, advancement::Advancement::Ptr>& advancements()
{
    static std::map<std::string, advancement::Advancement::Ptr> s_advancements;
    return s_advancements;
}

/**
 * @brief 文本组件的结构化形式
 *
 * 只保留「哪种文本 + 参数」：翻译键与字面量文本在两侧都必须一致，样式（颜色、粗体）
 * 属于客户端渲染细节，数据包里极少出现，暂不比对。
 */
[[nodiscard]] json summariseText(const text::ITextComponent& component)
{
    if (const auto* translatable = dynamic_cast<const text::TranslationTextComponent*>(&component);
        translatable != nullptr) {
        return json{{"translate", translatable->getKey()}};
    }
    return json{{"text", component.getUnformattedText()}};
}

/// 显示信息；进度没有 `display` 时输出 null（原版同样允许隐藏的进度）。
[[nodiscard]] json summariseDisplay(const advancement::AdvancementDisplay& display)
{
    const ItemStack& icon = display.getIcon();
    json iconJson = json::object();
    if (!icon.isEmpty() && icon.getItem() != nullptr) {
        iconJson = json{
            {"id", icon.getItem()->itemLocation().toString()},
            {"count", icon.getCount()},
        };
    }
    return json{
        {"icon", std::move(iconJson)},
        {"title", summariseText(display.getTitle())},
        {"description", summariseText(display.getDescription())},
        {"frame", advancement::toString(display.getFrame())},
        {"background", display.getBackground() ? json(display.getBackground()->texturePath.toString()) : json(nullptr)},
        {"toast", display.shouldShowToast()},
        {"announce", display.shouldAnnounceToChat()},
        {"hidden", display.isHidden()},
    };
}

[[nodiscard]] json summariseRewards(const advancement::AdvancementRewards& rewards)
{
    json loot = json::array();
    for (const auto& id : rewards.getLoot()) {
        loot.push_back(id.toString());
    }
    std::sort(loot.begin(), loot.end());

    json recipes = json::array();
    for (const auto& id : rewards.getRecipes()) {
        recipes.push_back(id.toString());
    }
    std::sort(recipes.begin(), recipes.end());

    return json{
        {"experience", rewards.getExperience()},
        {"loot", std::move(loot)},
        {"recipes", std::move(recipes)},
        {"function", rewards.getFunction() ? json(rewards.getFunction()->toString()) : json(nullptr)},
    };
}

/// 条件：名称 → 触发器 ID。条件参数（谓词）不在这一层比对。
[[nodiscard]] json summariseCriteria(const std::map<std::string, advancement::Criterion>& criteria)
{
    json result = json::object();
    for (const auto& [name, criterion] : criteria) {
        result[name] = criterion.getTrigger().toString();
    }
    return result;
}

void loadAll()
{
    static bool s_initialised = false;
    if (s_initialised) {
        return;
    }
    s_initialised = true;

    datapack::initializeRegistries();
    // 触发器注册表是空的时候所有条件都会退化成「未注册」的兜底实例，触发器 ID 也就对不上。
    advancement::CriterionTriggers::instance().registerBuiltinTriggers();

    const auto advancementDir = datapack::root() / "data" / "minecraft" / "advancement";
    advancement::AdvancementLoader loader;
    const auto files = datapack::jsonFiles(advancementDir);
    for (const auto& file : files) {
        std::FILE* handle = std::fopen(file.generic_string().c_str(), "rb");
        if (handle == nullptr) {
            continue;
        }
        std::string contents;
        char buffer[4096];
        while (const std::size_t read = std::fread(buffer, 1, sizeof(buffer), handle)) {
            contents.append(buffer, read);
        }
        std::fclose(handle);

        // 资源路径 → 进度 ID：`<ns>/advancement/<path>.json`。Loader 的 pathToAdvancementId
        // 是保护成员，这里按同一条规则从接近根的相对路径推导。
        const auto relative = std::filesystem::relative(file, datapack::root() / "data").generic_string();
        const auto slash = relative.find('/');
        if (slash == std::string::npos) {
            continue;
        }
        std::string rest = relative.substr(slash + 1);
        std::string namespaceName = relative.substr(0, slash);
        const std::string prefix = "advancement/";
        if (rest.rfind(prefix, 0) != 0) {
            continue;
        }
        rest = rest.substr(prefix.size());
        rest = rest.substr(0, rest.size() - std::string(".json").size());

        const ResourceLocation id(namespaceName + ":" + rest);
        auto parsed = loader.loadJson(id, contents);
        if (parsed.failed()) {
            // 加载失败不静默：写 stderr，原版侧会有这个单元而探针没有，比对报告里会体现。
            std::fprintf(stderr, "advancement: %s: %s\n", id.toString().c_str(), parsed.error().toString().c_str());
            continue;
        }
        advancements()[id.toString()] = std::make_shared<advancement::Advancement>(std::move(parsed).value());
    }
}

void writeList(FILE* out)
{
    loadAll();
    for (const auto& [id, unused] : advancements()) {
        std::fprintf(out, "%s\n", id.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    loadAll();
    const json input = json::parse(inputLine);
    const std::string id = input.at("advancement").get<std::string>();

    const auto found = advancements().find(id);
    if (found == advancements().end()) {
        return json{{"advancement", id}, {"missing", true}}.dump();
    }
    const advancement::Advancement& adv = *found->second;

    json result = json{
        {"advancement", id},
        {"parent", adv.getParent() ? json(adv.getParent()->toString()) : json(nullptr)},
        {"display", adv.getDisplay() ? summariseDisplay(*adv.getDisplay()) : json(nullptr)},
        {"criteria", summariseCriteria(adv.getCriteria())},
        {"requirements", adv.getRequirements()},
        {"rewards", summariseRewards(adv.getRewards().value_or(advancement::AdvancementRewards{}))},
    };
    return result.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("advancement", &writeList, &runLine)

} // namespace mc::diff
