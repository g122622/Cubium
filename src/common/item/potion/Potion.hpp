#pragma once

#include "../../entity/effect/EffectInstance.hpp"
#include "../../resource/ResourceLocation.hpp"
#include "PotionType.hpp"
#include <string>
#include <vector>

namespace mc {
namespace potion {

/**
 * @brief 药水类型类
 *
 * 定义一种药水的效果组合和属性。
 * 每种药水可以有多个效果，药水物品存储药水ID。
 *
 * 参考: net.minecraft.potion.Potion
 */
class Potion {
public:
    /**
     * @brief 构造空药水
     */
    Potion();

    /**
     * @brief 构造带基础名称的药水
     * @param baseName 基础名称（如 "night_vision"）
     */
    explicit Potion(std::string_view baseName);

    /**
     * @brief 构造带效果的药水
     * @param baseName 基础名称
     * @param effects 效果列表
     */
    Potion(std::string_view baseName, std::vector<entity::effect::EffectInstance> effects);

    /**
     * @brief 构造带单个效果的药水
     * @param effect 效果实例
     */
    explicit Potion(const entity::effect::EffectInstance& effect);

    /**
     * @brief 构造带多个效果的药水
     * @param baseName 基础名称
     * @param effects 效果数组
     */
    template <std::size_t N>
    Potion(std::string_view baseName, const entity::effect::EffectInstance (&effects)[N])
        : m_baseName(baseName)
        , m_effects(effects, effects + N)
        , m_id(nullptr)
    {}

    // ========== 属性获取 ==========

    /**
     * @brief 获取基础名称
     */
    [[nodiscard]] const std::string& baseName() const { return m_baseName; }

    /**
     * @brief 获取效果列表
     */
    [[nodiscard]] const std::vector<entity::effect::EffectInstance>& effects() const { return m_effects; }

    /**
     * @brief 是否有任何效果
     */
    [[nodiscard]] bool hasEffects() const { return !m_effects.empty(); }

    /**
     * @brief 是否有瞬间效果
     *
     * 瞬间效果包括：瞬间治疗、瞬间伤害
     */
    [[nodiscard]] bool hasInstantEffect() const;

    /**
     * @brief 获取带前缀的名称
     *
     * 用于生成翻译键，如 "potion.effect.minecraft.night_vision"
     *
     * @param prefix 前缀（如 "potion", "splash_potion", "lingering_potion"）
     * @return 完整名称
     */
    [[nodiscard]] std::string getNamePrefixed(std::string_view prefix) const;

    /**
     * @brief 获取药水ID
     * @return 资源位置，未注册返回空ResourceLocation
     */
    [[nodiscard]] ResourceLocation id() const { return m_id ? *m_id : ResourceLocation(); }

    // ========== 内部方法（供注册表使用） ==========

    /**
     * @brief 设置药水ID（仅供注册表使用）
     * @param id 资源位置
     */
    void setId(const ResourceLocation* id) { m_id = id; }

private:
    std::string m_baseName;
    std::vector<entity::effect::EffectInstance> m_effects;
    const ResourceLocation* m_id = nullptr; // 指向注册表中的ID

    friend class PotionRegistry;
};

} // namespace potion
} // namespace mc
