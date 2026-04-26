#include <gtest/gtest.h>

#include "common/entity/ai/goal/goals/TemptGoal.hpp"
#include "common/entity/ai/goal/goals/PanicGoal.hpp"
#include "common/entity/ai/goal/goals/attack/RangedAttackGoals.hpp"
#include "common/entity/ai/goal/goals/movement/MovementGoals.hpp"
#include "common/entity/core/CreatureEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/IWorld.hpp"
#include "common/core/Constants.hpp"

#include <algorithm>
#include <utility>

using namespace mc;
using namespace mc::entity::ai::goal;

namespace {

class TestGoalWorld final : public IWorld {
public:
    void setEntities(std::vector<Entity*> entities) {
        m_entities = std::move(entities);
    }

    void setAllWater(bool enabled) {
        m_allWater = enabled;
    }

    void setAllLava(bool enabled) {
        m_allLava = enabled;
    }

    void setWaterBlock(i32 x, i32 y, i32 z) {
        m_waterBlocks.push_back(BlockPos(x, y, z));
    }

    void setLavaBlock(i32 x, i32 y, i32 z) {
        m_lavaBlocks.push_back(BlockPos(x, y, z));
    }

    [[nodiscard]] const BlockState* getBlockState(i32, i32, i32) const override { return nullptr; }
    bool setBlock(i32, i32, i32, const BlockState*) override { return false; }
    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override { return nullptr; }
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 0; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override { return y >= mc::world::MIN_BUILD_HEIGHT && y < mc::world::MAX_BUILD_HEIGHT; }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity* except) const override {
        std::vector<Entity*> result;
        for (Entity* entity : m_entities) {
            if (entity == except) {
                continue;
            }
            result.push_back(entity);
        }
        return result;
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3& pos, f32 range, const Entity* except) const override {
        std::vector<Entity*> result;
        const f32 rangeSq = range * range;

        for (Entity* entity : m_entities) {
            if (entity == except) {
                continue;
            }

            if (pos.distanceSquared(entity->position()) <= rangeSq) {
                result.push_back(entity);
            }
        }

        return result;
    }

    [[nodiscard]] DimensionId dimension() const override { return 0; }
    [[nodiscard]] u64 seed() const override { return 0; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }

    [[nodiscard]] bool isWaterAt(const BlockPos& pos) const override {
        if (m_allWater) {
            return true;
        }

        return contains(m_waterBlocks, pos.x, pos.y, pos.z);
    }

    [[nodiscard]] bool isLavaAt(const BlockPos& pos) const override {
        if (m_allLava) {
            return true;
        }

        return contains(m_lavaBlocks, pos.x, pos.y, pos.z);
    }

private:
    [[nodiscard]] static bool contains(const std::vector<BlockPos>& blocks, i32 x, i32 y, i32 z) {
        const auto it = std::find_if(blocks.begin(), blocks.end(), [x, y, z](const BlockPos& pos) {
            return pos.x == x && pos.y == y && pos.z == z;
        });
        return it != blocks.end();
    }

    bool m_allWater = false;
    bool m_allLava = false;
    std::vector<Entity*> m_entities;
    std::vector<BlockPos> m_waterBlocks;
    std::vector<BlockPos> m_lavaBlocks;
};

class TestCreatureEntity final : public CreatureEntity {
public:
    TestCreatureEntity()
        : CreatureEntity(LegacyEntityType::Cow, 1) {
        registerAttributes();
        setHealth(maxHealth());
    }
};

class TestTemptItem final : public Item {
public:
    TestTemptItem() : Item(ItemProperties().maxStackSize(64)) {}
};

class TestPlainItem final : public Item {
public:
    TestPlainItem() : Item(ItemProperties().maxStackSize(64)) {}
};

class TestBowItem final : public Item {
public:
    TestBowItem() : Item(ItemProperties().maxStackSize(1)) {}

    [[nodiscard]] UseAction getUseAction(const ItemStack&) const override {
        return UseAction::Bow;
    }
};

class ExposedPanicGoal final : public PanicGoal {
public:
    using PanicGoal::PanicGoal;
    using PanicGoal::getRandomWaterPosition;
};

class ExposedWaterAvoidingRandomWalkingGoal final : public WaterAvoidingRandomWalkingGoal {
public:
    using WaterAvoidingRandomWalkingGoal::WaterAvoidingRandomWalkingGoal;
    using WaterAvoidingRandomWalkingGoal::isInWaterOrLava;
};

} // namespace

