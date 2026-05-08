#pragma once

#include "../Goal.hpp"
#include "../../../../core/Types.hpp"

namespace mc {

// 前向声明
class AnimalEntity;
class Player;

namespace entity::ai::goal {

/**
 * @brief 繁殖目标
 *
 * 使两只动物靠近并繁殖。
 *
 * 参考 MC 1.16.5 BreedGoal
 */
class BreedGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param animal 动物实体
     * @param speed 移动速度倍率
     */
    BreedGoal(AnimalEntity* animal, f64 speed);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "BreedGoal"; }

protected:
    /**
     * @brief 寻找附近的配偶
     * @return 附近的配偶，如果没有则返回 nullptr
     */
    [[nodiscard]] AnimalEntity* findNearbyMate();

    /**
     * @brief 生成幼体
     */
    void spawnBaby();

    AnimalEntity* m_animal;
    f64 m_speed;
    AnimalEntity* m_targetMate = nullptr;
    i32 m_spawnBabyDelay = 0;

    static constexpr i32 SPAWN_BABY_DELAY = 60; // 繁殖延迟（ticks）
    static constexpr f32 MATE_SEARCH_RANGE = 8.0f; // 配偶搜索范围
};

} // namespace entity::ai::goal
} // namespace mc
