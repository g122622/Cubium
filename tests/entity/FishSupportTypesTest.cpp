#include <gtest/gtest.h>

#include "common/entity/entities/passive/fish/AbstractGroupFishEntity.hpp"
#include "common/entity/entities/passive/fish/CodEntity.hpp"
#include "common/entity/entities/passive/fish/PufferfishEntity.hpp"
#include "common/entity/entities/passive/fish/SalmonEntity.hpp"
#include "common/entity/entities/passive/fish/TropicalFishEntity.hpp"
#include "common/entity/ai/goal/goals/movement/FollowSchoolLeaderGoal.hpp"
#include "common/world/IWorld.hpp"
#include "common/util/math/random/Random.hpp"

namespace mc {
namespace {

// ============================================================================
// Test World for FollowSchoolLeaderGoal
// ============================================================================

class TestFishWorld final : public IWorld {
public:
    void setEntities(std::vector<Entity*> entities) {
        m_entities = std::move(entities);
    }

    [[nodiscard]] const BlockState* getBlockState(i32, i32, i32) const override { return nullptr; }
    bool setBlockState(i32, i32, i32, const BlockState*) override { return false; }
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
    [[nodiscard]] bool isClientSide() override { return false; }
    [[nodiscard]] bool isWaterAt(const BlockPos&) const override { return true; }
    [[nodiscard]] bool isLavaAt(const BlockPos&) const override { return false; }

    [[nodiscard]] world::tick::TickManager& tickManager() override {
        throw std::runtime_error("TestFishWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override {
        throw std::runtime_error("TestFishWorld::tickManager not implemented");
    }

    [[nodiscard]] math::Random& getRandom() override {
        return m_random;
    }
    [[nodiscard]] const math::Random& getRandom() const override {
        return m_random;
    }

private:
    std::vector<Entity*> m_entities;
    mutable math::Random m_random{12345};  // 固定种子用于测试
};

TEST(AbstractGroupFishEntityTest, FollowerJoinAndLeaveUpdatesLeaderState)
{
    CodEntity leader(LegacyEntityType::Cod, 1);
    SalmonEntity follower(LegacyEntityType::Salmon, 2);

    EXPECT_FALSE(leader.isGroupLeader());
    EXPECT_EQ(leader.getGroupSize(), 1);
    EXPECT_FALSE(follower.hasGroupLeader());

    follower.joinGroup(leader);

    EXPECT_TRUE(follower.hasGroupLeader());
    EXPECT_EQ(follower.getGroupLeader(), &leader);
    EXPECT_EQ(leader.getGroupSize(), 2);
    EXPECT_TRUE(leader.isGroupLeader());
    EXPECT_TRUE(leader.canGroupGrow());

    follower.leaveGroup();

    EXPECT_FALSE(follower.hasGroupLeader());
    EXPECT_EQ(follower.getGroupLeader(), nullptr);
    EXPECT_EQ(leader.getGroupSize(), 1);
    EXPECT_FALSE(leader.isGroupLeader());
}

TEST(AbstractGroupFishEntityTest, UsesVanillaLeaderRangeAndClearsDeadLeaderOnTick)
{
    CodEntity leader(LegacyEntityType::Cod, 1);
    TropicalFishEntity follower(LegacyEntityType::TropicalFish, 2);

    leader.setPosition(0.0f, 62.0f, 0.0f);
    follower.setPosition(11.0f, 62.0f, 0.0f);
    follower.joinGroup(leader);

    EXPECT_FLOAT_EQ(follower.getSchoolingRange(), 11.0f);
    EXPECT_TRUE(follower.inRangeOfGroupLeader());

    follower.setPosition(11.1f, 62.0f, 0.0f);
    EXPECT_FALSE(follower.inRangeOfGroupLeader());

    leader.remove();
    follower.tick();

    EXPECT_FALSE(follower.hasGroupLeader());
    EXPECT_EQ(follower.getGroupLeader(), nullptr);
}

TEST(FishSupportTypesTest, SchoolingFishUseGroupLayerButPufferfishDoesNot)
{
    CodEntity cod(LegacyEntityType::Cod, 1);
    SalmonEntity salmon(LegacyEntityType::Salmon, 2);
    TropicalFishEntity tropicalFish(LegacyEntityType::TropicalFish, 3);
    PufferfishEntity pufferfish(LegacyEntityType::Pufferfish, 4);

    EXPECT_NE(dynamic_cast<AbstractGroupFishEntity*>(&cod), nullptr);
    EXPECT_NE(dynamic_cast<AbstractGroupFishEntity*>(&salmon), nullptr);
    EXPECT_NE(dynamic_cast<AbstractGroupFishEntity*>(&tropicalFish), nullptr);
    EXPECT_EQ(dynamic_cast<AbstractGroupFishEntity*>(&pufferfish), nullptr);

    EXPECT_TRUE(cod.canSchool());
    EXPECT_TRUE(salmon.canSchool());
    EXPECT_TRUE(tropicalFish.canSchool());
    EXPECT_FALSE(pufferfish.canSchool());

    EXPECT_EQ(cod.getMaxGroupSize(), 8);
    EXPECT_EQ(salmon.getMaxGroupSize(), 5);
    EXPECT_EQ(tropicalFish.getMaxGroupSize(), 8);
}

// ============================================================================
// FollowSchoolLeaderGoal Tests
// ============================================================================

class FollowSchoolLeaderGoalTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_world = std::make_unique<TestFishWorld>();
    }

