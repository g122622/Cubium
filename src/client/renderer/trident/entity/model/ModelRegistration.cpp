/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, the subject to the following conditions:
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

/**
 * @file ModelRegistration.cpp
 * @brief 统一注册所有实体模型到 ModelFactory
 *
 * 此文件集中注册所有实体模型，使用工厂注册表模式替代巨型 if-else 链。
 */

#include "ModelRegistration.hpp"
#include "core/ModelFactory.hpp"
#include <spdlog/spdlog.h>

// 动物模型
#include "animal/BatModel.hpp"
#include "animal/CatModel.hpp"
#include "animal/ChickenModel.hpp"
#include "animal/CowModel.hpp"
#include "animal/HorseModel.hpp"
#include "animal/LlamaModel.hpp"
#include "animal/OcelotModel.hpp"
#include "animal/PigModel.hpp"
#include "animal/PolarBearModel.hpp"
#include "animal/RabbitModel.hpp"
#include "animal/SheepModel.hpp"
#include "animal/SquidModel.hpp"
#include "animal/VillagerModel.hpp"
#include "animal/WolfModel.hpp"

// 水生生物模型
#include "aquatic/AquaticModels.hpp"
#include "aquatic/PufferfishModel.hpp"

// 基础模型
#include "base/BipedModel.hpp"
#include "base/QuadrupedModel.hpp"

// 怪物模型
#include "monster/BlazeModel.hpp"
#include "monster/CreeperModel.hpp"
#include "monster/EndermanModel.hpp"
#include "monster/MonsterVariantModels.hpp"
#include "monster/MoreMonsterModels.hpp"
#include "monster/SkeletonModel.hpp"
#include "monster/SpecialMonsterModels.hpp"
#include "monster/SpiderModel.hpp"
#include "monster/WitchModel.hpp"
#include "monster/ZombieModel.hpp"

// 下界生物模型
#include "nether/NetherModels.hpp"

// 玩家模型
#include "player/PlayerModel.hpp"

// 实体类型常量
#include "common/entity/core/EntityRegistry.hpp"

