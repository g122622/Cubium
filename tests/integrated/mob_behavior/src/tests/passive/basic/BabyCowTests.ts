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
    pollUntilSucceed(test, () => distance() < 3.5, {
        startTick: 4, interval: 2, maxTick: 400,
        onTimeout: () => test.assert(false,
            `baby did not follow mother: distance=${distance().toFixed(2)}, baby=${JSON.stringify(baby.location)}, ` +
            `mother=${JSON.stringify(mother.location)}`),
    });
}

export function registerBabyCowTests(): void {
    GameTest.register("MobBehaviorTests", "baby_cow_follows_mother", babyCowFollowsMother)
        .structureName("gametests:grass_pen")
        .maxTicks(500);
}
