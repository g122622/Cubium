#pragma once

#include "EffectInstance.hpp"
#include <vector>
#include <unordered_map>

namespace mc {

class LivingEntity;

namespace entity {
namespace effect {

/**
 * @brief 效果管理器
 *
 * 管理实体身上的所有效果实例。
 * 参考 MC 1.16.5 LivingEntity 的效果管理
 */
class EffectManager {
public:
    EffectManager() = default;

    // ========== 效果管理 ==========

    /**
     * @brief 添加效果
     * @param effect 效果实例
     * @param entity 受影响的实体
     * @return 是否成功添加（如果被覆盖也返回 true）
     */
    bool addEffect(EffectInstance effect, LivingEntity& entity);

    /**
     * @brief 移除效果
     * @param type 效果类型
     * @param entity 受影响的实体
     */
    void removeEffect(EffectType type, LivingEntity& entity);

    /**
     * @brief 移除所有效果
     * @param entity 受影响的实体
     */
    void removeAllEffects(LivingEntity& entity);

    /**
     * @brief 获取效果实例
     * @param type 效果类型
     * @return 效果实例指针，如果不存在返回 nullptr
     */
    [[nodiscard]] const EffectInstance* getEffect(EffectType type) const;
    [[nodiscard]] EffectInstance* getEffect(EffectType type);

    /**
     * @brief 检查是否有效果
     * @param type 效果类型
     */
    [[nodiscard]] bool hasEffect(EffectType type) const;

    /**
     * @brief 获取效果等级
     * @param type 效果类型
     * @return 效果等级（0 = 无效果，1 = I级，2 = II级，等）
     */
    [[nodiscard]] i32 getEffectLevel(EffectType type) const;

    /**
     * @brief 获取所有效果
     */
    [[nodiscard]] const std::vector<EffectInstance>& getAllEffects() const { return m_effects; }
    [[nodiscard]] std::vector<EffectInstance>& getAllEffects() { return m_effects; }

    // ========== 更新 ==========

    /**
     * @brief 更新所有效果（每tick调用）
     * @param entity 受影响的实体
     */
    void tick(LivingEntity& entity);

    // ========== 查询 ==========

    /**
     * @brief 获取效果数量
     */
    [[nodiscard]] size_t getEffectCount() const { return m_effects.size(); }

    /**
     * @brief 检查是否有任何有益效果
     */
    [[nodiscard]] bool hasBeneficialEffect() const;

    /**
     * @brief 检查是否有任何有害效果
     */
    [[nodiscard]] bool hasHarmfulEffect() const;

private:
    /**
     * @brief 查找效果索引
     * @param type 效果类型
     * @return 效果索引，如果不存在返回 -1
     */
    [[nodiscard]] i32 findEffectIndex(EffectType type) const;

private:
    std::vector<EffectInstance> m_effects;
};

} // namespace effect
} // namespace entity
} // namespace mc
