package dev.diegogamedev.meb;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.AccessDeniedException;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.util.concurrent.TimeUnit;

/** One writer owns the .tmp file; do not run two publishers for the same destination. */
public final class AtomicSelectionWriter {
    private final Path destination;
    private final Path temporary;
    private Selection lastWritten;

    public AtomicSelectionWriter(Path destination) {
        this.destination = destination.toAbsolutePath().normalize();
        this.temporary = this.destination.resolveSibling(this.destination.getFileName() + ".tmp");
    }

    public synchronized boolean writeIfChanged(Selection selection) throws IOException {
        if (selection == null || selection.equals(lastWritten)) return false;
        Files.createDirectories(destination.getParent());
        Files.writeString(temporary, selection.toJson(), StandardCharsets.UTF_8);
        // No non-atomic fallback: preserve the previous complete JSON if replacement fails.
        for (int attempt = 0; ; attempt++) {
            try {
                Files.move(temporary, destination, StandardCopyOption.ATOMIC_MOVE, StandardCopyOption.REPLACE_EXISTING);
                break;
            } catch (AccessDeniedException error) {
                // Windows readers may briefly hold a handle without FILE_SHARE_DELETE.
                if (attempt >= 7) throw error;
                try {
                    TimeUnit.MILLISECONDS.sleep(10);
                } catch (InterruptedException interrupted) {
                    Thread.currentThread().interrupt();
                    throw new IOException("Atomic selection replacement interrupted.", interrupted);
                }
            }
        }
        lastWritten = selection;
        return true;
    }
}
