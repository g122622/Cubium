// 幼年动物（FollowParentGoal + spawn_baby 事件）行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// 小牛必须靠近同类成年母牛。使用实体句柄，避免把游走的母牛当成小牛。
function babyCowFollowsMother(test: Test): void {
    const mother = test.spawn("minecraft:cow", { x: 6, y: 2, z: 4 });
    (mother.getComponent("minecraft:movement") as any).setCurrentValue(0);
    const baby = test.spawn("cow<minecraft:spawn_baby>", { x: 2, y: 2, z: 4 });
    const distance = () => Math.hypot(baby.location.x - mother.location.x, baby.location.z - mother.location.z);
    test.assert(distance() >= 3.5, "baby should start outside the follow stopping distance");
    let followedFirstPosition = false;
    let settledChecks = 0;
    let secondPhaseValid = false;
    let secondStart = { x: 0, z: 0 };
    pollUntilSucceed(test, () => {
        if (!followedFirstPosition) {
            // 先停稳十 tick，让第一条路径自然结束，避免旧路径惯性污染反向位移判定。
            if (distance() >= 3.0) {
                settledChecks = 0;
                return false;
            }
            if (++settledChecks < 10) return false;
        }
        if (!followedFirstPosition) {
            followedFirstPosition = true;
            secondStart = { x: baby.location.x, z: baby.location.z };
            const target = test.worldLocation({ x: 1, y: 2, z: 1 });
            mother.teleport({ x: target.x + 0.5, y: target.y, z: target.z + 0.5 });
            // 轮询会捕获异常，保留前提状态，失败后禁止任何后续成功。
            secondPhaseValid = distance() > 3.0;
            return false;
        }
        // 第一条路径向 +X；第二次必须向 -X、-Z，继续原路径无法通过。
        return secondPhaseValid && distance() < 3.5 &&
            baby.location.x < secondStart.x - 0.1 && baby.location.z < secondStart.z - 0.2;
    }, {
        startTick: 4, interval: 1, maxTick: 100,
        onTimeout: () => test.assert(false,
            `baby did not follow both mother positions: phase=${followedFirstPosition}, valid=${secondPhaseValid}, distance=${distance().toFixed(2)}, ` +
            `baby=${JSON.stringify(baby.location)}, ` +
            `mother=${JSON.stringify(mother.location)}`),
    });
}

export function registerBabyCowTests(): void {
    GameTest.register("MobBehaviorTests", "baby_cow_follows_mother", babyCowFollowsMother)
        .batch("baby_follow_parent")
        .structureName("gametests:grass_pen")
        .maxTicks(120);
}
