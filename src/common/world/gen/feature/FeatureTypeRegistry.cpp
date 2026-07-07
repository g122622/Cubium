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

#include "FeatureTypeRegistry.hpp"

#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/MonsterRoomFeature.hpp"

#include <nlohmann/json.hpp>

namespace mc {
namespace world::gen::feature {

namespace {

/**
 * @brief 剥离 "minecraft:" 命名空间前缀
 */
std::string stripNamespace(const std::string& s)
{
    constexpr std::string_view prefix = "minecraft:";
    if (s.size() > prefix.size() && s.substr(0, prefix.size()) == prefix) {
        return s.substr(prefix.size());
    }
    return s;
}

/**
 * @brief monster_room 工厂：config 为空，构造 ConfiguredMonsterRoomFeature
 */
Result<std::unique_ptr<ConfiguredFeatureBase>> createMonsterRoom(const nlohmann::json& /*configJson*/)
{
    std::unique_ptr<ConfiguredFeatureBase> feature = std::make_unique<ConfiguredMonsterRoomFeature>();
    return feature;
}

} // namespace

FeatureTypeRegistry& FeatureTypeRegistry::instance()
{
    static FeatureTypeRegistry s_instance;
    return s_instance;
}

void FeatureTypeRegistry::registerType(const std::string& type, Factory factory)
{
    m_factories[stripNamespace(type)] = std::move(factory);
}

Result<std::unique_ptr<ConfiguredFeatureBase>> FeatureTypeRegistry::create(
    const std::string& type, const nlohmann::json& configJson) const
{
    const std::string key = stripNamespace(type);
    const auto it = m_factories.find(key);
    if (it == m_factories.end()) {
        return Error(ErrorCode::NotFound,
            "Unregistered configured_feature type: '" + type +
                "'. This feature type has no C++ implementation yet; "
                "implement it and register in FeatureTypeRegistry.");
    }
    return it->second(configJson);
}

bool FeatureTypeRegistry::has(const std::string& type) const noexcept
{
    return m_factories.find(stripNamespace(type)) != m_factories.end();
}

void FeatureTypeRegistry::clear() noexcept
{
    m_factories.clear();
}

/**
 * @brief 初始化内置特征类型工厂
 *
 * 注册当前已实现的 feature type。未注册的 type 在加载时严格报错（见 create()）。
 */
void initializeBuiltinFeatureTypes()
{
    auto& reg = FeatureTypeRegistry::instance();
    reg.registerType("monster_room", createMonsterRoom);
    // TODO: 数据包共 54 种 configured_feature type，当前仅注册 monster_room。
    // 其余 53 种（tree/ore/random_patch/random_selector/geode/sculk_patch/large_dripstone/
    // disk/spring_feature/fossil/desert_well/forest_rock/... 等）尚未注册 C++ 工厂，
    // 加载对应 JSON 时会严格报错中断。按报错逐个补实现并在此 registerType。
}

} // namespace world::gen::feature
} // namespace mc
