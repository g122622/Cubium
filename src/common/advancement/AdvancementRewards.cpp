#include "AdvancementRewards.hpp"

namespace mc::advancement {

Result<AdvancementRewards> AdvancementRewards::fromJson(const nlohmann::json& json) {
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

nlohmann::json AdvancementRewards::toJson() const {
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
