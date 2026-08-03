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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY KIND, INCLUDING
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include "LootFunction.hpp"
#include "common/command/arguments/NbtPath.hpp"
#include "common/core/Types.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace loot {

/**
 * @brief 复制NBT函数
 *
 * 从掉落源（实体或方块实体）复制NBT数据到物品的自定义数据标签。
 * 支持三种合并策略：替换、追加、合并。
 *
 * NBT路径语法与MC命令 /data 中的路径语法一致，
 * 例如 "CustomName"、"Inventory[0].id"、"Attributes[{Name:\"generic.max_health\"}].Base"。
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
        Replace, // 替换目标路径的值
        Append,  // 追加到目标路径的列表中
        Merge    // 合并到目标路径的复合标签中
    };

    /**
     * @brief NBT操作定义
     */
    struct NbtOperation {
        std::string sourcePath;                    ///< NBT源路径（原始字符串）
        std::string targetPath;                    ///< NBT目标路径（原始字符串）
        Operation operation;                       ///< 操作类型
        mutable command::NbtPath parsedSourcePath; ///< 解析后的源路径（延迟解析）
        mutable command::NbtPath parsedTargetPath; ///< 解析后的目标路径（延迟解析）
        mutable bool pathsParsed = false;          ///< 路径是否已解析

        NbtOperation(std::string srcPath, std::string tgtPath, Operation op);
    };

    /**
     * @brief 构造复制NBT函数
     * @param source NBT来源
     */
    explicit CopyNbtFunction(Source source) noexcept;

    /**
     * @brief 添加NBT操作
     * @param sourcePath NBT源路径
     * @param targetPath NBT目标路径
     * @param operation 操作类型
     */
    void addOperation(const std::string& sourcePath, const std::string& targetPath, Operation operation);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const noexcept override;
    [[nodiscard]] std::string getType() const noexcept override { return "copy_nbt"; }

    [[nodiscard]] Source getSource() const noexcept { return m_source; }
    [[nodiscard]] const std::vector<NbtOperation>& getOperations() const noexcept { return m_operations; }

private:
    /**
     * @brief 从LootContext解析NBT来源对象，提取其NBT数据
     * @return 源NBT复合标签，如果来源不可用则返回nullptr
     */
    [[nodiscard]] std::unique_ptr<nbt::tags::compound_tag> _resolveSourceNbt(LootContext& context) const;

    /**
     * @brief 确保操作的NBT路径已解析
     */
    void _ensurePathsParsed(const NbtOperation& op) const;

    /**
     * @brief 对NBT操作应用合并策略
     */
    void _applyOperation(
        const NbtOperation& op, nbt::tags::compound_tag& sourceTag, nbt::tags::compound_tag& targetTag) const;

    /**
     * @brief 将 nlohmann::json 对象转换为 NBT compound_tag
     */
    [[nodiscard]] static std::unique_ptr<nbt::tags::compound_tag> _jsonToNbtCompound(const nlohmann::json& json);

    /**
     * @brief 将 NBT compound_tag 转换为 nlohmann::json 对象
     */
    [[nodiscard]] static nlohmann::json _nbtCompoundToJson(const nbt::tags::compound_tag& tag);

    Source m_source;
    std::vector<NbtOperation> m_operations;
};

} // namespace loot
} // namespace mc
