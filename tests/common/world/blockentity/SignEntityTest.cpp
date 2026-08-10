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

#include "world/blockentity/interactive/SignEntity.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "util/text/StringTextComponent.hpp"
#include "world/block/BlockPos.hpp"
#include "world/blockentity/BlockEntityType.hpp"
#include "world/blockentity/core/BlockEntityRegistry.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::blockentity;

// ========== SignEntityType 注册测试 ==========

class SignEntityRegistryTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 确保内置类型已注册
        BlockEntityRegistry::instance().registerBuiltinTypes();
    }
};

TEST_F(SignEntityRegistryTest, SignType_IsRegistered)
{
    // 验证 Sign 类型已在注册表中注册
    EXPECT_TRUE(BlockEntityRegistry::instance().hasType(BlockEntityType::Sign));
}

TEST_F(SignEntityRegistryTest, CreateSignEntity_ReturnsValidEntity)
{
    BlockPos pos(10, 64, -5);
    auto entity = BlockEntityRegistry::instance().create(BlockEntityType::Sign, pos);

    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->getType(), BlockEntityType::Sign);
    EXPECT_EQ(entity->getPos(), pos);
}

TEST_F(SignEntityRegistryTest, CreateSignEntity_CreatesSignEntity)
{
    BlockPos pos(0, 0, 0);
    auto entity = BlockEntityRegistry::instance().create(BlockEntityType::Sign, pos);

    // 验证创建的是 SignEntity 类型
    auto* signEntity = dynamic_cast<SignEntity*>(entity.get());
    EXPECT_NE(signEntity, nullptr);
}

TEST_F(SignEntityRegistryTest, CreateFromJson_SignEntity)
{
    nlohmann::json data;
    data["id"] = "minecraft:sign";
    data["x"] = 100;
    data["y"] = 64;
    data["z"] = -200;

    auto entity = BlockEntityRegistry::instance().createFromJson(data);

    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->getType(), BlockEntityType::Sign);
    EXPECT_EQ(entity->getPos().x, 100);
    EXPECT_EQ(entity->getPos().y, 64);
    EXPECT_EQ(entity->getPos().z, -200);
}

// ========== SignEntity 功能测试 ==========

class SignEntityTest : public ::testing::Test {
protected:
    void SetUp() override { signEntity = std::make_unique<SignEntity>(BlockPos(10, 20, 30)); }

    std::unique_ptr<SignEntity> signEntity;
};

TEST_F(SignEntityTest, Constructor_InitializesEmptyLines)
{
    for (i32 i = 0; i < SignEntity::LINE_COUNT; ++i) {
        const auto* line = signEntity->getLine(i);
        ASSERT_NE(line, nullptr);
        EXPECT_EQ(line->getUnformattedText(), "");
    }
}

TEST_F(SignEntityTest, Constructor_SetsCorrectType)
{
    EXPECT_EQ(signEntity->getType(), BlockEntityType::Sign);
}

TEST_F(SignEntityTest, Constructor_SetsCorrectPosition)
{
    EXPECT_EQ(signEntity->getPos(), BlockPos(10, 20, 30));
}

TEST_F(SignEntityTest, SetLine_ValidText)
{
    auto text = std::make_unique<text::StringTextComponent>("Hello World");
    EXPECT_TRUE(signEntity->setLine(0, std::move(text)));

    EXPECT_EQ(signEntity->getLineText(0), "Hello World");
}

TEST_F(SignEntityTest, SetLine_InvalidLine_ReturnsFalse)
{
    auto text = std::make_unique<text::StringTextComponent>("Test");
    EXPECT_FALSE(signEntity->setLine(-1, std::move(text)));
    EXPECT_FALSE(signEntity->setLine(4, std::move(text)));
}

