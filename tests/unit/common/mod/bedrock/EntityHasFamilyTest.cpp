#include <gtest/gtest.h>

#include "common/entity/core/Entity.hpp"
#include "common/entity/ecs/context/EntityRegistry.hpp"
#include "common/mod/bedrock/addon/content/entity/definition/AddonEntityDefinition.hpp"
#include "common/mod/bedrock/addon/content/entity/filters/base/VanillaEntityFamilies.hpp"

namespace {

using mc::Entity;
using mc::EntityInstanceId;
using mc::mod::bedrock::addon::AddonEntityDefinition;
using mc::mod::bedrock::addon::entityHasFamily;

/**
 * @brief 用一份最小 addon 实体定义构造实体，供家族判定使用
 *
 * type_family 既可能是字符串也可能是数组，官方 schema 两种都接受，
 * 所以这里把 family 直接作为 JSON 传进来。
 */
class EntityHasFamilyTest : public testing::Test {
protected:
    std::unique_ptr<Entity> createWithFamily(const nlohmann::json& family)
    {
        nlohmann::json document = {{"format_version", "1.21.0"},
            {"minecraft:entity",
                {{"description", {{"identifier", "test:family_mob"}, {"is_spawnable", false}}},
                    {"components", {{"minecraft:type_family", {{"family", family}}}}}}}};

        auto parsed = AddonEntityDefinition::parse(document);
        EXPECT_TRUE(parsed.success()) << parsed.error().message();
        if (parsed.failed()) {
            return nullptr;
        }
        return AddonEntityDefinition::create(std::make_shared<const AddonEntityDefinition>(std::move(parsed.value())),
            nullptr,
            m_registry,
            std::weak_ptr<mc::mod::bedrock::addon::EntityEventSink>{});
    }

    mc::ecs::EntityRegistry m_registry{"entity-family-test"};
};

} // namespace

// 普通实体既不是 mob 也不是 player，只能命中原版家族表或 addon 声明。
TEST(EntityHasFamilyBareTest, PlayerAndMobFallbacksAreFalseForBareEntity)
{
    mc::ecs::EntityRegistry registry{"entity-family-test"};
    Entity entity{EntityInstanceId(1), nullptr, registry};

    EXPECT_FALSE(entityHasFamily(entity, "mob")) << "a bare Entity is neither mob nor player";
    EXPECT_FALSE(entityHasFamily(entity, "player"));
}

// 未知家族名必须稳定返回 false，而不是抛错或断言崩溃——
// 行为包里的家族名不受本项目控制。
TEST(EntityHasFamilyBareTest, UnknownFamilyIsFalseNotAnError)
{
    mc::ecs::EntityRegistry registry{"entity-family-test"};
    Entity entity{EntityInstanceId(1), nullptr, registry};

    EXPECT_FALSE(entityHasFamily(entity, "definitely_not_a_family"));
    EXPECT_FALSE(entityHasFamily(entity, ""));
}

// addon 实体自己声明的 minecraft:type_family 优先级最高：
// 即使原版家族表里查不到，也必须按声明命中。
TEST_F(EntityHasFamilyTest, AddonTypeFamilyAcceptsStringForm)
{
    auto entity = createWithFamily("my_custom_family");
    ASSERT_NE(entity, nullptr);

    EXPECT_TRUE(entityHasFamily(*entity, "my_custom_family"));
    EXPECT_FALSE(entityHasFamily(*entity, "my_other_family"));
}

TEST_F(EntityHasFamilyTest, AddonTypeFamilyAcceptsArrayForm)
{
    auto entity = createWithFamily(nlohmann::json::array({"alpha", "beta"}));
    ASSERT_NE(entity, nullptr);

    EXPECT_TRUE(entityHasFamily(*entity, "alpha"));
    EXPECT_TRUE(entityHasFamily(*entity, "beta"));
    EXPECT_FALSE(entityHasFamily(*entity, "gamma"));
}
