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

#include "../../../../util/math/random/Random.hpp"
#include "../memory/MemoryModuleType.hpp"
#include "common/core/Types.hpp"
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
        : m_interval(interval)
    {}

    virtual ~Sensor() = default;

    /**
     * @brief 每tick调用
     * @param world 世界
     * @param entity 实体
     */
    void tick(IWorld* world, E* entity)
    {
        if (--m_counter <= 0) {
            m_counter = m_interval;
            update(world, entity);
        }
    }

    /**
     * @brief 初始化计数器（在首次tick时调用）
     *
     * 使用实体随机数生成器初始化计数器，使传感器更新分散在不同tick执行
     *
     * @param random 实体的随机数生成器
     */
    void initCounter(math::Random& random)
    {
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
    i32 m_counter = -1; // -1表示未初始化，初始化时随机设置
};

} // namespace sensor
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
