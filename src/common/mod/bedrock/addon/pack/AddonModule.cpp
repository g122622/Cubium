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

#include "common/mod/bedrock/addon/pack/AddonModule.hpp"
#include <string_view>

namespace mc::mod::bedrock::addon {

AddonModuleType AddonModule::parseType(std::string_view typeStr) noexcept
{
    if (typeStr == "script") {
        return AddonModuleType::Script;
    }
    if (typeStr == "data") {
        return AddonModuleType::Data;
    }
    if (typeStr == "resources") {
        return AddonModuleType::Resources;
    }
    if (typeStr == "skin_pack") {
        return AddonModuleType::SkinPack;
    }
    if (typeStr == "world_template") {
        return AddonModuleType::WorldTemplate;
    }
    return AddonModuleType::Unknown;
}

const char* AddonModule::typeToString() const noexcept
{
    switch (type) {
        case AddonModuleType::Script:
            return "script";
        case AddonModuleType::Data:
            return "data";
        case AddonModuleType::Resources:
            return "resources";
        case AddonModuleType::SkinPack:
            return "skin_pack";
        case AddonModuleType::WorldTemplate:
            return "world_template";
        default:
            return "unknown";
    }
}

} // namespace mc::mod::bedrock::addon
