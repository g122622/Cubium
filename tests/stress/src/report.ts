import fs from "node:fs";
import path from "node:path";
import { statistics, type TickSample, type PhaseSummary } from "./metrics.ts";
import type { Phase } from "./context.ts";

/** 所有动态内容先转义，再嵌入可离线查看的报告。 */
function escape(value: unknown): string {
    return String(value).replace(/[&<>"']/g, (char) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" })[char]!);
}

/** 原始 CSV 保留逐 tick 数据，图表按秒聚合并保留 tick P95。 */
function chart(title: string, unit: string, samples: readonly TickSample[], phases: readonly Phase[], fields: Array<{ label: string; color: string; read: (samples: TickSample[]) => number | null }>): string {
    if (samples.length === 0) return `<p>${escape(title)}：没有有效样本。</p>`;
    const origin = phases[0]?.startMs ?? samples[0].timeMs;
    const duration = Math.max(1, ((phases.at(-1)?.endMs ?? samples.at(-1)!.timeMs) - origin) / 1000);
    const buckets = new Map<number, TickSample[]>();
    for (const sample of samples) {
        const second = Math.floor((sample.timeMs - origin) / 1000);
        if (second < 0 || second > duration) continue;
        const bucket = buckets.get(second) ?? [];
        bucket.push(sample);
        buckets.set(second, bucket);
    }
    const series = fields.map((field) => ({ ...field, points: [...buckets].map(([second, rows]) => ({ second, value: field.read(rows) })).filter((point) => point.value !== null) }));
    const maximum = Math.max(1, ...series.flatMap((field) => field.points.map((point) => point.value!))) * 1.1;
    const x = (seconds: number): number => 65 + seconds / duration * 920;
    const y = (value: number): number => 230 - value / maximum * 190;
    const bands = phases.map((phase, index) => `<rect x="${x((phase.startMs - origin) / 1000)}" y="25" width="${Math.max(0, (phase.endMs - phase.startMs) / 1000 / duration * 920)}" height="205" fill="${index % 2 ? "#edf2f7" : "#f8fafc"}"/><text x="${x((phase.startMs - origin) / 1000) + 3}" y="20" font-size="11">${escape(phase.name)}</text>`).join("");
    const paths = series.map((field) => `<polyline fill="none" stroke="${field.color}" stroke-width="2" points="${field.points.map((point) => `${x(point.second)},${y(point.value!)}`).join(" ")}"/>${field.points.map((point) => `<circle cx="${x(point.second)}" cy="${y(point.value!)}" r="3" fill="${field.color}"><title>${escape(field.label)} · ${point.second}s · ${point.value!.toFixed(2)} ${escape(unit)}</title></circle>`).join("")}`).join("");
    const grid = [0, 0.25, 0.5, 0.75, 1].map((fraction) => `<line x1="65" x2="985" y1="${y(maximum * fraction)}" y2="${y(maximum * fraction)}" stroke="#d5dde5"/><text x="58" y="${y(maximum * fraction) + 4}" text-anchor="end" font-size="11">${(maximum * fraction).toFixed(1)}</text>`).join("");
    return `<h2>${escape(title)}（${escape(unit)}）</h2><p>${fields.map((field) => `<span style="color:${field.color}">● ${escape(field.label)}</span>`).join("　")}</p><svg viewBox="0 0 1000 270" role="img" aria-label="${escape(title)}">${bands}${grid}${paths}<text x="65" y="255">0 s</text><text x="985" y="255" text-anchor="end">${duration.toFixed(1)} s</text></svg>`;
}

/** 写出分阶段指标和鼠标悬停可读数的曲线。 */
export function writeReport(directory: string, id: string, summary: object, phases: readonly Phase[], phaseSummaries: readonly PhaseSummary[], samples: readonly TickSample[]): void {
    const format = (value: number | undefined): string => value === undefined ? "—" : value.toFixed(2);
    const rows = phaseSummaries.map((phase) => `<tr><td>${escape(phase.name)}</td><td>${phase.durationSeconds.toFixed(1)}</td><td>${format(phase.rssMiB?.p50)}</td><td>${format(phase.commitMiB?.p50)}</td><td>${format(phase.tickMs?.mean)}</td><td>${format(phase.tickMs?.p95)}</td><td>${format(phase.tickMs?.max)}</td><td>${phase.maxPlayers}</td><td>${phase.endLoadedChunks ?? "—"}</td></tr>`).join("");
    const memory = chart("服务端内存", "MiB", samples, phases, [
        { label: "工作集均值", color: "#2563eb", read: (rows) => statistics(rows.flatMap((row) => row.rssMiB === null ? [] : [row.rssMiB]))?.mean ?? null },
        { label: "提交量均值", color: "#9333ea", read: (rows) => statistics(rows.flatMap((row) => row.commitMiB === null ? [] : [row.commitMiB]))?.mean ?? null },
    ]);
    const ticks = chart("服务端 tick 执行耗时", "ms", samples, phases, [
        { label: "每秒均值", color: "#059669", read: (rows) => statistics(rows.map((row) => row.tickMs))?.mean ?? null },
        { label: "每秒 P95", color: "#e11d48", read: (rows) => statistics(rows.map((row) => row.tickMs))?.p95 ?? null },
    ]);
    fs.writeFileSync(path.join(directory, "report.html"), `<!doctype html><html lang="zh-CN"><meta charset="utf-8"><title>${escape(id)}</title><style>body{font:15px system-ui;color:#172033;max-width:1100px;margin:32px auto;padding:0 20px}table{border-collapse:collapse;width:100%}td,th{border:1px solid #d5dde5;padding:8px;text-align:right}td:first-child,th:first-child{text-align:left}svg{width:100%}pre{white-space:pre-wrap;overflow-wrap:anywhere;background:#f1f5f9;padding:16px}h2{margin-top:32px}</style><h1>${escape(id)}</h1><p>每档使用独立新世界。内存由已有专用线程采样，CSV 在每个 tick 完成后读取最近样本；tick 耗时不含 50ms 节流等待。悬停曲线采样点可查看数值。</p><p><a href="server-metrics.csv">原始逐 tick CSV</a> · <a href="summary.json">完整统计</a> · <a href="events.jsonl">连接事件</a></p><table><thead><tr><th>阶段</th><th>秒</th><th>工作集 P50 MiB</th><th>提交量 P50 MiB</th><th>tick 均值 ms</th><th>tick P95 ms</th><th>tick 最大 ms</th><th>最多在线</th><th>末尾已加载区块</th></tr></thead><tbody>${rows}</tbody></table>${memory}${ticks}<h2>完整结果</h2><pre>${escape(JSON.stringify(summary, null, 2))}</pre></html>`);
}

/** 汇总页按真实执行顺序列出全部档位与负载用例。 */
export function writeIndex(directory: string, results: Array<{ id: string; passed: boolean; errors: string[] }>): void {
    fs.writeFileSync(path.join(directory, "index.html"), `<!doctype html><html lang="zh-CN"><meta charset="utf-8"><title>Cubium 压力测试</title><style>body{font:16px system-ui;max-width:1000px;margin:40px auto}td,th{text-align:left;padding:12px;border-bottom:1px solid #ddd}</style><h1>Cubium 压力测试</h1><p>全部用例及移动速度档位串行执行。</p><table><tr><th>用例</th><th>结果</th><th>错误</th></tr>${results.map((result) => `<tr><td><a href="${encodeURIComponent(result.id)}/report.html">${escape(result.id)}</a></td><td>${result.passed ? "通过" : "失败"}</td><td>${escape(result.errors.join("; "))}</td></tr>`).join("")}</table></html>`);
}
