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

#include "IInventory.hpp"
#include "../entities/player/Player.hpp"

namespace mc {

// ============================================================================
// IInventory 默认实现
// ============================================================================

bool IInventory::isUsableByPlayer(const Player& player) const
{
    // 默认实现：始终返回 true
    // 子类应重写此方法以检查距离
    (void)player;
    return true;
}

void IInventory::openInventory(Player& player)
{
    // 默认实现：空操作
    // 子类可以重写以实现打开计数、音效等功能
    (void)player;
}

void IInventory::closeInventory(Player& player)
{
    // 默认实现：空操作
    // 子类可以重写以实现关闭计数、物品返还等功能
    (void)player;
}

} // namespace mc
