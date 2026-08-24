// GameTest RegistrationBuilder 跨服务端兼容垫片。
//
// 背景：Cubium 的 C++ 绑定在官方 @minecraft/server-gametest 的 RegistrationBuilder 之上扩展了
// skyAccess 链式方法（对齐 Java GameTest TestData 字段，类型声明见 cubium-gametest-augment.d.ts）。
// 官方基岩 BDS 的 RegistrationBuilder 没有 skyAccess 方法，测试代码调用 .skyAccess(true) 时抛
// TypeError: not a function。该异常发生在 main.js 顶层 register 链式阶段，会导致整个行为包加载
// 失败、所有测试都无法注册。
//
// 注意：setupTicks 是基岩 RegistrationBuilder 原生方法（见 index.d.ts），无需降级；Cubium 也支持。
// 仅 skyAccess 是 Cubium 专有，需在基岩侧降级为 no-op。
//
// 修复策略（双保险，按可靠性排序）：
// 1. RegistrationBuilder.prototype 注入：基岩 RegistrationBuilder 是导出 class，prototype 可扩展。
//    在 prototype 上注入 skyAccess no-op（返回 this 保持链式）。基岩实例自身无 skyAccess，沿原型链
//    命中注入的 no-op；Cubium 实例自身有 C++ 绑定的 skyAccess，覆盖 prototype，无副作用。
//    此方案不替换 register，最可靠。
// 2. register Proxy 包装（备用）：若 prototype 注入失败（如基岩 prototype 冻结），退而用 Proxy 包装
//    GameTest.register 返回的 builder，对 skyAccess 降级 no-op。基岩 ESM namespace 属性可能 read-only，
//    赋值失败时 try/catch 静默保留原 register。
//
// 在 main.ts 顶部最先 import 本模块（副作用执行）。

import * as GameTest from "@minecraft/server-gametest";
import { RegistrationBuilder } from "@minecraft/server-gametest";

// skyAccess / loadSpawnChunks 是 Cubium 在官方 RegistrationBuilder 之上扩展的专有链式方法。
// 基岩 BDS 的 RegistrationBuilder 无此方法，需降级为 no-op（返回 this 保持链式）。
// setupTicks 是基岩原生方法，无需在此处理。
const CUBIUM_ONLY_METHODS = new Set<string>(["skyAccess", "loadSpawnChunks"]);

/**
 * 策略 1：向 RegistrationBuilder.prototype 注入 Cubium 专有方法的 no-op 降级实现。
 * - 基岩侧：实例自身无 skyAccess，沿原型链命中此 no-op，链式不断。
 * - Cubium 侧：实例自身有 C++ 绑定的 skyAccess，原型方法被覆盖，无副作用。
 * 仅当 prototype 上尚不存在该方法时注入（避免覆盖 Cubium 真实实现）。
 */
function installPrototypeShim(): boolean {
    try {
        const proto = RegistrationBuilder.prototype;
        for (const name of CUBIUM_ONLY_METHODS) {
            if (typeof (proto as unknown as Record<string, unknown>)[name] !== "function") {
                // no-op：忽略参数，返回 this 保持链式调用 .skyAccess(true).setupTicks(20).maxTicks(...)
                Object.defineProperty(proto, name, {
                    value: function (this: RegistrationBuilder) {
                        return this;
                    },
                    writable: true,
                    configurable: true,
                    enumerable: false,
                });
            }
        }
        return true;
    } catch {
        // prototype 冻结或不可扩展：退回策略 2。
        return false;
    }
}

/**
 * 策略 2（备用）：用 Proxy 包装 GameTest.register 返回的 builder。
 * Cubium 专有方法在基岩侧不存在时降级 no-op；普通方法透传，返回值若是对象则继续 wrap。
 */
function wrapBuilder<T extends object>(builder: T): T {
    return new Proxy(builder, {
        get(target, prop, receiver) {
            const value = Reflect.get(target, prop, target);
            if (typeof value === "function") {
                return (...args: unknown[]) => {
                    const ret = value.apply(target, args);
                    return (ret !== null && typeof ret === "object") ? wrapBuilder(ret) : ret;
                };
            }
            if (typeof prop === "string" && CUBIUM_ONLY_METHODS.has(prop)) {
                return () => receiver;
            }
            return value;
        },
    });
}

// 先尝试策略 1（prototype 注入），失败则策略 2（替换 register + Proxy）。
const protoOk = installPrototypeShim();
if (!protoOk) {
    const originalRegister = GameTest.register;
    try {
        // 基岩 ESM namespace 属性可能 read-only，赋值失败由 catch 静默保留原 register。
        // 用 any 绕过 TS namespace 只读校验。
        // eslint-disable-next-line @typescript-eslint/no-explicit-any
        (GameTest as any).register = (...args: Parameters<typeof originalRegister>) => {
            const builder = originalRegister(...args);
            return wrapBuilder(builder as object);
        };
    } catch {
        // 保留原 register。若走到这里且基岩无 skyAccess，行为包仍会加载失败——
        // 但 prototype 注入应已成功，不会走到此分支。
    }
}
