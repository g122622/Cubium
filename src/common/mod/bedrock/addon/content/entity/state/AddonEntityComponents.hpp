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
#include "common/core/Types.hpp"
#include "common/mod/bedrock/addon/content/entity/events/EntityEventProgram.hpp"
#include "common/mod/bedrock/addon/content/entity/events/EntityEventTrigger.hpp"
#include "common/mod/bedrock/addon/content/entity/state/sensor/EntityEnvironmentSensor.hpp"
#include "common/mod/bedrock/addon/content/entity/state/timer/EntityTimerState.hpp"
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace mc {
class Entity;
class DamageSource;

namespace entity::serialization::components {
class ComponentSerializerRegistry;
}
} // namespace mc

namespace mc::mod::bedrock::addon {
class AddonEntityDefinition;

/** @brief 数据驱动实体当前活动组件、组件来源、活动组及下次 tick 的变更。 */
class AddonEntityComponents {
public:
    /** @brief 使用基础定义建立初始组件来源，尚不访问构造中的实体。 */
    explicit AddonEntityComponents(std::shared_ptr<const AddonEntityDefinition> definition);
    /** @brief 在组件挂载后安装初始物理与属性状态。 */
    void initialize(mc::Entity& entity);
    /** @brief 排队已验证的组件组操作，下一实体 tick 应用。 */
    void queue(const std::vector<EntityComponentGroupChange>& changes);
    /** @brief 按原事件顺序执行组件组替换、移除与重新初始化。 */
    void applyPending(mc::Entity& entity);
    /** @brief 在实体自身更新之后推进计时器，到期事件的修改留到后续 tick。 */
    void tickTimers(mc::Entity& entity);
    /** @brief 实体更新后按当前状态检查活动环境传感器。 */
    void tickEnvironment(mc::Entity& entity);
    /** @brief 已确认死亡时执行一次活动死亡触发器，返回是否首次处理。 */
    bool onDeath(mc::Entity& entity, mc::DamageSource& source);
    /** @brief 活动即时移除组件要求移除时使用无掉落的 discard 路径。 */
    bool despawnIfRequested(mc::Entity& entity) const;
    /** @brief 查询当前是否存在指定组件。 */
    [[nodiscard]] bool has(const std::string& name) const;
    /** @brief 返回当前有效组件配置，缺失时返回 nullptr。 */
    [[nodiscard]] const nlohmann::json* component(const std::string& name) const;
    /** @brief 返回当前碰撞箱宽度和高度。 */
    [[nodiscard]] f32 width() const;
    [[nodiscard]] f32 height() const;
    /** @brief 保存实际组件来源、活动组和未应用操作，不通过组重合并恢复状态。 */
    [[nodiscard]] nlohmann::json save() const;
    /** @brief 校验整个快照后恢复组件，在其他实体状态恢复前安装基础属性。 */
    Result<void> load(mc::Entity& entity, const nlohmann::json& saved);

private:
    /** @brief 将一个组件当前配置或缺失状态写入实体。 */
    void _apply(mc::Entity& entity, const std::string& name);

    std::shared_ptr<const AddonEntityDefinition> m_definition;
    std::map<std::string, std::string> m_sources;
    std::set<std::string> m_activeGroups;
    std::vector<EntityComponentGroupChange> m_pending;
    std::optional<EntityTimerState> m_timer;
    std::optional<EntityEventTrigger> m_onDeath;
    std::optional<EntityEnvironmentSensor> m_environmentSensor;
    bool m_deathHandled;
};

/** @brief 将活动组件状态接入实体序列化，先于生命值等组件恢复。 */
void registerAddonEntityComponentsSerializer(
    mc::entity::serialization::components::ComponentSerializerRegistry& registry);

} // namespace mc::mod::bedrock::addon
