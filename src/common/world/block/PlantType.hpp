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

#include "../../core/Types.hpp"

namespace mc {

/**
 * @brief 植物类型枚举
 *
 * 用于区分不同类型植物的土壤需求。
 * 植物通过 IPlantable 接口返回此类型，
 * 方块通过 canSustainPlant() 方法检查是否支持该类型。
 *
 * 参考: net.minecraftforge.common.PlantType
 */
enum class PlantType : u8 {
    /// 平原植物（大多数花草、树苗等）
    Plains,

    /// 沙漠植物（仙人掌）
    Desert,

    /// 海滩植物（甘蔗）
    Beach,

    /// 洞穴植物（蘑菇）
    Cave,

    /// 水生植物（睡莲、海草等）
    Water,

    /// 下界植物（地狱疣、菌类等）
    Nether,

    /// 农作物（小麦、胡萝卜等）
    Crop
};

/**
 * @brief 植物接口
 *
 * 实现此接口的方块被视为植物。
 * 植物可以报告其类型，并允许土壤方块检查是否支持该植物。
 *
 * 参考: net.minecraftforge.common.IPlantable
 */
class IPlantable {
public:
    virtual ~IPlantable() = default;

    /**
     * @brief 获取植物类型
     *
     * @param world 世界读取器
     * @param pos 植物位置
     * @return 植物类型
     */
    [[nodiscard]] virtual PlantType getPlantType(IBlockReader& world, const BlockPos& pos) const = 0;

    /**
     * @brief 获取植物方块状态
     *
     * 返回植物在此位置应该具有的状态。
     * 用于土壤方块检查支撑的植物类型。
     *
     * @param world 世界读取器
     * @param pos 植物位置
     * @return 植物方块状态
     */
    [[nodiscard]] virtual const BlockState& getPlant(IBlockReader& world, const BlockPos& pos) const = 0;
};

} // namespace mc