namespace mc::client::renderer::entity::model {

namespace ET = ::mc::entity::EntityTypeKeys;

void initializeModelRegistration()
{
    if (ModelFactory::isInitialized()) {
        spdlog::warn("ModelFactory already initialized, skipping model registration");
        return;
    }

    auto& factory = ModelFactory::instance();

    // ==================== 基础动物 ====================
    factory.registerModel(ET::PIG, []() { return std::make_unique<animal::PigModel>(); });
    factory.registerModel(ET::COW, []() { return std::make_unique<animal::CowModel>(); });
    factory.registerModel(ET::SHEEP, []() { return std::make_unique<animal::SheepModel>(); });
    factory.registerModel(ET::CHICKEN, []() { return std::make_unique<animal::ChickenModel>(); });
    factory.registerModel(ET::WOLF, []() { return std::make_unique<animal::WolfModel>(); });
    factory.registerModel(ET::OCELOT, []() { return std::make_unique<animal::OcelotModel>(0.0f); });
    factory.registerModel(ET::CAT, []() { return std::make_unique<animal::CatModel>(0.0f); });
    factory.registerModel(ET::HORSE, []() { return std::make_unique<animal::HorseModel>(0.0f); });
    factory.registerModel(ET::VILLAGER, []() { return std::make_unique<animal::VillagerModel>(0.0f); });
    factory.registerModel(ET::WANDERING_TRADER, []() { return std::make_unique<animal::VillagerModel>(0.0f); });

    // 动物 - 已有独立文件
    factory.registerModel(ET::RABBIT, []() { return std::make_unique<animal::RabbitModel>(); });
    factory.registerModel(ET::BAT, []() { return std::make_unique<animal::BatModel>(); });
    factory.registerModel(ET::SQUID, []() { return std::make_unique<animal::SquidModel>(); });
    // 发光鱿鱼复用鱿鱼模型（MC Java 中 GlowSquidModel extends SquidModel，无额外部件）
    factory.registerModel(ET::GLOW_SQUID, []() { return std::make_unique<animal::SquidModel>(); });
    factory.registerModel(ET::LLAMA, []() { return std::make_unique<animal::LlamaModel>(0.0f); });
    factory.registerModel(ET::TRADER_LLAMA, []() { return std::make_unique<animal::LlamaModel>(0.0f); });

    // 动物 - 马类变体（复用 HorseModel）
    factory.registerModel(ET::DONKEY, []() { return std::make_unique<animal::HorseModel>(0.0f); });
    factory.registerModel(ET::MULE, []() { return std::make_unique<animal::HorseModel>(0.0f); });
    factory.registerModel(ET::SKELETON_HORSE, []() { return std::make_unique<animal::HorseModel>(0.0f); });
    factory.registerModel(ET::ZOMBIE_HORSE, []() { return std::make_unique<animal::HorseModel>(0.0f); });

    // 动物 - 哞菇（复用 CowModel）
    factory.registerModel(ET::MOOSHROOM, []() { return std::make_unique<animal::CowModel>(); });

    // ==================== 特殊动物 (MoreMonsterModels) ====================
    factory.registerModel(ET::FOX, []() { return std::make_unique<monster::FoxModel>(); });
    factory.registerModel(ET::PANDA, []() { return std::make_unique<monster::PandaModel>(); });
    factory.registerModel(ET::POLAR_BEAR, []() { return std::make_unique<animal::PolarBearModel>(); });
    factory.registerModel(ET::BEE, []() { return std::make_unique<monster::BeeModel>(); });
    factory.registerModel(ET::PARROT, []() { return std::make_unique<monster::ParrotModel>(); });
    factory.registerModel(ET::IRON_GOLEM, []() { return std::make_unique<monster::IronGolemModel>(); });
    factory.registerModel(ET::SNOW_GOLEM, []() { return std::make_unique<monster::SnowGolemModel>(); });
    factory.registerModel(ET::COPPER_GOLEM, []() { return std::make_unique<monster::CopperGolemModel>(); });

    // ==================== 水生生物 ====================
    factory.registerModel(ET::COD, []() { return std::make_unique<aquatic::CodModel>(); });
    factory.registerModel(ET::SALMON, []() { return std::make_unique<aquatic::SalmonModel>(); });
    factory.registerModel(ET::DOLPHIN, []() { return std::make_unique<aquatic::DolphinModel>(); });
    factory.registerModel(ET::TURTLE, []() { return std::make_unique<aquatic::TurtleModel>(); });
    factory.registerModel(ET::TROPICAL_FISH, []() { return std::make_unique<aquatic::TropicalFishAModel>(); });

    // 河豚 - 默认使用小型模型，PufferfishRenderer 会根据膨胀状态切换
    factory.registerModel(ET::PUFFERFISH, []() { return std::make_unique<aquatic::PufferfishSmallModel>(); });

    // ==================== 怪物 ====================
    factory.registerModel(ET::ZOMBIE, []() { return std::make_unique<monster::ZombieModel>(); });
    factory.registerModel(ET::SKELETON, []() { return std::make_unique<monster::SkeletonModel>(); });
    factory.registerModel(ET::CREEPER, []() { return std::make_unique<monster::CreeperModel>(); });
    factory.registerModel(ET::SPIDER, []() { return std::make_unique<monster::SpiderModel>(); });
    factory.registerModel(ET::ENDERMAN, []() { return std::make_unique<monster::EndermanModel>(); });
    factory.registerModel(ET::BLAZE, []() { return std::make_unique<monster::BlazeModel>(); });

    // 怪物变体 (MonsterVariantModels)
    factory.registerModel(ET::ZOMBIE_VILLAGER, []() { return std::make_unique<monster::ZombieVillagerModel>(); });
    factory.registerModel(ET::DROWNED, []() { return std::make_unique<monster::DrownedModel>(); });
    factory.registerModel(ET::HUSK, []() { return std::make_unique<monster::HuskModel>(); });
    factory.registerModel(ET::STRAY, []() { return std::make_unique<monster::StrayModel>(); });
    factory.registerModel(ET::CAVE_SPIDER, []() { return std::make_unique<monster::CaveSpiderModel>(); });
    factory.registerModel(ET::GIANT, []() { return std::make_unique<monster::GiantModel>(); });

    // 特殊怪物 (SpecialMonsterModels)
    factory.registerModel(ET::WITHER, []() { return std::make_unique<monster::WitherModel>(); });
    factory.registerModel(ET::SLIME, []() { return std::make_unique<monster::SlimeModel>(); });
    factory.registerModel(ET::GUARDIAN, []() { return std::make_unique<monster::GuardianModel>(); });
    factory.registerModel(ET::ELDER_GUARDIAN, []() { return std::make_unique<monster::ElderGuardianModel>(); });
    factory.registerModel(ET::SHULKER, []() { return std::make_unique<monster::ShulkerModel>(); });
    factory.registerModel(ET::SILVERFISH, []() { return std::make_unique<monster::SilverfishModel>(); });
    factory.registerModel(ET::ENDERMITE, []() { return std::make_unique<monster::EndermiteModel>(); });

    // 更多怪物 (MoreMonsterModels)
    factory.registerModel(ET::PHANTOM, []() { return std::make_unique<monster::PhantomModel>(); });
    factory.registerModel(ET::WITHER_SKELETON, []() { return std::make_unique<monster::SkeletonModel>(); });
    // 沼骸骨（bogged）与普通骷髅结构相同，复用 SkeletonModel。
    // 拉弓状态通过 AbstractSkeletonEntity::DATA_CHARGING_BOW_PARAM 同步到
    // ClientEntity::isChargingBow()，由 EntityRendererManager::_applySkeletonArmPose
    // 设置右臂 BowAndArrow 姿态。
    factory.registerModel(ET::BOGGED, []() { return std::make_unique<monster::SkeletonModel>(); });

    // 灾厄村民 (MoreMonsterModels)
    factory.registerModel(ET::VEX, []() { return std::make_unique<monster::VexModel>(); });
    factory.registerModel(ET::VINDICATOR, []() { return std::make_unique<monster::IllagerModel>(); });
    factory.registerModel(ET::EVOKER, []() { return std::make_unique<monster::IllagerModel>(); });
    factory.registerModel(ET::PILLAGER, []() { return std::make_unique<monster::IllagerModel>(); });
    factory.registerModel(ET::ILLUSIONER, []() { return std::make_unique<monster::IllagerModel>(); });
    factory.registerModel(ET::RAVAGER, []() { return std::make_unique<monster::RavagerModel>(); });

    // 女巫（使用 WitchModel，继承自 VillagerModel，包含帽子、鼻子等独特部件）
    factory.registerModel(ET::WITCH, []() { return std::make_unique<monster::WitchModel>(); });

    // ==================== 下界生物 ====================
    factory.registerModel(ET::GHAST, []() { return std::make_unique<nether::GhastModel>(); });
    factory.registerModel(ET::MAGMA_CUBE, []() { return std::make_unique<nether::MagmaCubeModel>(); });
    factory.registerModel(ET::PIGLIN, []() { return std::make_unique<nether::PiglinModel>(); });
    factory.registerModel(ET::PIGLIN_BRUTE, []() { return std::make_unique<nether::PiglinModel>(); });
    factory.registerModel(ET::HOGLIN, []() { return std::make_unique<nether::BoarModel>(); });
    factory.registerModel(ET::ZOGLIN, []() { return std::make_unique<nether::BoarModel>(); });
    factory.registerModel(ET::ZOMBIFIED_PIGLIN, []() { return std::make_unique<nether::PiglinModel>(); });
    factory.registerModel(ET::STRIDER, []() { return std::make_unique<nether::StriderModel>(); });

    // ==================== 玩家 ====================
    factory.registerModel(ET::PLAYER, []() { return std::make_unique<player::PlayerModel>(0.0f, false); });

    ModelFactory::markInitialized();
    spdlog::info("ModelFactory: Registered {} model types", factory.size());
}

} // namespace mc::client::renderer::entity::model
