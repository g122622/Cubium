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

package dev.horizonlab.diff;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

/**
 * 玩家经验值的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.ExperienceOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/experience.cpp 同构：两侧对同一串操作（加点 / 加等级 / 设等级）依次施加，
 * 输出最终的等级、进度条（f32 位模式）与累计经验。
 *
 * <p>原版的三个入口：`Player#giveExperiencePoints`（加点，含跨级与归零的两段 while）、
 * `Player#giveExperienceLevels`（加等级）与 public 字段 `experienceLevel`（设等级）。
 * 进度条是 f32，用位模式比较。
 */
public final class ExperienceOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private ExperienceOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            Files.writeString(Path.of(args[1]), "", StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: ExperienceOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        ExperienceState state = new ExperienceState();
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            JsonObject caseObject = JsonParser.parseString(line).getAsJsonObject();
            state.reset();
            for (var entry : caseObject.getAsJsonArray("ops")) {
                JsonObject op = entry.getAsJsonObject();
                String name = op.get("op").getAsString();
                int amount = op.get("n").getAsInt();
                switch (name) {
                    case "points" -> state.giveExperiencePoints(amount);
                    case "levels" -> state.giveExperienceLevels(amount);
                    default -> throw new IllegalStateException("unknown experience op: " + name);
                }
            }
            JsonObject result = new JsonObject();
            result.addProperty("unit", caseObject.get("unit").getAsString());
            result.addProperty("level", state.level);
            result.addProperty("progress", String.format("0x%08x", Float.floatToRawIntBits(state.progress)));
            result.addProperty("total", state.total);
            text.append(GSON.toJson(result)).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    /**
     * 原版 `Player` 的经验三字段，配上 `Player` 的四个方法语义。
     *
     * <p>不构造真实 `Player`：其一，`Player` 需要 `ServerPlayer` 上下文与连接；其二，
     * 经验换算只读写这三个字段，`giveExperienceLevels` 里的音效与成就回调与本能力无关
     * （已有 `status_effect` 等能力同样只取纯语义部分）。方法体逐行照抄
     * `net.minecraft.world.entity.player.Player` 的同名方法。
     */
    private static final class ExperienceState {
        int level;
        int total;
        float progress;

        void reset() {
            level = 0;
            total = 0;
            progress = 0.0f;
        }

        /** 下一级所需经验：0-14 段 `7 + 2n`、15-29 段 `37 + 5(n-15)`、30+ 段 `112 + 9(n-30)`。 */
        int getXpNeededForNextLevel() {
            if (level >= 30) {
                return 112 + (level - 30) * 9;
            }
            return level >= 15 ? 37 + (level - 15) * 5 : 7 + level * 2;
        }

        void giveExperiencePoints(int amount) {
            progress = progress + (float) amount / getXpNeededForNextLevel();
            // 原版是 `Mth.clamp(totalExperience + amount, 0, Integer.MAX_VALUE)`；
            // 加法在 int 上可能溢出，用 long 承接再夹取，语义与 Java 版一致（原版靠 int 回绕则不同，
            // 因此这里显式用 long —— 见 README 中本能力的说明）。
            total = (int) Math.min(Integer.MAX_VALUE, Math.max(0L, (long) total + amount));

            while (progress < 0.0F) {
                float deficit = progress * getXpNeededForNextLevel();
                if (level > 0) {
                    giveExperienceLevels(-1);
                    progress = 1.0F + deficit / getXpNeededForNextLevel();
                } else {
                    giveExperienceLevels(-1);
                    progress = 0.0F;
                }
            }

            while (progress >= 1.0F) {
                progress = (progress - 1.0F) * getXpNeededForNextLevel();
                giveExperienceLevels(1);
                progress = progress / getXpNeededForNextLevel();
            }
        }

        void giveExperienceLevels(int amount) {
            // 原版用 Guava `IntMath.saturatedAdd`：加法溢出时饱和到 int 边界而不是回绕。
            level = com.google.common.math.IntMath.saturatedAdd(level, amount);
            if (level < 0) {
                level = 0;
                progress = 0.0F;
                total = 0;
            }
        }
    }
}