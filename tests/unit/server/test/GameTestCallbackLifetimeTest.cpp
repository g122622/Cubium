#include "server/test/framework/function/BaseGameTestFunction.hpp"
#include "server/test/framework/function/SyncGameTestRunResult.hpp"
#include "server/test/framework/helper/NullGameTestHelper.hpp"
#include "server/test/framework/instance/BaseGameTestInstance.hpp"
#include "server/test/framework/registry/GameTestRegistry.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

namespace {
class _Function final : public mc::test::BaseGameTestFunction {
public:
    _Function(std::string name, bool scriptBacked, int& releases)
        : BaseGameTestFunction("lifetime", std::move(name), "empty", mc::test::TestData{})
        , m_scriptBacked(scriptBacked)
        , m_releases(releases)
    {}

    std::unique_ptr<mc::test::IGameTestFunctionContext> createContext(mc::test::IGameTestHelper&) const override
    {
        return std::make_unique<mc::test::EmptyGameTestFunctionContext>();
    }
    std::unique_ptr<mc::test::IGameTestFunctionRunResult> run(
        mc::test::IGameTestHelper&, mc::test::IGameTestFunctionContext&) const override
    {
        return std::make_unique<mc::test::SyncGameTestRunResult>(mc::test::pass());
    }
    bool isScriptBacked() const override { return m_scriptBacked; }
    void releaseScriptResources() override { ++m_releases; }

private:
    bool m_scriptBacked;
    int& m_releases;
};

class _HelperProvider final : public mc::test::IGameTestHelperProvider {
public:
    std::unique_ptr<mc::test::IGameTestHelper> createGameTestHelper(mc::test::BaseGameTestInstance&) override
    {
        return std::make_unique<mc::test::NullGameTestHelper>();
    }
    std::unique_ptr<mc::test::IGameTestHelperProvider> clone() const override
    {
        return std::make_unique<_HelperProvider>();
    }
};

class _Instance final : public mc::test::BaseGameTestInstance {
public:
    explicit _Instance(const mc::test::BaseGameTestFunction& function)
        : BaseGameTestInstance(function, std::make_unique<_HelperProvider>())
    {}

protected:
    bool hasStructureBlock() const override { return true; }
    void clearStructure() override {}
    void spawnStructure() override {}
    mc::i32 _getLevelTick() const override { return 0; }
    bool _isTestReady() override { return true; }
};
} // namespace

TEST(GameTestCallbackLifetime, CallbackCanScheduleFutureCallbacksWithoutInvalidatingCurrentTick)
{
    int releases = 0;
    _Function function("scheduled", false, releases);
    _Instance instance(function);
    std::vector<int> executed;
    instance.registerRunAtTickTime(0, [&] {
        executed.push_back(1);
        // 多次扩容覆盖闭包自身及遍历游标失效，未来回调仍需在指定 tick 各执行一次。
        for (int index = 0; index < 256; ++index) {
            instance.registerRunAtTickTime(1, [&, index] {
                executed.push_back(index + 10);
                return mc::test::pass();
            });
        }
        executed.push_back(2);
        return mc::test::pass();
    });
    instance.registerRunAtTickTime(0, [&] {
        executed.push_back(3);
        return mc::test::pass();
    });
    instance.startExecution();
    instance.tick();
    EXPECT_EQ(executed, (std::vector<int>{1, 2, 3}));
    instance.tick();
    ASSERT_EQ(executed.size(), 259U);
    for (int index = 0; index < 256; ++index) {
        EXPECT_EQ(executed[static_cast<std::size_t>(index) + 3], index + 10);
    }
    instance.tick();
    EXPECT_EQ(executed.size(), 259U);
}

TEST(GameTestCallbackLifetime, CompletedCallbackStopsRemainingWorkInTheTick)
{
    int releases = 0;
    _Function function("finished", false, releases);
    _Instance instance(function);
    bool ranAfterCompletion = false;
    instance.registerRunAtTickTime(0, [&] {
        instance.succeed();
        return mc::test::pass();
    });
    instance.registerRunAtTickTime(0, [&] {
        ranAfterCompletion = true;
        return mc::test::pass();
    });
    instance.startExecution();
    instance.tick();
    EXPECT_EQ(instance.state(), mc::test::GameTestState::Succeeded);
    EXPECT_FALSE(ranAfterCompletion);
}

TEST(GameTestCallbackLifetime, ScriptCleanupRemovesAllIndicesAndKeepsNativeRegistration)
{
    auto& registry = mc::test::GameTestRegistry::instance();
    registry.clearAllTestMethods();
    int releases = 0;
    auto script = std::make_shared<_Function>("script", true, releases);
    std::weak_ptr<_Function> lifetime = script;
    ASSERT_TRUE(registry.registerTestMethod("shared-class", script));
    ASSERT_TRUE(registry.registerTestMethod("shared-class", std::make_shared<_Function>("native", false, releases)));
    script.reset();
    registry.releaseAllScriptResources();
    EXPECT_EQ(releases, 1);
    EXPECT_TRUE(lifetime.expired());
    EXPECT_EQ(registry.getTestFunction("script"), nullptr);
    ASSERT_EQ(registry.allTestFunctions().size(), 1U);
    EXPECT_EQ(registry.allTestFunctions().front()->testName(), "native");
    // 脚本重载能再次注册同名条目；重复清理不会访问已擦除的索引节点。
    ASSERT_TRUE(registry.registerTestMethod("shared-class", std::make_shared<_Function>("script", true, releases)));
    registry.releaseAllScriptResources();
    EXPECT_EQ(releases, 2);
    registry.releaseAllScriptResources();
    EXPECT_EQ(releases, 2);
    registry.clearAllTestMethods();
}
