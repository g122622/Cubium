/**
 * @brief EyeOfEnderEntity 物品字段测试
 *
 * 覆盖 vanilla EyeOfEnder 的 Item 字段语义：默认末影之眼、空值回退、
 * 非空物品归一化为 1 个，以及同步参数 DATA_ITEM_STACK 的镜像。
 */

#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/projectile/OtherProjectiles.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/network/ir/packets/play/ItemStackView.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity;

class EyeOfEnderItemTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 末影之眼默认物品需先完成物品注册。
        Items::ensureInitialized();
    }

    void SetUp() override
    {
        m_eye = std::make_unique<EyeOfEnderEntity>(EntityInstanceId(0), mc::test::testEcsRegistry());
    }

    void TearDown() override
    {
        m_eye.reset();
    }

    std::unique_ptr<EyeOfEnderEntity> m_eye;
};

TEST_F(EyeOfEnderItemTest, Item_DefaultsToEnderEye)
{
    ASSERT_NE(Items::ENDER_EYE, nullptr);
    const ItemStack item = m_eye->getItem();
    EXPECT_EQ(item.getItem(), Items::ENDER_EYE);
    EXPECT_EQ(item.getCount(), 1);
}

TEST_F(EyeOfEnderItemTest, SetItem_NormalizesCountToOne)
{
    ItemStack stack(*Items::ENDER_EYE, 16);
    m_eye->setItem(stack);

    const ItemStack stored = m_eye->getItem();
    EXPECT_EQ(stored.getItem(), Items::ENDER_EYE);
    EXPECT_EQ(stored.getCount(), 1);
}

TEST_F(EyeOfEnderItemTest, SetItem_EmptyFallsBackToDefault)
{
    m_eye->setItem(ItemStack());
    const ItemStack stored = m_eye->getItem();
    ASSERT_NE(stored.getItem(), nullptr);
    EXPECT_EQ(stored.getItem(), Items::ENDER_EYE);
}

TEST_F(EyeOfEnderItemTest, SetItem_MirrorsSyncedData)
{
    m_eye->setItem(ItemStack(*Items::ENDER_EYE, 1));
    const auto view =
        m_eye->dataManager().get<network::ir::play::ItemStackView>(EyeOfEnderEntity::DATA_ITEM_STACK_PARAM);
    EXPECT_GT(view.itemId, 0u);
    EXPECT_EQ(view.count, 1);
}