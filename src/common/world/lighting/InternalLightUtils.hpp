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

// 前向声明
class IWorld;
class BlockPos;

/**
 * @brief 内部光照计算工具
 *
 * 提供用于游戏机制的内部光照计算。
 * 内部光照考虑天空减暗、天气等因素。
 *
 * 参考: net.minecraft.world.World (getLight, getLightSubtracted等方法)
 */
namespace InternalLightUtils {

/**
 * @brief 计算天空减暗因子
 *
 * 根据时间和天气计算天空减暗。
 * 天空减暗影响天空光照的有效值。
 *
 * @param dayTime 世界时间 (0-23999)
 * @param isRaining 是否下雨
 * @param isThundering 是否雷暴
 * @return 天空减暗因子 (0-11)
 *
 * 参考: net.minecraft.world.World#calculateSkylightSubtracted
 */
[[nodiscard]] i32 calculateSkyDarkening(i64 dayTime, bool isRaining, bool isThundering);

/**
 * @brief 计算指定时间的默认天空减暗
 *
 * 不考虑天气，仅根据时间计算。
 *
 * @param dayTime 世界时间 (0-23999)
 * @return 天空减暗因子 (0-11)
 */
[[nodiscard]] i32 calculateDefaultSkyDarkening(i64 dayTime);

/**
 * @brief 使用角度计算天空减暗
 *
 * @param celestialAngle 天体角度 (0.0-1.0)
 * @return 天空减暗因子 (0-11)
 */
[[nodiscard]] i32 calculateSkyDarkeningFromAngle(f32 celestialAngle);

/**
 * @brief 计算内部光照等级
 *
 * 取方块光照和天空光照的最大值。
 * 天空光照已经考虑了天空减暗。
 *
 * @param blockLight 方块光照等级 (0-15)
 * @param skyLight 天空光照等级 (0-15)，应该已经减去天空减暗因子
 * @return 内部光照等级 (0-15)
 */
[[nodiscard]] i32 calculateInternalLight(u8 blockLight, u8 skyLight);

/**
 * @brief 计算原始亮度
 *
 * 考虑天空减暗因子的最终亮度值。
 * 这是游戏中用于生物生成、植物生长等的亮度。
 *
 * @param blockLight 方块光照等级 (0-15)
 * @param skyLight 天空光照等级 (0-15，未减暗)
 * @param skyDarkening 天空减暗因子 (0-11)
 * @return 原始亮度 (0-15)
 *
 * 参考: net.minecraft.world.World#getLightSubtracted
 */
[[nodiscard]] i32 calculateRawBrightness(u8 blockLight, u8 skyLight, i32 skyDarkening);

/**
 * @brief 检查位置是否足够黑暗以生成敌对生物
 *
 * @param rawBrightness 原始亮度
 * @return 如果足够黑暗返回true
 */
[[nodiscard]] bool isDarkEnoughForSpawning(i32 rawBrightness);

/**
 * @brief 检查位置是否足够明亮以防止敌对生物生成
 *
 * @param rawBrightness 原始亮度
 * @return 如果足够明亮返回true
 */
[[nodiscard]] bool isBrightEnoughToPreventSpawning(i32 rawBrightness);

/**
 * @brief 检查是否可以睡觉
 *
 * @param rawBrightness 原始亮度
 * @param isThundering 是否雷暴
 * @return 如果可以睡觉返回true
 */
[[nodiscard]] bool canSleep(i32 rawBrightness, bool isThundering);

/**
 * @brief 获取天体角度
 *
 * 将世界时间转换为天体角度。
 * 这是简化的线性映射版本，用于光照计算。
 *
 * @param dayTime 世界时间 (0-23999)
 * @return 天体角度 (0.0-1.0)
 *         0.0 = 日出, 0.25 = 正午, 0.5 = 日落, 0.75 = 午夜
 */
[[nodiscard]] f32 getCelestialAngle(i64 dayTime);

/**
 * @brief 获取 MC 1.16.5 原版天体角度
 *
 * 使用 MC 1.16.5 原版公式计算天体角度。
 * 参考: net.minecraft.world.DimensionType.func_236032_b_()
 *
 * 公式:
 *   d0 = frac(dayTime / 24000.0 - 0.25)
 *   d1 = 0.5 - cos(d0 * π) / 2.0
 *   result = (d0 * 2.0 + d1) / 3.0
 *
 * @param dayTime 世界时间 (0-23999)
 * @return 天体角度 (0.0-1.0)
 *         0.0 = 正午, 0.25 = 日落, 0.5 = 午夜, 0.75 = 日出
 *
 * 关键值:
 *   - dayTime = 0     (日出) → celestialAngle ≈ 0.75
 *   - dayTime = 6000  (正午) → celestialAngle = 0.0
 *   - dayTime = 12000 (日落) → celestialAngle ≈ 0.25
 *   - dayTime = 18000 (午夜) → celestialAngle = 0.5
 *   - dayTime = 22000-22600 (黎明) → celestialAngle ≈ 0.65-0.69
 */
[[nodiscard]] f32 getCelestialAngleMC(i64 dayTime);

/**
 * @brief 获取太阳高度角
 *
 * 正值表示白天，负值表示夜晚。
 *
 * @param celestialAngle 天体角度 (0.0-1.0)
 * @return 太阳高度角 (弧度)
 */
[[nodiscard]] f32 getSunAngle(f32 celestialAngle);

/**
 * @brief 检查是否为白天
 *
 * @param dayTime 世界时间
 * @return 如果是白天返回true
 */
[[nodiscard]] bool isDaytime(i64 dayTime);

/**
 * @brief 检查是否为夜晚
 *
 * @param dayTime 世界时间
 * @return 如果是夜晚返回true
 */
[[nodiscard]] bool isNighttime(i64 dayTime);

/**
 * @brief 月相索引
 *
 * @param dayTime 世界时间
 * @return 月相索引 (0-7)
 */
[[nodiscard]] i32 getMoonPhase(i64 dayTime);

/**
 * @brief 月亮亮度因子
 *
 * @param moonPhase 月相索引 (0-7)
 * @return 月亮亮度因子 (0.0-1.0)
 */
[[nodiscard]] f32 getMoonBrightness(i32 moonPhase);

} // namespace InternalLightUtils

} // namespace mc
