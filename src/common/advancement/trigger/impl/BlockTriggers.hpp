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

#pragma once

#include "../CriterionTrigger.hpp"
#include "../conditions/BlockPredicate.hpp"
#include "../conditions/ItemPredicate.hpp"
#include "../conditions/LocationPredicate.hpp"
#include "common/advancement/MinMaxBounds.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <nlohmann/json_fwd.hpp>

// 前向声明
namespace mc {
class BlockState;
class IWorld;
class BlockPos;
class ItemStack;
} // namespace mc

namespace mc::advancement {

// 前向声明 Instance 类
class EnterBlockTriggerInstance;
class PlacedBlockTriggerInstance;
class SlideDownBlockTriggerInstance;
class BeeNestDestroyedTriggerInstance;

/**
 * @brief 进入方块触发器
 *
 * 当玩家进入方块（站在方块内）时触发。
 */
class EnterBlockTrigger : public AbstractCriterionTrigger<EnterBlockTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:enter_block";

    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    /**
     * @brief 触发检测
     * @param player 玩家
     * @param state 方块状态
     */
    void trigger(class ServerPlayer& player, const BlockState& state);

    // 静态工厂方法
    static std::shared_ptr<EnterBlockTriggerInstance> block(const ResourceLocation& blockId);
};

/**
 * @brief 进入方块触发器实例
 */
class EnterBlockTriggerInstance : public CriterionInstance<EnterBlockTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:enter_block";

    EnterBlockTriggerInstance() = default;
    EnterBlockTriggerInstance(BlockPredicate block, LocationPredicate location);

    /**
     * @brief 检查条件是否满足
     * @param state 方块状态
     * @param world 世界
     * @param pos 方块位置
     * @return 是否满足
     */
    [[nodiscard]] bool test(const BlockState& state, const IWorld& world, const BlockPos& pos) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    BlockPredicate m_block;
    LocationPredicate m_location;
};

/**
 * @brief 放置方块触发器
 *
 * 当玩家放置方块时触发。
 */
class PlacedBlockTrigger : public AbstractCriterionTrigger<PlacedBlockTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:placed_block";

    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    void trigger(ServerPlayer& player, const BlockState& state, const BlockPos& pos, const ItemStack& item);
};

/**
 * @brief 放置方块触发器实例
 */
class PlacedBlockTriggerInstance : public CriterionInstance<PlacedBlockTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:placed_block";

    PlacedBlockTriggerInstance() = default;
    PlacedBlockTriggerInstance(BlockPredicate block, LocationPredicate location, ItemPredicate item);

    [[nodiscard]] bool test(
        const BlockState& state, const IWorld& world, const BlockPos& pos, const ItemStack& item) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    BlockPredicate m_block;
    LocationPredicate m_location;
    ItemPredicate m_item;
};

/**
 * @brief 在方块上滑落触发器
 *
 * 当玩家在方块上滑落（如蜂蜜块）时触发。
 */
class SlideDownBlockTrigger : public AbstractCriterionTrigger<SlideDownBlockTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:slide_down_block";

    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    void trigger(class ServerPlayer& player, const BlockState& state);
};

/**
 * @brief 在方块上滑落触发器实例
 */
class SlideDownBlockTriggerInstance : public CriterionInstance<SlideDownBlockTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:slide_down_block";

    SlideDownBlockTriggerInstance() = default;
    explicit SlideDownBlockTriggerInstance(BlockPredicate block);

    [[nodiscard]] bool test(const BlockState& state) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    BlockPredicate m_block;
};

/**
 * @brief 破坏蜂巢触发器
 *
 * 当玩家破坏蜂巢/蜂箱时触发。
 */
class BeeNestDestroyedTrigger : public AbstractCriterionTrigger<BeeNestDestroyedTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:bee_nest_destroyed";

    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    void trigger(ServerPlayer& player, const BlockState& state, const ItemStack& tool, i32 numBeesInside);
};

/**
 * @brief 破坏蜂巢触发器实例
 */
class BeeNestDestroyedTriggerInstance : public CriterionInstance<BeeNestDestroyedTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:bee_nest_destroyed";

    BeeNestDestroyedTriggerInstance() = default;
    BeeNestDestroyedTriggerInstance(BlockPredicate block, ItemPredicate item, IntBounds numBees);

    [[nodiscard]] bool test(const BlockState& state, const ItemStack& tool, i32 numBeesInside) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    BlockPredicate m_block;
    ItemPredicate m_item;
    IntBounds m_numBees;
};

} // namespace mc::advancement
