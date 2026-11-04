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
 * @brief 差分能力：实体类型的注册尺寸（宽 / 高 / 眼睛高度 / 是否固定）。
 *
 * `list`：全部实体类型资源名
 * `run` ：一行 → { "width": 位模式, "height": ..., "eyeHeight": ..., "fixed": bool }
 *
 * ## 为什么需要它
 *
 * `EntityDimensions` 的四项数值是**原版硬编码在每个 `EntityType.Builder` 调用里**的
 * 注册数据，不在任何数据包中，也没有任何现成机制能校验它们抄写是否正确。它们同时是：
 *
 * - 碰撞箱（`makeBoundingBox`，宽一半作半径、高作 Y 跨度）；
 * - 眼睛高度（`Entity.getEyeY`、视线判定、龙息/凝视类机制）；
 * - 姿势与缩放（`EntityDimensions.scale`）的基数。
 *
 * 抄错一位小数（如把 1.95 写成 1.9）不会报错、不会崩溃，只会让该实体在若干帧后
 * 卡进方块或视线的落点差一格——属于最难靠人工发现的一类数据错误。
 *
 * ## 输出口径
 *
 * **只输出浮点位模式**（`f64` 的 IEEE-754 位表示，先把值收敛到 `f32` 再提升回 `f64`）。
 * 原版的宽高是 Java `float`，两侧的十进制最短表示不同（`Float.toString` vs `%.6g`），
 * 直接比字符串会产生大量伪差异；位模式则精确且无格式歧义。
 *
 * `fixed` 对应 `EntityDimensions.fixed`：为 true 时该实体**不随姿势改变尺寸**。
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
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/registry/VanillaEntities.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 实体注册是重型全局状态，探针是单次进程，静态标志足够。
void ensureEntities()
{
    static bool s_initialized = false;
    if (!s_initialized) {
        entity::VanillaEntities::registerAll();
        s_initialized = true;
    }
}

/**
 * @brief 把 f32 尺寸值转成其 IEEE-754 位模式字符串
 *
 * 先把值收敛到 f32（原版口径），再零扩展成 f64 取位，避免两侧 f32->f64 提升方式不同
 * 造成的差异；同时保留位模式的无格式歧义特性。
 */
std::string floatBits(f32 value)
{
    std::uint32_t bits32 = 0;
    std::memcpy(&bits32, &value, sizeof(bits32));
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "0x%08x", bits32);
    return buffer;
}

void writeList(FILE* out)
{
    ensureEntities();
    std::vector<std::string> names;
    for (const entity::EntityType* type : entity::EntityRegistry::instance().getAllTypes()) {
        names.push_back(type->name());
    }
    // 排序去重：编排层直接比对两侧列表，顺序不稳会制造伪差异。
    std::sort(names.begin(), names.end());
    names.erase(std::unique(names.begin(), names.end()), names.end());
    for (const std::string& name : names) {
        std::fprintf(out, "%s\n", name.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureEntities();
    const json input = json::parse(inputLine);
    const std::string requested = input.at("unit").get<std::string>();

    const entity::EntityType* type = entity::EntityRegistry::instance().getType(requested);
    if (type == nullptr) {
        return json{{"unit", requested}, {"missing", true}}.dump();
    }

    const entity::EntitySize size = type->size();
    return json{{"unit", requested},
        {"width", floatBits(size.width())},
        {"height", floatBits(size.height())},
        {"eyeHeight", floatBits(size.eyeHeight())},
        {"fixed", size.isFixed()}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(entity_type_size, &writeList, &runLine)

} // namespace mc::diff
