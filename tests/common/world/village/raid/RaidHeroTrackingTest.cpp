#include "common/command/ICommandSource.hpp"
#include "common/util/UuidUtils.hpp"
#include "common/world/village/Village.hpp"
#include "common/world/village/raid/Raid.hpp"
#include <gtest/gtest.h>

namespace mc {
namespace world::village::raid {
namespace test {

/**
 * @brief Raid 英雄追踪测试
 *
 * 测试 Raid 类中的英雄追踪功能：
 * - addHero: 添加英雄
 * - isHero: 检查是否为英雄
 * - heroes: 获取所有英雄 UUID
 * - addContribution: 增加贡献值
 * - getContribution: 获取贡献值
 */
class RaidHeroTrackingTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 创建测试用的 UUID
        uuid1 = util::uuidFromString("00000000000000000000000000000001");
        uuid2 = util::uuidFromString("00000000000000000000000000000002");
        uuid3 = util::uuidFromString("00000000000000000000000000000003");
    }

    Uuid uuid1;
    Uuid uuid2;
    Uuid uuid3;
};

TEST_F(RaidHeroTrackingTest, AddHero_AddsUuidToHeroesSet)
{
    Raid raid(1, nullptr);

    raid.addHero(uuid1, EntityId(100));

    EXPECT_TRUE(raid.isHero(uuid1));
    EXPECT_FALSE(raid.isHero(uuid2));
}

TEST_F(RaidHeroTrackingTest, AddHero_DoesNotDuplicateEntries)
{
    Raid raid(1, nullptr);

    raid.addHero(uuid1, EntityId(100));
    raid.addHero(uuid1, EntityId(101)); // 相同 UUID，不同 EntityId

    EXPECT_TRUE(raid.isHero(uuid1));
    const auto& heroes = raid.heroes();
    EXPECT_EQ(heroes.size(), 1u);
}

TEST_F(RaidHeroTrackingTest, AddHero_MultipleHeroes)
{
    Raid raid(1, nullptr);

    raid.addHero(uuid1, EntityId(100));
    raid.addHero(uuid2, EntityId(200));
    raid.addHero(uuid3, EntityId(300));

    EXPECT_TRUE(raid.isHero(uuid1));
    EXPECT_TRUE(raid.isHero(uuid2));
    EXPECT_TRUE(raid.isHero(uuid3));

    const auto& heroes = raid.heroes();
    EXPECT_EQ(heroes.size(), 3u);
}

TEST_F(RaidHeroTrackingTest, IsHero_ReturnsFalseForNonHero)
{
    Raid raid(1, nullptr);

    EXPECT_FALSE(raid.isHero(uuid1));
    EXPECT_FALSE(raid.isHero(uuid2));
}

TEST_F(RaidHeroTrackingTest, Heroes_ReturnsAllHeroUuids)
{
    Raid raid(1, nullptr);

    raid.addHero(uuid1, EntityId(100));
    raid.addHero(uuid2, EntityId(200));

    const auto& heroes = raid.heroes();

    EXPECT_EQ(heroes.size(), 2u);
    EXPECT_NE(heroes.find(uuid1), heroes.end());
    EXPECT_NE(heroes.find(uuid2), heroes.end());
    EXPECT_EQ(heroes.find(uuid3), heroes.end());
}

TEST_F(RaidHeroTrackingTest, AddContribution_IncreasesContribution)
{
    Raid raid(1, nullptr);

    raid.addHero(uuid1, EntityId(100));
    raid.addContribution(uuid1, 1);
    raid.addContribution(uuid1, 2);

    EXPECT_EQ(raid.getContribution(uuid1), 3);
}

TEST_F(RaidHeroTrackingTest, AddContribution_DoesNotCreateNewHero)
{
    Raid raid(1, nullptr);

    // 对非英雄玩家增加贡献值不会添加为新英雄
    raid.addContribution(uuid1, 5);

    EXPECT_FALSE(raid.isHero(uuid1));
    EXPECT_EQ(raid.getContribution(uuid1), 0); // 不存在时返回 0
}

TEST_F(RaidHeroTrackingTest, GetContribution_ReturnsZeroForNonParticipant)
{
    Raid raid(1, nullptr);

    EXPECT_EQ(raid.getContribution(uuid1), 0);
    EXPECT_EQ(raid.getContribution(uuid2), 0);
}

TEST_F(RaidHeroTrackingTest, MultipleHeroesWithDifferentContributions)
{
    Raid raid(1, nullptr);

    raid.addHero(uuid1, EntityId(100));
    raid.addHero(uuid2, EntityId(200));
    raid.addHero(uuid3, EntityId(300));

    raid.addContribution(uuid1, 5);
    raid.addContribution(uuid2, 10);
    raid.addContribution(uuid2, 3); // uuid2 再加 3
    raid.addContribution(uuid3, 1);

    EXPECT_EQ(raid.getContribution(uuid1), 5);
    EXPECT_EQ(raid.getContribution(uuid2), 13);
    EXPECT_EQ(raid.getContribution(uuid3), 1);
}

} // namespace test
} // namespace world::village::raid
} // namespace mc