    void TearDown() override {
        m_world.reset();
    }

    std::unique_ptr<TestFishWorld> m_world;
};

TEST_F(FollowSchoolLeaderGoalTest, ShouldNotExecuteWhenIsGroupLeader) {
    // 创建两条鳕鱼
    auto leader = std::make_unique<CodEntity>(LegacyEntityType::Cod, 1);
    auto follower = std::make_unique<CodEntity>(LegacyEntityType::Cod, 2);

    leader->setWorld(m_world.get());
    follower->setWorld(m_world.get());

    // follower 加入 leader 的群体
    follower->joinGroup(*leader);

    // leader 是群体首领
    EXPECT_TRUE(leader->isGroupLeader());
    EXPECT_FALSE(leader->hasGroupLeader());

    // leader 不应该执行 FollowSchoolLeaderGoal
    entity::ai::goal::FollowSchoolLeaderGoal goal(leader.get());
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(FollowSchoolLeaderGoalTest, ShouldExecuteWhenHasGroupLeader) {
    auto leader = std::make_unique<CodEntity>(LegacyEntityType::Cod, 1);
    auto follower = std::make_unique<CodEntity>(LegacyEntityType::Cod, 2);

    leader->setWorld(m_world.get());
    follower->setWorld(m_world.get());

    leader->setPosition(0.0f, 62.0f, 0.0f);
    follower->setPosition(5.0f, 62.0f, 0.0f);

    // follower 加入群体
    follower->joinGroup(*leader);

    EXPECT_TRUE(follower->hasGroupLeader());
    EXPECT_EQ(follower->getGroupLeader(), leader.get());

    // follower 应该执行
    entity::ai::goal::FollowSchoolLeaderGoal goal(follower.get());
    EXPECT_TRUE(goal.shouldExecute());
}

TEST_F(FollowSchoolLeaderGoalTest, ShouldContinueExecutingWhenInRange) {
    auto leader = std::make_unique<CodEntity>(LegacyEntityType::Cod, 1);
    auto follower = std::make_unique<CodEntity>(LegacyEntityType::Cod, 2);

    leader->setWorld(m_world.get());
    follower->setWorld(m_world.get());

    // 设置位置在跟随范围内（默认 11 格）
    leader->setPosition(0.0f, 62.0f, 0.0f);
    follower->setPosition(5.0f, 62.0f, 0.0f);

    follower->joinGroup(*leader);

    entity::ai::goal::FollowSchoolLeaderGoal goal(follower.get());
    goal.shouldExecute();  // 触发初始化

    EXPECT_TRUE(follower->inRangeOfGroupLeader());
    EXPECT_TRUE(goal.shouldContinueExecuting());
}

TEST_F(FollowSchoolLeaderGoalTest, ShouldNotContinueExecutingWhenOutOfRange) {
    auto leader = std::make_unique<CodEntity>(LegacyEntityType::Cod, 1);
    auto follower = std::make_unique<CodEntity>(LegacyEntityType::Cod, 2);

    leader->setWorld(m_world.get());
    follower->setWorld(m_world.get());

    // 设置位置在跟随范围外（默认 11 格，设置 12 格）
    leader->setPosition(0.0f, 62.0f, 0.0f);
    follower->setPosition(12.0f, 62.0f, 0.0f);

    follower->joinGroup(*leader);

    entity::ai::goal::FollowSchoolLeaderGoal goal(follower.get());
    goal.shouldExecute();

    EXPECT_FALSE(follower->inRangeOfGroupLeader());
    EXPECT_FALSE(goal.shouldContinueExecuting());
}

TEST_F(FollowSchoolLeaderGoalTest, ShouldLeaveGroupOnReset) {
    auto leader = std::make_unique<CodEntity>(LegacyEntityType::Cod, 1);
    auto follower = std::make_unique<CodEntity>(LegacyEntityType::Cod, 2);

    leader->setWorld(m_world.get());
    follower->setWorld(m_world.get());

    leader->setPosition(0.0f, 62.0f, 0.0f);
    follower->setPosition(5.0f, 62.0f, 0.0f);

    follower->joinGroup(*leader);
    EXPECT_TRUE(follower->hasGroupLeader());
    EXPECT_EQ(leader->getGroupSize(), 2);

    entity::ai::goal::FollowSchoolLeaderGoal goal(follower.get());
    goal.shouldExecute();
    goal.resetTask();

    // resetTask 后应该离开群体
    EXPECT_FALSE(follower->hasGroupLeader());
    EXPECT_EQ(leader->getGroupSize(), 1);
}

TEST_F(FollowSchoolLeaderGoalTest, ShouldFindGroupToJoin) {
    // 创建一个可扩群的首领
    auto leader = std::make_unique<CodEntity>(LegacyEntityType::Cod, 1);
    auto follower1 = std::make_unique<CodEntity>(LegacyEntityType::Cod, 2);
    auto follower2 = std::make_unique<CodEntity>(LegacyEntityType::Cod, 3);

    leader->setWorld(m_world.get());
    follower1->setWorld(m_world.get());
    follower2->setWorld(m_world.get());

    // 设置位置在搜索范围内（8 格）
    leader->setPosition(0.0f, 62.0f, 0.0f);
    follower1->setPosition(2.0f, 62.0f, 0.0f);  // 在搜索范围内
    follower2->setPosition(3.0f, 62.0f, 0.0f);  // 在搜索范围内

    // leader 已经有一个跟随者，成为可扩群的首领
    follower2->joinGroup(*leader);
    EXPECT_TRUE(leader->isGroupLeader());
    EXPECT_TRUE(leader->canGroupGrow());  // 最大 8 条，当前 2 条

    // 设置世界实体列表
    m_world->setEntities({leader.get(), follower1.get(), follower2.get()});

    // follower1 搜索群体
    entity::ai::goal::FollowSchoolLeaderGoal goal(follower1.get());
    // 多次调用直到冷却结束
    for (int i = 0; i < 250; ++i) {
        if (goal.shouldExecute()) {
            break;
        }
    }

    // follower1 应该加入 leader 的群体
    EXPECT_TRUE(follower1->hasGroupLeader());
    EXPECT_EQ(follower1->getGroupLeader(), leader.get());
    EXPECT_EQ(leader->getGroupSize(), 3);
}

TEST_F(FollowSchoolLeaderGoalTest, ShouldRespectMaxGroupSize) {
    // SalmonEntity 最大群体大小为 5
    auto leader = std::make_unique<SalmonEntity>(LegacyEntityType::Salmon, 1);
    std::vector<std::unique_ptr<SalmonEntity>> followers;

    leader->setWorld(m_world.get());
    leader->setPosition(0.0f, 62.0f, 0.0f);

    std::vector<Entity*> entities = {leader.get()};

    // 创建并加入 4 条鱼（达到最大群体大小 5）
    for (int i = 0; i < 4; ++i) {
        auto follower = std::make_unique<SalmonEntity>(LegacyEntityType::Salmon, static_cast<EntityId>(i + 2));
        follower->setWorld(m_world.get());
        follower->setPosition(static_cast<f32>(i + 1), 62.0f, 0.0f);
        follower->joinGroup(*leader);
        entities.push_back(follower.get());
        followers.push_back(std::move(follower));
    }

    EXPECT_EQ(leader->getGroupSize(), 5);
    EXPECT_FALSE(leader->canGroupGrow());  // 已满员

    // 创建第 6 条鱼
    auto extraFollower = std::make_unique<SalmonEntity>(LegacyEntityType::Salmon, 10);
    extraFollower->setWorld(m_world.get());
    extraFollower->setPosition(5.0f, 62.0f, 0.0f);
    entities.push_back(extraFollower.get());

    m_world->setEntities(entities);

    // 满员后不应该再加入
    entity::ai::goal::FollowSchoolLeaderGoal goal(extraFollower.get());
    for (int i = 0; i < 250; ++i) {
        goal.shouldExecute();
    }

    // 群体已满，extraFollower 无法加入
    EXPECT_FALSE(extraFollower->hasGroupLeader());
    EXPECT_EQ(leader->getGroupSize(), 5);
}

TEST_F(FollowSchoolLeaderGoalTest, RecruitFollowersWorks) {
    auto leader = std::make_unique<CodEntity>(LegacyEntityType::Cod, 1);
    auto follower1 = std::make_unique<CodEntity>(LegacyEntityType::Cod, 2);
    auto follower2 = std::make_unique<CodEntity>(LegacyEntityType::Cod, 3);
    auto follower3 = std::make_unique<CodEntity>(LegacyEntityType::Cod, 4);

    leader->setWorld(m_world.get());

    // 先让 follower1 加入，使 leader 成为首领
    follower1->joinGroup(*leader);
    EXPECT_TRUE(leader->isGroupLeader());

    // 使用 recruitFollowers 招募其他跟随者
    std::vector<AbstractGroupFishEntity*> candidates = {follower2.get(), follower3.get()};
    leader->recruitFollowers(candidates);

    // 两条鱼应该加入
    EXPECT_TRUE(follower2->hasGroupLeader());
    EXPECT_TRUE(follower3->hasGroupLeader());
    EXPECT_EQ(leader->getGroupSize(), 4);
}

TEST_F(FollowSchoolLeaderGoalTest, MoveToGroupLeaderWorks) {
    auto leader = std::make_unique<CodEntity>(LegacyEntityType::Cod, 1);
    auto follower = std::make_unique<CodEntity>(LegacyEntityType::Cod, 2);

    leader->setWorld(m_world.get());
    follower->setWorld(m_world.get());

    leader->setPosition(0.0f, 62.0f, 0.0f);
    follower->setPosition(10.0f, 62.0f, 0.0f);

    follower->joinGroup(*leader);

    // moveToGroupLeader 不应该崩溃
    follower->moveToGroupLeader();

    // 验证群体关系仍然存在
    EXPECT_TRUE(follower->hasGroupLeader());
    EXPECT_EQ(follower->getGroupLeader(), leader.get());
}

// ============================================================================
// AbstractFishEntity FromBucket Tests
// ============================================================================

class AbstractFishEntityFromBucketTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_world = std::make_unique<TestFishWorld>();
    }

