/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include "world/blockentity/interactive/DecoratedPotPattern.hpp"
#include "util/assert/AssertAll.hpp"
#include <cstddef>
#include <string>

namespace mc {
namespace blockentity {

namespace {

/// 图案简单名称表，索引与 DecoratedPotPattern 枚举值对应
const char* const PATTERN_NAMES[] = {
    "blank",      // Blank
    "angler",     // Angler
    "archer",     // Archer
    "arms_up",    // ArmsUp
    "blade",      // Blade
    "brewer",     // Brewer
    "burn",       // Burn
    "danger",     // Danger
    "explorer",   // Explorer
    "friend",     // Friend
    "heart",      // Heart
    "heartbreak", // Heartbreak
    "howl",       // Howl
    "miner",      // Miner
    "mourner",    // Mourner
    "plenty",     // Plenty
    "prize",      // Prize
    "sheaf",      // Sheaf
    "shelter",    // Shelter
    "skull",      // Skull
    "snort",      // Snort
    "flow",       // Flow
    "guster",     // Guster
    "scrape"      // Scrape
};

/// 图案纹理资源路径表，索引与 DecoratedPotPattern 枚举值对应
/// 非 Blank 图案格式为 "{name}_pottery_pattern"，Blank 图案为 "decorated_pot_side"
const char* const PATTERN_ASSET_IDS[] = {
    "decorated_pot_side",         // Blank
    "angler_pottery_pattern",     // Angler
    "archer_pottery_pattern",     // Archer
    "arms_up_pottery_pattern",    // ArmsUp
    "blade_pottery_pattern",      // Blade
    "brewer_pottery_pattern",     // Brewer
    "burn_pottery_pattern",       // Burn
    "danger_pottery_pattern",     // Danger
    "explorer_pottery_pattern",   // Explorer
    "friend_pottery_pattern",     // Friend
    "heart_pottery_pattern",      // Heart
    "heartbreak_pottery_pattern", // Heartbreak
    "howl_pottery_pattern",       // Howl
    "miner_pottery_pattern",      // Miner
    "mourner_pottery_pattern",    // Mourner
    "plenty_pottery_pattern",     // Plenty
    "prize_pottery_pattern",      // Prize
    "sheaf_pottery_pattern",      // Sheaf
    "shelter_pottery_pattern",    // Shelter
    "skull_pottery_pattern",      // Skull
    "snort_pottery_pattern",      // Snort
    "flow_pottery_pattern",       // Flow
    "guster_pottery_pattern",     // Guster
    "scrape_pottery_pattern"      // Scrape
};

constexpr size_t PATTERN_COUNT = static_cast<size_t>(DecoratedPotPattern::Count);

} // namespace

DecoratedPotPattern DecoratedPotPatterns::byName(const std::string& name)
{
    for (size_t i = 0; i < PATTERN_COUNT; ++i) {
        if (name == PATTERN_NAMES[i]) {
            return static_cast<DecoratedPotPattern>(i);
        }
    }
    return DecoratedPotPattern::Blank;
}

std::string DecoratedPotPatterns::getAssetId(DecoratedPotPattern pattern)
{
    const size_t index = static_cast<size_t>(pattern);
    MC_ASSERT_RELEASE(index < PATTERN_COUNT);
    return PATTERN_ASSET_IDS[index];
}

std::string DecoratedPotPatterns::getTranslationKey(DecoratedPotPattern pattern)
{
    if (pattern == DecoratedPotPattern::Blank) {
        return "block.minecraft.decorated_pot";
    }
    const size_t index = static_cast<size_t>(pattern);
    MC_ASSERT_RELEASE(index < PATTERN_COUNT);
    return std::string("item.minecraft.") + PATTERN_NAMES[index] + "_pottery_sherd";
}

std::string DecoratedPotPatterns::getName(DecoratedPotPattern pattern)
{
    const size_t index = static_cast<size_t>(pattern);
    MC_ASSERT_RELEASE(index < PATTERN_COUNT);
    return PATTERN_NAMES[index];
}

} // namespace blockentity
} // namespace mc
