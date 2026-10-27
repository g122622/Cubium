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

#pragma once

#include "common/core/Result.hpp"
#include "common/entity/ai/brain/schedule/Activity.hpp"
#include "common/util/TriState.hpp"
#include "common/world/attribute/AttributeModifier.hpp"
#include "common/world/attribute/EnvironmentAttribute.hpp"
#include "common/world/level/MoonPhase.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <string>

namespace mc {
namespace world {
namespace attribute {

/**
 * @brief 预定义环境属性
 *
 * MC 1.21.11 interface EnvironmentAttributes。
 * 每个属性在注册表中的名字同时作为数据包 `keyframes.properties.<name>` 中的引用名
 * （例如 `minecraft:visual/sky_color`），因此命名必须与 vanilla 完全一致。
 *
 * BED_RULE 依赖床行为规则子系统（BedRule 类型），尚未实现。
 */
class EnvironmentAttributes {
public:
    // =========================================================================
    // 视觉属性（visual/*）
    // =========================================================================

    /// 雾颜色（RGB）
    [[nodiscard]] static const EnvironmentAttribute<i32>& FOG_COLOR();
    /// 雾开始距离
    [[nodiscard]] static const EnvironmentAttribute<f32>& FOG_START_DISTANCE();
    /// 雾结束距离（非负）
    [[nodiscard]] static const EnvironmentAttribute<f32>& FOG_END_DISTANCE();
    /// 天空雾结束距离（非负）
    [[nodiscard]] static const EnvironmentAttribute<f32>& SKY_FOG_END_DISTANCE();
    /// 云层雾结束距离（非负）
    [[nodiscard]] static const EnvironmentAttribute<f32>& CLOUD_FOG_END_DISTANCE();
    /// 水下雾颜色（RGB）
    [[nodiscard]] static const EnvironmentAttribute<i32>& WATER_FOG_COLOR();
    /// 水下雾开始距离
    [[nodiscard]] static const EnvironmentAttribute<f32>& WATER_FOG_START_DISTANCE();
    /// 水下雾结束距离（非负）
    [[nodiscard]] static const EnvironmentAttribute<f32>& WATER_FOG_END_DISTANCE();
    /// 天空颜色（RGB）
    [[nodiscard]] static const EnvironmentAttribute<i32>& SKY_COLOR();
    /// 日出日落颜色（ARGB）
    [[nodiscard]] static const EnvironmentAttribute<i32>& SUNRISE_SUNSET_COLOR();
    /// 云颜色（ARGB）
    [[nodiscard]] static const EnvironmentAttribute<i32>& CLOUD_COLOR();
    /// 云层高度
    [[nodiscard]] static const EnvironmentAttribute<f32>& CLOUD_HEIGHT();
    /// 太阳角度（度）
    [[nodiscard]] static const EnvironmentAttribute<f32>& SUN_ANGLE();
    /// 月亮角度（度）
    [[nodiscard]] static const EnvironmentAttribute<f32>& MOON_ANGLE();
    /// 星空角度（度）
    [[nodiscard]] static const EnvironmentAttribute<f32>& STAR_ANGLE();
    /// 月相
    [[nodiscard]] static const EnvironmentAttribute<MoonPhase>& MOON_PHASE();
    /// 星空亮度（单位区间）
    [[nodiscard]] static const EnvironmentAttribute<f32>& STAR_BRIGHTNESS();
    /// 天空光照颜色（RGB）
    [[nodiscard]] static const EnvironmentAttribute<i32>& SKY_LIGHT_COLOR();
    /// 天空光照系数（单位区间）
    [[nodiscard]] static const EnvironmentAttribute<f32>& SKY_LIGHT_FACTOR();
    /// 滴水石水滴粒子是否生成（粒子子系统未接入前，值仅作数据承载）
    [[nodiscard]] static const EnvironmentAttribute<bool>& DEFAULT_DRIPSTONE_PARTICLE();
    /// 环境粒子是否生成（粒子子系统未接入前，值仅作数据承载）
    [[nodiscard]] static const EnvironmentAttribute<bool>& AMBIENT_PARTICLES();

    // =========================================================================
    // 音频属性（audio/*）
    // =========================================================================

    /// 背景音乐是否播放（音乐子系统未接入前，值仅作数据承载）
    [[nodiscard]] static const EnvironmentAttribute<bool>& BACKGROUND_MUSIC();
    /// 音乐音量（单位区间）
    [[nodiscard]] static const EnvironmentAttribute<f32>& MUSIC_VOLUME();
    /// 环境音是否播放（音效子系统未接入前，值仅作数据承载）
    [[nodiscard]] static const EnvironmentAttribute<bool>& AMBIENT_SOUNDS();
    /// 萤火虫灌木音效是否开启
    [[nodiscard]] static const EnvironmentAttribute<bool>& FIREFLY_BUSH_SOUNDS();

    // =========================================================================
    // 玩法属性（gameplay/*）
    // =========================================================================

