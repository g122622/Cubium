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
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.resources.Identifier;
import net.minecraft.world.level.gamerules.GameRule;

/**
 * 游戏规则的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.GameRuleOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.GameRuleOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/gamerule.cpp 同构：每个规则报分类、类型、默认值（整型另报取值范围）。
 * 单元名不带命名空间——`/gamerule` 命令与存档都用短名。
 *
 * <p>取值范围由原版参数类型反推：{@code IntegerArgumentType.getMinimum/getMaximum}。
 */
public final class GameRuleOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private GameRuleOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> names = new ArrayList<>();
            for (Identifier id : BuiltInRegistries.GAME_RULE.keySet()) {
                names.add(id.getPath());
            }
            names.sort(Comparator.naturalOrder());
            StringBuilder text = new StringBuilder();
            for (String name : names) {
                text.append(name).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: GameRuleOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String name = JsonParser.parseString(line).getAsJsonObject().get("rule").getAsString();
            text.append(GSON.toJson(summarise(name))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject summarise(String name) {
        JsonObject result = new JsonObject();
        result.addProperty("rule", name);

        GameRule<?> rule = BuiltInRegistries.GAME_RULE
                .getOptional(Identifier.withDefaultNamespace(name))
                .orElse(null);
        if (rule == null) {
            result.addProperty("missing", true);
            return result;
        }

        // GameRuleCategory 是 record(Identifier id)，分类名取路径部分（player/mobs/...）。
        result.addProperty("category", rule.category().id().getPath());
        Object defaultValue = rule.defaultValue();
        if (defaultValue instanceof Boolean flag) {
            result.addProperty("type", "boolean");
            result.addProperty("default", flag);
            return result;
        }

        result.addProperty("type", "integer");
        result.addProperty("default", (Integer) defaultValue);
        if (rule.argument() instanceof com.mojang.brigadier.arguments.IntegerArgumentType integer) {
            result.addProperty("min", integer.getMinimum());
            result.addProperty("max", integer.getMaximum());
        }
        return result;
    }
}