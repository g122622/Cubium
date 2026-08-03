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

#include "BlockPredicate.hpp"
#include "common/core/Result.hpp"
#include "common/item/loot/StatePropertiesPredicate.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/fluid/Fluid.hpp"
#include <optional>
#include <string>
#include <utility>
#include <nlohmann/json_fwd.hpp>

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
        // 获取流体状态
        const fluid::FluidState* fluidState = state.getFluidState();
        if (fluidState == nullptr) {
            return false;
        }

        // 空流体不匹配任何非空流体
        if (fluidState->isEmpty()) {
            return false;
        }

        // 获取期望的流体
        fluid::Fluid* expectedFluid = fluid::Fluid::getFluid(m_fluid.value());
        if (expectedFluid == nullptr) {
            // 未知的流体ID，不匹配
            return false;
        }

        // 使用 isEquivalentTo 比较（水和流动水视为等效）
        if (!fluidState->getFluid().isEquivalentTo(*expectedFluid)) {
            return false;
        }
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
