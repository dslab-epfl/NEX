#!/usr/bin/env python3

import os
import json
import glob
from pathlib import Path

def extract_data_from_json(filepath):
    """
    Extract PROTOACC performance data from a classification JSON file.
    Returns tuple of (total_inference_duration, warmup_duration, pure_inference_duration, time_taken, real_time).
    """
    with open(filepath, 'r') as f:
        data = f.read()

    
    total_inference_duration = None
    warmup_duration = None
    pure_inference_duration = None
    time_taken = None
    real_time = None
    
    import re
    
    perf0_matches = re.findall(r'PERF 0, ns new (\d+)', data)
    if perf0_matches:
        total = 0
        for ele in perf0_matches:
            total += int(ele)
        # perf0_number = total // len(perf0_matches)  # Average over all PERF
        # perf0_number = total // len(perf0_matches)  # Average over all PERF
        perf0_number = int(perf0_matches[0]) 

    perf1_matches = re.findall(r'PERF 1, ns new (\d+)', data)
    if perf1_matches:
        total = 0
        for ele in perf1_matches:
            total += int(ele)
        perf1_number = total // len(perf1_matches)  # Average over all PERF
        perf1_number = int(perf1_matches[0]) 

    assert len(perf0_matches) > 0, "No PERF 0 matches found"

    time_taken_matches = re.findall(r'Execution time \(ms\): (\d+)', data) 
    if time_taken_matches:
        time_taken = float(time_taken_matches[-1]) / 1000.0  # Convert ms to seconds

    print(time_taken, perf0_number, perf1_number)

    latency_data = perf0_number
    # if "bench2" in filepath or "bench5" in filepath:
    #     # gem5 is inaccurate for perf0 portion
    #     latency_data = perf1_number
        
    return latency_data, time_taken/len(perf0_matches)

def filename_to_label(filename):
    """
    Convert PROTOACC classification filename to meaningful label.
    """
    base_name = "Protoacc-NEX-"

    acc_name = "rtl" if "rtl" in filename else "dsim"

    if "bench" in filename:
        # Extract the bench number
        bench_name = filename.split('-')[0]
        base_name += f"{bench_name}-{acc_name}"
    
    return base_name

def main():
    script_dir = Path(__file__).parent
    
    # Look for PROTOACC classification results
    search_dirs = [
        script_dir/'..'/'protoacc',
    ]
    
    all_results = []
    filename_to_label_map = {}  # Store filename to label mapping
    
    for search_dir in search_dirs:
        if not search_dir.exists():
            continue
            
        print(f"Searching in {search_dir}")
        # Find all *protoacc*-1.json files
        log_files = list(search_dir.glob('*.txt'))
        
        if log_files:
            print(f"Found {len(log_files)} PROTOACC files in {search_dir}")

        for log_file in log_files:
            print(f"Processing {log_file.name}...")
            
            try:
                time_taken, real_time = extract_data_from_json(log_file)
                
                label = filename_to_label(log_file.name)
                
                # Store filename to label mapping
                filename_to_label_map[log_file.name] = label
                
                result = {
                    'filename': log_file.name,
                    'label': label,
                    'latency': time_taken,
                    'real_time': real_time,
                    'source_dir': str(search_dir)
                }
                
                all_results.append(result)
                
                print(f"  Label: {label}")
                print(f"  Time taken: {time_taken}ns" if time_taken else "  Time taken: None")
                print(f"  Real time: {real_time:.2f}s" if real_time else "  Real time: None")
                
            except Exception as e:
                print(f"  Error processing {log_file.name}: {e}")
    
    if not all_results:
        print("No files or error occurred found in any search directory")
        return
    

    # Also create a Python data file for easy import
    data_file = script_dir / 'compiled_data/protoacc_data.py'
    with open(data_file, 'w') as f:
        f.write("# PROTOACC extracted performance data\n\n")
        
        f.write("# Filename to label mapping\n")
        f.write("filename_to_label_map = {\n")
        for filename, label in filename_to_label_map.items():
            f.write(f"    '{filename}': '{label}',\n")
        f.write("}\n\n")
        
        f.write("# Combined data as dictionary\n")
        f.write("performance_data = {\n")
        for result in all_results:
            f.write(f"    '{result['label']}': {{\n")
            f.write(f"        'latency': {result['latency']},\n")
            f.write(f"        'real_time': {result['real_time']},\n")
            f.write(f"        'filename': '{result['filename']}'\n")
            f.write(f"    }},\n")
        f.write("}\n")
    
    print(f"Python data file: {data_file}")
    print(f"Processed {len(all_results)} files total")
    

if __name__ == "__main__":
    main()
