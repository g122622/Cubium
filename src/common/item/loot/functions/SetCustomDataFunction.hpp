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

#include "LootFunction.hpp"
#include "common/core/Result.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include <memory>
#include <optional>
#include <string>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace loot {

/**
 * @brief 设置NBT标签函数
 *
 * 设置物品的自定义NBT数据。
 * 参考: net.minecraft.loot.functions.SetNBT
 *
 * 用于设置物品的自定义数据，如药水效果、显示名称等。
 */
class SetCustomDataFunction : public LootFunction {
public:
    /**
     * @brief 校验并构造设置自定义数据函数
     *
     * 在加载战利品表时即按 Mojangson 解析 tag 字段，非法 NBT 直接返回错误，使整张战利品表加载失败
     *（对应 vanilla set_custom_data 用 TagParser.LENIENT_CODEC 在 codec 阶段解析）。
     *
     * @param nbtString 自定义数据字符串（Mojangson 复合标签文本）
     * @return 函数实例；解析失败返回 InvalidData 错误
     */
    [[nodiscard]] static Result<std::unique_ptr<SetCustomDataFunction>> create(const std::string& nbtString);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const noexcept override;
    [[nodiscard]] std::string getType() const noexcept override { return "set_custom_data"; }

    [[nodiscard]] const std::string& getNbtString() const noexcept { return m_nbtString; }

    /**
     * @brief 直接构造（不做校验），仅供 create() 使用
     *
     * @param nbtString 自定义数据原始文本
     * @param parsedTag 加载期解析结果
     */
    SetCustomDataFunction(const std::string& nbtString, std::optional<nlohmann::json> parsedTag);

private:
    std::string m_nbtString;
    /// 加载期解析得到的自定义数据，apply() 直接合并，不再重复解析
    std::optional<nlohmann::json> m_parsedTag;
};

} // namespace loot
} // namespace mc
