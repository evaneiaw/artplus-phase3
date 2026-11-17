import os
import sys
import struct

exe_path = r"C:\Esko\bg_prog_artpro+_v240\ArtPro+.app\Contents\Windows\ArtPro+.exe"

with open(exe_path, "rb") as f:
    f.seek(60)
    pe_offset = struct.unpack("<I", f.read(4))[0]
    
    f.seek(pe_offset)
    pe_signature = f.read(4)
    
    machine, num_sections, time_date_stamp, ptr_symbol_table, num_symbols, size_of_optional_header, characteristics = struct.unpack("<HHIIIHH", f.read(20))
    print(f"Number of Sections: {num_sections}")
    print(f"Size of Optional Header: {size_of_optional_header}")
    
    # Section headers start right after COFF header (20 bytes) + optional header
    section_headers_offset = pe_offset + 4 + 20 + size_of_optional_header
    f.seek(section_headers_offset)
    
    print(f"\n--- Section Headers ---")
    for i in range(num_sections):
        sec_bytes = f.read(40)
        name = sec_bytes[:8].rstrip(b"\x00").decode("latin1", errors="ignore")
        virtual_size, virtual_address, raw_data_size, raw_data_ptr = struct.unpack("<IIII", sec_bytes[8:24])
        print(f"Section {name}: VA=0x{virtual_address:X}, VSize=0x{virtual_size:X}, RawPtr=0x{raw_data_ptr:X}, RawSize=0x{raw_data_size:X}")
