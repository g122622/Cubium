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

#include "Sensor.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/brain/memory/MemoryModuleType.hpp"
#include "common/entity/core/AgeableEntity.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/EntityUtils.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/interfaces/IMob.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include <algorithm>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace mc {
class IWorld;

namespace world {
namespace village {
class VillageManager;
namespace poi {
class PointOfInterestStorage;
enum class PointOfInterestType : u16;
} // namespace poi
} // namespace village
class GlobalPos;
} // namespace world
} // namespace mc

namespace mc {
namespace entity {
namespace ai {
namespace brain {
namespace sensor {

/**
 * @brief 最近玩家传感器
 *
 * 检测附近的玩家并存储到记忆模块。
 *
 * @tparam E 实体类型，需要有 brain() 方法返回 Brain<E>&
 */
template <typename E>
class NearestPlayersSensor : public Sensor<E> {
public:
    NearestPlayersSensor()
        : Sensor<E>(20) // 每20tick更新一次
    {}

    [[nodiscard]] std::unordered_set<const memory::MemoryModuleTypeBase*> getUsedMemories() const override
    {
        return {memory::MemoryModuleTypes::NEAREST_PLAYERS,
            memory::MemoryModuleTypes::NEAREST_VISIBLE_PLAYER,
            memory::MemoryModuleTypes::NEAREST_VISIBLE_TARGETABLE_PLAYER};
    }

protected:
    void update(IWorld* world, E* entity) override;
};

/**
 * @brief 最近可见生物传感器
 *
 * 检测附近可见的生物并存储到记忆模块。
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

    [[nodiscard]] std::unordered_set<const memory::MemoryModuleTypeBase*> getUsedMemories() const override
    {
        return {memory::MemoryModuleTypes::VISIBLE_MOBS, memory::MemoryModuleTypes::NEAREST_VISIBLE_NEMESIS};
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
 *
 * @tparam E 实体类型，需要有 brain() 方法返回 Brain<E>&
 */
template <typename E>
class HurtBySensor : public Sensor<E> {
public:
    HurtBySensor()
        : Sensor<E>(1) // 每 tick 检查（伤害需要快速响应）
    {}

    [[nodiscard]] std::unordered_set<const memory::MemoryModuleTypeBase*> getUsedMemories() const override
    {
        return {memory::MemoryModuleTypes::HURT_BY, memory::MemoryModuleTypes::HURT_BY_ENTITY};
    }

protected:
    void update(IWorld* world, E* entity) override;
};

/**
 * @brief 附近实体传感器
 *
 * 检测附近的所有实体并分类存储。
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

    [[nodiscard]] std::unordered_set<const memory::MemoryModuleTypeBase*> getUsedMemories() const override
    {
        return {memory::MemoryModuleTypes::MOBS};
    }

protected:
    void update(IWorld* world, E* entity) override;

private:
    f32 m_range;
};

/**
 * @brief 村民敌对生物传感器
 *
 * 参考原版 VillagerHostilesSensor 实现，使用精确的实体类型到检测距离映射，
 * 而非将所有 MobEntity 视为敌对。每种敌对生物有独立的检测距离阈值。
 *
 * 村民会检测并逃离以下实体（距离单位：方块）：
 * - 溺尸(8)、唤魔者(12)、尸壳(8)、幻术师(12)、掠夺者(15)、劫掠兽(12)
 * - 恼鬼(8)、卫道士(10)、疣猪兽(10)、僵尸(8)、僵尸村民(8)
 *
 * @tparam E 实体类型，需要有 brain() 方法返回 Brain<E>&
 */
template <typename E>
class VillagerHostilesSensor : public Sensor<E> {
public:
    VillagerHostilesSensor()
        : Sensor<E>(20)
    {}

    [[nodiscard]] std::unordered_set<const memory::MemoryModuleTypeBase*> getUsedMemories() const override
    {
        return {memory::MemoryModuleTypes::MOBS, memory::MemoryModuleTypes::NEAREST_HOSTILE};
    }

protected:
    void update(IWorld* world, E* entity) override;

private:
    /**
     * @brief 判断实体是否为村民需要逃离的敌对生物
     * @param entity 待检查的实体
     * @return 如果是村民需要逃离的敌对生物，返回其检测距离；否则返回 0
     */
    static f32 getHostileDetectionRange(const LivingEntity* entity);

    /**
     * @brief 初始化村民敌对生物距离映射表
     */
    static std::unordered_map<const entity::EntityType*, f32> createHostileDistanceMap();
};

/**
 * @brief 工作站点传感器
 *
 * 检测村民的工作站点。
 *
 * @tparam E 实体类型，需要有 brain() 方法返回 Brain<E>&
 */
template <typename E>
class WorkStationSensor : public Sensor<E> {
public:
    WorkStationSensor()
        : Sensor<E>(40) // 每2秒更新一次
    {}

    [[nodiscard]] std::unordered_set<const memory::MemoryModuleTypeBase*> getUsedMemories() const override
    {
        return {memory::MemoryModuleTypes::JOB_SITE, memory::MemoryModuleTypes::POTENTIAL_JOB_SITE};
    }

protected:
    void update(IWorld* world, E* entity) override;
};

/**
 * @brief 居住点传感器
 *
 * 检测村民的家和集会点。
 *
 * @tparam E 实体类型，需要有 brain() 方法返回 Brain<E>&
 */
template <typename E>
class VillagePoiSensor : public Sensor<E> {
public:
    VillagePoiSensor()
        : Sensor<E>(40) // 每2秒更新一次
    {}

