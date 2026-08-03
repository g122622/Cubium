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

#include "common/mod/bedrock/addon/plugin/ScriptPackPermissions.hpp"
#include "common/core/Types.hpp"
#include <string>
#include <vector>

namespace mc::mod::bedrock::addon {

ScriptPackPermissions::ScriptPackPermissions(const std::vector<std::string>& capabilities)
{
    for (const auto& cap : capabilities) {
        if (cap == "script_eval") {
            setPermission(ScriptPermission::AllowEval);
        }
    }
}

void ScriptPackPermissions::setPermission(ScriptPermission perm, bool enabled) noexcept
{
    if (enabled) {
        m_flags |= static_cast<u32>(perm);
    } else {
        m_flags &= ~static_cast<u32>(perm);
    }
}

bool ScriptPackPermissions::hasPermission(ScriptPermission perm) const noexcept
{
    return (m_flags & static_cast<u32>(perm)) != 0;
}

} // namespace mc::mod::bedrock::addon
