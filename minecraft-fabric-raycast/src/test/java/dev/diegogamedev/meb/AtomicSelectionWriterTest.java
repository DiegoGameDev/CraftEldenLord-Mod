package dev.diegogamedev.meb;

import com.google.gson.JsonParser;
import java.io.IOException;
import java.io.FileInputStream;
import java.nio.file.AccessDeniedException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.attribute.FileTime;
import java.util.concurrent.Executors;
import java.util.concurrent.atomic.AtomicBoolean;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;
import static org.junit.jupiter.api.Assertions.*;
import static org.junit.jupiter.api.Assumptions.assumeTrue;

class AtomicSelectionWriterTest {
    @TempDir Path directory;
    private static final Selection STONE = new Selection("minecraft:stone", "minecraft:overworld", 128, 64, -32);

    @Test void writesExactContractAndSkipsUnchangedOrMissingTarget() throws Exception {
        Path target = directory.resolve("nested/minecraft_selection.json");
        AtomicSelectionWriter writer = new AtomicSelectionWriter(target);
        assertFalse(writer.writeIfChanged(null));
        assertFalse(Files.exists(target));
        assertTrue(writer.writeIfChanged(STONE));
        var json = JsonParser.parseString(Files.readString(target)).getAsJsonObject();
        assertEquals(4, json.size());
        assertEquals(1, json.get("schema_version").getAsInt());
        assertEquals("minecraft:stone", json.get("minecraft_id").getAsString());
        assertEquals("minecraft:overworld", json.get("dimension").getAsString());
        assertEquals(3, json.getAsJsonObject("position").size());
        assertEquals(128, json.getAsJsonObject("position").get("x").getAsInt());
        assertEquals(64, json.getAsJsonObject("position").get("y").getAsInt());
        assertEquals(-32, json.getAsJsonObject("position").get("z").getAsInt());
        FileTime marker = FileTime.fromMillis(123456000L);
        Files.setLastModifiedTime(target, marker);
        assertFalse(writer.writeIfChanged(new Selection("minecraft:stone", "minecraft:overworld", 128, 64, -32)));
        assertFalse(writer.writeIfChanged(null));
        assertEquals(marker, Files.getLastModifiedTime(target));
        assertFalse(Files.exists(target.resolveSibling("minecraft_selection.json.tmp")));
    }

    @Test void eachRelevantChangeReplacesExistingFile() throws Exception {
        Path target = directory.resolve("minecraft_selection.json");
        AtomicSelectionWriter writer = new AtomicSelectionWriter(target);
        for (Selection selection : new Selection[] {STONE,
                new Selection("minecraft:diamond_block", "minecraft:overworld", 128, 64, -32),
                new Selection("minecraft:diamond_block", "minecraft:the_nether", 128, 64, -32),
                new Selection("minecraft:diamond_block", "minecraft:the_nether", -20, 65, 18)}) {
            assertTrue(writer.writeIfChanged(selection));
            assertEquals(selection.toJson(), Files.readString(target));
        }
    }

    @Test void failedReplacementPreservesOldJsonAndCanRetrySameSelection() throws Exception {
        Path target = directory.resolve("minecraft_selection.json");
        AtomicSelectionWriter writer = new AtomicSelectionWriter(target);
        writer.writeIfChanged(STONE);
        Path temporary = target.resolveSibling("minecraft_selection.json.tmp");
        Files.createDirectory(temporary);
        Selection changed = new Selection("minecraft:dirt", "minecraft:overworld", 1, 2, 3);
        assertThrows(IOException.class, () -> writer.writeIfChanged(changed));
        assertEquals(STONE.toJson(), Files.readString(target));
        Files.delete(temporary);
        assertTrue(writer.writeIfChanged(changed));
        assertEquals(changed.toJson(), Files.readString(target));
    }

    @Test void readerSeesCompleteSnapshotsDuringAtomicReplacements() throws Exception {
        Path target = directory.resolve("minecraft_selection.json");
        AtomicSelectionWriter writer = new AtomicSelectionWriter(target);
        writer.writeIfChanged(STONE);
        AtomicBoolean finished = new AtomicBoolean();
        try (var executor = Executors.newSingleThreadExecutor()) {
            var reader = executor.submit(() -> {
                int reads = 0;
                while (!finished.get() || reads < 100) {
                    try {
                        var json = JsonParser.parseString(Files.readString(target)).getAsJsonObject();
                        assertEquals(4, json.size());
                        assertEquals(1, json.get("schema_version").getAsInt());
                        assertTrue(json.getAsJsonObject("position").has("z"));
                        reads++;
                    } catch (IOException error) {
                        throw new AssertionError(error);
                    }
                    // The real hook polls every 333 ms; keep this reader much more aggressive.
                    Thread.sleep(1);
                }
                return reads;
            });
            try {
                for (int x = 0; x < 100; x++) {
                    writer.writeIfChanged(new Selection("minecraft:stone", "minecraft:overworld", x, 64, -32));
                }
            } finally {
                finished.set(true);
            }
            assertTrue(reader.get() >= 100);
        }
    }

    @Test void windowsLockedDestinationPreservesOldJsonAndRetriesAfterRelease() throws Exception {
        assumeTrue(System.getProperty("os.name").startsWith("Windows"));
        Path target = directory.resolve("minecraft_selection.json");
        AtomicSelectionWriter writer = new AtomicSelectionWriter(target);
        writer.writeIfChanged(STONE);
        Selection changed = new Selection("minecraft:dirt", "minecraft:overworld", 1, 2, 3);
        try (var lockedReader = new FileInputStream(target.toFile())) {
            assertTrue(lockedReader.read() >= 0);
            assertThrows(AccessDeniedException.class, () -> writer.writeIfChanged(changed));
            assertEquals(STONE.toJson(), Files.readString(target));
        }
        assertTrue(writer.writeIfChanged(changed));
        assertEquals(changed.toJson(), Files.readString(target));
    }

    @Test void exportsRealJavaWriterFixtureForCppSmokeTest() throws Exception {
        Path fixture = Path.of(System.getProperty("meb.testOutput"), "minecraft_selection.json");
        assertTrue(new AtomicSelectionWriter(fixture).writeIfChanged(STONE));
        assertEquals(STONE.toJson(), Files.readString(fixture));
    }
}
