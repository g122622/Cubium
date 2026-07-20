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

// 测试 Template::placeInWorld 中实体放置时的身体/头部朝向同步逻辑。
// 收敛 src/common/world/gen/feature/template/Template.cpp:1032 处的历史 TODO：
// 结构模板旋转/镜像生成的实体需要让 body/head rotation 跟随 finalYaw，
// 与 MC 1.21.11 StructureTemplate#placeEntities 中的
//   setYBodyRot(f); setYHeadRot(f);
// 行为对齐。

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/passive/basic/PigEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/feature/template/Template.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

using namespace mc;
using namespace mc::world::gen::feature::template_;
using mc::entity::serialization::nbt_helper::putFloatList;
using mc::entity::serialization::nbt_keys::ROTATION;

namespace mc {
namespace {

/// 捕获 spawnEntity 调用的测试世界
class EntityPlacementTestWorld final : public test::BaseTestWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        return state != nullptr ? state->getFluidState() : fluid::Fluid::getFluidState(0);
    }

    EntityId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityId>(m_spawnedEntities.size());
    }

    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override {}

    [[nodiscard]] Entity* lastSpawnedEntity() const
    {
        return m_spawnedEntities.empty() ? nullptr : m_spawnedEntities.back().get();
    }

    [[nodiscard]] size_t spawnedCount() const { return m_spawnedEntities.size(); }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
};

class TemplateEntityPlacementTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        entity::VanillaEntities::registerAll();
    }

    /// 构造一个带 1 个 pig 实体的最小模板（含一个石头方块以通过 placeInWorld 的调色板检查）
    Template makePigTemplate(f32 nbtYaw = 0.0f, f32 nbtPitch = 0.0f, bool withNbt = false)
    {
        Template templ;
        templ.setSize(BlockPos(1, 2, 1));

        // 添加一个调色板，含一个石头方块（避免 placeInWorld 因空调色板提前 return）
        std::vector<BlockInfo> blocks;
        blocks.emplace_back(BlockPos(0, 0, 0), VanillaBlocks::STONE->defaultState().stateId());
        templ.addPalette(Palette(std::move(blocks)));

        // 添加 pig 实体
        TemplateEntityInfo entityInfo;
        entityInfo.typeId = "minecraft:pig";
        entityInfo.posx = 0.5;
        entityInfo.posy = 1.0;
        entityInfo.posz = 0.5;
        entityInfo.blockPos = BlockPos(0, 1, 0);

        if (withNbt) {
            auto nbt = std::make_unique<nbt::CompoundTag>();
            putFloatList(*nbt, ROTATION, {nbtYaw, nbtPitch});
            entityInfo.nbt = std::move(nbt);
        }

        templ.addEntity(entityInfo);
        return templ;
    }

    /// 计算 finalYaw，与 Template.cpp 内部逻辑一致
    /// finalYaw = wrapDegrees(rotate(yaw) + (mirror(yaw) - yaw))
    static f32 computeExpectedFinalYaw(f32 yaw, Mirror mirror, Rotation rotation)
    {
        f32 wrapped = math::wrapDegrees(yaw);

        f32 mirrored = wrapped;
        switch (mirror) {
            case Mirror::FrontBack:
                mirrored = -wrapped;
                break;
            case Mirror::LeftRight:
                mirrored = 180.0f - wrapped;
                break;
            default:
                break;
        }

        f32 rotated = wrapped;
        switch (rotation) {
            case Rotation::Clockwise90:
                rotated = wrapped + 90.0f;
                break;
            case Rotation::Clockwise180:
                rotated = wrapped + 180.0f;
                break;
            case Rotation::CounterClockwise90:
                rotated = wrapped + 270.0f;
                break;
            default:
                break;
        }

        return math::wrapDegrees(rotated + (mirrored - wrapped));
    }
};

// ============================================================================
// 直接验证 setYBodyRot / setYHeadRot 接口
// ============================================================================

TEST_F(TemplateEntityPlacementTest, EntityBase_SetYBodyRot_DefaultNoOp)
{
    // Entity 基类的 setYBodyRot/setYHeadRot 默认为空实现，但 LivingEntity 重写后会写入字段。
    // 通过 Entity* 基类指针调用应正确分发到 LivingEntity 重写。
    PigEntity pig(EntityId(1));
    Entity* basePtr = &pig;

    basePtr->setYBodyRot(45.0f);
    basePtr->setYHeadRot(60.0f);

    // PigEntity 是 LivingEntity，重写后应写入字段
    EXPECT_FLOAT_EQ(pig.renderYawOffset(), 45.0f);
    EXPECT_FLOAT_EQ(pig.rotationYawHead(), 60.0f);
}

