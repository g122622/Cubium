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
import java.util.List;
import java.util.Locale;
import net.minecraft.network.chat.contents.TranslatableContents;
import net.minecraft.world.scores.Team;

/**
 * {@code Team.CollisionRule} 与 {@code Team.Visibility} 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.TeamRuleOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link Team.CollisionRule#getSerializedName()} /
 * {@link Team.Visibility#getSerializedName()} / {@code getDisplayName()}。
 *
 * <p>{@code BY_ID} 是私有字段，通过公开的 {@code STREAM_CODEC} 解码验证越界行为；
 * 本例改用反射读取以保持简洁。
 *
 * <p>本能力不需要注册表。
 *
 * <p>单元表必须与 probes/team_rule.cpp 逐项一致。
 */
public final class TeamRuleOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "collisionName|ALWAYS",
                    "collisionName|NEVER",
                    "collisionName|PUSH_OTHER_TEAMS",
                    "collisionName|PUSH_OWN_TEAM",
                    "collisionId|ALWAYS",
                    "collisionId|PUSH_OWN_TEAM",
                    "collisionById|0",
                    "collisionById|3",
                    "collisionById|4",
                    "collisionById|-1",
                    "collisionById|1000",
                    "collisionDisplay|ALWAYS",
                    "collisionDisplay|PUSH_OTHER_TEAMS",
                    "visibilityName|ALWAYS",
                    "visibilityName|NEVER",
                    "visibilityName|HIDE_FOR_OTHER_TEAMS",
                    "visibilityName|HIDE_FOR_OWN_TEAM",
                    "visibilityId|ALWAYS",
                    "visibilityId|HIDE_FOR_OWN_TEAM",
                    "visibilityById|0",
                    "visibilityById|3",
                    "visibilityById|4",
                    "visibilityById|-1",
                    "visibilityDisplay|ALWAYS",
                    "visibilityDisplay|HIDE_FOR_OWN_TEAM",
                    "collisionCount",
                    "visibilityCount");

    private TeamRuleOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (String unit : UNITS) {
                text.append(unit).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println(
                "usage: TeamRuleOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String unit = JsonParser.parseString(line).getAsJsonObject().get("unit").getAsString();
            text.append(GSON.toJson(summarise(unit))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    /** 从 {@code Component} 取出翻译键。 */
    private static String translationKey(net.minecraft.network.chat.Component component) {
        if (component.getContents() instanceof TranslatableContents translatable) {
            return translatable.getKey();
        }
        throw new IllegalStateException("team_rule: 期望 TranslatableContents，实际是 "
                + component.getContents().getClass().getName());
    }

    private static Team.CollisionRule collisionByName(String name) {
        return Team.CollisionRule.valueOf(name.toUpperCase(Locale.ROOT));
    }

    private static Team.Visibility visibilityByName(String name) {
        return Team.Visibility.valueOf(name.toUpperCase(Locale.ROOT));
    }

    /** `ByIdMap.continuous(..., OutOfBoundsStrategy.ZERO)`：越界返回 `values()[0]`。 */
    private static <T> T byId(T[] values, int id) {
        return id >= 0 && id < values.length ? values[id] : values[0];
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        if (parts.length == 1) {
            switch (parts[0]) {
                case "collisionCount" -> out.addProperty("value",
                        String.valueOf(Team.CollisionRule.values().length));
                case "visibilityCount" -> out.addProperty("value",
                        String.valueOf(Team.Visibility.values().length));
                default -> throw new IllegalArgumentException("team_rule: 无法识别的单元: " + unit);
            }
            return out;
        }
        if (parts.length != 2) {
            throw new IllegalArgumentException("team_rule: 单元段数不符: " + unit);
        }

        switch (parts[0]) {
            case "collisionName" -> out.addProperty("value",
                    collisionByName(parts[1]).getSerializedName());
            case "collisionId" -> out.addProperty("value",
                    String.valueOf(collisionByName(parts[1]).id));
            case "collisionById" -> out.addProperty("value",
                    byId(Team.CollisionRule.values(), Integer.parseInt(parts[1])).name());
            case "collisionDisplay" -> out.addProperty("value",
                    translationKey(collisionByName(parts[1]).getDisplayName()));
            case "visibilityName" -> out.addProperty("value",
                    visibilityByName(parts[1]).getSerializedName());
            case "visibilityId" -> out.addProperty("value",
                    String.valueOf(visibilityByName(parts[1]).id));
            case "visibilityById" -> out.addProperty("value",
                    byId(Team.Visibility.values(), Integer.parseInt(parts[1])).name());
            case "visibilityDisplay" -> out.addProperty("value",
                    translationKey(visibilityByName(parts[1]).getDisplayName()));
            default -> throw new IllegalArgumentException("team_rule: 无法识别的单元: " + unit);
        }

        return out;
    }
}