    [[nodiscard]] std::unordered_set<const memory::MemoryModuleTypeBase*> getUsedMemories() const override
    {
        return {memory::MemoryModuleTypes::HOME,
            memory::MemoryModuleTypes::MEETING_POINT,
            memory::MemoryModuleTypes::NEAREST_BED};
    }

protected:
    void update(IWorld* world, E* entity) override;
};

/**
 * @brief 幼崽传感器
 *
 * 检测附近的幼年和成年实体。
 *
 * @tparam E 实体类型，需要有 brain() 方法返回 Brain<E>&
 */
template <typename E>
class BabySensor : public Sensor<E> {
public:
    BabySensor()
        : Sensor<E>(20)
    {}

    [[nodiscard]] std::unordered_set<const memory::MemoryModuleTypeBase*> getUsedMemories() const override
    {
        return {memory::MemoryModuleTypes::VISIBLE_VILLAGER_BABIES, memory::MemoryModuleTypes::NEAREST_VISIBLE_ADULT};
    }

protected:
    void update(IWorld* world, E* entity) override;
};

/**
 * @brief 避险传感器
 *
 * 检测需要避险的目标。默认实现使用 IMob 标记接口判断敌对生物，
 * 即所有继承 MonsterEntity 的实体都被视为需要避险的目标。
 * 玩家在创造/旁观模式下不会触发避险。
 *
 * 特定实体的避险逻辑应通过专门的传感器实现（如 VillagerHostilesSensor）
 * 或通过 AvoidEntityGoal 配合谓词来实现，而不是修改此通用传感器。
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

    [[nodiscard]] std::unordered_set<const memory::MemoryModuleTypeBase*> getUsedMemories() const override
    {
        return {memory::MemoryModuleTypes::AVOID_TARGET, memory::MemoryModuleTypes::NEAREST_REPELLENT};
    }

    /**
     * @brief 判断是否需要躲避某实体
     *
     * 默认实现使用 IMob 标记接口判断敌对生物。
     * 所有继承 MonsterEntity 的实体（即实现了 IMob 接口）都被视为需要避险。
     * 玩家在创造/旁观模式下不触发避险。
     *
     * @param self 自身实体
     * @param other 其他实体
     * @return 是否需要躲避
     */
    static bool shouldAvoid(E* self, LivingEntity* other);

protected:
    void update(IWorld* world, E* entity) override;

private:
    f32 m_range;
};

/**
 * @brief 诱惑玩家传感器
 *
 * 检测手持可诱惑物品的最近玩家，写入 TEMPTING_PLAYER 记忆。
 * 用于 TemptTask 等任务，使动物被手持食物的玩家吸引。
 *
 * @tparam E 实体类型，需要有 brain() 方法返回 Brain<E>&
 */
template <typename E>
class TemptingPlayerSensor : public Sensor<E> {
public:
    using ItemPredicate = std::function<bool(const ItemStack&)>;

    explicit TemptingPlayerSensor(ItemPredicate itemPredicate, f32 range = 10.0f, i32 interval = 20)
        : Sensor<E>(interval)
        , m_itemPredicate(std::move(itemPredicate))
        , m_range(range)
    {}

    [[nodiscard]] std::unordered_set<const memory::MemoryModuleTypeBase*> getUsedMemories() const override
    {
        return {memory::MemoryModuleTypes::TEMPTING_PLAYER};
    }

protected:
    void update(IWorld* world, E* entity) override;

private:
    ItemPredicate m_itemPredicate;
    f32 m_range;
};

/**
 * @brief 可交互门传感器
 *
 * 扫描实体附近路径上的木门，写入 INTERACTABLE_DOORS 记忆。
 * 用于 InteractWithDoorTask 等任务，使实体能够自动开关门。
 *
 * @tparam E 实体类型，需要有 brain() 方法返回 Brain<E>&
 */
template <typename E>
class InteractableDoorsSensor : public Sensor<E> {
public:
    explicit InteractableDoorsSensor(f32 range = 4.0f, i32 interval = 20)
        : Sensor<E>(interval)
        , m_range(range)
    {}

    [[nodiscard]] std::unordered_set<const memory::MemoryModuleTypeBase*> getUsedMemories() const override
    {
        return {memory::MemoryModuleTypes::INTERACTABLE_DOORS, memory::MemoryModuleTypes::OPENED_DOORS};
    }

protected:
    void update(IWorld* world, E* entity) override;

private:
    f32 m_range;
};

/**
 * @brief 主人受伤传感器
 *
 * 检测驯服动物的主人是否受到攻击，写入 OWNER_HURT_BY 记忆。
 * 用于 ProtectOwnerTask 等任务，使宠物在主人被攻击时保护主人。
 *
 * 仅适用于 TameableEntity 子类。如果实体未驯服或主人不存在，
 * 传感器将清除 OWNER_HURT_BY 记忆。
 *
 * @tparam E 实体类型，需要有 brain() 方法返回 Brain<E>&
 */
template <typename E>
class OwnerHurtBySensor : public Sensor<E> {
public:
    OwnerHurtBySensor()
        : Sensor<E>(1) // 每 tick 检查（需要快速响应主人被攻击事件）
    {}

    [[nodiscard]] std::unordered_set<const memory::MemoryModuleTypeBase*> getUsedMemories() const override
    {
        return {memory::MemoryModuleTypes::OWNER_HURT_BY};
    }

protected:
    void update(IWorld* world, E* entity) override;
};

} // namespace sensor
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