TEST(AiGoalRegressionTest, TemptGoal_UsesTemptingPlayerHandItems) {
    TestGoalWorld world;
    TestCreatureEntity creature;
    creature.setWorld(&world);
    creature.setPosition(0.0f, 64.0f, 0.0f);

    TestTemptItem temptItem;
    TestPlainItem nonTemptItem;

    Player closePlayer(2, "ClosePlayer");
    closePlayer.setWorld(&world);
    closePlayer.setPosition(2.0f, 64.0f, 0.0f);
    closePlayer.setHealth(closePlayer.maxHealth());
    closePlayer.getHeldItem(Hand::MainHand) = ItemStack(&nonTemptItem, 1);
    closePlayer.getHeldItem(Hand::OffHand) = ItemStack(&nonTemptItem, 1);

    Player temptingPlayer(3, "TemptingPlayer");
    temptingPlayer.setWorld(&world);
    temptingPlayer.setPosition(6.0f, 64.0f, 0.0f);
    temptingPlayer.setHealth(temptingPlayer.maxHealth());
    temptingPlayer.getHeldItem(Hand::MainHand) = ItemStack(&nonTemptItem, 1);
    temptingPlayer.getHeldItem(Hand::OffHand) = ItemStack(&temptItem, 1);

    world.setEntities({&closePlayer, &temptingPlayer});

    TemptGoal goal(&creature, 1.0, [&temptItem](const ItemStack& stack) {
        return stack.getItem() == &temptItem;
    });

    EXPECT_TRUE(goal.shouldExecute());
    goal.startExecuting();
    EXPECT_TRUE(goal.shouldContinueExecuting());

    temptingPlayer.getHeldItem(Hand::OffHand) = ItemStack(&nonTemptItem, 1);
    EXPECT_FALSE(goal.shouldContinueExecuting());
}

TEST(AiGoalRegressionTest, PanicGoal_FindsNearbyWaterWhenBurning) {
    TestGoalWorld world;
    world.setAllWater(true);
    TestCreatureEntity creature;
    creature.setWorld(&world);
    creature.setPosition(10.0f, 64.0f, 10.0f);
    creature.setFire(40);

    ExposedPanicGoal goal(&creature, 1.0);

    EXPECT_TRUE(goal.shouldExecute());
    const BlockPos waterPos = goal.getRandomWaterPosition(8, 4);
    EXPECT_NE(waterPos.x, 0);
    EXPECT_NE(waterPos.y, 0);
    EXPECT_NE(waterPos.z, 0);
}

TEST(AiGoalRegressionTest, WaterAvoidingRandomWalkingGoal_DetectsWaterAndLava) {
    TestGoalWorld world;
    TestCreatureEntity creature;
    creature.setWorld(&world);
    creature.setPosition(0.0f, 64.0f, 0.0f);

    ExposedWaterAvoidingRandomWalkingGoal goal(&creature, 1.0, 1.0f);

    world.setWaterBlock(0, 64, 0);
    world.setLavaBlock(1, 64, 0);

    EXPECT_TRUE(goal.isInWaterOrLava(0.2, 64.1, 0.2));
    EXPECT_TRUE(goal.isInWaterOrLava(1.8, 64.0, 0.2));
    EXPECT_FALSE(goal.isInWaterOrLava(5.0, 64.0, 5.0));
    EXPECT_TRUE(goal.shouldExecute());
}

TEST(AiGoalRegressionTest, RangedBowAttackGoal_RequiresBowUseAction) {
    TestGoalWorld world;
    TestCreatureEntity creature;
    creature.setWorld(&world);
    creature.setPosition(0.0f, 64.0f, 0.0f);

    TestCreatureEntity target;
    target.setWorld(&world);
    target.setPosition(10.0f, 64.0f, 0.0f);

    TestBowItem bowItem;
    TestPlainItem plainItem;

    creature.setMainHandItem(ItemStack(&plainItem, 1));
    creature.setAttackTarget(&target);

    RangedBowAttackGoal goal(&creature, 1.0, 20, 40);
    EXPECT_FALSE(goal.shouldExecute());

    creature.setMainHandItem(ItemStack(&bowItem, 1));
    EXPECT_TRUE(goal.shouldExecute());
}
