package dev.diegogamedev.meb;

import com.google.gson.JsonObject;

public record Selection(String minecraftId, String dimension, int x, int y, int z) {
    public String toJson() {
        JsonObject position = new JsonObject();
        position.addProperty("x", x);
        position.addProperty("y", y);
        position.addProperty("z", z);
        JsonObject root = new JsonObject();
        root.addProperty("schema_version", 1);
        root.addProperty("minecraft_id", minecraftId);
        root.addProperty("dimension", dimension);
        root.add("position", position);
        return root.toString() + "\n";
    }
}