TEST_F(SignEntityTest, SetLine_AllLines)
{
    for (i32 i = 0; i < SignEntity::LINE_COUNT; ++i) {
        auto text = std::make_unique<text::StringTextComponent>("Line " + std::to_string(i));
        EXPECT_TRUE(signEntity->setLine(i, std::move(text)));
    }

    for (i32 i = 0; i < SignEntity::LINE_COUNT; ++i) {
        EXPECT_EQ(signEntity->getLineText(i), "Line " + std::to_string(i));
    }
}

TEST_F(SignEntityTest, SetLineFromLegacy_PlainText)
{
    EXPECT_TRUE(signEntity->setLineFromLegacy(0, "Plain Text"));
    EXPECT_EQ(signEntity->getLineText(0), "Plain Text");
}

TEST_F(SignEntityTest, GetLine_OutOfRange_ReturnsNullptr)
{
    EXPECT_EQ(signEntity->getLine(-1), nullptr);
    EXPECT_EQ(signEntity->getLine(4), nullptr);
}

TEST_F(SignEntityTest, ClearLines_ClearsAllLines)
{
    signEntity->setLineFromLegacy(0, "Line 0");
    signEntity->setLineFromLegacy(1, "Line 1");
    signEntity->setLineFromLegacy(2, "Line 2");
    signEntity->setLineFromLegacy(3, "Line 3");

    signEntity->clearLines();

    for (i32 i = 0; i < SignEntity::LINE_COUNT; ++i) {
        EXPECT_EQ(signEntity->getLineText(i), "");
    }
}

TEST_F(SignEntityTest, Editable_InitiallyTrue)
{
    EXPECT_TRUE(signEntity->isEditable());
}

TEST_F(SignEntityTest, SetEditable_ChangesState)
{
    signEntity->setEditable(false);
    EXPECT_FALSE(signEntity->isEditable());

    signEntity->setEditable(true);
    EXPECT_TRUE(signEntity->isEditable());
}

TEST_F(SignEntityTest, PlayerWhoMayEdit_InitiallyEmpty)
{
    EXPECT_TRUE(signEntity->getPlayerWhoMayEdit().empty());
}

TEST_F(SignEntityTest, TextColor_InitiallyZero)
{
    EXPECT_EQ(signEntity->getTextColor(), 0);
}

TEST_F(SignEntityTest, SetTextColor_ChangesColor)
{
    signEntity->setTextColor(14); // Red dye color
    EXPECT_EQ(signEntity->getTextColor(), 14);
}

TEST_F(SignEntityTest, Glowing_InitiallyFalse)
{
    EXPECT_FALSE(signEntity->isGlowing());
}

TEST_F(SignEntityTest, SetGlowing_ChangesState)
{
    signEntity->setGlowing(true);
    EXPECT_TRUE(signEntity->isGlowing());

    signEntity->setGlowing(false);
    EXPECT_FALSE(signEntity->isGlowing());
}

TEST_F(SignEntityTest, OnlyOpsCanSetNbt_AlwaysTrue)
{
    EXPECT_TRUE(signEntity->onlyOpsCanSetNbt());
}

TEST_F(SignEntityTest, Save_PreservesBasicInfo)
{
    signEntity->setLineFromLegacy(0, "Hello");
    signEntity->setTextColor(5);
    signEntity->setGlowing(true);

    nlohmann::json data;
    signEntity->save(data);

    EXPECT_EQ(data["id"], "minecraft:sign");
    EXPECT_EQ(data["x"].get<i32>(), 10);
    EXPECT_EQ(data["y"].get<i32>(), 20);
    EXPECT_EQ(data["z"].get<i32>(), 30);
}

