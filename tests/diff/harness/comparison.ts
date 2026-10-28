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

/** 对象仅比较参照声明的字段；数组必须等长，再递归比较各元素。 */
export function projectDeclaredFields(template: unknown, actual: unknown): unknown {
    if (Array.isArray(template)) {
        if (!Array.isArray(actual) || template.length !== actual.length) {
            return actual;
        }
        return template.map((item, index) => projectDeclaredFields(item, actual[index]));
    }
    if (template !== null && typeof template === "object") {
        if (actual === null || typeof actual !== "object" || Array.isArray(actual)) {
            return actual;
        }
        const source = actual as Record<string, unknown>;
        const result: Record<string, unknown> = {};
        for (const [field, expected] of Object.entries(template as Record<string, unknown>)) {
            result[field] = projectDeclaredFields(expected, source[field]);
        }
        return result;
    }
    return actual;
}
