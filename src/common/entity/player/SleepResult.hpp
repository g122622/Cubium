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
namespace entity {

/**
 * @brief 睡眠尝试结果枚举
 *
 * 参考 MC 1.16.5 PlayerEntity.SleepResult
 * 定义玩家尝试睡眠时可能返回的各种结果。
 */
enum class SleepResult : u8 {
    OK,                // 成功入睡
    NOT_POSSIBLE_HERE, // 此维度不能睡眠（下界/末地）
    NOT_POSSIBLE_NOW,  // 现在不是睡眠时间（非夜晚）
    TOO_FAR_AWAY,      // 离床太远
    OBSTRUCTED,        // 床被阻挡（上方没有空间）
    OTHER_PROBLEM,     // 其他问题（如已经在睡眠中）
    NOT_SAFE           // 周围有怪物
};

/**
 * @brief 获取睡眠失败的翻译键
 *
 * 返回对应于睡眠结果的 Minecraft 翻译键。
 * 用于在客户端显示错误消息。
 *
 * @param result 睡眠结果
 * @return 翻译键字符串（如 "block.minecraft.bed.no_sleep"）
 */
[[nodiscard]] inline const char* getSleepResultMessage(SleepResult result)
{
    switch (result) {
        case SleepResult::NOT_POSSIBLE_NOW:
            return "block.minecraft.bed.no_sleep";
        case SleepResult::TOO_FAR_AWAY:
            return "block.minecraft.bed.too_far_away";
        case SleepResult::OBSTRUCTED:
            return "block.minecraft.bed.obstructed";
        case SleepResult::NOT_SAFE:
            return "block.minecraft.bed.not_safe";
        case SleepResult::NOT_POSSIBLE_HERE:
        case SleepResult::OTHER_PROBLEM:
        default:
            // NOT_POSSIBLE_HERE 用于下界/末地爆炸场景，不显示消息
            // OTHER_PROBLEM 不显示消息
            return nullptr;
    }
}

/**
 * @brief 检查睡眠结果是否表示成功
 * @param result 睡眠结果
 * @return true 如果成功入睡
 */
[[nodiscard]] inline bool isSleepSuccess(SleepResult result)
{
    return result == SleepResult::OK;
}

} // namespace entity
} // namespace mc
