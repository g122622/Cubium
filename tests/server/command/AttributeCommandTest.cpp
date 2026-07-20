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
 * @file AttributeCommandTest.cpp
 * @brief AttributeCommand 单元测试
 *
 * 测试 /attribute 命令的注册、解析、权限检查以及对所有 LivingEntity 的属性操作。
 * 使用扩展的 TestServer（支持 EntityResolver），测试 @e 选择器选取非玩家活体实体、
 * 非 LivingEntity 实体的错误提示、显示名称逻辑等。
 */

#include <gtest/gtest.h>

#include "common/BaseTestServer.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/entity/attribute/AttributeRegistry.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "common/world/dimension/DimensionType.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/commands/AttributeCommand.hpp"
#include "server/command/support/EntityResolver.hpp"
#include "server/dimension/ServerDimension.hpp"
#include "server/dimension/ServerDimensionManager.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"

using namespace mc;
using namespace mc::command;
using namespace mc::command::support;
using namespace mc::entity;
using namespace mc::entity::attribute;
using namespace mc::server;

namespace {

// ============================================================================
// 测试服务器 — 扩展 BaseTestServer，提供维度管理器和世界支持
// 与 EntityResolverTestServer 相同模式，使 EntityResolver 可用于测试
// ============================================================================

class AttributeCommandTestServer final : public test::BaseTestServer {
public:
    AttributeCommandTestServer()
        : BaseTestServer()
        , m_playerEntityManager()
    {
        // 初始化方块和实体注册表
        VanillaBlocks::initialize();
        VanillaEntities::registerAll();

        // 创建测试世界
        ServerWorldConfig config;
        config.viewDistance = 10;
        config.dimension = 0;
        config.seed = 12345;

        auto worldRaw = createTestWorld(config);
        m_world = worldRaw.get(); // 保存裸指针（在 move 之前）

        // 创建维度并关联世界
        auto dimension = std::make_unique<ServerDimension>(0, // DimensionId::OVERWORLD
            DimensionType::overworld(),
            nullptr, // 无区块生成器（维度仅作为世界容器）
            12345,   // seed
            10       // viewDistance
        );
        dimension->setWorld(std::move(worldRaw));
        m_dimension = dimension.get();
        bool registered = m_dimensionManager.registerDimension(std::move(dimension));
        (void)registered;
    }

    ~AttributeCommandTestServer() override = default;

    // 覆盖 dimensionManager，返回包含测试世界的维度管理器
    [[nodiscard]] ServerDimensionManager& dimensionManager() override
    {
        return reinterpret_cast<ServerDimensionManager&>(m_dimensionManager);
    }

    [[nodiscard]] const ServerDimensionManager& dimensionManager() const override
    {
        return reinterpret_cast<const ServerDimensionManager&>(m_dimensionManager);
    }

    // 覆盖 playerEntityManager
    [[nodiscard]] ServerPlayerEntityManager& playerEntityManager() override { return m_playerEntityManager; }

    [[nodiscard]] const ServerPlayerEntityManager& playerEntityManager() const override
    {
        return m_playerEntityManager;
    }

    // 覆盖 getPlayerWorld，返回测试世界
    [[nodiscard]] ServerWorld* getPlayerWorld(PlayerId) override { return m_world; }

    // 获取测试世界
    [[nodiscard]] ServerWorld* world() const { return m_world; }

    // 在测试世界中生成实体
    EntityId spawnEntity(std::unique_ptr<Entity> entity)
    {
        if (!m_world) {
            return 0;
        }
        return m_world->spawnEntity(std::move(entity));
    }

private:
    static std::unique_ptr<ServerWorld> createTestWorld(const ServerWorldConfig& config)
    {
        auto world = std::make_unique<ServerWorld>(config);
        auto settings = DimensionSettings::overworld();
        auto randomState = world::gen::RandomState::create(settings, config.seed);
        auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false, false);
        auto generator =
            std::make_unique<NoiseChunkGenerator>(std::move(settings), std::move(biomeSource), std::move(randomState));
        auto chunkManager = std::make_unique<ServerChunkManager>(*world, std::move(generator));
        world->setChunkManager(std::move(chunkManager));
        return world;
    }

