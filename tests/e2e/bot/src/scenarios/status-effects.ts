/*
 * status-effects.ts — 场景 L：状态效果的客户端下发。
 *
 * 覆盖「服务端施加效果 → 客户端收到 entity_effect(cb 130) → 状态栏出现效果图标」这条链路。
 * 此前完全没有自动化覆盖，而它是典型的「服务端静默生效、客户端毫无反应」：
 *
 *   1. 效果本身由服务端权威施加（`LivingEntity::addEffect` → `EffectManager`），
 *      属性修饰符（如 speed 的移动速度加成）也在服务端侧正确生效；
 *   2. 但**客户端只能靠 entity_effect 包得知效果存在**——服务端不下发时，玩家屏幕上
 *      没有任何效果图标、也没有剩余时间条，而服务端却在按效果结算（移动更快、持续掉血等）。
 *      两侧状态长期背离且零日志。
 *
 * 判据取**客户端侧**的 `bot.entity.effects`：它由 mineflayer 在收到 entity_effect 时填充
 * （prismarine-entity 的 effects 表，键为效果 id）。服务端不下发该包时它恒为空。
 */

import type { CaseDefinition } from "../case.ts";
import { expectTrue } from "../assert/expect.ts";
import { runCommand } from "./shared.ts";
import { waitForCondition } from "../bot/wait.ts";

/** 效果名解析（客户端注册表里 speed 恒为 0 号效果）。 */
const SPEED_EFFECT_ID = 0;

/** 命令反馈的等待上限。 */
const COMMAND_FEEDBACK_TIMEOUT_MS = 5_000;
/** 等待 entity_effect 到达的上限。 */
const EFFECT_TIMEOUT_MS = 10_000;

/** 读 bot 自身实体上的效果表（键为效果 id）。 */
function selfEffects(bot: unknown): Record<number, { amplifier?: number; duration?: number }> {
    const entity = (bot as { entity?: { effects?: Record<number, { amplifier?: number; duration?: number }> } }).entity;
    return entity?.effects ?? {};
}

export const statusEffectCases: readonly CaseDefinition[] = [
    {
        id: "effects/give_effect_reaches_client",
        title: "服务端施加效果后客户端收到 entity_effect（状态栏能看到效果）",
        servers: ["cubium", "vanilla"],
        botCount: 1,
        opPlayers: true,
        skipReason: null,        async run({ bot }): Promise<Record<string, unknown>> {
            // 以自身为目标施加 speed（0 号效果）。命令名不带命名空间——两侧均接受裸名。
            await runCommand(bot, "/effect give @s speed 30 0", COMMAND_FEEDBACK_TIMEOUT_MS);

            // 服务端若正确下发，客户端会收到 entity_effect 并把 speed 记进 effects 表。
            await waitForCondition(() => selfEffects(bot)[SPEED_EFFECT_ID] !== undefined, {
                timeoutMs: EFFECT_TIMEOUT_MS,
                pollMs: 100,
                what: "客户端收到自身 speed 效果（entity_effect cb 130）",
                describe: () =>
                    `当前客户端已知效果：${JSON.stringify(Object.keys(selfEffects(bot)))}` +
                    "（为空即服务端未下发 entity_effect）",
            });

            const effect = selfEffects(bot)[SPEED_EFFECT_ID];
            expectTrue(effect !== undefined, "speed 效果未出现在客户端效果表中");
            return {
                speedEffectVisibleOnClient: true,
                effectId: SPEED_EFFECT_ID,
                amplifierIsZero: effect.amplifier === 0,
            };
        },
    },
];
