#pragma once

#include "../../../core/Types.hpp"
#include "CaveCarver.hpp"
#include <unordered_set>

namespace mc {

// 方块ID类型别名
using BlockId = u32;

/**
 * @brief 下界洞穴雕刻器
 *
 * 参考 MC 1.16.5 NetherCaveCarver，专门用于下界维度的洞穴生成。
 * 与主世界洞穴的主要区别：
 *
 * - 最大高度 128（下界高度限制）
 * - 可雕刻方块包含下界特有方块（地狱岩、灵魂沙、玄武岩等）
 * - 可雕刻流体包含熔岩
 * - 更少但更大的洞穴（getMaxCaveCount = 10）
 * - 更扁平的洞穴形状（verticalScale = 5.0）
 * - Y <= 31 时使用熔岩填充，Y >= 32 时使用 CAVE_AIR
 *
 * 使用方法：
 * @code
 * NetherCaveCarver carver;
 * CarvingMask mask(chunkX, chunkZ);
 * ProbabilityConfig config(0.2f);
 * carver.carve(chunk, biomeProvider, 31, chunkX, chunkZ, mask, config);
 * @endcode
 *
 * @note 参考 MC 1.16.5 NetherCaveCarver
 */
class NetherCaveCarver : public CaveCarver {
public:
    /**
     * @brief 构造下界洞穴雕刻器
     * 下界最大高度为 128
     */
    NetherCaveCarver();

    ~NetherCaveCarver() override = default;

protected:
    /**
     * @brief 获取最大洞穴生成尝试次数
     * 下界洞穴更少但更大
     * @return 10
     */
    [[nodiscard]] i32 getMaxCaveCount() const override { return 10; }

    /**
     * @brief 获取洞穴半径
     * 下界洞穴半径更大：(nextFloat() * 2.0F + nextFloat()) * 2.0F
     * @param rng 随机数生成器
     * @return 半径
     */
    [[nodiscard]] f32 getCaveRadius(math::IRandom& rng) const override;

    /**
     * @brief 获取垂直缩放因子
     * 下界洞穴更扁平
     * @return 5.0
     */
    [[nodiscard]] f32 getVerticalScale() const override { return 5.0f; }

    /**
     * @brief 获取熔岩填充高度
     * 下界熔岩海高度为 31
     * @return 32（Y <= 31 使用熔岩填充）
     */
    [[nodiscard]] i32 getLavaLevel() const override { return 32; }

    /**
     * @brief 获取洞穴起始Y坐标
     * 下界使用完整高度范围
     * @param rng 随机数生成器
     * @return Y坐标
     */
    [[nodiscard]] i32 getCaveStartY(math::IRandom& rng) const override;

    /**
     * @brief 检查是否可以雕刻该方块
     * 下界版本的方块雕刻检查
     * @param state 当前方块状态
     * @param aboveState 上方方块状态
     * @return 是否可以雕刻
     */
    [[nodiscard]] bool canCarveBlock(const BlockState* state, const BlockState* aboveState) const override;

private:
    /**
     * @brief 检查方块是否可以被雕刻
     * 包含下界特有方块
     * @param state 方块状态
     * @return 是否可雕刻
     */
    [[nodiscard]] static bool isNetherCarvable(const BlockState& state);
};

} // namespace mc
