#pragma once

#include "../CriterionTrigger.hpp"
#include "../conditions/LocationPredicate.hpp"
#include <memory>

namespace mc::advancement {

// 前向声明 Instance 类
class LocationTriggerInstance;

/**
 * @brief 位置触发器
 *
 * 当玩家位于特定位置时触发。
 * 也用于 slept_in_bed, hero_of_the_village, voluntary_exile 等。
 * 参考 MC 1.16.5: net.minecraft.advancements.criterion.LocationTrigger
 */
class LocationTrigger : public AbstractCriterionTrigger<LocationTriggerInstance> {
public:
    /**
     * @brief 触发器ID
     */
    static constexpr const char* TRIGGER_ID = "minecraft:location";

    /**
     * @brief 获取触发器ID
     */
    [[nodiscard]] ResourceLocation getId() const override {
        return ResourceLocation(TRIGGER_ID);
    }

    /**
     * @brief 从JSON反序列化实例
     */
    [[nodiscard]] Result<std::shared_ptr<LocationTriggerInstance>> fromJson(const nlohmann::json& json);

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
    [[nodiscard]] bool test(const class World& world, f64 x, f64 y, f64 z) const;

    /**
     * @brief 从JSON解析
     */
    Result<void> fromJson(const nlohmann::json& json);

    /**
     * @brief 序列化条件为JSON
     */
    [[nodiscard]] nlohmann::json conditionsToJson() const;

    // ========== Getters ==========

    [[nodiscard]] const LocationPredicate& getLocation() const noexcept { return m_location; }

private:
    LocationPredicate m_location;
};

/**
 * @brief 睡觉触发器
 *
 * 当玩家在床上睡觉时触发。
 * 继承 LocationTrigger 但使用不同的触发器ID。
 */
class SleptInBedTrigger : public LocationTrigger {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:slept_in_bed";

    [[nodiscard]] ResourceLocation getId() const override {
        return ResourceLocation(TRIGGER_ID);
    }
};

/**
 * @brief 村庄英雄触发器
 *
 * 当玩家在村庄获得"村庄英雄"效果时触发。
 */
class HeroOfTheVillageTrigger : public LocationTrigger {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:hero_of_the_village";

    [[nodiscard]] ResourceLocation getId() const override {
        return ResourceLocation(TRIGGER_ID);
    }
};

/**
 * @brief 自愿流放触发器
 *
 * 当玩家获得"不祥之兆"效果时触发。
 */
class VoluntaryExileTrigger : public LocationTrigger {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:voluntary_exile";

    [[nodiscard]] ResourceLocation getId() const override {
        return ResourceLocation(TRIGGER_ID);
    }
};

} // namespace mc::advancement
