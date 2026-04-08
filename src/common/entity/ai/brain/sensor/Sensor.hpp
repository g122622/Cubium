#pragma once

#include "../memory/MemoryModuleType.hpp"
#include <unordered_set>
#include <random>

namespace mc {

// Forward declarations
class LivingEntity;

namespace server {
class ServerWorld;
}

namespace entity {
namespace ai {
namespace brain {
namespace sensor {

// 使用完整命名空间
using ::mc::server::ServerWorld;

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
        : m_interval(interval), m_counter(static_cast<i32>(randomInt(interval))) {}

    virtual ~Sensor() = default;

    /**
     * @brief 每tick调用
     * @param world 世界
     * @param entity 实体
     */
    void tick(ServerWorld* world, E* entity) {
        if (--m_counter <= 0) {
            m_counter = m_interval;
            update(world, entity);
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
    virtual void update(ServerWorld* world, E* entity) = 0;

    i32 m_interval;
    i32 m_counter;

private:
    static i32 randomInt(i32 bound) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<i32> dist(0, bound - 1);
        return dist(gen);
    }
};

} // namespace sensor
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