TEST_F(SignEntityTest, Load_PreservesTextLines)
{
    // 创建原始数据
    signEntity->setLineFromLegacy(0, "Line 0");
    signEntity->setLineFromLegacy(1, "Line 1");
    signEntity->setLineFromLegacy(2, "Line 2");
    signEntity->setLineFromLegacy(3, "Line 3");
    signEntity->setTextColor(7);
    signEntity->setGlowing(true);

    nlohmann::json data;
    signEntity->save(data);

    // 加载到新实体
    auto loaded = std::make_unique<SignEntity>(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded->load(data));

    for (i32 i = 0; i < SignEntity::LINE_COUNT; ++i) {
        EXPECT_EQ(loaded->getLineText(i), "Line " + std::to_string(i));
    }
    EXPECT_EQ(loaded->getTextColor(), 7);
    EXPECT_TRUE(loaded->isGlowing());
}

TEST_F(SignEntityTest, Clone_CreatesExactCopy)
{
    signEntity->setLineFromLegacy(0, "Test Line");
    signEntity->setTextColor(12);
    signEntity->setGlowing(true);
    signEntity->setEditable(false);

    auto copy = signEntity->clone();

    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->getType(), BlockEntityType::Sign);
    EXPECT_EQ(copy->getPos(), BlockPos(10, 20, 30));

    auto* signCopy = dynamic_cast<SignEntity*>(copy.get());
    ASSERT_NE(signCopy, nullptr);
    EXPECT_EQ(signCopy->getLineText(0), "Test Line");
    EXPECT_EQ(signCopy->getTextColor(), 12);
    EXPECT_TRUE(signCopy->isGlowing());
    EXPECT_FALSE(signCopy->isEditable());
}

TEST_F(SignEntityTest, Constants_CorrectValues)
{
    EXPECT_EQ(SignEntity::LINE_COUNT, 4);
    EXPECT_EQ(SignEntity::MAX_LINE_LENGTH, 15);
}

// ========== SignEntity 涂蜡状态测试 ==========

TEST_F(SignEntityTest, Waxed_InitiallyFalse)
{
    EXPECT_FALSE(signEntity->isWaxed());
}

TEST_F(SignEntityTest, SetWaxed_ChangesState)
{
    EXPECT_TRUE(signEntity->setWaxed(true));
    EXPECT_TRUE(signEntity->isWaxed());

    EXPECT_TRUE(signEntity->setWaxed(false));
    EXPECT_FALSE(signEntity->isWaxed());
}

TEST_F(SignEntityTest, SetWaxed_SameValue_ReturnsFalse)
{
    // 初始未涂蜡，再次设置未涂蜡应返回 false
    EXPECT_FALSE(signEntity->setWaxed(false));

    // 涂蜡后，再次设置涂蜡应返回 false
    EXPECT_TRUE(signEntity->setWaxed(true));
    EXPECT_FALSE(signEntity->setWaxed(true));
}

TEST_F(SignEntityTest, SetWaxed_PreventsTextModification)
{
    signEntity->setLineFromLegacy(0, "Original Text");

    // 涂蜡后不应允许修改文字
    signEntity->setWaxed(true);
    EXPECT_FALSE(signEntity->setLineFromLegacy(0, "Modified Text"));
    EXPECT_EQ(signEntity->getLineText(0), "Original Text");
}

TEST_F(SignEntityTest, SetWaxed_PreventsSetLine)
{
    signEntity->setLineFromLegacy(0, "Original");

    signEntity->setWaxed(true);
    auto newText = std::make_unique<text::StringTextComponent>("New");
    EXPECT_FALSE(signEntity->setLine(0, std::move(newText)));
    EXPECT_EQ(signEntity->getLineText(0), "Original");
}

TEST_F(SignEntityTest, SetWaxed_PreventsSetLines)
{
    signEntity->setLineFromLegacy(0, "Original");

    signEntity->setWaxed(true);
    std::array<std::unique_ptr<text::ITextComponent>, SignEntity::LINE_COUNT> lines;
    for (auto& line : lines) {
        line = std::make_unique<text::StringTextComponent>("New");
    }
    signEntity->setLines(std::move(lines));
    EXPECT_EQ(signEntity->getLineText(0), "Original");
}

