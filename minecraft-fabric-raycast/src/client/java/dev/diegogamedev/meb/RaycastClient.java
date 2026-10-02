package dev.diegogamedev.meb;

import java.nio.file.Path;
import net.fabricmc.api.ClientModInitializer;
import net.fabricmc.fabric.api.client.event.lifecycle.v1.ClientLifecycleEvents;
import net.fabricmc.fabric.api.client.event.lifecycle.v1.ClientTickEvents;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.world.phys.BlockHitResult;
import net.minecraft.world.phys.HitResult;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public final class RaycastClient implements ClientModInitializer {
    private static final Logger LOGGER = LoggerFactory.getLogger("MinecraftEldenBridge");
    private static final String DEFAULT_SELECTION = "D:/Elden Ring Mod Project IA/runtime-state/minecraft_selection.json";

    @Override
    public void onInitializeClient() {
        String configured = System.getProperty("meb.selectionJson");
        if (configured == null || configured.isBlank()) configured = System.getenv("MEB_SELECTION_JSON");
        if (configured == null || configured.isBlank()) configured = DEFAULT_SELECTION;
        final Path destination;
        try {
            destination = Path.of(configured);
            if (!destination.isAbsolute()) throw new IllegalArgumentException("Selection path must be absolute.");
        } catch (RuntimeException error) {
            LOGGER.error("Invalid selection JSON path; bridge disabled: {}", configured, error);
            return;
        }
        SelectionPublisher publisher = new SelectionPublisher(destination, LOGGER);
        LOGGER.info("MinecraftEldenBridge raycast loaded. Selection file: {}", destination);
        ClientLifecycleEvents.CLIENT_STOPPING.register(client -> publisher.close());
        ClientTickEvents.END_CLIENT_TICK.register(client -> {
            if (client.player == null || client.level == null || client.getCameraEntity() == null) {
                publisher.offer(null);
                return;
            }
            // Vanilla 26.3 targeting, refreshed at tick end even when no frame was rendered.
            HitResult hit = client.player.raycastHitResult(1.0F, client.getCameraEntity());
            if (hit instanceof BlockHitResult block && hit.getType() == HitResult.Type.BLOCK) {
                var position = block.getBlockPos();
                var state = client.level.getBlockState(position);
                publisher.offer(new Selection(BuiltInRegistries.BLOCK.getKey(state.getBlock()).toString(),
                        client.level.dimension().identifier().toString(),
                        position.getX(), position.getY(), position.getZ()));
            } else {
                publisher.offer(null);
            }
        });
    }
}
