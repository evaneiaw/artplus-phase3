import os
import re
import collections

exe_path = r"C:\Esko\bg_prog_artpro+_v240\ArtPro+.app\Contents\Windows\ArtPro+.exe"

print(f"Scanning strings and imports from {exe_path}...")

with open(exe_path, "rb") as f:
    data = f.read()

# DLL imports scan
dlls = set(re.findall(rb'[a-zA-Z0-9_\-\.]+\.dll', data, re.IGNORECASE))
print("\n--- Imported / Referenced DLLs ---")
for dll in sorted(dlls):
    print(dll.decode('latin1'))

# Interesting keyword search
keywords = [
    "PDF", "ArtPro", "Esko", "Trap", "Screen", "Separation", "Spot", "CMYK",
    "Marks", "Step", "Repeat", "Action", "ActionList", "BarCode", "Bleed",
    "TrimBox", "MediaBox", "CropBox", "BleedBox", "ArtBox", "Studio", "3D",
    "CF2", "ARD", "MFG", "Qt", "Preflight", "Overprint"
]

results = collections.defaultdict(list)

# ASCII strings search (min length 5)
ascii_strings = re.findall(rb'[a-zA-Z0-9_/\-\.\:\;\ \<\>\=\"]{5,}', data)
print(f"\nTotal ASCII strings found: {len(ascii_strings)}")

for s in ascii_strings:
    str_val = s.decode('latin1', errors='ignore')
    for kw in keywords:
        if kw.lower() in str_val.lower():
            results[kw].append(str_val)

print("\n--- Keyword Match Summary ---")
for kw, matches in results.items():
    print(f"Keyword '{kw}': {len(matches)} matches (Sample: {matches[:3]})")
