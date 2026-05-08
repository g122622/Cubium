#pragma once

#include "../memory/MemoryModuleType.hpp"
#include "../../../../util/math/random/Random.hpp"
#include <unordered_set>

namespace mc {

// Forward declarations
class LivingEntity;
class IWorld;

namespace entity {
namespace ai {
namespace brain {
namespace sensor {

/**
 * @brief 传感器基类
 *
 * 传感器负责定期更新实体的记忆模块
 * 参考 MC 1.16.5 Sensor
 *
 * @tparam E 实体类型
 */
template <typename E>
class Sensor {
public:
    /**
     * @brief 构造传感器
     * @param interval 更新间隔(ticks)
     */
    explicit Sensor(i32 interval = 20)
        : m_interval(interval), m_counter(0) {}

    virtual ~Sensor() = default;

    /**
     * @brief 每tick调用
     * @param world 世界
     * @param entity 实体
     */
    void tick(IWorld* world, E* entity) {
        if (--m_counter <= 0) {
            m_counter = m_interval;
            update(world, entity);
        }
    }

    /**
     * @brief 初始化counter（在首次tick时调用）
     * MC 1.16.5: 使用实体随机数生成器初始化counter
     * @param random 实体的随机数生成器
     */
    void initCounter(math::Random& random) {
        if (m_counter < 0) {
            m_counter = random.nextInt(m_interval);
        }
    }

    /**
     * @brief 获取此传感器使用的内存模块类型
     */
    [[nodiscard]] virtual std::unordered_set<const memory::MemoryModuleTypeBase*> getUsedMemories() const = 0;

protected:
    /**
     * @brief 更新传感器(子类实现)
     * @param world 世界
     * @param entity 实体
     */
    virtual void update(IWorld* world, E* entity) = 0;

    i32 m_interval;
    i32 m_counter = -1;  // MC 1.16.5: -1表示未初始化，初始化时随机设置
};

} // namespace sensor
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
