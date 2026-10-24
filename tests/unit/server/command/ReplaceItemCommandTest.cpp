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
 * IMPLIED, INCLUDING A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/**
 * @file ReplaceItemCommandTest.cpp
 * @brief ReplaceItemCommand 单元测试
 *
 * 测试 /replaceitem 命令的注册、解析、权限检查和槽位逻辑，
 * 重点覆盖光标槽位 (player.cursor) 的两种路径：
 * - 有打开的容器菜单时使用 AbstractContainerMenu::setCarriedItem
 * - 无打开的容器菜单时回退到 PlayerInventory::setCarriedItem
 */

#include <gtest/gtest.h>

#include "common/BaseTestServer.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/command/arguments/ItemSlotArgument.hpp"
#include "common/entity/entities/passive/horse/AbstractHorseEntity.hpp"
#include "common/entity/inventory/AbstractContainerMenu.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/item/Items.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/commands/ReplaceItemCommand.hpp"
#include "server/dimension/ServerDimension.hpp"
#include "server/dimension/ServerDimensionManager.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/gen/RandomState.hpp"
#include "server/world/gen/biome/source/MultiNoiseBiomeSource.hpp"
#include "server/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "server/world/gen/settings/DimensionSettings.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"

using mc::server::ServerChunkManager;
using mc::server::ServerPlayerEntityManager;
using mc::server::ServerWorld;
using mc::server::ServerWorldConfig;

namespace mc {
namespace command {

// ============================================================================
// 测试服务器
// ============================================================================

class ReplaceItemTestServer final : public mc::test::BaseTestServer {
public:
    ReplaceItemTestServer() { Items::initialize(); }

    // 覆盖 dimensionManager，返回一个未注册任何维度的空 DimensionManager。
    // 这样 source.world() 经 dimensionManager().getDimension() 返回 nullptr，
    // 命令走 "World not available" 分支返回 0，避免 BaseTestServer 默认实现
    // 抛 std::logic_error 进而在 noexcept 的 world() 中触发 std::terminate。
    [[nodiscard]] ServerDimensionManager& dimensionManager() override { return m_dimensionManager; }

    [[nodiscard]] const ServerDimensionManager& dimensionManager() const override { return m_dimensionManager; }

private:
    // 真实 ServerDimensionManager（nullptr 构造：仅用于 getPlayerDimension 等 map 查询，不调
    // initialize 故不解引用内部 m_server；RelWithDebInfo 下构造断言 MC_ASSERT(server!=nullptr) 不生效）。
    // 替代旧 reinterpret_cast<ServerDimensionManager&>(基类DimensionManager) UB——派生类独有
    // m_playerDimensions 越界读基类内存致 TeleportCommand::teleportPlayers 调 getPlayerDimension 时 SEH。
    ServerDimensionManager m_dimensionManager{nullptr};
};

/// 带真实测试世界的服务器：@e 选择器需要按世界实体列表解析（马匹等非玩家实体目标）。
/// 世界构造与 EntityResolverTest 一致（噪声区块生成器 + 注册到维度管理器）。
class ReplaceItemWorldTestServer final : public mc::test::BaseTestServer {
public:
    ReplaceItemWorldTestServer()
        : m_playerEntityManager()
    {
        VanillaBlocks::initialize();
        entity::VanillaEntities::registerAll();

        ServerWorldConfig config;
        config.viewDistance = 4;
        config.dimension = 0;
        config.seed = 12345;

        auto world = std::make_unique<ServerWorld>(config);
        auto settings = DimensionSettings::overworld();
        auto randomState = world::gen::RandomState::create(settings, config.seed);
        auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false, false);
        auto generator =
            std::make_unique<NoiseChunkGenerator>(std::move(settings), std::move(biomeSource), std::move(randomState));
        world->setChunkManager(std::make_unique<ServerChunkManager>(*world, std::move(generator)));

        auto dimension =
            std::make_unique<ServerDimension>(0, DimensionType::overworld(), config.seed, config.viewDistance);
        m_world = world.get();
        dimension->setWorld(std::move(world));
        (void)m_dimensionManager.registerDimension(std::move(dimension));
    }

    ~ReplaceItemWorldTestServer() override = default;

