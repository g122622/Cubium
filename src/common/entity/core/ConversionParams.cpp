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

#include "ConversionParams.hpp"

namespace mc {

ConversionParams ConversionParams::single(
    const Entity& /*from*/, bool preserveEquipment, bool preserveCanPickUpLoot, bool copyNbt)
{
    ConversionParams params;
    params.preserveEquipment = preserveEquipment;
    params.preserveCanPickUpLoot = preserveCanPickUpLoot;
    if (copyNbt) {
        // TODO(convert-copy-nbt): 自定义 NBT（custom data）体系落地后在此填入原实体的
        //   自定义数据快照；当前实体尚无独立的 custom data 容器，copyNbt=true 暂为空。
        params.copyNbt = nbt::tags::compound_tag{};
    }
    return params;
}

} // namespace mc