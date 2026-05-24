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

#include "SpecialMonsterRenderers.hpp"
#include "../../core/EntityRendererManager.hpp"

namespace mc::client::renderer::entity::renderer::monster {

void registerSpecialMonsterRenderers(EntityRendererManager& manager)
{
    manager.registerRenderer("minecraft:wither",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<WitherRenderer>(); });

    manager.registerRenderer(
        "minecraft:slime", []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<SlimeRenderer>(); });

    manager.registerRenderer("minecraft:guardian",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<GuardianRenderer>(); });

    manager.registerRenderer("minecraft:elder_guardian",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<ElderGuardianRenderer>(); });

    manager.registerRenderer("minecraft:shulker",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<ShulkerRenderer>(); });

    manager.registerRenderer("minecraft:silverfish",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<SilverfishRenderer>(); });

    manager.registerRenderer("minecraft:endermite",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<EndermiteRenderer>(); });
}

void registerIllagerRenderers(EntityRendererManager& manager)
{
    manager.registerRenderer(
        "minecraft:vex", []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<VexRenderer>(); });

    manager.registerRenderer("minecraft:vindicator",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<VindicatorRenderer>(); });

    manager.registerRenderer("minecraft:evoker",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<EvokerRenderer>(); });

    manager.registerRenderer("minecraft:pillager",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<PillagerRenderer>(); });

    manager.registerRenderer("minecraft:ravager",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<RavagerRenderer>(); });

    manager.registerRenderer(
        "minecraft:witch", []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<WitchRenderer>(); });
}

} // namespace mc::client::renderer::entity::renderer::monster
