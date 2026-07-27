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

#include <gtest/gtest.h>

#include "server/core/ConnectionManager.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/interaction/InventoryManager.hpp"
#include "server/interaction/MiningManager.hpp"
#include "server/world/ServerWorld.hpp"

#include "common/TempDirHelper.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/Items.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/util/UuidUtils.hpp"
#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/world/storage/SingleLevelStorageManager.hpp"
#include "server/world/ServerChunkManager.hpp"

#include <filesystem>
#include <utility>

using namespace mc;

namespace {

/**
 * @brief MiningManager 测试夹具
 */
class MiningManagerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化方块和物品
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();

        // 打开存档：ServerWorld::initialize 要求 m_storage 已设置且 isOpen()。
        // 跨进程唯一目录由 helper 用 PID 组合 token 生成，避免 CTest -j16 同秒目录撞车
        m_testDir = mc::test::makeUniqueTestDir("mc_mining_manager_test");

        world::storage::SingleLevelStorageConfig storageConfig;
        auto openResult = m_storage.open(m_testDir, storageConfig);
        ASSERT_TRUE(openResult.success()) << openResult.error().message();

        // 创建测试世界
        server::ServerWorldConfig config;
        config.viewDistance = 8;
        config.dimension = 0;
        config.seed = 114514;
        // 注意：isDebugWorld 字段已移除，改用 isDebugWorld() 方法通过检测区块生成器类型判断

        m_world = std::make_unique<server::ServerWorld>(config);
        m_world->setSharedStorage(&m_storage);
        // 装配区块管理器（ServerWorld::initialize 亦要求 m_chunkManager != nullptr）
        auto settings = DimensionSettings::overworld();
        auto randomState = world::gen::RandomState::create(settings, config.seed);
        auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false, false);
        auto generator =
            std::make_unique<NoiseChunkGenerator>(std::move(settings), std::move(biomeSource), std::move(randomState));
        auto chunkManager = std::make_unique<server::ServerChunkManager>(*m_world, std::move(generator));
        m_world->setChunkManager(std::move(chunkManager));

        auto worldInit = m_world->initialize();
        ASSERT_TRUE(worldInit.success());

        // 创建玩家管理器
        m_playerManager = std::make_unique<server::core::PlayerManager>();
        // 新网络层 addPlayer 第4参为 ServerClientConnection*；本测试只验证挖矿逻辑，
        // 不依赖连接真发包，故传 nullptr（与 BaseTestServer::addTestPlayer 一致）。
        m_player = m_playerManager->addPlayer(
            m_playerId, mc::util::uuidToString(mc::util::generateOfflineUuid("MiningTester")), "MiningTester", nullptr);
        ASSERT_NE(m_player, nullptr);

        // 设置玩家位置
        m_player->x = 0.5f;
        m_player->y = 64.0f;
        m_player->z = 0.5f;
        m_player->yaw = 0.0f;
        m_player->pitch = 0.0f;
        m_player->gameMode = GameMode::Survival;
        m_player->onGround = true;

        // 创建物品栏管理器
        m_inventoryManager = std::make_unique<server::interaction::InventoryManager>(*m_playerManager);
        m_inventoryManager->initializeInventory(m_playerId);

        // 创建连接管理器
        m_connectionManager = std::make_unique<server::core::ConnectionManager>(*m_playerManager);

        // 创建挖掘管理器
        m_miningManager = std::make_unique<server::interaction::MiningManager>(*m_playerManager, *m_connectionManager);
        m_miningManager->setInventoryManager(m_inventoryManager.get());
    }

    void TearDown() override
    {
        m_miningManager.reset();
        m_inventoryManager.reset();
        m_connectionManager.reset();
        m_playerManager.reset();

        if (m_world) {
            m_world->shutdown();
            m_world.reset();
        }
        m_storage.close();
        mc::test::removeTestDir(m_testDir);
    }

    /**
     * @brief 设置玩家手持物品
     */
    void setHeldItem(const Item& item, i32 count)
    {
        PlayerInventory* inventory = m_inventoryManager->getInventory(m_playerId);
        ASSERT_NE(inventory, nullptr);
        inventory->setSelectedSlot(0);
        inventory->setItem(0, ItemStack(item, count));
    }

    /**
     * @brief 设置玩家手持物品（带附魔）
     */
    void setHeldItemWithEnchantment(const Item& item, i32 count, const std::string& enchantmentId, i32 level)
    {
        PlayerInventory* inventory = m_inventoryManager->getInventory(m_playerId);
        ASSERT_NE(inventory, nullptr);
        inventory->setSelectedSlot(0);
        ItemStack stack(item, count);
        stack.addEnchantment(enchantmentId, level);
        inventory->setItem(0, stack);
    }

    /**
     * @brief 获取玩家数据
     */
    server::ServerPlayerData* getPlayerData() { return m_playerManager->getPlayer(m_playerId); }