TEST_F(SignEntityTest, SetWaxed_PreventsClearLines)
{
    signEntity->setLineFromLegacy(0, "Original");

    signEntity->setWaxed(true);
    signEntity->clearLines();
    EXPECT_EQ(signEntity->getLineText(0), "Original");
}

TEST_F(SignEntityTest, Unwaxed_AllowsTextModification)
{
    signEntity->setLineFromLegacy(0, "Original");
    EXPECT_TRUE(signEntity->setLineFromLegacy(0, "Modified"));
    EXPECT_EQ(signEntity->getLineText(0), "Modified");
}

TEST_F(SignEntityTest, SaveLoad_PreservesWaxedState)
{
    signEntity->setLineFromLegacy(0, "Test");
    signEntity->setWaxed(true);

    nlohmann::json data;
    signEntity->save(data);
    EXPECT_TRUE(data["is_waxed"].get<bool>());

    auto loaded = std::make_unique<SignEntity>(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded->load(data));
    EXPECT_TRUE(loaded->isWaxed());
}

TEST_F(SignEntityTest, SaveLoad_DefaultWaxedFalse)
{
    signEntity->setLineFromLegacy(0, "Test");

    nlohmann::json data;
    signEntity->save(data);
    // 未涂蜡时保存不应包含 is_waxed 键，或值为 false
    if (data.contains("is_waxed")) {
        EXPECT_FALSE(data["is_waxed"].get<bool>());
    }

    auto loaded = std::make_unique<SignEntity>(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded->load(data));
    EXPECT_FALSE(loaded->isWaxed());
}

TEST_F(SignEntityTest, Clone_PreservesWaxedState)
{
    signEntity->setLineFromLegacy(0, "Test");
    signEntity->setWaxed(true);

    auto copy = signEntity->clone();
    auto* signCopy = dynamic_cast<SignEntity*>(copy.get());
    ASSERT_NE(signCopy, nullptr);
    EXPECT_TRUE(signCopy->isWaxed());
}

// ========== hasEditableText 测试 ==========

TEST_F(SignEntityTest, HasEditableText_NotWaxed_ReturnsTrue)
{
    // 默认未涂蜡，文本可编辑
    EXPECT_TRUE(signEntity->hasEditableText());
}

TEST_F(SignEntityTest, HasEditableText_Waxed_ReturnsFalse)
{
    signEntity->setWaxed(true);
    EXPECT_FALSE(signEntity->hasEditableText());
}

TEST_F(SignEntityTest, HasEditableText_UnwaxRestoresEditable)
{
    signEntity->setWaxed(true);
    EXPECT_FALSE(signEntity->hasEditableText());

    signEntity->setWaxed(false);
    EXPECT_TRUE(signEntity->hasEditableText());
}

TEST_F(SignEntityTest, HasEditableText_InitiallyTrue)
{
    // 新构造的 SignEntity 未涂蜡，hasEditableText 应为 true
    SignEntity freshSign(BlockPos(0, 0, 0));
    EXPECT_TRUE(freshSign.hasEditableText());
}

// ========== 编辑者追踪测试 ==========

TEST_F(SignEntityTest, SetAllowedPlayerEditor_SetsUuid)
{
    signEntity->setAllowedPlayerEditor("test-uuid-123");
    EXPECT_EQ(signEntity->getPlayerWhoMayEdit(), "test-uuid-123");
}

TEST_F(SignEntityTest, SetAllowedPlayerEditor_ClearWithEmptyString)
{
    signEntity->setAllowedPlayerEditor("test-uuid-123");
    EXPECT_EQ(signEntity->getPlayerWhoMayEdit(), "test-uuid-123");

    signEntity->setAllowedPlayerEditor("");
    EXPECT_TRUE(signEntity->getPlayerWhoMayEdit().empty());
}

