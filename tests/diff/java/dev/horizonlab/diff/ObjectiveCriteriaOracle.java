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
import java.lang.reflect.Field;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import java.util.Locale;
import net.minecraft.world.scores.criteria.ObjectiveCriteria;

/**
 * {@code ObjectiveCriteria} 判据名与渲染类型的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.ObjectiveCriteriaOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt;
 *       &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>通过反射读取原版静态常量，再调用 {@link ObjectiveCriteria#getName()} /
 * {@link ObjectiveCriteria#isReadOnly()} / {@link ObjectiveCriteria#getDefaultRenderType()}。
 *
 * <p>本能力不需要注册表。
 *
 * <p>单元表必须与 probes/objective_criteria.cpp 逐项一致。
 */
public final class ObjectiveCriteriaOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "name|DUMMY",
                    "name|TRIGGER",
                    "name|DEATH_COUNT",
                    "name|KILL_COUNT_PLAYERS",
                    "name|KILL_COUNT_ALL",
                    "name|HEALTH",
                    "name|FOOD",
                    "name|AIR",
                    "name|ARMOR",
                    "name|EXPERIENCE",
                    "name|LEVEL",
                    "readOnly|DUMMY",
                    "readOnly|DEATH_COUNT",
                    "readOnly|HEALTH",
                    "readOnly|EXPERIENCE",
                    "readOnly|LEVEL",
                    "renderType|HEALTH",
                    "renderType|DUMMY",
                    "renderType|FOOD",
                    "renderType|LEVEL",
                    "teamKill|0",
                    "teamKill|1",
                    "teamKill|15",
                    "killedByTeam|0",
                    "killedByTeam|1",
                    "killedByTeam|15",
                    "teamKillCount",
                    "killedByTeamCount");

    private ObjectiveCriteriaOracle() {}

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
        System.err.println("usage: ObjectiveCriteriaOracle list <units.txt> "
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

    /** 按常量名反射读取 {@link ObjectiveCriteria} 静态字段。 */
    private static ObjectiveCriteria constantByName(String constant) {
        try {
            Field field = ObjectiveCriteria.class.getField(constant.toUpperCase(Locale.ROOT));
            return (ObjectiveCriteria) field.get(null);
        } catch (ReflectiveOperationException exception) {
            throw new IllegalStateException(
                    "objective_criteria: 无法读取常量 " + constant, exception);
        }
    }

    /** 按数组名反射读取 {@link ObjectiveCriteria} 静态数组。 */
    private static ObjectiveCriteria[] arrayByName(String arrayName) {
        try {
            Field field = ObjectiveCriteria.class.getField(arrayName);
            return (ObjectiveCriteria[]) field.get(null);
        } catch (ReflectiveOperationException exception) {
            throw new IllegalStateException(
                    "objective_criteria: 无法读取数组 " + arrayName, exception);
        }
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        if (parts.length == 1) {
            switch (parts[0]) {
                case "teamKillCount" ->
                        out.addProperty("value", String.valueOf(arrayByName("TEAM_KILL").length));
                case "killedByTeamCount" -> out.addProperty("value",
                        String.valueOf(arrayByName("KILLED_BY_TEAM").length));
                default -> throw new IllegalArgumentException(
                        "objective_criteria: 无法识别的单元: " + unit);
            }
            return out;
        }
        if (parts.length != 2) {
            throw new IllegalArgumentException("objective_criteria: 单元段数不符: " + unit);
        }

        switch (parts[0]) {
            case "name" -> out.addProperty("value", constantByName(parts[1]).getName());
            case "readOnly" -> out.addProperty("value",
                    String.valueOf(constantByName(parts[1]).isReadOnly()));
            case "renderType" -> out.addProperty("value",
                    constantByName(parts[1]).getDefaultRenderType().getSerializedName());
            case "teamKill" -> out.addProperty("value",
                    arrayByName("TEAM_KILL")[Integer.parseInt(parts[1])].getName());
            case "killedByTeam" -> out.addProperty("value",
                    arrayByName("KILLED_BY_TEAM")[Integer.parseInt(parts[1])].getName());
            default -> throw new IllegalArgumentException(
                    "objective_criteria: 无法识别的单元: " + unit);
        }

        return out;
    }
}