protected:
    static constexpr PlayerId m_playerId = 1;
    static constexpr PlayerId m_inventoryId = m_playerId; // 同一个 ID

    std::unique_ptr<server::ServerWorld> m_world;
    std::unique_ptr<server::core::PlayerManager> m_playerManager;
    std::unique_ptr<server::core::ConnectionManager> m_connectionManager;
    std::unique_ptr<server::interaction::InventoryManager> m_inventoryManager;
    std::unique_ptr<server::interaction::MiningManager> m_miningManager;
    server::ServerPlayerData* m_player = nullptr;

    world::storage::SingleLevelStorageManager m_storage;
    std::filesystem::path m_testDir;
};

// ============================================================================
// 基础挖掘测试
// ============================================================================

TEST_F(MiningManagerTest, StartAndAbortMining)
{
    BlockPos pos(0, 63, 0);

    // 开始挖掘
    m_miningManager->startMining(m_playerId, pos, m_playerId);

    EXPECT_TRUE(m_miningManager->isMining(m_playerId));
    EXPECT_FLOAT_EQ(m_miningManager->getMiningProgress(m_playerId), 0.0f);

    auto miningPos = m_miningManager->getMiningPosition(m_playerId);
    EXPECT_TRUE(miningPos.has_value());
    EXPECT_EQ(miningPos.value(), pos);

    // 中止挖掘
    m_miningManager->abortMining(m_playerId);

    EXPECT_FALSE(m_miningManager->isMining(m_playerId));
    EXPECT_FALSE(m_miningManager->getMiningPosition(m_playerId).has_value());
}

TEST_F(MiningManagerTest, CreativeModeInstantBreak)
{
    // 设置创造模式
    m_player->gameMode = GameMode::Creative;

    // 设置石头方块
    m_world->setBlockState(0, 63, 0, &VanillaBlocks::STONE->defaultState());

    // 开始挖掘
    m_miningManager->startMining(m_playerId, BlockPos(0, 63, 0), m_playerId);

    // 创造模式应该瞬间破坏
    // tick 后进度应该达到 1.0
    m_miningManager->tick(*m_world);

    // 在创造模式下，tick 之后应该已经完成挖掘（进度 >= 1.0）
    // 注意：由于 tick 会检查进度并调用回调，所以挖掘状态可能已经变为 false
    // 让我们直接检查进度是否达到 1.0 或挖掘已完成
    EXPECT_TRUE(m_miningManager->getMiningProgress(m_playerId) >= 1.0f || !m_miningManager->isMining(m_playerId));
}

TEST_F(MiningManagerTest, UnbreakableBlockReturnsZeroSpeed)
{
    // 设置基岩（不可破坏）
    m_world->setBlockState(0, 63, 0, &VanillaBlocks::BEDROCK->defaultState());

    m_miningManager->startMining(m_playerId, BlockPos(0, 63, 0), m_playerId);
    m_miningManager->tick(*m_world);

    // 基岩硬度为 -1，应该无法破坏
    // 挖掘进度应该保持为 0
    EXPECT_FLOAT_EQ(m_miningManager->getMiningProgress(m_playerId), 0.0f);
    EXPECT_TRUE(m_miningManager->isMining(m_playerId)); // 仍然在挖掘状态，但进度为 0
}

// ============================================================================
// 挖掘速度计算测试
// ============================================================================

