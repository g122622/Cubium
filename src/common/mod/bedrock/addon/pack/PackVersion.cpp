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

#include "common/mod/bedrock/addon/pack/PackVersion.hpp"
#include "common/core/Types.hpp"
#include <string>
#include <vector>

namespace mc::mod::bedrock::addon {

PackVersion PackVersion::fromVector(const std::vector<i32>& v)
{
    PackVersion version;
    if (v.size() >= 1) {
        version.major = v[0];
    }
    if (v.size() >= 2) {
        version.minor = v[1];
    }
    if (v.size() >= 3) {
        version.patch = v[2];
    }
    return version;
}

std::vector<i32> PackVersion::toVector() const noexcept
{
    return {major, minor, patch};
}

std::string PackVersion::toString() const noexcept
{
    return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
}

bool PackVersion::operator==(const PackVersion& o) const noexcept
{
    return major == o.major && minor == o.minor && patch == o.patch;
}

bool PackVersion::operator!=(const PackVersion& o) const noexcept
{
    return !(*this == o);
}

bool PackVersion::operator<(const PackVersion& o) const noexcept
{
    if (major != o.major) {
        return major < o.major;
    }
    if (minor != o.minor) {
        return minor < o.minor;
    }
    return patch < o.patch;
}

bool PackVersion::operator<=(const PackVersion& o) const noexcept
{
    return *this < o || *this == o;
}

bool PackVersion::operator>(const PackVersion& o) const noexcept
{
    return o < *this;
}

bool PackVersion::operator>=(const PackVersion& o) const noexcept
{
    return o <= *this;
}

bool PackVersion::isCompatibleWith(const PackVersion& required) const noexcept
{
    // 主版本号必须一致
    if (major != required.major) {
        return false;
    }
    // 次版本号必须大于等于要求
    if (minor > required.minor) {
        return true;
    }
    if (minor < required.minor) {
        return false;
    }
    // 补丁版本号必须大于等于要求
    return patch >= required.patch;
}

} // namespace mc::mod::bedrock::addon
