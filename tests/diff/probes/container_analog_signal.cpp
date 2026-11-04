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
 * @brief 差分能力：容器的红石信号强度（`AbstractContainerMenu#getRedstoneSignalFromContainer`）。
 *
 * `list`：一组容器填充布局
 * `run` ：一行 -> { "signal": i32 }
 *
 * ## 原版算法
 *
 * ```java
 * float f = 0.0F;
 * for (int i = 0; i < container.getContainerSize(); i++) {
 *     ItemStack stack = container.getItem(i);
 *     if (!stack.isEmpty())
 *         f += (float)stack.getCount() / container.getMaxStackSize(stack);
 * }
 * f /= container.getContainerSize();
 * return Mth.lerpDiscrete(f, 0, 15);
 * ```
 *
 * 而 `Mth.lerpDiscrete(f, a, b)` 是 `a + floor(f * (b - a - 1)) + (f > 0 ? 1 : 0)`，
 * 代入 a=0 / b=15 得 `floor(f * 14) + (f > 0 ? 1 : 0)`。
 *
 * ## 三个容易写错的点
 *
 * 1. **每格按各自的 `maxStackSize` 归一化**，不是统一按 64。物品栏里混着可堆叠 64
 *    的物品与不可堆叠 1 的物品（工具、桶）时，若一律除以 64，放了 16 个工具的小箱子
 *    会被算成「只装了 4%」，比较器几乎没输出；正确算法下它们占满每一格。
 * 2. **`f > 0` 才 +1**，因此只要有一格不是空，最低也有 1，而空箱子是 0。
 * 3. 除法顺序：先逐格累加再整体除以**槽位数**，而不是先算每格比例再取平均到 15。
 *
 * ## 单元格式
 *
 * `<slots>;<count>/<max>|<count>/<max>|...`
 *
 * - `slots`：容器槽位数；
 * - 每格用 `数量/堆叠上限` 表示，`0/64` 表示空格子。格子数可以少于 `slots`，
 *   其余按空格处理。
 */

#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/blockentity/core/SimpleInventory.hpp"
#include "common/world/redstone/RedstoneHelper.hpp"

#include "vanilla_datapack.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

std::vector<std::string> splitPipe(std::string_view text)
{
    std::vector<std::string> tokens;
    std::string current;
    for (char ch : text) {
        if (ch == '|') {
            tokens.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    tokens.push_back(current);
    return tokens;
}

/**
 * 场景表：`<slots>;<count>/<max>|...`
 *
 * 覆盖：空箱、单格满/半、部分填充、满箱、不可堆叠物品混合、单格容器、
 * 大容器、以及刚好跨过每个整数边界的填充率。
 */
constexpr const char* SCENES[] = {
    // 空容器
    "27;",
    "9;0/64|0/64|0/64|0/64|0/64|0/64|0/64|0/64|0/64",
    // 单格容器：0 -> 满
    "1;1/64",
    "1;32/64",
    "1;64/64",
    // 单格不可堆叠
    "1;1/1",
    // 27 格里只放一格
    "27;1/64",
    "27;64/64",
    // 混入不可堆叠物品（工具）：每格 1/1 即视为满格
    "9;1/1|1/1|1/1|||",
    // 半箱
    "9;64/64|64/64|64/64|64/64|||",
    // 满箱
    "9;64/64|64/64|64/64|64/64|64/64|64/64|64/64|64/64|64/64",
    // 一格 16 个可堆叠物品（16/64 = 0.25 格）
    "1;16/64",
    "1;48/64",
    // 大容器（54 格箱子）
    "54;64/64",
    "54;64/64|64/64|64/64",
};

void writeList(FILE* out)
{
    for (const char* scene : SCENES) {
        std::fprintf(out, "%s\n", scene);
    }
}

/// 按堆叠上限挑一个真实物品（结果缓存，避免每格都扫全表）。
const Item* pickItemWithMaxStack(i32 maxSize)
{
    static std::unordered_map<i32, const Item*> cache;
    const auto found = cache.find(maxSize);
    if (found != cache.end()) {
        return found->second;
    }

    const Item* selected = nullptr;
    ItemRegistry::instance().forEachItem([&selected, maxSize](Item& item) {
        if (selected != nullptr) {
            return;
        }
        if (item.maxStackSize() == maxSize) {
            selected = &item;
        }
    });

    cache.emplace(maxSize, selected);
    return selected;
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    const std::size_t semicolon = unit.find(';');
    if (semicolon == std::string::npos) {
        throw std::runtime_error("container_analog_signal: 单元应为 'slots;cells'：" + unit);
    }
    // 物品注册表与数据包是惰性初始化的，探针是单次进程，静态标志足够。
    static const bool s_initialized = []() {
        datapack::initializeRegistries();
        Items::initialize();
        return true;
    }();
    (void)s_initialized;

    const i32 slots = std::stoi(unit.substr(0, semicolon));
    const std::string cellsPart = unit.substr(semicolon + 1);

    blockentity::SimpleInventory inventory(slots);

    if (!cellsPart.empty()) {
        const std::vector<std::string> cells = splitPipe(cellsPart);
        for (std::size_t i = 0; i < cells.size() && static_cast<i32>(i) < slots; ++i) {
            const std::string& cell = cells[i];
            if (cell.empty()) {
                continue;
            }
            const std::size_t slash = cell.find('/');
            if (slash == std::string::npos) {
                throw std::runtime_error("container_analog_signal: 格子应为 'count/max'：" + cell);
            }
            const i32 count = std::stoi(cell.substr(0, slash));
            const i32 maxSize = std::stoi(cell.substr(slash + 1));
            if (count <= 0) {
                continue;
            }

            // 按需要的 maxStackSize 选一个**真实且上限匹配**的物品：
            // 单元里的 `max` 是期望的堆叠上限，必须找到一个该上限的物品，
            // 否则 ItemStack::getMaxStackSize() 会返回物品自己的值，比值就不是单元要的。
            const Item* item = pickItemWithMaxStack(maxSize);
            if (item == nullptr) {
                throw std::runtime_error(
                    "container_analog_signal: 找不到堆叠上限为 " + std::to_string(maxSize) + " 的物品");
            }
            inventory.setItem(static_cast<i32>(i), ItemStack(*item, count));
        }
    }

    const i32 signal = world::redstone::RedstoneHelper::calcRedstoneFromInventory(inventory);
    return json{{"unit", unit}, {"signal", signal}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("container_analog_signal", &writeList, &runLine)

} // namespace mc::diff
