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

#include "AbstractGroupFishEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <memory>
#include <optional>

namespace mc {

namespace resource {
class ResourceLocation;
}

/**
 * @brief 鲑鱼实体
 *
 * 鲑鱼属于群游鱼类，最大群体大小固定为 5。
 */
class SalmonEntity : public AbstractGroupFishEntity {
public:
    /**
     * @brief 构造鲑鱼实体
     * @param id 实体 ID
     */
    SalmonEntity(EntityInstanceId id);
    ~SalmonEntity() override = default;

    SalmonEntity(const SalmonEntity&) = delete;
    SalmonEntity& operator=(const SalmonEntity&) = delete;
    SalmonEntity(SalmonEntity&&) noexcept = delete;
    SalmonEntity& operator=(SalmonEntity&&) noexcept = delete;

    /// 本类继承链标识（parent = AbstractGroupFishEntity::classInfo()）。见 Entity::classInfo()。
    // vanilla 1.21.11 Salmon 自带 DATA_TYPE@17(Int，体型 small/medium/large=0/1/2)，
    // 见 registerData。项目体型业务联动暂未实现，占位 id17 对齐 vanilla 字段表上限。
    static const entity::EntityClassInfo& classInfo();

    /**
     * @brief 创建鲑鱼实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief vanilla 鲑鱼最大群体大小为 5
     */
    [[nodiscard]] i32 getMaxGroupSize() const override { return 5; }

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 0.1f; }

    /**
     * @brief 获取扑腾声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getFlopSound() const override;

    /**
     * @brief 获取环境声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    /**
     * @brief 获取死亡声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    /**
     * @brief 获取受伤声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

protected:
    void registerAttributes() override;

    // ========== 同步数据注册 ==========
    // 派生类构造函数须显式调用 registerData()（C++ 基类构造期虚函数不派发，参考 AbstractFishEntity）。
    void registerData() override;

private:
    // ========== 同步数据参数（vanilla 1.21.11 Salmon.DATA_TYPE，见 registerData） ==========
    // id17 DATA_TYPE（体型 small/medium/large=0/1/2）。TODO: 体型业务联动暂未实现，占位默认 0。
    static entity::DataParameter<i32> DATA_TYPE_PARAM;
};

} // namespace mc
