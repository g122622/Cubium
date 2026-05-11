#include "BlockTriggers.hpp"

namespace mc::advancement {

// ========== EnterBlockTrigger ==========

EnterBlockTrigger::Instance::Instance(BlockPredicate block, LocationPredicate location)
    : m_block(std::move(block))
    , m_location(std::move(location)) {
}

bool EnterBlockTrigger::Instance::test(const BlockState& state, const World& world, const BlockPos& pos) const {
    if (!m_block.test(state)) {
        return false;
    }
    if (!m_location.test(world, pos)) {
        return false;
    }
    return true;
}

Result<void> EnterBlockTrigger::Instance::fromJson(const nlohmann::json& json) {
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

nlohmann::json EnterBlockTrigger::Instance::conditionsToJson() const {
    nlohmann::json json;

    if (!m_block.isAny()) {
        json["block"] = m_block.toJson();
    }
    if (!m_location.isAny()) {
        json["location"] = m_location.toJson();
    }

    return json;
}

Result<std::shared_ptr<EnterBlockTrigger::Instance>> EnterBlockTrigger::fromJson(const nlohmann::json& json) {
    auto instance = std::make_shared<Instance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void EnterBlockTrigger::trigger(ServerPlayer& player, const BlockState& state) {
    // [TODO 阶段2+3：事件系统集成] 由 EnterBlockEvent 触发
    MC_UNUSED(player);
    MC_UNUSED(state);
}

std::shared_ptr<EnterBlockTrigger::Instance> EnterBlockTrigger::block(const ResourceLocation& blockId) {
    BlockPredicate pred;
    pred = BlockPredicate::fromJson(nlohmann::json{{"block", blockId.toString()}}).value();
    return std::make_shared<Instance>(pred, LocationPredicate{});
}

// ========== PlacedBlockTrigger ==========

PlacedBlockTrigger::Instance::Instance(
    BlockPredicate block,
    LocationPredicate location,
    ItemPredicate item
)
    : m_block(std::move(block))
    , m_location(std::move(location))
    , m_item(std::move(item)) {
}

bool PlacedBlockTrigger::Instance::test(
    const BlockState& state,
    const World& world,
    const BlockPos& pos,
    const ItemStack& item
) const {
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

Result<void> PlacedBlockTrigger::Instance::fromJson(const nlohmann::json& json) {
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

nlohmann::json PlacedBlockTrigger::Instance::conditionsToJson() const {
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

Result<std::shared_ptr<PlacedBlockTrigger::Instance>> PlacedBlockTrigger::fromJson(const nlohmann::json& json) {
    auto instance = std::make_shared<Instance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void PlacedBlockTrigger::trigger(
    ServerPlayer& player,
    const BlockState& state,
    const BlockPos& pos,
    const ItemStack& item
) {
    // [TODO 阶段2+3：事件系统集成] 由 BlockPlaceEvent 触发
    MC_UNUSED(player);
    MC_UNUSED(state);
    MC_UNUSED(pos);
    MC_UNUSED(item);
}

// ========== SlideDownBlockTrigger ==========

SlideDownBlockTrigger::Instance::Instance(BlockPredicate block)
    : m_block(std::move(block)) {
}

bool SlideDownBlockTrigger::Instance::test(const BlockState& state) const {
    return m_block.test(state);
}

Result<void> SlideDownBlockTrigger::Instance::fromJson(const nlohmann::json& json) {
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

nlohmann::json SlideDownBlockTrigger::Instance::conditionsToJson() const {
    if (!m_block.isAny()) {
        return {{"block", m_block.toJson()}};
    }
    return nullptr;
}

Result<std::shared_ptr<SlideDownBlockTrigger::Instance>> SlideDownBlockTrigger::fromJson(const nlohmann::json& json) {
    auto instance = std::make_shared<Instance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void SlideDownBlockTrigger::trigger(ServerPlayer& player, const BlockState& state) {
    // [TODO 阶段2+3：事件系统集成] 由 SlideDownBlockEvent 触发
    MC_UNUSED(player);
    MC_UNUSED(state);
}

// ========== BeeNestDestroyedTrigger ==========

BeeNestDestroyedTrigger::Instance::Instance(BlockPredicate block, ItemPredicate item, IntBounds numBees)
    : m_block(std::move(block))
    , m_item(std::move(item))
    , m_numBees(std::move(numBees)) {
}

bool BeeNestDestroyedTrigger::Instance::test(
    const BlockState& state,
    const ItemStack& tool,
    i32 numBeesInside
) const {
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

Result<void> BeeNestDestroyedTrigger::Instance::fromJson(const nlohmann::json& json) {
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

nlohmann::json BeeNestDestroyedTrigger::Instance::conditionsToJson() const {
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

Result<std::shared_ptr<BeeNestDestroyedTrigger::Instance>> BeeNestDestroyedTrigger::fromJson(const nlohmann::json& json) {
    auto instance = std::make_shared<Instance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void BeeNestDestroyedTrigger::trigger(
    ServerPlayer& player,
    const BlockState& state,
    const ItemStack& tool,
    i32 numBeesInside
) {
    // [TODO 阶段2+3：事件系统集成] 由 BeeNestDestroyedEvent 触发
    MC_UNUSED(player);
    MC_UNUSED(state);
    MC_UNUSED(tool);
    MC_UNUSED(numBeesInside);
}

} // namespace mc::advancement
