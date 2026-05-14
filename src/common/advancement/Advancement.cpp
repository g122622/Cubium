#include "Advancement.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "trigger/CriterionTriggers.hpp"

namespace mc::advancement {

Advancement::Advancement(ResourceLocation id,
    std::optional<ResourceLocation> parent,
    std::optional<AdvancementDisplay> display,
    std::optional<AdvancementRewards> rewards,
    std::map<std::string, Criterion> criteria,
    std::vector<std::vector<std::string>> requirements)
    : m_id(std::move(id))
    , m_parent(std::move(parent))
    , m_display(std::move(display))
    , m_rewards(std::move(rewards))
    , m_criteria(std::move(criteria))
    , m_requirements(std::move(requirements))
{}

void Advancement::addChild(Ptr child) const
{
    m_children.push_back(std::move(child));
}

std::unique_ptr<text::ITextComponent> Advancement::getDisplayText() const
{
    if (!m_display.has_value()) {
        return std::make_unique<text::StringTextComponent>(m_id.toString());
    }
    return m_display->getTitle().deepCopy();
}

Result<Advancement> Advancement::fromJson(const ResourceLocation& id, const nlohmann::json& json)
{
    if (!json.is_object()) {
        return Error(ErrorCode::ResourceParseError, "Advancement '" + id.toString() + "' must be a JSON object");
    }

    // 解析父成就
    std::optional<ResourceLocation> parent;
    if (json.contains("parent")) {
        parent = ResourceLocation::parse(json["parent"].get<std::string>());
    }

    // 解析显示信息
    std::optional<AdvancementDisplay> display;
    if (json.contains("display")) {
        auto displayResult = AdvancementDisplay::fromJson(json["display"]);
        if (displayResult.failed()) {
            return displayResult.error();
        }
        display = std::move(displayResult.value());
    }

    // 解析奖励
    std::optional<AdvancementRewards> rewards;
    if (json.contains("rewards")) {
        auto rewardsResult = AdvancementRewards::fromJson(json["rewards"]);
        if (rewardsResult.failed()) {
            return rewardsResult.error();
        }
        rewards = std::move(rewardsResult.value());
    }

    // 解析条件
    std::map<std::string, Criterion> criteria;
    if (json.contains("criteria") && json["criteria"].is_object()) {
        for (const auto& [name, criterionJson] : json["criteria"].items()) {
            auto criterionResult = Criterion::fromJson(name, criterionJson);
            if (criterionResult.failed()) {
                return criterionResult.error();
            }
            criteria[name] = std::move(criterionResult.value());
        }
    }

    // 解析需求矩阵
    std::vector<std::vector<std::string>> requirements;
    if (json.contains("requirements") && json["requirements"].is_array()) {
        for (const auto& group : json["requirements"]) {
            if (!group.is_array()) {
                return Error(ErrorCode::ResourceParseError,
                    "Advancement '" + id.toString() + "' requirements must be array of arrays");
            }
            std::vector<std::string> reqGroup;
            for (const auto& criterion : group) {
                reqGroup.push_back(criterion.get<std::string>());
            }
            requirements.push_back(std::move(reqGroup));
        }
    } else {
        // 默认需求：每个条件独立成一个组（AND关系）
        for (const auto& [name, _] : criteria) {
            requirements.push_back({name});
        }
    }

    return Advancement(
        id, std::move(parent), std::move(display), std::move(rewards), std::move(criteria), std::move(requirements));
}

nlohmann::json Advancement::toJson() const
{
    nlohmann::json json;

    // 父成就
    if (m_parent.has_value()) {
        json["parent"] = m_parent->toString();
    }

    // 显示信息
    if (m_display.has_value()) {
        json["display"] = m_display->toJson();
    }

    // 奖励
    if (m_rewards.has_value() && !m_rewards->isEmpty()) {
        json["rewards"] = m_rewards->toJson();
    }

    // 条件
    nlohmann::json criteriaJson;
    for (const auto& [name, criterion] : m_criteria) {
        criteriaJson[name] = criterion.toJson();
    }
    json["criteria"] = std::move(criteriaJson);

    // 需求矩阵（只在非默认情况下写入）
    bool isDefaultRequirements = (m_requirements.size() == m_criteria.size());
    if (!isDefaultRequirements) {
        nlohmann::json requirementsJson = nlohmann::json::array();
        for (const auto& group : m_requirements) {
            nlohmann::json groupJson = nlohmann::json::array();
            for (const auto& name : group) {
                groupJson.push_back(name);
            }
            requirementsJson.push_back(std::move(groupJson));
        }
        json["requirements"] = std::move(requirementsJson);
    }

    return json;
}

// ========== Builder实现 ==========

Advancement::Builder& Advancement::Builder::parent(const ResourceLocation& p)
{
    m_parent = p;
    return *this;
}

Advancement::Builder& Advancement::Builder::display(AdvancementDisplay d)
{
    m_display = std::move(d);
    return *this;
}

Advancement::Builder& Advancement::Builder::rewards(AdvancementRewards r)
{
    m_rewards = std::move(r);
    return *this;
}

Advancement::Builder& Advancement::Builder::criterion(
    const std::string& name, std::shared_ptr<ICriterionInstance> instance)
{
    m_criteria[name] = Criterion(name, std::move(instance));
    return *this;
}

Advancement::Builder& Advancement::Builder::requirements(const std::vector<std::vector<std::string>>& req)
{
    m_requirements = req;
    return *this;
}

Advancement::Builder& Advancement::Builder::requirementsStrategy(RequirementsStrategy strategy)
{
    m_requirementsStrategy = strategy;
    return *this;
}

Result<Advancement> Advancement::Builder::build()
{
    if (!m_id.isValid()) {
        return Error(ErrorCode::InvalidArgument, "Advancement ID is required");
    }

    std::vector<std::vector<std::string>> requirements = m_requirements;
    if (requirements.empty()) {
        // 根据策略生成需求矩阵
        if (m_requirementsStrategy == RequirementsStrategy::OR) {
            // OR策略：所有条件放在一个组中
            std::vector<std::string> allCriteria;
            for (const auto& [name, _] : m_criteria) {
                allCriteria.push_back(name);
            }
            requirements.push_back(std::move(allCriteria));
        } else {
            // AND策略：每个条件独立成一个组
            for (const auto& [name, _] : m_criteria) {
                requirements.push_back({name});
            }
        }
    }

    return Advancement(m_id,
        std::move(m_parent),
        std::move(m_display),
        std::move(m_rewards),
        std::move(m_criteria),
        std::move(requirements));
}

Advancement::Ptr Advancement::Builder::registerTo(std::function<void(Ptr)> consumer, const ResourceLocation& id)
{
    m_id = id;
    auto result = build();
    if (result.failed()) {
        return nullptr;
    }
    auto advancement = std::make_shared<Advancement>(std::move(result.value()));
    if (consumer) {
        consumer(advancement);
    }
    return advancement;
}

} // namespace mc::advancement
