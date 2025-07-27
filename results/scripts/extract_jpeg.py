#!/usr/bin/env python3

import os
import json
import glob
from pathlib import Path

def extract_data_from_json(filepath):
    """
    Extract JPEG performance data from a classification JSON file.
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
    
    real_time_matches = re.findall(r'Real latency ([\d.]+)', data)
    if real_time_matches:
        real_time = float(real_time_matches[-1])

    # Extract "Time taken: <>" - find all and take the last one
    time_taken_matches = re.findall(r'Time taken ([\d.]+)', data)
    time_taken = 0.0
    if time_taken_matches:
            for ele in time_taken_matches:
                time_taken += int(ele)*1000.0

    
    return time_taken, real_time


def filename_to_label(filename):
    """
    Convert JPEG classification filename to meaningful label.
    """
    base_name = "JPEG-NEX-"
    model_name = "single"

    acc_name = ""
    if "rtl" in filename:
        acc_name = "rtl"
    elif "dsim" in filename:
        acc_name = "dsim"

    if "multi" in filename:
        model_name = "multi"
        # grep the number at the end 
        # grep 4 here for example: classify_multi-resnet18_v1-JPEG-go3-lpn-4-1.json
        base_name += filename.split('_')[-2] + "devices" + '-'
    
    base_name += model_name + '-' + acc_name
    return base_name

def main():
    script_dir = Path(__file__).parent
    
    # Look for JPEG classification results
    search_dirs = [
        script_dir/'..'/'jpeg',
    ]
    
    all_results = []
    filename_to_label_map = {}  # Store filename to label mapping
    
    for search_dir in search_dirs:
        if not search_dir.exists():
            continue
            
        print(f"Searching in {search_dir}")
        # Find all *jpeg*-1.json files
        log_files = list(search_dir.glob('*'))
        
        if log_files:
            print(f"Found {len(log_files)} JPEG files in {search_dir}")

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
        print("No files found in any search directory")
        return
    

    # Also create a Python data file for easy import
    data_file = script_dir / 'compiled_data/jpeg_data.py'
    with open(data_file, 'w') as f:
        f.write("# JPEG extracted performance data\n\n")
        
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