    /// 天空光照等级（0~15）
    [[nodiscard]] static const EnvironmentAttribute<f32>& SKY_LIGHT_LEVEL();
    /// 能否开始袭击
    [[nodiscard]] static const EnvironmentAttribute<bool>& CAN_START_RAID();
    /// 水是否蒸发
    [[nodiscard]] static const EnvironmentAttribute<bool>& WATER_EVAPORATES();
    /// 重生锚是否可用
    [[nodiscard]] static const EnvironmentAttribute<bool>& RESPAWN_ANCHOR_WORKS();
    /// 下界传送门是否生成猪灵
    [[nodiscard]] static const EnvironmentAttribute<bool>& NETHER_PORTAL_SPAWNS_PIGLINS();
    /// 岩浆是否加速流动
    [[nodiscard]] static const EnvironmentAttribute<bool>& FAST_LAVA();
    /// 火焰是否更快熄灭
    [[nodiscard]] static const EnvironmentAttribute<bool>& INCREASED_FIRE_BURNOUT();
    /// 眼眸花是否开放（三态）
    [[nodiscard]] static const EnvironmentAttribute<util::TriState>& EYEBLOSSOM_OPEN();
    /// 海龟蛋孵化概率（单位区间）
    [[nodiscard]] static const EnvironmentAttribute<f32>& TURTLE_EGG_HATCH_CHANCE();
    /// 猪灵是否僵尸化
    [[nodiscard]] static const EnvironmentAttribute<bool>& PIGLINS_ZOMBIFY();
    /// 雪傀儡是否融化
    [[nodiscard]] static const EnvironmentAttribute<bool>& SNOW_GOLEM_MELTS();
    /// 嘎枝是否激活
    [[nodiscard]] static const EnvironmentAttribute<bool>& CREAKING_ACTIVE();
    /// 地表史莱姆生成概率（单位区间）
    [[nodiscard]] static const EnvironmentAttribute<f32>& SURFACE_SLIME_SPAWN_CHANCE();
    /// 猫醒来时送礼概率（单位区间）
    [[nodiscard]] static const EnvironmentAttribute<f32>& CAT_WAKING_UP_GIFT_CHANCE();
    /// 蜜蜂是否留守蜂巢
    [[nodiscard]] static const EnvironmentAttribute<bool>& BEES_STAY_IN_HIVE();
    /// 被骑乘的鹦鹉螺是否入水（对齐 vanilla EnvironmentAttributes.NAUTILUS_ENTERS_WATER_WHEN_RIDDEN）
    [[nodiscard]] static const EnvironmentAttribute<bool>& NAUTILUS_ENTERS_WATER_WHEN_RIDDEN();
    /// 怪物是否在日光下燃烧
    [[nodiscard]] static const EnvironmentAttribute<bool>& MONSTERS_BURN();
    /// 犰狳鳞甲脱落间隔（秒，unit float；对齐 vanilla EnvironmentAttributes.ARMADILLO_SCUTE_DROP_INTERVAL）
    [[nodiscard]] static const EnvironmentAttribute<f32>& ARMADILLO_SCUTE_DROP_INTERVAL();
    /// 能否生成劫掠巡逻队
    [[nodiscard]] static const EnvironmentAttribute<bool>& CAN_PILLAGER_PATROL_SPAWN();
    /// 成年村民活动
    [[nodiscard]] static const EnvironmentAttribute<entity::ai::brain::schedule::Activity>& VILLAGER_ACTIVITY();
    /// 幼年村民活动
    [[nodiscard]] static const EnvironmentAttribute<entity::ai::brain::schedule::Activity>& BABY_VILLAGER_ACTIVITY();

    // =========================================================================
    // 注册
    // =========================================================================

    /**
     * @brief 颜色属性使用的参数化修改器
     *
     * 数据包轨道上的 `modifier` 可指定参数化修改器，其参数不是属性值本身，
     * 因此不能直接使用类型库里的常量单例。本函数按操作与参数构造
     * 一个以属性值类型为参数的修改器；类型库不支持该操作时返回 nullptr。
     *
     * @param attribute 颜色属性
     * @param operation 操作标识
     * @param argument 修改器参数（ARGB 颜色 / 灰度混合参数，按操作解释）
     * @return 修改器；不支持时返回 nullptr
     */
    [[nodiscard]] static std::shared_ptr<const AttributeModifier<i32, i32>> colorModifier(
        const EnvironmentAttribute<i32>& attribute,
        AttributeModifierOperation operation,
        ColorModifierArgument argument);

    /// 属性在注册表中的完整名（`minecraft:<path>`）
    [[nodiscard]] static const std::string& packageName(const void* attribute);

    /**
     * @brief 把全部预定义属性登记进 EnvironmentAttributeRegistry
     *
     * 注册顺序即网络同步序号；可在属性被引用前多次调用，已存在的条目会被跳过。
     *
     * @return 本次新登记的数量
     */
    static Result<size_t> registerAll();
};

} // namespace attribute
} // namespace world
} // namespace mc