    void TearDown() override {
        m_world.reset();
    }

    std::unique_ptr<TestFishWorld> m_world;
};

TEST_F(AbstractFishEntityFromBucketTest, DefaultFromBucketIsFalse) {
    CodEntity cod(LegacyEntityType::Cod, 1);
    EXPECT_FALSE(cod.isFromBucket());
}

TEST_F(AbstractFishEntityFromBucketTest, SetFromBucketToTrue) {
    CodEntity cod(LegacyEntityType::Cod, 1);
    cod.setFromBucket(true);
    EXPECT_TRUE(cod.isFromBucket());
}

TEST_F(AbstractFishEntityFromBucketTest, SetFromBucketToFalse) {
    CodEntity cod(LegacyEntityType::Cod, 1);
    cod.setFromBucket(true);
    cod.setFromBucket(false);
    EXPECT_FALSE(cod.isFromBucket());
}

TEST_F(AbstractFishEntityFromBucketTest, FromBucketFishPreventsDespawn) {
    CodEntity cod(LegacyEntityType::Cod, 1);

    // 默认情况下，鱼不在被骑乘状态，preventDespawn 返回 false
    EXPECT_FALSE(cod.preventDespawn());

    // 设置 FromBucket 后，preventDespawn 应该返回 true
    cod.setFromBucket(true);
    EXPECT_TRUE(cod.preventDespawn());

    // 再次设置 false，preventDespawn 应该返回 false
    cod.setFromBucket(false);
    EXPECT_FALSE(cod.preventDespawn());
}