TEST_F(TemplateEntityPlacementTest, LivingEntity_SetYBodyRot_WritesRenderYawOffset)
{
    PigEntity pig(EntityId(1));
    LivingEntity* living = &pig;

    living->setYBodyRot(90.0f);
    living->setYHeadRot(180.0f);

    // setYBodyRot 应写入 m_renderYawOffset（与 MC setYBodyRot 写入 yBodyRot 等价）
    EXPECT_FLOAT_EQ(living->renderYawOffset(), 90.0f);
    // setYHeadRot 应写入 m_rotationYawHead（与 MC setYHeadRot 写入 yHeadRot 等价）
    EXPECT_FLOAT_EQ(living->rotationYawHead(), 180.0f);
}

// ============================================================================
// placeInWorld 实体放置：body/head rotation 同步
// ============================================================================

TEST_F(TemplateEntityPlacementTest, PlaceInWorld_NoRotation_SyncsBodyHeadToYaw)
{
    // 默认朝向（无旋转/镜像）下，结构放置的 pig 身体与头部朝向应等于 finalYaw（=0）
    auto templ = makePigTemplate();
    EntityPlacementTestWorld world;

    PlacementSettings settings;
    settings.setRotation(Rotation::None);
    settings.setMirror(Mirror::None);

    math::Random rng(42);
    templ.placeInWorld(world, BlockPos(0, 0, 0), settings, rng);

    ASSERT_EQ(world.spawnedCount(), 1u);
    auto* spawned = world.lastSpawnedEntity();
    ASSERT_NE(spawned, nullptr);

    // finalYaw 应为 0（默认朝向）
    EXPECT_FLOAT_EQ(spawned->yaw(), 0.0f);

    auto* living = dynamic_cast<LivingEntity*>(spawned);
    ASSERT_NE(living, nullptr);
    EXPECT_FLOAT_EQ(living->renderYawOffset(), 0.0f);
    EXPECT_FLOAT_EQ(living->rotationYawHead(), 0.0f);
}

TEST_F(TemplateEntityPlacementTest, PlaceInWorld_Rotation90_SyncsBodyHeadToFinalYaw)
{
    // Clockwise90 旋转：pig 默认 yaw=0，finalYaw 应为 90，body/head 应同步到 90
    auto templ = makePigTemplate();
    EntityPlacementTestWorld world;

    PlacementSettings settings;
    settings.setRotation(Rotation::Clockwise90);

    math::Random rng(42);
    templ.placeInWorld(world, BlockPos(0, 0, 0), settings, rng);

    ASSERT_EQ(world.spawnedCount(), 1u);
    auto* spawned = world.lastSpawnedEntity();
    ASSERT_NE(spawned, nullptr);

    const f32 expectedYaw = computeExpectedFinalYaw(0.0f, Mirror::None, Rotation::Clockwise90);
    EXPECT_FLOAT_EQ(spawned->yaw(), expectedYaw);

    auto* living = dynamic_cast<LivingEntity*>(spawned);
    ASSERT_NE(living, nullptr);
    EXPECT_FLOAT_EQ(living->renderYawOffset(), expectedYaw)
        << "body rotation (renderYawOffset) should be synced to finalYaw";
    EXPECT_FLOAT_EQ(living->rotationYawHead(), expectedYaw)
        << "head rotation (rotationYawHead) should be synced to finalYaw";
}

TEST_F(TemplateEntityPlacementTest, PlaceInWorld_Rotation180_SyncsBodyHeadToFinalYaw)
{
    auto templ = makePigTemplate();
    EntityPlacementTestWorld world;

    PlacementSettings settings;
    settings.setRotation(Rotation::Clockwise180);

    math::Random rng(42);
    templ.placeInWorld(world, BlockPos(0, 0, 0), settings, rng);

    ASSERT_EQ(world.spawnedCount(), 1u);
    auto* spawned = world.lastSpawnedEntity();
    ASSERT_NE(spawned, nullptr);

    const f32 expectedYaw = computeExpectedFinalYaw(0.0f, Mirror::None, Rotation::Clockwise180);
    EXPECT_FLOAT_EQ(spawned->yaw(), expectedYaw);

    auto* living = dynamic_cast<LivingEntity*>(spawned);
    ASSERT_NE(living, nullptr);
    EXPECT_FLOAT_EQ(living->renderYawOffset(), expectedYaw);
    EXPECT_FLOAT_EQ(living->rotationYawHead(), expectedYaw);
}

