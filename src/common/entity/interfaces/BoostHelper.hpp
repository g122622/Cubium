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

#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/util/nbt/Nbt.hpp"

namespace mc {

// Forward declarations
class MobEntity; // MobEntity 定义在 mc 命名空间，不是 mc::entity

/**
 * @brief 加速辅助类
 *
 * 管理可骑乘实体的鞍和加速状态。
 * 用于猪、炽足兽等可骑乘实体。
 *
 * 数据同步：
 * - BOOST_TIME_PARAM: 加速时间，由 boost() 设置并同步到客户端
 * - SADDLED_PARAM: 鞍状态，由 setSaddledFromBoolean() 设置
 *
 * 客户端通过 syncFromDataManager() 从 EntityDataManager 读取数据并重置状态。
 */
class BoostHelper {
public:
    /**
     * @brief 默认构造函数
     *
     * 用于延迟初始化场景。必须稍后调用 init() 设置数据管理器。
     */
    BoostHelper() = default;

    /**
     * @brief 构造函数
     * @param manager EntityDataManager 引用
     * @param boostTimeParam 加速时间数据参数
     * @param saddledParam 鞍状态数据参数
     */
    BoostHelper(entity::EntityDataManager& manager,
        entity::DataParameter<i32> boostTimeParam,
        entity::DataParameter<bool> saddledParam)
        : m_manager(&manager)
        , m_boostTimeParam(boostTimeParam)
        , m_saddledParam(saddledParam)
        , m_initialized(true)
    {}

    /**
     * @brief 初始化数据管理器引用
     * @param manager EntityDataManager 引用
     * @param boostTimeParam 加速时间数据参数
     * @param saddledParam 鞍状态数据参数
     */
    void init(entity::EntityDataManager& manager,
        entity::DataParameter<i32> boostTimeParam,
        entity::DataParameter<bool> saddledParam)
    {
        m_manager = &manager;
        m_boostTimeParam = boostTimeParam;
        m_saddledParam = saddledParam;
        m_initialized = true;
    }

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] bool isInitialized() const noexcept { return m_initialized; }

    /**
     * @brief 从数据管理器同步数据
     *
     * 在客户端调用，从 EntityDataManager 读取加速时间并重置状态。
     * 此方法应在实体注册数据参数后调用。
     */
    void syncFromDataManager()
    {
        if (!m_initialized) return;
        saddledRaw = true;
        field_233611_b_ = 0;
        boostTimeRaw = m_manager->get(m_boostTimeParam);
    }

    /**
     * @brief 触发加速
     * @tparam Random 随机数生成器类型
     * @param rng 随机数生成器
     * @return 如果成功加速返回true
     */
    template <typename Random>
    bool boost(Random& rng)
    {
        if (saddledRaw) {
            return false;
        }

        saddledRaw = true;
        field_233611_b_ = 0;
        // 随机生成加速时间 [140, 980]
        boostTimeRaw = rng.nextInt(841) + 140;
        if (m_initialized) {
            m_manager->set(m_boostTimeParam, boostTimeRaw);
        }
        return true;
    }

    /**
     * @brief 写入NBT
     *
     * 只保存鞍状态，加速时间不持久化（每次加载后需要重新触发加速）。
     * NBT格式没有布尔类型，使用Byte存储布尔值。
     *
     * @param tag NBT复合标签
     */
    void writeToNbt(nbt::tags::compound_tag& tag) const
    {
        // NBT 没有布尔类型，使用 i8 (Byte) 存储
        tag.put("Saddle", static_cast<i8>(getSaddled() ? 1 : 0));
    }

    /**
     * @brief 从NBT读取
     *
     * 只读取鞍状态，加速状态在加载后重置为未加速。
     *
     * @param tag NBT复合标签
     */
    void readFromNbt(const nbt::tags::compound_tag& tag)
    {
        auto it = tag.value.find("Saddle");
        if (it != tag.value.end()) {
            // NBT 没有布尔类型，从 Byte 读取
            if (it->second->id() == nbt::TagId::Byte) {
                i8 value = dynamic_cast<const nbt::tags::byte_tag&>(*it->second).value;
                setSaddledFromBoolean(value != 0);
            }
        }
    }

    /**
     * @brief 从布尔值设置鞍状态
     *
     * 通过 EntityDataManager 同步鞍状态到客户端。
     *
     * @param saddled 是否有鞍
     */
    void setSaddledFromBoolean(bool saddled)
    {
        if (m_initialized) {
            m_manager->set(m_saddledParam, saddled);
        }
    }

    /**
     * @brief 获取鞍状态
     * @return 是否有鞍
     *
     * 从 EntityDataManager 读取鞍状态。
     */
    [[nodiscard]] bool getSaddled() const
    {
        if (m_initialized) {
            return m_manager->get(m_saddledParam);
        }
        return false;
    }

    /**
     * @brief 设置加速时间
     * @param time 加速时间
     */
    void setBoostTime(i32 time)
    {
        if (m_initialized) {
            m_manager->set(m_boostTimeParam, time);
        }
    }

    /**
     * @brief 获取加速时间
     * @return 加速时间（ticks）
     */
    [[nodiscard]] i32 getBoostTime() const
    {
        if (m_initialized) {
            return m_manager->get(m_boostTimeParam);
        }
        return 0;
    }

    /**
     * @brief 是否正在加速
     * @return 是否正在加速
     *
     * 加速期间：field_233611_b_ 从 0 递增到 boostTimeRaw（包含边界值）
     * 当 field_233611_b_ > boostTimeRaw 时，加速结束
     */
    [[nodiscard]] bool isBoosting() const noexcept { return saddledRaw && field_233611_b_ <= boostTimeRaw; }

    /**
     * @brief Tick更新
     *
     * 每tick调用以更新加速状态。
     * @return 是否需要继续加速
     */
    bool tick() noexcept
    {
        if (saddledRaw) {
            field_233611_b_++;
            if (field_233611_b_ > boostTimeRaw) {
                saddledRaw = false;
                return false;
            }
            return true;
        }
        return false;
    }

    // 公开成员
    bool saddledRaw = false; ///< 原始鞍状态（加速中时为true）
    i32 field_233611_b_ = 0; ///< 当前加速tick
    i32 boostTimeRaw = 0;    ///< 原始加速时间

private:
    entity::EntityDataManager* m_manager = nullptr;
    entity::DataParameter<i32> m_boostTimeParam{0};
    entity::DataParameter<bool> m_saddledParam{0};
    bool m_initialized = false;
};

} // namespace mc
