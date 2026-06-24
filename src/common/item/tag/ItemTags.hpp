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

#include "ItemTag.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace mc {
namespace item::tag {

/**
 * @brief 物品标签注册表
 *
 * 负责集中管理 ItemTag 的创建、查询和遍历。
 */
class ItemTags {
public:
    // ========== 内置物品标签 ==========

    /**
     * @brief 花朵标签
     *
     * 包含所有可用于蜜蜂繁殖和授粉的花朵物品。
     */
    static ItemTag& FLOWERS();

    /**
     * @brief 地毯标签
     *
     * 包含所有颜色的地毯物品。
     * 用于羊驼装饰槽位判断。
     */
    static ItemTag& CARPETS();

    /**
     * @brief 减振物品标签
     *
     * 包含所有羊毛物品和地毯物品。
     * 掉落的羊毛物品不会触发振动信号。
     */
    static ItemTag& DAMPENS_VIBRATIONS();

    /**
     * @brief 防火物品标签
     *
     * 包含所有防火物品（下界合金锭、下界合金碎片、远古残骸、下界星等）。
     * 掉落的防火物品实体免疫火焰和岩浆伤害。
     */
    static ItemTag& FIRE_RESISTANT();

    /**
     * @brief 初始化所有内置物品标签
     *
     * 必须在 ItemRegistry 初始化之后调用。
     * 注册所有花朵物品到 FLOWERS 标签。
     */
    static void initialize();

    /**
     * @brief 注册或获取指定ID的标签。
     * @param id 标签ID。
     * @return 标签引用。
     */
    static ItemTag& registerTag(const ResourceLocation& id);

    /**
     * @brief 根据ID获取标签。
     * @param id 标签ID。
     * @return 标签指针，不存在返回 nullptr。
     */
    [[nodiscard]] static ItemTag* getTag(const ResourceLocation& id);

    /**
     * @brief 根据ID字符串获取标签。
     * @param id 标签ID字符串（namespace:path）。
     * @return 标签指针，不存在返回 nullptr。
     */
    [[nodiscard]] static ItemTag* getTag(const std::string& id);

    /**
     * @brief 遍历全部标签。
     * @param callback 回调函数。
     */
    static void forEachTag(std::function<void(ItemTag&)> callback);

private:
    ItemTags() = delete;

    static std::unordered_map<ResourceLocation, std::unique_ptr<ItemTag>>& tags();
    static bool s_initialized;
};

} // namespace item::tag
} // namespace mc