TEST_F(SignEntityTest, ClearAllowedPlayerEditor_ClearsUuid)
{
    signEntity->setAllowedPlayerEditor("test-uuid-456");
    EXPECT_FALSE(signEntity->getPlayerWhoMayEdit().empty());

    signEntity->clearAllowedPlayerEditor();
    EXPECT_TRUE(signEntity->getPlayerWhoMayEdit().empty());
}

TEST_F(SignEntityTest, OtherPlayerIsEditing_NoEditor_ReturnsFalse)
{
    // 没有编辑者时，otherPlayerIsEditing 对任何玩家都返回 false
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setUuid("player-uuid-1");
    EXPECT_FALSE(signEntity->otherPlayerIsEditing(player));
}

TEST_F(SignEntityTest, OtherPlayerIsEditing_SamePlayer_ReturnsFalse)
{
    // 当前玩家就是编辑者时，otherPlayerIsEditing 返回 false
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setUuid("player-uuid-1");
    signEntity->setAllowedPlayerEditor("player-uuid-1");
    EXPECT_FALSE(signEntity->otherPlayerIsEditing(player));
}

TEST_F(SignEntityTest, OtherPlayerIsEditing_DifferentPlayer_ReturnsTrue)
{
    // 另一个玩家是编辑者时，otherPlayerIsEditing 返回 true
    Player interactingPlayer(EntityInstanceId(2), "InteractingPlayer", mc::test::testEcsRegistry());
    interactingPlayer.setUuid("interacting-uuid");
    signEntity->setAllowedPlayerEditor("editor-uuid");
    EXPECT_TRUE(signEntity->otherPlayerIsEditing(interactingPlayer));
}

TEST_F(SignEntityTest, OtherPlayerIsEditing_AfterClear_ReturnsFalse)
{
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setUuid("player-uuid-1");
    signEntity->setAllowedPlayerEditor("editor-uuid");
    EXPECT_TRUE(signEntity->otherPlayerIsEditing(player));

    signEntity->clearAllowedPlayerEditor();
    EXPECT_FALSE(signEntity->otherPlayerIsEditing(player));
}

TEST_F(SignEntityTest, NeedsTick_NoEditor_ReturnsFalse)
{
    // 没有编辑者时不需要 tick
    EXPECT_FALSE(signEntity->needsTick());
}

TEST_F(SignEntityTest, NeedsTick_WithEditor_ReturnsTrue)
{
    // 有编辑者时需要 tick
    signEntity->setAllowedPlayerEditor("some-uuid");
    EXPECT_TRUE(signEntity->needsTick());

    signEntity->clearAllowedPlayerEditor();
    EXPECT_FALSE(signEntity->needsTick());
}

TEST_F(SignEntityTest, Clone_DoesNotCopyEditor)
{
    // 编辑者状态是运行时瞬态数据，不应被复制
    signEntity->setLineFromLegacy(0, "Test");
    signEntity->setAllowedPlayerEditor("editor-uuid");

    auto copy = signEntity->clone();
    auto* signCopy = dynamic_cast<SignEntity*>(copy.get());
    ASSERT_NE(signCopy, nullptr);
    // 编辑者 UUID 不应被复制到克隆中
    EXPECT_TRUE(signCopy->getPlayerWhoMayEdit().empty());
}

// ========== 需要模拟 IWorld 的测试 ==========

namespace {

/**
 * @brief 测试用世界存根 - 支持 getEntityByUuid 和 getBlockEntity
 *
 * 继承自 BaseTestWorld，添加 UUID→Entity 映射，用于测试
 * SignEntity::playerIsTooFarAwayToEdit 和 SignEntity::tick。
 */
class SignEditorTestWorld final : public mc::test::BaseTestWorld {
public:
    SignEditorTestWorld() = default;

    [[nodiscard]] BlockEntity* getBlockEntity(const BlockPos& pos) override
    {
        auto it = m_blockEntities.find(pos);
        return it != m_blockEntities.end() ? it->second.get() : nullptr;
    }

