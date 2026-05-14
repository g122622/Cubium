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

#include "NetherRenderers.hpp"
#include "../../core/EntityRendererManager.hpp"

namespace mc::client::renderer::entity::renderer::nether {

void registerNetherRenderers(EntityRendererManager& manager)
{
    manager.registerRenderer(
        "minecraft:ghast", []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<GhastRenderer>(); });

    manager.registerRenderer("minecraft:magma_cube",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<MagmaCubeRenderer>(); });

    manager.registerRenderer("minecraft:piglin",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<PiglinRenderer>(); });

    manager.registerRenderer("minecraft:piglin_brute",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<PiglinBruteRenderer>(); });

    manager.registerRenderer("minecraft:hoglin",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<HoglinRenderer>(); });

    manager.registerRenderer("minecraft:zoglin",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<ZoglinRenderer>(); });

    manager.registerRenderer("minecraft:strider",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<StriderRenderer>(); });
}

} // namespace mc::client::renderer::entity::renderer::nether