TEST_F(TemplateEntityPlacementTest, PlaceInWorld_Rotation270_SyncsBodyHeadToFinalYaw)
{
    auto templ = makePigTemplate();
    EntityPlacementTestWorld world;

    PlacementSettings settings;
    settings.setRotation(Rotation::CounterClockwise90);

    math::Random rng(42);
    templ.placeInWorld(world, BlockPos(0, 0, 0), settings, rng);

    ASSERT_EQ(world.spawnedCount(), 1u);
    auto* spawned = world.lastSpawnedEntity();
    ASSERT_NE(spawned, nullptr);

    const f32 expectedYaw = computeExpectedFinalYaw(0.0f, Mirror::None, Rotation::CounterClockwise90);
    EXPECT_FLOAT_EQ(spawned->yaw(), expectedYaw);

    auto* living = dynamic_cast<LivingEntity*>(spawned);
    ASSERT_NE(living, nullptr);
    EXPECT_FLOAT_EQ(living->renderYawOffset(), expectedYaw);
    EXPECT_FLOAT_EQ(living->rotationYawHead(), expectedYaw);
}

TEST_F(TemplateEntityPlacementTest, PlaceInWorld_MirrorFrontBack_SyncsBodyHeadToFinalYaw)
{
    // FrontBack 镜像：yaw=0 -> mirror=-0=0 -> finalYaw = 0 + (0 - 0) = 0
    auto templ = makePigTemplate();
    EntityPlacementTestWorld world;

    PlacementSettings settings;
    settings.setMirror(Mirror::FrontBack);

    math::Random rng(42);
    templ.placeInWorld(world, BlockPos(0, 0, 0), settings, rng);

    ASSERT_EQ(world.spawnedCount(), 1u);
    auto* spawned = world.lastSpawnedEntity();
    ASSERT_NE(spawned, nullptr);

    const f32 expectedYaw = computeExpectedFinalYaw(0.0f, Mirror::FrontBack, Rotation::None);
    EXPECT_FLOAT_EQ(spawned->yaw(), expectedYaw);

    auto* living = dynamic_cast<LivingEntity*>(spawned);
    ASSERT_NE(living, nullptr);
    EXPECT_FLOAT_EQ(living->renderYawOffset(), expectedYaw);
    EXPECT_FLOAT_EQ(living->rotationYawHead(), expectedYaw);
}

TEST_F(TemplateEntityPlacementTest, PlaceInWorld_MirrorLeftRight_SyncsBodyHeadToFinalYaw)
{
    // LeftRight 镜像：yaw=0 -> mirror=180 -> finalYaw = 0 + (180 - 0) = 180
    auto templ = makePigTemplate();
    EntityPlacementTestWorld world;

    PlacementSettings settings;
    settings.setMirror(Mirror::LeftRight);

    math::Random rng(42);
    templ.placeInWorld(world, BlockPos(0, 0, 0), settings, rng);

    ASSERT_EQ(world.spawnedCount(), 1u);
    auto* spawned = world.lastSpawnedEntity();
    ASSERT_NE(spawned, nullptr);

    const f32 expectedYaw = computeExpectedFinalYaw(0.0f, Mirror::LeftRight, Rotation::None);
    EXPECT_FLOAT_EQ(spawned->yaw(), expectedYaw);

    auto* living = dynamic_cast<LivingEntity*>(spawned);
    ASSERT_NE(living, nullptr);
    EXPECT_FLOAT_EQ(living->renderYawOffset(), expectedYaw);
    EXPECT_FLOAT_EQ(living->rotationYawHead(), expectedYaw);
}

TEST_F(TemplateEntityPlacementTest, PlaceInWorld_RotationAndMirror_Combined_SyncsBodyHead)
{
    // 组合：Clockwise90 + LeftRight
    auto templ = makePigTemplate();
    EntityPlacementTestWorld world;

    PlacementSettings settings;
    settings.setRotation(Rotation::Clockwise90);
    settings.setMirror(Mirror::LeftRight);

    math::Random rng(42);
    templ.placeInWorld(world, BlockPos(0, 0, 0), settings, rng);

    ASSERT_EQ(world.spawnedCount(), 1u);
    auto* spawned = world.lastSpawnedEntity();
    ASSERT_NE(spawned, nullptr);

    const f32 expectedYaw = computeExpectedFinalYaw(0.0f, Mirror::LeftRight, Rotation::Clockwise90);
    EXPECT_FLOAT_EQ(spawned->yaw(), expectedYaw);

    auto* living = dynamic_cast<LivingEntity*>(spawned);
    ASSERT_NE(living, nullptr);
    EXPECT_FLOAT_EQ(living->renderYawOffset(), expectedYaw);
    EXPECT_FLOAT_EQ(living->rotationYawHead(), expectedYaw);
}

