#include "ExperienceManager.hpp"
#include "ExperienceConstants.hpp"
#include "../entities/player/Player.hpp"
#include "../../util/math/MathUtils.hpp"
#include <algorithm>
#include <cmath>

namespace mc {
namespace entity {
namespace experience {

ExperienceManager::ExperienceManager(Player& player)
    : m_player(player)
{
}

void ExperienceManager::addExperience(i32 amount) {
    if (amount <= 0) {
        return;
    }

    m_totalExperience += amount;

    // 计算进度条增量
    i32 capacity = getExperienceForNextLevel();
    f32 progressGain = static_cast<f32>(amount) / static_cast<f32>(capacity);

    m_progress += progressGain;

    // 处理升级
    while (m_progress >= 1.0f) {
        m_progress -= 1.0f;
        m_level++;
        handleLevelUp();
    }

    validateState();
    markDirty();

    // 触发回调
    if (m_experienceChangeCallback) {
        m_experienceChangeCallback(m_totalExperience);
    }
}

bool ExperienceManager::consumeExperience(i32 amount) {
    if (amount <= 0) {
        return true;
    }

    // 计算当前总经验
    i32 currentTotal = getExperienceForLevel(m_level) +
                       static_cast<i32>(m_progress * static_cast<f32>(getExperienceForNextLevel()));

    if (m_totalExperience < amount) {
        return false;
    }

    m_totalExperience -= amount;

    // 重新计算等级和进度
    i32 newLevel = getLevelFromExperience(m_totalExperience);
    i32 xpForNewLevel = getExperienceForLevel(newLevel);
    i32 xpInCurrentLevel = m_totalExperience - xpForNewLevel;
    i32 capacity = calculateBarCapacity(newLevel);

    m_level = newLevel;
    m_progress = capacity > 0 ? static_cast<f32>(xpInCurrentLevel) / static_cast<f32>(capacity) : 0.0f;

    validateState();
    markDirty();

    if (m_experienceChangeCallback) {
        m_experienceChangeCallback(m_totalExperience);
    }

    return true;
}

bool ExperienceManager::consumeLevels(i32 levels) {
    if (levels <= 0) {
        return true;
    }

    if (m_level < levels) {
        return false;
    }

    i32 oldLevel = m_level;
    m_level -= levels;

    // 确保进度有效
    i32 capacity = getExperienceForNextLevel();
    i32 xpInCurrentLevel = static_cast<i32>(m_progress * static_cast<f32>(capacity));

    // 更新总经验
    m_totalExperience = getExperienceForLevel(m_level) + xpInCurrentLevel;

    validateState();
    markDirty();

    if (m_levelChangeCallback) {
        m_levelChangeCallback(oldLevel, m_level);
    }

    return true;
}

void ExperienceManager::setExperience(i32 level, f32 progress, i32 totalExperience) {
    m_level = std::max(0, level);
    m_progress = std::clamp(progress, 0.0f, 1.0f);
    m_totalExperience = std::max(0, totalExperience);

    validateState();
    markDirty();

    if (m_experienceChangeCallback) {
        m_experienceChangeCallback(m_totalExperience);
    }
}

void ExperienceManager::setLevel(i32 level) {
    i32 oldLevel = m_level;
    m_level = std::max(0, level);

    // 重新计算总经验
    i32 xpInCurrentLevel = static_cast<i32>(m_progress * static_cast<f32>(getExperienceForNextLevel()));
    m_totalExperience = getExperienceForLevel(m_level) + xpInCurrentLevel;

    validateState();
    markDirty();

    if (m_levelChangeCallback && oldLevel != m_level) {
        m_levelChangeCallback(oldLevel, m_level);
    }
}

void ExperienceManager::addLevels(i32 levels) {
    if (levels == 0) {
        return;
    }

    i32 oldLevel = m_level;
    m_level = std::max(0, m_level + levels);

    if (m_level > constants::MAX_EXPERIENCE_LEVEL) {
        m_level = constants::MAX_EXPERIENCE_LEVEL;
    }

    // 更新总经验
    if (levels > 0) {
        m_totalExperience = getExperienceForLevel(m_level) +
                           static_cast<i32>(m_progress * static_cast<f32>(getExperienceForNextLevel()));
    }

    validateState();
    markDirty();

    if (m_levelChangeCallback && oldLevel != m_level) {
        m_levelChangeCallback(oldLevel, m_level);
    }
}

void ExperienceManager::reset() {
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

i32 ExperienceManager::getExperienceForNextLevel() const {
    return calculateBarCapacity(m_level);
}

i32 ExperienceManager::getExperienceForLevel(i32 level) {
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

i32 ExperienceManager::getLevelFromExperience(i32 totalExperience) {
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

i32 ExperienceManager::calculateBarCapacity(i32 level) {
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

void ExperienceManager::resetXpSeed(math::Random& rng) {
    m_xpSeed = rng.nextInt();
}

bool ExperienceManager::onEnchant(i32 levels, math::Random& rng) {
    if (!consumeLevels(levels)) {
        return false;
    }

    resetXpSeed(rng);
    return true;
}

i32 ExperienceManager::calculateDeathDropXp() const {
    // 死亡掉落经验 = min(level * 7, 100)
    return std::min(m_level * constants::DEATH_XP_PER_LEVEL, constants::MAX_DEATH_XP_DROP);
}

void ExperienceManager::updateProgress() {
    // 确保进度在有效范围内
    m_progress = std::clamp(m_progress, 0.0f, 1.0f);

    // 如果进度为0且等级为0，总经验也应为0
    if (m_level == 0 && m_progress <= 0.0f) {
        m_totalExperience = 0;
        m_progress = 0.0f;
    }
}

void ExperienceManager::handleLevelUp() {
    i32 oldLevel = m_level - 1;

    // 每5级播放升级音效
    if (m_level % 5 == 0 && m_level > m_lastLevelUpSoundLevel) {
        m_lastLevelUpSoundLevel = m_level;
        // TODO: 播放升级音效
        // 音量计算: 如果等级 > 30 则为 1.0，否则为 level / 30.0
    }

    if (m_levelChangeCallback) {
        m_levelChangeCallback(oldLevel, m_level);
    }
}

void ExperienceManager::handleLevelDown() {
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

void ExperienceManager::validateState() {
    // 确保等级在有效范围
    if (m_level < 0) {
        m_level = 0;
    }
    if (m_level > constants::MAX_EXPERIENCE_LEVEL) {
        m_level = constants::MAX_EXPERIENCE_LEVEL;
    }

    // 确保进度在有效范围
    if (m_progress < 0.0f) {
        handleLevelDown();
    }
    if (m_progress >= 1.0f) {
        while (m_progress >= 1.0f) {
            m_progress -= 1.0f;
            m_level++;
            handleLevelUp();
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
