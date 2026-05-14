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

#include "../../resource/ResourceLocation.hpp"
#include "ItemTag.hpp"
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
    static std::unordered_map<ResourceLocation, std::unique_ptr<ItemTag>>& tags();
};

} // namespace item::tag
} // namespace mc