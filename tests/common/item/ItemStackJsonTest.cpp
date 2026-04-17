#include <gtest/gtest.h>

#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/core/ItemStack.hpp"

using namespace mc;

namespace {

/**
 * @brief 按资源路径懒注册测试用物品。
 * @param path 资源路径。
 * @return 已注册物品指针。
 */
Item* ensureTestItem(const char* path) {
    auto& registry = ItemRegistry::instance();
    const ResourceLocation id("minecraft", path);
    if (Item* existing = registry.getItem(id); existing != nullptr) {
        return existing;
    }

    return &registry.registerItem(id, ItemProperties().maxStackSize(64));
}

} // namespace

TEST(ItemStackJsonTest, RoundTrip_WithRegisteredItem_PreservesItemAndCount) {
    Items::initialize();
    Item* diamond = ensureTestItem("diamond");
    ASSERT_NE(diamond, nullptr);

    ItemStack original(diamond, 13);
    nlohmann::json json = original.toJson();

    auto parsed = ItemStack::fromJson(json);
    ASSERT_TRUE(parsed.success()) << parsed.error().message();

    const ItemStack& result = parsed.value();
    EXPECT_EQ(result.getItem(), diamond);
    EXPECT_EQ(result.getCount(), 13);
}
