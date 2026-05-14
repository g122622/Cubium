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
 * @brief 光照类型枚举
 *
 * 定义两种光照类型：天空光照和方块光照。
 *
 * 参考: net.minecraft.world.LightType
 *
 * 光照说明:
 * - SKY (天空光照): 从天空传播的自然光，露天位置为15级
 * - BLOCK (方块光照): 光源方块发出的光，如火把、萤石等
 *
 * 光照传播规则:
 * - 方块光照向相邻6个方向传播时衰减1级
 * - 天空光照向下传播时不衰减，向其他方向传播时衰减1级
 * - 透明方块可能阻挡或衰减光照
 */
enum class LightType : u8 {
    /**
     * @brief 天空光照
     *
     * - 露天位置默认为15级
     * - 不随时间变化（日夜通过亮度曲线实现）
     * - 向下传播不衰减，其他方向衰减1级
     * - 某些方块（树叶、水）会使天空光散射（衰减1级后传播）
     */
    SKY = 0,

    /**
     * @brief 方块光照
     *
     * - 光源方块发出（如火把14级、萤石15级）
     * - 向所有6个方向传播时都衰减1级
     * - 传播基于曼哈顿距离
     */
    BLOCK = 1
};

/**
 * @brief 光照常量
 */
namespace LightConstants {
/**
 * @brief 最大光照等级
 */
constexpr u8 MAX_LIGHT = 15;

/**
 * @brief 最小光照等级
 */
constexpr u8 MIN_LIGHT = 0;

/**
 * @brief 天空光照默认值
 */
constexpr u8 SKY_LIGHT_DEFAULT = 15;

/**
 * @brief 方块光照默认值
 */
constexpr u8 BLOCK_LIGHT_DEFAULT = 0;

/**
 * @brief 光照传播衰减值
 */
constexpr u8 PROPAGATION_DECAY = 1;

/**
 * @brief 检查光照等级是否有效
 */
[[nodiscard]] constexpr bool isValidLight(u8 light)
{
    return light <= MAX_LIGHT;
}

/**
 * @brief 获取光照类型的默认值
 */
[[nodiscard]] constexpr u8 getDefaultValue(LightType type)
{
    return type == LightType::SKY ? SKY_LIGHT_DEFAULT : BLOCK_LIGHT_DEFAULT;
}
} // namespace LightConstants

} // namespace mc
