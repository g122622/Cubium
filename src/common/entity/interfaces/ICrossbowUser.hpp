#pragma once

#include "common/core/Types.hpp"
#include "common/entity/interfaces/IRangedAttackMob.hpp"

namespace mc {
namespace entity {

// Forward declarations
class ItemStack;

/**
 * @brief 弩使用者接口 - 用于可以使用弩的实体
 *
 * 实现此接口的实体可以使用弩进行攻击。
 * 例如：掠夺者、猪灵等。
 *
 * 参考 MC 1.16.5 ICrossbowUser
 */
class ICrossbowUser : public IRangedAttackMob {
public:
    ~ICrossbowUser() override = default;

    /**
     * @brief 设置弩的装填状态
     * @param charging 是否正在装填
     */
    virtual void setChargingCrossbow(bool charging) = 0;

    /**
     * @brief 检查是否正在装填弩
     * @return 如果正在装填返回true
     */
    virtual bool isChargingCrossbow() const = 0;

    /**
     * @brief 当弩装填完成时调用
     * @param crossbow 弩物品
     */
    virtual void onCrossbowLoadComplete(ItemStack& crossbow) = 0;

    /**
     * @brief 发射弩箭
     * @param target 目标实体
     * @param crossbow 弩物品
     * @param charge 蓄力程度
     */
    virtual void shootCrossbow(LivingEntity* target, ItemStack& crossbow, f32 charge) = 0;

    /**
     * @brief 获取弩装填时间（ticks）
     * @return 装填时间
     */
    virtual i32 getCrossbowChargeTime() const { return 25; }
};

} // namespace entity
} // namespace mc
