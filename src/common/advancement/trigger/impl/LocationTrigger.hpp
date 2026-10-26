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
#include "common/advancement/trigger/conditions/CriterionContext.hpp"
#include "common/advancement/trigger/conditions/LocationPredicate.hpp"
#include "common/advancement/trigger/impl/GenericTriggers.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <nlohmann/json_fwd.hpp>

namespace mc::advancement {

// 前向声明 Instance 类
class LocationTriggerInstance;

/**
 * @brief 位置触发器
 *
 * 玩家进入新方块位置 / 切换维度时派发，条件只有 player（位置就在玩家上下文里）。
 */
class LocationTrigger : public AbstractCriterionTrigger<PlayerContextTriggerInstance> {
public:
    /**
     * @brief 触发器ID
     */
    static constexpr const char* TRIGGER_ID = "minecraft:location";

    /**
     * @brief 获取触发器ID
     */
    [[nodiscard]] ResourceLocation getId() const override
    {
        return ResourceLocation(TRIGGER_ID);
    }

    /**
     * @brief 触发检测
     * @param player 玩家
     */
    void trigger(class ServerPlayer& player);

    // ========== 静态工厂方法 ==========

    /**
     * @brief 在指定位置
     */
    static std::shared_ptr<LocationTriggerInstance> atLocation(const LocationPredicate& location);

    /**
     * @brief 在指定生物群系
     */
    static std::shared_ptr<LocationTriggerInstance> inBiome(const ResourceLocation& biome);

    /**
     * @brief 在指定维度
     */
    static std::shared_ptr<LocationTriggerInstance> inDimension(const ResourceLocation& dimension);
};

/**
 * @brief 位置触发器实例
 */
class LocationTriggerInstance : public CriterionInstance<LocationTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:location";

    LocationTriggerInstance() = default;

    /**
     * @brief 构造实例
     * @param location 位置谓词
     */
    explicit LocationTriggerInstance(LocationPredicate location);

    /**
     * @brief 检查条件是否满足
     * @param world 世界
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @return 是否满足
     */
    [[nodiscard]] bool test(const class IWorld& world, f64 x, f64 y, f64 z) const;

    /**
     * @brief 携带上下文判定（"player" 条件支持 ContextAwarePredicate 条件列表写法，
     *        如 [{"condition":"minecraft:entity_properties","entity":"this",
     *              "predicate":{"location":{"biomes":"minecraft:badlands"}}}]）
     */
    [[nodiscard]] bool test(const CriterionContext& context) const;

    /**
     * @brief 从JSON解析
     */
    Result<void> fromJson(const nlohmann::json& json);

    /**
     * @brief 序列化条件为JSON
     */
    [[nodiscard]] nlohmann::json conditionsToJson() const;

    // ========== Getters ==========

    [[nodiscard]] const LocationPredicate& getLocation() const noexcept
    {
        return m_location;
    }

private:
    LocationPredicate m_location;
    /// "player" 条件的上下文写法（数据包 adventuring_time 等使用）
    ContextAwarePredicate m_playerContext;
};

/**
 * @brief 睡觉触发器
 *
 * 玩家在床上入睡时触发。条件只有 player（vanilla SleptInBedTrigger：空 TriggerInstance，
 * 位置信息在上下文里）。
 */
class SleptInBedTrigger : public AbstractCriterionTrigger<PlayerContextTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:slept_in_bed";

    [[nodiscard]] ResourceLocation getId() const override
    {
        return ResourceLocation(TRIGGER_ID);
    }
};

/**
 * @brief 村庄英雄触发器
 *
 * 玩家获得「村庄英雄」效果时触发（vanilla PlayerTrigger）。
 */
class HeroOfTheVillageTrigger : public AbstractCriterionTrigger<PlayerContextTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:hero_of_the_village";

    [[nodiscard]] ResourceLocation getId() const override
    {
        return ResourceLocation(TRIGGER_ID);
    }
};

/**
 * @brief 自愿流放触发器
 *
 * 玩家获得「不祥之兆」效果时触发（vanilla PlayerTrigger）。
 */
class VoluntaryExileTrigger : public AbstractCriterionTrigger<PlayerContextTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:voluntary_exile";

    [[nodiscard]] ResourceLocation getId() const override
    {
        return ResourceLocation(TRIGGER_ID);
    }
};

} // namespace mc::advancement
