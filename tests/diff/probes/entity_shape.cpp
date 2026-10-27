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
 * @file entity_shape.cpp
 * @brief 差分能力：实体类型的碰撞箱尺寸。
 *
 * `list`：全部实体类型 ID
 * `run` ：宽度、高度、眼睛高度、是否固定尺寸、追踪距离、更新间隔
 *
 * 尺寸按位以十六进制输出。实体宽高决定碰撞、寻路与刷怪位置，
 * 错一位在游戏里表现为"卡住"或"穿模"，单测几乎不可能覆盖全部实体。
 */
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/entity/core/EntityClassification.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/registry/VanillaEntities.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

using entity::EntityClassification;
using entity::EntityRegistry;
using entity::EntitySize;
using entity::EntityType;

void ensureEntities()
{
    static bool s_initialized = false;
    if (!s_initialized) {
        entity::VanillaEntities::registerAll();
        s_initialized = true;
    }
}

std::string floatBits(f32 value)
{
    u32 bits = 0;
    static_assert(sizeof(bits) == sizeof(value), "f32 必须是 4 字节");
    std::memcpy(&bits, &value, sizeof(bits));
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "0x%08x", bits);
    return buffer;
}

/**
 * @brief 分类的稳定名字。
 *
 * 用名字而不是枚举数值：数值是本项目的内部编号，参照数据给的是
 * building / breeding / hostile / misc 这类类型名，编号对齐没有意义。
 */
const char* classificationName(EntityClassification classification)
{
    switch (classification) {
        case EntityClassification::Monster:
            return "monster";
        case EntityClassification::Creature:
            return "creature";
        case EntityClassification::Ambient:
            return "ambient";
        case EntityClassification::Axolotls:
            return "axolotls";
        case EntityClassification::UndergroundWaterCreature:
            return "underground_water_creature";
        case EntityClassification::WaterCreature:
            return "water_creature";
        case EntityClassification::WaterAmbient:
            return "water_ambient";
        case EntityClassification::Misc:
            return "misc";
    }
    return "unknown";
}

void writeList(FILE* out)
{
    ensureEntities();
    std::vector<std::string> names;
    for (const EntityType* type : EntityRegistry::instance().getAllTypes()) {
        names.push_back(type->name());
    }
    std::sort(names.begin(), names.end());
    for (const auto& name : names) {
        std::fprintf(out, "%s\n", name.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureEntities();
    const json input = json::parse(inputLine);
    const std::string requested = input.at("entity").get<std::string>();

    const EntityType* type = EntityRegistry::instance().getType(requested);
    if (type == nullptr) {
        return json{{"entity", requested}, {"missing", true}}.dump();
    }

    const EntitySize size = type->size();
    return json{{"entity", requested},
        {"width", floatBits(size.width())},
        {"height", floatBits(size.height())},
        {"eyeHeight", floatBits(size.eyeHeight())},
        {"fixed", size.isFixed()},
        {"classification", classificationName(type->classification())},
        {"trackingRange", type->trackingRange()},
        {"updateInterval", type->updateInterval()}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("entity_shape", &writeList, &runLine)

} // namespace mc::diff
