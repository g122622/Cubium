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
 * LIABILITY, WHETHER IN AN EVENT OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include "FluidStatus.hpp"
#include "common/core/Types.hpp"
#include <memory>

namespace mc {

// 前向声明
class BlockState;

namespace world::gen::density {
class NoiseRouter;
class NoiseChunk;
} // namespace world::gen::density

namespace math {
class PositionalRandomFactory;
} // namespace math

namespace world::gen::aquifer {

/**
 * @brief 含水层采样器（MC 1.21 Aquifer）
 *
 * 在噪声地形生成过程中，确定每个方块位置是否应该被流体替代。
 * 当 finalDensity < 0（空腔）时，含水层系统决定空腔内填充水、熔岩还是空气。
 *
 * 主世界使用 NoiseBasedAquifer 实现基于噪声的含水层分布；
 * 下界/末地使用禁用含水层的空实现。
 */
class Aquifer {
public:
    virtual ~Aquifer() = default;

    /**
     * @brief 计算指定位置的流体/方块
     *
     * 在 finalDensity < 0 时调用，确定该位置应该填充什么。
     *
     * @param blockX 方块 X 坐标
     * @param blockY 方块 Y 坐标
     * @param blockZ 方块 Z 坐标
     * @param densityValue 当前方块处的 finalDensity 值
     * @return 方块状态指针，nullptr 表示保持默认（石头）
     */
    [[nodiscard]] virtual const BlockState* computeSubstance(i32 blockX, i32 blockY, i32 blockZ, f64 densityValue) = 0;

    /**
     * @brief 是否应安排流体更新
     *
     * 在 computeSubstance 之后调用。当含水层边界处流体类型变化时，
     * 需要安排流体更新以触发流动。
     */
    [[nodiscard]] virtual bool shouldScheduleFluidUpdate() const = 0;

    // ========== 工厂方法 ==========

    /**
     * @brief 创建基于噪声的含水层采样器
     *
     * @param noiseChunk NoiseChunk 引用
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @param router 噪声路由器
     * @param positionalRandom 位置随机工厂
     * @param minY 世界最低 Y
     * @param height 世界高度
     * @param globalFluidPicker 全局流体选择器
     * @return 含水层采样器实例
     */
    [[nodiscard]] static std::unique_ptr<Aquifer> createNoiseBased(const density::NoiseChunk& noiseChunk,
        i32 chunkX,
        i32 chunkZ,
        const density::NoiseRouter& router,
        const math::PositionalRandomFactory& positionalRandom,
        i32 minY,
        i32 height,
        FluidPicker globalFluidPicker);

    /**
     * @brief 创建禁用含水层的空实现
     *
     * 在 finalDensity < 0 时直接返回全局流体。
     *
     * @param globalFluidPicker 全局流体选择器
     * @return 禁用含水层的空实现
     */
    [[nodiscard]] static std::unique_ptr<Aquifer> createDisabled(FluidPicker globalFluidPicker);
};

} // namespace world::gen::aquifer
} // namespace mc