    [[nodiscard]] const BlockEntity* getBlockEntity(const BlockPos& pos) const override
    {
        auto it = m_blockEntities.find(pos);
        return it != m_blockEntities.end() ? it->second.get() : nullptr;
    }

    void setBlockEntity(const BlockPos& pos, BlockEntity* entity) override
    {
        if (entity != nullptr) {
            m_blockEntities[pos] = std::unique_ptr<BlockEntity>(entity);
        } else {
            m_blockEntities.erase(pos);
        }
    }

    [[nodiscard]] Entity* getEntityByUuid(const std::string& uuid) override
    {
        auto it = m_uuidToEntity.find(uuid);
        return it != m_uuidToEntity.end() ? it->second : nullptr;
    }

    [[nodiscard]] const Entity* getEntityByUuid(const std::string& uuid) const override
    {
        auto it = m_uuidToEntity.find(uuid);
        return it != m_uuidToEntity.end() ? it->second : nullptr;
    }

    void playEvent(i32 eventId, const BlockPos& pos, i32 data) override { m_events.push_back({eventId, pos, data}); }

    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override
    {
        // 测试中忽略音效
    }

    void addParticle(
        particle::ParticleTypeId, const Vector3&, const Vector3&, const Vector3& = Vector3(0, 0, 0), u32 = 1) override
    {
        // 测试中忽略粒子效果
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("SignEditorTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("SignEditorTestWorld::tickManager not implemented");
    }

    /// 注册一个玩家到世界中（用于 getEntityByUuid 查找）
    void registerPlayer(Player& player) { m_uuidToEntity[player.uuid()] = &player; }

    /// 注销一个玩家（模拟离线）
    void unregisterPlayer(const std::string& uuid) { m_uuidToEntity.erase(uuid); }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockEntity>> m_blockEntities;
    std::unordered_map<std::string, Entity*> m_uuidToEntity;

    struct EventRecord {
        i32 eventId;
        BlockPos pos;
        i32 data;
    };
    std::vector<EventRecord> m_events;
};

} // anonymous namespace

// ========== playerIsTooFarAwayToEdit 测试 ==========

class SignEntityEditorTest : public ::testing::Test {
protected:
    void SetUp() override { signEntity = std::make_unique<SignEntity>(BlockPos(10, 64, 20)); }

