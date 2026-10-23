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

#include "common/mod/bedrock/addon/content/AddonContentManager.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/mod/bedrock/addon/content/entity/definition/AddonEntityDefinition.hpp"
#include "common/mod/bedrock/addon/pack/BehaviorPack.hpp"
#include "common/mod/bedrock/addon/pack/BehaviorPackList.hpp"

#include <algorithm>
#include <map>
#include <nlohmann/json.hpp>

namespace mc::mod::bedrock::addon {

AddonContentManager::~AddonContentManager()
{
    clear();
}

Result<std::vector<std::string>> AddonContentManager::loadEntityDefinitions(BehaviorPackList& packs)
{
    struct Candidate {
        std::string packUuid;
        std::shared_ptr<const AddonEntityDefinition> definition;
    };

    std::map<std::string, Candidate> candidates;
    for (const auto* pack : packs.getEnabledPacks()) {
        const bool behavior = std::any_of(
            pack->manifest().modules.begin(), pack->manifest().modules.end(), [](const AddonModule& module) {
                return module.type == AddonModuleType::Data || module.type == AddonModuleType::Script;
            });
        if (!behavior) {
            continue;
        }

        auto files = pack->listResources("entities", ".json");
        if (files.failed()) {
            return files.error();
        }
        for (const auto& file : files.value()) {
            auto source = pack->readResource(file);
            if (source.failed()) {
                return source.error();
            }
            // 行为包 JSON 允许注释；解析失败携带包名和路径，不能继续注册部分定义。
            const auto document =
                nlohmann::json::parse(source.value().begin(), source.value().end(), nullptr, false, true);
            if (document.is_discarded()) {
                return Error(ErrorCode::InvalidData, "Invalid entity JSON in pack " + pack->uuid() + ": " + file);
            }
            auto parsed = AddonEntityDefinition::parse(document);
            if (parsed.failed()) {
                return Error(parsed.error().code(),
                    "Entity definition in " + pack->uuid() + "/" + file + ": " + parsed.error().message());
            }
            auto definition = std::make_shared<const AddonEntityDefinition>(std::move(parsed.value()));
            const auto& id = definition->identifier();
            if (const auto existing = candidates.find(id); existing != candidates.end()) {
                if (existing->second.packUuid == pack->uuid()) {
                    return Error(ErrorCode::AlreadyExists, "Duplicate entity definition within pack: " + id);
                }
                // 包列表按高优先级在前排序，同名定义由栈中更高的包决定。
                continue;
            }
            candidates.emplace(id, Candidate{pack->uuid(), std::move(definition)});
        }
    }

    auto& registry = mc::entity::EntityRegistry::instance();
    for (const auto& [id, candidate] : candidates) {
        if (const auto owned = m_entities.find(id); owned != m_entities.end()) {
            if (registry.getType(id) != owned->second.type || owned->second.packUuid != candidate.packUuid ||
                owned->second.definition->document() != candidate.definition->document()) {
                return Error(ErrorCode::InvalidState, "Entity definition changed without unloading its content: " + id);
            }
        } else if (registry.hasType(id)) {
            // TODO: 用定义组合器支持行为包覆盖原版实体；不得直接覆盖其稳定类型对象。
            return Error(ErrorCode::AlreadyExists, "Entity type is owned by another registration: " + id);
        }
    }

    std::vector<std::string> added;
    for (const auto& [id, candidate] : candidates) {
        if (m_entities.contains(id)) {
            continue;
        }
        const auto definition = candidate.definition;
        auto builder = mc::entity::EntityType::Builder(
            [definition](mc::IWorld* world, mc::ecs::EntityRegistry& registry) {
                return AddonEntityDefinition::create(definition, world, registry);
            },
            mc::entity::EntityClassification::Misc);
        builder.size(definition->width(), definition->height()).canSummon(definition->canSummon()).updateInterval(1);
        if (definition->fireImmune()) {
            builder.immuneToFire();
        }
        auto registered = registry.registerType(id, builder.build());
        if (registered.failed()) {
            rollback(added);
            return registered.error();
        }
        m_entities.emplace(id, EntityRegistration{candidate.packUuid, definition, registry.getType(id)});
        added.push_back(id);
    }
    return added;
}

void AddonContentManager::rollback(const std::vector<std::string>& identifiers)
{
    auto& registry = mc::entity::EntityRegistry::instance();
    for (const auto& id : identifiers) {
        const auto found = m_entities.find(id);
        if (found != m_entities.end()) {
            registry.unregisterType(id, found->second.type);
            m_entities.erase(found);
        }
    }
}

void AddonContentManager::clear()
{
    auto& registry = mc::entity::EntityRegistry::instance();
    for (const auto& [id, registration] : m_entities) {
        registry.unregisterType(id, registration.type);
    }
    m_entities.clear();
}

} // namespace mc::mod::bedrock::addon
