#pragma once

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <optional>
#include <vector>
#include <nlohmann/json.hpp>

namespace mc::advancement {

/**
 * @brief 成就奖励
 *
 * 当成就完成时发放的奖励，包括经验、战利品表、配方和函数。
 * 参考 MC 1.16.5: net.minecraft.advancements.AdvancementRewards
 *
 * JSON格式示例：
 * @code
 * {
 *   "experience": 100,
 *   "loot": ["minecraft:chests/spawn_bonus_chest"],
 *   "recipes": ["minecraft:diamond_sword"],
 *   "function": "minecraft:my_function"
 * }
 * @endcode
 */
class AdvancementRewards {
public:
    AdvancementRewards() = default;

    /**
     * @brief 构造奖励
     * @param experience 经验值
     * @param loot 战利品表ID列表
     * @param recipes 配方ID列表
     * @param function 函数ID
     */
    AdvancementRewards(u32 experience,
        std::vector<ResourceLocation> loot,
        std::vector<ResourceLocation> recipes,
        std::optional<ResourceLocation> function)
        : m_experience(experience)
        , m_loot(std::move(loot))
        , m_recipes(std::move(recipes))
        , m_function(std::move(function))
    {}

    /**
     * @brief 获取经验值奖励
     */
    [[nodiscard]] u32 getExperience() const noexcept { return m_experience; }

    /**
     * @brief 获取战利品表列表
     */
    [[nodiscard]] const std::vector<ResourceLocation>& getLoot() const noexcept { return m_loot; }

    /**
     * @brief 获取配方列表
     */
    [[nodiscard]] const std::vector<ResourceLocation>& getRecipes() const noexcept { return m_recipes; }

    /**
     * @brief 获取函数ID
     */
    [[nodiscard]] const std::optional<ResourceLocation>& getFunction() const noexcept { return m_function; }

    /**
     * @brief 是否为空奖励
     */
    [[nodiscard]] bool isEmpty() const noexcept
    {
        return m_experience == 0 && m_loot.empty() && m_recipes.empty() && !m_function.has_value();
    }

    /**
     * @brief 从JSON解析
     * @param json JSON对象
     * @return 奖励或错误
     */
    static Result<AdvancementRewards> fromJson(const nlohmann::json& json);

    /**
     * @brief 序列化为JSON
     */
    [[nodiscard]] nlohmann::json toJson() const;

    /**
     * @brief 空奖励实例
     */
    static AdvancementRewards empty() { return AdvancementRewards(); }

private:
    u32 m_experience = 0;
    std::vector<ResourceLocation> m_loot;
    std::vector<ResourceLocation> m_recipes;
    std::optional<ResourceLocation> m_function;
};

} // namespace mc::advancement
