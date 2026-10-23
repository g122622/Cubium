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

#include "common/core/Result.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc::entity {
class EntityType;
}

namespace mc::mod::bedrock::addon {
class AddonEntityDefinition;
class BehaviorPackList;

/** @brief 行为包内容加载门面，管理定义验证、类型注册及批次撤销。 */
class AddonContentManager {
public:
    AddonContentManager() = default;
    ~AddonContentManager();
    AddonContentManager(const AddonContentManager&) = delete;
    AddonContentManager& operator=(const AddonContentManager&) = delete;

    /** @brief 按包优先级装载实体定义，成功返回本批新增的类型名。 */
    [[nodiscard]] Result<std::vector<std::string>> loadEntityDefinitions(BehaviorPackList& packs);
    /** @brief 仅撤销指定批次的类型查找，不影响已经存在的类型。 */
    void rollback(const std::vector<std::string>& identifiers);
    /** @brief 撤销本门面注册的全部类型，存活实体继续持有其定义和类型。 */
    void clear();

private:
    struct EntityRegistration {
        std::string packUuid;
        std::shared_ptr<const AddonEntityDefinition> definition;
        const mc::entity::EntityType* type;
    };

    std::unordered_map<std::string, EntityRegistration> m_entities;
};

} // namespace mc::mod::bedrock::addon
