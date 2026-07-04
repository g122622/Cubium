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
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"

namespace mc::entity {

/**
 * @brief 监守者怒气等级
 *
 * 监守者（Warden）根据当前怒气值（anger）分为三个等级，每个等级对应：
 * - 不同的环境音效（getAmbientSound）
 * - 不同的"倾听"音效（getListeningSound，监守者锁定目标时播放）
 * - 不同的心跳频率（项目尚未实现心跳，预留扩展）
 *
 * 等级阈值（与 MC 1.21.11 AngerLevel 完全一致）：
 * - Calmed:   anger < 40  → WARDEN_AMBIENT          / WARDEN_LISTENING
 * - Agitated: 40 ≤ anger < 80 → WARDEN_AGITATED     / WARDEN_LISTENING_ANGRY
 * - Angry:    anger ≥ 80  → WARDEN_ANGRY            / WARDEN_LISTENING_ANGRY
 *
 * @note 当前实现为简化版：监守者尚未引入完整的 AngerManagement（每实体独立
 *       怒气追踪），WardenEntity 仅维护一个聚合怒气值 m_anger，由
 *       increaseAnger() 累加、tick() 中自然衰减。这与 MC 1.21.11 的多目标
 *       怒气管理在语义上有差异，但足以支持环境音效切换、心跳频率调整等
 *       客户端表现。后续实现 AngerManagement 后可平滑迁移。
 *
 * 参考: net.minecraft.world.entity.monster.warden.AngerLevel
 */
enum class WardenAngerLevel : u8 {
    /// 平静：怒气 < 40
    Calmed = 0,
    /// 激怒：40 ≤ 怒气 < 80
    Agitated = 1,
    /// 愤怒：怒气 ≥ 80（锁定目标进行攻击）
    Angry = 2,
};

/**
 * @brief 各等级对应的最小怒气阈值
 * @param level 怒气等级
 * @return 该等级所需的最小怒气值（Calmed=0, Agitated=40, Angry=80）
 */
[[nodiscard]] inline constexpr i32 wardenAngerLevelMinimumAnger(WardenAngerLevel level) noexcept
{
    switch (level) {
        case WardenAngerLevel::Calmed:
            return 0;
        case WardenAngerLevel::Agitated:
            return 40;
        case WardenAngerLevel::Angry:
            return 80;
    }
    return 0;
}

/**
 * @brief 根据怒气值反查怒气等级
 * @param anger 当前怒气值（≥ 0）
 * @return 对应的怒气等级（anger < 40 → Calmed, < 80 → Agitated, 否则 Angry）
 *
 * 与 MC 1.21.11 AngerLevel.byAnger(int) 行为一致：从高到低匹配，
 * 优先返回满足 anger ≥ minimumAnger 的最高等级。
 */
[[nodiscard]] inline constexpr WardenAngerLevel wardenAngerLevelByAnger(i32 anger) noexcept
{
    if (anger >= wardenAngerLevelMinimumAnger(WardenAngerLevel::Angry)) {
        return WardenAngerLevel::Angry;
    }
    if (anger >= wardenAngerLevelMinimumAnger(WardenAngerLevel::Agitated)) {
        return WardenAngerLevel::Agitated;
    }
    return WardenAngerLevel::Calmed;
}

/**
 * @brief 是否处于愤怒（Angry）等级
 * @param level 怒气等级
 * @return 当 level == Angry 时返回 true
 *
 * 对应 MC 1.21.11 AngerLevel.isAngry()。监守者仅在 Angry 等级才会真正
 * 锁定目标进行主动攻击（项目当前简化：直接通过 targetSelector 触发）。
 */
[[nodiscard]] inline constexpr bool wardenAngerLevelIsAngry(WardenAngerLevel level) noexcept
{
    return level == WardenAngerLevel::Angry;
}

/**
 * @brief 获取环境音效
 * @param level 怒气等级
 * @return 对应的环境音效 ResourceLocation（Calmed→WARDEN_AMBIENT,
 *         Agitated→WARDEN_AGITATED, Angry→WARDEN_ANGRY）
 *
 * 对应 MC 1.21.11 AngerLevel.getAmbientSound()。
 * 调用方（WardenEntity::getAmbientSound）需自行额外判断姿态
 * （ROARING/DIGGING/EMERGING 时返回 std::nullopt）。
 */
[[nodiscard]] inline const ResourceLocation& wardenAngerLevelAmbientSound(WardenAngerLevel level) noexcept
{
    switch (level) {
        case WardenAngerLevel::Calmed:
            return SoundEvents::ENTITY_WARDEN_AMBIENT;
        case WardenAngerLevel::Agitated:
            return SoundEvents::ENTITY_WARDEN_AGITATED;
        case WardenAngerLevel::Angry:
            return SoundEvents::ENTITY_WARDEN_ANGRY;
    }
    return SoundEvents::ENTITY_WARDEN_AMBIENT;
}

/**
 * @brief 获取"倾听"音效（监守者察觉振动/目标时播放）
 * @param level 怒气等级
 * @return 对应的倾听音效（Calmed→WARDEN_LISTENING,
 *         Agitated/Angry→WARDEN_LISTENING_ANGRY）
 *
 * 对应 MC 1.21.11 AngerLevel.getListeningSound()。
 */
[[nodiscard]] inline const ResourceLocation& wardenAngerLevelListeningSound(WardenAngerLevel level) noexcept
{
    switch (level) {
        case WardenAngerLevel::Calmed:
            return SoundEvents::ENTITY_WARDEN_LISTENING;
        case WardenAngerLevel::Agitated:
        case WardenAngerLevel::Angry:
            return SoundEvents::ENTITY_WARDEN_LISTENING_ANGRY;
    }
    return SoundEvents::ENTITY_WARDEN_LISTENING;
}

} // namespace mc::entity
