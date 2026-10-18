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

#include "ListOperation.hpp"
#include <optional>
#include <string>

namespace mc {
namespace loot {

std::optional<ListOperationKind> listOperationKindFromName(std::string_view name)
{
    constexpr std::string_view PREFIX = "minecraft:";
    if (name.size() > PREFIX.size() && name.substr(0, PREFIX.size()) == PREFIX) {
        name.remove_prefix(PREFIX.size());
    }
    if (name == "append") {
        return ListOperationKind::Append;
    }
    if (name == "insert") {
        return ListOperationKind::Insert;
    }
    if (name == "replace_all") {
        return ListOperationKind::ReplaceAll;
    }
    return std::nullopt;
}

} // namespace loot
} // namespace mc
