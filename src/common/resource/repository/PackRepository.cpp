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

#include "PackRepository.hpp"

#include "common/perfetto/TraceEvents.hpp"
#include <spdlog/spdlog.h>

#include <algorithm>

using namespace mc::trace;

namespace mc::resource {

void PackRepository::loadFromSettings(const ResourcePackListOption& settings)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "PackRepository::loadFromSettings");

    for (const auto& entry : settings.getSortedEnabledEntries()) {
        auto result = addPack(std::filesystem::path(entry.path), entry.enabled, entry.priority);
        if (result.failed()) {
            spdlog::warn("Failed to add resource pack from settings {}: {}", entry.path, result.error().toString());
        }
    }
}

void PackRepository::saveToSettings(ResourcePackListOption& settings) const
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Resource, "PackRepository::saveToSettings");

    std::vector<ResourcePackEntry> entries;
    {
        auto allPacks = getAllPacks();
        entries.reserve(allPacks.size());
        for (const auto& info : allPacks) {
            entries.emplace_back(info.path, info.enabled, info.priority);
        }
    }

    settings.setEntries(std::move(entries));
}

void PackRepository::onPackListChanged()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Resource, "PackRepository::onPackListChanged");
}

} // namespace mc::resource
