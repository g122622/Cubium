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
 */

#include "client/resource/atlas/IdentifierPattern.hpp"
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <regex>
#include <string>
#include <nlohmann/json_fwd.hpp>

namespace mc::client::resource::atlas {

bool IdentifierPattern::matches(const ResourceLocation& loc) const
{
    if (namespaceRegex) {
        if (!std::regex_search(loc.namespace_(), *namespaceRegex)) {
            return false;
        }
    }
    if (pathRegex) {
        if (!std::regex_search(loc.path(), *pathRegex)) {
            return false;
        }
    }
    return true;
}

Result<IdentifierPattern> IdentifierPattern::parse(const nlohmann::json& j)
{
    IdentifierPattern pattern;
    if (j.contains("namespace")) {
        const auto& ns = j["namespace"];
        if (!ns.is_string()) {
            return Error(ErrorCode::ResourceParseError, "filter pattern namespace must be a string");
        }
        try {
            pattern.namespaceRegex = std::regex(ns.get<std::string>());
        }
        catch (const std::regex_error& e) {
            return Error(ErrorCode::ResourceParseError, std::string("Invalid namespace regex: ") + e.what());
        }
    }
    if (j.contains("path")) {
        const auto& p = j["path"];
        if (!p.is_string()) {
            return Error(ErrorCode::ResourceParseError, "filter pattern path must be a string");
        }
        try {
            pattern.pathRegex = std::regex(p.get<std::string>());
        }
        catch (const std::regex_error& e) {
            return Error(ErrorCode::ResourceParseError, std::string("Invalid path regex: ") + e.what());
        }
    }
    return pattern;
}

} // namespace mc::client::resource::atlas