TEST_F(MiningManagerTest, HasteEffectIncreasesMiningSpeed)
{
    // 设置生存模式
    m_player->gameMode = GameMode::Survival;
    m_player->onGround = true;

    // 设置石头方块
    m_world->setBlockState(0, 63, 0, &VanillaBlocks::STONE->defaultState());

    // 设置钻石镐
    const Item* diamondPickaxe = Items::DIAMOND_PICKAXE;
    ASSERT_NE(diamondPickaxe, nullptr);
    setHeldItem(*diamondPickaxe, 1);

    // 记录基础挖掘进度
    m_miningManager->startMining(m_playerId, BlockPos(0, 63, 0), m_playerId);
    m_miningManager->tick(*m_world);
    f32 baseProgress = m_miningManager->getMiningProgress(m_playerId);
    m_miningManager->abortMining(m_playerId);

    // 添加急迫 II 效果 (amplifier = 1)
    m_player->addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Haste, 600, 1, false, true, true));

    // 使用急迫效果后再次挖掘
    m_miningManager->startMining(m_playerId, BlockPos(0, 63, 0), m_playerId);
    m_miningManager->tick(*m_world);
    f32 hasteProgress = m_miningManager->getMiningProgress(m_playerId);

    // 急迫效果应该增加挖掘速度
    // 急迫 II 乘数 = 1 + (1 + 1) * 0.2 = 1.4
    EXPECT_GT(hasteProgress, baseProgress);
    EXPECT_NEAR(hasteProgress, baseProgress * 1.4f, 0.01f);
}

TEST_F(MiningManagerTest, MiningFatigueDecreasesMiningSpeed)
{
    // 设置生存模式
    m_player->gameMode = GameMode::Survival;
    m_player->onGround = true;

    // 设置石头方块
    m_world->setBlockState(0, 63, 0, &VanillaBlocks::STONE->defaultState());

    // 设置钻石镐
    const Item* diamondPickaxe = Items::DIAMOND_PICKAXE;
    ASSERT_NE(diamondPickaxe, nullptr);
    setHeldItem(*diamondPickaxe, 1);

    // 记录基础挖掘进度
    m_miningManager->startMining(m_playerId, BlockPos(0, 63, 0), m_playerId);
    m_miningManager->tick(*m_world);
    f32 baseProgress = m_miningManager->getMiningProgress(m_playerId);
    m_miningManager->abortMining(m_playerId);

    // 添加挖掘疲劳 II 效果 (amplifier = 1)
    m_player->addEffect(
        entity::effect::EffectInstance(entity::effect::EffectType::MiningFatigue, 600, 1, false, true, true));

    // 使用挖掘疲劳效果后再次挖掘
    m_miningManager->startMining(m_playerId, BlockPos(0, 63, 0), m_playerId);
    m_miningManager->tick(*m_world);
    f32 fatigueProgress = m_miningManager->getMiningProgress(m_playerId);

    // 挖掘疲劳应该降低挖掘速度
    // 挖掘疲劳 II 乘数 = 0.09
    EXPECT_LT(fatigueProgress, baseProgress);
    EXPECT_NEAR(fatigueProgress, baseProgress * 0.09f, 0.001f);
}

TEST_F(MiningManagerTest, OffGroundPenalty)
{
    // 设置生存模式
    m_player->gameMode = GameMode::Survival;

    // 设置石头方块
    m_world->setBlockState(0, 63, 0, &VanillaBlocks::STONE->defaultState());

    // 设置钻石镐
    const Item* diamondPickaxe = Items::DIAMOND_PICKAXE;
    ASSERT_NE(diamondPickaxe, nullptr);
    setHeldItem(*diamondPickaxe, 1);

    // 在地面上的挖掘进度
    m_player->onGround = true;
    m_miningManager->startMining(m_playerId, BlockPos(0, 63, 0), m_playerId);
    m_miningManager->tick(*m_world);
    f32 groundProgress = m_miningManager->getMiningProgress(m_playerId);
    m_miningManager->abortMining(m_playerId);

    // 在空中的挖掘进度（应该降低 5 倍）
    m_player->onGround = false;
    m_miningManager->startMining(m_playerId, BlockPos(0, 63, 0), m_playerId);
    m_miningManager->tick(*m_world);
    f32 airProgress = m_miningManager->getMiningProgress(m_playerId);

    // 空中挖掘应该慢 5 倍
    EXPECT_NEAR(airProgress, groundProgress / 5.0f, 0.001f);
}