    [[nodiscard]] ServerDimensionManager& dimensionManager() override { return m_dimensionManager; }
    [[nodiscard]] const ServerDimensionManager& dimensionManager() const override { return m_dimensionManager; }
    [[nodiscard]] ServerPlayerEntityManager& playerEntityManager() override { return m_playerEntityManager; }
    [[nodiscard]] const ServerPlayerEntityManager& playerEntityManager() const override
    {
        return m_playerEntityManager;
    }
    [[nodiscard]] ServerWorld* getPlayerWorld(PlayerId) override { return m_world; }

    EntityInstanceId spawn(std::unique_ptr<Entity> entity) { return m_world->spawnEntity(std::move(entity)); }

private:
    ServerDimensionManager m_dimensionManager{nullptr};
    ServerPlayerEntityManager m_playerEntityManager;
    ServerWorld* m_world = nullptr;
};

// ============================================================================
// 测试 fixture
// ============================================================================

class ReplaceItemCommandTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        ReplaceItemCommand::registerTo(m_server.commandRegistry().dispatcher());
    }

    ReplaceItemTestServer m_server;
    ServerCommandSource m_console = ServerCommandSource::forConsole(&m_server);
};

// ============================================================================
// 命令注册测试
// ============================================================================

TEST_F(ReplaceItemCommandTest, CommandIsRegistered)
{
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    bool found = false;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "replaceitem") {
            found = true;
            break;
        }
    }

    EXPECT_TRUE(found) << "replaceitem command should be registered";
}

TEST_F(ReplaceItemCommandTest, RequiresPermissionLevel2)
{
    ServerCommandSource lowPermSource(&m_server, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 0, 0, "test");

    // 权限不足应无法执行（命令解析因权限不足而抛出异常或返回0）
    bool permissionDenied = false;
    try {
        const auto result =
            m_server.commandRegistry().execute("replaceitem entity @p weapon.mainhand minecraft:stone", lowPermSource);
        permissionDenied = (result.value() == 0);
    }
    catch (...) {
        permissionDenied = true;
    }
    EXPECT_TRUE(permissionDenied) << "replaceitem command should require permission level 2";
}

// ============================================================================
// ItemSlot 光标槽位解析测试
// ============================================================================

TEST_F(ReplaceItemCommandTest, ItemSlot_CursorSlotParsing)
{
    // player.cursor 应解析为 499
    StringReader reader("player.cursor");
    auto slot = ItemSlotArgumentType::itemSlot()->parse(reader);
    EXPECT_EQ(slot.slotIndex(), 499);
    EXPECT_TRUE(slot.isCursorSlot());
}

TEST_F(ReplaceItemCommandTest, ItemSlot_CursorSlotAndHorseChestOverlap)
{
    // player.cursor (499) 和 horse.chest (499) 编号重叠
    StringReader cursorReader("player.cursor");
    auto cursorSlot = ItemSlotArgumentType::itemSlot()->parse(cursorReader);
    EXPECT_EQ(cursorSlot.slotIndex(), 499);

    StringReader chestReader("horse.chest");
    auto chestSlot = ItemSlotArgumentType::itemSlot()->parse(chestReader);
    EXPECT_EQ(chestSlot.slotIndex(), 499);

    // 两者都返回 slotIndex 499，isCursorSlot 和 isHorseChestSlot 都为 true
    EXPECT_TRUE(cursorSlot.isCursorSlot());
    EXPECT_TRUE(cursorSlot.isHorseChestSlot());
    EXPECT_TRUE(chestSlot.isCursorSlot());
    EXPECT_TRUE(chestSlot.isHorseChestSlot());

    // 在玩家上下文中，isCursorSlot 优先匹配
    EXPECT_EQ(cursorSlot.slotIndex(), chestSlot.slotIndex());
}

TEST_F(ReplaceItemCommandTest, ItemSlot_NumericSlot499IsCursor)
{
    // 原版没有纯数字槽位写法；499 只能通过 player.cursor / horse.chest 这两个名字得到。
    StringReader reader("499");
    EXPECT_THROW(ItemSlotArgumentType::itemSlot()->parse(reader), CommandException);

    StringReader named("player.cursor");
    auto slot = ItemSlotArgumentType::itemSlot()->parse(named);
    EXPECT_EQ(slot.slotIndex(), 499);
    EXPECT_TRUE(slot.isCursorSlot());
    EXPECT_TRUE(slot.isHorseChestSlot());
}

