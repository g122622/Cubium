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
 * @brief 差分能力：乐器（数据包 `data/<ns>/instrument/` 下的 JSON，原版 8 种山羊角音色）。
 *
 * `list`：全部乐器 ID（排序）
 * `run` ：{ "instrument": "minecraft:admire_goat_horn" }
 *         → { "soundEvent": "minecraft:item.goat_horn.sound.4",
 *             "useDuration": "7", "range": "256",
 *             "descriptionKey": "instrument.minecraft.admire_goat_horn",
 *             "useDurationTicks": 140,
 *             "volume": "16" }
 *
 * 对应原版 `record Instrument(Holder<SoundEvent> soundEvent, float useDuration,
 * float range, Component description)`。
 *
 * 三处口径：
 * - `useDuration` / `range` / `volume` 是 f32，两侧按「6 位有效数字再去掉尾随零」格式化
 *   成字符串输出，避免 C++ 与 Java 的浮点打印差异（food_properties / jukebox_song
 *   两个能力都踩过这个坑）。
 * - `description` 比**翻译键**而不是渲染后的文本。
 * - `useDurationTicks` 与 `volume` 是派生值，单独比：它们各自复刻了原版
 *   `InstrumentItem` 里的一行换算（`Mth.floor(useDuration * 20.0F)` 与
 *   `range / 16.0F`），换算口径错了这两个字段会立刻暴露。
 */
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/item/instrument/InstrumentDefinition.hpp"
#include "common/item/instrument/InstrumentRegistry.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "server/item/instrument/InstrumentLoader.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/**
 * @brief 把浮点数格式化成「6 位有效数字，去掉尾随零」的字符串。
 *
 * 两侧必须用完全相同的规则：C++ 的 `%.6g` 与 Java 的 `%g` 行为不同
 * （`178.0f` 一边给 `178`、另一边给 `178.000`），所以这里先按 6 位有效数字格式化，
 * 再由双方各自剥掉尾随零与多余的小数点。
 */
std::string formatFloat(float value)
{
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.6g", static_cast<double>(value));
    std::string text(buffer);
    // 去掉尾随零（"7.00000" -> "7"，"0.500000" -> "0.5"）。
    if (text.find('.') != std::string::npos && text.find('e') == std::string::npos &&
        text.find('E') == std::string::npos) {
        while (!text.empty() && text.back() == '0') {
            text.pop_back();
        }
        if (!text.empty() && text.back() == '.') {
            text.pop_back();
        }
    }
    return text;
}

/**
 * @brief 确保乐器注册表已从数据包加载。
 */
void ensureInstrumentsLoaded()
{
    static bool s_initialized = false;
    if (s_initialized) {
        return;
    }

    const char* root = std::getenv("MC_DIFF_VANILLA_DATAPACK");
    if (root == nullptr || *root == '\0') {
        throw std::runtime_error("MC_DIFF_VANILLA_DATAPACK 未设置：需要指向原版数据包根目录（含 data/ 的那一层）");
    }
    const std::filesystem::path packRoot(root);
    auto repository = std::make_unique<resource::DataPackRepository>();
    const auto scanResult = repository->scanDirectory(packRoot.parent_path());
    if (!scanResult.success() || scanResult.value() == 0) {
        throw std::runtime_error("instrument: 数据包扫描失败：" + packRoot.string());
    }

    // 与服务端 `RegistryBootstrap` 里的顺序一致：先清空、登记内置条目，再让数据包
    // 覆盖同名条目。少掉 `initialize()` 这一步，注册表在数据包缺失某些条目时会空掉；
    // 少掉 `clear()` 则会在同一进程内重复加载时残留旧条目。
    auto& registry = item::instrument::InstrumentRegistry::instance();
    registry.clear();
    registry.initialize();

    const auto loadResult = server::item::InstrumentLoader::loadFromDataPackRepository(*repository);
    if (loadResult.failed()) {
        throw std::runtime_error("instrument: 乐器加载失败：" + loadResult.error().toString());
    }
    s_initialized = true;
}

void writeList(FILE* out)
{
    ensureInstrumentsLoaded();
    std::vector<std::string> names;
    for (const auto& [id, definition] : item::instrument::InstrumentRegistry::instance().getAll()) {
        if (definition != nullptr) {
            names.push_back(id.toString());
        }
    }
    // 注册表内部无序，排序后再输出，两侧才可比。
    std::sort(names.begin(), names.end());
    for (const auto& name : names) {
        std::fprintf(out, "%s\n", name.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureInstrumentsLoaded();
    const json input = json::parse(inputLine);
    const std::string requested = input.at("instrument").get<std::string>();

    const auto* definition = item::instrument::InstrumentRegistry::instance().get(ResourceLocation(requested));
    if (definition == nullptr) {
        return json{{"instrument", requested}, {"missing", true}}.dump();
    }

    // 描述比翻译键而不是渲染文本（唱片机歌曲能力踩过 getString() 的坑）。
    // `ITextComponent` 没有暴露翻译键的访问器，但它序列化成 JSON 时会把键写进
    // `translate` 字段，这里从序列化结果里取——比给组件类加一个新接口影响面更小。
    std::string descriptionKey;
    if (definition->description() != nullptr) {
        const auto serialised = definition->description()->toJson();
        const auto translateIt = serialised.find("translate");
        if (translateIt != serialised.end() && translateIt->is_string()) {
            descriptionKey = translateIt->get<std::string>();
        } else {
            // 非可翻译组件（纯文本等）退化为渲染文本，与原版 getString() 的口径一致。
            descriptionKey = definition->description()->getUnformattedText();
        }
    }

    return json{{"instrument", requested},
        {"soundEvent", definition->soundEvent().toString()},
        {"useDuration", formatFloat(definition->useDuration())},
        {"range", formatFloat(definition->range())},
        {"descriptionKey", descriptionKey},
        {"useDurationTicks", definition->useDurationTicks()},
        {"volume", formatFloat(definition->volume())}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("instrument", &writeList, &runLine)

} // namespace mc::diff
