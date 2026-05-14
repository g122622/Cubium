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

#include "MonsterVariantRenderers.hpp"
#include "../../core/EntityRendererManager.hpp"

namespace mc::client::renderer::entity::renderer::monster {

void registerMonsterVariantRenderers(EntityRendererManager& manager)
{
    manager.registerRenderer("minecraft:zombie_villager",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<ZombieVillagerRenderer>(); });

    manager.registerRenderer("minecraft:drowned",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<DrownedRenderer>(); });

    manager.registerRenderer(
        "minecraft:husk", []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<HuskRenderer>(); });

    manager.registerRenderer(
        "minecraft:stray", []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<StrayRenderer>(); });

    manager.registerRenderer("minecraft:cave_spider",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<CaveSpiderRenderer>(); });

    manager.registerRenderer(
        "minecraft:giant", []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<GiantRenderer>(); });
}

} // namespace mc::client::renderer::entity::renderer::monster
