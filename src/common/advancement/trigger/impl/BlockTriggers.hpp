#pragma once

#include "../CriterionTrigger.hpp"
#include "conditions/BlockPredicate.hpp"
#include "conditions/LocationPredicate.hpp"
#include <memory>

// 前向声明
namespace mc {
    struct BlockState;
    class World;
    struct BlockPos;
}

namespace mc::advancement {

/**
 * @brief 进入方块触发器
 *
 * 当玩家进入方块（站在方块内）时触发。
 * 参考 MC 1.16.5: net.minecraft.advancements.criterion.EnterBlockTrigger
 */
class EnterBlockTrigger : public AbstractCriterionTrigger<EnterBlockTrigger> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:enter_block";

    class Instance : public CriterionInstance<Instance> {
    public:
        Instance() = default;
        Instance(BlockPredicate block, LocationPredicate location);

        /**
         * @brief 检查条件是否满足
         * @param state 方块状态
         * @param world 世界
         * @param pos 方块位置
         * @return 是否满足
         */
        [[nodiscard]] bool test(const BlockState& state, const World& world, const BlockPos& pos) const;

        Result<void> fromJson(const nlohmann::json& json);
        [[nodiscard]] nlohmann::json conditionsToJson() const;

    private:
        BlockPredicate m_block;
        LocationPredicate m_location;
    };

    [[nodiscard]] ResourceLocation getId() const override {
        return ResourceLocation(TRIGGER_ID);
    }

    [[nodiscard]] Result<std::shared_ptr<Instance>> fromJson(const nlohmann::json& json);

    /**
     * @brief 触发检测
     * @param player 玩家
     * @param state 方块状态
     */
    void trigger(class ServerPlayer& player, const BlockState& state);

    // 静态工厂方法
    static std::shared_ptr<Instance> block(const ResourceLocation& blockId);
};

/**
 * @brief 放置方块触发器
 *
 * 当玩家放置方块时触发。
 * 参考 MC 1.16.5: net.minecraft.advancements.criterion.PlacedBlockTrigger
 */
class PlacedBlockTrigger : public AbstractCriterionTrigger<PlacedBlockTrigger> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:placed_block";

    class Instance : public CriterionInstance<Instance> {
    public:
        Instance() = default;
        Instance(
            BlockPredicate block,
            LocationPredicate location,
            ItemPredicate item
        );

        [[nodiscard]] bool test(
            const BlockState& state,
            const World& world,
            const BlockPos& pos,
            const class ItemStack& item
        ) const;

        Result<void> fromJson(const nlohmann::json& json);
        [[nodiscard]] nlohmann::json conditionsToJson() const;

    private:
        BlockPredicate m_block;
        LocationPredicate m_location;
        ItemPredicate m_item;
    };

    [[nodiscard]] ResourceLocation getId() const override {
        return ResourceLocation(TRIGGER_ID);
    }

    [[nodiscard]] Result<std::shared_ptr<Instance>> fromJson(const nlohmann::json& json);

    void trigger(
        class ServerPlayer& player,
        const BlockState& state,
        const BlockPos& pos,
        const class ItemStack& item
    );
};

/**
 * @brief 在方块上滑落触发器
 *
 * 当玩家在方块上滑落（如蜂蜜块）时触发。
 */
class SlideDownBlockTrigger : public AbstractCriterionTrigger<SlideDownBlockTrigger> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:slide_down_block";

    class Instance : public CriterionInstance<Instance> {
    public:
        Instance() = default;
        explicit Instance(BlockPredicate block);

        [[nodiscard]] bool test(const BlockState& state) const;

        Result<void> fromJson(const nlohmann::json& json);
        [[nodiscard]] nlohmann::json conditionsToJson() const;

    private:
        BlockPredicate m_block;
    };

    [[nodiscard]] ResourceLocation getId() const override {
        return ResourceLocation(TRIGGER_ID);
    }

    [[nodiscard]] Result<std::shared_ptr<Instance>> fromJson(const nlohmann::json& json);

    void trigger(class ServerPlayer& player, const BlockState& state);
};

/**
 * @brief 破坏蜂巢触发器
 *
 * 当玩家破坏蜂巢/蜂箱时触发。
 */
class BeeNestDestroyedTrigger : public AbstractCriterionTrigger<BeeNestDestroyedTrigger> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:bee_nest_destroyed";

    class Instance : public CriterionInstance<Instance> {
    public:
        Instance() = default;
        Instance(BlockPredicate block, ItemPredicate item, IntBounds numBees);

        [[nodiscard]] bool test(
            const BlockState& state,
            const class ItemStack& tool,
            i32 numBeesInside
        ) const;

        Result<void> fromJson(const nlohmann::json& json);
        [[nodiscard]] nlohmann::json conditionsToJson() const;

    private:
        BlockPredicate m_block;
        ItemPredicate m_item;
        IntBounds m_numBees;
    };

    [[nodiscard]] ResourceLocation getId() const override {
        return ResourceLocation(TRIGGER_ID);
    }

    [[nodiscard]] Result<std::shared_ptr<Instance>> fromJson(const nlohmann::json& json);

    void trigger(
        class ServerPlayer& player,
        const BlockState& state,
        const class ItemStack& tool,
        i32 numBeesInside
    );
};

} // namespace mc::advancement
