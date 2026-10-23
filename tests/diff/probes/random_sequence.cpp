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
 * @file random_sequence.cpp
 * @brief 差分能力：随机数生成器的逐位序列。
 *
 * `list`：`<算法>:<种子>[:<预热次数>]` 形式的单元
 * `run` ：→ 依次调用 nextInt/nextLong/nextFloat/nextDouble/nextInt(bound)/nextBits(1..8)
 *          得到的值（整数原样、浮点用位模式）
 *
 * 世界生成、刷怪、战利品全建立在这几个生成器上：序列差一位，整片地形就会不同。
 * 单个调用点很难看出问题，只能把序列整段对齐。
 */
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "common/util/math/random/Xoroshiro128ppRandom.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

constexpr const char* ALGORITHMS[] = {"xoroshiro128pp", "java_legacy"};

constexpr std::int64_t SEEDS[] = {0, 1, 42, 123456789, -1, 9007199254740993LL};

constexpr int WARMUPS[] = {0, 7};

/** i32 序列（覆盖正负、极值、bound）。 */
constexpr std::int32_t INT_BOUNDS[] = {1, 2, 7, 16, 100, 1000, 2147483647};

std::string f32Bits(f32 value)
{
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "0x%08x", bits);
    return buffer;
}

std::string f64Bits(f64 value)
{
    std::uint64_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    char buffer[24];
    std::snprintf(buffer, sizeof(buffer), "0x%016llx", static_cast<unsigned long long>(bits));
    return buffer;
}

std::unique_ptr<math::IRandom> make(const std::string& algorithm, std::int64_t seed)
{
    if (algorithm == "xoroshiro128pp") {
        return std::make_unique<math::Xoroshiro128ppRandom>(static_cast<u64>(seed));
    }
    if (algorithm == "java_legacy") {
        return std::make_unique<math::JavaLegacyRandom>(seed);
    }
    return nullptr;
}

void writeList(FILE* out)
{
    for (const char* algorithm : ALGORITHMS) {
        for (const auto seed : SEEDS) {
            for (const auto warmup : WARMUPS) {
                std::fprintf(out, "%s:%lld:%d\n", algorithm, static_cast<long long>(seed), warmup);
            }
        }
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    const size_t firstSep = unit.find(':');
    const size_t lastSep = unit.rfind(':');
    if (firstSep == std::string::npos || firstSep == lastSep) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }
    const std::string algorithm = unit.substr(0, firstSep);
    const std::string seedText = unit.substr(firstSep + 1, lastSep - firstSep - 1);
    const int warmup = std::stoi(unit.substr(lastSep + 1));

    std::unique_ptr<math::IRandom> random = make(algorithm, std::stoll(seedText));
    if (random == nullptr) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }
    for (int i = 0; i < warmup; ++i) {
        static_cast<void>(random->nextU64());
    }

    std::vector<std::int64_t> longs;
    std::vector<std::int32_t> ints;
    std::vector<std::int32_t> bounded[std::size(INT_BOUNDS)];
    std::vector<std::string> floats;
    std::vector<std::string> doubles;
    std::vector<std::int32_t> narrow;

    for (int i = 0; i < 8; ++i) {
        longs.push_back(random->nextLong());
        ints.push_back(random->nextInt());
        for (size_t b = 0; b < std::size(INT_BOUNDS); ++b) {
            bounded[b].push_back(random->nextInt(INT_BOUNDS[b]));
        }
        floats.push_back(f32Bits(random->nextFloat()));
        doubles.push_back(f64Bits(random->nextDouble()));
        // 窄位宽 nextBits：原版 nextBits(1/2/.../8) 的结果落在 (-2^(b-1), 2^(b-1))，
        // 高位置零，与 nextInt(bound) 的位宽路径同源。
        narrow.push_back(random->nextBits(static_cast<i32>((i % 8) + 1)));
    }

    json boundedJson = json::array();
    for (const auto& column : bounded) {
        boundedJson.push_back(column);
    }

    return json{{"unit", unit},
        {"long", longs},
        {"int", ints},
        {"bounded", std::move(boundedJson)},
        {"narrow", narrow},
        {"float", floats},
        {"double", doubles}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("random_sequence", &writeList, &runLine)

} // namespace mc::diff