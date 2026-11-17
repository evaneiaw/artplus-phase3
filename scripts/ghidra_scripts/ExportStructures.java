// ExportStructures.java - Ghidra Script
import ghidra.app.script.GhidraScript;
import ghidra.program.model.data.*;
import java.io.*;

public class ExportStructures extends GhidraScript {
    @Override
    protected void run() throws Exception {
        String projectName = currentProgram.getName();
        File outputDir = new File("H:/Desktop/ArtPlus/rag/ghidra/exports/" + projectName);
        outputDir.mkdirs();
        
        File outFile = new File(outputDir, "datatypes.csv");
        try (PrintWriter pw = new PrintWriter(new FileWriter(outFile))) {
            pw.println("Name,Size,CategoryPath");
            DataTypeManager dtm = currentProgram.getDataTypeManager();
            var iter = dtm.getAllDataTypes();
            while (iter.hasNext() && !monitor.isCancelled()) {
                DataType dt = iter.next();
                pw.println(String.format("%s,%d,%s", dt.getName(), dt.getLength(), dt.getCategoryPath()));
            }
        }
    }
}