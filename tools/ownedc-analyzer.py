#!/usr/bin/env python3
import sys
import re
import argparse

def analyze_file(filepath):
    warnings = 0
    errors = 0
    
    # Regex patterns
    raw_malloc_re = re.compile(r'\b(malloc|calloc|realloc)\s*\(')
    raw_free_re = re.compile(r'\b(free)\s*\(')
    owner_alloc_re = re.compile(r'\b(owner_malloc|owner_calloc|owner_realloc)\b')
    owned_macro_re = re.compile(r'\bOWNED\b')
    
    try:
        with open(filepath, 'r') as f:
            for line_no, line in enumerate(f, start=1):
                # Ignore single-line comments for simplicity in this MVP
                clean_line = line.split('//')[0]
                
                # Rule 1: Detect raw memory allocation
                match = raw_malloc_re.search(clean_line)
                if match:
                    func_name = match.group(1)
                    col = match.start() + 1
                    print(f"{filepath}:{line_no}:{col}: warning: use of raw '{func_name}' detected. Use 'owner_{func_name}' instead for memory safety. [-Wownedc-raw-memory]")
                    warnings += 1
                
                # Rule 1b: Detect raw free
                match = raw_free_re.search(clean_line)
                if match:
                    func_name = match.group(1)
                    col = match.start() + 1
                    print(f"{filepath}:{line_no}:{col}: error: use of raw '{func_name}' detected. Use 'owner_free' instead to prevent double-free/use-after-free. [-Wownedc-raw-free]")
                    errors += 1
                    
                # Rule 2: Detect owner_malloc without OWNED macro
                match = owner_alloc_re.search(clean_line)
                if match:
                    func_name = match.group(1)
                    if not owned_macro_re.search(clean_line):
                        col = match.start() + 1
                        print(f"{filepath}:{line_no}:{col}: warning: '{func_name}' used without 'OWNED' macro. This memory must be manually freed using 'owner_free'. [-Wownedc-manual-free]")
                        warnings += 1

    except Exception as e:
        print(f"Error processing {filepath}: {e}", file=sys.stderr)
        
    return errors, warnings

def main():
    parser = argparse.ArgumentParser(description="OwnedC Static Analyzer")
    parser.add_argument("files", nargs="+", help="C source files to analyze")
    args = parser.parse_args()
    
    total_errors = 0
    total_warnings = 0
    
    for filepath in args.files:
        e, w = analyze_file(filepath)
        total_errors += e
        total_warnings += w
        
    if total_errors > 0 or total_warnings > 0:
        print(f"\nOwnedC Analyzer finished with {total_errors} errors and {total_warnings} warnings.")
        if total_errors > 0:
            sys.exit(1)
    
if __name__ == "__main__":
    main()
