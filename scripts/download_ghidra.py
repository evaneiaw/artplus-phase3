import urllib.request
import os
import sys

url = "https://github.com/NationalSecurityAgency/ghidra/releases/download/Ghidra_12.2_build/ghidra_12.2_PUBLIC_20261016.zip"
output_path = r"C:\tools\ghidra.zip"

print(f"Downloading Ghidra from {url}...")
print(f"Output: {output_path}")

def download_with_progress(url, filepath):
    os.makedirs(os.path.dirname(filepath), exist_ok=True)
    
    response = urllib.request.urlopen(url)
    total_size = int(response.headers.get('Content-Length', 0))
    block_size = 8192
    downloaded = 0
    
    with open(filepath, 'wb') as f:
        while True:
            buffer = response.read(block_size)
            if not buffer:
                break
            f.write(buffer)
            downloaded += len(buffer)
            if total_size > 0:
                percent = downloaded * 100 / total_size
                print(f"\rProgress: {downloaded}/{total_size} bytes ({percent:.1f}%)", end='', flush=True)
    
    print()
    return downloaded

try:
    size = download_with_progress(url, output_path)
    print(f"Download complete: {size} bytes ({size/(1024*1024):.1f} MB)")
except Exception as e:
    print(f"Download failed: {e}")
    sys.exit(1)