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

#include "ParticleData.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include <memory>
#include <string>

namespace mc::client::renderer::trident::particle::data {

/**
 * @brief 方块粒子数据
 *
 * 用于需要方块状态信息的粒子类型，如方块破坏、下落灰尘等。
 *
 * 用法示例：
 * @code
 * BlockState state = BlockRegistry::get(BlockId::Stone).getDefaultState();
 * auto blockData = std::make_unique<BlockParticleData>(ParticleTypeId::Block, state);
 * @endcode
 */
class BlockParticleData : public ParticleData {
public:
    /**
     * @brief 构造方块粒子数据
     *
     * @param type 粒子类型 ID（必须是 Block, Breaking 或 FallingDust）
     * @param blockState 方块状态
     */
    BlockParticleData(ParticleTypeId type, BlockState blockState);

    ~BlockParticleData() override = default;

    // 允许拷贝
    BlockParticleData(const BlockParticleData&) = default;
    BlockParticleData& operator=(const BlockParticleData&) = default;

    // 允许移动
    BlockParticleData(BlockParticleData&&) noexcept = default;
    BlockParticleData& operator=(BlockParticleData&&) noexcept = default;

    // ========================================================================
    // ParticleData 接口实现
    // ========================================================================

    [[nodiscard]] ParticleTypeId getType() const override { return m_type; }
    [[nodiscard]] std::string getTypeName() const override;
    [[nodiscard]] std::string getParameters() const override;
    [[nodiscard]] std::unique_ptr<ParticleData> clone() const override;

    // ========================================================================
    // 方块特有方法
    // ========================================================================

    /**
     * @brief 获取方块状态
     *
     * @return 方块状态
     */
    [[nodiscard]] const BlockState& getBlockState() const { return m_blockState; }

private:
    ParticleTypeId m_type;
    BlockState m_blockState;
};

} // namespace mc::client::renderer::trident::particle::data
