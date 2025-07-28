#!/bin/bash

# Script to update configuration values across all experiment configurations
# Updates CONFIG_PROJECT_PATH, CONFIG_TOTAL_CORES, CONFIG_SIM_CORES, and CONFIG_EXTRA_COST_TIME
# in all config.h and config.mk files under experiments/configs/

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NEX_ROOT="$(dirname "$SCRIPT_DIR")"
MAIN_CONFIG_H="$NEX_ROOT/include/config/config.h"
MAIN_CONFIG_MK="$NEX_ROOT/config.mk"
CONFIGS_DIR="$NEX_ROOT/experiments/configs"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Updating experiment configurations...${NC}"

# Check if main config files exist
if [[ ! -f "$MAIN_CONFIG_H" ]]; then
    echo -e "${RED}Error: Main config.h not found at $MAIN_CONFIG_H${NC}"
    exit 1
fi

if [[ ! -f "$MAIN_CONFIG_MK" ]]; then
    echo -e "${RED}Error: Main config.mk not found at $MAIN_CONFIG_MK${NC}"
    exit 1
fi

# Function to extract config value from config.h
extract_config_h_value() {
    local config_name="$1"
    local config_file="$2"
    grep "^#define $config_name " "$config_file" | sed "s/^#define $config_name //" | tr -d '\n'
}

# Function to extract config value from config.mk
extract_config_mk_value() {
    local config_name="$1"
    local config_file="$2"
    grep "^$config_name=" "$config_file" | sed "s/^$config_name=//" | tr -d '\n'
}

# Extract values from main config files
echo "Reading values from main configuration files..."

PROJECT_PATH_H=$(extract_config_h_value "CONFIG_PROJECT_PATH" "$MAIN_CONFIG_H")
TOTAL_CORES_H=$(extract_config_h_value "CONFIG_TOTAL_CORES" "$MAIN_CONFIG_H")
SIM_CORES_H=$(extract_config_h_value "CONFIG_SIM_CORES" "$MAIN_CONFIG_H")
EXTRA_COST_TIME_H=$(extract_config_h_value "CONFIG_EXTRA_COST_TIME" "$MAIN_CONFIG_H")

PROJECT_PATH_MK=$(extract_config_mk_value "CONFIG_PROJECT_PATH" "$MAIN_CONFIG_MK")
TOTAL_CORES_MK=$(extract_config_mk_value "CONFIG_TOTAL_CORES" "$MAIN_CONFIG_MK")
SIM_CORES_MK=$(extract_config_mk_value "CONFIG_SIM_CORES" "$MAIN_CONFIG_MK")
EXTRA_COST_TIME_MK=$(extract_config_mk_value "CONFIG_EXTRA_COST_TIME" "$MAIN_CONFIG_MK")

echo -e "${YELLOW}Main config.h values:${NC}"
echo "  CONFIG_PROJECT_PATH = $PROJECT_PATH_H"
echo "  CONFIG_TOTAL_CORES = $TOTAL_CORES_H"
echo "  CONFIG_SIM_CORES = $SIM_CORES_H"
echo "  CONFIG_EXTRA_COST_TIME = $EXTRA_COST_TIME_H"

echo -e "${YELLOW}Main config.mk values:${NC}"
echo "  CONFIG_PROJECT_PATH = $PROJECT_PATH_MK"
echo "  CONFIG_TOTAL_CORES = $TOTAL_CORES_MK"
echo "  CONFIG_SIM_CORES = $SIM_CORES_MK"
echo "  CONFIG_EXTRA_COST_TIME = $EXTRA_COST_TIME_MK"

# Function to update config.h file
update_config_h() {
    local config_file="$1"
    echo "  Updating $config_file"
    
    # Create a temporary file
    local temp_file="${config_file}.tmp"
    
    # Process the file line by line
    while IFS= read -r line; do
        case "$line" in
            "#define CONFIG_PROJECT_PATH "*)
                echo "#define CONFIG_PROJECT_PATH $PROJECT_PATH_H"
                ;;
            "#define CONFIG_TOTAL_CORES "*)
                echo "#define CONFIG_TOTAL_CORES $TOTAL_CORES_H"
                ;;
            "#define CONFIG_SIM_CORES "*)
                echo "#define CONFIG_SIM_CORES $SIM_CORES_H"
                ;;
            "#define CONFIG_EXTRA_COST_TIME "*)
                echo "#define CONFIG_EXTRA_COST_TIME $EXTRA_COST_TIME_H"
                ;;
            *)
                echo "$line"
                ;;
        esac
    done < "$config_file" > "$temp_file"
    
    # Replace the original file
    mv "$temp_file" "$config_file"
}

# Function to update config.mk file
update_config_mk() {
    local config_file="$1"
    echo "  Updating $config_file"
    
    # Create a temporary file
    local temp_file="${config_file}.tmp"
    
    # Process the file line by line
    while IFS= read -r line; do
        case "$line" in
            "CONFIG_PROJECT_PATH="*)
                echo "CONFIG_PROJECT_PATH=$PROJECT_PATH_MK"
                ;;
            "CONFIG_TOTAL_CORES="*)
                echo "CONFIG_TOTAL_CORES=$TOTAL_CORES_MK"
                ;;
            "CONFIG_SIM_CORES="*)
                echo "CONFIG_SIM_CORES=$SIM_CORES_MK"
                ;;
            "CONFIG_EXTRA_COST_TIME="*)
                echo "CONFIG_EXTRA_COST_TIME=$EXTRA_COST_TIME_MK"
                ;;
            *)
                echo "$line"
                ;;
        esac
    done < "$config_file" > "$temp_file"
    
    # Replace the original file
    mv "$temp_file" "$config_file"
}

# Find and update all config.h files in experiment directories
echo -e "${GREEN}Updating config.h files...${NC}"
updated_h_count=0
while IFS= read -r -d '' config_file; do
    # Skip the main config.h file
    if [[ "$config_file" != "$MAIN_CONFIG_H" ]]; then
        update_config_h "$config_file"
        ((updated_h_count++))
    fi
done < <(find "$CONFIGS_DIR" -name "config.h" -type f -print0) || true

# Find and update all config.mk files in experiment directories
echo -e "${GREEN}Updating config.mk files...${NC}"
updated_mk_count=0
while IFS= read -r -d '' config_file; do
    # Skip the main config.mk file
    if [[ "$config_file" != "$MAIN_CONFIG_MK" ]]; then
        update_config_mk "$config_file"
        ((updated_mk_count++))
    fi
done < <(find "$CONFIGS_DIR" -name "config.mk" -type f -print0) || true

echo -e "${GREEN}Configuration update completed!${NC}"
echo "Updated $updated_h_count config.h files"
echo "Updated $updated_mk_count config.mk files"

# Verify some of the updates
echo -e "${YELLOW}Verification (first few updated files):${NC}"
find "$CONFIGS_DIR" -name "config.h" -type f | head -3 | while read -r file; do
    echo "  $file:"
    grep -E "CONFIG_(PROJECT_PATH|TOTAL_CORES|SIM_CORES|EXTRA_COST_TIME)" "$file" | head -4 | sed 's/^/    /'
done
