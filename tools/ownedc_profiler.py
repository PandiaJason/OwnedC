import json
import sys
import os

HTML_TEMPLATE = """
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>OwnedC Memory Profiler</title>
    <style>
        body {{ font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; background-color: #f5f5f7; color: #1d1d1f; margin: 0; padding: 2rem; }}
        h1 {{ font-size: 2.5rem; font-weight: 700; margin-bottom: 0.5rem; }}
        .subtitle {{ color: #86868b; margin-bottom: 2rem; }}
        .dashboard {{ display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 1.5rem; margin-bottom: 2rem; }}
        .card {{ background: white; border-radius: 12px; padding: 1.5rem; box-shadow: 0 4px 6px rgba(0,0,0,0.05); }}
        .card h3 {{ margin-top: 0; color: #86868b; font-size: 0.9rem; text-transform: uppercase; letter-spacing: 0.05em; }}
        .card .value {{ font-size: 2.5rem; font-weight: 700; }}
        table {{ width: 100%; border-collapse: collapse; background: white; border-radius: 12px; overflow: hidden; box-shadow: 0 4px 6px rgba(0,0,0,0.05); }}
        th, td {{ padding: 1rem; text-align: left; border-bottom: 1px solid #f0f0f0; }}
        th {{ background: #fdfdfd; color: #86868b; font-weight: 600; text-transform: uppercase; font-size: 0.8rem; letter-spacing: 0.05em; }}
        tr:last-child td {{ border-bottom: none; }}
        .badge {{ padding: 0.25rem 0.75rem; border-radius: 9999px; font-size: 0.8rem; font-weight: 600; display: inline-block; }}
        .badge.owned {{ background: #e3f2fd; color: #1565c0; }}
        .badge.shared {{ background: #e8f5e9; color: #2e7d32; }}
        .badge.borrowed {{ background: #fff3e0; color: #ef6c00; }}
        .badge.released {{ background: #ffebee; color: #c62828; }}
        .ptr {{ font-family: monospace; color: #d81b60; }}
    </style>
</head>
<body>
    <h1>OwnedC Memory Profiler</h1>
    <p class="subtitle">Live Allocation Graph Visualization</p>
    
    <div class="dashboard">
        <div class="card">
            <h3>Total Allocations</h3>
            <div class="value">{total_allocs}</div>
        </div>
        <div class="card">
            <h3>Total Bytes Allocated</h3>
            <div class="value">{total_bytes} bytes</div>
        </div>
        <div class="card">
            <h3>Active Borrows</h3>
            <div class="value">{total_borrows}</div>
        </div>
    </div>
    
    <table>
        <thead>
            <tr>
                <th>Pointer</th>
                <th>Size (Bytes)</th>
                <th>State</th>
                <th>Owner Thread</th>
                <th>Borrows</th>
            </tr>
        </thead>
        <tbody>
            {table_rows}
        </tbody>
    </table>
</body>
</html>
"""

def generate_report(json_path):
    if not os.path.exists(json_path):
        print(f"Error: {json_path} not found.")
        sys.exit(1)
        
    with open(json_path, 'r') as f:
        data = json.load(f)
        
    allocs = data.get("allocations", [])
    total_bytes = sum(a["size"] for a in allocs)
    total_borrows = sum(a["borrow_count"] for a in allocs)
    
    # State mapping
    state_map = {
        "1": "owned",
        "2": "borrowed",
        "3": "shared",
        "4": "released"
    }
    state_name_map = {
        "1": "OWNED",
        "2": "BORROWED",
        "3": "SHARED",
        "4": "RELEASED"
    }
    
    rows = []
    for a in allocs:
        state_class = state_map.get(str(a["state"]), "owned")
        state_name = state_name_map.get(str(a["state"]), "UNKNOWN")
        row = f"""
            <tr>
                <td class="ptr">{a["ptr"]}</td>
                <td>{a["size"]}</td>
                <td><span class="badge {state_class}">{state_name}</span></td>
                <td>{a["owner_thread"]}</td>
                <td>{a["borrow_count"]}</td>
            </tr>
        """
        rows.append(row)
        
    html = HTML_TEMPLATE.format(
        total_allocs=len(allocs),
        total_bytes=total_bytes,
        total_borrows=total_borrows,
        table_rows="\\n".join(rows)
    )
    
    report_path = "profiler_report.html"
    with open(report_path, "w") as f:
        f.write(html)
        
    print(f"Profiler report generated: {report_path}")

if __name__ == "__main__":
    import sys
    if len(sys.argv) < 2:
        print("Usage: python ownedc_profiler.py <memory_dump.json>")
        sys.exit(1)
    generate_report(sys.argv[1])
