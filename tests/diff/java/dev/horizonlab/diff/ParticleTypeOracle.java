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
import java.util.List;
import net.minecraft.SharedConstants;
import net.minecraft.core.particles.ParticleType;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.server.Bootstrap;

/**
 * 粒子类型协议 ID 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.ParticleTypeOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>按 {@code BuiltInRegistries.PARTICLE_TYPE} 的迭代顺序（即协议 ID 顺序）枚举
 * 注册表 ID，不硬编码列表。
 *
 * <p>单元表必须与 probes/particle_type.cpp 的 VANILLA_PARTICLES 逐项一致。
 */
public final class ParticleTypeOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private ParticleTypeOracle() {}

    /** 原版粒子类型注册表 ID，按协议 ID 升序。 */
    private static List<String> collect() {
        List<String> out = new ArrayList<>();
        for (ParticleType<?> type : BuiltInRegistries.PARTICLE_TYPE) {
            out.add(BuiltInRegistries.PARTICLE_TYPE.getKey(type).getPath());
        }
        return out;
    }

    public static void main(String[] args) throws IOException {
        SharedConstants.tryDetectVersion();
        Bootstrap.bootStrap();
        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            List<String> all = collect();
            for (int i = 0; i < all.size(); i++) {
                text.append(i).append('|').append(all.get(i)).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: ParticleTypeOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        List<String> all = collect();
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String unit = JsonParser.parseString(line).getAsJsonObject().get("unit").getAsString();
            int bar = unit.indexOf('|');
            int id = Integer.parseInt(unit.substring(0, bar));
            String name = (id >= 0 && id < all.size()) ? all.get(id) : "OUT_OF_RANGE";
            JsonObject row = new JsonObject();
            row.addProperty("result", name);
            row.addProperty("unit", unit);
            text.append(GSON.toJson(row)).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }
}