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

#include "../../core/LivingRenderer.hpp"
#include "../../layer/entity/VillagerLayer.hpp"
#include "../../model/animal/VillagerModel.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>

namespace mc::client::renderer::entity {
class EntityRendererManager;
}

namespace mc::client::renderer::entity::renderer::animal {

/**
 * @brief 村民渲染器
 *
 * 参考 MC 1.16.5 VillagerRenderer
 * 支持不同职业和生物群系类型的村民纹理。
 *
 * 村民外观由多层纹理叠加实现：
 * 1. 基础纹理 - 村民身体
 * 2. 类型层 - 根据生物群系（沙漠、丛林、平原等）
 * 3. 职业层 - 根据职业（农民、图书管理员等）
 * 4. 等级徽章层 - 根据交易等级（石头、铁、金、绿宝石、钻石）
 */
class VillagerRenderer : public core::LivingRenderer<::mc::entity::VillagerEntity, model::animal::VillagerModel> {
public:
    VillagerRenderer();
    ~VillagerRenderer() override = default;

    /**
     * @brief 获取村民纹理
     *
     * 返回基础村民纹理。类型层、职业层和等级徽章层由 VillagerLayer 渲染。
     */
    [[nodiscard]] ResourceLocation getEntityTexture(::mc::entity::VillagerEntity& entity) override;
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::entity::VillagerEntity& entity) const override;

private:
    /**
     * @brief 初始化层渲染器
     */
    void initLayers();
};

/**
 * @brief 注册村民渲染器
 */
void registerVillagerRenderer(EntityRendererManager& manager);

} // namespace mc::client::renderer::entity::renderer::animal
