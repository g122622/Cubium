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
import net.minecraft.world.scores.Team;

/**
 * {@code Team.Visibility} / {@code Team.CollisionRule} 字符串映射的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.TeamRuleStringsOracle list &lt;units.txt&gt;
 *       | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link Team.Visibility#getSerializedName()} /
 * {@link Team.CollisionRule#getSerializedName()}，反向用 {@code EnumCodec.byName}。
 *
 * <p>**本能力的探针对接 Cubium 的真实实现**
 * （`teamVisibilityToString` / `teamVisibilityFromString` 等）。
 *
 * <p>原版 {@code byName} 对未知输入返回 {@code null}——本 oracle 输出字面量 {@code null}，
 * 而 Cubium 的 {@code fromString} 回退到 {@code Always}。两者若不一致即暴露差异。
 *
 * <p>本能力不需要注册表。
 *
 * <p>单元表必须与 probes/team_rule_strings.cpp 逐项一致。
 */
public final class TeamRuleStringsOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "count",
                    "visibilityToString|ALWAYS",
                    "visibilityToString|NEVER",
                    "visibilityToString|HIDE_FOR_OTHER_TEAMS",
                    "visibilityToString|HIDE_FOR_OWN_TEAM",
                    "collisionToString|ALWAYS",
                    "collisionToString|NEVER",
                    "collisionToString|PUSH_OTHER_TEAMS",
                    "collisionToString|PUSH_OWN_TEAM",
                    "visibilityFromString|always",
                    "visibilityFromString|never",
                    "visibilityFromString|hideForOtherTeams",
                    "visibilityFromString|hideForOwnTeam",
                    "visibilityFromString|dunno",
                    "visibilityFromString|",
                    "visibilityFromString|ALWAYS",
                    "visibilityFromString|hide_for_other_teams",
                    "collisionFromString|always",
                    "collisionFromString|never",
                    "collisionFromString|pushOtherTeams",
                    "collisionFromString|pushOwnTeam",
                    "collisionFromString|dunno",
                    "collisionFromString|",
                    "collisionFromString|push_other_teams");

    private TeamRuleStringsOracle() {}

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
        System.err.println("usage: TeamRuleStringsOracle list <units.txt> "
                + "| run <input.jsonl> <output.jsonl>");
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

    /** 原版 {@code StringRepresentable.fromEnum} 的 {@code byName}：未命中返回 null。 */
    private static Team.Visibility visibilityByName(String name) {
        for (Team.Visibility visibility : Team.Visibility.values()) {
            if (visibility.getSerializedName().equals(name)) {
                return visibility;
            }
        }
        return null;
    }

    private static Team.CollisionRule collisionByName(String name) {
        for (Team.CollisionRule rule : Team.CollisionRule.values()) {
            if (rule.getSerializedName().equals(name)) {
                return rule;
            }
        }
        return null;
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        if (parts.length == 1) {
            if (!parts[0].equals("count")) {
                throw new IllegalArgumentException("team_rule_strings: 无法识别的单元: " + unit);
            }
            out.addProperty("value", String.valueOf(Team.Visibility.values().length));
            return out;
        }
        if (parts.length != 2) {
            throw new IllegalArgumentException("team_rule_strings: 单元段数不符: " + unit);
        }

        switch (parts[0]) {
            case "visibilityToString" -> out.addProperty("value",
                    Team.Visibility.valueOf(parts[1]).getSerializedName());
            case "collisionToString" -> out.addProperty("value",
                    Team.CollisionRule.valueOf(parts[1]).getSerializedName());
            case "visibilityFromString" -> {
                Team.Visibility visibility = visibilityByName(parts[1]);
                out.addProperty("value", visibility == null ? "null" : visibility.name());
            }
            case "collisionFromString" -> {
                Team.CollisionRule rule = collisionByName(parts[1]);
                out.addProperty("value", rule == null ? "null" : rule.name());
            }
            default -> throw new IllegalArgumentException(
                    "team_rule_strings: 无法识别的单元: " + unit);
        }

        return out;
    }
}