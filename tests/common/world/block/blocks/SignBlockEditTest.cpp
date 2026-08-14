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

/**
 * @file SignBlockEditTest.cpp
 * @brief 告示牌编辑流程核心逻辑测试
 *
 * 测试 handleUpdateSignPacket 和 SignBlock::onBlockActivated 所依赖的
 * SignEntity 核心业务逻辑路径：
 * - 编辑者校验：setAllowedPlayerEditor / getPlayerWhoMayEdit / clearAllowedPlayerEditor
 * - 涂蜡校验：isWaxed 阻止文本修改
 * - 文本更新：setLineFromLegacy 正确更新行内容
 * - hasEditableText：涂蜡告示牌不可编辑
 * - 完整编辑流程：设置编辑锁 → 校验 → 更新文本 → 清除编辑锁
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "util/text/StringTextComponent.hpp"
#include "world/block/BlockPos.hpp"
#include "world/blockentity/BlockEntity.hpp"
#include "world/blockentity/BlockEntityType.hpp"
#include "world/blockentity/core/BlockEntityRegistry.hpp"
#include "world/blockentity/interactive/SignEntity.hpp"

#include <memory>
#include <unordered_map>

using namespace mc;
using namespace mc::blockentity;

namespace {

/// 测试用世界，支持 getBlockEntity / setBlockEntity / getEntityByUuid
class SignEditTestWorld final : public mc::test::BaseTestWorld {
public:
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

    void playEvent(i32 eventId, const BlockPos& pos, i32 data) override { MC_UNUSED(eventId, pos, data); }

    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override {}

    void addParticle(particle::ParticleTypeId, const Vector3&, const Vector3&, const Vector3&, u32) override {}

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("SignEditTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("SignEditTestWorld::tickManager not implemented");
    }

    void registerPlayer(Player& player) { m_uuidToEntity[player.uuid()] = &player; }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockEntity>> m_blockEntities;
    std::unordered_map<std::string, Entity*> m_uuidToEntity;
};

} // namespace

// ============================================================================
// 编辑者校验测试（handleUpdateSignPacket 编辑者校验路径）
// ============================================================================

class SignEditorLogicTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        BlockEntityRegistry::instance().registerBuiltinTypes();
        m_signEntity = std::make_unique<SignEntity>(BlockPos(10, 64, 20));
    }

    std::unique_ptr<SignEntity> m_signEntity;
};

TEST_F(SignEditorLogicTest, SetAllowedPlayerEditor_SetsUuid)
{
    m_signEntity->setAllowedPlayerEditor("player-uuid");
    EXPECT_EQ(m_signEntity->getPlayerWhoMayEdit(), "player-uuid");
}

TEST_F(SignEditorLogicTest, ClearAllowedPlayerEditor_ClearsUuid)
{
    m_signEntity->setAllowedPlayerEditor("player-uuid");
    m_signEntity->clearAllowedPlayerEditor();
    EXPECT_TRUE(m_signEntity->getPlayerWhoMayEdit().empty());
}

TEST_F(SignEditorLogicTest, EditorCheck_MatchingUuid_AllowsUpdate)
{
    // 模拟 handleUpdateSignPacket 的编辑者校验：UUID 匹配时允许更新
    const std::string editorUuid = "player-uuid";
    m_signEntity->setAllowedPlayerEditor(editorUuid);

    // handleUpdateSignPacket 中的检查：getPlayerWhoMayEdit() == player->uuid
    EXPECT_EQ(m_signEntity->getPlayerWhoMayEdit(), editorUuid);

    // 通过校验后更新文本
    m_signEntity->setLineFromLegacy(0, "Hello");
    m_signEntity->setLineFromLegacy(1, "World");
    EXPECT_EQ(m_signEntity->getLineText(0), "Hello");
    EXPECT_EQ(m_signEntity->getLineText(1), "World");
}

TEST_F(SignEditorLogicTest, EditorCheck_MismatchedUuid_RejectsUpdate)
{
    // 模拟 handleUpdateSignPacket 的编辑者校验：UUID 不匹配时不更新
    m_signEntity->setAllowedPlayerEditor("editor-uuid");
    const std::string otherUuid = "other-uuid";

    // handleUpdateSignPacket 中的检查逻辑
    if (m_signEntity->getPlayerWhoMayEdit() != otherUuid) {
        // 不执行文本更新，直接返回
    } else {
        FAIL() << "Should not reach update when UUID mismatches";
    }

    // 文本应仍为空
    EXPECT_TRUE(m_signEntity->getLineText(0).empty());
}

// ============================================================================
// 涂蜡校验测试（handleUpdateSignPacket 涂蜡校验路径）
// ============================================================================

TEST_F(SignEditorLogicTest, WaxedCheck_PreventsTextUpdate)
{
    // 模拟 handleUpdateSignPacket 的涂蜡校验：涂蜡时清除编辑锁并返回
    m_signEntity->setAllowedPlayerEditor("player-uuid");
    m_signEntity->setWaxed(true);

    // handleUpdateSignPacket 中的涂蜡检查逻辑
    if (m_signEntity->isWaxed()) {
        m_signEntity->clearAllowedPlayerEditor();
        // 不执行文本更新，直接返回
    } else {
        FAIL() << "Should not reach update when waxed";
    }

    // 编辑锁应被清除
    EXPECT_TRUE(m_signEntity->getPlayerWhoMayEdit().empty());
    // 文本应仍为空
    EXPECT_TRUE(m_signEntity->getLineText(0).empty());
}

TEST_F(SignEditorLogicTest, NotWaxed_AllowsTextUpdate)
{
    m_signEntity->setAllowedPlayerEditor("player-uuid");

    // handleUpdateSignPacket 中的涂蜡检查逻辑：未涂蜡时继续
    if (!m_signEntity->isWaxed()) {
        m_signEntity->setLineFromLegacy(0, "Updated");
    }

    EXPECT_EQ(m_signEntity->getLineText(0), "Updated");
}

// ============================================================================
// 文本更新测试（handleUpdateSignPacket 文本更新路径）
// ============================================================================

TEST_F(SignEditorLogicTest, TextUpdate_AllFourLinesUpdated)
{
    // 模拟 handleUpdateSignPacket 的文本更新逻辑
    m_signEntity->setAllowedPlayerEditor("player-uuid");

    std::array<std::string, 4> newLines = {"Line1", "Line2", "Line3", "Line4"};
    for (i32 i = 0; i < 4; ++i) {
        m_signEntity->setLineFromLegacy(i, newLines[static_cast<size_t>(i)]);
    }

    for (i32 i = 0; i < 4; ++i) {
        EXPECT_EQ(m_signEntity->getLineText(i), newLines[static_cast<size_t>(i)]);
    }

    // 更新后清除编辑锁（handleUpdateSignPacket 最后一步）
    m_signEntity->clearAllowedPlayerEditor();
    EXPECT_TRUE(m_signEntity->getPlayerWhoMayEdit().empty());
}

TEST_F(SignEditorLogicTest, TextUpdate_OverwritesExistingText)
{
    // 先设置初始文本
    m_signEntity->setLineFromLegacy(0, "Old Text");
    m_signEntity->setLineFromLegacy(1, "Old Line 2");

    // 模拟 handleUpdateSignPacket 覆盖更新
    std::array<std::string, 4> newLines = {"New", "", "", ""};
    for (i32 i = 0; i < 4; ++i) {
        m_signEntity->setLineFromLegacy(i, newLines[static_cast<size_t>(i)]);
    }

    EXPECT_EQ(m_signEntity->getLineText(0), "New");
    EXPECT_TRUE(m_signEntity->getLineText(1).empty());
    EXPECT_TRUE(m_signEntity->getLineText(2).empty());
    EXPECT_TRUE(m_signEntity->getLineText(3).empty());
}

// ============================================================================
// hasEditableText 测试（SignBlock::onBlockActivated 中的编辑前置检查）
// ============================================================================

TEST_F(SignEditorLogicTest, HasEditableText_NotWaxed_ReturnsTrue)
{
    EXPECT_TRUE(m_signEntity->hasEditableText());
}

TEST_F(SignEditorLogicTest, HasEditableText_Waxed_ReturnsFalse)
{
    m_signEntity->setWaxed(true);
    EXPECT_FALSE(m_signEntity->hasEditableText());
}

// ============================================================================
// 完整编辑流程集成测试
// ============================================================================

TEST_F(SignEditorLogicTest, FullUpdateSignFlow_Success)
{
    // 完整模拟 handleUpdateSignPacket 的成功路径：
    // 1. 编辑者匹配 → 2. 未涂蜡 → 3. 更新文本 → 4. 清除编辑锁
    const std::string editorUuid = "player-uuid";
    m_signEntity->setAllowedPlayerEditor(editorUuid);

    // 步骤1：编辑者校验
    EXPECT_EQ(m_signEntity->getPlayerWhoMayEdit(), editorUuid);

    // 步骤2：涂蜡校验
    EXPECT_FALSE(m_signEntity->isWaxed());

    // 步骤3：更新4行文本
    std::array<std::string, 4> newLines = {"A", "B", "C", "D"};
    for (i32 i = 0; i < 4; ++i) {
        m_signEntity->setLineFromLegacy(i, newLines[static_cast<size_t>(i)]);
    }

    // 步骤4：清除编辑锁
    m_signEntity->clearAllowedPlayerEditor();

    // 验证最终状态
    for (i32 i = 0; i < 4; ++i) {
        EXPECT_EQ(m_signEntity->getLineText(i), newLines[static_cast<size_t>(i)]);
    }
    EXPECT_TRUE(m_signEntity->getPlayerWhoMayEdit().empty());
}

TEST_F(SignEditorLogicTest, FullUpdateSignFlow_Waxed_Rejected)
{
    // 完整模拟 handleUpdateSignPacket 的涂蜡拒绝路径
    m_signEntity->setAllowedPlayerEditor("player-uuid");
    m_signEntity->setWaxed(true);

    // 步骤1：编辑者校验通过
    // 步骤2：涂蜡校验失败 → 清除编辑锁，不更新文本
    if (m_signEntity->isWaxed()) {
        m_signEntity->clearAllowedPlayerEditor();
        return;
    }

    FAIL() << "Should have returned early due to waxed";
}

TEST_F(SignEditorLogicTest, FullUpdateSignFlow_WrongEditor_Rejected)
{
    // 完整模拟 handleUpdateSignPacket 的编辑者不匹配拒绝路径
    m_signEntity->setAllowedPlayerEditor("editor-uuid");
    const std::string requestingUuid = "other-uuid";

    // 编辑者校验失败 → 不更新文本，直接返回
    if (m_signEntity->getPlayerWhoMayEdit() != requestingUuid) {
        // 验证文本未被修改
        EXPECT_TRUE(m_signEntity->getLineText(0).empty());
        return;
    }

    FAIL() << "Should have returned early due to editor mismatch";
}

// ============================================================================
// otherPlayerIsEditing 测试（SignBlock::onBlockActivated 中的并发编辑检查）
// ============================================================================

class SignEditorConcurrencyTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        BlockEntityRegistry::instance().registerBuiltinTypes();
        m_signEntity = std::make_unique<SignEntity>(BlockPos(10, 64, 20));

        m_player1 = std::make_unique<Player>(EntityInstanceId(1), "Player1", mc::test::testEcsRegistry());
        m_player1->setUuid("uuid-1");
        m_player1->setPosition(Vector3(10.5f, 64.5f, 20.5f));
        m_world.registerPlayer(*m_player1);

        m_player2 = std::make_unique<Player>(EntityInstanceId(2), "Player2", mc::test::testEcsRegistry());
        m_player2->setUuid("uuid-2");
        m_player2->setPosition(Vector3(11.5f, 64.5f, 20.5f));
        m_world.registerPlayer(*m_player2);
    }

    SignEditTestWorld m_world;
    std::unique_ptr<SignEntity> m_signEntity;
    std::unique_ptr<Player> m_player1;
    std::unique_ptr<Player> m_player2;
};

TEST_F(SignEditorConcurrencyTest, OtherPlayerIsEditing_NoEditor_ReturnsFalse)
{
    // 没有设置编辑者时，任何玩家都不算"其他玩家在编辑"
    EXPECT_FALSE(m_signEntity->otherPlayerIsEditing(*m_player1));
    EXPECT_FALSE(m_signEntity->otherPlayerIsEditing(*m_player2));
}

TEST_F(SignEditorConcurrencyTest, OtherPlayerIsEditing_SamePlayer_ReturnsFalse)
{
    // player1 是编辑者，player1 自己检查应返回 false
    m_signEntity->setAllowedPlayerEditor(m_player1->uuid());
    EXPECT_FALSE(m_signEntity->otherPlayerIsEditing(*m_player1));
}

TEST_F(SignEditorConcurrencyTest, OtherPlayerIsEditing_DifferentPlayer_ReturnsTrue)
{
    // player1 是编辑者，player2 检查应返回 true
    m_signEntity->setAllowedPlayerEditor(m_player1->uuid());
    EXPECT_TRUE(m_signEntity->otherPlayerIsEditing(*m_player2));
}
