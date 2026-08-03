/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, the subject to the following conditions:
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

#include "ModelFactory.hpp"
#include "client/renderer/trident/entity/model/core/EntityModel.hpp"
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <spdlog/spdlog.h>

namespace mc::client::renderer::entity::model {

bool ModelFactory::s_initialized = false;

ModelFactory& ModelFactory::instance()
{
    static ModelFactory factory;
    return factory;
}

std::string ModelFactory::_normalizeEntityTypeId(const std::string& entityTypeId)
{
    std::string normalizedId = entityTypeId;
    if (normalizedId.find(':') == std::string::npos) {
        normalizedId = "minecraft:" + normalizedId;
    }
    return normalizedId;
}

void ModelFactory::registerModel(const std::string& entityTypeId, ModelCreator creator)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::string normalizedId = _normalizeEntityTypeId(entityTypeId);

    if (m_creators.find(normalizedId) != m_creators.end()) {
        spdlog::warn("ModelFactory: Model already registered for '{}', overwriting", normalizedId);
    }

    m_creators[normalizedId] = std::move(creator);
}

std::unique_ptr<EntityModel> ModelFactory::createModel(const std::string& entityTypeId) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::string normalizedId = _normalizeEntityTypeId(entityTypeId);

    auto it = m_creators.find(normalizedId);
    if (it == m_creators.end()) {
        return nullptr;
    }

    return it->second();
}

bool ModelFactory::hasModel(const std::string& entityTypeId) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::string normalizedId = _normalizeEntityTypeId(entityTypeId);

    return m_creators.find(normalizedId) != m_creators.end();
}

size_t ModelFactory::size() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_creators.size();
}

} // namespace mc::client::renderer::entity::model
