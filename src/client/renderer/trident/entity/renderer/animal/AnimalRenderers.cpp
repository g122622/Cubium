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

#include "AnimalRenderers.hpp"
#include "../../core/EntityRendererManager.hpp"
#include "BatModel.hpp"
#include "RabbitModel.hpp"
#include "SquidModel.hpp"

namespace mc::client::renderer::entity::renderer::animal {

void registerAnimalRenderers(EntityRendererManager& manager)
{
    // 猪
    manager.registerRenderer(
        "minecraft:pig", []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<PigRenderer>(); });

    // 牛
    manager.registerRenderer(
        "minecraft:cow", []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<CowRenderer>(); });

    // 羊
    manager.registerRenderer(
        "minecraft:sheep", []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<SheepRenderer>(); });

    // 哞菇
    manager.registerRenderer("minecraft:mooshroom",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<MooshroomRenderer>(); });

    // 鸡
    manager.registerRenderer("minecraft:chicken",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<ChickenRenderer>(); });

    // 兔子
    manager.registerRenderer("minecraft:rabbit",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<RabbitRenderer>(); });

    // 蝙蝠
    manager.registerRenderer(
        "minecraft:bat", []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<BatRenderer>(); });

    // 鱿鱼
    manager.registerRenderer(
        "minecraft:squid", []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<SquidRenderer>(); });

    // 已有的动物（狼、猫、豹猫、马、村民）
    // 这些在单独的文件中注册
}

} // namespace mc::client::renderer::entity::renderer::animal
