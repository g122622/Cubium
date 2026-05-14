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

#include "../../Block.hpp"

namespace mc {

class IWorld;

namespace blocks {

/**
 * @brief 被感染方块基类
 *
 * 外观与普通方块相同，但被破坏时会生成蠹虫。
 *
 * 参考: net.minecraft.block.InfestedBlock
 */
class InfestedBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param hostBlock 被感染的方块ID
     * @param properties 方块属性
     */
    InfestedBlock(u32 hostBlock, const BlockProperties& properties);
    ~InfestedBlock() override = default;

    // ========== 破坏 ==========

    void onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    // ========== 掉落 ==========

    [[nodiscard]] u32 getHostBlock() const { return m_hostBlock; }

private:
    /// 被感染的方块ID
    u32 m_hostBlock;
};

} // namespace blocks
} // namespace mc
