#include <gtest/gtest.h>

#include "server/world/drop/BlockDropHandler.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "common/item/Items.hpp"
#include "common/world/block/VanillaBlocks.hpp"

#include <mutex>
#include <vector>

namespace mc {

namespace {

void ensureRegistriesInitialized() {
    static std::once_flag s_once;
    std::call_once(s_once, [] {
        VanillaBlocks::initialize();
        Items::initialize();
    });
}

} // namespace

TEST(BlockDropHandlerTest, SpawnDropsToEntityManagerCreatesItemEntities) {
    ensureRegistriesInitialized();

    ASSERT_NE(Items::APPLE, nullptr);

    EntityManager entityManager;
    const BlockPos pos(12, 80, -4);
    const std::vector<ItemStack> drops{ItemStack(*Items::APPLE, 2)};

    const auto spawned = BlockDropHandler::spawnDrops(
        entityManager,
        nullptr,
        pos,
        drops,
        "");

    ASSERT_EQ(spawned.size(), 1u);
    EXPECT_TRUE(entityManager.hasEntity(spawned[0]));
    EXPECT_EQ(entityManager.entityCount(), 1u);

    const Entity* entity = entityManager.getEntity(spawned[0]);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->legacyType(), LegacyEntityType::Item);
}

} // namespace mc
