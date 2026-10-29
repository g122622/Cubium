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
 * 两侧结果的投影与本比较。
 *
 * 语义：对象只比参照声明过的字段（参照没给的信息不是「不一致」）；数组必须等长，
 * 等长时逐元素比较，**并按规范序重排**——数组顺序只有在调用方显式声明为无序时才被忽略。
 *
 * @param template 参照一侧的值，决定「哪些字段算数」
 * @param actual 被比较一侧的值
 * @param unorderedArrays 视为无序的数组路径（用 `/` 连接字段名，根路径为 `""`）。
 *        只有明确在这里列出的路径才排序，其余数组按顺序逐元素比较。
 * @param path 当前递归路径（内部使用）
 */
export function projectDeclaredFields(
    template: unknown,
    actual: unknown,
    unorderedArrays: readonly string[] = [],
    path = "",
): unknown {
    if (Array.isArray(template)) {
        if (!Array.isArray(actual) || template.length !== actual.length) {
            return actual;
        }
        const projected = template.map((item, index) =>
            projectDeclaredFields(item, actual[index], unorderedArrays, path),
        );
        return unorderedArrays.includes(path) ? sortCanonically(projected) : projected;
    }
    if (template !== null && typeof template === "object") {
        if (actual === null || typeof actual !== "object" || Array.isArray(actual)) {
            return actual;
        }
        const source = actual as Record<string, unknown>;
        const result: Record<string, unknown> = {};
        for (const [field, expected] of Object.entries(template as Record<string, unknown>)) {
            const childPath = path === "" ? field : `${path}/${field}`;
            result[field] = projectDeclaredFields(expected, source[field], unorderedArrays, childPath);
        }
        return result;
    }
    return actual;
}

/**
 * 按规范顺序重排：先把元素自身递归规范化（对象键排序），再取其 JSON 文本排序。
 *
 * 元素是对象、且对象内部还有数组时，内部数组是否无序由调用方在路径里声明；
 * 这里只保证「同一组元素、不同排列」比较结果相同。
 */
function sortCanonically(value: unknown): unknown {
    if (Array.isArray(value)) {
        return value
            .map((item) => sortCanonically(item))
            .sort((left, right) => {
                const leftKey = JSON.stringify(left) ?? "";
                const rightKey = JSON.stringify(right) ?? "";
                return leftKey < rightKey ? -1 : leftKey > rightKey ? 1 : 0;
            });
    }
    if (value !== null && typeof value === "object") {
        const source = value as Record<string, unknown>;
        const result: Record<string, unknown> = {};
        for (const field of Object.keys(source).sort()) {
            result[field] = sortCanonically(source[field]);
        }
        return result;
    }
    return value;
}