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

#pragma once
#include "BlockSetType.hpp"

namespace mc {
/** @brief 木材族描述；告示牌与栅栏门共用同一份定义。 */
struct WoodType {
    std::string_view name;
    const BlockSetType& setType;
    const BlockSoundType& soundType;
    const BlockSoundType& hangingSignSoundType;
    const ResourceLocation& fenceGateClose;
    const ResourceLocation& fenceGateOpen;

    [[nodiscard]] bool operator==(const WoodType& other) const noexcept
    {
        return name == other.name;
    }
};

namespace WoodTypes {
extern const WoodType OAK;
extern const WoodType SPRUCE;
extern const WoodType BIRCH;
extern const WoodType JUNGLE;
extern const WoodType ACACIA;
extern const WoodType DARK_OAK;
extern const WoodType PALE_OAK;
extern const WoodType CRIMSON;
extern const WoodType WARPED;
extern const WoodType MANGROVE;
extern const WoodType CHERRY;
extern const WoodType BAMBOO;
} // namespace WoodTypes
} // namespace mc