TEST_F(MiningManagerTest, DifferentToolMaterialsHaveDifferentSpeeds)
{
    // 设置生存模式
    m_player->gameMode = GameMode::Survival;
    m_player->onGround = true;

    // 设置石头方块
    m_world->setBlockState(0, 63, 0, &VanillaBlocks::STONE->defaultState());

    // 木质镐
    const Item* woodPickaxe = Items::WOODEN_PICKAXE;
    ASSERT_NE(woodPickaxe, nullptr);
    setHeldItem(*woodPickaxe, 1);
    m_miningManager->startMining(m_playerId, BlockPos(0, 63, 0), m_playerId);
    m_miningManager->tick(*m_world);
    f32 woodProgress = m_miningManager->getMiningProgress(m_playerId);
    m_miningManager->abortMining(m_playerId);

    // 铁质镐
    const Item* ironPickaxe = Items::IRON_PICKAXE;
    ASSERT_NE(ironPickaxe, nullptr);
    setHeldItem(*ironPickaxe, 1);
    m_miningManager->startMining(m_playerId, BlockPos(0, 63, 0), m_playerId);
    m_miningManager->tick(*m_world);
    f32 ironProgress = m_miningManager->getMiningProgress(m_playerId);
    m_miningManager->abortMining(m_playerId);

    // 钻石镐
    const Item* diamondPickaxe = Items::DIAMOND_PICKAXE;
    ASSERT_NE(diamondPickaxe, nullptr);
    setHeldItem(*diamondPickaxe, 1);
    m_miningManager->startMining(m_playerId, BlockPos(0, 63, 0), m_playerId);
    m_miningManager->tick(*m_world);
    f32 diamondProgress = m_miningManager->getMiningProgress(m_playerId);
    m_miningManager->abortMining(m_playerId);

    // 验证不同材质的挖掘速度顺序
    // 钻石(8.0) > 铁(6.0) > 木(2.0)
    EXPECT_GT(diamondProgress, ironProgress);
    EXPECT_GT(ironProgress, woodProgress);
}

TEST_F(MiningManagerTest, WrongToolIsSlowerThanCorrectTool)
{
    // 设置生存模式
    m_player->gameMode = GameMode::Survival;
    m_player->onGround = true;

    // 设置石头方块（需要镐）
    m_world->setBlockState(0, 63, 0, &VanillaBlocks::STONE->defaultState());

    // 使用正确的工具（钻石镐）
    const Item* diamondPickaxe = Items::DIAMOND_PICKAXE;
    ASSERT_NE(diamondPickaxe, nullptr);
    setHeldItem(*diamondPickaxe, 1);
    m_miningManager->startMining(m_playerId, BlockPos(0, 63, 0), m_playerId);
    m_miningManager->tick(*m_world);
    f32 correctToolProgress = m_miningManager->getMiningProgress(m_playerId);
    m_miningManager->abortMining(m_playerId);

    // 使用错误的工具（钻石剑）
    const Item* diamondSword = Items::DIAMOND_SWORD;
    ASSERT_NE(diamondSword, nullptr);
    setHeldItem(*diamondSword, 1);
    m_miningManager->startMining(m_playerId, BlockPos(0, 63, 0), m_playerId);
    m_miningManager->tick(*m_world);
    f32 wrongToolProgress = m_miningManager->getMiningProgress(m_playerId);
    m_miningManager->abortMining(m_playerId);

    // 正确工具应该更快
    // 正确工具除数 30，错误工具除数 100
    EXPECT_GT(correctToolProgress, wrongToolProgress);
}

// ============================================================================
// 边界情况测试
// ============================================================================