// ============================================================================
// ItemSlot 其他槽位解析测试
// ============================================================================

TEST_F(ReplaceItemCommandTest, ItemSlot_EquipmentSlots)
{
    const struct {
        std::string name;
        i32 expectedIndex;
    } equipmentSlots[] = {
        {"weapon.mainhand", 98},
        {"weapon.offhand", 99},
        {"armor.head", 103},
        {"armor.chest", 102},
        {"armor.legs", 101},
        {"armor.feet", 100},
        {"armor.body", 105},
        {"saddle", 106},
    };

    for (const auto& [name, expected] : equipmentSlots) {
        StringReader reader(name);
        auto slot = ItemSlotArgumentType::itemSlot()->parse(reader);
        EXPECT_EQ(slot.slotIndex(), expected) << "Slot '" << name << "' should be " << expected;
        EXPECT_TRUE(slot.isEquipmentSlot()) << "Slot '" << name << "' should be equipment";
    }
}

TEST_F(ReplaceItemCommandTest, ItemSlot_EnderChestSlots)
{
    StringReader reader("enderchest.0");
    auto slot = ItemSlotArgumentType::itemSlot()->parse(reader);
    EXPECT_EQ(slot.slotIndex(), 200);
    EXPECT_TRUE(slot.isEnderChestSlot());

    StringReader reader2("enderchest.26");
    auto slot2 = ItemSlotArgumentType::itemSlot()->parse(reader2);
    EXPECT_EQ(slot2.slotIndex(), 226);
    EXPECT_TRUE(slot2.isEnderChestSlot());
}

TEST_F(ReplaceItemCommandTest, ItemSlot_PlayerInventorySlots)
{
    // 快捷栏 0-8
    StringReader reader("hotbar.4");
    auto slot = ItemSlotArgumentType::itemSlot()->parse(reader);
    EXPECT_EQ(slot.slotIndex(), 4);
    EXPECT_TRUE(slot.isPlayerInventorySlot());

    // 原版没有纯数字槽位写法；背包槽位只用 inventory.N 这类名字
    StringReader reader2("20");
    EXPECT_THROW(ItemSlotArgumentType::itemSlot()->parse(reader2), CommandException);

    StringReader reader3("inventory.20");
    auto slot2 = ItemSlotArgumentType::itemSlot()->parse(reader3);
    EXPECT_EQ(slot2.slotIndex(), 29);
    EXPECT_TRUE(slot2.isPlayerInventorySlot());
}

TEST_F(ReplaceItemCommandTest, ItemSlot_UnsupportedSlotsRecognized)
{
    // 合成槽位
    StringReader craftingReader("player.crafting.0");
    auto craftingSlot = ItemSlotArgumentType::itemSlot()->parse(craftingReader);
    EXPECT_TRUE(craftingSlot.isCraftingSlot());

    // 村民槽位
    StringReader villagerReader("villager.3");
    auto villagerSlot = ItemSlotArgumentType::itemSlot()->parse(villagerReader);
    EXPECT_TRUE(villagerSlot.isVillagerSlot());

    // 马匹槽位
    StringReader horseReader("horse.5");
    auto horseSlot = ItemSlotArgumentType::itemSlot()->parse(horseReader);
    EXPECT_TRUE(horseSlot.isHorseSlot());
}

// ============================================================================
// ItemSlot 无效槽位测试
// ============================================================================

TEST_F(ReplaceItemCommandTest, ItemSlot_InvalidSlotName)
{
    StringReader reader("invalid.slot");
    EXPECT_THROW(auto slot = ItemSlotArgumentType::itemSlot()->parse(reader), CommandException);
}

TEST_F(ReplaceItemCommandTest, ItemSlot_EnderChestOutOfRange)
{
    // enderchest.27 超出范围
    StringReader reader("enderchest.27");
    EXPECT_THROW(auto slot = ItemSlotArgumentType::itemSlot()->parse(reader), CommandException);
}

// ============================================================================
// 非玩家实体目标：马匹库存槽位（对齐 vanilla /item replace entity → AbstractHorse.getSlot）
// ============================================================================