    SignEditorTestWorld world;
    std::unique_ptr<SignEntity> signEntity;
};

TEST_F(SignEntityEditorTest, PlayerIsTooFarAwayToEdit_PlayerOffline_ReturnsTrue)
{
    // 玩家不在世界中（UUID 查找不到），应返回 true（太远/离线）
    EXPECT_TRUE(signEntity->playerIsTooFarAwayToEdit(world, "offline-uuid"));
}

TEST_F(SignEntityEditorTest, PlayerIsTooFarAwayToEdit_PlayerNearby_ReturnsFalse)
{
    // 玩家在告示牌附近（告示牌位于 10,64,20，中心为 10.5,64.5,20.5）
    Player player(EntityInstanceId(1), "NearbyPlayer", mc::test::testEcsRegistry());
    player.setUuid("nearby-uuid");
    player.setPosition(Vector3(10.5f, 64.5f, 20.5f)); // 在告示牌中心位置
    world.registerPlayer(player);

    EXPECT_FALSE(signEntity->playerIsTooFarAwayToEdit(world, "nearby-uuid"));
}

TEST_F(SignEntityEditorTest, PlayerIsTooFarAwayToEdit_PlayerFarAway_ReturnsTrue)
{
    // 玩家距离告示牌很远（告示牌在 10,64,20，玩家在 100,64,20）
    Player player(EntityInstanceId(1), "FarPlayer", mc::test::testEcsRegistry());
    player.setUuid("far-uuid");
    player.setPosition(Vector3(100.0f, 64.0f, 20.0f));
    world.registerPlayer(player);

    EXPECT_TRUE(signEntity->playerIsTooFarAwayToEdit(world, "far-uuid"));
}

TEST_F(SignEntityEditorTest, PlayerIsTooFarAwayToEdit_PlayerAtExactLimit_ReturnsFalse)
{
    // 玩家恰好在最大交互距离内（MAX_EDIT_DISTANCE = 8.0）
    // 告示牌中心：(10.5, 64.5, 20.5)，玩家位于距离 < 8.0 的位置
    Player player(EntityInstanceId(1), "LimitPlayer", mc::test::testEcsRegistry());
    player.setUuid("limit-uuid");
    player.setPosition(Vector3(10.5f, 64.5f, 25.0f)); // 距离 ≈ 4.5 < 8.0
    world.registerPlayer(player);

    EXPECT_FALSE(signEntity->playerIsTooFarAwayToEdit(world, "limit-uuid"));
}

TEST_F(SignEntityEditorTest, PlayerIsTooFarAwayToEdit_PlayerJustBeyondLimit_ReturnsTrue)
{
    // 玩家刚好超出最大交互距离
    Player player(EntityInstanceId(1), "BeyondPlayer", mc::test::testEcsRegistry());
    player.setUuid("beyond-uuid");
    player.setPosition(Vector3(10.5f, 64.5f, 30.0f)); // 距离 ≈ 9.5 > 8.0
    world.registerPlayer(player);

    EXPECT_TRUE(signEntity->playerIsTooFarAwayToEdit(world, "beyond-uuid"));
}

TEST_F(SignEntityEditorTest, PlayerIsTooFarAwayToEdit_PlayerDisconnected_ReturnsTrue)
{
    // 玩家注册后断开连接（从 UUID 映射中移除）
    Player player(EntityInstanceId(1), "DisconnectPlayer", mc::test::testEcsRegistry());
    player.setUuid("disconnect-uuid");
    player.setPosition(Vector3(10.5f, 64.5f, 20.5f)); // 附近
    world.registerPlayer(player);

    // 连接时应该可编辑
    EXPECT_FALSE(signEntity->playerIsTooFarAwayToEdit(world, "disconnect-uuid"));

    // 断开后应返回 true
    world.unregisterPlayer("disconnect-uuid");
    EXPECT_TRUE(signEntity->playerIsTooFarAwayToEdit(world, "disconnect-uuid"));
}

// ========== tick 自动清除编辑者测试 ==========

TEST_F(SignEntityEditorTest, Tick_ClearsEditorWhenPlayerGoesOffline)
{
    // 设置编辑者
    signEntity->setAllowedPlayerEditor("offline-uuid");
    EXPECT_TRUE(signEntity->needsTick());

    // 玩家不在线（未注册到世界），tick 应清除编辑者
    signEntity->tick(world);
    EXPECT_TRUE(signEntity->getPlayerWhoMayEdit().empty());
    EXPECT_FALSE(signEntity->needsTick());
}

TEST_F(SignEntityEditorTest, Tick_ClearsEditorWhenPlayerMovesTooFar)
{
    // 设置编辑者（附近玩家）
    Player player(EntityInstanceId(1), "EditorPlayer", mc::test::testEcsRegistry());
    player.setUuid("editor-uuid");
    player.setPosition(Vector3(10.5f, 64.5f, 20.5f)); // 在告示牌附近
    world.registerPlayer(player);

    signEntity->setAllowedPlayerEditor("editor-uuid");
    EXPECT_TRUE(signEntity->needsTick());

    // tick 时编辑者在附近，不应被清除
    signEntity->tick(world);
    EXPECT_EQ(signEntity->getPlayerWhoMayEdit(), "editor-uuid");

    // 玩家走远
    player.setPosition(Vector3(100.0f, 64.0f, 20.0f));

    // tick 时编辑者超出范围，应被清除
    signEntity->tick(world);
    EXPECT_TRUE(signEntity->getPlayerWhoMayEdit().empty());
    EXPECT_FALSE(signEntity->needsTick());
}

TEST_F(SignEntityEditorTest, Tick_DoesNotClearEditorWhenPlayerStaysNearby)
{
    // 设置编辑者（附近玩家）
    Player player(EntityInstanceId(1), "EditorPlayer", mc::test::testEcsRegistry());
    player.setUuid("editor-uuid");
    player.setPosition(Vector3(10.5f, 64.5f, 20.5f));
    world.registerPlayer(player);

    signEntity->setAllowedPlayerEditor("editor-uuid");

    // 多次 tick，编辑者一直在附近
    for (int i = 0; i < 10; ++i) {
        signEntity->tick(world);
    }
    EXPECT_EQ(signEntity->getPlayerWhoMayEdit(), "editor-uuid");
    EXPECT_TRUE(signEntity->needsTick());
}

TEST_F(SignEntityEditorTest, Tick_ClearsEditorWhenPlayerDisconnects)
{
    Player player(EntityInstanceId(1), "EditorPlayer", mc::test::testEcsRegistry());
    player.setUuid("editor-uuid");
    player.setPosition(Vector3(10.5f, 64.5f, 20.5f));
    world.registerPlayer(player);

    signEntity->setAllowedPlayerEditor("editor-uuid");

    // tick 时编辑者还在
    signEntity->tick(world);
    EXPECT_EQ(signEntity->getPlayerWhoMayEdit(), "editor-uuid");

    // 玩家断开连接
    world.unregisterPlayer("editor-uuid");

    // tick 时应清除编辑者
    signEntity->tick(world);
    EXPECT_TRUE(signEntity->getPlayerWhoMayEdit().empty());
    EXPECT_FALSE(signEntity->needsTick());
}

TEST_F(SignEntityEditorTest, Tick_NoEditor_DoesNothing)
{
    // 没有编辑者时 tick 应该无操作
    signEntity->tick(world);
    EXPECT_TRUE(signEntity->getPlayerWhoMayEdit().empty());
}

// ========== otherPlayerIsEditing 集成场景测试 ==========

TEST_F(SignEntityEditorTest, OtherPlayerIsEditing_PlayerWithEmptyUuid)
{
    // 玩家 UUID 为空时，otherPlayerIsEditing 应正确处理
    Player player(EntityInstanceId(1), "EmptyUuidPlayer", mc::test::testEcsRegistry());
    // Entity 默认 UUID 为空字符串
    signEntity->setAllowedPlayerEditor("");
    EXPECT_FALSE(signEntity->otherPlayerIsEditing(player));

    signEntity->setAllowedPlayerEditor("some-other-uuid");
    EXPECT_TRUE(signEntity->otherPlayerIsEditing(player));
}

TEST_F(SignEntityEditorTest, OtherPlayerIsEditing_SwitchBetweenPlayers)
{
    Player player1(EntityInstanceId(1), "Player1", mc::test::testEcsRegistry());
    player1.setUuid("uuid-1");
    Player player2(EntityInstanceId(2), "Player2", mc::test::testEcsRegistry());
    player2.setUuid("uuid-2");

    // 无编辑者时，两个玩家都不被阻止
    EXPECT_FALSE(signEntity->otherPlayerIsEditing(player1));
    EXPECT_FALSE(signEntity->otherPlayerIsEditing(player2));

    // player1 编辑中
    signEntity->setAllowedPlayerEditor("uuid-1");
    EXPECT_FALSE(signEntity->otherPlayerIsEditing(player1)); // 同一人
    EXPECT_TRUE(signEntity->otherPlayerIsEditing(player2));  // 其他人

    // 切换到 player2 编辑
    signEntity->setAllowedPlayerEditor("uuid-2");
    EXPECT_TRUE(signEntity->otherPlayerIsEditing(player1));  // 其他人
    EXPECT_FALSE(signEntity->otherPlayerIsEditing(player2)); // 同一人
}
