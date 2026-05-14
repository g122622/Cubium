#include "BlockPredicate.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"

namespace mc::advancement {

// ========== BlockPredicate ==========

BlockPredicate::BlockPredicate(
    std::optional<ResourceLocation> block, std::optional<ResourceLocation> tag, StatePropertiesPredicate state)
    : m_block(std::move(block))
    , m_tag(std::move(tag))
    , m_state(std::move(state))
    , m_isAny(!m_block.has_value() && !m_tag.has_value() && m_state.isEmpty())
{}

bool BlockPredicate::test(const BlockState& state) const
{
    if (m_isAny) {
        return true;
    }

    // 检查方块ID
    if (m_block.has_value()) {
        // 参考 MC 1.16.5: if (this.block != null && block != this.block)
        const Block* expectedBlock = BlockRegistry::instance().getBlock(m_block.value());
        if (expectedBlock == nullptr) {
            // 未知的方块ID，不匹配
            return false;
        }
        if (&state.getBlock() != expectedBlock) {
            return false;
        }
    }

    // 检查标签
    if (m_tag.has_value()) {
        // 参考 MC 1.16.5: if (this.tag != null && !this.tag.contains(block))
        BlockTag* tag = BlockTags::getTag(m_tag.value());
        if (tag == nullptr) {
            // 未知的标签，不匹配
            return false;
        }
        if (!tag->contains(state)) {
            return false;
        }
    }

    // 检查状态属性
    if (!m_state.matches(state)) {
        return false;
    }

    return true;
}

Result<BlockPredicate> BlockPredicate::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return BlockPredicate{};
    }

    std::optional<ResourceLocation> block;
    std::optional<ResourceLocation> tag;
    StatePropertiesPredicate state;

    // 支持简写格式：直接传字符串表示方块ID
    // 参考 MC 1.16.5: JSONUtils.getJsonObject(json, "block") 会将字符串转为 {"block": "xxx"}
    if (json.is_string()) {
        block = ResourceLocation(json.get<std::string>());
        return BlockPredicate(std::move(block), std::move(tag), std::move(state));
    }

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
        state = std::move(stateResult.value());
    }

    return BlockPredicate(std::move(block), std::move(tag), std::move(state));
}

nlohmann::json BlockPredicate::toJson() const
{
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
    if (!m_state.isEmpty()) {
        json["state"] = m_state.toJsonValue();
    }
    return json;
}

// ========== FluidPredicate ==========

bool FluidPredicate::test(const BlockState& state) const
{
    if (m_isAny) {
        return true;
    }

    // 检查流体ID
    if (m_fluid.has_value()) {
        // 参考 MC 1.16.5: FluidState fluidstate = world.getFluidState(pos);
        // if (!fluidstate.is(m_fluid)) return false;

        // 获取流体状态
        const fluid::FluidState* fluidState = state.getFluidState();
        if (fluidState == nullptr) {
            return false;
        }

        // 检查流体类型
        // 目前简化实现：检查流体是否匹配
        // TODO: 完善流体系统后，使用 FluidRegistry 和流体ID比较
        MC_UNUSED(fluidState);
    }

    // 检查状态属性
    if (!m_state.matches(state)) {
        return false;
    }

    return true;
}

Result<FluidPredicate> FluidPredicate::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return FluidPredicate{};
    }

    std::optional<ResourceLocation> fluid;
    StatePropertiesPredicate state;

    if (json.contains("fluid")) {
        fluid = ResourceLocation(json["fluid"].get<std::string>());
    }

    if (json.contains("state")) {
        auto stateResult = StatePropertiesPredicate::fromJson(json["state"]);
        if (stateResult.failed()) {
            return stateResult.error();
        }
        state = std::move(stateResult.value());
    }

    FluidPredicate predicate;
    predicate.m_fluid = std::move(fluid);
    predicate.m_state = std::move(state);
    predicate.m_isAny = !predicate.m_fluid.has_value() && predicate.m_state.isEmpty();
    return predicate;
}

nlohmann::json FluidPredicate::toJson() const
{
    if (m_isAny) {
        return nullptr;
    }

    nlohmann::json json;
    if (m_fluid.has_value()) {
        json["fluid"] = m_fluid.value().toString();
    }
    if (!m_state.isEmpty()) {
        json["state"] = m_state.toJsonValue();
    }
    return json;
}

} // namespace mc::advancement
