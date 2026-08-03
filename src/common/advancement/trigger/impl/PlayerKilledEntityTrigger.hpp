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

#pragma once

#include "common/advancement/trigger/CriterionTrigger.hpp"
#include "common/advancement/trigger/conditions/EntityPredicate.hpp"
#include "common/advancement/trigger/conditions/LocationPredicate.hpp"
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <nlohmann/json_fwd.hpp>

// 前向声明
namespace mc {
class Entity;
class DamageSource;
} // namespace mc

namespace mc::advancement {

// 前向声明 Instance 类
class PlayerKilledEntityTriggerInstance;
class EntityKilledPlayerTriggerInstance;

/**
 * @brief 玩家击杀实体触发器
 *
 * 当玩家击杀实体时触发。
 */
class PlayerKilledEntityTrigger : public AbstractCriterionTrigger<PlayerKilledEntityTriggerInstance> {
public:
    /**
     * @brief 触发器ID
     */
    static constexpr const char* TRIGGER_ID = "minecraft:player_killed_entity";

    /**
     * @brief 获取触发器ID
     */
    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

    /**
     * @brief 从JSON反序列化实例
     */
    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    /**
     * @brief 触发检测
     * @param player 玩家
     * @param entity 被击杀的实体
     * @param source 伤害源
     */
    void trigger(class ServerPlayer& player, const Entity& entity, const DamageSource& source);

    // ========== 静态工厂方法 ==========

    static std::shared_ptr<PlayerKilledEntityTriggerInstance> entityKilled();
    static std::shared_ptr<PlayerKilledEntityTriggerInstance> entityKilled(const EntityPredicate& entity);
    static std::shared_ptr<PlayerKilledEntityTriggerInstance> killedByEntity(const EntityPredicate& killer);
};

/**
 * @brief 玩家击杀实体触发器实例
 */
class PlayerKilledEntityTriggerInstance : public CriterionInstance<PlayerKilledEntityTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:player_killed_entity";

    PlayerKilledEntityTriggerInstance() noexcept = default;

    /**
     * @brief 构造实例
     * @param entity 实体谓词
     * @param killingBlow 伤害源谓词
     */
    PlayerKilledEntityTriggerInstance(EntityPredicate entity, DamageSourcePredicate killingBlow);

    /**
     * @brief 检查条件是否满足
     * @param entity 被击杀的实体
     * @param source 伤害源
     * @return 是否满足
     */
    [[nodiscard]] bool test(const Entity& entity, const DamageSource& source) const;

    /**
     * @brief 从JSON解析
     */
    Result<void> fromJson(const nlohmann::json& json);

    /**
     * @brief 序列化条件为JSON
     */
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    EntityPredicate m_entity;
    DamageSourcePredicate m_killingBlow;
};

/**
 * @brief 实体击杀玩家触发器
 *
 * 当实体击杀玩家时触发。
 */
class EntityKilledPlayerTrigger : public AbstractCriterionTrigger<EntityKilledPlayerTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:entity_killed_player";

    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    void trigger(class ServerPlayer& player, const Entity& entity, const DamageSource& source);
};

/**
 * @brief 实体击杀玩家触发器实例
 */
class EntityKilledPlayerTriggerInstance : public CriterionInstance<EntityKilledPlayerTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:entity_killed_player";

    EntityKilledPlayerTriggerInstance() noexcept = default;
    EntityKilledPlayerTriggerInstance(EntityPredicate entity, DamageSourcePredicate killingBlow);

    /**
     * @brief 检查条件是否满足
     * @param entity 击杀者实体
     * @param source 伤害源
     * @return 是否满足
     */
    [[nodiscard]] bool test(const Entity& entity, const DamageSource& source) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    EntityPredicate m_entity;
    DamageSourcePredicate m_killingBlow;
};

} // namespace mc::advancement
