#include "AdvancementProgress.hpp"
#include <chrono>

namespace mc::advancement {

// ========== CriterionProgress ==========

void CriterionProgress::obtain() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    m_obtainedTime = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

void CriterionProgress::reset() {
    m_obtainedTime = std::nullopt;
}

CriterionProgress CriterionProgress::fromJson(const nlohmann::json& json) {
    CriterionProgress progress;
    if (json.is_string()) {
        // 旧格式：时间字符串 "2024-01-15 10:30:45 +0800"
        // 简化处理，标记为已完成
        progress.m_obtainedTime = 0;
    } else if (json.is_object() && json.contains("obtainedTime")) {
        progress.m_obtainedTime = json["obtainedTime"].get<i64>();
    } else if (json.is_number()) {
        progress.m_obtainedTime = json.get<i64>();
    }
    return progress;
}

nlohmann::json CriterionProgress::toJson() const {
    if (!isObtained()) {
        return nullptr;
    }
    return m_obtainedTime.value();
}

// ========== AdvancementProgress ==========

AdvancementProgress::AdvancementProgress(Advancement::Ptr advancement)
    : m_advancement(std::move(advancement)) {
    if (m_advancement) {
        // 初始化所有条件
        for (const auto& [name, _] : m_advancement->getCriteria()) {
            m_criteria[name] = CriterionProgress();
        }
        m_requirements = m_advancement->getRequirements();
    }
}

bool AdvancementProgress::grantCriterion(const std::string& criterion) {
    auto it = m_criteria.find(criterion);
    if (it == m_criteria.end()) {
        return false;
    }

    if (it->second.isObtained()) {
        return false;  // 已经完成
    }

    it->second.obtain();
    return true;
}

bool AdvancementProgress::revokeCriterion(const std::string& criterion) {
    auto it = m_criteria.find(criterion);
    if (it == m_criteria.end()) {
        return false;
    }

    if (!it->second.isObtained()) {
        return false;  // 已经未完成
    }

    it->second.reset();
    return true;
}

bool AdvancementProgress::isDone() const {
    return computeDone();
}

bool AdvancementProgress::computeDone() const {
    if (m_requirements.empty()) {
        return false;
    }

    // 每个需求组必须至少有一个条件满足
    for (const auto& group : m_requirements) {
        bool groupSatisfied = false;
        for (const auto& criterion : group) {
            auto it = m_criteria.find(criterion);
            if (it != m_criteria.end() && it->second.isObtained()) {
                groupSatisfied = true;
                break;
            }
        }
        if (!groupSatisfied) {
            return false;
        }
    }
    return true;
}

bool AdvancementProgress::hasProgress() const {
    for (const auto& [_, progress] : m_criteria) {
        if (progress.isObtained()) {
            return true;
        }
    }
    return false;
}

f32 AdvancementProgress::getPercent() const {
    if (m_requirements.empty()) {
        return 0.0f;
    }

    size_t completed = countCompletedRequirements();
    return static_cast<f32>(completed) / static_cast<f32>(m_requirements.size());
}

std::string AdvancementProgress::getProgressText() const {
    size_t completed = countCompletedRequirements();
    size_t total = m_requirements.size();

    if (completed == total) {
        return "Done";
    }
    return std::to_string(completed) + "/" + std::to_string(total);
}

size_t AdvancementProgress::countCompletedCriteria() const {
    size_t count = 0;
    for (const auto& [_, progress] : m_criteria) {
        if (progress.isObtained()) {
            ++count;
        }
    }
    return count;
}

size_t AdvancementProgress::countUncompletedCriteria() const {
    return m_criteria.size() - countCompletedCriteria();
}

size_t AdvancementProgress::countRemainingRequirements() const {
    size_t count = 0;
    for (const auto& group : m_requirements) {
        bool groupSatisfied = false;
        for (const auto& criterion : group) {
            auto it = m_criteria.find(criterion);
            if (it != m_criteria.end() && it->second.isObtained()) {
                groupSatisfied = true;
                break;
            }
        }
        if (!groupSatisfied) {
            ++count;
        }
    }
    return count;
}

size_t AdvancementProgress::countCompletedRequirements() const {
    if (m_requirements.empty()) {
        return 0;
    }

    size_t count = 0;
    for (const auto& group : m_requirements) {
        bool groupSatisfied = false;
        for (const auto& criterion : group) {
            auto it = m_criteria.find(criterion);
            if (it != m_criteria.end() && it->second.isObtained()) {
                groupSatisfied = true;
                break;
            }
        }
        if (groupSatisfied) {
            ++count;
        }
    }
    return count;
}

CriterionProgress* AdvancementProgress::getCriterion(const std::string& name) {
    auto it = m_criteria.find(name);
    return it != m_criteria.end() ? &it->second : nullptr;
}

const CriterionProgress* AdvancementProgress::getCriterion(const std::string& name) const {
    auto it = m_criteria.find(name);
    return it != m_criteria.end() ? &it->second : nullptr;
}

void AdvancementProgress::update(Advancement::Ptr advancement) {
    m_advancement = std::move(advancement);
    if (!m_advancement) {
        m_criteria.clear();
        m_requirements.clear();
        return;
    }

    // 获取新的条件名称集合
    std::set<std::string> newCriteriaNames;
    for (const auto& [name, _] : m_advancement->getCriteria()) {
        newCriteriaNames.insert(name);
    }

    // 移除不存在的条件
    for (auto it = m_criteria.begin(); it != m_criteria.end(); ) {
        if (newCriteriaNames.find(it->first) == newCriteriaNames.end()) {
            it = m_criteria.erase(it);
        } else {
            ++it;
        }
    }

    // 添加新条件
    for (const auto& name : newCriteriaNames) {
        if (m_criteria.find(name) == m_criteria.end()) {
            m_criteria[name] = CriterionProgress();
        }
    }

    // 更新需求矩阵
    m_requirements = m_advancement->getRequirements();
}

Result<AdvancementProgress> AdvancementProgress::fromJson(const nlohmann::json& json,
                                                           Advancement::Ptr advancement) {
    if (!json.is_object()) {
        return Error(ErrorCode::ResourceParseError, "AdvancementProgress must be a JSON object");
    }

    AdvancementProgress progress(std::move(advancement));

    // 解析条件进度
    if (json.contains("criteria") && json["criteria"].is_object()) {
        for (const auto& [name, criterionJson] : json["criteria"].items()) {
            progress.m_criteria[name] = CriterionProgress::fromJson(criterionJson);
        }
    }

    return progress;
}

nlohmann::json AdvancementProgress::toJson() const {
    nlohmann::json json;

    // 序列化条件进度
    nlohmann::json criteriaJson;
    for (const auto& [name, progress] : m_criteria) {
        if (progress.isObtained()) {
            criteriaJson[name] = progress.toJson();
        }
    }

    if (!criteriaJson.empty()) {
        json["criteria"] = std::move(criteriaJson);
    }

    // 序列化完成状态
    json["done"] = isDone();

    return json;
}

} // namespace mc::advancement
