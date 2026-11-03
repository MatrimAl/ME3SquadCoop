// Export a caller-selected offline feature from an already analyzed program.
// Source bytes are never modified. Results belong outside the repository.
// @category ME3SquadCoop.Offline
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import com.google.gson.*;
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.framework.Application;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;

public class ExportFeature extends GhidraScript {
    private final Gson gson = new GsonBuilder().setPrettyPrinting().create();
    private void save(Path path, JsonObject value) throws Exception {
        Path staged = path.resolveSibling(path.getFileName() + ".partial");
        Files.writeString(staged, gson.toJson(value) + "\n", StandardCharsets.UTF_8);
        Files.move(staged, path, StandardCopyOption.REPLACE_EXISTING);
    }
    @Override public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 2) throw new IllegalArgumentException("manifest and output directory required");
        JsonObject manifest = JsonParser.parseString(Files.readString(Path.of(args[0]))).getAsJsonObject();
        String digest = currentProgram.getExecutableSHA256();
        if (!manifest.get("sha256").getAsString().equalsIgnoreCase(digest))
            throw new IllegalStateException("Imported program digest differs from manifest");
        Path output = Path.of(args[1]);
        Files.createDirectories(output);
        JsonObject report = new JsonObject();
        report.addProperty("kind", "ghidra-offline-feature-export");
        report.addProperty("provider_version", Application.getApplicationVersion());
        report.addProperty("sha256", digest);
        report.addProperty("program", currentProgram.getName());
        report.addProperty("image_base", currentProgram.getImageBase().toString());
        report.addProperty("game_executed", false);
        report.addProperty("tests_run", false);
        report.addProperty("status", "exporting");
        JsonArray rows = new JsonArray();
        report.add("functions", rows);
        JsonArray limits = new JsonArray();
        limits.add("Pseudocode is provider output, not original source or ABI proof.");
        limits.add("Analyzed function bodies can omit paths; recorded ranges do not prove semantic completeness.");
        limits.add("Unresolved indirect calls remain unresolved; no-return analysis assumptions are not repaired.");
        limits.add("Static output does not establish gameplay or rendering behavior.");
        report.add("limits", limits);
        DecompInterface decompiler = new DecompInterface();
        try {
            if (!decompiler.openProgram(currentProgram))
                throw new IllegalStateException("Decompiler could not open analyzed program");
            for (JsonElement item : manifest.getAsJsonArray("functions")) {
                monitor.checkCancelled();
                JsonObject target = item.getAsJsonObject();
                String role = target.get("role").getAsString();
                if (!role.matches("[a-z][a-z0-9_-]{0,79}"))
                    throw new IllegalArgumentException("Invalid role name");
                long rva = Long.decode(target.get("rva").getAsString());
                Address address = currentProgram.getImageBase().add(rva);
                JsonObject row = new JsonObject();
                row.addProperty("role", role);
                row.addProperty("requested_rva", "0x" + Long.toHexString(rva));
                row.addProperty("requested_address", address.toString());
                Function function = currentProgram.getFunctionManager().getFunctionAt(address);
                if (function == null) {
                    row.addProperty("status", "no-function-at-exact-entry");
                } else {
                    row.addProperty("entry", function.getEntryPoint().toString());
                    row.addProperty("name", function.getName());
                    row.addProperty("no_return", function.hasNoReturn());
                    JsonArray ranges = new JsonArray();
                    for (AddressRange range : function.getBody().getAddressRanges()) {
                        JsonObject bounds = new JsonObject();
                        bounds.addProperty("start", range.getMinAddress().toString());
                        bounds.addProperty("end_inclusive", range.getMaxAddress().toString());
                        ranges.add(bounds);
                    }
                    row.add("analyzed_body_ranges", ranges);
                    JsonArray calls = new JsonArray();
                    for (Instruction instruction : currentProgram.getListing().getInstructions(function.getBody(), true)) {
                        if (!instruction.getFlowType().isCall()) continue;
                        JsonObject call = new JsonObject();
                        call.addProperty("site", instruction.getAddress().toString());
                        call.addProperty("instruction", instruction.toString());
                        JsonArray flows = new JsonArray();
                        for (Address flow : instruction.getFlows()) flows.add(flow.toString());
                        call.add("analyzed_targets", flows);
                        call.addProperty("unresolved", flows.isEmpty());
                        calls.add(call);
                    }
                    row.add("calls", calls);
                    // Zero disables the decompiler's per-function deadline.
                    // Headless cancellation remains available to the operator.
                    DecompileResults result = decompiler.decompileFunction(function, 0, monitor);
                    boolean completed = result.decompileCompleted() && result.getDecompiledFunction() != null;
                    row.addProperty("status", completed ? "decompiled" : "decompilation-incomplete");
                    row.addProperty("diagnostic", result.getErrorMessage());
                    if (completed) row.addProperty("pseudocode", result.getDecompiledFunction().getC());
                }
                rows.add(row);
                save(output.resolve(role + ".json"), row);
                save(output.resolve("feature.json"), report);
                println("FEATURE_EXPORT " + role + " " + row.get("status").getAsString());
            }
            report.addProperty("status", "export-complete");
            save(output.resolve("feature.json"), report);
        } finally {
            decompiler.dispose();
        }
    }
}
