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

#include "BlockTriggers.hpp"
#include "common/advancement/MinMaxBounds.hpp"
#include "common/advancement/trigger/CriterionTrigger.hpp"
#include "common/advancement/trigger/conditions/BlockPredicate.hpp"
#include "common/advancement/trigger/conditions/ItemPredicate.hpp"
#include "common/advancement/trigger/conditions/LocationPredicate.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include <memory>
#include <utility>
#include <nlohmann/json_fwd.hpp>

namespace mc::advancement {

// ========== EnterBlockTriggerInstance ==========

EnterBlockTriggerInstance::EnterBlockTriggerInstance(BlockPredicate block, LocationPredicate location)
    : m_block(std::move(block))
    , m_location(std::move(location))
{}

bool EnterBlockTriggerInstance::test(const BlockState& state, const IWorld& world, const BlockPos& pos) const
{
    if (!m_block.test(state)) {
        return false;
    }
    if (!m_location.test(world, pos)) {
        return false;
    }
    return true;
}

Result<void> EnterBlockTriggerInstance::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return {};
    }

    if (json.contains("block")) {
        auto result = BlockPredicate::fromJson(json["block"]);
        if (result.failed()) {
            return result.error();
        }
        m_block = result.value();
    }

    if (json.contains("location")) {
        auto result = LocationPredicate::fromJson(json["location"]);
        if (result.failed()) {
            return result.error();
        }
        m_location = result.value();
    }

    return {};
}

nlohmann::json EnterBlockTriggerInstance::conditionsToJson() const
{
    nlohmann::json json;

    if (!m_block.isAny()) {
        json["block"] = m_block.toJson();
    }
    if (!m_location.isAny()) {
        json["location"] = m_location.toJson();
    }

    return json;
}

// ========== EnterBlockTrigger ==========

Result<std::shared_ptr<ICriterionInstance>> EnterBlockTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<EnterBlockTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void EnterBlockTrigger::trigger(ServerPlayer& player, const BlockState& state)
{
    // 触发器通过 AdvancementEventHandler::onEnterBlock() 调用基类的 trigger() 模板方法
    // 此方法保留作为接口，实际触发逻辑在服务端事件处理器中
    MC_UNUSED(player);
    MC_UNUSED(state);
}

std::shared_ptr<EnterBlockTriggerInstance> EnterBlockTrigger::block(const ResourceLocation& blockId)
{
    BlockPredicate pred;
    pred = BlockPredicate::fromJson(nlohmann::json{{"block", blockId.toString()}}).value();
    return std::make_shared<EnterBlockTriggerInstance>(pred, LocationPredicate{});
}

// ========== PlacedBlockTriggerInstance ==========

PlacedBlockTriggerInstance::PlacedBlockTriggerInstance(
    BlockPredicate block, LocationPredicate location, ItemPredicate item)
    : m_block(std::move(block))
    , m_location(std::move(location))
    , m_item(std::move(item))
{}

bool PlacedBlockTriggerInstance::test(
    const BlockState& state, const IWorld& world, const BlockPos& pos, const ItemStack& item) const
{
    if (!m_block.test(state)) {
        return false;
    }
    if (!m_location.test(world, pos)) {
        return false;
    }
    if (!m_item.test(item)) {
        return false;
    }
    return true;
}

Result<void> PlacedBlockTriggerInstance::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return {};
    }

    if (json.contains("block")) {
        auto result = BlockPredicate::fromJson(json["block"]);
        if (result.failed()) {
            return result.error();
        }
        m_block = result.value();
    }

    if (json.contains("location")) {
        auto result = LocationPredicate::fromJson(json["location"]);
        if (result.failed()) {
            return result.error();
        }
        m_location = result.value();
    }

    if (json.contains("item")) {
        auto result = ItemPredicate::fromJson(json["item"]);
        if (result.failed()) {
            return result.error();
        }
        m_item = result.value();
    }

    return {};
}

nlohmann::json PlacedBlockTriggerInstance::conditionsToJson() const
{
    nlohmann::json json;

    if (!m_block.isAny()) {
        json["block"] = m_block.toJson();
    }
    if (!m_location.isAny()) {
        json["location"] = m_location.toJson();
    }
    if (!m_item.isAny()) {
        json["item"] = m_item.toJson();
    }

    return json;
}

// ========== PlacedBlockTrigger ==========