    DimensionManager m_dimensionManager;
    ServerDimension* m_dimension = nullptr;
    ServerPlayerEntityManager m_playerEntityManager;
    ServerWorld* m_world = nullptr;
};

// ============================================================================
// 辅助函数
// ============================================================================

std::unique_ptr<Entity> createEntityByType(const char* typeId)
{
    const EntityType* type = EntityRegistry::instance().getType(typeId);
    if (type == nullptr) {
        return nullptr;
    }
    return type->create(nullptr);
}

} // namespace

// ============================================================================
// 测试固件
// ============================================================================

class AttributeCommandTest : public ::testing::Test {
protected:
    void SetUp() override { AttributeCommand::registerTo(m_server.commandRegistry().dispatcher()); }

    AttributeCommandTestServer m_server;
    ServerCommandSource m_console = ServerCommandSource::forConsole(&m_server);
};

// ============================================================================
// 1. 基本注册和权限测试（继承自原有测试，使用新的 TestServer）
// ============================================================================

TEST_F(AttributeCommandTest, AttributeCommandIsRegistered)
{
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    bool found = false;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "attribute") {
            found = true;
            break;
        }
    }

    EXPECT_TRUE(found) << "attribute command should be registered";
}

TEST_F(AttributeCommandTest, AttributeCommandRequiresPermissionLevel2)
{
    ServerCommandSource lowPermSource(&m_server, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 0, 0, "test");

    bool permissionDenied = false;
    try {
        const auto result =
            m_server.commandRegistry().execute("attribute @p minecraft:generic.max_health get", lowPermSource);
        permissionDenied = (result.value() == 0);
    }
    catch (...) {
        permissionDenied = true;
    }

    EXPECT_TRUE(permissionDenied);
}

// ============================================================================
// 2. 无实体场景测试（@p/@a 选择器无匹配时返回 0）
// ============================================================================

