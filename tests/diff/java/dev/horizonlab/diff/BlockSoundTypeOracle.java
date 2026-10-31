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
import net.minecraft.core.registries.Registries;
import net.minecraft.resources.Identifier;
import net.minecraft.resources.ResourceKey;
import net.minecraft.sounds.SoundEvent;
import net.minecraft.world.level.block.Block;
import net.minecraft.world.level.block.SoundType;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.level.block.state.properties.Property;

/**
 * 方块声音组的原版 oracle（{@code net.minecraft.world.level.block.SoundType}）。
 *
 * <pre>
 *   java dev.horizonlab.diff.BlockSoundTypeOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.BlockSoundTypeOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>输出的五个字段直接来自原版 {@code SoundType} 的访问器，事件名取自
 * {@code BuiltInRegistries.SOUND_EVENT} 的注册键——这正是客户端拿去做声音查找的键，
 * 所以「名字对上」就等于「播的是同一个声音」。
 *
 * <p>单元通常是方块 ID（取其默认状态）。饰纹陶罐的声音组随 {@code CRACKED} 状态切换，
 * 因此另外列举 {@code minecraft:decorated_pot#cracked=true}：走的是原版
 * {@code BlockState#getSoundType()} 这条<b>带状态</b>的路径，与探针用的是同一条入口。
 */
public final class BlockSoundTypeOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private BlockSoundTypeOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (String id : units()) {
                text.append(id).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: BlockSoundTypeOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static List<String> units() {
        // 必须先触发 bootstrap，否则 BuiltInRegistries 尚未填充（静态初始化阶段读它会抛
        // "Not bootstrapped"）。
        LootServerData.get();

        List<String> units = new ArrayList<>();
        for (Identifier id : BuiltInRegistries.BLOCK.keySet()) {
            units.add(id.toString());
        }
        // 状态相关的变体（与探针的 stateVariants() 一一对应）。
        units.add("minecraft:decorated_pot#cracked=true");
        units.sort(String::compareTo);
        return units;
    }

    private static void run(Path input, Path output) throws IOException {
        var blockLookup = LootServerData.get().registries.compositeAccess().lookupOrThrow(Registries.BLOCK);

        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            JsonObject request = JsonParser.parseString(line).getAsJsonObject();
            String unit = request.get("block").getAsString();

            JsonObject result = new JsonObject();
            result.addProperty("block", unit);

            BlockState state = resolveState(blockLookup, unit);
            if (state == null) {
                result.addProperty("ok", false);
                result.addProperty("error", "unknown block");
            } else {
                SoundType sound = state.getSoundType();
                result.addProperty("ok", true);
                result.addProperty("break", name(sound.getBreakSound()));
                result.addProperty("step", name(sound.getStepSound()));
                result.addProperty("place", name(sound.getPlaceSound()));
                result.addProperty("hit", name(sound.getHitSound()));
                result.addProperty("fall", name(sound.getFallSound()));
                result.addProperty("volume", sound.getVolume());
                result.addProperty("pitch", sound.getPitch());
            }
            text.append(GSON.toJson(result)).append('\n');
        }
        Files.writeString(output, text.toString(), StandardCharsets.UTF_8);
    }

    /** 解析单元为方块的某个状态；未知方块返回 {@code null}。 */
    private static BlockState resolveState(
        net.minecraft.core.HolderLookup.RegistryLookup<Block> blockLookup,
        String unit
    ) {
        int hashPos = unit.indexOf('#');
        String blockId = hashPos < 0 ? unit : unit.substring(0, hashPos);

        Block block = blockLookup
            .get(ResourceKey.create(Registries.BLOCK, Identifier.parse(blockId)))
            .map(holder -> holder.value())
            .orElse(null);
        if (block == null) {
            return null;
        }

        BlockState state = block.defaultBlockState();
        if (hashPos >= 0) {
            String query = unit.substring(hashPos + 1);
            for (String pair : query.split(",")) {
                String[] parts = pair.split("=", 2);
                if (parts.length != 2) {
                    continue;
                }
                Property<?> property = block.getStateDefinition().getProperty(parts[0]);
                if (property == null) {
                    // 探针按同一份变体表构造状态，属性对不上说明两侧的变体表不一致，直接判否。
                    return null;
                }
                state = withProperty(state, property, parts[1]);
                if (state == null) {
                    return null;
                }
            }
        }
        return state;
    }

    /** 按字符串值设置一个属性；无法解析时返回 {@code null}。 */
    @SuppressWarnings({"unchecked", "rawtypes"})
    private static BlockState withProperty(BlockState state, Property property, String value) {
        var parsed = property.getValue(value);
        if (parsed.isEmpty()) {
            return null;
        }
        // 类型擦除：属性与取值的类型参数在运行期一致，由 property 自身的解析保证。
        return state.setValue(property, (Comparable) parsed.get());
    }

    /** 声音事件的注册键；未注册（哨兵值）时回退到其 location。 */
    private static String name(SoundEvent sound) {
        Identifier id = BuiltInRegistries.SOUND_EVENT.getKey(sound);
        return id == null ? sound.location().toString() : id.toString();
    }
}