namespace {

/// 创建指定类型的实体并放入测试世界（返回非拥有指针，实体归世界所有）。
Entity* spawnTestEntity(ReplaceItemWorldTestServer& server, const mc::entity::EntityType& type, f32 x, f32 z)
{
    std::unique_ptr<Entity> entity = type.create(nullptr, mc::test::testEcsRegistry());
    if (entity == nullptr) {
        return nullptr;
    }
    entity->setPosition(x, 64.0, z);
    Entity* raw = entity.get();
    server.spawn(std::move(entity));
    return raw;
}

class ReplaceItemEntityTargetTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        ReplaceItemCommand::registerTo(m_server.commandRegistry().dispatcher());
        m_source = ServerCommandSource::forConsole(&m_server);
    }

    i32 runCommand(const std::string& input)
    {
        try {
            const auto result = m_server.commandRegistry().execute(input, m_source);
            return result.success() ? result.value() : 0;
        }
        catch (...) {
            return 0;
        }
    }

    ReplaceItemWorldTestServer m_server;
    ServerCommandSource m_source = ServerCommandSource::forConsole(nullptr);
};

} // namespace

TEST_F(ReplaceItemEntityTargetTest, HorseSaddleSlot_AcceptsSaddle)
{
    Entity* entity = spawnTestEntity(m_server, *entity::VanillaEntityTypeKeys::HORSE, 0.0f, 0.0f);
    ASSERT_NE(entity, nullptr);

    // 原版里 500 不是合法槽位名，用 horse.0（马匹库存第 0 槽 = 鞍槽）
    EXPECT_EQ(runCommand("replaceitem entity @e[type=minecraft:horse,limit=1] horse.0 minecraft:saddle"), 1);

    auto* horse = dynamic_cast<AbstractHorseEntity*>(entity);
    ASSERT_NE(horse, nullptr);
    EXPECT_EQ(horse->getEquipment(0).getItem(), Items::SADDLE);
    EXPECT_TRUE(horse->hasSaddle());
}

TEST_F(ReplaceItemEntityTargetTest, HorseSaddleSlot_RejectsNonSaddle)
{
    Entity* entity = spawnTestEntity(m_server, *entity::VanillaEntityTypeKeys::HORSE, 0.0f, 0.0f);
    ASSERT_NE(entity, nullptr);

    // 鞍槽只接受鞍：普通物品写入失败，命令报错返回 0。
    // 原版没有数字槽位写法；horse.0 是马匹鞍槽，放石头应失败（命令返回 0 条成功）。
    EXPECT_EQ(runCommand("replaceitem entity @e[type=minecraft:horse,limit=1] horse.0 minecraft:stone"), 0);

    auto* horse = dynamic_cast<AbstractHorseEntity*>(entity);
    ASSERT_NE(horse, nullptr);
    EXPECT_TRUE(horse->getEquipment(0).isEmpty());
}

TEST_F(ReplaceItemEntityTargetTest, HorseInventorySlot_OutOfRangeFails)
{
    spawnTestEntity(m_server, *entity::VanillaEntityTypeKeys::HORSE, 0.0f, 0.0f);

    // 普通马库存大小为 2（鞍 + 马铠），514 超出：无任何实体命中槽位。
    EXPECT_EQ(runCommand("replaceitem entity @e[type=minecraft:horse,limit=1] horse.14 minecraft:saddle"), 0);
}

TEST_F(ReplaceItemEntityTargetTest, NonPlayerEntity_EquipmentSlot_IsWritten)
{
    Entity* entity = spawnTestEntity(m_server, *entity::VanillaEntityTypeKeys::ZOMBIE, 0.0f, 0.0f);
    ASSERT_NE(entity, nullptr);

    // 原版 SlotArgument 里 head=103（feet=100）：走命名写法 armor.head。
    EXPECT_EQ(runCommand("replaceitem entity @e[type=minecraft:zombie,limit=1] armor.head minecraft:iron_helmet"), 1);

    auto* living = dynamic_cast<LivingEntity*>(entity);
    ASSERT_NE(living, nullptr);
    EXPECT_EQ(living->getEquipment(mc::EquipmentSlot::Head).getItem(), Items::IRON_HELMET);
}

TEST_F(ReplaceItemEntityTargetTest, NoMatchingEntity_ReturnsZero)
{
    EXPECT_EQ(runCommand("replaceitem entity @e[type=minecraft:horse] horse.0 minecraft:saddle"), 0);
}

} // namespace command
} // namespace mc
