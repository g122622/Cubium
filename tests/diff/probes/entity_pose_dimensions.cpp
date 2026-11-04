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
 * @brief 差分能力：实体在指定姿态下的尺寸（宽 / 高 / 眼睛高度）。
 *
 * `list`：实体类型 × 姿态 的全部组合
 * `run` ：一行 -> { "width": 位模式, "height": ..., "eyeHeight": ... }
 *
 * ## 原版语义（`LivingEntity`）
 *
 * ```java
 * public final EntityDimensions getDimensions(Pose pose) {
 *     return pose == Pose.SLEEPING
 *         ? SLEEPING_DIMENSIONS                      // 恒定 (0.2, 0.2, eye 0.2)
 *         : this.getDefaultDimensions(pose).scale(this.getScale());
 * }
 * protected EntityDimensions getDefaultDimensions(Pose pose) {
 *     return this.getType().getDimensions().scale(this.getAgeScale());
 * }
 * ```
 *
 * 两个要点：
 *
 * 1. `SLEEPING` 是**所有生物共用的固定尺寸** (0.2, 0.2, 0.2)，与实体类型、年龄、
 *    缩放都无关——它是 `LivingEntity` 里的 `final` 方法，子类改不了；
 * 2. 其余姿态走「类型尺寸 × 年龄缩放 × 实体缩放」。
 *
 * ## 输出口径
 *
 * 三项都用 f32 位模式，两侧同构，避开十进制格式差异。
 */

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityPose.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/registry/VanillaEntities.hpp"

#include "probe_world.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/**
 * 参与测试的实体类型。
 *
 * 选型原则：覆盖不同尺寸量级与覆写了 `getDefaultDimensions` 的类，
 * 同时避开需要特殊前置状态才能构造的类型。
 */
constexpr const char* TYPES[] = {
    "minecraft:pig",
    "minecraft:player",
    "minecraft:zombie",
    "minecraft:creeper",
    "minecraft:sniffer",
    "minecraft:goat",
    "minecraft:polar_bear",
    "minecraft:phantom",
    "minecraft:pufferfish",
    "minecraft:slime",
    "minecraft:bat",
    "minecraft:axolotl",
    "minecraft:frog",
    "minecraft:warden",
    "minecraft:breeze",
    "minecraft:allay",
};

/// 18 个姿态名，下标与 `EntityPose` 枚举值一致。
constexpr const char* POSES[] = {"standing",
    "fall_flying",
    "sleeping",
    "swimming",
    "spin_attack",
    "crouching",
    "long_jumping",
    "dying",
    "croaking",
    "using_tongue",
    "sitting",
    "roaring",
    "sniffing",
    "emerging",
    "digging",
    "sliding",
    "shooting",
    "inhaling"};

std::string floatBits(f32 value)
{
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "0x%08x", bits);
    return buffer;
}

/// 实体注册是重型全局状态，探针是单次进程，静态标志足够。
void ensureEntities()
{
    static bool s_initialized = false;
    if (!s_initialized) {
        entity::VanillaEntities::registerAll();
        s_initialized = true;
    }
}

void writeList(FILE* out)
{
    ensureEntities();
    for (const char* type : TYPES) {
        for (const char* pose : POSES) {
            std::fprintf(out, "%s|%s\n", type, pose);
        }
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureEntities();
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    const std::size_t separator = unit.find('|');
    if (separator == std::string::npos) {
        throw std::runtime_error("entity_pose_dimensions: 单元应为 'type|pose'：" + unit);
    }
    const std::string typeName = unit.substr(0, separator);
    const std::string poseName = unit.substr(separator + 1);

    std::size_t poseIndex = 0;
    bool foundPose = false;
    for (std::size_t i = 0; i < std::size(POSES); ++i) {
        if (poseName == POSES[i]) {
            poseIndex = i;
            foundPose = true;
            break;
        }
    }
    if (!foundPose) {
        throw std::runtime_error("entity_pose_dimensions: 未知姿态：" + poseName);
    }

    probe::NullWorld world;
    std::unique_ptr<Entity> instance = probe::createEntity(world, typeName);
    if (instance == nullptr) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }

    const auto pose = static_cast<EntityPose>(poseIndex);
    const entity::EntitySize size = instance->getDimensions(pose);

    return json{{"unit", unit},
        {"width", floatBits(size.width())},
        {"height", floatBits(size.height())},
        {"eyeHeight", floatBits(size.eyeHeight())}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(entity_pose_dimensions, &writeList, &runLine)

} // namespace mc::diff