TEST_F(MiningManagerTest, UnknownBlockReturnsInstantBreak)
{
    // 不设置任何方块（空气）
    // 空气方块硬度为 0，应该瞬间破坏
    m_miningManager->startMining(m_playerId, BlockPos(0, 63, 0), m_playerId);
    m_miningManager->tick(*m_world);

    // 空气应该瞬间破坏
    EXPECT_TRUE(m_miningManager->getMiningProgress(m_playerId) >= 1.0f || !m_miningManager->isMining(m_playerId));
}

TEST_F(MiningManagerTest, MultipleMiningSessionsDontConflict)
{
    // 开始挖掘第一个方块
    m_miningManager->startMining(m_playerId, BlockPos(0, 63, 0), m_playerId);
    EXPECT_TRUE(m_miningManager->isMining(m_playerId));
    EXPECT_EQ(m_miningManager->getMiningPosition(m_playerId).value(), BlockPos(0, 63, 0));

    // 开始挖掘另一个方块应该覆盖之前的
    m_miningManager->startMining(m_playerId, BlockPos(1, 63, 0), m_playerId);
    EXPECT_TRUE(m_miningManager->isMining(m_playerId));
    EXPECT_EQ(m_miningManager->getMiningPosition(m_playerId).value(), BlockPos(1, 63, 0));
}

// ============================================================================
// 水下挖掘测试
// ============================================================================

TEST_F(MiningManagerTest, UnderwaterMiningPenalty)
{
    // 设置生存模式
    m_player->gameMode = GameMode::Survival;
    m_player->onGround = true;

    // 设置石头方块
    m_world->setBlockState(0, 63, 0, &VanillaBlocks::STONE->defaultState());

    // 设置钻石镐
    const Item* diamondPickaxe = Items::DIAMOND_PICKAXE;
    ASSERT_NE(diamondPickaxe, nullptr);
    setHeldItem(*diamondPickaxe, 1);

    // 记录在地面上的挖掘进度（玩家眼睛位置约 y + 1.62 = 65.62）
    // 当前玩家在 y=64，眼睛在 65.62，没有水
    m_player->y = 64.0f;
    m_miningManager->startMining(m_playerId, BlockPos(0, 63, 0), m_playerId);
    m_miningManager->tick(*m_world);
    f32 groundProgress = m_miningManager->getMiningProgress(m_playerId);
    m_miningManager->abortMining(m_playerId);

    // 设置水源方块在玩家眼睛位置
    // 玩家 y=64，眼睛高度 1.62，眼睛位置约 65.62
    // 需要在 y=65 处放置水
    m_world->setBlockState(0, 65, 0, &VanillaBlocks::WATER->defaultState());

    // 再次挖掘 - 眼睛在水中的挖掘速度应该降低 5 倍
    m_miningManager->startMining(m_playerId, BlockPos(0, 63, 0), m_playerId);
    m_miningManager->tick(*m_world);
    f32 underwaterProgress = m_miningManager->getMiningProgress(m_playerId);

    // 水下挖掘速度应该是地面上的 1/5
    EXPECT_NEAR(underwaterProgress, groundProgress / 5.0f, 0.001f);
}

TEST_F(MiningManagerTest, AquaAffinityNegatesUnderwaterPenalty)
{
    // 设置生存模式
    m_player->gameMode = GameMode::Survival;
    m_player->onGround = true;

    // 设置石头方块
    m_world->setBlockState(0, 63, 0, &VanillaBlocks::STONE->defaultState());

    // 设置带有水下速掘附魔的头盔
    const Item* diamondHelmet = Items::DIAMOND_HELMET;
    ASSERT_NE(diamondHelmet, nullptr);
    PlayerInventory* inventory = m_inventoryManager->getInventory(m_playerId);
    ASSERT_NE(inventory, nullptr);
    ItemStack helmetStack(*diamondHelmet, 1);
    helmetStack.addEnchantment("minecraft:aqua_affinity", 1);
    inventory->setHelmet(helmetStack);

    // 设置钻石镐
    const Item* diamondPickaxe = Items::DIAMOND_PICKAXE;
    ASSERT_NE(diamondPickaxe, nullptr);
    setHeldItem(*diamondPickaxe, 1);

    // 记录在地面上的挖掘进度
    m_player->y = 64.0f;
    m_miningManager->startMining(m_playerId, BlockPos(0, 63, 0), m_playerId);
    m_miningManager->tick(*m_world);
    f32 groundProgress = m_miningManager->getMiningProgress(m_playerId);
    m_miningManager->abortMining(m_playerId);

    // 设置水源方块在玩家眼睛位置
    m_world->setBlockState(0, 65, 0, &VanillaBlocks::WATER->defaultState());

    // 水下速掘附魔应该抵消水下挖掘惩罚
    m_miningManager->startMining(m_playerId, BlockPos(0, 63, 0), m_playerId);
    m_miningManager->tick(*m_world);
    f32 underwaterWithAquaAffinity = m_miningManager->getMiningProgress(m_playerId);

    // 有水下速掘附魔时，水下挖掘速度应该接近地面速度
    EXPECT_NEAR(underwaterWithAquaAffinity, groundProgress, 0.001f);
}

