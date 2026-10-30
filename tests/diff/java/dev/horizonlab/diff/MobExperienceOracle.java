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
import net.minecraft.world.entity.EntitySpawnReason;
import net.minecraft.world.entity.EntityType;
import net.minecraft.world.entity.Mob;

/**
 * 生物死亡经验的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.MobExperienceOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.MobExperienceOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/mob_experience.cpp 同构：用原版工厂把每种生物造出来，读构造函数里设好的
 * {@code xpReward}（vanilla Mob.java:123）。装备加成按「身上没有装备」计算，恒为 0。
 *
 * <p>为什么不用 {@code Mob#getBaseExperienceReward}：它需要真实伤害事件（掠夺附魔），
 * 而本能力只比「裸生物」的固定值。构造实体则必须有一个能通过 {@code EntityType#create}
 * 的 {@code Level}，直接用 {@link LootLevel} 建好的最小服务端世界。
 */
public final class MobExperienceOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /**
     * 原版经验值字段 {@code Mob#xpReward}：protected，且只有 {@code Mob#getBaseExperienceReward}
     * 能读到，而后者还要求一个真实伤害事件（掠夺附魔加成）。反射读一次比在子类里开洞更省事，
     * 也不会因为原版改字段名而静默给出错误值——拿不到时按「原版侧没有数据」处理。
     */
    private static final java.lang.reflect.Field XP_REWARD = xpRewardField();

    private MobExperienceOracle() {}

    private static java.lang.reflect.Field xpRewardField() {
        try {
            var field = Mob.class.getDeclaredField("xpReward");
            field.setAccessible(true);
            return field;
        } catch (ReflectiveOperationException | RuntimeException ignored) {
            return null;
        }
    }

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> names = new ArrayList<>();
            for (Identifier id : BuiltInRegistries.ENTITY_TYPE.keySet()) {
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
        System.err.println("usage: MobExperienceOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String name = JsonParser.parseString(line).getAsJsonObject().get("entity").getAsString();
            text.append(GSON.toJson(summarise(name))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject summarise(String entityId) {
        JsonObject result = new JsonObject();
        result.addProperty("entity", entityId);

        EntityType<?> type =
                BuiltInRegistries.ENTITY_TYPE.getOptional(Identifier.parse(entityId)).orElse(null);
        if (type == null) {
            result.addProperty("missing", true);
            return result;
        }

        if (XP_REWARD == null) {
            result.addProperty("missing", true);
            return result;
        }
        int reward;
        try {
            // COMMAND 原因：不触发任何生成期随机化（怪物装备、羊颜色等都在 finalizeSpawn 里），
            // 经验值只由构造函数决定，两侧因此看到的是同一个固定值。
            var entity = type.create(LootLevel.get().level, EntitySpawnReason.COMMAND);
            if (!(entity instanceof Mob mob)) {
                // 非生物实体没有死亡经验（投射物、掉落物、展示实体……）。
                result.addProperty("missing", true);
                return result;
            }
            reward = XP_REWARD.getInt(mob);
        } catch (ReflectiveOperationException | RuntimeException error) {
            // 个别生物的构造函数依赖真实世界状态而在这里构造不出来：记为「原版侧没有数据」，
            // 由 harness 的哨兵清单登记，而不是让整个 oracle 挂掉。
            result.addProperty("missing", true);
            return result;
        }
        result.addProperty("experienceValue", reward);
        result.addProperty("equipmentBonus", 0);
        return result;
    }
}