TEST_F(AbstractFishEntityFromBucketTest, FromBucketFishCannotDespawn) {
    CodEntity cod(LegacyEntityType::Cod, 1);

    // 默认情况下，鱼可以消失（没有自定义名称）
    EXPECT_TRUE(cod.canDespawn(128.0));

    // 设置 FromBucket 后，canDespawn 应该返回 false
    cod.setFromBucket(true);
    EXPECT_FALSE(cod.canDespawn(128.0));
    EXPECT_FALSE(cod.canDespawn(0.0));  // 即使玩家很近

    // 设置自定义名称也阻止消失
    cod.setFromBucket(false);
    cod.setCustomName("Nemo");  // 使用 setCustomName(String) 重载
    EXPECT_FALSE(cod.canDespawn(128.0));
}

TEST_F(AbstractFishEntityFromBucketTest, AllFishTypesSupportFromBucket) {
    // 测试所有鱼类实体都支持 FromBucket
    CodEntity cod(LegacyEntityType::Cod, 1);
    SalmonEntity salmon(LegacyEntityType::Salmon, 2);
    PufferfishEntity pufferfish(LegacyEntityType::Pufferfish, 3);
    TropicalFishEntity tropicalFish(LegacyEntityType::TropicalFish, 4);

    // 所有鱼类默认不是从桶放出的
    EXPECT_FALSE(cod.isFromBucket());
    EXPECT_FALSE(salmon.isFromBucket());
    EXPECT_FALSE(pufferfish.isFromBucket());
    EXPECT_FALSE(tropicalFish.isFromBucket());

    // 设置后都能正确响应
    cod.setFromBucket(true);
    salmon.setFromBucket(true);
    pufferfish.setFromBucket(true);
    tropicalFish.setFromBucket(true);

    EXPECT_TRUE(cod.isFromBucket());
    EXPECT_TRUE(salmon.isFromBucket());
    EXPECT_TRUE(pufferfish.isFromBucket());
    EXPECT_TRUE(tropicalFish.isFromBucket());

    // 设置 FromBucket 后都能阻止消失
    EXPECT_TRUE(cod.preventDespawn());
    EXPECT_TRUE(salmon.preventDespawn());
    EXPECT_TRUE(pufferfish.preventDespawn());
    EXPECT_TRUE(tropicalFish.preventDespawn());

    EXPECT_FALSE(cod.canDespawn(128.0));
    EXPECT_FALSE(salmon.canDespawn(128.0));
    EXPECT_FALSE(pufferfish.canDespawn(128.0));
    EXPECT_FALSE(tropicalFish.canDespawn(128.0));
}

} // namespace
} // namespace mc
