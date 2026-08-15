/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permitted persons to whom the Software is
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

#include "common/core/Types.hpp"                                      // u64
#include "common/mod/bedrock/addon/binding/IScriptBindingContext.hpp" // IScriptBindingContext
#include "common/mod/bedrock/addon/binding/ScriptClassBinding.hpp"    // ScriptObjectRegistry
#include "common/mod/bedrock/addon/binding/ScriptClassRegistry.hpp"   // classIdByName/protoByName
#include "common/world/IWorld.hpp"                                    // mc::IWorld
#include "common/world/block/BlockPos.hpp"                            // mc::BlockPos
#include "common/world/block/BlockState.hpp"                          // mc::BlockState

namespace mc::mod::bedrock::addon {

/// Block JS 类 opaque 持有的快照（owned=true，JS GC 时 delete）。
/// 持 const BlockState*（BlockRegistry 全局拥有，非拥有指针）+ BlockPos + world 回指。
/// world 仅供 isAir 等衍生查询；非拥有，绑定层保证调用期 world 存活（绑定期世界长于 JS 对象）。
///
/// 从 MinecraftModuleFactory.cpp 匿名命名空间提升至公共头，使 server 侧
/// （ScriptTestHelper 的 getBlock/assertBlockState 绑定）能跨模块构造 Block JS 对象，
/// 打通 test.getBlock(pos).permutation.getState("age") 链路。
struct ScriptBlockRef {
    const mc::BlockState* state = nullptr;
    mc::BlockPos pos{};
    mc::IWorld* world = nullptr;
};

/**
 * @brief 将世界坐标处的方块快照包装为 @minecraft/server Block JS 对象（owned）。
 *
 * 新建 ScriptBlockRef 持有 state/pos/world 快照，经 ScriptObjectRegistry::wrap 构造 Block
 * JS 对象（owned=true + destroy 回调 delete ScriptBlockRef，对齐 ClassRegistrar::wrap 范式）。
 *
 * classId/proto 经 ScriptClassRegistry 运行时回查 classIdByName("Block")/protoByName("Block")，
 * 对齐 resolveItemStackClassId 既定模式：引擎重建后 classId 重新分配，闭包捕获的旧 classId 会与
 * unwrap 入参路径用 classIdByName 取的最新 classId 失配。详见
 * [[script-classid-cross-rebuild-mismatch]]。
 *
 * @param ctx 绑定上下文。
 * @param state 方块状态（非拥有，BlockRegistry 全局拥有）。
 * @param pos 方块世界坐标。
 * @param world 世界回指（非拥有，仅供衍生查询）。
 * @return Block JS 对象句柄；若 Block 类未注册（registry 查不到 classId/proto）返回 undefined。
 */
[[nodiscard]] inline void* wrapBlock(
    IScriptBindingContext& ctx, const mc::BlockState* state, const mc::BlockPos& pos, mc::IWorld* world)
{
    const u64 blockClassId = ScriptClassRegistry::instance().classIdByName("Block");
    void* blockProto = ScriptClassRegistry::instance().protoByName("Block");
    if (blockClassId == 0 || blockProto == nullptr) {
        // Block 类未注册（绑定期未完成或引擎重建中），防御性返回 undefined。
        return ctx.createUndefined();
    }
    auto* ref = new ScriptBlockRef{state, pos, world};
    return ScriptObjectRegistry::wrap(ctx,
        blockClassId,
        blockProto,
        ref,
        true, // owned：JS GC 时 delete ScriptBlockRef
        "Block",
        [](void* p) { delete static_cast<ScriptBlockRef*>(p); });
}

/**
 * @brief 从 Block JS 对象取回 ScriptBlockRef*（非拥有，调用期有效）。
 *
 * 与 wrapBlock 配对的 unwrap 路径。classId 运行时回查 classIdByName("Block") 保证与 wrap 用同一
 * 最新 classId（引擎重建后失配防护，详见 [[script-classid-cross-rebuild-mismatch]]）。
 *
 * @param ctx 绑定上下文。
 * @param val Block JS 对象句柄。
 * @return ScriptBlockRef*；非 Block 对象或失效返回 nullptr。
 */
[[nodiscard]] inline ScriptBlockRef* unwrapBlock(IScriptBindingContext& ctx, void* val)
{
    const u64 blockClassId = ScriptClassRegistry::instance().classIdByName("Block");
    if (blockClassId == 0) {
        return nullptr;
    }
    return static_cast<ScriptBlockRef*>(ScriptObjectRegistry::unwrap(ctx, val, blockClassId));
}

} // namespace mc::mod::bedrock::addon
