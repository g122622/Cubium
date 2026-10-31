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
 * @file jukebox_song.cpp
 * @brief 差分能力：唱片机歌曲（`data/minecraft/jukebox_song/*.json`）。
 *
 * `list`：全部歌曲 ID（排序）
 * `run` ：{ "song": "minecraft:13" }
 *         → { "soundEvent": "minecraft:music_disc.13",
 *             "descriptionKey": "jukebox_song.minecraft.13",
 *             "lengthInSeconds": "178", "comparatorOutput": 1 }
 *
 * 原版 `JukeboxSong` 是
 * `record JukeboxSong(Holder<SoundEvent> soundEvent, Component description,
 *  float lengthInSeconds, int comparatorOutput)`
 * 四个字段本能力全都比。
 *
 * `lengthInSeconds` 是 f32，按 6 位有效数字输出成字符串：同一个 `178.0f`，Java 侧
 * 经 Gson 打出 `178.0`，C++ 侧 nlohmann 会打出 `178`。两者是同一个数，差别只在打印
 * 精度，统一口径后才能逐字节比较（这个坑在 food_properties 里踩过一次）。
 */
#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/jukebox/JukeboxSong.hpp"
#include "common/sound/jukebox/JukeboxSongs.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/**
 * @brief 把 f32 按 6 位有效数字格式化。
 *
 * 与 Java 侧 `Float.toString` 的口径对齐，避免 `178.0f` 在两侧打成 `178.0` / `178`
 * 这种纯粹由打印精度造成的假差异。
 */
std::string formatSeconds(f32 value)
{
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.6g", static_cast<f64>(value));
    return buffer;
}

void ensureSongs()
{
    if (!JukeboxSongs::isInitialized()) {
        JukeboxSongs::initialize();
    }
}

void writeList(FILE* out)
{
    ensureSongs();
    std::vector<std::string> names;
    JukeboxSongs::forEachSong([&](const ResourceLocation& id, const JukeboxSong&) {
        names.push_back(id.toString());
    });
    // 注册表内部是无序 map，排序后再输出，两侧才可比。
    std::sort(names.begin(), names.end());
    for (const auto& name : names) {
        std::fprintf(out, "%s\n", name.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureSongs();
    const json input = json::parse(inputLine);
    const std::string requested = input.at("song").get<std::string>();

    const JukeboxSong* song = JukeboxSongs::getSong(ResourceLocation(requested));
    if (song == nullptr) {
        return json{{"song", requested}, {"missing", true}}.dump();
    }

    return json{{"song", requested},
        {"soundEvent", song->getSoundEventId().toString()},
        {"descriptionKey", song->getDescriptionKey()},
        {"lengthInSeconds", formatSeconds(song->getLengthInSeconds())},
        {"comparatorOutput", song->getComparatorOutput()}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("jukebox_song", &writeList, &runLine)

} // namespace mc::diff
