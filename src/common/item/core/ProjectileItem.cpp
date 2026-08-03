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
 * IMPLIED, INCLUDING WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR USE OF OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "ProjectileItem.hpp"

#include "common/core/Types.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"

namespace mc {
namespace item {

void ProjectileItem::shoot(entity::ProjectileEntity& projectile,
    f32 directionX,
    f32 directionY,
    f32 directionZ,
    f32 power,
    f32 uncertainty) const
{
    // 默认实现：委托给弹射物的 shoot 方法
    projectile.shoot(directionX, directionY, directionZ, power, uncertainty);
}

} // namespace item
} // namespace mc
