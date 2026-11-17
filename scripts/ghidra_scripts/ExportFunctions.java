// ExportFunctions.java - Ghidra Script to export function list, symbols, strings and cross-references
// Run with: analyzeHeadless <project> <name> -postScript ExportFunctions.java -scriptPath <path>

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import ghidra.program.model.symbol.Symbol;
import ghidra.program.model.symbol.SymbolIterator;
import ghidra.program.model.symbol.SymbolType;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.address.AddressSetView;
import ghidra.program.model.mem.*;
import ghidra.program.model.data.*;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator; 
import ghidra.program.model.symbol.ReferenceManager;
import ghidra.program.model.listing.Data;
import ghidra.program.model.listing.DataIterator; 
import ghidra.util.task.TaskMonitor;
import java.io.File;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.util.Set;
import java.util.HashSet;

public class ExportFunctions extends GhidraScript {

    @Override
    protected void run() throws Exception {
        String projectName = currentProgram.getName();
        File outputDir = new File("H:/Desktop/ArtPlus/rag/ghidra/exports/" + projectName);
        outputDir.mkdirs();
        
        exportFunctions(outputDir);
        exportSymbols(outputDir);
        exportCrossReferences(outputDir);
        exportStrings(outputDir);
        exportCallGraphSimplified(outputDir);
        
        println("Export complete for " + projectName);
    }
    
    private void exportFunctions(File outputDir) throws Exception {
        File outFile = new File(outputDir, "functions.csv");
        try (PrintWriter pw = new PrintWriter(new FileWriter(outFile))) {
            pw.println("Address,Name,ReturnType,ParameterCount");
            
            FunctionIterator funcIter = currentProgram.getFunctionManager().getFunctions(true);
            int count = 0;
            while (funcIter.hasNext() && !monitor.isCancelled()) {
                Function func = funcIter.next();
                pw.println(String.format("%s,%s,%s,%d",
                    func.getEntryPoint(),
                    sanitize(func.getName()),
                    func.getReturnType().getName(),
                    func.getParameterCount()
                ));
                count++;
            }
            println("Exported " + count + " functions to " + outFile.getAbsolutePath());
        }
    }
    
    private void exportSymbols(File outputDir) throws Exception {
        File outFile = new File(outputDir, "symbols.csv");
        try (PrintWriter pw = new PrintWriter(new FileWriter(outFile))) {
            pw.println("Address,Name,Type,ParentNamespace,IsExternal,IsDynamic");
            
            SymbolIterator symIter = currentProgram.getSymbolTable().getAllSymbols(true);
            int count = 0;
            while (symIter.hasNext() && !monitor.isCancelled()) {
                Symbol sym = symIter.next();
                pw.println(String.format("%s,%s,%s,%s,%b,%b",
                    sym.getAddress(),
                    sanitize(sym.getName()),
                    sym.getSymbolType(),
                    sym.getParentNamespace() != null ? sanitize(sym.getParentNamespace().getName()) : "global",
                    sym.isExternal(),
                    sym.isDynamic()
                ));
                count++;
            }
            println("Exported " + count + " symbols to " + outFile.getAbsolutePath());
        }
    }
    
    private void exportCrossReferences(File outputDir) throws Exception {
        File outFile = new File(outputDir, "xrefs.csv");
        try (PrintWriter pw = new PrintWriter(new FileWriter(outFile))) {
            pw.println("FromAddress,ToAddress,FromSymbol,ToSymbol,RefType");
            
            ReferenceManager refManager = currentProgram.getReferenceManager();
            var listing = currentProgram.getListing();
            DataIterator dataIter = listing.getDefinedData(true);
            
            Set<Reference> processedRefs = new HashSet<>(); // To avoid duplicates
            int count = 0;
            while(dataIter.hasNext() && !monitor.isCancelled()) {
                Data data = dataIter.next();
                // Correctly handle Reference[] array
                Reference[] refs = refManager.getReferencesFrom(data.getAddress());
                for (Reference ref : refs) { 
                    if (processedRefs.add(ref)) { // Add only if not already processed
                        String fromSymbol = getSymbolName(ref.getFromAddress());
                        String toSymbol = getSymbolName(ref.getToAddress());
                        
                        pw.println(String.format("%s,%s,%s,%s,%s",
                            ref.getFromAddress(),
                            ref.getToAddress(),
                            sanitize(fromSymbol),
                            sanitize(toSymbol),
                            ref.getReferenceType()
                        ));
                        count++;
                    }
                }
            }
            println("Exported " + count + " cross-references to " + outFile.getAbsolutePath());
        }
    }
    
    private String getSymbolName(ghidra.program.model.address.Address address) {
        Symbol s = currentProgram.getSymbolTable().getPrimarySymbol(address);
        return s != null ? s.getName() : "";
    }

    private void exportStrings(File outputDir) throws Exception {
        File outFile = new File(outputDir, "strings.csv");
        try (PrintWriter pw = new PrintWriter(new FileWriter(outFile))) {
            pw.println("Address,String");
            var listing = currentProgram.getListing();
            DataIterator dataIter = listing.getDefinedData(true); 
            int count = 0;
            while(dataIter.hasNext() && !monitor.isCancelled()) {
                Data data = dataIter.next(); 
                String dtName = data.getDataType().getName();
                if (dtName.contains("string") || dtName.contains("String") || dtName.contains("unicode")) {
                    Object val = data.getValue();
                    if (val != null) {
                        pw.println(String.format("%s,\"%s\"", data.getAddress(), sanitize(val.toString())));
                        count++;
                    }
                }
            }
            println("Exported " + count + " strings to " + outFile.getAbsolutePath());
        }
    }
    
    private void exportCallGraphSimplified(File outputDir) throws Exception {
        File outFile = new File(outputDir, "callgraph_simplified.csv");
        try (PrintWriter pw = new PrintWriter(new FileWriter(outFile))) {
            pw.println("CallerAddress,CallerName,CalleeAddress,CalleeName");
            
            FunctionIterator funcIter = currentProgram.getFunctionManager().getFunctions(true);
            int count = 0;
            while (funcIter.hasNext() && !monitor.isCancelled()) {
                Function caller = funcIter.next();
                Set<Function> callees = caller.getCalledFunctions(monitor);
                for (Function callee : callees) {
                    pw.println(String.format("%s,%s,%s,%s",
                        caller.getEntryPoint(),
                        sanitize(caller.getName()),
                        callee.getEntryPoint(),
                        sanitize(callee.getName())
                    ));
                    count++;
                }
            }
            println("Exported " + count + " call graph entries to " + outFile.getAbsolutePath());
        }
    }

    private String sanitize(String s) {
        if (s == null) return "";
        return s.replace("\"", "\"\"").replace("\n", " ").replace("\r", " ").replace(",", ";");
    }
}
