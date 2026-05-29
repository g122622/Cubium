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

#pragma once

#include <memory>
#include "LootFunction.hpp"
#include "common/core/Types.hpp"
#include <string>
#include <vector>

namespace mc {
namespace loot {

/**
 * @brief 复制NBT函数
 *
 * 从掉落源复制NBT数据到物品。
 * 参考: net.minecraft.loot.functions.CopyNbt
 *
 * 用于复制实体或方块的NBT数据到掉落物品。
 */
class CopyNbtFunction : public LootFunction {
public:
    /**
     * @brief NBT来源
     */
    enum class Source : u8 {
        This,         // 当前实体
        Killer,       // 击杀者
        KillerPlayer, // 击杀玩家
        BlockEntity   // 方块实体
    };

    /**
     * @brief NBT操作类型
     */
    enum class Operation : u8 {
        Replace, // 替换
        Append,  // 追加
        Merge    // 合并
    };

    /**
     * @brief NBT操作定义
     */
    struct NbtOperation {
        std::string sourcePath;
        std::string targetPath;
        Operation operation;
    };

    /**
     * @brief 构造复制NBT函数
     * @param source NBT来源
     */
    explicit CopyNbtFunction(Source source);

    /**
     * @brief 添加NBT操作
     */
    void addOperation(const std::string& sourcePath, const std::string& targetPath, Operation operation);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const override;
    [[nodiscard]] std::string getType() const override { return "copy_nbt"; }

    [[nodiscard]] Source getSource() const { return m_source; }
    [[nodiscard]] const std::vector<NbtOperation>& getOperations() const { return m_operations; }

private:
    Source m_source;
    std::vector<NbtOperation> m_operations;
};

} // namespace loot
} // namespace mc