Result<std::shared_ptr<ICriterionInstance>> PlacedBlockTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<PlacedBlockTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void PlacedBlockTrigger::trigger(
    ServerPlayer& player, const BlockState& state, const BlockPos& pos, const ItemStack& item)
{
    // 触发器通过 AdvancementEventHandler::onBlockPlaced() 调用基类的 trigger() 模板方法
    // 此方法保留作为接口，实际触发逻辑在服务端事件处理器中
    MC_UNUSED(player);
    MC_UNUSED(state);
    MC_UNUSED(pos);
    MC_UNUSED(item);
}

// ========== SlideDownBlockTriggerInstance ==========

SlideDownBlockTriggerInstance::SlideDownBlockTriggerInstance(BlockPredicate block)
    : m_block(std::move(block))
{}

bool SlideDownBlockTriggerInstance::test(const BlockState& state) const
{
    return m_block.test(state);
}

Result<void> SlideDownBlockTriggerInstance::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return {};
    }

    if (json.contains("block")) {
        auto result = BlockPredicate::fromJson(json["block"]);
        if (result.failed()) {
            return result.error();
        }
        m_block = result.value();
    }

    return {};
}

nlohmann::json SlideDownBlockTriggerInstance::conditionsToJson() const
{
    if (!m_block.isAny()) {
        return {{"block", m_block.toJson()}};
    }
    return nullptr;
}

// ========== SlideDownBlockTrigger ==========

Result<std::shared_ptr<ICriterionInstance>> SlideDownBlockTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<SlideDownBlockTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void SlideDownBlockTrigger::trigger(ServerPlayer& player, const BlockState& state)
{
    // 触发器通过 AdvancementEventHandler::onSlideDownBlock() 调用基类的 trigger() 模板方法
    // 此方法保留作为接口，实际触发逻辑在服务端事件处理器中
    MC_UNUSED(player);
    MC_UNUSED(state);
}

// ========== BeeNestDestroyedTriggerInstance ==========

BeeNestDestroyedTriggerInstance::BeeNestDestroyedTriggerInstance(
    BlockPredicate block, ItemPredicate item, IntBounds numBees)
    : m_block(std::move(block))
    , m_item(std::move(item))
    , m_numBees(std::move(numBees))
{}

bool BeeNestDestroyedTriggerInstance::test(const BlockState& state, const ItemStack& tool, i32 numBeesInside) const
{
    if (!m_block.test(state)) {
        return false;
    }
    if (!m_item.test(tool)) {
        return false;
    }
    if (!m_numBees.test(numBeesInside)) {
        return false;
    }
    return true;
}

Result<void> BeeNestDestroyedTriggerInstance::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return {};
    }

    if (json.contains("block")) {
        auto result = BlockPredicate::fromJson(json["block"]);
        if (result.failed()) {
            return result.error();
        }
        m_block = result.value();
    }

    if (json.contains("item")) {
        auto result = ItemPredicate::fromJson(json["item"]);
        if (result.failed()) {
            return result.error();
        }
        m_item = result.value();
    }

    if (json.contains("num_bees_inside")) {
        m_numBees = IntBounds::fromJson(json["num_bees_inside"]);
    }

    return {};
}

nlohmann::json BeeNestDestroyedTriggerInstance::conditionsToJson() const
{
    nlohmann::json json;

    if (!m_block.isAny()) {
        json["block"] = m_block.toJson();
    }
    if (!m_item.isAny()) {
        json["item"] = m_item.toJson();
    }
    if (!m_numBees.isUnbounded()) {
        json["num_bees_inside"] = m_numBees.toJson();
    }

    return json;
}

// ========== BeeNestDestroyedTrigger ==========

Result<std::shared_ptr<ICriterionInstance>> BeeNestDestroyedTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<BeeNestDestroyedTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void BeeNestDestroyedTrigger::trigger(
    ServerPlayer& player, const BlockState& state, const ItemStack& tool, i32 numBeesInside)
{
    // 触发器通过 AdvancementEventHandler::onBeeNestDestroyed() 调用基类的 trigger() 模板方法
    // 此方法保留作为接口，实际触发逻辑在服务端事件处理器中
    MC_UNUSED(player);
    MC_UNUSED(state);
    MC_UNUSED(tool);
    MC_UNUSED(numBeesInside);
}

} // namespace mc::advancement
