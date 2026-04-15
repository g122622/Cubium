#include <gtest/gtest.h>

#include "common/entity/entities/passive/horse/AbstractChestedHorseEntity.hpp"
#include "common/entity/entities/passive/horse/DonkeyEntity.hpp"
#include "common/entity/entities/passive/horse/HorseEntity.hpp"
#include "common/entity/entities/passive/horse/LlamaEntity.hpp"
#include "common/entity/entities/passive/horse/MuleEntity.hpp"

namespace mc {
namespace {

TEST(AbstractChestedHorseEntityTest, CoversChestHorseSubtypesButNotPlainHorse)
{
    HorseEntity horse(LegacyEntityType::Horse, 1);
    DonkeyEntity donkey(LegacyEntityType::Donkey, 2);
    MuleEntity mule(LegacyEntityType::Mule, 3);
    LlamaEntity llama(LegacyEntityType::Llama, 4);

    EXPECT_EQ(dynamic_cast<AbstractChestedHorseEntity*>(&horse), nullptr);
    EXPECT_NE(dynamic_cast<AbstractChestedHorseEntity*>(&donkey), nullptr);
    EXPECT_NE(dynamic_cast<AbstractChestedHorseEntity*>(&mule), nullptr);
    EXPECT_NE(dynamic_cast<AbstractChestedHorseEntity*>(&llama), nullptr);
}

TEST(AbstractChestedHorseEntityTest, UsesVanillaStyleChestInventorySizing)
{
    DonkeyEntity donkey(LegacyEntityType::Donkey, 1);
    MuleEntity mule(LegacyEntityType::Mule, 2);
    LlamaEntity llama(LegacyEntityType::Llama, 3);

    EXPECT_FALSE(donkey.hasChest());
    EXPECT_EQ(donkey.getInventorySize(), 2);
    donkey.setChest(true);
    EXPECT_EQ(donkey.getInventoryColumns(), 5);
    EXPECT_EQ(donkey.getInventorySize(), 17);

    EXPECT_FALSE(mule.hasChest());
    EXPECT_EQ(mule.getInventorySize(), 2);
    mule.setChest(true);
    EXPECT_EQ(mule.getInventoryColumns(), 5);
    EXPECT_EQ(mule.getInventorySize(), 17);

    llama.setStrength(1);
    EXPECT_EQ(llama.getInventorySize(), 2);
    EXPECT_EQ(llama.getInventoryColumns(), 1);
    llama.setStrength(5);
    llama.setChest(true);
    EXPECT_EQ(llama.getInventoryColumns(), 5);
    EXPECT_EQ(llama.getInventorySize(), 17);
}

TEST(HorseSupportTypesTest, LlamaStrengthIsClampedToVanillaRange)
{
    LlamaEntity llama(LegacyEntityType::Llama, 1);

    llama.setStrength(0);
    EXPECT_EQ(llama.getStrength(), 1);

    llama.setStrength(3);
    EXPECT_EQ(llama.getStrength(), 3);
    EXPECT_EQ(llama.getInventoryColumns(), 3);

    llama.setStrength(8);
    EXPECT_EQ(llama.getStrength(), 5);
    EXPECT_EQ(llama.getInventoryColumns(), 5);
}

} // namespace
} // namespace mc
