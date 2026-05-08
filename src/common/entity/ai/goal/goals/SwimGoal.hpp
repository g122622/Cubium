#pragma once

#include "../Goal.hpp"
#include "../../../../core/Types.hpp"

namespace mc {

// 前向声明
class MobEntity;

namespace entity::ai::goal {

/**
 * @brief 游泳目标
 *
 * 当实体在水中或岩浆中时，尝试向上游动。
 *
 * 参考 MC 1.16.5 SwimGoal
 */
class SwimGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param mob 拥有此目标的生物
     */
    explicit SwimGoal(MobEntity* mob);

    [[nodiscard]] bool shouldExecute() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "SwimGoal"; }

private:
    MobEntity* m_mob;
};

} // namespace entity::ai::goal
} // namespace mc
