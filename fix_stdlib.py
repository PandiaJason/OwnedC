import os
import re

replacements = [
    (r'\brealloc\(', 'OWNEDC_DEFAULT_REALLOC('),
    (r'\bfree\(', 'OWNEDC_DEFAULT_FREE('),
    (r'\bstrlen\(', 'OWNEDC_STRLEN('),
    (r'\bmemcpy\(', 'OWNEDC_MEMCPY('),
    (r'\bfprintf\(stderr,\s*', 'OWNEDC_PRINTF('),
    (r'\bprintf\(', 'OWNEDC_PRINTF('),
    (r'\babort\(\)', 'OWNEDC_ABORT()'),
    (r'\bmemset\(', 'OWNEDC_MEMSET(')
]

for root, _, files in os.walk('src'):
    for file in files:
        if file.endswith('.c') and file != 'runtime.c':
            path = os.path.join(root, file)
            with open(path, 'r') as f:
                content = f.read()
            
            for pat, repl in replacements:
                content = re.sub(pat, repl, content)
                
            with open(path, 'w') as f:
                f.write(content)

print("Done")
