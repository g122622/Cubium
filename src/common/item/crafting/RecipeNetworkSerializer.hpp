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

#include "core/Result.hpp"
#include "item/crafting/IRecipe.hpp"
#include "item/crafting/ShapedRecipe.hpp"
#include "item/crafting/ShapelessRecipe.hpp"
#include "item/crafting/SmeltingRecipe.hpp"
#include "item/crafting/SmithingRecipe.hpp"
#include "item/crafting/StonecuttingRecipe.hpp"
#include "network/packet/PacketSerializer.hpp"
#include <memory>

namespace mc {
namespace crafting {

/**
 * @brief 配方网络序列化器
 *
 * 提供配方与网络数据包之间的序列化/反序列化功能。
 * 参考 MC 1.16.5 的配方网络协议。
 *
 * 序列化格式：
 * - 配方类型（VarInt）
 * - 配方ID（std::string）
 * - 配方组（std::string）
 * - 配方特定数据
 *
 * 使用示例：
 * @code
 * // 序列化
 * PacketSerializer ser;
 * RecipeNetworkSerializer::serialize(recipe, ser);
 *
 * // 反序列化
 * PacketDeserializer deser(data, size);
 * auto result = RecipeNetworkSerializer::deserialize(deser);
 * if (result.success()) {
 *     std::unique_ptr<CraftingRecipe> recipe = std::move(result.value());
 * }
 * @endcode
 */
class RecipeNetworkSerializer {
public:
    // ========== 合成配方序列化 ==========

    /**
     * @brief 序列化合成配方到网络包
     * @param recipe 配方
     * @param ser 序列化器
     */
    static void serialize(const CraftingRecipe& recipe, network::PacketSerializer& ser);

    /**
     * @brief 从网络包反序列化合成配方
     * @param deser 反序列化器
     * @return 配方，或错误
     */
    [[nodiscard]] static Result<std::unique_ptr<CraftingRecipe>> deserialize(network::PacketDeserializer& deser);

    // ========== 熔炼配方序列化 ==========

    /**
     * @brief 序列化熔炼配方到网络包
     * @param recipe 熔炼配方
     * @param ser 序列化器
     */
    static void serializeSmelting(const SmeltingRecipe& recipe, network::PacketSerializer& ser);

    /**
     * @brief 从网络包反序列化熔炼配方
     * @param deser 反序列化器
     * @param type 配方类型（Smelting, Blasting, Smoking, CampfireCooking）
     * @return 熔炼配方，或错误
     */
    [[nodiscard]] static Result<std::unique_ptr<SmeltingRecipe>> deserializeSmelting(
        network::PacketDeserializer& deser, RecipeType type);

    // ========== 切石机配方序列化 ==========

    /**
     * @brief 序列化切石机配方到网络包
     * @param recipe 切石机配方
     * @param ser 序列化器
     */
    static void serializeStonecutting(const StonecuttingRecipe& recipe, network::PacketSerializer& ser);

    /**
     * @brief 从网络包反序列化切石机配方
     * @param deser 反序列化器
     * @param id 配方ID
     * @return 切石机配方，或错误
     */
    [[nodiscard]] static Result<std::unique_ptr<StonecuttingRecipe>> deserializeStonecutting(
        network::PacketDeserializer& deser, const ResourceLocation& id);

    // ========== 锻造台配方序列化 ==========

    /**
     * @brief 序列化锻造台配方到网络包
     * @param recipe 锻造台配方
     * @param ser 序列化器
     */
    static void serializeSmithing(const SmithingRecipe& recipe, network::PacketSerializer& ser);

    /**
     * @brief 从网络包反序列化锻造台配方
     * @param deser 反序列化器
     * @param id 配方ID
     * @return 锻造台配方，或错误
     */
    [[nodiscard]] static Result<std::unique_ptr<SmithingRecipe>> deserializeSmithing(
        network::PacketDeserializer& deser, const ResourceLocation& id);

    // ========== 工具方法 ==========

    /**
     * @brief 写入配方类型
     * @param type 配方类型
     * @param ser 序列化器
     */
    static void writeRecipeType(RecipeType type, network::PacketSerializer& ser);

    /**
     * @brief 读取配方类型
     * @param deser 反序列化器
     * @return 配方类型，或错误
     */
    [[nodiscard]] static Result<RecipeType> readRecipeType(network::PacketDeserializer& deser);

private:
    // 有序合成配方序列化
    static void serializeShaped(const ShapedRecipe& recipe, network::PacketSerializer& ser);
    [[nodiscard]] static Result<std::unique_ptr<ShapedRecipe>> deserializeShaped(
        network::PacketDeserializer& deser, const ResourceLocation& id, const std::string& group);

    // 无序合成配方序列化
    static void serializeShapeless(const ShapelessRecipe& recipe, network::PacketSerializer& ser);
    [[nodiscard]] static Result<std::unique_ptr<ShapelessRecipe>> deserializeShapeless(
        network::PacketDeserializer& deser, const ResourceLocation& id, const std::string& group);
};

} // namespace crafting
} // namespace mc
