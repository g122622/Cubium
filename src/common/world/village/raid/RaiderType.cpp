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

#include "RaiderType.hpp"
#include "common/core/Types.hpp"

namespace mc {
namespace world {
namespace village {
namespace raid {

// ============================================================================
// RaiderTypeHelper 实现
// ============================================================================

const char* RaiderTypeHelper::getName(RaiderType type) noexcept
{
    switch (type) {
        case RaiderType::Pillager:
            return "Pillager";
        case RaiderType::Vindicator:
            return "Vindicator";
        case RaiderType::Evoker:
            return "Evoker";
        case RaiderType::Ravager:
            return "Ravager";
        case RaiderType::Witch:
            return "Witch";
        default:
            return "Unknown";
    }
}

f32 RaiderTypeHelper::getBaseHealth(RaiderType type) noexcept
{
    switch (type) {
        case RaiderType::Pillager:
            return 24.0f;
        case RaiderType::Vindicator:
            return 24.0f;
        case RaiderType::Evoker:
            return 24.0f;
        case RaiderType::Ravager:
            return 100.0f;
        case RaiderType::Witch:
            return 26.0f;
        default:
            return 20.0f;
    }
}

i32 RaiderTypeHelper::getSpawnWeight(RaiderType type, i32 wave) noexcept
{
    // 根据波次调整生成权重
    // 后期波次增加更强的敌人权重
    switch (type) {
        case RaiderType::Pillager:
            return 10; // 始终常见

        case RaiderType::Vindicator:
            return wave >= 3 ? 5 : 0; // 第3波后出现

        case RaiderType::Evoker:
            return wave >= 5 ? 1 : 0; // 第5波后出现

        case RaiderType::Ravager:
            return wave >= 6 ? 1 : 0; // 第6波后出现

        case RaiderType::Witch:
            return wave >= 4 ? 2 : 0; // 第4波后出现

        default:
            return 0;
    }
}

bool RaiderTypeHelper::canRideRavager(RaiderType type) noexcept
{
    // 掠夺者和灾厄村民可以骑劫掠兽
    switch (type) {
        case RaiderType::Pillager:
        case RaiderType::Vindicator:
        case RaiderType::Evoker:
            return true;
        default:
            return false;
    }
}

} // namespace raid
} // namespace village
} // namespace world
} // namespace mc
