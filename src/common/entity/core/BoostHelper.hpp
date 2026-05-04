#pragma once

#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"

namespace mc {

// Forward declarations
class MobEntity;  // MobEntity 定义在 mc 命名空间，不是 mc::entity

/**
 * @brief 加速辅助类
 *
 * 管理可骑乘实体的鞍和加速状态。
 * 用于猪、炽足兽等可骑乘实体。
 *
 * 参考 MC 1.16.5 BoostHelper
 */
class BoostHelper {
public:
    /**
     * @brief 构造函数
     */
    BoostHelper() = default;

    /**
     * @brief 重置状态
     *
     * 当加速时间数据同步时调用。
     * MC 1.16.5: func_233616_a_()
     */
    void reset() {
        saddledRaw = true;
        boostingTick = 0;
        boostTimeRaw = m_boostTime;
    }

    /**
     * @brief 触发加速
     *
     * MC 1.16.5: boost(Random)
     * @param rng 随机数生成器
     * @return 如果成功加速返回true
     */
    bool boost(math::Random& rng) {
        if (saddledRaw) {
            return false;
        }

        saddledRaw = true;
        boostingTick = 0;
        // MC 1.16.5: rand.nextInt(841) + 140 -> [140, 980]
        boostTimeRaw = rng.nextInt(140, 980);
        m_boostTime = boostTimeRaw;
        return true;
    }

    /**
     * @brief 写入NBT
     *
     * MC 1.16.5: func_233618_a_()
     * TODO: 待NBT系统完善后实现
     */
    void writeToNBT(/* CompoundNBT& nbt */) const {
        // nbt.putBoolean("Saddle", m_saddled);
    }

    /**
     * @brief 从NBT读取
     *
     * MC 1.16.5: func_233621_b_()
     * TODO: 待NBT系统完善后实现
     */
    void readFromNBT(/* const CompoundNBT& nbt */) const {
        // setSaddled(nbt.getBoolean("Saddle"));
    }

    /**
     * @brief 设置鞍状态
     * @param saddled 是否有鞍
     */
    void setSaddled(bool saddled) {
        m_saddled = saddled;
    }

    /**
     * @brief 获取鞍状态
     * @return 是否有鞍
     */
    [[nodiscard]] bool getSaddled() const {
        return m_saddled;
    }

    /**
     * @brief 获取加速时间
     * @return 加速时间（ticks）
     */
    [[nodiscard]] i32 getBoostTime() const {
        return m_boostTime;
    }

    /**
     * @brief 设置加速时间
     * @param time 加速时间
     */
    void setBoostTime(i32 time) {
        m_boostTime = time;
        boostTimeRaw = time;
    }

    /**
     * @brief 是否正在加速
     * @return 是否正在加速
     */
    [[nodiscard]] bool isBoosting() const {
        return saddledRaw && boostingTick < boostTimeRaw;
    }

    /**
     * @brief 获取加速进度
     * @return 加速进度 (0.0 - 1.0)
     */
    [[nodiscard]] f32 getBoostProgress() const {
        if (boostTimeRaw <= 0) {
            return 0.0f;
        }
        return static_cast<f32>(boostingTick) / static_cast<f32>(boostTimeRaw);
    }

    /**
     * @brief Tick更新
     *
     * 每tick调用以更新加速状态。
     * @return 是否需要继续加速
     */
    bool tick() {
        if (saddledRaw) {
            boostingTick++;
            if (boostingTick > boostTimeRaw) {
                saddledRaw = false;
                return false;
            }
            return true;
        }
        return false;
    }

    // 公开成员（与MC保持一致）
    bool saddledRaw = false;     ///< 原始鞍状态（加速中时为true）
    i32 boostingTick = 0;        ///< 当前加速tick
    i32 boostTimeRaw = 0;        ///< 原始加速时间

private:
    bool m_saddled = false;       ///< 是否装备鞍
    i32 m_boostTime = 0;          ///< 加速时间
};

} // namespace mc
