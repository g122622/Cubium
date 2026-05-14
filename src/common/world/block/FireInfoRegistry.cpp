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

#include "FireInfoRegistry.hpp"
#include "VanillaBlocks.hpp"

namespace mc {
namespace blocks {

// ============================================================================
// FireInfoRegistry
// ============================================================================

FireInfoRegistry& FireInfoRegistry::instance()
{
    static FireInfoRegistry s_instance;
    return s_instance;
}

void FireInfoRegistry::registerFireInfo(u32 blockId, i32 encouragement, i32 flammability)
{
    m_fireInfos[blockId] = FireInfo(encouragement, flammability);
}

FireInfo FireInfoRegistry::getFireInfo(u32 blockId) const
{
    auto it = m_fireInfos.find(blockId);
    if (it != m_fireInfos.end()) {
        return it->second;
    }
    return FireInfo(0, 0);
}

i32 FireInfoRegistry::getFlammability(u32 blockId) const
{
    return getFireInfo(blockId).flammability;
}

i32 FireInfoRegistry::getEncouragement(u32 blockId) const
{
    return getFireInfo(blockId).encouragement;
}

void FireInfoRegistry::clear()
{
    m_fireInfos.clear();
}

void FireInfoRegistry::initializeVanillaFireInfos()
{
    // 参考 MC 1.16.5: net.minecraft.block.FireBlock.init()
    // 参数: encouragement (蔓延速度), flammability (可燃性)
    // 可燃性范围: 0-300，值越高越容易被点燃和烧毁
    //
    // 注意：这里的注册使用占位 ID，实际的方块燃烧参数
    // 应该在 VanillaBlocks 初始化后通过 Block::registerFireInfo() 方法注册。
    // 此函数目前仅作为示例保留，实际使用时需要在方块初始化时调用：
    // if (VanillaBlocks::OAK_PLANKS != nullptr) {
    //     registerFireInfo(VanillaBlocks::OAK_PLANKS->blockId(), 5, 20);
    // }
    //
    // 目前方块燃烧参数在 Block 基类的 getFlammability/getFireSpreadSpeed 方法中
    // 通过 Material::isFlammable() 和其他属性进行判断，此注册表作为扩展预留。
}

} // namespace blocks
} // namespace mc
