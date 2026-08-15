// 跨服务端周期轮询工具：在 Cubium 与官方基岩 BDS 两端统一实现"每隔 N tick 检查条件，满足即 succeed，
// 超时则报错"的语义。
//
// 背景：基岩 BDS 的 Test.succeedWhen(callback) 语义是"每 tick 跑回调，回调不抛异常即标记成功"——
// 即回调里 test.assert 失败（抛异常）会被基岩当作**立即 FAIL**，而非"该 tick 不成功、继续等下个 tick"。
// Cubium 的 succeedWhen 则把 assert 失败当"条件未满足、继续轮询"（BaseGameTestInstance.cpp:88-90
// allPass=false 不 fail）。两端语义不一致：依赖 Cubium"继续轮询"语义的 succeedWhen+assert 测试
// 在基岩会首 tick 立即 FAIL。
//
// 解决：用 Test.runAtTickTime(tick, callback)（两端语义一致：在指定 tick 跑一次 callback）实现周期轮询。
// 预注册方案：测试函数内一次性预生成检查点 tick 列表 [startTick, startTick+interval, ..., maxTick]，
// 逐个调 runAtTickTime 注册。每个检查点 callback 检查 condition()：满足调 test.succeed()；不满足则
// 若是最后一个检查点（maxTick）调 onTimeout()（通常抛 assert 报清晰错误），否则啥也不做等下个检查点。
//
// 为何预注册而非自递归：BaseGameTestInstance::tick（BaseGameTestInstance.cpp:52）用 range-based for
// 遍历 m_runAtTickTime vector 执行到期 callback。若 callback 内再调 runAtTickTime（自递归注册下一个
// 检查点），会触发 m_runAtTickTime.emplace_back（:183）——若 vector 扩容重分配，range-based for 的
// 迭代器失效导致 UB/崩溃。预注册在测试函数体执行期间完成所有 emplace_back，遍历期间不再修改 vector，
// 彻底规避此风险。
//
// succeed 后剩余检查点不执行：BaseGameTestInstance::tick 开头 isDone(m_state) 即 return（:47-49），
// succeed 后状态为 Succeeded，后续 tick 跳过 runAtTickTime 执行。
//
// callback 返回值约定：runAtTickTime callback 经 wrapJsCallback（ScriptCallbackUtil.hpp:71-85）包装，
// 不抛异常→pass()（空 optional，不 fail 不 succeed）；抛异常→fail(...)（立即 FAIL）。故 condition()
// 的异常由 helper 内 try/catch 兜底转为"未满足"（不抛），onTimeout() 内 test.assert(false,...) 抛出的
// 异常才会触发 FAIL（这是期望的超时报错路径）。
//
// 适用场景：伤害类测试（实体需等待 AI 触发 + 无敌帧节流后才掉血）、AI 行为测试（goal 触发时序
// 非确定）等"条件何时满足不可预测、需轮询等待"的场景。比 succeedWhen+assert 更稳健，两端通用。

import type { Test } from "@minecraft/server-gametest";

export interface PollOptions {
  /** 首次检查的 tick（测试开始后），默认 10（留 spawn 注册稳定时间）。须 >=1（Cubium runAtTickTime 拒绝 tick=0）。 */
  startTick?: number;
  /** 检查间隔（tick），默认 20（1 秒）。 */
  interval?: number;
  /** 超时 tick：超过此 tick 仍未满足调 onTimeout。应 <= 测试注册的 maxTicks，否则测试先 ExecutionTimeout。 */
  maxTick: number;
  /** 超时时调用，通常 test.assert(false, "清晰错误信息")（抛异常经 wrapJsCallback 转 FAIL）。 */
  onTimeout: () => void;
}

/**
 * 周期轮询直到 condition 返回 true 调 test.succeed()，超时调 onTimeout()。
 *
 * 预注册 [startTick, startTick+interval, ..., maxTick] 检查点，每个检查点查 condition()：
 * 满足→succeed；最后检查点（maxTick）仍未满足→onTimeout（抛 assert FAIL）；中间检查点未满足→空过。
 *
 * @param test GameTest Test 对象
 * @param condition 条件谓词，返回 true 表示满足（将 succeed）。内部已 try/catch 兜底，异常视为"未满足"。
 * @param opts 轮询参数
 */
export function pollUntilSucceed(test: Test, condition: () => boolean, opts: PollOptions): void {
  const startTick = opts.startTick ?? 10;
  const interval = opts.interval ?? 20;
  const maxTick = opts.maxTick;

  // 预生成检查点列表：[startTick, startTick+interval, ..., maxTick]。
  // 循环用 t < maxTick（不含 maxTick），最后单独 push(maxTick) 确保最后一个检查点恰在 maxTick
  // （触发 onTimeout 的边界），且即使 maxTick 不是 interval 整数倍也覆盖。
  const ticks: number[] = [];
  for (let t = startTick; t < maxTick; t += interval) {
    ticks.push(t);
  }
  ticks.push(maxTick);
  const lastTick = ticks[ticks.length - 1];

  for (const tick of ticks) {
    const isLast = tick === lastTick;
    test.runAtTickTime(tick, () => {
      let satisfied = false;
      try {
        satisfied = condition();
      } catch {
        // condition 抛异常（如实体暂时不存在）视为"未满足"，等下个检查点（不抛出，避免被 wrapJsCallback 转 FAIL）。
        satisfied = false;
      }
      if (satisfied) {
        test.succeed();
        return;
      }
      if (isLast) {
        // 最后一个检查点（maxTick）仍未满足→超时，onTimeout 内通常 assert(false,...) 抛异常触发 FAIL。
        opts.onTimeout();
      }
      // 非最后检查点未满足：啥也不做，等下个检查点。
    });
  }
}
