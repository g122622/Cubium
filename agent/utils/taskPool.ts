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

/**
 * 任务执行结果
 */
export interface TaskResult<T> {
    success: boolean;
    value?: T;
    error?: unknown;
    index: number;
}

/**
 * 任务执行回调参数
 */
export interface TaskCompleteCallback<T> {
    success: boolean;
    value?: T;
    error?: unknown;
    index: number;
}

/**
 * 通用的任务池，支持并发控制
 *
 * @example
 * ```ts
 * const pool = new TaskPool(5);
 *
 * const tasks = urls.map(url => () => fetch(url));
 *
 * await pool.runAll(tasks, (result) => {
 *   if (result.success) {
 *     console.log(`Task ${result.index} completed:`, result.value);
 *   } else {
 *     console.error(`Task ${result.index} failed:`, result.error);
 *   }
 * });
 * ```
 */
export class TaskPool {
    private readonly maxConcurrency: number;
    private runningTasks = 0;
    private readonly semaphoreQueue: Array<() => void> = [];

    /**
     * @param maxConcurrency 最大并发数，必须大于 0
     */
    constructor(maxConcurrency: number) {
        if (maxConcurrency <= 0) {
            throw new Error("maxConcurrency must be greater than 0");
        }
        this.maxConcurrency = maxConcurrency;
    }

    /**
     * 获取一个执行槽位（信号量 acquire）
     */
    private async acquireSlot(): Promise<void> {
        if (this.runningTasks < this.maxConcurrency) {
            this.runningTasks++;
            return;
        }

        return new Promise<void>(resolve => {
            this.semaphoreQueue.push(resolve);
        });
    }

    /**
     * 释放一个执行槽位，唤醒等待者（信号量 release）
     */
    private releaseSlot(): void {
        this.runningTasks--;
        const next = this.semaphoreQueue.shift();

        if (next) {
            this.runningTasks++;
            next();
        }
    }

    /**
     * 并行执行所有任务，每个任务完成时触发回调
     *
     * @param tasks 任务函数数组
     * @param onTaskComplete 单个任务完成时的回调（可选）
     * @returns 所有任务的结果数组，顺序与输入一致
     */
    async runAll<T>(
        tasks: Array<() => Promise<T>>,
        onTaskComplete?: (result: TaskCompleteCallback<T>) => void | Promise<void>
    ): Promise<TaskResult<T>[]> {
        if (tasks.length === 0) {
            return [];
        }

        const results: TaskResult<T>[] = new Array(tasks.length).fill(null as unknown as TaskResult<T>);
        const taskPromises: Promise<void>[] = [];

        for (let i = 0; i < tasks.length; i++) {
            const index = i;
            const task = tasks[i];

            taskPromises.push(
                (async () => {
                    await this.acquireSlot();

                    try {
                        const value = await task();
                        results[index] = { success: true, value, index };

                        if (onTaskComplete) {
                            try {
                                await onTaskComplete({ success: true, value, index });
                            } catch (callbackError) {
                                console.error(
                                    `TaskPool: onTaskComplete callback failed for task ${index}:`,
                                    callbackError
                                );
                            }
                        }
                    } catch (error) {
                        results[index] = { success: false, error, index };

                        if (onTaskComplete) {
                            try {
                                await onTaskComplete({ success: false, error, index });
                            } catch (callbackError) {
                                console.error(
                                    `TaskPool: onTaskComplete callback failed for task ${index}:`,
                                    callbackError
                                );
                            }
                        }
                    } finally {
                        this.releaseSlot();
                    }
                })()
            );
        }

        await Promise.all(taskPromises);
        return results;
    }

    /**
     * 获取当前运行中的任务数量
     */
    getRunningCount(): number {
        return this.runningTasks;
    }

    /**
     * 获取等待中的任务数量
     */
    getWaitingCount(): number {
        return this.semaphoreQueue.length;
    }
}
