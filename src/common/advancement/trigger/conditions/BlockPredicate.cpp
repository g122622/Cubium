#include "BlockPredicate.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::advancement {

// ========== StatePropertiesPredicate ==========

bool StatePropertiesPredicate::test(const BlockState& state) const {
    if (m_isAny) {
        return true;
    }
    // TODO: 检查状态属性
    MC_UNUSED(state);
    return true;
}

Result<StatePropertiesPredicate> StatePropertiesPredicate::fromJson(const nlohmann::json& json) {
    MC_UNUSED(json);
    return StatePropertiesPredicate{};
}

nlohmann::json StatePropertiesPredicate::toJson() const {
    return nullptr;
}

// ========== BlockPredicate ==========

bool BlockPredicate::test(const BlockState& state) const {
    if (m_isAny) {
        return true;
    }

    // 检查方块ID
    if (m_block.has_value()) {
        // TODO: 获取方块ID比较
        // if (state.getBlock().getId() != m_block.value()) return false;
    }

    // 检查标签
    if (m_tag.has_value()) {
        // TODO: 检查方块是否在标签中
        // if (!state.getBlock().isInTag(m_tag.value())) return false;
    }

    // 检查状态属性
    if (!m_state.test(state)) {
        return false;
    }

    return true;
}

Result<BlockPredicate> BlockPredicate::fromJson(const nlohmann::json& json) {
    if (json.is_null()) {
        return BlockPredicate{};
    }

    std::optional<ResourceLocation> block;
    std::optional<ResourceLocation> tag;
    StatePropertiesPredicate state;

    if (json.contains("block")) {
        block = ResourceLocation(json["block"].get<std::string>());
    }

    if (json.contains("tag")) {
        tag = ResourceLocation(json["tag"].get<std::string>());
    }

    if (json.contains("state")) {
        auto stateResult = StatePropertiesPredicate::fromJson(json["state"]);
        if (stateResult.failed()) {
            return stateResult.error();
        }
        state = stateResult.value();
    }

    BlockPredicate predicate;
    predicate.m_block = std::move(block);
    predicate.m_tag = std::move(tag);
    predicate.m_state = std::move(state);
    predicate.m_isAny = !predicate.m_block.has_value() && !predicate.m_tag.has_value() && predicate.m_state.isAny();
    return predicate;
}

nlohmann::json BlockPredicate::toJson() const {
    if (m_isAny) {
        return nullptr;
    }

    nlohmann::json json;
    if (m_block.has_value()) {
        json["block"] = m_block.value().toString();
    }
    if (m_tag.has_value()) {
        json["tag"] = m_tag.value().toString();
    }
    if (!m_state.isAny()) {
        json["state"] = m_state.toJson();
    }
    return json;
}

// ========== FluidPredicate ==========

bool FluidPredicate::test(const BlockState& state) const {
    if (m_isAny) {
        return true;
    }

    // TODO: 检查流体
    MC_UNUSED(state);
    return true;
}

Result<FluidPredicate> FluidPredicate::fromJson(const nlohmann::json& json) {
    if (json.is_null()) {
        return FluidPredicate{};
    }

    std::optional<ResourceLocation> fluid;

    if (json.contains("fluid")) {
        fluid = ResourceLocation(json["fluid"].get<std::string>());
    }

    FluidPredicate predicate;
    predicate.m_fluid = std::move(fluid);
    predicate.m_isAny = !predicate.m_fluid.has_value();
    return predicate;
}

nlohmann::json FluidPredicate::toJson() const {
    if (m_isAny) {
        return nullptr;
    }

    nlohmann::json json;
    if (m_fluid.has_value()) {
        json["fluid"] = m_fluid.value().toString();
    }
    return json;
}

} // namespace mc::advancement
