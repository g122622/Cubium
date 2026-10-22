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

#include "common/world/attribute/EnvironmentAttributes.hpp"
#include "common/world/attribute/EnvironmentAttributeRegistryDataPack.hpp"

#include "common/util/assert/AssertAll.hpp"
#include "common/world/attribute/AttributeTypes.hpp"
#include "common/world/attribute/EnvironmentAttributeRegistry.hpp"

#include <string>

namespace mc {
namespace world {
namespace attribute {
namespace {

using Activity = entity::ai::brain::schedule::Activity;
using TriState = util::TriState;

/**
 * @brief 属性注册名持有者
 *
 * 属性在构造时尚未登记进注册表，因此名字以惰性提供者的形式存入属性对象：
 * 每个属性拥有一个独立的名字槽，注册完成后调用 name() 才能得到结果。
 */
class _AttributeName {
public:
    explicit _AttributeName(std::string path)
        : m_path(std::move(path))
    {}

    [[nodiscard]] const std::string& get() const
    {
        MC_ASSERT_RELEASE_MSG(!m_fullName.empty(), "Environment attribute name queried before registration");
        return m_fullName;
    }

    /// 数据包中的完整名（`minecraft:<path>`）
    [[nodiscard]] const std::string& fullName() const noexcept { return m_fullName; }

