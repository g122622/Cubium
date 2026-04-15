#include <gtest/gtest.h>

#include "common/entity/entities/player/ChatVisibility.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/player/PlayerModelPart.hpp"

namespace mc {
namespace {

TEST(ChatVisibilityTest, NormalizesIdsLikeVanilla)
{
    EXPECT_EQ(getChatVisibilityById(0), ChatVisibility::Full);
    EXPECT_EQ(getChatVisibilityById(1), ChatVisibility::System);
    EXPECT_EQ(getChatVisibilityById(2), ChatVisibility::Hidden);
    EXPECT_EQ(getChatVisibilityById(3), ChatVisibility::Full);
    EXPECT_EQ(getChatVisibilityById(-1), ChatVisibility::Hidden);

    EXPECT_STREQ(getChatVisibilityTranslationKey(ChatVisibility::System), "options.chat.visibility.system");
}

TEST(PlayerModelPartTest, ExposesVanillaMasksAndNames)
{
    EXPECT_EQ(getPlayerModelPartId(PlayerModelPart::Cape), 0);
    EXPECT_EQ(getPlayerModelPartMask(PlayerModelPart::Cape), 0x01);
    EXPECT_EQ(getPlayerModelPartMask(PlayerModelPart::Hat), 0x40);
    EXPECT_EQ(PLAYER_MODEL_PARTS_ALL_MASK, 0x7F);

    EXPECT_STREQ(getPlayerModelPartName(PlayerModelPart::LeftSleeve), "left_sleeve");
    EXPECT_STREQ(getPlayerModelPartTranslationKey(PlayerModelPart::RightPantsLeg),
                 "options.modelPart.right_pants_leg");
}

TEST(PlayerSupportTypesTest, PlayerTracksChatVisibilityAndModelParts)
{
    Player player(1, "Steve");

    EXPECT_EQ(player.chatVisibility(), ChatVisibility::Full);
    EXPECT_TRUE(player.isWearing(PlayerModelPart::Cape));
    EXPECT_TRUE(player.isWearing(PlayerModelPart::Hat));

    player.setChatVisibility(ChatVisibility::Hidden);
    player.setModelPartEnabled(PlayerModelPart::Cape, false);
    player.setModelPartEnabled(PlayerModelPart::Hat, false);

    EXPECT_EQ(player.chatVisibility(), ChatVisibility::Hidden);
    EXPECT_FALSE(player.isWearing(PlayerModelPart::Cape));
    EXPECT_FALSE(player.isWearing(PlayerModelPart::Hat));
    EXPECT_TRUE(player.isWearing(PlayerModelPart::Jacket));

    player.setPlayerModelParts(getPlayerModelPartMask(PlayerModelPart::Hat));
    EXPECT_TRUE(player.isWearing(PlayerModelPart::Hat));
    EXPECT_FALSE(player.isWearing(PlayerModelPart::Jacket));
}

} // namespace
} // namespace mc