TEST_F(MiningManagerTest, EyesPositionDetectionInWater)
{
    // 玩家位置 y=64，眼睛高度 1.62
    // 眼睛位置 = 64 + 1.62 = 65.62
    // 检测点向下偏移 0.11，所以检测点 Y = 65.51

    // 测试 1: 眼睛不在水中（没有水方块）
    m_player->y = 64.0f;
    m_player->onGround = true;

    // 不放置水方块
    m_world->setBlockState(0, 63, 0, &VanillaBlocks::STONE->defaultState());

    // 设置钻石镐
    const Item* diamondPickaxe = Items::DIAMOND_PICKAXE;
    ASSERT_NE(diamondPickaxe, nullptr);
    setHeldItem(*diamondPickaxe, 1);

    // 记录基准挖掘速度
    m_miningManager->startMining(m_playerId, BlockPos(0, 63, 0), m_playerId);
    m_miningManager->tick(*m_world);
    f32 baseProgress = m_miningManager->getMiningProgress(m_playerId);
    m_miningManager->abortMining(m_playerId);

    // 测试 2: 眼睛在水中（在 y=65 放置水源）
    m_world->setBlockState(0, 65, 0, &VanillaBlocks::WATER->defaultState());

    m_miningManager->startMining(m_playerId, BlockPos(0, 63, 0), m_playerId);
    m_miningManager->tick(*m_world);
    f32 inWaterProgress = m_miningManager->getMiningProgress(m_playerId);

    // 眼睛在水中时挖掘速度应该降低 5 倍
    EXPECT_NEAR(inWaterProgress, baseProgress / 5.0f, 0.001f);
}

TEST_F(MiningManagerTest, OffGroundAndUnderwaterPenaltiesStack)
{
    // 设置生存模式
    m_player->gameMode = GameMode::Survival;

    // 设置石头方块
    m_world->setBlockState(0, 63, 0, &VanillaBlocks::STONE->defaultState());

    // 设置钻石镐
    const Item* diamondPickaxe = Items::DIAMOND_PICKAXE;
    ASSERT_NE(diamondPickaxe, nullptr);
    setHeldItem(*diamondPickaxe, 1);

    // 在地面且不在水中的挖掘进度
    m_player->y = 64.0f;
    m_player->onGround = true;
    m_miningManager->startMining(m_playerId, BlockPos(0, 63, 0), m_playerId);
    m_miningManager->tick(*m_world);
    f32 baseProgress = m_miningManager->getMiningProgress(m_playerId);
    m_miningManager->abortMining(m_playerId);

    // 设置水源方块
    m_world->setBlockState(0, 65, 0, &VanillaBlocks::WATER->defaultState());

    // 既不在地面，眼睛又在水中
    m_player->onGround = false;
    m_miningManager->startMining(m_playerId, BlockPos(0, 63, 0), m_playerId);
    m_miningManager->tick(*m_world);
    f32 stackedPenaltyProgress = m_miningManager->getMiningProgress(m_playerId);

    // 两种惩罚应该叠加：水下 /5，空中 /5 = 总共 /25
    EXPECT_NEAR(stackedPenaltyProgress, baseProgress / 25.0f, 0.0001f);
}

} // namespace
