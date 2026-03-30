#include "MooshroomEntity.hpp"
#include "../../../core/Types.hpp"
#include "../../../item/ItemStack.hpp"
#include "../../core/EntityRegistry.hpp"
#include <random>

namespace mc {

MooshroomEntity::MooshroomEntity(LegacyEntityType type, EntityId id)
    : CowEntity(type, id)
{
    // 默认红色哞菇
    // 注册 AI 目标（继承自 CowEntity）
    registerGoals();
}

std::unique_ptr<Entity> MooshroomEntity::create(IWorld* /*world*/) {
    return std::make_unique<MooshroomEntity>(LegacyEntityType::Unknown, 0);
}

std::vector<ItemStack> MooshroomEntity::shear() {
    // 剪毛后变成普通牛
    // TODO: 返回红色或棕色蘑菇
    // std::vector<ItemStack> drops;
    // if (isRed()) {
    //     drops.emplace_back(Items::RED_MUSHROOM, 5);
    // } else {
    //     drops.emplace_back(Items::BROWN_MUSHROOM, 5);
    // }
    // return drops;
    return {};
}

bool MooshroomEntity::canBeStewed(const ItemStack& itemStack) const {
    // TODO: 检查是否是空碗
    // return itemStack.getItem() == Items::BOWL;
    (void)itemStack;
    return false;
}

ItemStack MooshroomEntity::getStew() {
    // TODO: 返回蘑菇汤
    // if (isBrown() && m_effect) {
    //     // 棕色哞菇返回带效果的迷之炖菜
    //     return ItemStack(Items::SUSPICIOUS_STEW);
    // }
    // return ItemStack(Items::MUSHROOM_STEW);
    return ItemStack();
}

std::unique_ptr<AnimalEntity> MooshroomEntity::spawnBaby(AnimalEntity& partner) {
    // TODO: 创建小哞菇
    // auto baby = std::make_unique<MooshroomEntity>(LegacyEntityType::Unknown, 0);
    // baby->setChild(true);
    //
    // // 遗传父母的皮肤类型
    // static std::random_device rd;
    // static std::mt19937 gen(rd());
    // std::uniform_int_distribution<int> dist(0, 1);
    //
    // MooshroomEntity* parent = dynamic_cast<MooshroomEntity*>(&partner);
    // if (parent && isRed() && parent->isBrown()) {
    //     // 红色和棕色混合有极小概率产生棕色
    //     baby->setMooshroomType(dist(gen) == 0 ? MooshroomType::Brown : MooshroomType::Red);
    // } else {
    //     baby->setMooshroomType(m_mooshroomType);
    // }
    //
    // return baby;
    (void)partner;
    return nullptr;
}

void MooshroomEntity::onStruckByLightning() {
    // 红色哞菇被雷击后变成棕色哞菇
    if (isRed()) {
        setMooshroomType(MooshroomType::Brown);
        // TODO: 生成粒子效果
    }
}

void MooshroomEntity::registerGoals() {
    // 调用父类方法（牛的行为）
    CowEntity::registerGoals();

    // 哞菇没有额外行为，完全继承牛的行为
}

} // namespace mc
