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
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/util/math/random/PositionalRandomFactory.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/density/NoiseChunk.hpp"

namespace mc::world::gen::density {

class DensityFunction;

/**
 * @brief 矿脉填充器 — MC 1.21.11 OreVeinifier
 *
 * 使用 veinToggle/veinRidged/veinGap 三个密度函数决定矿脉位置和矿石类型。
 * 铜矿脉出现在 Y=0~50，铁矿脉出现在 Y=-60~-8。
 *
 * 算法流程：
 * 1. veinToggle > 0 → Copper，否则 → Iron
 * 2. 检查 Y 范围约束
 * 3. 计算边缘衰减（接近 Y 边界时变薄）
 * 4. 随机跳过（70% 通过率）
 * 5. veinRidged >= 0 时跳过（不在矿脉脊线内）
 * 6. richness 概率 + gap noise 过滤 → 矿石或填充物
 */
class OreVeinifier final : public BlockStateFiller {
public:
    /**
     * @brief 构造矿脉填充器
     * @param veinToggle 矿脉切换密度函数（正=铜，负=铁）
     * @param veinRidged 矿脉脊线密度函数
     * @param veinGap 矿脉间隙密度函数
     * @param randomFactory 位置化随机工厂
     */
    OreVeinifier(const DensityFunction& veinToggle,
        const DensityFunction& veinRidged,
        const DensityFunction& veinGap,
        const math::PositionalRandomFactory& randomFactory);

    [[nodiscard]] const BlockState* calculate(i32 blockX, i32 blockY, i32 blockZ, f64 density) override;

private:
    const DensityFunction& m_veinToggle;
    const DensityFunction& m_veinRidged;
    const DensityFunction& m_veinGap;
    const math::PositionalRandomFactory& m_randomFactory;

    // 缓存的方块状态指针（懒初始化）
    const BlockState* m_copperOre = nullptr;
    const BlockState* m_deepslateCopperOre = nullptr;
    const BlockState* m_rawCopperBlock = nullptr;
    const BlockState* m_granite = nullptr;
    const BlockState* m_ironOre = nullptr;
    const BlockState* m_deepslateIronOre = nullptr;
    const BlockState* m_rawIronBlock = nullptr;
    const BlockState* m_tuff = nullptr;

    bool m_initialized = false;

    /**
     * @brief 懒初始化方块状态指针
     *
     * 在首次 calculate() 调用时从 BlockRegistry 查找方块状态。
     * 不能在构造函数中查找，因为 VanillaBlocks 可能尚未初始化。
     */
    void ensureInitialized();
};

} // namespace mc::world::gen::density
