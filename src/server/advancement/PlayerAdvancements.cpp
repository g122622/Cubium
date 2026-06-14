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

#include "PlayerAdvancements.hpp"
#include "common/advancement/AdvancementManager.hpp"
#include "common/advancement/trigger/CriterionTriggers.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "server/player/ServerPlayer.hpp"
#include <spdlog/spdlog.h>

namespace mc::server {

PlayerAdvancements::PlayerAdvancements(PlayerId playerId)
    : m_playerId(playerId)
{}

PlayerAdvancements::~PlayerAdvancements() noexcept = default;

bool PlayerAdvancements::grantCriterion(mc::advancement::AdvancementPtr advancement, const std::string& criterion)
{
    if (!advancement) {
        return false;
    }

    bool isNewProgress = false;
    auto it = m_progress.find(advancement);
    if (it == m_progress.end()) {
        // 创建新的进度
        auto [inserted, success] = m_progress.emplace(advancement, mc::advancement::AdvancementProgress(advancement));
        if (!success) {
            return false;
        }
        it = inserted;
        isNewProgress = true;

        // 新进度条目：先为该成就的所有条件注册触发器监听
        // grantCriterion 后续会注销已完成条件的监听器
        registerListeners(advancement);
    }

    bool wasDone = it->second.isDone();
    bool changed = it->second.grantCriterion(criterion);
    if (changed) {
        m_progressChanged.insert(advancement);

        // 如果成就刚完成，发放奖励
        if (!wasDone && it->second.isDone()) {
            spdlog::info("Player {} completed advancement: {}", m_playerId, advancement->getId().toString());

            // 发放奖励
            const auto& rewards = advancement->getRewards();
            if (rewards.has_value() && !rewards->isEmpty()) {
                _grantRewards(*rewards);
            }

            // 成就完成后注销所有监听器
            unregisterListeners(advancement);
        } else {
            // 条件完成但成就未完成，注销该条件的监听器
            const auto& criteria = advancement->getCriteria();
            auto criterionIt = criteria.find(criterion);
            if (criterionIt != criteria.end()) {
                const auto triggerId = criterionIt->second.getTrigger();
                auto* triggerBase = mc::advancement::CriterionTriggers::instance().getTrigger(triggerId);
                if (triggerBase != nullptr) {
                    triggerBase->removeListenerForCriterion(*this, advancement, criterion);
                }
            }
        }

        // 更新可见性
        _ensureVisibility(advancement);
    } else if (isNewProgress) {
        // 新进度但条件授予无变化（条件已满足），注销不必要的监听器
        unregisterListeners(advancement);
    }

    return changed;
}

bool PlayerAdvancements::revokeCriterion(mc::advancement::AdvancementPtr advancement, const std::string& criterion)
{
    if (!advancement) {
        return false;
    }

    auto it = m_progress.find(advancement);
    if (it == m_progress.end()) {
        return false;
    }

    bool wasDone = it->second.isDone();
    bool changed = it->second.revokeCriterion(criterion);
    if (changed) {
        m_progressChanged.insert(advancement);
        _ensureVisibility(advancement);

        // 如果撤销条件导致已完成→未完成状态变化，需要重新注册监听器
        if (wasDone && !it->second.isDone()) {
            // 成就从已完成变为未完成，重新注册所有未完成条件的监听器
            registerListeners(advancement);
        } else {
            // 仅撤销单个条件，为该条件重新注册监听器
            const auto& criteria = advancement->getCriteria();
            auto criterionIt = criteria.find(criterion);
            if (criterionIt != criteria.end()) {
                const auto triggerId = criterionIt->second.getTrigger();
                auto* triggerBase = mc::advancement::CriterionTriggers::instance().getTrigger(triggerId);
                if (triggerBase != nullptr) {
                    const auto& instance = criterionIt->second.getTriggerInstance();
                    if (instance != nullptr) {
                        triggerBase->addListenerForCriterion(*this, advancement, criterion, instance);
                    }
                }
            }
        }
    }

    return changed;
}

bool PlayerAdvancements::grantAllCriteria(mc::advancement::AdvancementPtr advancement)
{
    if (!advancement) {
        return false;
    }

    bool anyChanged = false;
    for (const auto& [criterionName, _] : advancement->getCriteria()) {
        if (grantCriterion(advancement, criterionName)) {
            anyChanged = true;
        }
    }
    return anyChanged;
}

bool PlayerAdvancements::revokeAllCriteria(mc::advancement::AdvancementPtr advancement)
{
    if (!advancement) {
        return false;
    }

    bool anyChanged = false;
    for (const auto& [criterionName, _] : advancement->getCriteria()) {
        if (revokeCriterion(advancement, criterionName)) {
            anyChanged = true;
        }
    }
    return anyChanged;
}

mc::advancement::AdvancementProgress* PlayerAdvancements::getProgress(mc::advancement::AdvancementPtr advancement)
{
    auto it = m_progress.find(advancement);
    return it != m_progress.end() ? &it->second : nullptr;
}

const mc::advancement::AdvancementProgress* PlayerAdvancements::getProgress(
    mc::advancement::AdvancementPtr advancement) const
{
    auto it = m_progress.find(advancement);
    return it != m_progress.end() ? &it->second : nullptr;
}

bool PlayerAdvancements::isDone(mc::advancement::AdvancementPtr advancement) const
{
    const auto* progress = getProgress(advancement);
    return progress != nullptr && progress->isDone();
}

bool PlayerAdvancements::hasProgress(mc::advancement::AdvancementPtr advancement) const
{
    const auto* progress = getProgress(advancement);
    return progress != nullptr && progress->hasProgress();
}

bool PlayerAdvancements::isVisible(mc::advancement::AdvancementPtr advancement) const
{
    return m_visible.count(advancement) > 0;
}

const std::set<mc::advancement::AdvancementPtr>& PlayerAdvancements::getVisibleAdvancements() const
{
    return m_visible;
}

const std::set<mc::advancement::AdvancementPtr>& PlayerAdvancements::getProgressChangedAdvancements() const
{
    return m_progressChanged;
}

void PlayerAdvancements::clearProgressChanged()
{
    m_progressChanged.clear();
}

void PlayerAdvancements::onAdvancementsReloaded(mc::advancement::AdvancementManager& manager)
{
    // 注销所有旧的监听器
    for (const auto& [advancement, progress] : m_progress) {
        unregisterListeners(advancement);
    }

    // 清空进度
    m_progress.clear();
    m_visible.clear();
    m_progressChanged.clear();
    m_visibilityChanged.clear();

    // 重新加载所有成就
    // TODO: 从持久化数据恢复进度
    MC_UNUSED(manager);
}

void PlayerAdvancements::flushAdvancements(mc::advancement::AdvancementManager& manager)
{
    // 遍历成就管理器中所有已注册的成就
    manager.forEach([this](mc::advancement::AdvancementPtr advancement) {
        // 跳过已有进度的成就（loadFromJson 已为它们注册监听器）
        auto it = m_progress.find(advancement);
        if (it != m_progress.end()) {
            // 已有进度，为未完成的成就注册监听器
            if (!it->second.isDone()) {
                registerListeners(advancement);
            }
        } else {
            // 没有进度的新成就，创建空的进度条目并注册监听器
            m_progress.emplace(advancement, mc::advancement::AdvancementProgress(advancement));
            registerListeners(advancement);
        }

        // 检查可见性
        _ensureVisibility(advancement);

        return true; // 继续遍历
    });
}

bool PlayerAdvancements::loadFromJson(const nlohmann::json& json, mc::advancement::AdvancementManager& manager)
{
    if (!json.is_object()) {
        return false;
    }

    // 解析进度
    if (json.contains("advancements")) {
        for (const auto& [advId, progressJson] : json["advancements"].items()) {
            auto advancement = manager.get(mc::ResourceLocation(advId));
            if (!advancement) {
                spdlog::warn("Unknown advancement in player data: {}", advId);
                continue;
            }

            auto progressResult = mc::advancement::AdvancementProgress::fromJson(progressJson, advancement);
            if (progressResult.failed()) {
                spdlog::warn(
                    "Failed to parse advancement progress for {}: {}", advId, progressResult.error().message());
                continue;
            }

            m_progress.emplace(advancement, std::move(progressResult.value()));
        }
    }

    // 更新可见性
    _updateVisibility(&manager);

    // 为所有已加载的成就注册监听器
    for (const auto& [advancement, progress] : m_progress) {
        if (!progress.isDone()) {
            registerListeners(advancement);
        }
    }

    return true;
}

nlohmann::json PlayerAdvancements::toJson() const
{
    nlohmann::json json;

    // 保存进度
    nlohmann::json advancements;
    for (const auto& [advancement, progress] : m_progress) {
        if (progress.hasProgress()) {
            advancements[advancement->getId().toString()] = progress.toJson();
        }
    }
    json["advancements"] = std::move(advancements);

    return json;
}

void PlayerAdvancements::registerListeners(mc::advancement::AdvancementPtr advancement)
{
    if (!advancement) {
        return;
    }

    // 仅当成就未完成时才注册监听器
    auto it = m_progress.find(advancement);
    if (it != m_progress.end() && it->second.isDone()) {
        return;
    }

    // 为每个未完成的条件注册触发器监听
    for (const auto& [criterionName, criterion] : advancement->getCriteria()) {
        // 跳过已完成的条件
        if (it != m_progress.end()) {
            const auto* criterionProgress = it->second.getCriterion(criterionName);
            if (criterionProgress != nullptr && criterionProgress->isObtained()) {
                continue;
            }
        }

        const auto triggerId = criterion.getTrigger();
        auto* triggerBase = mc::advancement::CriterionTriggers::instance().getTrigger(triggerId);
        if (triggerBase == nullptr) {
            spdlog::warn(
                "Unknown trigger: {} for advancement: {}", triggerId.toString(), advancement->getId().toString());
            continue;
        }

        // 获取触发器实例
        const auto& instance = criterion.getTriggerInstance();
        if (instance == nullptr) {
            continue;
        }

        // 通过类型擦除接口注册监听器
        triggerBase->addListenerForCriterion(*this, advancement, criterionName, instance);
    }
}

void PlayerAdvancements::unregisterListeners(mc::advancement::AdvancementPtr advancement)
{
    if (!advancement) {
        return;
    }

    // 为每个已完成条件或成就已完成时注销监听器
    auto it = m_progress.find(advancement);
    const bool advancementDone = it != m_progress.end() && it->second.isDone();

    for (const auto& [criterionName, criterion] : advancement->getCriteria()) {
        // 跳过未完成的条件（除非整个成就已完成）
        if (!advancementDone && it != m_progress.end()) {
            const auto* criterionProgress = it->second.getCriterion(criterionName);
            if (criterionProgress == nullptr || !criterionProgress->isObtained()) {
                continue;
            }
        }

        const auto triggerId = criterion.getTrigger();
        auto* triggerBase = mc::advancement::CriterionTriggers::instance().getTrigger(triggerId);
        if (triggerBase == nullptr) {
            continue;
        }

        // 通过类型擦除接口移除监听器
        triggerBase->removeListenerForCriterion(*this, advancement, criterionName);
    }
}

void PlayerAdvancements::_ensureVisibility(mc::advancement::AdvancementPtr advancement)
{
    if (!advancement) {
        return;
    }

    bool wasVisible = m_visible.count(advancement) > 0;
    bool shouldShow = this->_shouldShow(advancement);

    if (shouldShow != wasVisible) {
        if (shouldShow) {
            m_visible.insert(advancement);
        } else {
            m_visible.erase(advancement);
        }
        m_visibilityChanged.insert(advancement);
    }
}

void PlayerAdvancements::_updateVisibility(mc::advancement::AdvancementManager* manager)
{
    m_visible.clear();

    // 遍历所有进度中的成就，检查可见性
    for (const auto& [advancement, progress] : m_progress) {
        if (_shouldShow(advancement, manager)) {
            m_visible.insert(advancement);
        }
    }

    m_visibilityChanged.clear();
}

bool PlayerAdvancements::_shouldShow(
    mc::advancement::AdvancementPtr advancement, mc::advancement::AdvancementManager* manager) const
{
    if (!advancement) {
        return false;
    }

    // 如果成就已完成或有进度，则可见
    if (isDone(advancement) || hasProgress(advancement)) {
        return true;
    }

    // 如果成就的显示信息标记为隐藏，则不可见
    const auto& display = advancement->getDisplay();
    if (display.has_value() && display->isHidden()) {
        return false;
    }

    // 如果有父成就，需要检查父成就是否已完成（从而确定此成就是否可见）
    // Minecraft 中，如果父成就未完成，子成就仍然可见（只要不是隐藏的），
    // 但如果父成就本身不可见，则子成就也不可见。
    // 简化实现：有显示信息且非隐藏的成就都可见
    if (display.has_value()) {
        return true;
    }

    // 没有显示信息的成就不可见（技术成就）
    return false;
}

void PlayerAdvancements::_grantRewards(const mc::advancement::AdvancementRewards& rewards)
{
    // 发放经验值
    if (rewards.getExperience() > 0 && m_player != nullptr) {
        m_player->addExperience(static_cast<i32>(rewards.getExperience()));
    }

    // 解锁配方
    if (!rewards.getRecipes().empty() && m_player != nullptr) {
        m_player->unlockRecipes(rewards.getRecipes());
    }

    // 战利品表和函数的发放需要 LootTable 系统和命令系统的支持
    // TODO: 实现战利品表发放（需要 LootTable 系统）
    // TODO: 实现函数执行（需要命令/函数系统）
    if (!rewards.getLoot().empty()) {
        spdlog::info("Advancement rewards: {} loot tables pending (LootTable system not yet implemented)",
            rewards.getLoot().size());
    }

    if (rewards.getFunction().has_value()) {
        spdlog::info("Advancement rewards: function {} pending (function system not yet implemented)",
            rewards.getFunction()->toString());
    }
}

} // namespace mc::server
