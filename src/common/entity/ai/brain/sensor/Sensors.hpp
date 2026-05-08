#pragma once

#include "Sensor.hpp"
#include "../Brain.hpp"
#include "../../../core/LivingEntity.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../../core/AgeableEntity.hpp"
#include "../../../core/EntityUtils.hpp"
#include "../../../entities/player/Player.hpp"
#include <vector>
#include <algorithm>

namespace mc {
class IWorld;

namespace world {
namespace village {
class VillageManager;
namespace poi {
class PointOfInterestStorage;
enum class PointOfInterestType : u16;
}
}
class GlobalPos;
}
}

namespace mc {
namespace entity {
namespace ai {
namespace brain {
namespace sensor {

/**
 * @brief 最近玩家传感器
 *
 * 检测附近的玩家并存储到记忆模块。
 * 参考 MC 1.16.5 NearestPlayersSensor
 *
 * @tparam E 实体类型，需要有 brain() 方法返回 Brain<E>&
 */
template <typename E>
class NearestPlayersSensor : public Sensor<E> {
public:
    NearestPlayersSensor()
        : Sensor<E>(20)  // 每20tick更新一次
    {}

    [[nodiscard]] std::unordered_set<const memory::MemoryModuleTypeBase*> getUsedMemories() const override {
        return {
            memory::MemoryModuleTypes::NEAREST_PLAYERS,
            memory::MemoryModuleTypes::NEAREST_VISIBLE_PLAYER,
            memory::MemoryModuleTypes::NEAREST_VISIBLE_TARGETABLE_PLAYER
        };
    }

protected:
    void update(IWorld* world, E* entity) override;
};

/**
 * @brief 最近可见生物传感器
 *
 * 检测附近可见的生物并存储到记忆模块。
 * 参考 MC 1.16.5 NearestVisibleLivingEntitySensor
 *
 * @tparam E 实体类型，需要有 brain() 方法返回 Brain<E>&
 */
template <typename E>
class NearestVisibleLivingEntitySensor : public Sensor<E> {
public:
    explicit NearestVisibleLivingEntitySensor(f32 range = 16.0f, i32 interval = 20)
        : Sensor<E>(interval)
        , m_range(range)
    {}

    [[nodiscard]] std::unordered_set<const memory::MemoryModuleTypeBase*> getUsedMemories() const override {
        return {
            memory::MemoryModuleTypes::VISIBLE_MOBS,
            memory::MemoryModuleTypes::NEAREST_VISIBLE_NEMESIS
        };
    }

protected:
    void update(IWorld* world, E* entity) override;

private:
    f32 m_range;
};

/**
 * @brief 受伤传感器
 *
 * 检测实体受到的伤害并存储到记忆模块。
 * 参考 MC 1.16.5 HurtBySensor
 *
 * @tparam E 实体类型，需要有 brain() 方法返回 Brain<E>&
 */
template <typename E>
class HurtBySensor : public Sensor<E> {
public:
    HurtBySensor()
        : Sensor<E>(1)  // 每 tick 检查（伤害需要快速响应）
    {}

    [[nodiscard]] std::unordered_set<const memory::MemoryModuleTypeBase*> getUsedMemories() const override {
        return {
            memory::MemoryModuleTypes::HURT_BY,
            memory::MemoryModuleTypes::HURT_BY_ENTITY
        };
    }

protected:
    void update(IWorld* world, E* entity) override;
};

/**
 * @brief 附近实体传感器
 *
 * 检测附近的所有实体并分类存储。
 * 参考 MC 1.16.5 MobSensor
 *
 * @tparam E 实体类型，需要有 brain() 方法返回 Brain<E>&
 */
template <typename E>
class MobSensor : public Sensor<E> {
public:
    explicit MobSensor(f32 range = 16.0f, i32 interval = 20)
        : Sensor<E>(interval)
        , m_range(range)
    {}

    [[nodiscard]] std::unordered_set<const memory::MemoryModuleTypeBase*> getUsedMemories() const override {
        return {
            memory::MemoryModuleTypes::MOBS,
            memory::MemoryModuleTypes::NEAREST_HOSTILE
        };
    }

protected:
    void update(IWorld* world, E* entity) override;

private:
    f32 m_range;
};

/**
 * @brief 工作站点传感器
 *
 * 检测村民的工作站点。
 * 参考 MC 1.16.5 SecondaryPointsSensor
 *
 * @tparam E 实体类型，需要有 brain() 方法返回 Brain<E>&
 */
template <typename E>
class WorkStationSensor : public Sensor<E> {
public:
    WorkStationSensor()
        : Sensor<E>(40)  // 每2秒更新一次
    {}

    [[nodiscard]] std::unordered_set<const memory::MemoryModuleTypeBase*> getUsedMemories() const override {
        return {
            memory::MemoryModuleTypes::JOB_SITE,
            memory::MemoryModuleTypes::POTENTIAL_JOB_SITE
        };
    }

protected:
    void update(IWorld* world, E* entity) override;
};

/**
 * @brief 居住点传感器
 *
 * 检测村民的家和集会点。
 * 参考 MC 1.16.5 VillagerPoiSensor
 *
 * @tparam E 实体类型，需要有 brain() 方法返回 Brain<E>&
 */
template <typename E>
class VillagePoiSensor : public Sensor<E> {
public:
    VillagePoiSensor()
        : Sensor<E>(40)  // 每2秒更新一次
    {}

    [[nodiscard]] std::unordered_set<const memory::MemoryModuleTypeBase*> getUsedMemories() const override {
        return {
            memory::MemoryModuleTypes::HOME,
            memory::MemoryModuleTypes::MEETING_POINT,
            memory::MemoryModuleTypes::NEAREST_BED
        };
    }

protected:
    void update(IWorld* world, E* entity) override;
};

/**
 * @brief 幼崽传感器
 *
 * 检测附近的幼年和成年实体。
 * 参考 MC 1.16.5 GolemSensor
 *
 * @tparam E 实体类型，需要有 brain() 方法返回 Brain<E>&
 */
template <typename E>
class BabySensor : public Sensor<E> {
public:
    BabySensor()
        : Sensor<E>(20)
    {}

    [[nodiscard]] std::unordered_set<const memory::MemoryModuleTypeBase*> getUsedMemories() const override {
        return {
            memory::MemoryModuleTypes::VISIBLE_VILLAGER_BABIES,
            memory::MemoryModuleTypes::NEAREST_VISIBLE_ADULT
        };
    }

protected:
    void update(IWorld* world, E* entity) override;
};

/**
 * @brief 避险传感器
 *
 * 检测需要避险的目标。
 * 参考 MC 1.16.5 AvoidEntitySensor
 *
 * @tparam E 实体类型，需要有 brain() 方法返回 Brain<E>&
 */
template <typename E>
class AvoidEntitySensor : public Sensor<E> {
public:
    explicit AvoidEntitySensor(f32 range = 16.0f, i32 interval = 20)
        : Sensor<E>(interval)
        , m_range(range)
    {}

    [[nodiscard]] std::unordered_set<const memory::MemoryModuleTypeBase*> getUsedMemories() const override {
        return {
            memory::MemoryModuleTypes::AVOID_TARGET,
            memory::MemoryModuleTypes::NEAREST_REPELLENT
        };
    }

protected:
    void update(IWorld* world, E* entity) override;

    /**
     * @brief 判断是否需要躲避某实体
     * @param self 自身实体
     * @param other 其他实体
     * @return 是否需要躲避
     */
    static bool shouldAvoid(E* self, LivingEntity* other);

private:
    f32 m_range;
};

} // namespace sensor
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
