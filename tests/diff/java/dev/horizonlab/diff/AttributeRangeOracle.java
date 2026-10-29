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
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.resources.Identifier;
import net.minecraft.world.entity.ai.attributes.Attribute;
import net.minecraft.world.entity.ai.attributes.RangedAttribute;

/**
 * 属性定义的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.AttributeRangeOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.AttributeRangeOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/attribute_range.cpp 同构：属性注册名、默认值与取值上下限，数值按位输出。
 *
 * <p>属性表是静态注册表（35 项），不依赖数据包；但默认值、上下限都是 {@code RangedAttribute}
 * 构造参数，只能在 bootstrap 之后读到，因此不走 DataPackRegistries。
 *
 * <p>原版有、项目没有的属性不列进 {@code list}：单元集合由探针决定（项目自己的注册表），
 * 缺哪些属性由 {@code JavaAttributeIdMap} 的映射表反映。这样「项目多出来的自研属性」
 * （{@code forge.entity_gravity} 等）也能被察觉。
 */
public final class AttributeRangeOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private AttributeRangeOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> names = new ArrayList<>();
            for (Identifier id : BuiltInRegistries.ATTRIBUTE.keySet()) {
                names.add(id.toString());
            }
            names.sort(String::compareTo);
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
        System.err.println("usage: AttributeRangeOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String name = JsonParser.parseString(line).getAsJsonObject().get("attribute").getAsString();
            text.append(GSON.toJson(summarise(name))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject summarise(String name) {
        JsonObject result = new JsonObject();
        result.addProperty("attribute", name);

        Attribute attribute = BuiltInRegistries.ATTRIBUTE.getOptional(Identifier.parse(name)).orElse(null);
        if (attribute == null) {
            result.addProperty("missing", true);
            return result;
        }

        result.addProperty("defaultValue", doubleBits(attribute.getDefaultValue()));
        if (attribute instanceof RangedAttribute ranged) {
            result.addProperty("minValue", doubleBits(ranged.getMinValue()));
            result.addProperty("maxValue", doubleBits(ranged.getMaxValue()));
        } else {
            // 非区间属性（原版目前没有实例）没有上下限，按 ±∞ 归一，与项目侧「不钳制」一致。
            result.addProperty("minValue", doubleBits(Double.NEGATIVE_INFINITY));
            result.addProperty("maxValue", doubleBits(Double.POSITIVE_INFINITY));
        }
        return result;
    }

    private static String doubleBits(double value) {
        return String.format("0x%016x", Double.doubleToRawLongBits(value));
    }
}