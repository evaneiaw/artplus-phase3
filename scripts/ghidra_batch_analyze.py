import os
import subprocess
import sys

GHIDRA_PATH = r"C:\tools\ghidra_12.2_PUBLIC\support\analyzeHeadless.bat"
PROJECT_DIR = r"H:\Desktop\ArtPlus\tools\ghidra_projects"
ESKO_DLL_DIR = r"C:\Esko\bg_prog_artpro+_v240\ArtPro+.app\Contents\Windows"

# Key DLLs mapped to functional areas
TARGET_DLLS = {
    "pdf_document": [
        "Document.dll",
        "PDFConversionRules.dll",
        "PDFObjectLayer.dll",
        "PDFRenderingLayer.dll",
        "PDFAccessLayer.dll",
        "PDFObjectActions.dll",
        "PDFObjectViewLayer.dll",
    ],
    "geometry_paths": [
        "Cornering.dll",
        "CurveApprox.dll",
        "GeoMapper.dll",
        "Snapping.dll",
        "CCurve.dll",
        "Offset.dll",
    ],
    "separations_color": [
        "DocumentEditSeparations.dll",
        "DocumentInkEater.dll",
        "CMS.dll",
        "GMGOpenColorModel.dll",
        "ICC.dll",
        "lcms.dll",
        "GMGOpenColor.dll",
    ],
    "trapping": [
        "DocumentTrap.dll",
        "DocumentFlexoTools.dll",
    ],
    "screening": [
        "ScreenDataDLL.dll",
    ],
    "marks": [
        "DocumentMarks.dll",
        "DocumentBarCode.dll",
        "BarCodeLib.dll",
        "BarCodeWrapper.dll",
        "ArtProBarcodes.dll",
    ],
    "step_repeat": [
        "StepAndRepeat.dll",
        "StepAndRepeatCAD.dll",
        "LayoutEngine.dll",
    ],
    "preflight": [
        "PreflightProfileLayer.dll",
        "PreflightProfileViewLayer.dll",
        "CertifiedPDFPreflightLayer.dll",
    ],
    "action_lists": [
        "NDLModelActionList.dll",
        "NDLModelActionListTypes.dll",
        "DocumentAutomationEngine.dll",
        "NDLModel.dll",
        "NDLTypes.dll",
    ],
    "packaging_structures": [
        "Pack2D.dll",
        "Pack3D.dll",
        "StructuralDesignImportProfile.dll",
        "DocumentStructuralDesign.dll",
    ],
    "studio_3d": [
        "StudioDesignerCoreController.dll",
        "StudioDesignerCoreRenderEngine.dll",
        "StudioCommon.dll",
        "Pack3D.dll",
        "StudioGLTF.dll",
        "StudioRendererBase.dll",
        "StudioRendererLib3d.dll",
    ],
}

def run_ghidra_headless(project_name, dll_name, dll_path, script_name=None):
    """Run Ghidra headless analysis on a single DLL."""
    cmd = [
        GHIDRA_PATH,
        PROJECT_DIR,
        project_name,
        "-import", dll_path,
        "-overwrite",
        "-noanalysis" if script_name else "",
    ]
    
    if script_name:
        script_path = os.path.join(os.path.dirname(__file__), "ghidra_scripts", script_name)
        cmd.extend(["-postScript", script_name, "-scriptPath", os.path.join(os.path.dirname(__file__), "ghidra_scripts")])
    
    # Remove empty strings
    cmd = [c for c in cmd if c]
    
    print(f"Running: {' '.join(cmd)}")
    result = subprocess.run(cmd, capture_output=True, text=True, timeout=300)
    return result

def create_projects_and_analyze():
    """Create separate Ghidra projects for each functional area."""
    os.makedirs(os.path.join(os.path.dirname(__file__), "ghidra_scripts"), exist_ok=True)
    
    for area, dlls in TARGET_DLLS.items():
        project_name = f"ArtProPlus_{area}"
        print(f"\n=== Creating project: {project_name} ===")
        
        for dll in dlls:
            dll_path = os.path.join(ESKO_DLL_DIR, dll)
            if not os.path.exists(dll_path):
                print(f"  SKIP: {dll} not found")
                continue
            
            print(f"  Analyzing: {dll} ({os.path.getsize(dll_path)} bytes)")
            result = run_ghidra_headless(project_name, dll, dll_path, "ExportFunctions.java")
            
            if result.returncode != 0:
                print(f"    ERROR: {result.stderr[:500]}")
            else:
                print(f"    OK")

if __name__ == "__main__":
    create_projects_and_analyze()