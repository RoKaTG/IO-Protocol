#!/bin/bash

# Check if the correct number of arguments are provided
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <directory_to_rename>"
    exit 1
fi

# Set the directory to rename based on the argument provided
DIRECTORY_TO_RENAME=$1
FORMATTED_DIR="logs/formatted_data/${DIRECTORY_TO_RENAME}"

# Function to rename CSV files in the specified directory structure
rename_csv_files() {
    for access_pattern in RAND SEQ; do  # Loop through access patterns: Random (RAND) and Sequential (SEQ)
        for read_write in READ WRITE; do  # Loop through operations: Read and Write
            for size in 1M 2M 4M 8M 1s 128k 16k 512k 8k; do  # Loop through different IO sizes
                for file_size in 256M 1G 4G; do  # Loop through file sizes: 256MB, 1GB, 4GB
                    for type in energy perf; do  # Loop through file types: energy and performance (perf)
                        # Define the base directory path for small_size_io
                        local base_dir="${FORMATTED_DIR}/${read_write}/small_size_io/${size}/${access_pattern}/${file_size}/${type}"
                        # Check if the directory exists
                        if [ -d "${base_dir}" ]; then
                            # Define the path to the merged CSV file
                            local csv_file="${base_dir}/data_merged.csv"
                            # If the merged CSV file exists, rename it
                            if [ -f "${csv_file}" ]; then
                                # Define the new name for the CSV file
                                local new_csv_file="${type}_${access_pattern}_buffer${file_size}_io${size}.csv"
                                # Rename the CSV file
                                mv "${csv_file}" "${base_dir}/${new_csv_file}"
                            fi
                        fi

                        # Repeat the process for big_size_io
                        base_dir="${FORMATTED_DIR}/${read_write}/big_size_io/${size}/${access_pattern}/${file_size}/${type}"
                        if [ -d "${base_dir}" ]; then
                            csv_file="${base_dir}/data_merged.csv"
                            if [ -f "${csv_file}" ]; then
                                new_csv_file="${type}_${access_pattern}_buffer${file_size}_io${size}.csv"
                                mv "${csv_file}" "${base_dir}/${new_csv_file}"
                            fi
                        fi
                    done
                done
            done

            # Rename CSV files in the baseline directories
            find "${FORMATTED_DIR}" -type d -name "baseline" | while read -r baseline_dir; do
                local csv_file="${baseline_dir}/data_merged.csv"
                if [ -f "${csv_file}" ]; then
                    mv "${csv_file}" "${baseline_dir}/baseline.csv"
                fi
            done
        done
    done
}

# Call the rename_csv_files function to start the renaming process
rename_csv_files

