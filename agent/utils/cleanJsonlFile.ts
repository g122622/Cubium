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

import fs from 'fs/promises';

/**
 * 清洗 JSONL 文件内容，转换为人类可读的对话文本。
 * - 保留用户直接输入的消息（role: user, type: text）
 * - 保留助手的直接回复（role: assistant, type: text）
 * - 保留助手的思考内容（type: thinking）
 * - 保留助手的工具调用（type: tool_use，不含工具返回结果）
 * - 跳过工具返回结果（tool_result）
 * - 保留待办事项（type: attachment, todo_reminder）和最后提示（last-prompt）作为摘要
 *
 * @param filePath - JSONL 文件路径
 * @returns 清洗后的纯文本字符串
 */
export async function cleanJsonlFile(filePath: string): Promise<string> {
    /**
     * 从用户消息的 content 数组中提取所有 type === 'text' 的文本内容。
     */
    function extractTextFromUserContent(content: any[]): string {
        if (!Array.isArray(content)) return '';
        const textParts: string[] = [];
        for (const item of content) {
            if (item.type === 'text' && typeof item.text === 'string') {
                textParts.push(item.text);
            }
        }
        return textParts.join(' ').trim();
    }

    /**
     * 从助手消息的 content 数组中提取所有内容（text、thinking、tool_use）。
     * 不包括工具返回结果。
     */
    function extractContentFromAssistantMessage(content: any[]): string {
        if (!Array.isArray(content)) return '';
        const parts: string[] = [];
        for (const item of content) {
            // 1. 文本内容
            if (item.type === 'text' && typeof item.text === 'string') {
                parts.push(item.text);
            }
            // 2. 思考内容
            else if (item.type === 'thinking' && typeof item.thinking === 'string') {
                parts.push(`<Thinking> ${item.thinking} </Thinking>`);
            }
            // 3. 工具调用（不含工具返回结果）
            else if (item.type === 'tool_use' && item.name) {
                const toolName = item.name;
                const toolInput = item.input ? JSON.stringify(item.input) : '';
                parts.push(`<ToolCall> ${toolName}(${toolInput}) </ToolCall>`);
            }
        }
        return parts.join('\n').trim();
    }

    const content = await fs.readFile(filePath, 'utf-8');
    const lines = content.split(/\r?\n/).filter(line => line.trim().length > 0);

    const outputParts: string[] = [];

    for (const line of lines) {
        let obj: any;
        try {
            obj = JSON.parse(line);
        } catch {
            continue; // 忽略无效 JSON 行
        }

        // 1. 用户消息（直接提问或指令）
        if (obj.type === 'user' && obj.message?.role === 'user') {
            const userText = extractTextFromUserContent(obj.message.content);
            if (userText) {
                outputParts.push(`<User> ${userText} </User>\n`);
            }
        }

        // 2. 助手消息（包括文本、思考、工具调用）
        else if (obj.type === 'assistant' && obj.message?.role === 'assistant') {
            const assistantContent = extractContentFromAssistantMessage(obj.message.content);
            if (assistantContent) {
                outputParts.push(`<Assistant> ${assistantContent} </Assistant>\n`);
            }
        }

        // 3. 待办事项附件
        else if (obj.type === 'attachment' && obj.attachment?.type === 'todo_reminder') {
            const todoItems = obj.attachment.content;
            if (Array.isArray(todoItems) && todoItems.length > 0) {
                outputParts.push(`<TodoList> 待办事项:\n`);
                for (const item of todoItems) {
                    const status = item.status === 'completed' ? '[x]' : '[ ]';
                    outputParts.push(`${status} ${item.content}\n`);
                }
                outputParts.push(`</TodoList>\n`);
            }
        }

        // 4. 最后提示（last-prompt）
        else if (obj.type === 'last-prompt' && obj.lastPrompt) {
            outputParts.push(`<LastPrompt> ${obj.lastPrompt} </LastPrompt>\n`);
        }
    }

    return outputParts.join('').trim();
}