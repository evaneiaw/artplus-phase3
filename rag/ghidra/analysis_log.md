# Análise Ghidra Headless - Log Bruto (Trecho)

```
INFO  IMPORTING: Loaded 0 additional files (HeadlessAnalyzer)
INFO  ANALYZING all memory and code: file:///H:/Desktop/ArtPlus/tools/ghidra_projects/ArtProPlus.exe (HeadlessAnalyzer)
WARN  Ignoring leading '_' chars on no-return name '___raise_securityfailure' specified in file: C:\tools\ghidra_12.2_PUBLIC\Ghidra\Features\Base\data\PEFunctionsThatDoNotReturn (NoReturnFunctionAnalyzer)
WARN  Ignoring leading '_' chars on no-return name '___report_rangecheckfailure' specified in file: C:\tools\ghidra_12.2_PUBLIC\Ghidra\Features\Base\data\PEFunctionsThatDoNotReturn (NoReturnFunctionAnalyzer)
INFO  Unable to find DWARF information, skipping DWARF analysis (DWARFAnalyzer)
INFO  Skipping PDB processing: missing PDB information in program metadata (PdbUniversalAnalyzer)
WARN  WARNING! Repairing body of function at 140f36350 (ClearFlowAndRepairCmd)
INFO  hit non-returning function, restarting decompiler switch analyzer later (DecompilerSwitchAnalyzer)
...
[Centenares de avisos de descompilação: "Unable to read bytes at ram:0x20CCCCCC"]
...
```

**Interpretação**: O padrão `0xCCCCCCCC` é fill de memória não inicializada (Debug MSVC). Confirma que o binário retém características de build não-stripped.