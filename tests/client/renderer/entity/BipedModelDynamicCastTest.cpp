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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
 * OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/**
 * @file BipedModelDynamicCastTest.cpp
 * @brief dynamic_cast<BipedModel*> 安全性测试
 *
 * 验证 EntityRendererManager::_createModelForEntity 中的
 * `dynamic_cast<model::BipedModel*>(model.get())` 守卫：
 *
 *  - BipedModel 派生模型（PlayerModel 等）应成功转换
 *  - 非 BipedModel 派生模型应返回 nullptr，避免误触发 _applyBipedElytraState
 *
 * 非派生模型覆盖三类继承路径：
 *  - 直接继承 EntityModel：CreeperModel / SpiderModel / VillagerModel / WitchModel
 *  - 继承 AgeableModel：ChickenModel
 *  - 继承 QuadrupedModel（即 AgeableModel 的另一支）：PigModel / CowModel / SheepModel
 *  - QuadrupedModel 本身
 *
 * 由于测试目标不链接 EntityRendererManager.cpp（依赖 Vulkan），无法直接测试
 * `_applyBipedElytraState`；本测试通过模拟 `_createModelForEntity` 中的
 * `dynamic_cast` 守卫步骤，验证类型分发逻辑的安全性。
 */

#include <gtest/gtest.h>

#include "client/renderer/trident/entity/model/animal/AnimalModels.hpp"
#include "client/renderer/trident/entity/model/animal/VillagerModel.hpp"
#include "client/renderer/trident/entity/model/base/BipedModel.hpp"
#include "client/renderer/trident/entity/model/base/QuadrupedModel.hpp"
#include "client/renderer/trident/entity/model/monster/CreeperModel.hpp"
#include "client/renderer/trident/entity/model/monster/SpiderModel.hpp"
#include "client/renderer/trident/entity/model/monster/WitchModel.hpp"
#include "client/renderer/trident/entity/model/player/PlayerModel.hpp"

#include <memory>

using namespace mc::client::renderer::entity::model;
using namespace mc::client::renderer::entity::model::player;
using namespace mc::client::renderer::entity::model::animal;
using namespace mc::client::renderer::entity::model::monster;

namespace mc::client::renderer::entity::model {
namespace {

/**
 * @brief 模拟 EntityRendererManager::_createModelForEntity 中的 dynamic_cast 守卫
 *
 * 真实代码（EntityRendererManager.cpp:973-976）：
 *   auto* bipedModel = dynamic_cast<model::BipedModel*>(model.get());
 *   if (bipedModel != nullptr) {
 *       _applyBipedElytraState(*bipedModel, entity);
 *   }
 *
 * 此 helper 通过指针形式接收模型，返回 dynamic_cast 结果，
 * 不依赖 Vulkan/EntityRendererManager，便于在单元测试中验证类型分发安全性。
 */
bool isBipedModel(EntityModel* model) noexcept
{
    return dynamic_cast<BipedModel*>(model) != nullptr;
}

// ========== BipedModel 派生模型应成功转换 ==========

TEST(BipedModelDynamicCastTest, BipedModel_Itself_IsBiped)
{
    BipedModel model;
    EXPECT_TRUE(isBipedModel(&model));
}

TEST(BipedModelDynamicCastTest, PlayerModel_IsBiped)
{
    PlayerModel model(0.0f, false);
    EXPECT_TRUE(isBipedModel(&model));
}

// ========== 直接继承 EntityModel 的非 BipedModel 模型 ==========

TEST(BipedModelDynamicCastTest, CreeperModel_NotBiped)
{
    CreeperModel model;
    EXPECT_FALSE(isBipedModel(&model));
}

TEST(BipedModelDynamicCastTest, SpiderModel_NotBiped)
{
    SpiderModel model;
    EXPECT_FALSE(isBipedModel(&model));
}

TEST(BipedModelDynamicCastTest, VillagerModel_NotBiped)
{
    VillagerModel model(0.0f);
    EXPECT_FALSE(isBipedModel(&model));
}

// ========== 继承 VillagerModel 的非 BipedModel 模型 ==========

TEST(BipedModelDynamicCastTest, WitchModel_NotBiped)
{
    // WitchModel 继承 VillagerModel，VillagerModel 继承 EntityModel，不是 BipedModel 派生
    WitchModel model;
    EXPECT_FALSE(isBipedModel(&model));
}

// ========== 继承 AgeableModel 但非 BipedModel 的模型 ==========

TEST(BipedModelDynamicCastTest, ChickenModel_NotBiped)
{
    // ChickenModel 直接继承 AgeableModel，与 BipedModel 是兄弟关系
    ChickenModel model;
    EXPECT_FALSE(isBipedModel(&model));
}

// ========== QuadrupedModel 派生模型（与 BipedModel 是兄弟） ==========

TEST(BipedModelDynamicCastTest, QuadrupedModel_Itself_NotBiped)
{
    QuadrupedModel model;
    EXPECT_FALSE(isBipedModel(&model));
}

TEST(BipedModelDynamicCastTest, PigModel_NotBiped)
{
    PigModel model;
    EXPECT_FALSE(isBipedModel(&model));
}

TEST(BipedModelDynamicCastTest, CowModel_NotBiped)
{
    CowModel model;
    EXPECT_FALSE(isBipedModel(&model));
}

TEST(BipedModelDynamicCastTest, SheepModel_NotBiped)
{
    SheepModel model;
    EXPECT_FALSE(isBipedModel(&model));
}

// ========== 通过 EntityModel 父类指针访问时的 dynamic_cast 行为 ==========

TEST(BipedModelDynamicCastTest, PlayerModel_ThroughEntityModelBase_IsBiped)
{
    // 模拟 _createModelForEntity 的真实场景：模型由 ModelFactory 创建，
    // 返回 std::unique_ptr<EntityModel>，再 dynamic_cast 到 BipedModel*。
    std::unique_ptr<EntityModel> model = std::make_unique<PlayerModel>(0.0f, false);
    EXPECT_TRUE(isBipedModel(model.get()));
}

TEST(BipedModelDynamicCastTest, CreeperModel_ThroughEntityModelBase_NotBiped)
{
    std::unique_ptr<EntityModel> model = std::make_unique<CreeperModel>();
    EXPECT_FALSE(isBipedModel(model.get()));
}

TEST(BipedModelDynamicCastTest, PigModel_ThroughEntityModelBase_NotBiped)
{
    std::unique_ptr<EntityModel> model = std::make_unique<PigModel>();
    EXPECT_FALSE(isBipedModel(model.get()));
}

// ========== nullptr 守卫 ==========

TEST(BipedModelDynamicCastTest, Nullptr_ReturnsNullptr)
{
    // dynamic_cast<BipedModel*>(nullptr) 返回 nullptr，不会触发 _applyBipedElytraState
    EXPECT_FALSE(isBipedModel(nullptr));
}

} // namespace
} // namespace mc::client::renderer::entity::model
