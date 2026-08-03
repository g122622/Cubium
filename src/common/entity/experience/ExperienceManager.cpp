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

#include "ExperienceManager.hpp"
#include "ExperienceConstants.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/random/Random.hpp"
#include <algorithm>

namespace mc {
namespace entity {
namespace experience {

ExperienceManager::ExperienceManager(Player& player)
    : m_player(player)
{}

void ExperienceManager::addExperience(i32 amount)
{
    if (amount == 0) {
        return;
    }

    if (amount < 0) {
        i32 capacity = getExperienceForNextLevel();
        m_progress += static_cast<f32>(amount) / static_cast<f32>(capacity);

        while (m_progress < 0.0f) {
            if (m_level > 0) {
                m_level--;
                i32 newCapacity = getExperienceForNextLevel();
                f32 deficit = m_progress * static_cast<f32>(newCapacity);
                m_progress = 1.0f + deficit / static_cast<f32>(newCapacity);
                _handleLevelDown();
            } else {
                m_progress = 0.0f;
                m_totalExperience = 0;
                break;
            }
        }

        capacity = getExperienceForNextLevel();
        m_totalExperience = getExperienceForLevel(m_level) + static_cast<i32>(m_progress * static_cast<f32>(capacity));
    } else {
        m_totalExperience += amount;

        i32 capacity = getExperienceForNextLevel();
        m_progress += static_cast<f32>(amount) / static_cast<f32>(capacity);

        while (m_progress >= 1.0f) {
            f32 excessProgress = m_progress - 1.0f;
            i32 oldCapacity = getExperienceForNextLevel();

            m_level++;
            _handleLevelUp();

            i32 newCapacity = getExperienceForNextLevel();
            m_progress = excessProgress * static_cast<f32>(oldCapacity) / static_cast<f32>(newCapacity);
        }
    }

    _validateState();
    markDirty();

    if (m_experienceChangeCallback) {
        m_experienceChangeCallback(m_totalExperience);
    }
}

bool ExperienceManager::consumeExperience(i32 amount)
{
    if (amount <= 0) {
        return true;
    }

    if (m_totalExperience < amount) {
        return false;
    }

    m_totalExperience -= amount;

    i32 newLevel = getLevelFromExperience(m_totalExperience);
    i32 xpForNewLevel = getExperienceForLevel(newLevel);
    i32 xpInCurrentLevel = m_totalExperience - xpForNewLevel;
    i32 capacity = calculateBarCapacity(newLevel);

    m_level = newLevel;
    m_progress = capacity > 0 ? static_cast<f32>(xpInCurrentLevel) / static_cast<f32>(capacity) : 0.0f;

    _validateState();
    markDirty();

    if (m_experienceChangeCallback) {
        m_experienceChangeCallback(m_totalExperience);
    }

    return true;
}

bool ExperienceManager::consumeLevels(i32 levels)
{
    if (levels <= 0) {
        return true;
    }

    if (m_level < levels) {
        return false;
    }

    i32 oldLevel = m_level;
    m_level -= levels;

    i32 capacity = calculateBarCapacity(m_level);
    i32 xpInCurrentLevel = static_cast<i32>(m_progress * static_cast<f32>(capacity));
    m_totalExperience = getExperienceForLevel(m_level) + xpInCurrentLevel;

    _validateState();
    markDirty();

    if (m_levelChangeCallback) {
        m_levelChangeCallback(oldLevel, m_level);
    }

    return true;
}

void ExperienceManager::setExperience(i32 level, f32 progress, i32 totalExperience)
{
    m_level = std::max(0, level);
    m_progress = std::clamp(progress, 0.0f, 1.0f);
    m_totalExperience = std::max(0, totalExperience);

    _validateState();
    markDirty();

    if (m_experienceChangeCallback) {
        m_experienceChangeCallback(m_totalExperience);
    }
}

void ExperienceManager::setLevel(i32 level)
{
    i32 oldLevel = m_level;
    m_level = std::max(0, level);

    // 重新计算总经验
    i32 xpInCurrentLevel = static_cast<i32>(m_progress * static_cast<f32>(getExperienceForNextLevel()));
    m_totalExperience = getExperienceForLevel(m_level) + xpInCurrentLevel;

    _validateState();
    markDirty();

    if (m_levelChangeCallback && oldLevel != m_level) {
        m_levelChangeCallback(oldLevel, m_level);
    }
}

void ExperienceManager::addLevels(i32 levels)
{
    if (levels == 0) {
        return;
    }

    i32 oldLevel = m_level;

    m_level += levels;

    if (m_level < 0) {
        // 负等级时重置所有经验
        m_level = 0;
        m_progress = 0.0f;
        m_totalExperience = 0;
    } else {
        // 正等级时不重新计算 totalExperience
        // totalExperience 只在需要时才重新计算
    }

    // 更新 totalExperience 以保持一致性
    if (m_level > 0 || m_progress > 0.0f) {
        m_totalExperience = getExperienceForLevel(m_level) +
            static_cast<i32>(m_progress * static_cast<f32>(getExperienceForNextLevel()));
    }

    _validateState();
    markDirty();

    if (m_levelChangeCallback && oldLevel != m_level) {
        m_levelChangeCallback(oldLevel, m_level);
    }
}

void ExperienceManager::reset()
{
    i32 oldLevel = m_level;
    m_level = 0;
    m_progress = 0.0f;
    m_totalExperience = 0;

    markDirty();

    if (m_levelChangeCallback && oldLevel != 0) {
        m_levelChangeCallback(oldLevel, 0);
    }

    if (m_experienceChangeCallback) {
        m_experienceChangeCallback(0);
    }
}

i32 ExperienceManager::getExperienceForNextLevel() const noexcept
{
    return calculateBarCapacity(m_level);
}

i32 ExperienceManager::getExperienceForLevel(i32 level) noexcept
{
    if (level <= 0) {
        return 0;
    }

    // 使用公式计算累计经验
    // 等级 0-14: 每级需要 7 + level * 2
    // 等级 15-29: 每级需要 37 + (level - 15) * 5
    // 等级 30+: 每级需要 112 + (level - 30) * 9

    if (level <= 15) {
        // 累加: sum(7 + 2*i) for i in 0..level-1
        // = 7*level + 2*(0+1+...+level-1)
        // = 7*level + level*(level-1)
        // = level * (level + 6)
        return level * (level + 6);
    } else if (level <= 30) {
        // 前15级: 15 * 21 = 315
        // 等级 15-29: sum(37 + 5*(i-15)) for i in 15..level-1
        // 设 j = i - 15, j 从 0 到 level-16
        // = sum(37 + 5*j) = 37*(level-15) + 5*sum(j) for j in 0..level-16
        // = 37*(level-15) + 5*(level-15)*(level-16)/2
        i32 levelsAfter15 = level - 15;
        return 315 + 37 * levelsAfter15 + 5 * levelsAfter15 * (levelsAfter15 - 1) / 2;
    } else {
        // 前15级: 315
        // 等级 15-30: 37 + 5*(30-15) = 112, 累加 = 315 + sum(37+5*j) for j in 0..14
        // = 315 + 37*15 + 5*14*15/2 = 315 + 555 + 525 = 1395
        // 等级 30+: sum(112 + 9*(i-30)) for i in 30..level-1
        i32 levelsAfter30 = level - 30;
        return 1395 + 112 * levelsAfter30 + 9 * levelsAfter30 * (levelsAfter30 - 1) / 2;
    }
}

i32 ExperienceManager::getLevelFromExperience(i32 totalExperience) noexcept
{
    if (totalExperience <= 0) {
        return 0;
    }

    // 使用二分查找
    i32 low = 0;
    i32 high = constants::MAX_EXPERIENCE_LEVEL;

    while (low < high) {
        i32 mid = low + (high - low + 1) / 2;
        if (getExperienceForLevel(mid) <= totalExperience) {
            low = mid;
        } else {
            high = mid - 1;
        }
    }

    return low;
}

i32 ExperienceManager::calculateBarCapacity(i32 level) noexcept
{
    if (level >= 30) {
        // 等级 30+: 112 + (level - 30) * 9
        // 范围: 112 - 382 (30级时112, 21862级时最大)
        return 112 + (level - 30) * 9;
    } else if (level >= 15) {
        // 等级 15-29: 37 + (level - 15) * 5
        // 范围: 37 - 107
        return 37 + (level - 15) * 5;
    } else {
        // 等级 0-14: 7 + level * 2
        // 范围: 7 - 35
        return 7 + level * 2;
    }
}

void ExperienceManager::resetXpSeed(math::Random& rng)
{
    m_xpSeed = rng.nextInt();
}

bool ExperienceManager::onEnchant(i32 levels, math::Random& rng)
{
    // 直接消耗等级，不检查是否足够
    m_level -= levels;

    if (m_level < 0) {
        // 等级变为负数时重置所有经验
        m_level = 0;
        m_progress = 0.0f;
        m_totalExperience = 0;
    }

    // 重置附魔种子
    resetXpSeed(rng);

    _validateState();
    markDirty();

    return true;
}

i32 ExperienceManager::calculateDeathDropXp() const noexcept
{
    // 死亡掉落经验 = min(level * 7, 100)
    return std::min(m_level * constants::DEATH_XP_PER_LEVEL, constants::MAX_DEATH_XP_DROP);
}

void ExperienceManager::_updateProgress()
{
    // 确保进度在有效范围内
    m_progress = std::clamp(m_progress, 0.0f, 1.0f);

    // 如果进度为0且等级为0，总经验也应为0
    if (m_level == 0 && m_progress <= 0.0f) {
        m_totalExperience = 0;
        m_progress = 0.0f;
    }
}

void ExperienceManager::_handleLevelUp()
{
    i32 oldLevel = m_level - 1;

    // 播放升级音效，等级是5的倍数且距离上次播放至少100 tick时播放
    if (m_level % 5 == 0) {
        u32 currentTick = m_player.ticksExisted();
        // 检查是否满足时间间隔要求（100 tick = 5秒）
        if (static_cast<f32>(m_lastXpSoundTick) < static_cast<f32>(currentTick) - 100.0f) {
            m_lastXpSoundTick = currentTick;

            // 音量计算: 等级 > 30 时为 1.0，否则为 level / 30.0
            // 最终音量 = 基础音量 * 0.75
            f32 baseVolume = (m_level > 30) ? 1.0f : static_cast<f32>(m_level) / 30.0f;
            f32 volume = baseVolume * 0.75f;
            constexpr f32 pitch = 1.0f;

            m_player.playSound(SoundEvents::ENTITY_PLAYER_LEVELUP, volume, pitch);
        }
    }

    if (m_levelChangeCallback) {
        m_levelChangeCallback(oldLevel, m_level);
    }
}

void ExperienceManager::_handleLevelDown()
{
    if (m_level <= 0) {
        m_level = 0;
        m_progress = 0.0f;
        return;
    }

    // 进度为负时降级
    while (m_progress < 0.0f && m_level > 0) {
        m_level--;
        i32 capacity = calculateBarCapacity(m_level);
        m_progress += 1.0f;

        if (m_levelChangeCallback) {
            m_levelChangeCallback(m_level + 1, m_level);
        }
    }

    if (m_level == 0 && m_progress < 0.0f) {
        m_progress = 0.0f;
    }
}

void ExperienceManager::_validateState()
{
    // 确保等级在有效范围
    if (m_level < 0) {
        m_level = 0;
    }
    if (m_level > constants::MAX_EXPERIENCE_LEVEL) {
        m_level = constants::MAX_EXPERIENCE_LEVEL;
    }

    // 确保进度在有效范围
    if (m_progress < 0.0f) {
        _handleLevelDown();
    }
    if (m_progress >= 1.0f) {
        while (m_progress >= 1.0f) {
            m_progress -= 1.0f;
            m_level++;
            _handleLevelUp();
        }
    }

    // 确保进度在 [0, 1) 范围
    m_progress = std::clamp(m_progress, 0.0f, 0.9999f);

    // 确保总经验非负
    if (m_totalExperience < 0) {
        m_totalExperience = 0;
    }

    // 检查总经验与等级/进度的一致性（可选，调试时使用）
    // i32 expectedTotal = getExperienceForLevel(m_level) +
    //                     static_cast<i32>(m_progress * getExperienceForNextLevel());
    // if (m_totalExperience != expectedTotal) {
    //     // 可以选择修正或警告
    // }
}

} // namespace experience
} // namespace entity
} // namespace mc
