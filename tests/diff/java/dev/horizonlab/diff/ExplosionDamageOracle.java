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
 * 爆炸对实体伤害与击退的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.ExplosionDamageOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>公式直接照抄 {@code ExplosionDamageCalculator.getEntityDamageAmount} 与
 * {@code ServerExplosion.hurtEntities} 的循环体，不做任何「改良」——本能力要钉住的
 * 正是这两个地方的口径：伤害公式里的距离比例<b>不钳制</b>，返回值<b>不取整</b>。
 */
public final class ExplosionDamageOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 与 probes/explosion_damage.cpp 的常量保持一致。 */
    private static final float DAMAGE_MULTIPLIER = 7.0F;

    private ExplosionDamageOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            String[] radii = {"1", "2", "3", "4", "6", "8"};
            String[] distances = {"0", "0.5", "1", "2", "3.5", "5", "8", "16"};
            String[] seen = {"0", "0.25", "0.5", "0.75", "1"};
            String[] resistance = {"0", "0.5"};

            StringBuilder text = new StringBuilder();
            for (String radius : radii) {
                for (String distance : distances) {
                    for (String s : seen) {
                        for (String r : resistance) {
                            text.append(radius).append('|').append(distance).append('|').append(s).append('|').append(r).append('\n');
                        }
                    }
                }
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: ExplosionDamageOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            var obj = JsonParser.parseString(line).getAsJsonObject();
            double radius = obj.get("radius").getAsDouble();
            double distance = obj.get("distance").getAsDouble();
            double seenPercent = obj.get("seenPercent").getAsDouble();
            double knockbackResistance = obj.get("knockbackResistance").getAsDouble();
            text.append(GSON.toJson(summarise(obj, radius, distance, seenPercent, knockbackResistance)))
                    .append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject summarise(
            JsonObject input, double radius, double distance, double seenPercent, double knockbackResistance) {
        JsonObject result = new JsonObject();

        // 原版：float f = radius * 2.0F; double d0 = sqrt(distSq) / f;
        double damageRadius = (float) radius * 2.0F;
        double ratio = distance / damageRadius;

        // 调用方：if (!(d0 > 1.0)) 才处理该实体。
        boolean inRange = !(ratio > 1.0);

        // 伤害：d0 不钳制，返回值不取整。
        double impact = (1.0 - ratio) * seenPercent;
        double damage = (impact * impact + impact) / 2.0 * DAMAGE_MULTIPLIER * damageRadius + 1.0;

        // 击退：f1 = 1.0（默认击退倍率），f2 = seenPercent。
        double knockbackMultiplier = 1.0;
        double knockback = (1.0 - ratio) * seenPercent * knockbackMultiplier * (1.0 - knockbackResistance);

        result.addProperty("radius", input.get("radius").getAsString());
        result.addProperty("distance", input.get("distance").getAsString());
        result.addProperty("seenPercent", input.get("seenPercent").getAsString());
        result.addProperty("knockbackResistance", input.get("knockbackResistance").getAsString());
        result.addProperty("damageRadius", formatFloat(damageRadius));
        result.addProperty("ratio", formatFloat(ratio));
        result.addProperty("inRange", inRange);
        result.addProperty("damage", formatFloat(damage));
        result.addProperty("knockback", formatFloat((float) knockback));
        return result;
    }

    private static String formatFloat(double value) {
        String text = String.format("%.6g", value);
        if (text.indexOf('.') >= 0 && text.indexOf('e') < 0 && text.indexOf('E') < 0) {
            while (text.endsWith("0")) {
                text = text.substring(0, text.length() - 1);
            }
            if (text.endsWith(".")) {
                text = text.substring(0, text.length() - 1);
            }
        }
        return text;
    }
}