    /// 注册表中的路径名（不含 `minecraft:`）
    [[nodiscard]] const std::string& path() const noexcept { return m_path; }

private:
    std::string m_path;
    std::string m_fullName = "minecraft:" + m_path;
};

template <typename Value>
EnvironmentAttribute<Value> _build(_AttributeName& name,
    const AttributeType<Value>& type,
    Value defaultValue,
    AttributeRange<Value> range,
    bool syncable,
    bool positional,
    bool spatiallyInterpolated)
{
    auto builder = EnvironmentAttribute<Value>::builder(
        [&name]() -> const std::string& { return name.get(); }, type, std::move(defaultValue));
    builder.valueRange(std::move(range));
    if (syncable) {
        builder.syncable();
    }
    if (!positional) {
        builder.notPositional();
    }
    if (spatiallyInterpolated) {
        builder.spatiallyInterpolated();
    }
    return builder.build();
}

/// 同步 + 空间插值的 RGB 颜色属性
EnvironmentAttribute<i32> _rgbColor(_AttributeName& name, i32 defaultValue)
{
    return _build<i32>(name, AttributeTypes::RGB_COLOR(), defaultValue, AttributeRange<i32>::any(), true, true, true);
}

/// 同步 + 空间插值的 ARGB 颜色属性
EnvironmentAttribute<i32> _argbColor(_AttributeName& name, i32 defaultValue)
{
    return _build<i32>(name, AttributeTypes::ARGB_COLOR(), defaultValue, AttributeRange<i32>::any(), true, true, true);
}

/// 同步 + 空间插值的浮点属性
EnvironmentAttribute<f32> _visualFloat(
    _AttributeName& name, f32 defaultValue, AttributeRange<f32> range = AttributeRange<f32>::any())
{
    return _build<f32>(name, AttributeTypes::FLOAT(), defaultValue, std::move(range), true, true, true);
}

/// 同步 + 空间插值的角度属性
EnvironmentAttribute<f32> _angle(_AttributeName& name)
{
    return _build<f32>(name, AttributeTypes::ANGLE_DEGREES(), 0.0f, AttributeRange<f32>::any(), true, true, true);
}

/// 同步、与位置无关的布尔属性
EnvironmentAttribute<bool> _syncableBoolean(_AttributeName& name, bool defaultValue)
{
    return _build<bool>(name, AttributeTypes::BOOLEAN(), defaultValue, AttributeRange<bool>::any(), true, false, false);
}

/// 不同步、与位置相关的布尔属性
EnvironmentAttribute<bool> _gameplayBoolean(_AttributeName& name, bool defaultValue)
{
    return _build<bool>(name, AttributeTypes::BOOLEAN(), defaultValue, AttributeRange<bool>::any(), false, true, false);
}

/// 不同步、与位置相关的单位区间浮点属性（vanilla 中 gameplay/* 概率类属性均非空间插值）
EnvironmentAttribute<f32> _unitFloat(_AttributeName& name, f32 defaultValue)
{
    return _build<f32>(
        name, AttributeTypes::FLOAT(), defaultValue, AttributeRange<f32>::unitFloat(), false, true, false);
}

/// 村民活动属性（同步、与位置无关）
EnvironmentAttribute<Activity> _activity(_AttributeName& name)
{
    return _build<Activity>(
        name, AttributeTypes::ACTIVITY(), Activity::IDLE, AttributeRange<Activity>::any(), true, false, false);
}

_AttributeName& _name_FOG_COLOR()
{
    static _AttributeName name("visual/fog_color");
    return name;
}

_AttributeName& _name_FOG_START_DISTANCE()
{
    static _AttributeName name("visual/fog_start_distance");
    return name;
}

_AttributeName& _name_FOG_END_DISTANCE()
{
    static _AttributeName name("visual/fog_end_distance");
    return name;
}

_AttributeName& _name_SKY_FOG_END_DISTANCE()
{
    static _AttributeName name("visual/sky_fog_end_distance");
    return name;
}

_AttributeName& _name_CLOUD_FOG_END_DISTANCE()
{
    static _AttributeName name("visual/cloud_fog_end_distance");
    return name;
}

_AttributeName& _name_WATER_FOG_COLOR()
{
    static _AttributeName name("visual/water_fog_color");
    return name;
}

_AttributeName& _name_WATER_FOG_START_DISTANCE()
{
    static _AttributeName name("visual/water_fog_start_distance");
    return name;
}

_AttributeName& _name_WATER_FOG_END_DISTANCE()
{
    static _AttributeName name("visual/water_fog_end_distance");
    return name;
}

_AttributeName& _name_SKY_COLOR()
{
    static _AttributeName name("visual/sky_color");
    return name;
}

_AttributeName& _name_SUNRISE_SUNSET_COLOR()
{
    static _AttributeName name("visual/sunrise_sunset_color");
    return name;
}

_AttributeName& _name_CLOUD_COLOR()
{
    static _AttributeName name("visual/cloud_color");
    return name;
}

_AttributeName& _name_CLOUD_HEIGHT()
{
    static _AttributeName name("visual/cloud_height");
    return name;
}

_AttributeName& _name_SUN_ANGLE()
{
    static _AttributeName name("visual/sun_angle");
    return name;
}

_AttributeName& _name_MOON_ANGLE()
{
    static _AttributeName name("visual/moon_angle");
    return name;
}

_AttributeName& _name_STAR_ANGLE()
{
    static _AttributeName name("visual/star_angle");
    return name;
}

_AttributeName& _name_MOON_PHASE()
{
    static _AttributeName name("visual/moon_phase");
    return name;
}

_AttributeName& _name_STAR_BRIGHTNESS()
{
    static _AttributeName name("visual/star_brightness");
    return name;
}

_AttributeName& _name_SKY_LIGHT_COLOR()
{
    static _AttributeName name("visual/sky_light_color");
    return name;
}

_AttributeName& _name_SKY_LIGHT_FACTOR()
{
    static _AttributeName name("visual/sky_light_factor");
    return name;
}

_AttributeName& _name_DEFAULT_DRIPSTONE_PARTICLE()
{
    static _AttributeName name("visual/default_dripstone_particle");
    return name;
}

_AttributeName& _name_AMBIENT_PARTICLES()
{
    static _AttributeName name("visual/ambient_particles");
    return name;
}

_AttributeName& _name_BACKGROUND_MUSIC()
{
    static _AttributeName name("audio/background_music");
    return name;
}

_AttributeName& _name_MUSIC_VOLUME()
{
    static _AttributeName name("audio/music_volume");
    return name;
}

_AttributeName& _name_AMBIENT_SOUNDS()
{
    static _AttributeName name("audio/ambient_sounds");
    return name;
}

_AttributeName& _name_FIREFLY_BUSH_SOUNDS()
{
    static _AttributeName name("audio/firefly_bush_sounds");
    return name;
}

_AttributeName& _name_SKY_LIGHT_LEVEL()
{
    static _AttributeName name("gameplay/sky_light_level");
    return name;
}

_AttributeName& _name_CAN_START_RAID()
{
    static _AttributeName name("gameplay/can_start_raid");
    return name;
}

_AttributeName& _name_WATER_EVAPORATES()
{
    static _AttributeName name("gameplay/water_evaporates");
    return name;
}

_AttributeName& _name_RESPAWN_ANCHOR_WORKS()
{
    static _AttributeName name("gameplay/respawn_anchor_works");
    return name;
}

_AttributeName& _name_NETHER_PORTAL_SPAWNS_PIGLINS()
{
    static _AttributeName name("gameplay/nether_portal_spawns_piglin");
    return name;
}

_AttributeName& _name_FAST_LAVA()
{
    static _AttributeName name("gameplay/fast_lava");
    return name;
}

_AttributeName& _name_INCREASED_FIRE_BURNOUT()
{
    static _AttributeName name("gameplay/increased_fire_burnout");
    return name;
}

_AttributeName& _name_EYEBLOSSOM_OPEN()
{
    static _AttributeName name("gameplay/eyeblossom_open");
    return name;
}

_AttributeName& _name_TURTLE_EGG_HATCH_CHANCE()
{
    static _AttributeName name("gameplay/turtle_egg_hatch_chance");
    return name;
}

_AttributeName& _name_PIGLINS_ZOMBIFY()
{
    static _AttributeName name("gameplay/piglins_zombify");
    return name;
}

_AttributeName& _name_SNOW_GOLEM_MELTS()
{
    static _AttributeName name("gameplay/snow_golem_melts");
    return name;
}

_AttributeName& _name_CREAKING_ACTIVE()
{
    static _AttributeName name("gameplay/creaking_active");
    return name;
}

_AttributeName& _name_SURFACE_SLIME_SPAWN_CHANCE()
{
    static _AttributeName name("gameplay/surface_slime_spawn_chance");
    return name;
}

_AttributeName& _name_CAT_WAKING_UP_GIFT_CHANCE()
{
    static _AttributeName name("gameplay/cat_waking_up_gift_chance");
    return name;
}

_AttributeName& _name_BEES_STAY_IN_HIVE()
{
    static _AttributeName name("gameplay/bees_stay_in_hive");
    return name;
}

_AttributeName& _name_NAUTILUS_ENTERS_WATER_WHEN_RIDDEN()
{
    static _AttributeName name("gameplay/nautilus_enters_water_when_ridden");
    return name;
}

_AttributeName& _name_MONSTERS_BURN()
{
    static _AttributeName name("gameplay/monsters_burn");
    return name;
}

_AttributeName& _name_CAN_PILLAGER_PATROL_SPAWN()
{
    static _AttributeName name("gameplay/can_pillager_patrol_spawn");
    return name;
}

_AttributeName& _name_VILLAGER_ACTIVITY()
{
    static _AttributeName name("gameplay/villager_activity");
    return name;
}

_AttributeName& _name_BABY_VILLAGER_ACTIVITY()
{
    static _AttributeName name("gameplay/baby_villager_activity");
    return name;
}

} // namespace

// ============================================================================
// 视觉属性
// ============================================================================

const EnvironmentAttribute<i32>& EnvironmentAttributes::FOG_COLOR()
{
    static const EnvironmentAttribute<i32> instance = _rgbColor(_name_FOG_COLOR(), 0);
    return instance;
}

const EnvironmentAttribute<f32>& EnvironmentAttributes::FOG_START_DISTANCE()
{
    static const EnvironmentAttribute<f32> instance = _visualFloat(_name_FOG_START_DISTANCE(), 0.0f);
    return instance;
}

const EnvironmentAttribute<f32>& EnvironmentAttributes::FOG_END_DISTANCE()
{
    static const EnvironmentAttribute<f32> instance =
        _visualFloat(_name_FOG_END_DISTANCE(), 1024.0f, AttributeRange<f32>::nonNegativeFloat());
    return instance;
}

const EnvironmentAttribute<f32>& EnvironmentAttributes::SKY_FOG_END_DISTANCE()
{
    static const EnvironmentAttribute<f32> instance =
        _visualFloat(_name_SKY_FOG_END_DISTANCE(), 512.0f, AttributeRange<f32>::nonNegativeFloat());
    return instance;
}

const EnvironmentAttribute<f32>& EnvironmentAttributes::CLOUD_FOG_END_DISTANCE()
{
    static const EnvironmentAttribute<f32> instance =
        _visualFloat(_name_CLOUD_FOG_END_DISTANCE(), 2048.0f, AttributeRange<f32>::nonNegativeFloat());
    return instance;
}

const EnvironmentAttribute<i32>& EnvironmentAttributes::WATER_FOG_COLOR()
{
    static const EnvironmentAttribute<i32> instance = _rgbColor(_name_WATER_FOG_COLOR(), -16448205);
    return instance;
}

const EnvironmentAttribute<f32>& EnvironmentAttributes::WATER_FOG_START_DISTANCE()
{
    static const EnvironmentAttribute<f32> instance = _visualFloat(_name_WATER_FOG_START_DISTANCE(), -8.0f);
    return instance;
}

const EnvironmentAttribute<f32>& EnvironmentAttributes::WATER_FOG_END_DISTANCE()
{
    static const EnvironmentAttribute<f32> instance =
        _visualFloat(_name_WATER_FOG_END_DISTANCE(), 96.0f, AttributeRange<f32>::nonNegativeFloat());
    return instance;
}

const EnvironmentAttribute<i32>& EnvironmentAttributes::SKY_COLOR()
{
    static const EnvironmentAttribute<i32> instance = _rgbColor(_name_SKY_COLOR(), 0);
    return instance;
}

const EnvironmentAttribute<i32>& EnvironmentAttributes::SUNRISE_SUNSET_COLOR()
{
    static const EnvironmentAttribute<i32> instance = _argbColor(_name_SUNRISE_SUNSET_COLOR(), 0);
    return instance;
}

const EnvironmentAttribute<i32>& EnvironmentAttributes::CLOUD_COLOR()
{
    static const EnvironmentAttribute<i32> instance = _argbColor(_name_CLOUD_COLOR(), 0);
    return instance;
}

const EnvironmentAttribute<f32>& EnvironmentAttributes::CLOUD_HEIGHT()
{
    static const EnvironmentAttribute<f32> instance = _visualFloat(_name_CLOUD_HEIGHT(), 192.33f);
    return instance;
}

const EnvironmentAttribute<f32>& EnvironmentAttributes::SUN_ANGLE()
{
    static const EnvironmentAttribute<f32> instance = _angle(_name_SUN_ANGLE());
    return instance;
}

const EnvironmentAttribute<f32>& EnvironmentAttributes::MOON_ANGLE()
{
    static const EnvironmentAttribute<f32> instance = _angle(_name_MOON_ANGLE());
    return instance;
}

const EnvironmentAttribute<f32>& EnvironmentAttributes::STAR_ANGLE()
{
    static const EnvironmentAttribute<f32> instance = _angle(_name_STAR_ANGLE());
    return instance;
}

const EnvironmentAttribute<MoonPhase>& EnvironmentAttributes::MOON_PHASE()
{
    static const EnvironmentAttribute<MoonPhase> instance = _build<MoonPhase>(_name_MOON_PHASE(),
        AttributeTypes::MOON_PHASE(),
        MoonPhase::FullMoon,
        AttributeRange<MoonPhase>::any(),
        true,
        false,
        false);
    return instance;
}

const EnvironmentAttribute<f32>& EnvironmentAttributes::STAR_BRIGHTNESS()
{
    static const EnvironmentAttribute<f32> instance =
        _visualFloat(_name_STAR_BRIGHTNESS(), 0.0f, AttributeRange<f32>::unitFloat());
    return instance;
}

const EnvironmentAttribute<i32>& EnvironmentAttributes::SKY_LIGHT_COLOR()
{
    static const EnvironmentAttribute<i32> instance = _rgbColor(_name_SKY_LIGHT_COLOR(), -1);
    return instance;
}

const EnvironmentAttribute<f32>& EnvironmentAttributes::SKY_LIGHT_FACTOR()
{
    static const EnvironmentAttribute<f32> instance =
        _visualFloat(_name_SKY_LIGHT_FACTOR(), 1.0f, AttributeRange<f32>::unitFloat());
    return instance;
}

const EnvironmentAttribute<bool>& EnvironmentAttributes::DEFAULT_DRIPSTONE_PARTICLE()
{
    static const EnvironmentAttribute<bool> instance = _syncableBoolean(_name_DEFAULT_DRIPSTONE_PARTICLE(), true);
    return instance;
}

const EnvironmentAttribute<bool>& EnvironmentAttributes::AMBIENT_PARTICLES()
{
    static const EnvironmentAttribute<bool> instance = _syncableBoolean(_name_AMBIENT_PARTICLES(), false);
    return instance;
}

// ============================================================================
// 音频属性
// ============================================================================

const EnvironmentAttribute<bool>& EnvironmentAttributes::BACKGROUND_MUSIC()
{
    static const EnvironmentAttribute<bool> instance = _syncableBoolean(_name_BACKGROUND_MUSIC(), false);
    return instance;
}

const EnvironmentAttribute<f32>& EnvironmentAttributes::MUSIC_VOLUME()
{
    static const EnvironmentAttribute<f32> instance = _build<f32>(
        _name_MUSIC_VOLUME(), AttributeTypes::FLOAT(), 1.0f, AttributeRange<f32>::unitFloat(), true, true, false);
    return instance;
}

const EnvironmentAttribute<bool>& EnvironmentAttributes::AMBIENT_SOUNDS()
{
    static const EnvironmentAttribute<bool> instance = _syncableBoolean(_name_AMBIENT_SOUNDS(), false);
    return instance;
}

const EnvironmentAttribute<bool>& EnvironmentAttributes::FIREFLY_BUSH_SOUNDS()
{
    static const EnvironmentAttribute<bool> instance = _syncableBoolean(_name_FIREFLY_BUSH_SOUNDS(), false);
    return instance;
}

// ============================================================================
// 玩法属性
// ============================================================================

const EnvironmentAttribute<f32>& EnvironmentAttributes::SKY_LIGHT_LEVEL()
{
    static const EnvironmentAttribute<f32> instance = _build<f32>(_name_SKY_LIGHT_LEVEL(),
        AttributeTypes::FLOAT(),
        15.0f,
        AttributeRange<f32>::ofFloat(0.0f, 15.0f),
        true,
        false,
        false);
    return instance;
}

const EnvironmentAttribute<bool>& EnvironmentAttributes::CAN_START_RAID()
{
    static const EnvironmentAttribute<bool> instance = _gameplayBoolean(_name_CAN_START_RAID(), true);
    return instance;
}

const EnvironmentAttribute<bool>& EnvironmentAttributes::WATER_EVAPORATES()
{
    static const EnvironmentAttribute<bool> instance = _syncableBoolean(_name_WATER_EVAPORATES(), false);
    return instance;
}

const EnvironmentAttribute<bool>& EnvironmentAttributes::RESPAWN_ANCHOR_WORKS()
{
    static const EnvironmentAttribute<bool> instance = _gameplayBoolean(_name_RESPAWN_ANCHOR_WORKS(), false);
    return instance;
}

const EnvironmentAttribute<bool>& EnvironmentAttributes::NETHER_PORTAL_SPAWNS_PIGLINS()
{
    static const EnvironmentAttribute<bool> instance = _gameplayBoolean(_name_NETHER_PORTAL_SPAWNS_PIGLINS(), false);
    return instance;
}

const EnvironmentAttribute<bool>& EnvironmentAttributes::FAST_LAVA()
{
    static const EnvironmentAttribute<bool> instance = _syncableBoolean(_name_FAST_LAVA(), false);
    return instance;
}

const EnvironmentAttribute<bool>& EnvironmentAttributes::INCREASED_FIRE_BURNOUT()
{
    static const EnvironmentAttribute<bool> instance = _gameplayBoolean(_name_INCREASED_FIRE_BURNOUT(), false);
    return instance;
}

const EnvironmentAttribute<util::TriState>& EnvironmentAttributes::EYEBLOSSOM_OPEN()
{
    static const EnvironmentAttribute<util::TriState> instance = _build<TriState>(_name_EYEBLOSSOM_OPEN(),
        AttributeTypes::TRI_STATE(),
        TriState::Default,
        AttributeRange<TriState>::any(),
        false,
        true,
        false);
    return instance;
}

const EnvironmentAttribute<f32>& EnvironmentAttributes::TURTLE_EGG_HATCH_CHANCE()
{
    static const EnvironmentAttribute<f32> instance = _unitFloat(_name_TURTLE_EGG_HATCH_CHANCE(), 0.0f);
    return instance;
}

const EnvironmentAttribute<bool>& EnvironmentAttributes::PIGLINS_ZOMBIFY()
{
    static const EnvironmentAttribute<bool> instance = _syncableBoolean(_name_PIGLINS_ZOMBIFY(), true);
    return instance;
}

const EnvironmentAttribute<bool>& EnvironmentAttributes::SNOW_GOLEM_MELTS()
{
    static const EnvironmentAttribute<bool> instance = _gameplayBoolean(_name_SNOW_GOLEM_MELTS(), false);
    return instance;
}

const EnvironmentAttribute<bool>& EnvironmentAttributes::CREAKING_ACTIVE()
{
    static const EnvironmentAttribute<bool> instance = _syncableBoolean(_name_CREAKING_ACTIVE(), false);
    return instance;
}

const EnvironmentAttribute<f32>& EnvironmentAttributes::SURFACE_SLIME_SPAWN_CHANCE()
{
    static const EnvironmentAttribute<f32> instance = _unitFloat(_name_SURFACE_SLIME_SPAWN_CHANCE(), 0.0f);
    return instance;
}

const EnvironmentAttribute<f32>& EnvironmentAttributes::CAT_WAKING_UP_GIFT_CHANCE()
{
    static const EnvironmentAttribute<f32> instance = _unitFloat(_name_CAT_WAKING_UP_GIFT_CHANCE(), 0.0f);
    return instance;
}

const EnvironmentAttribute<bool>& EnvironmentAttributes::BEES_STAY_IN_HIVE()
{
    static const EnvironmentAttribute<bool> instance = _gameplayBoolean(_name_BEES_STAY_IN_HIVE(), false);
    return instance;
}

const EnvironmentAttribute<bool>& EnvironmentAttributes::NAUTILUS_ENTERS_WATER_WHEN_RIDDEN()
{
    // 默认 false：仅海洋维度把它置为 true（对齐 vanilla EnvironmentAttributes 的维度属性层）。
    static const EnvironmentAttribute<bool> instance =
        _gameplayBoolean(_name_NAUTILUS_ENTERS_WATER_WHEN_RIDDEN(), false);
    return instance;
}

const EnvironmentAttribute<bool>& EnvironmentAttributes::MONSTERS_BURN()
{
    static const EnvironmentAttribute<bool> instance = _gameplayBoolean(_name_MONSTERS_BURN(), false);
    return instance;
}

const EnvironmentAttribute<bool>& EnvironmentAttributes::CAN_PILLAGER_PATROL_SPAWN()
{
    static const EnvironmentAttribute<bool> instance = _gameplayBoolean(_name_CAN_PILLAGER_PATROL_SPAWN(), true);
    return instance;
}

const EnvironmentAttribute<Activity>& EnvironmentAttributes::VILLAGER_ACTIVITY()
{
    static const EnvironmentAttribute<Activity> instance = _activity(_name_VILLAGER_ACTIVITY());
    return instance;
}

const EnvironmentAttribute<Activity>& EnvironmentAttributes::BABY_VILLAGER_ACTIVITY()
{
    static const EnvironmentAttribute<Activity> instance = _activity(_name_BABY_VILLAGER_ACTIVITY());
    return instance;
}

std::shared_ptr<const AttributeModifier<i32, i32>> EnvironmentAttributes::colorModifier(
    const EnvironmentAttribute<i32>& attribute, AttributeModifierOperation operation, ColorModifierArgument argument)
{
    if (!attribute.type().isAllowedModifier(operation)) {
        return nullptr;
    }

    switch (operation) {
        case AttributeModifierOperation::AlphaBlend:
            return std::make_shared<TypedModifierAdapter<i32, i32>>(
                RgbColorModifier::ALPHA_BLEND, [](i32 value) { return value; });
        case AttributeModifierOperation::Add:
            return nonOwningSharedPtr(&RgbColorModifier::ADD);
        case AttributeModifierOperation::Subtract:
            return nonOwningSharedPtr(&RgbColorModifier::SUBTRACT);
        case AttributeModifierOperation::Multiply:
            return nonOwningSharedPtr(&RgbColorModifier::MULTIPLY);
        case AttributeModifierOperation::BlendToGray:
            break;
        default:
            return nullptr;
    }

    // 灰度混合的参数是轨道给出的 (brightness, factor)，而时间线采样只产出属性值本身：
    // 参数被折叠为常量，轨道值仅用于时间线推进。
    const i32 blend = argument.blendToGray;
    return std::make_shared<TypedModifierAdapter<i32, i32>>(BlendToGrayModifier{}, [blend](i32) { return blend; });
}

const std::string& EnvironmentAttributes::packageName(const void* attribute)
{
    const std::string* name = EnvironmentAttributeRegistry::attributeName(attribute);
    MC_ASSERT_RELEASE_MSG(name != nullptr, "EnvironmentAttributes::packageName: attribute not registered");
    return *name;
}

// ============================================================================
// 注册
// ============================================================================

Result<size_t> EnvironmentAttributes::registerAll()
{
    AttributeTypes::registerAll();

    size_t registered = 0;
    // 按完整注册名登记：数据包与内部查询均以 `minecraft:<path>` 引用属性
    const auto registerOne = [&registered](_AttributeName& name, const auto& attribute) {
        const auto result = EnvironmentAttributeRegistry::registerAttribute(name.fullName(), attribute);
        if (result.success()) {
            ++registered;
        }
    };

    registerOne(_name_FOG_COLOR(), FOG_COLOR());
    registerOne(_name_FOG_START_DISTANCE(), FOG_START_DISTANCE());
    registerOne(_name_FOG_END_DISTANCE(), FOG_END_DISTANCE());
    registerOne(_name_SKY_FOG_END_DISTANCE(), SKY_FOG_END_DISTANCE());
    registerOne(_name_CLOUD_FOG_END_DISTANCE(), CLOUD_FOG_END_DISTANCE());
    registerOne(_name_WATER_FOG_COLOR(), WATER_FOG_COLOR());
    registerOne(_name_WATER_FOG_START_DISTANCE(), WATER_FOG_START_DISTANCE());
    registerOne(_name_WATER_FOG_END_DISTANCE(), WATER_FOG_END_DISTANCE());
    registerOne(_name_SKY_COLOR(), SKY_COLOR());
    registerOne(_name_SUNRISE_SUNSET_COLOR(), SUNRISE_SUNSET_COLOR());
    registerOne(_name_CLOUD_COLOR(), CLOUD_COLOR());
    registerOne(_name_CLOUD_HEIGHT(), CLOUD_HEIGHT());
    registerOne(_name_SUN_ANGLE(), SUN_ANGLE());
    registerOne(_name_MOON_ANGLE(), MOON_ANGLE());
    registerOne(_name_STAR_ANGLE(), STAR_ANGLE());
    registerOne(_name_MOON_PHASE(), MOON_PHASE());
    registerOne(_name_STAR_BRIGHTNESS(), STAR_BRIGHTNESS());
    registerOne(_name_SKY_LIGHT_COLOR(), SKY_LIGHT_COLOR());
    registerOne(_name_SKY_LIGHT_FACTOR(), SKY_LIGHT_FACTOR());
    registerOne(_name_DEFAULT_DRIPSTONE_PARTICLE(), DEFAULT_DRIPSTONE_PARTICLE());
    registerOne(_name_AMBIENT_PARTICLES(), AMBIENT_PARTICLES());

    registerOne(_name_BACKGROUND_MUSIC(), BACKGROUND_MUSIC());
    registerOne(_name_MUSIC_VOLUME(), MUSIC_VOLUME());
    registerOne(_name_AMBIENT_SOUNDS(), AMBIENT_SOUNDS());
    registerOne(_name_FIREFLY_BUSH_SOUNDS(), FIREFLY_BUSH_SOUNDS());

    registerOne(_name_SKY_LIGHT_LEVEL(), SKY_LIGHT_LEVEL());
    registerOne(_name_CAN_START_RAID(), CAN_START_RAID());
    registerOne(_name_WATER_EVAPORATES(), WATER_EVAPORATES());
    registerOne(_name_RESPAWN_ANCHOR_WORKS(), RESPAWN_ANCHOR_WORKS());
    registerOne(_name_NETHER_PORTAL_SPAWNS_PIGLINS(), NETHER_PORTAL_SPAWNS_PIGLINS());
    registerOne(_name_FAST_LAVA(), FAST_LAVA());
    registerOne(_name_INCREASED_FIRE_BURNOUT(), INCREASED_FIRE_BURNOUT());
    registerOne(_name_EYEBLOSSOM_OPEN(), EYEBLOSSOM_OPEN());
    registerOne(_name_TURTLE_EGG_HATCH_CHANCE(), TURTLE_EGG_HATCH_CHANCE());
    registerOne(_name_PIGLINS_ZOMBIFY(), PIGLINS_ZOMBIFY());
    registerOne(_name_SNOW_GOLEM_MELTS(), SNOW_GOLEM_MELTS());
    registerOne(_name_CREAKING_ACTIVE(), CREAKING_ACTIVE());
    registerOne(_name_SURFACE_SLIME_SPAWN_CHANCE(), SURFACE_SLIME_SPAWN_CHANCE());
    registerOne(_name_CAT_WAKING_UP_GIFT_CHANCE(), CAT_WAKING_UP_GIFT_CHANCE());
    registerOne(_name_BEES_STAY_IN_HIVE(), BEES_STAY_IN_HIVE());
    registerOne(_name_NAUTILUS_ENTERS_WATER_WHEN_RIDDEN(), NAUTILUS_ENTERS_WATER_WHEN_RIDDEN());
    registerOne(_name_MONSTERS_BURN(), MONSTERS_BURN());
    registerOne(_name_CAN_PILLAGER_PATROL_SPAWN(), CAN_PILLAGER_PATROL_SPAWN());
    registerOne(_name_VILLAGER_ACTIVITY(), VILLAGER_ACTIVITY());
    registerOne(_name_BABY_VILLAGER_ACTIVITY(), BABY_VILLAGER_ACTIVITY());

    return registered;
}

} // namespace attribute
} // namespace world
} // namespace mc