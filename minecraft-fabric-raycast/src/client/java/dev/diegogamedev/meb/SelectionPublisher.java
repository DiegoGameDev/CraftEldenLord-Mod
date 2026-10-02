package dev.diegogamedev.meb;

import java.io.IOException;
import java.nio.file.Path;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;
import org.slf4j.Logger;

final class SelectionPublisher implements AutoCloseable {
    private final AtomicReference<Selection> latest = new AtomicReference<>();
    private final ScheduledExecutorService worker;
    private final AtomicSelectionWriter writer;
    private final Logger logger;
    private long nextRetry;
    private boolean failed;

    SelectionPublisher(Path destination, Logger logger) {
        this.logger = logger;
        this.writer = new AtomicSelectionWriter(destination);
        this.worker = Executors.newSingleThreadScheduledExecutor(task -> {
            Thread thread = new Thread(task, "meb-selection-writer");
            thread.setDaemon(true);
            return thread;
        });
        worker.scheduleWithFixedDelay(this::publish, 0, 50, TimeUnit.MILLISECONDS);
    }

    void offer(Selection selection) {
        latest.set(selection);
    }

    private void publish() {
        if (failed && System.nanoTime() - nextRetry < 0) return;
        try {
            Selection selection = latest.get();
            if (selection == null) return;
            if (writer.writeIfChanged(selection)) logger.debug("Published selection: {}", selection);
            if (failed) logger.info("Shared selection JSON writes recovered.");
            failed = false;
        } catch (IOException | RuntimeException error) {
            if (!failed) logger.warn("Cannot publish selection JSON; retrying every 5 seconds. "
                    + "Check permissions and use a local filesystem supporting atomic replacement.", error);
            failed = true;
            nextRetry = System.nanoTime() + TimeUnit.SECONDS.toNanos(5);
        }
    }

    @Override
    public void close() {
        worker.shutdown();
        try {
            if (!worker.awaitTermination(2, TimeUnit.SECONDS)) worker.shutdownNow();
        } catch (InterruptedException error) {
            worker.shutdownNow();
            Thread.currentThread().interrupt();
        }
    }
}
