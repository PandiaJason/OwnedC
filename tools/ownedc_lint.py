import re
import sys

def lint_file(filepath):
    print(f"Linting {filepath}...")
    
    with open(filepath, 'r') as f:
        lines = f.readlines()
        
    in_function = False
    current_func = ""
    tracked_ptrs = {} # name -> state
    
    # Regexes
    func_start = re.compile(r'^\s*\w+\s+\w+\s*\(.*?\)\s*\{')
    malloc_re = re.compile(r'(\w+)\s*=\s*(?:\([^)]+\))?\s*owner_malloc\s*\(')
    free_re = re.compile(r'owner_free\s*\(\s*(\w+)\s*\)')
    return_re = re.compile(r'return\s+(\w+)\s*;')
    borrow_re = re.compile(r'ownership_borrow\s*\(\s*(\w+)\s*\)')
    release_re = re.compile(r'ownership_release\s*\(\s*(\w+)\s*\)')
    
    errors = 0
    
    for i, line in enumerate(lines):
        line = line.strip()
        
        # Super simple function parsing
        if '{' in line and not in_function and func_start.search(line):
            in_function = True
            current_func = line.split('{')[0].strip()
            tracked_ptrs.clear()
            continue
            
        if in_function:
            # Check malloc
            m = malloc_re.search(line)
            if m:
                ptr_name = m.group(1)
                tracked_ptrs[ptr_name] = 'OWNED'
                
            # Check free
            m = free_re.search(line)
            if m:
                ptr_name = m.group(1)
                if ptr_name in tracked_ptrs:
                    if tracked_ptrs[ptr_name] == 'FREED':
                        print(f"[{filepath}:{i+1}] ERROR: Double-free detected on '{ptr_name}' in {current_func}")
                        errors += 1
                    else:
                        tracked_ptrs[ptr_name] = 'FREED'
            
            # Check borrow
            m = borrow_re.search(line)
            if m:
                ptr_name = m.group(1)
                if ptr_name in tracked_ptrs and tracked_ptrs[ptr_name] == 'FREED':
                    print(f"[{filepath}:{i+1}] ERROR: Use-after-free (borrow) detected on '{ptr_name}' in {current_func}")
                    errors += 1
                    
            # Check returns
            m = return_re.search(line)
            if m:
                ret_ptr = m.group(1)
                if ret_ptr in tracked_ptrs:
                    tracked_ptrs[ret_ptr] = 'RETURNED'
                    
            # End of function
            if line == '}':
                in_function = False
                for ptr, state in tracked_ptrs.items():
                    if state == 'OWNED':
                        print(f"[{filepath}:{i+1}] ERROR: Memory leak detected! '{ptr}' is never freed or returned in {current_func}")
                        errors += 1
                        
    if errors == 0:
        print(f"Success: No ownership issues found in {filepath}!")
    else:
        print(f"Found {errors} errors in {filepath}.")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python ownedc_lint.py <file.c>")
        sys.exit(1)
    lint_file(sys.argv[1])
