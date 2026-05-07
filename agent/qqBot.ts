/**
 * QQ 机器人日志转发模块
 *
 * 功能：劫持 console 方法，将日志消息通过 QQ 机器人转发到指定账号
 * - 劫持 console.log, console.error, console.warn, console.info
 * - 添加时间戳和日志级别前缀
 * - 通过 WebSocket 长连接主动发送私聊消息
 */

import { createOpenAPI, createWebsocket, AvailableIntentsEventsEnum } from 'qq-bot-sdk';

// 机器人配置
const BOT_CONFIG = {
  appID: '1903974008',
  secret: '9C2d096qOjrnTt40',
  intents: [AvailableIntentsEventsEnum.GROUP_AND_C2C_EVENT],
};

// 目标 QQ 号
const TARGET_QQ = '0043a7d2619190185bfecfa5c3ea2ffa';

// 日志级别映射
const LOG_LEVELS = {
  log: 'LOG',
  error: 'ERROR',
  warn: 'WARN',
  info: 'INFO',
} as const;

type LogLevel = keyof typeof LOG_LEVELS;

// 保存原始 console 方法
const originalConsole = {
  log: console.log.bind(console),
  error: console.error.bind(console),
  warn: console.warn.bind(console),
  info: console.info.bind(console),
};

// API 客户端
let client: ReturnType<typeof createOpenAPI> | null = null;

/**
 * 格式化时间戳
 */
function formatTimestamp(): string {
  const now = new Date();
  const year = now.getFullYear();
  const month = String(now.getMonth() + 1).padStart(2, '0');
  const day = String(now.getDate()).padStart(2, '0');
  const hour = String(now.getHours()).padStart(2, '0');
  const minute = String(now.getMinutes()).padStart(2, '0');
  const second = String(now.getSeconds()).padStart(2, '0');
  return `${year}-${month}-${day} ${hour}:${minute}:${second}`;
}

/**
 * 将参数转换为字符串
 */
function stringifyArgs(args: unknown[]): string {
  return args
    .map((arg) => {
      if (typeof arg === 'string') {
        return arg;
      }
      if (arg instanceof Error) {
        return `${arg.name}: ${arg.message}\n${arg.stack || ''}`;
      }
      try {
        return JSON.stringify(arg, null, 2);
      } catch {
        return String(arg);
      }
    })
    .join(' ');
}

/**
 * 发送私聊消息（fire-and-forget）
 */
function sendC2CMessage(content: string): void {
  if (!client) {
    return;
  }

  // 异步发送，不等待结果
  client.c2cApi
    .postMessage(TARGET_QQ, {
      content,
      msg_type: 1
    })
    .catch((error) => {
      // 仅在原始控制台打印错误
      originalConsole.error('[QQ Bot] 消息发送失败:', error);
    });
}

/**
 * 创建劫持的 console 方法
 */
function createHookedMethod(level: LogLevel): (...args: unknown[]) => void {
  return (...args: unknown[]) => {
    // 1. 先调用原始方法（保证控制台正常输出）
    originalConsole[level](...args);

    // 2. 格式化消息内容
    const timestamp = formatTimestamp();
    const levelLabel = LOG_LEVELS[level];
    const content = stringifyArgs(args);
    const formattedMessage = `[${timestamp}] [${levelLabel}] ${content}`;

    // 3. 发送消息（fire-and-forget）
    sendC2CMessage(formattedMessage);
  };
}

/**
 * 初始化 QQ 机器人
 */
export async function initQQBot(): Promise<void> {
  try {
    // 创建 API 客户端
    client = createOpenAPI(BOT_CONFIG);

    // 创建 WebSocket 连接（保持连接活跃）
    const ws = createWebsocket(BOT_CONFIG);

    // 监听连接事件
    ws.on('READY', () => {
      originalConsole.log('[QQ Bot] WebSocket 连接就绪');
    });

    ws.on('ERROR', (data) => {
      originalConsole.error('[QQ Bot] WebSocket 错误:', data);
    });

    // 劫持 console 方法
    console.log = createHookedMethod('log');
    console.error = createHookedMethod('error');
    console.warn = createHookedMethod('warn');
    console.info = createHookedMethod('info');

    originalConsole.log('[QQ Bot] 初始化完成，console 方法已劫持');
    originalConsole.log(`[QQ Bot] 目标账号: ${TARGET_QQ}`);
  } catch (error) {
    originalConsole.error('[QQ Bot] 初始化失败:', error);
    throw error;
  }
}

/**
 * 关闭 QQ 机器人（恢复原始 console）
 */
export function closeQQBot(): void {
  console.log = originalConsole.log;
  console.error = originalConsole.error;
  console.warn = originalConsole.warn;
  console.info = originalConsole.info;

  originalConsole.log('[QQ Bot] 已关闭，console 方法已恢复');
}
