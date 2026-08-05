#include "server/test/minecraft/environment/MinecraftEnvironmentApplier.hpp"

#include "common/test/framework/environment/AllOfEnvironment.hpp"
#include "common/test/framework/environment/FunctionsEnvironment.hpp"
#include "common/test/framework/environment/SetGameRulesEnvironment.hpp"
#include "common/test/framework/environment/TimeOfDayEnvironment.hpp"
#include "common/test/framework/environment/WeatherEnvironment.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "server/core/TimeManager.hpp" // TimeManager::setDayTime
#include "server/world/ServerWorld.hpp"
#include "server/world/weather/WeatherManager.hpp" // WeatherManager::setClear/setRain/setThunder

#include <spdlog/spdlog.h>

namespace mc::test {

namespace {

GameTestResult _applyOneSetup(const TestEnvironmentDefinition& env, mc::server::ServerWorld& world)
{
    if (auto* allOf = dynamic_cast<const AllOfEnvironment*>(&env)) {
        for (const auto& child : allOf->definitions()) {
            if (child) {
                if (GameTestResult r = _applyOneSetup(*child, world); !isPass(r)) {
                    return r;
                }
            }
        }
        return mc::test::pass();
    }
    if (auto* tod = dynamic_cast<const TimeOfDayEnvironment*>(&env)) {
        auto* tm = world.timeManager();
        if (tm == nullptr) {
            return mc::test::fail(
                GameTestErrorType::LevelStateModificationFailed, "TimeOfDayEnvironment: world has no TimeManager");
        }
        tm->setDayTime(static_cast<i64>(tod->time()));
        return mc::test::pass();
    }
    if (auto* weather = dynamic_cast<const WeatherEnvironment*>(&env)) {
        auto* wm = world.weatherManager();
        if (wm == nullptr) {
            return mc::test::fail(
                GameTestErrorType::LevelStateModificationFailed, "WeatherEnvironment: world has no WeatherManager");
        }
        switch (weather->type()) {
            case WeatherEnvironment::Type::Clear:
                wm->setClear(100000);
                break;
            case WeatherEnvironment::Type::Rain:
                wm->setRain(100000);
                break;
            case WeatherEnvironment::Type::Thunder:
                wm->setThunder(100000);
                break;
        }
        return mc::test::pass();
    }
    if (auto* rules = dynamic_cast<const SetGameRulesEnvironment*>(&env)) {
        // TODO: GameRules 用类型化键（BooleanGameRuleKey/IntegerGameRuleKey）而非字符串名，
        // 需建立规则名→键的映射表后调 getGameRules().setBoolean/setInt。当前仅记日志跳过。
        MC_UNUSED(rules);
        spdlog::warn(
            "[GameTest] SetGameRulesEnvironment.applySetup: typed-key mapping not yet implemented, skipping {} rule(s)",
            rules->rules().size());
        return mc::test::pass();
    }
    if (auto* funcs = dynamic_cast<const FunctionsEnvironment*>(&env)) {
        // TODO: .mcfunction 系统未就绪，FunctionsEnvironment 不可应用
        MC_UNUSED(funcs);
        return mc::test::fail(GameTestErrorType::MethodNotImplemented,
            "FunctionsEnvironment.applySetup requires .mcfunction system (not ready)");
    }
    return mc::test::fail(
        GameTestErrorType::MethodNotImplemented, "MinecraftEnvironmentApplier: unknown environment type");
}

GameTestResult _applyOneTeardown(const TestEnvironmentDefinition& env, mc::server::ServerWorld& world)
{
    if (auto* allOf = dynamic_cast<const AllOfEnvironment*>(&env)) {
        // 逆序还原
        for (auto it = allOf->definitions().rbegin(); it != allOf->definitions().rend(); ++it) {
            if (*it) {
                _applyOneTeardown(**it, world);
            }
        }
        return mc::test::pass();
    }
    if (dynamic_cast<const WeatherEnvironment*>(&env) != nullptr) {
        // 对齐 Java Weather.teardown: resetWeatherCycle
        if (auto* wm = world.weatherManager()) {
            wm->resetWeather();
        }
        return mc::test::pass();
    }
    // TimeOfDay/SetGameRules/Functions 无 teardown 语义（对齐 Java）
    return mc::test::pass();
}

} // namespace

GameTestResult MinecraftEnvironmentApplier::applySetup(
    const TestEnvironmentDefinition& env, mc::server::ServerWorld& world)
{
    return _applyOneSetup(env, world);
}

GameTestResult MinecraftEnvironmentApplier::applyTeardown(
    const TestEnvironmentDefinition& env, mc::server::ServerWorld& world)
{
    return _applyOneTeardown(env, world);
}

} // namespace mc::test
