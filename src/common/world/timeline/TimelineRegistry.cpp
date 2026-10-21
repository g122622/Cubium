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

#include "common/world/timeline/TimelineRegistry.hpp"

#include "common/world/timeline/Timelines.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <deque>

namespace mc {
namespace world {
namespace timeline {
namespace {

struct _RegistryStorage {
    std::vector<TimelineRegistry::Entry> entries;
    /// 数据包注册的时间线（内置项为 nullptr）
    std::deque<std::unique_ptr<Timeline>> owned;
};

_RegistryStorage& _storage()
{
    static _RegistryStorage storage;
    return storage;
}

std::string _qualifiedName(std::string_view name)
{
    constexpr std::string_view DEFAULT_NAMESPACE = "minecraft:";
    return name.starts_with(DEFAULT_NAMESPACE) ? std::string(name) : "minecraft:" + std::string(name);
}

} // namespace

TimelineRegistry::Entry::Entry(std::string name, const Timeline* timeline)
    : name(std::move(name))
    , timeline(timeline)
{}

const TimelineRegistry::Entry* TimelineRegistry::find(std::string_view name)
{
    const std::string qualified = _qualifiedName(name);
    const auto& entries = _storage().entries;
    const auto it = std::find_if(
        entries.begin(), entries.end(), [&qualified](const Entry& entry) { return entry.name == qualified; });
    return it == entries.end() ? nullptr : &*it;
}

const std::vector<TimelineRegistry::Entry>& TimelineRegistry::all()
{
    return _storage().entries;
}

Result<void> TimelineRegistry::_register(
    std::string_view name, const Timeline* builtin, std::unique_ptr<Timeline> owned)
{
    auto& storage = _storage();
    const std::string qualified = _qualifiedName(name);

    // 先压入数据包时间线再查找同名项：deque 保证元素地址稳定，先压入可让覆盖逻辑看到最新内容
    const Timeline* timeline = builtin;
    if (owned != nullptr) {
        storage.owned.push_back(std::move(owned));
        timeline = storage.owned.back().get();
    }

    const auto it = std::find_if(storage.entries.begin(), storage.entries.end(), [&qualified](const Entry& entry) {
        return entry.name == qualified;
    });

    if (it != storage.entries.end()) {
        // 数据包覆盖内置项时替换指针；内置项重复登记时保持原状
        if (timeline != nullptr) {
            it->timeline = timeline;
        }
        return Result<void>::ok();
    }

    if (timeline == nullptr) {
        return Error::invalidArgument("Timeline entry has no timeline: " + qualified);
    }

    storage.entries.push_back(Entry{qualified, timeline});
    return Result<void>::ok();
}

Result<size_t> TimelineRegistry::registerAll()
{
    // 强制构建各预定义时间线后再登记（Meyers singleton）
    const std::vector<Entry> builtins = {
        Entry{"minecraft:day", &Timelines::DAY()},
        Entry{"minecraft:moon", &Timelines::MOON()},
        Entry{"minecraft:villager_schedule", &Timelines::VILLAGER_SCHEDULE()},
        Entry{"minecraft:early_game", &Timelines::EARLY_GAME()},
    };

    size_t registered = 0;
    for (const auto& builtin : builtins) {
        const bool existed = find(builtin.name) != nullptr;
        const auto result = _register(builtin.name, builtin.timeline, nullptr);
        if (result.success() && !existed) {
            ++registered;
        }
    }
    return registered;
}

Result<void> TimelineRegistry::registerFromDataPack(std::string_view name, std::unique_ptr<Timeline> timeline)
{
    if (name.empty()) {
        return Error::invalidArgument("Timeline name cannot be empty");
    }
    if (timeline == nullptr) {
        return Error::invalidArgument("Timeline cannot be null: " + std::string(name));
    }
    return _register(name, nullptr, std::move(timeline));
}

} // namespace timeline
} // namespace world
} // namespace mc