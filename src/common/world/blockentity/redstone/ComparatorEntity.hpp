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
#include "common/world/block/BlockPos.hpp"
#include "world/blockentity/BlockEntity.hpp"
#include <memory>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace blockentity {

/**
 * @brief 红石比较器方块实体
 *
 * 比较器需要存储其输出信号强度，因为输出信号在更新时需要保持稳定。
 * 这对于前端信号保持特性非常重要。
 *
 * ## 功能
 * - 存储输出信号强度
 * - NBT序列化/反序列化
 *
 * ## 参考
 * - MC 1.16.5: net.minecraft.tileentity.ComparatorTileEntity
 *
 * ## 容易踩的坑
 * - 输出信号需要持久化，否则重新加载世界后信号会丢失
 * - 输出信号由比较器在 updateState 时设置，getActiveSignal 时读取
 */
class ComparatorEntity : public BlockEntity {
public:
    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit ComparatorEntity(const BlockPos& pos);

    // ========== BlockEntity 接口实现 ==========

    /**
     * @brief 从JSON加载数据
     * @param data JSON数据
     * @return 是否成功
     */
    bool load(const nlohmann::json& data) override;

    /**
     * @brief 保存数据到JSON
     * @param data 输出JSON数据
     */
    void save(nlohmann::json& data) const override;

    /**
     * @brief 创建方块实体的副本
     * @return 副本的unique_ptr
     */
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

    // ========== 比较器特有方法 ==========

    /**
     * @brief 获取输出信号强度
     * @return 输出信号强度 0-15
     */
    [[nodiscard]] i32 getOutputSignal() const { return m_outputSignal; }

    /**
     * @brief 设置输出信号强度
     * @param signal 输出信号强度 0-15
     */
    void setOutputSignal(i32 signal);

private:
    /// 输出信号强度 (0-15)
    i32 m_outputSignal = 0;
};

} // namespace blockentity
} // namespace mc
