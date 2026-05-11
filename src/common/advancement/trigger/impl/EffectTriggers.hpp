#pragma once

#include "../CriterionTrigger.hpp"
#include <memory>

namespace mc::advancement {

/**
 * @brief Tick触发器
 *
 * 每tick触发检测（用于持续检测条件）。
 * 参考 MC 1.16.5: net.minecraft.advancements.criterion.TickTrigger
 */
class TickTrigger : public AbstractCriterionTrigger<TickTrigger> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:tick";

    class Instance : public CriterionInstance<Instance> {
    public:
        Instance() = default;

        [[nodiscard]] bool test() const { return true; }

        Result<void> fromJson(const nlohmann::json& json);
        [[nodiscard]] nlohmann::json conditionsToJson() const;
    };

    [[nodiscard]] ResourceLocation getId() const override {
        return ResourceLocation(TRIGGER_ID);
    }

    [[nodiscard]] Result<std::shared_ptr<Instance>> fromJson(const nlohmann::json& json);

    void trigger(class ServerPlayer& player);
};

/**
 * @brief 配方解锁触发器
 *
 * 当玩家解锁配方时触发。
 */
class RecipeUnlockedTrigger : public AbstractCriterionTrigger<RecipeUnlockedTrigger> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:recipe_unlocked";

    class Instance : public CriterionInstance<Instance> {
    public:
        Instance() = default;
        explicit Instance(ResourceLocation recipe);

        [[nodiscard]] bool test(const ResourceLocation& recipe) const;

        Result<void> fromJson(const nlohmann::json& json);
        [[nodiscard]] nlohmann::json conditionsToJson() const;

    private:
        ResourceLocation m_recipe;
    };

    [[nodiscard]] ResourceLocation getId() const override {
        return ResourceLocation(TRIGGER_ID);
    }

    [[nodiscard]] Result<std::shared_ptr<Instance>> fromJson(const nlohmann::json& json);

    void trigger(class ServerPlayer& player, const ResourceLocation& recipe);
};

/**
 * @brief 效果变化触发器
 *
 * 当玩家获得/失去效果时触发。
 */
class EffectsChangedTrigger : public AbstractCriterionTrigger<EffectsChangedTrigger> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:effects_changed";

    class Instance : public CriterionInstance<Instance> {
    public:
        Instance() = default;
        explicit Instance(MobEffectsPredicate effects);

        [[nodiscard]] bool test(const class Entity& entity) const;

        Result<void> fromJson(const nlohmann::json& json);
        [[nodiscard]] nlohmann::json conditionsToJson() const;

    private:
        MobEffectsPredicate m_effects;
    };

    [[nodiscard]] ResourceLocation getId() const override {
        return ResourceLocation(TRIGGER_ID);
    }

    [[nodiscard]] Result<std::shared_ptr<Instance>> fromJson(const nlohmann::json& json);

    void trigger(class ServerPlayer& player);
};

/**
 * @brief 酿造药水触发器
 *
 * 当玩家酿造药水时触发。
 */
class BrewedPotionTrigger : public AbstractCriterionTrigger<BrewedPotionTrigger> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:brewed_potion";

    class Instance : public CriterionInstance<Instance> {
    public:
        Instance() = default;
        explicit Instance(ResourceLocation potion);

        [[nodiscard]] bool test(const ResourceLocation& potion) const;

        Result<void> fromJson(const nlohmann::json& json);
        [[nodiscard]] nlohmann::json conditionsToJson() const;

    private:
        ResourceLocation m_potion;
    };

    [[nodiscard]] ResourceLocation getId() const override {
        return ResourceLocation(TRIGGER_ID);
    }

    [[nodiscard]] Result<std::shared_ptr<Instance>> fromJson(const nlohmann::json& json);

    void trigger(class ServerPlayer& player, const ResourceLocation& potion);
};

} // namespace mc::advancement