TEST_F(TemplateEntityPlacementTest, PlaceInWorld_NonZeroNbtYaw_SyncsBodyHeadToTransformedYaw)
{
    // 模板中实体的 NBT 自带非零 Rotation[0]（yaw=45），
    // placeInWorld 应基于此 yaw 计算 finalYaw 并同步 body/head
    auto templ = makePigTemplate(45.0f, 0.0f, /*withNbt=*/true);
    EntityPlacementTestWorld world;

    PlacementSettings settings;
    settings.setRotation(Rotation::Clockwise90);

    math::Random rng(42);
    templ.placeInWorld(world, BlockPos(0, 0, 0), settings, rng);

    ASSERT_EQ(world.spawnedCount(), 1u);
    auto* spawned = world.lastSpawnedEntity();
    ASSERT_NE(spawned, nullptr);

    // 期望：yaw=45，rotatedYaw=45+90=135，mirroredYaw=45，finalYaw=wrapDegrees(135+(45-45))=135
    const f32 expectedYaw = computeExpectedFinalYaw(45.0f, Mirror::None, Rotation::Clockwise90);
    EXPECT_NEAR(spawned->yaw(), expectedYaw, 0.01f);

    auto* living = dynamic_cast<LivingEntity*>(spawned);
    ASSERT_NE(living, nullptr);
    EXPECT_NEAR(living->renderYawOffset(), expectedYaw, 0.01f);
    EXPECT_NEAR(living->rotationYawHead(), expectedYaw, 0.01f);
}

TEST_F(TemplateEntityPlacementTest, PlaceInWorld_ReadsNbtRotation_BodyHeadSyncedToNbtYaw)
{
    // 当 settings 无旋转/镜像时，body/head 应等于 NBT 中的 yaw
    // （readFromNBT 同步 body/head 到 yaw，placeInWorld 后续覆盖为 finalYaw=yaw）
    auto templ = makePigTemplate(90.0f, 0.0f, /*withNbt=*/true);
    EntityPlacementTestWorld world;

    PlacementSettings settings; // 默认无旋转/镜像

    math::Random rng(42);
    templ.placeInWorld(world, BlockPos(0, 0, 0), settings, rng);

    ASSERT_EQ(world.spawnedCount(), 1u);
    auto* spawned = world.lastSpawnedEntity();
    ASSERT_NE(spawned, nullptr);

    // finalYaw = wrapDegrees(90 + (90 - 90)) = 90
    EXPECT_NEAR(spawned->yaw(), 90.0f, 0.01f);

    auto* living = dynamic_cast<LivingEntity*>(spawned);
    ASSERT_NE(living, nullptr);
    EXPECT_NEAR(living->renderYawOffset(), 90.0f, 0.01f);
    EXPECT_NEAR(living->rotationYawHead(), 90.0f, 0.01f);
}

// ============================================================================
// readFromNBT 同步 body/head rotation 到 yaw
// ============================================================================

TEST_F(TemplateEntityPlacementTest, ReadFromNbt_SyncsBodyHeadToYaw)
{
    // 验证 Entity::readFromNBT 加载 Rotation 后会同步 body/head rotation 到 yaw
    // 对齐 MC 1.21.11 Entity#load 中的 setYHeadRot(getYRot()) / setYBodyRot(getYRot())
    PigEntity pig(EntityId(1));

    // 构造 NBT：Rotation=[123.0, 0.0]
    nbt::CompoundTag tag;
    putFloatList(tag, ROTATION, {123.0f, 0.0f});

    auto result = pig.readFromNBT(tag);
    EXPECT_FALSE(result.failed());

    // yaw 应为 123
    EXPECT_NEAR(pig.yaw(), 123.0f, 0.01f);

    // body/head rotation 应被同步到 yaw
    EXPECT_NEAR(pig.renderYawOffset(), 123.0f, 0.01f) << "readFromNBT 应同步 body rotation (renderYawOffset) 到 yaw";
    EXPECT_NEAR(pig.rotationYawHead(), 123.0f, 0.01f) << "readFromNBT 应同步 head rotation (rotationYawHead) 到 yaw";
}

} // namespace
} // namespace mc
