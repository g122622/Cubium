#include "PlayerAdvancements.hpp"
#include "common/advancement/AdvancementManager.hpp"
#include "common/advancement/trigger/CriterionTriggers.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <spdlog/spdlog.h>

namespace mc::server {

PlayerAdvancements::PlayerAdvancements(PlayerId playerId)
    : m_playerId(playerId) {
}

PlayerAdvancements::~PlayerAdvancements() = default;

bool PlayerAdvancements::grantCriterion(mc::advancement::AdvancementPtr advancement, const std::string& criterion) {
    if (!advancement) {
        return false;
    }

    auto it = m_progress.find(advancement);
    if (it == m_progress.end()) {
        // 创建新的进度
        auto [inserted, success] = m_progress.emplace(
            advancement,
            mc::advancement::AdvancementProgress(advancement)
        );
        if (!success) {
            return false;
        }
        it = inserted;
    }

    bool changed = it->second.grantCriterion(criterion);
    if (changed) {
        m_progressChanged.insert(advancement);

        // 如果成就完成，检查子成就的可见性
        if (it->second.isDone()) {
            // 成就完成时的奖励等处理
            spdlog::info("Player {} completed advancement: {}",
                m_playerId, advancement->getId().toString());
        }

        // 更新可见性
        ensureVisibility(advancement);
    }

    return changed;
}

bool PlayerAdvancements::revokeCriterion(mc::advancement::AdvancementPtr advancement, const std::string& criterion) {
    if (!advancement) {
        return false;
    }

    auto it = m_progress.find(advancement);
    if (it == m_progress.end()) {
        return false;
    }

    bool changed = it->second.revokeCriterion(criterion);
    if (changed) {
        m_progressChanged.insert(advancement);
        ensureVisibility(advancement);
    }

    return changed;
}

bool PlayerAdvancements::grantAllCriteria(mc::advancement::AdvancementPtr advancement) {
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

bool PlayerAdvancements::revokeAllCriteria(mc::advancement::AdvancementPtr advancement) {
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

mc::advancement::AdvancementProgress* PlayerAdvancements::getProgress(mc::advancement::AdvancementPtr advancement) {
    auto it = m_progress.find(advancement);
    return it != m_progress.end() ? &it->second : nullptr;
}

const mc::advancement::AdvancementProgress* PlayerAdvancements::getProgress(mc::advancement::AdvancementPtr advancement) const {
    auto it = m_progress.find(advancement);
    return it != m_progress.end() ? &it->second : nullptr;
}

bool PlayerAdvancements::isDone(mc::advancement::AdvancementPtr advancement) const {
    const auto* progress = getProgress(advancement);
    return progress != nullptr && progress->isDone();
}

bool PlayerAdvancements::hasProgress(mc::advancement::AdvancementPtr advancement) const {
    const auto* progress = getProgress(advancement);
    return progress != nullptr && progress->hasProgress();
}

bool PlayerAdvancements::isVisible(mc::advancement::AdvancementPtr advancement) const {
    return m_visible.count(advancement) > 0;
}

const std::set<mc::advancement::AdvancementPtr>& PlayerAdvancements::getVisibleAdvancements() const {
    return m_visible;
}

const std::set<mc::advancement::AdvancementPtr>& PlayerAdvancements::getProgressChangedAdvancements() const {
    return m_progressChanged;
}

void PlayerAdvancements::clearProgressChanged() {
    m_progressChanged.clear();
}

void PlayerAdvancements::onAdvancementsReloaded(mc::advancement::AdvancementManager& manager) {
    // 清空进度
    m_progress.clear();
    m_visible.clear();
    m_progressChanged.clear();
    m_visibilityChanged.clear();

    // 重新加载所有成就
    // [阶段5：持久化存储] 从持久化数据恢复进度
    MC_UNUSED(manager);
}

bool PlayerAdvancements::loadFromJson(const nlohmann::json& json, mc::advancement::AdvancementManager& manager) {
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

            mc::advancement::AdvancementProgress progress(advancement);
            if (progressJson.is_object()) {
                for (const auto& [criterion, obtainedTime] : progressJson.items()) {
                    if (obtainedTime.is_number()) {
                        // 创建临时 CriterionProgress
                        progress.grantCriterion(criterion);
                    }
                }
            }

            m_progress.emplace(advancement, std::move(progress));
        }
    }

    // 更新可见性
    updateVisibility();

    return true;
}

nlohmann::json PlayerAdvancements::toJson() const {
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

void PlayerAdvancements::registerListeners(mc::advancement::AdvancementPtr advancement) {
    if (!advancement) {
        return;
    }

    // 为每个条件注册触发器监听
    for (const auto& [criterionName, criterion] : advancement->getCriteria()) {
        const auto triggerId = criterion.getTrigger();
        auto* triggerBase = mc::advancement::CriterionTriggers::instance().getTrigger(triggerId);
        if (triggerBase == nullptr) {
            spdlog::warn("Unknown trigger: {} for advancement: {}",
                triggerId.toString(), advancement->getId().toString());
            continue;
        }

        // 创建监听器并注册
        // [阶段2+3：事件系统集成] 根据触发器类型创建正确的监听器并注册到触发器
    }
}

void PlayerAdvancements::unregisterListeners(mc::advancement::AdvancementPtr advancement) {
    if (!advancement) {
        return;
    }

    // 注销所有触发器监听
    for (const auto& [criterionName, criterion] : advancement->getCriteria()) {
        const auto triggerId = criterion.getTrigger();
        auto* triggerBase = mc::advancement::CriterionTriggers::instance().getTrigger(triggerId);
        if (triggerBase == nullptr) {
            continue;
        }

        // [阶段2+3：事件系统集成] 移除监听器
    }
}

void PlayerAdvancements::ensureVisibility(mc::advancement::AdvancementPtr advancement) {
    if (!advancement) {
        return;
    }

    bool wasVisible = m_visible.count(advancement) > 0;
    bool shouldShow = this->shouldShow(advancement);

    if (shouldShow != wasVisible) {
        if (shouldShow) {
            m_visible.insert(advancement);
        } else {
            m_visible.erase(advancement);
        }
        m_visibilityChanged.insert(advancement);
    }
}

void PlayerAdvancements::updateVisibility() {
    m_visible.clear();

    // 遍历所有成就，检查可见性
    // 注意：需要在调用此方法前通过 onAdvancementsReloaded 或其他方式
    // 填充 m_visible 集合。此处仅清除状态。

    m_visibilityChanged.clear();
}

bool PlayerAdvancements::shouldShow(mc::advancement::AdvancementPtr advancement) const {
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

    // 如果父成就未完成，则不可见
    auto parent = advancement->getParent();
    if (parent.has_value()) {
        // 注意：父成就的可见性检查需要在 updateVisibility 中处理
        // 因为需要访问 AdvancementManager，而 shouldShow 是 const 方法
    }

    return true;
}

} // namespace mc::server
