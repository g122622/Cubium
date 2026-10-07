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
        // 阻塞点：服务端侧的效果变更**从不下发** entity_effect(cb 130)。实测 `/effect give`
        // 后服务端日志确认效果已施加（属性修饰符生效、剩余时间递减），但客户端收到的
        // entity_effect 计数恒为 0，`bot.entity.effects` 永远为空——玩家屏幕上没有效果图标，
        // 服务端却在按效果结算（移动速度加成、持续伤害/回血等），两侧状态长期背离且无任何日志。
        // 根因：`LivingEntity::addEffect`/`EffectManager::addEffect` 只写服务端状态，没有任何
        // 「效果变更 → 广播」通路；且 clientbound 的 `entity_effect`(cb 130) / `remove_entity_effect`
        // (cb 76) / `update_mob_effect` 三层（IR + codec + 协议表登记）全缺，无法发送。
        // 证据：见本次 refresh 运行落盘的 artifacts（runId 见运行输出），bot-trace 中
        //       entity_effect 计数为 0，而服务端日志显示效果已施加。
        // 解除条件：补齐 effect 三包的 IR/codec/协议表登记 + 服务端「效果变更即广播」通路
        //           （对齐 vanilla ServerPlayer.onEffectUpdated/onEffectRemoved → 广播给追踪者），
        //           然后移除此跳过标记。
        skipReason:
            "服务端施加状态效果后从不下发 entity_effect(cb 130)（且该包 IR/codec/协议表三层全缺），" +
            "客户端状态栏看不到任何效果；待补齐效果同步链路后移除此跳过标记",
        async run({ bot }): Promise<Record<string, unknown>> {
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
