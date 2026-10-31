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
 * @file experience.cpp
 * @brief 差分能力：玩家经验值的等级换算与操作序列。
 *
 * `list`：不枚举（场景由编排层用固定种子生成，两侧读同一份输入）
 * `run` ：{ "unit": "...", "ops": [{"op":"points","n":7} | {"op":"levels","n":3}, ...] }
 *         → { "level": i32, "progress": "0x…", "total": i32 }
 *
 * ## 覆盖什么
 *
 * 三层语义：
 *   1. **纯公式**：累计经验 ↔ 等级的换算、每级所需经验的阶梯（0-14 / 15-29 / 30+ 三段）；
 *   2. **状态机**：加点与扣点后等级、进度条、累计经验的联动（原版 `giveExperiencePoints`
 *      的两次 while 循环，含跨多级与降到 0 级的夹取）；
 *   3. **浮点**：`experienceProgress` 是 f32，进度条按位模式比较，避免十进制往返掩盖末位差异。
 *
 * ## 与世界的依赖
 *
 * `ExperienceManager` 只在「升级播放音效」与「读取 `ticksExisted`」处用到玩家。
 * 探针给一个未进世的 `Player`，世界的音效实现是空操作，因此**不会触碰任何世界状态**。
 */
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/experience/ExperienceManager.hpp"

#include "probe.hpp"
#include "probe_world.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/** f32 的位模式：跨语言逐位比较，避免十进制往返带来的假差异。 */
std::string f32Bits(f32 value)
{
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "0x%08x", bits);
    return buffer;
}

/**
 * @brief 一个未进世的玩家，供 `ExperienceManager` 构造。
 *
 * 经验换算不读写玩家状态；构造它只是满足 `ExperienceManager` 的构造签名。
 */
struct ExperienceFixture {
    probe::NullWorld world;
    std::unique_ptr<Player> player;

    ExperienceFixture()
        : player(std::make_unique<Player>(EntityInstanceId(0), "diff_xp", *world.entityRegistry()))
    {}

    ExperienceFixture(const ExperienceFixture&) = delete;
    ExperienceFixture& operator=(const ExperienceFixture&) = delete;
    ExperienceFixture(ExperienceFixture&&) = delete;
    ExperienceFixture& operator=(ExperienceFixture&&) = delete;
};

/**
 * `list` 不枚举场景：场景（操作序列）完全由编排层生成的输入行携带。
 */
void writeList(FILE* /*out*/) {}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);

    ExperienceFixture fixture;
    entity::experience::ExperienceManager experience(*fixture.player);

    for (const auto& entry : input.at("ops")) {
        const std::string op = entry.at("op").get<std::string>();
        const i32 amount = entry.at("n").get<i32>();
        if (op == "points") {
            experience.addExperience(amount);
        } else if (op == "levels") {
            experience.addLevels(amount);
        } else {
            throw std::runtime_error("unknown experience op: " + op);
        }
    }

    return json{{"unit", input.at("unit").get<std::string>()},
        {"level", experience.getLevel()},
        {"progress", f32Bits(experience.getProgress())},
        {"total", experience.getTotalExperience()}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("experience", &writeList, &runLine)

} // namespace mc::diff
