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
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/base/sound/BlockSoundType.hpp"
#include <string_view>

namespace mc {
enum class PressurePlateSensitivity : u8 { Everything, Mobs };

/** @brief 同一方块族共享的声音与激活规则；实例在进程期间保持不变。 */
struct BlockSetType {
    std::string_view name;
    bool canOpenByHand;
    bool canOpenByWindCharge;
    bool canButtonBeActivatedByArrows;
    PressurePlateSensitivity pressurePlateSensitivity;
    const BlockSoundType& soundType;
    const ResourceLocation& doorClose;
    const ResourceLocation& doorOpen;
    const ResourceLocation& trapdoorClose;
    const ResourceLocation& trapdoorOpen;
    const ResourceLocation& pressurePlateClickOff;
    const ResourceLocation& pressurePlateClickOn;
    const ResourceLocation& buttonClickOff;
    const ResourceLocation& buttonClickOn;
};

namespace BlockSetTypes {
extern const BlockSetType IRON;
extern const BlockSetType COPPER;
extern const BlockSetType GOLD;
extern const BlockSetType STONE;
extern const BlockSetType POLISHED_BLACKSTONE;
extern const BlockSetType OAK;
extern const BlockSetType SPRUCE;
extern const BlockSetType BIRCH;
extern const BlockSetType JUNGLE;
extern const BlockSetType ACACIA;
extern const BlockSetType DARK_OAK;
extern const BlockSetType PALE_OAK;
extern const BlockSetType CRIMSON;
extern const BlockSetType WARPED;
extern const BlockSetType MANGROVE;
extern const BlockSetType CHERRY;
extern const BlockSetType BAMBOO;
} // namespace BlockSetTypes
} // namespace mc
