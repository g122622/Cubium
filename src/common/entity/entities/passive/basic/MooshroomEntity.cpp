#include "MooshroomEntity.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
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

std::vector<ItemStack> MooshroomEntity::shear(Player* /*player*/) {
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
    // MC 1.16.5: MooshroomEntity.func_241841_a() (onStruckByLightning)
    // 红色哞菇 -> 棕色哞菇
    // 棕色哞菇 -> 红色哞菇
    // 播放 convert 音效并生成粒子

    // 切换类型
    MooshroomType newType = isRed() ? MooshroomType::Brown : MooshroomType::Red;
    setMooshroomType(newType);

    // 播放转换音效
    playSound(SoundEvents::ENTITY_MOOSHROOM_CONVERT, 2.0f, 1.0f);

    // MC 1.16.5: 生成爆炸粒子效果
    // 在客户端生成粒子
    if (world() != nullptr && world()->isClientSide()) {
        using namespace mc::client::renderer::trident::particle;
        math::Random& random = world()->getRandom();

        // 生成多个爆炸粒子
        i32 particleCount = 20;
        for (i32 i = 0; i < particleCount; ++i) {
            f32 offsetX = (random.nextFloat() - 0.5f) * width();
            f32 offsetY = random.nextFloat() * height();
            f32 offsetZ = (random.nextFloat() - 0.5f) * width();

            world()->addParticle(
                ParticleTypeId::Explosion,
                Vector3(x() + offsetX, y() + offsetY, z() + offsetZ),
                Vector3(0.0, 0.0, 0.0)
            );
        }
    }
}

void MooshroomEntity::registerGoals() {
    // 调用父类方法（牛的行为）
    CowEntity::registerGoals();

    // 哞菇没有额外行为，完全继承牛的行为
}

} // namespace mc
