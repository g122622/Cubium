#include <gtest/gtest.h>

#include "common/entity/entities/passive/horse/LlamaEntity.hpp"
#include "common/entity/entities/passive/horse/TraderLlamaEntity.hpp"
#include "common/entity/entities/player/Player.hpp"

namespace mc {
namespace {

TEST(TraderLlamaEntityTest, InheritsFromLlamaAndExposesTraderFlag)
{
    TraderLlamaEntity traderLlama(LegacyEntityType::TraderLlama, 1);

    EXPECT_NE(dynamic_cast<LlamaEntity*>(&traderLlama), nullptr);
    EXPECT_TRUE(traderLlama.isTraderLlama());
    EXPECT_EQ(traderLlama.getDespawnDelay(), 47999);
}

TEST(TraderLlamaEntityTest, UntamedTraderLlamaDespawnsWhenDelayExpires)
{
    TraderLlamaEntity traderLlama(LegacyEntityType::TraderLlama, 1);
    traderLlama.setDespawnDelay(1);

    traderLlama.tick();

    EXPECT_TRUE(traderLlama.isRemoved());
}

TEST(TraderLlamaEntityTest, TamedOrRiddenTraderLlamaDoesNotDespawn)
{
    TraderLlamaEntity tamedTraderLlama(LegacyEntityType::TraderLlama, 1);
    tamedTraderLlama.setTame(true);
    tamedTraderLlama.setDespawnDelay(1);
    tamedTraderLlama.tick();
    EXPECT_FALSE(tamedTraderLlama.isRemoved());
    EXPECT_EQ(tamedTraderLlama.getDespawnDelay(), 1);

    TraderLlamaEntity riddenTraderLlama(LegacyEntityType::TraderLlama, 2);
    Player rider(1, "Steve");
    riddenTraderLlama.setRider(&rider);
    riddenTraderLlama.setDespawnDelay(1);
    riddenTraderLlama.tick();
    EXPECT_FALSE(riddenTraderLlama.isRemoved());
    EXPECT_EQ(riddenTraderLlama.getDespawnDelay(), 1);
}

} // namespace
} // namespace mc