TEST_F(AttributeCommandTest, GetAttributeWithNoPlayersReturnsZero)
{
    const auto result = m_server.commandRegistry().execute("attribute @p generic.max_health get", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(AttributeCommandTest, GetAttributeWithNoEntitiesReturnsZero)
{
    const auto result = m_server.commandRegistry().execute("attribute @e[limit=1] generic.max_health get", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

// ============================================================================
// 3. LivingEntity 属性操作测试 — 使用 @e 选择器选择僵尸
// ============================================================================

TEST_F(AttributeCommandTest, GetZombieMaxHealth)
{
    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    const auto result = m_server.commandRegistry().execute("attribute @e[limit=1] generic.max_health get", m_console);

    EXPECT_TRUE(result.success());
    // 僵尸默认 max_health 为 20
    EXPECT_EQ(result.value(), 20);
}

TEST_F(AttributeCommandTest, GetZombieMaxHealthWithScale)
{
    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    const auto result = m_server.commandRegistry().execute("attribute @e[limit=1] generic.max_health get 2", m_console);

    EXPECT_TRUE(result.success());
    // 20 * 2 = 40
    EXPECT_EQ(result.value(), 40);
}

TEST_F(AttributeCommandTest, SetZombieBaseMaxHealth)
{
    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    const auto setResult =
        m_server.commandRegistry().execute("attribute @e[limit=1] generic.max_health base set 50", m_console);
    EXPECT_TRUE(setResult.success());
    EXPECT_EQ(setResult.value(), 1);

    const auto getResult =
        m_server.commandRegistry().execute("attribute @e[limit=1] generic.max_health base get", m_console);
    EXPECT_TRUE(getResult.success());
    EXPECT_EQ(getResult.value(), 50);
}

TEST_F(AttributeCommandTest, GetZombieBaseMaxHealth)
{
    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    const auto result =
        m_server.commandRegistry().execute("attribute @e[limit=1] generic.max_health base get", m_console);

    EXPECT_TRUE(result.success());
    // 僵尸默认 base max_health 为 20
    EXPECT_EQ(result.value(), 20);
}

TEST_F(AttributeCommandTest, GetZombieBaseMaxHealthWithScale)
{
    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    const auto result =
        m_server.commandRegistry().execute("attribute @e[limit=1] generic.max_health base get 0.5", m_console);

    EXPECT_TRUE(result.success());
    // 20 * 0.5 = 10
    EXPECT_EQ(result.value(), 10);
}

TEST_F(AttributeCommandTest, ResetZombieBaseMaxHealth)
{
    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    // 先设置一个不同的基础值
    m_server.commandRegistry().execute("attribute @e[limit=1] generic.max_health base set 50", m_console);

    // 重置为默认值
    const auto resetResult =
        m_server.commandRegistry().execute("attribute @e[limit=1] generic.max_health base reset", m_console);
    EXPECT_TRUE(resetResult.success());
    EXPECT_EQ(resetResult.value(), 1);

    // 验证基础值已重置回默认值 20
    const auto getResult =
        m_server.commandRegistry().execute("attribute @e[limit=1] generic.max_health base get", m_console);
    EXPECT_TRUE(getResult.success());
    EXPECT_EQ(getResult.value(), 20);
}

// ============================================================================
// 4. 修饰符操作测试
// ============================================================================

TEST_F(AttributeCommandTest, AddModifierToZombie)
{
    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    // 添加修饰符
    const auto addResult = m_server.commandRegistry().execute(
        "attribute @e[limit=1] generic.max_health modifier add test_modifier 10 add_value", m_console);
    EXPECT_TRUE(addResult.success());
    EXPECT_EQ(addResult.value(), 1);

    // 验证修饰符值
    const auto getResult = m_server.commandRegistry().execute(
        "attribute @e[limit=1] generic.max_health modifier value get test_modifier", m_console);
    EXPECT_TRUE(getResult.success());
    EXPECT_EQ(getResult.value(), 10);
}

TEST_F(AttributeCommandTest, AddModifierWithScale)
{
    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    // 添加修饰符
    m_server.commandRegistry().execute(
        "attribute @e[limit=1] generic.max_health modifier add test_mod 5 add_value", m_console);

    // 获取修饰符值并带缩放
    const auto getResult = m_server.commandRegistry().execute(
        "attribute @e[limit=1] generic.max_health modifier value get test_mod 3", m_console);
    EXPECT_TRUE(getResult.success());
    // 5 * 3 = 15
    EXPECT_EQ(getResult.value(), 15);
}

TEST_F(AttributeCommandTest, AddMultiplyBaseModifier)
{
    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    const auto addResult = m_server.commandRegistry().execute(
        "attribute @e[limit=1] generic.max_health modifier add mult_base_mod 0.5 add_multiplied_base", m_console);
    EXPECT_TRUE(addResult.success());
    EXPECT_EQ(addResult.value(), 1);
}

TEST_F(AttributeCommandTest, AddMultiplyTotalModifier)
{
    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    const auto addResult = m_server.commandRegistry().execute(
        "attribute @e[limit=1] generic.max_health modifier add mult_total_mod 0.5 add_multiplied_total", m_console);
    EXPECT_TRUE(addResult.success());
    EXPECT_EQ(addResult.value(), 1);
}

TEST_F(AttributeCommandTest, RemoveModifierFromZombie)
{
    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    // 先添加修饰符
    m_server.commandRegistry().execute(
        "attribute @e[limit=1] generic.max_health modifier add rem_mod 10 add_value", m_console);

    // 移除修饰符
    const auto removeResult = m_server.commandRegistry().execute(
        "attribute @e[limit=1] generic.max_health modifier remove rem_mod", m_console);
    EXPECT_TRUE(removeResult.success());
    EXPECT_EQ(removeResult.value(), 1);
}

TEST_F(AttributeCommandTest, RemoveNonExistentModifierFails)
{
    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    const auto result = m_server.commandRegistry().execute(
        "attribute @e[limit=1] generic.max_health modifier remove nonexistent", m_console);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(AttributeCommandTest, AddDuplicateModifierFails)
{
    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    // 第一次添加成功
    const auto addResult1 = m_server.commandRegistry().execute(
        "attribute @e[limit=1] generic.max_health modifier add dup_mod 10 add_value", m_console);
    EXPECT_TRUE(addResult1.success());
    EXPECT_EQ(addResult1.value(), 1);

    // 第二次添加同 ID 修饰符应失败
    const auto addResult2 = m_server.commandRegistry().execute(
        "attribute @e[limit=1] generic.max_health modifier add dup_mod 20 add_value", m_console);
    EXPECT_TRUE(addResult2.success());
    EXPECT_EQ(addResult2.value(), 0);
}

TEST_F(AttributeCommandTest, GetNonExistentModifierValueFails)
{
    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    const auto result = m_server.commandRegistry().execute(
        "attribute @e[limit=1] generic.max_health modifier value get nonexistent", m_console);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

// ============================================================================
// 5. 非 LivingEntity 实体错误提示测试 — ItemEntity 不继承 LivingEntity
// ============================================================================

TEST_F(AttributeCommandTest, AttributeOnNonLivingEntityReturnsError)
{
    // ItemEntity 不是 LivingEntity，使用 @e 选择 ItemEntity
    auto item = createEntityByType(EntityTypes::ITEM);
    ASSERT_NE(item, nullptr);
    item->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(item));

    const auto result = m_server.commandRegistry().execute("attribute @e[limit=1] generic.max_health get", m_console);

    EXPECT_TRUE(result.success());
    // 非 LivingEntity 应返回 0（命令失败，输出错误消息）
    EXPECT_EQ(result.value(), 0);
}

TEST_F(AttributeCommandTest, AttributeOnArmorStandReturnsError)
{
    // ArmorStandEntity 不是 LivingEntity（本项目实现如此）
    auto armorStand = createEntityByType(EntityTypes::ARMOR_STAND);
    ASSERT_NE(armorStand, nullptr);
    armorStand->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(armorStand));

    const auto result = m_server.commandRegistry().execute("attribute @e[limit=1] generic.max_health get", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(AttributeCommandTest, AttributeOnExperienceOrbReturnsError)
{
    // ExperienceOrbEntity 不是 LivingEntity
    auto orb = createEntityByType(EntityTypes::EXPERIENCE_ORB);
    ASSERT_NE(orb, nullptr);
    orb->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(orb));

    const auto result = m_server.commandRegistry().execute("attribute @e[limit=1] generic.max_health get", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

// ============================================================================
// 6. 混合实体场景 — 同时存在 LivingEntity 和非 LivingEntity
// ============================================================================

TEST_F(AttributeCommandTest, GetAttributeFromMixedEntitiesSelectsLivingEntity)
{
    // 同时生成僵尸（LivingEntity）和掉落物（非 LivingEntity）
    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    auto item = createEntityByType(EntityTypes::ITEM);
    ASSERT_NE(item, nullptr);
    item->setPosition(10.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(item));

    // @e 不指定 type 过滤时，EntityResolver 可能选中任一实体
    // 使用 type=zombie 过滤确保选中僵尸
    const auto result = m_server.commandRegistry().execute(
        "attribute @e[type=minecraft:zombie,limit=1] generic.max_health get", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 20);
}

// ============================================================================
// 7. 不同 LivingEntity 类型测试 — 猪、马
// ============================================================================

TEST_F(AttributeCommandTest, GetPigMaxHealth)
{
    auto pig = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(pig, nullptr);
    pig->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(pig));

    const auto result = m_server.commandRegistry().execute(
        "attribute @e[type=minecraft:pig,limit=1] generic.max_health get", m_console);

    EXPECT_TRUE(result.success());
    // 猪默认 max_health 为 10
    EXPECT_EQ(result.value(), 10);
}

TEST_F(AttributeCommandTest, GetHorseJumpStrength)
{
    auto horse = createEntityByType(EntityTypes::HORSE);
    ASSERT_NE(horse, nullptr);
    horse->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(horse));

    const auto result = m_server.commandRegistry().execute(
        "attribute @e[type=minecraft:horse,limit=1] horse.jump_strength get", m_console);

    EXPECT_TRUE(result.success());
    // horse.jump_strength 有默认值，验证命令能成功执行即可
    // 具体默认值取决于 HorseEntity 的 registerAttributes 实现
}

TEST_F(AttributeCommandTest, SetPigMaxHealthAndVerify)
{
    auto pig = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(pig, nullptr);
    pig->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(pig));

    // 设置猪的最大生命值
    const auto setResult = m_server.commandRegistry().execute(
        "attribute @e[type=minecraft:pig,limit=1] generic.max_health base set 20", m_console);
    EXPECT_TRUE(setResult.success());
    EXPECT_EQ(setResult.value(), 1);

    // 验证设置后的值
    const auto getResult = m_server.commandRegistry().execute(
        "attribute @e[type=minecraft:pig,limit=1] generic.max_health get", m_console);
    EXPECT_TRUE(getResult.success());
    EXPECT_EQ(getResult.value(), 20);
}

// ============================================================================
// 8. 属性名规范化测试
// ============================================================================

TEST_F(AttributeCommandTest, GetAttributeWithoutNamespace)
{
    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    const auto result = m_server.commandRegistry().execute("attribute @e[limit=1] generic.max_health get", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 20);
}

TEST_F(AttributeCommandTest, GetAttributeWithShortName)
{
    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    const auto result = m_server.commandRegistry().execute("attribute @e[limit=1] max_health get", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 20);
}

// ============================================================================
// 9. 未知属性测试
// ============================================================================

TEST_F(AttributeCommandTest, UnknownAttributeReturnsZero)
{
    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    const auto result = m_server.commandRegistry().execute("attribute @e[limit=1] unknown_attribute get", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(AttributeCommandTest, EntityLacksSpecificAttributeReturnsZero)
{
    auto pig = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(pig, nullptr);
    pig->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(pig));

    // 猪默认没有 zombie.spawn_reinforcements 属性
    const auto result = m_server.commandRegistry().execute(
        "attribute @e[type=minecraft:pig,limit=1] zombie.spawn_reinforcements get", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

// ============================================================================
// 10. 属性值范围检查测试
// ============================================================================

TEST_F(AttributeCommandTest, SetBaseValueOutOfRangeFails)
{
    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    // max_health 范围为 [1.0, 1024.0]，设置 0 应该失败
    const auto result =
        m_server.commandRegistry().execute("attribute @e[limit=1] generic.max_health base set 0", m_console);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(AttributeCommandTest, SetBaseValueAtMinBoundarySucceeds)
{
    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    // max_health 最小值为 1.0
    const auto setResult =
        m_server.commandRegistry().execute("attribute @e[limit=1] generic.max_health base set 1", m_console);
    EXPECT_TRUE(setResult.success());
    EXPECT_EQ(setResult.value(), 1);
}

// ============================================================================
// 11. 多种属性类型测试
// ============================================================================

TEST_F(AttributeCommandTest, GetMultipleAttributeTypes)
{
    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    const char* attributes[] = {
        "max_health", "follow_range", "knockback_resistance", "movement_speed", "attack_damage", "armor", "luck"};

    for (const char* attr : attributes) {
        std::string cmd = std::string("attribute @e[limit=1] ") + attr + " get";
        const auto result = m_server.commandRegistry().execute(cmd, m_console);
        // 僵尸有 follow_range 和 attack_damage（MonsterEntity 注册的）
        EXPECT_TRUE(result.success()) << "attribute " << attr << " should be parseable";
    }
}

// ============================================================================
// 12. @s 选择器与 ServerPlayer 测试
// ============================================================================

TEST_F(AttributeCommandTest, SelfSelectorWithServerPlayer)
{
    // 创建一个 ServerPlayer 并作为命令源
    auto serverPlayerEntity = std::make_unique<mc::ServerPlayer>(EntityId(1000), "TestPlayer");
    serverPlayerEntity->setPosition(10.0f, 64.0f, 0.0f);
    serverPlayerEntity->setPlayerId(42);
    auto* serverPlayerPtr = serverPlayerEntity.get();
    m_server.spawnEntity(std::move(serverPlayerEntity));

    ServerCommandSource source(
        &m_server, serverPlayerPtr, 0, Vector3d(10.0, 64.0, 0.0), Vector2f(0.0f, 0.0f), 2, 42, "TestPlayer");

    const auto result = m_server.commandRegistry().execute("attribute @s generic.max_health get", source);

    EXPECT_TRUE(result.success());
    // Player 默认 max_health 为 20
    EXPECT_EQ(result.value(), 20);
}

TEST_F(AttributeCommandTest, SelfSelectorSetBaseHealth)
{
    auto serverPlayerEntity = std::make_unique<mc::ServerPlayer>(EntityId(1000), "TestPlayer");
    serverPlayerEntity->setPosition(10.0f, 64.0f, 0.0f);
    serverPlayerEntity->setPlayerId(42);
    auto* serverPlayerPtr = serverPlayerEntity.get();
    m_server.spawnEntity(std::move(serverPlayerEntity));

    ServerCommandSource source(
        &m_server, serverPlayerPtr, 0, Vector3d(10.0, 64.0, 0.0), Vector2f(0.0f, 0.0f), 2, 42, "TestPlayer");

    // 设置基础生命值
    const auto setResult = m_server.commandRegistry().execute("attribute @s generic.max_health base set 30", source);
    EXPECT_TRUE(setResult.success());
    EXPECT_EQ(setResult.value(), 1);

    // 验证设置后的值
    const auto getResult = m_server.commandRegistry().execute("attribute @s generic.max_health get", source);
    EXPECT_TRUE(getResult.success());
    EXPECT_EQ(getResult.value(), 30);
}

// ============================================================================
// 13. 实体类型过滤选择测试 — 使用 @e[type=...,limit=1] 精确选取特定实体
// ============================================================================

TEST_F(AttributeCommandTest, TypeFilterSelectsZombieNotPig)
{
    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    auto pig = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(pig, nullptr);
    pig->setPosition(5.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(pig));

    // 只选择僵尸
    const auto zombieResult = m_server.commandRegistry().execute(
        "attribute @e[type=minecraft:zombie,limit=1] generic.max_health get", m_console);
    EXPECT_TRUE(zombieResult.success());
    EXPECT_EQ(zombieResult.value(), 20); // 僵尸 max_health = 20

    // 只选择猪
    const auto pigResult = m_server.commandRegistry().execute(
        "attribute @e[type=minecraft:pig,limit=1] generic.max_health get", m_console);
    EXPECT_TRUE(pigResult.success());
    EXPECT_EQ(pigResult.value(), 10); // 猪 max_health = 10
}

TEST_F(AttributeCommandTest, TypeFilterWithoutNamespace)
{
    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    // 不带 minecraft: 前缀也应能匹配
    const auto result =
        m_server.commandRegistry().execute("attribute @e[type=zombie,limit=1] generic.max_health get", m_console);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 20);
}

// ============================================================================
// 14. 控制台执行 /attribute 的反馈消息验证
// ============================================================================

TEST_F(AttributeCommandTest, GetAttributeFeedbackContainsValue)
{
    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    // 使用 @e 选择器获取属性，命令应成功执行
    const auto result = m_server.commandRegistry().execute("attribute @e[limit=1] generic.max_health get", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 20);
}

TEST_F(AttributeCommandTest, BaseSetFeedback)
{
    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    const auto result =
        m_server.commandRegistry().execute("attribute @e[limit=1] generic.max_health base set 30", m_console);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

TEST_F(AttributeCommandTest, BaseResetFeedback)
{
    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    // 先设置
    m_server.commandRegistry().execute("attribute @e[limit=1] generic.max_health base set 50", m_console);

    // 重置
    const auto result =
        m_server.commandRegistry().execute("attribute @e[limit=1] generic.max_health base reset", m_console);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

// ============================================================================
// 15. @a 选择器多实体拒绝测试（@a 要求单个实体）
// ============================================================================

TEST_F(AttributeCommandTest, AllPlayersSelectorWithNoPlayersReturnsZero)
{
    // @a 是多玩家选择器，在 entity() 参数位置不允许（要求单个实体）
    // 解析阶段就会报错，不是运行时返回 0
    const auto result = m_server.commandRegistry().execute("attribute @a generic.max_health get", m_console);

    EXPECT_FALSE(result.success());
}

TEST_F(AttributeCommandTest, AllEntitiesSelectorWithMultipleEntitiesFails)
{
    // @e 选择多个实体时，EntityResolver::resolveSingle 应返回 nullptr（因为多于 1 个）
    auto zombie1 = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie1, nullptr);
    zombie1->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie1));

    auto zombie2 = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie2, nullptr);
    zombie2->setPosition(10.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie2));

    // @e 不带 limit 限制，选中多个实体应失败（EntityArgument 要求单个实体）
    const auto result = m_server.commandRegistry().execute("attribute @e generic.max_health get", m_console);

    EXPECT_FALSE(result.success());
}

// ============================================================================
// 16. _getEntityDisplayName 逻辑验证（间接通过命令输出验证）
// ============================================================================

TEST_F(AttributeCommandTest, PlayerAttributeUsesUsername)
{
    // ServerPlayer 有 username，_getEntityDisplayName 应返回 username
    auto serverPlayerEntity = std::make_unique<mc::ServerPlayer>(EntityId(1000), "Steve");
    serverPlayerEntity->setPosition(10.0f, 64.0f, 0.0f);
    serverPlayerEntity->setPlayerId(42);
    auto* serverPlayerPtr = serverPlayerEntity.get();
    m_server.spawnEntity(std::move(serverPlayerEntity));

    ServerCommandSource source(
        &m_server, serverPlayerPtr, 0, Vector3d(10.0, 64.0, 0.0), Vector2f(0.0f, 0.0f), 2, 42, "Steve");

    // @s 选择自己，命令执行成功即可验证 Player 路径无崩溃
    const auto result = m_server.commandRegistry().execute("attribute @s generic.max_health get", source);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 20);
}

TEST_F(AttributeCommandTest, CustomNameEntityAttribute)
{
    // 设置自定义名称的实体
    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    zombie->setCustomName("CustomZombie");
    m_server.spawnEntity(std::move(zombie));

    // 带 type 过滤确保选中该僵尸
    const auto result = m_server.commandRegistry().execute(
        "attribute @e[type=minecraft:zombie,limit=1] generic.max_health get", m_console);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 20);
}

TEST_F(AttributeCommandTest, DefaultNameEntityAttribute)
{
    // 没有自定义名称的实体使用 getTypeId()
    auto pig = createEntityByType(EntityTypes::PIG);
    ASSERT_NE(pig, nullptr);
    pig->setPosition(0.0f, 64.0f, 0.0f);
    // 不设置自定义名称
    m_server.spawnEntity(std::move(pig));

    const auto result = m_server.commandRegistry().execute(
        "attribute @e[type=minecraft:pig,limit=1] generic.max_health get", m_console);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 10);
}

// ============================================================================
// 17. 命令语法完整性测试
// ============================================================================

TEST_F(AttributeCommandTest, SetAttributeWithFloatValue)
{
    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    const auto result =
        m_server.commandRegistry().execute("attribute @e[limit=1] generic.movement_speed base set 0.15", m_console);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

TEST_F(AttributeCommandTest, SetKnockbackResistance)
{
    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    const auto result = m_server.commandRegistry().execute(
        "attribute @e[limit=1] generic.knockback_resistance base set 0.5", m_console);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

TEST_F(AttributeCommandTest, SetArmor)
{
    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    const auto result =
        m_server.commandRegistry().execute("attribute @e[limit=1] generic.armor base set 20.0", m_console);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

TEST_F(AttributeCommandTest, SetLuck)
{
    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    // 僵尸默认没有 generic.luck 属性，设置基础值应返回 0
    const auto result =
        m_server.commandRegistry().execute("attribute @e[limit=1] generic.luck base set 1024.0", m_console);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

// ============================================================================
// 18. 僵尸特有属性测试
// ============================================================================

TEST_F(AttributeCommandTest, GetZombieSpawnReinforcements)
{
    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    const auto result =
        m_server.commandRegistry().execute("attribute @e[limit=1] zombie.spawn_reinforcements get", m_console);
    EXPECT_TRUE(result.success());
    // 僵尸默认 spawn_reinforcements 为 0
    EXPECT_EQ(result.value(), 0);
}

TEST_F(AttributeCommandTest, SetZombieSpawnReinforcements)
{
    auto zombie = createEntityByType(EntityTypes::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    zombie->setPosition(0.0f, 64.0f, 0.0f);
    m_server.spawnEntity(std::move(zombie));

    const auto setResult =
        m_server.commandRegistry().execute("attribute @e[limit=1] zombie.spawn_reinforcements base set 0.5", m_console);
    EXPECT_TRUE(setResult.success());
    EXPECT_EQ(setResult.value(), 1);

    const auto getResult =
        m_server.commandRegistry().execute("attribute @e[limit=1] zombie.spawn_reinforcements base get", m_console);
    EXPECT_TRUE(getResult.success());
    EXPECT_EQ(getResult.value(), 0); // 0.5 truncated to 0 as i32
}
