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

#include "AmbientEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/MobEntity.hpp"

namespace mc {

// 继承链标识（复刻 vanilla ClassTreeIdRegistry，parent = MobEntity::classInfo()）。
// 本类无同步字段，classInfo 仅作父链遍历节点：子类 ClassRegisterGuard 沿父链查找最高 id
// 时穿过本类（lastAssignedId=-1）直达父链已分配 id 的基类，子类首字段续接其后。
const entity::EntityClassInfo& AmbientEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"AmbientEntity", &MobEntity::classInfo()};
    return s_classInfo;
}

AmbientEntity::AmbientEntity(EntityInstanceId id)
    : MobEntity(id)
{
    // 注册属性
    registerAttributes();
}

void AmbientEntity::registerAttributes()
{
    // 调用父类方法
    MobEntity::registerAttributes();

    // 环境生物的基础属性
    // 通常不需要设置额外属性
}

} // namespace mc
