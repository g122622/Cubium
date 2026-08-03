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

#include "AdvancementRewards.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc::advancement {

Result<AdvancementRewards> AdvancementRewards::fromJson(const nlohmann::json& json)
{
    if (!json.is_object()) {
        return Error(ErrorCode::ResourceParseError, "Rewards must be a JSON object");
    }

    u32 experience = 0;
    std::vector<ResourceLocation> loot;
    std::vector<ResourceLocation> recipes;
    std::optional<ResourceLocation> function;

    // 解析经验值
    if (json.contains("experience")) {
        experience = json["experience"].get<u32>();
    }

    // 解析战利品表
    if (json.contains("loot") && json["loot"].is_array()) {
        for (const auto& item : json["loot"]) {
            std::string lootId = item.get<std::string>();
            loot.push_back(ResourceLocation::parse(lootId));
        }
    }

    // 解析配方
    if (json.contains("recipes") && json["recipes"].is_array()) {
        for (const auto& item : json["recipes"]) {
            std::string recipeId = item.get<std::string>();
            recipes.push_back(ResourceLocation::parse(recipeId));
        }
    }

    // 解析函数
    if (json.contains("function")) {
        std::string functionId = json["function"].get<std::string>();
        function = ResourceLocation::parse(functionId);
    }

    return AdvancementRewards(experience, std::move(loot), std::move(recipes), std::move(function));
}

nlohmann::json AdvancementRewards::toJson() const
{
    if (isEmpty()) {
        return nullptr;
    }

    nlohmann::json json;

    if (m_experience > 0) {
        json["experience"] = m_experience;
    }

    if (!m_loot.empty()) {
        nlohmann::json lootArray = nlohmann::json::array();
        for (const auto& loc : m_loot) {
            lootArray.push_back(loc.toString());
        }
        json["loot"] = std::move(lootArray);
    }

    if (!m_recipes.empty()) {
        nlohmann::json recipesArray = nlohmann::json::array();
        for (const auto& loc : m_recipes) {
            recipesArray.push_back(loc.toString());
        }
        json["recipes"] = std::move(recipesArray);
    }

    if (m_function.has_value()) {
        json["function"] = m_function->toString();
    }

    return json;
}

} // namespace mc::advancement
