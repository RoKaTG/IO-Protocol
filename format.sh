#!/bin/bash

# Function to handle errors and revert changes.
# This section is currently commented out but can be used to handle errors by rolling back any changes made.
#error_exit() {
#    echo "Error detected. Reverting changes..."
#    # Remove any files and directories created in logs/formatted_data.
#    rm -rf "${FORMATTED_DIR}/${DIRECTORY_TO_MOVE}"
#    mv "${BRUTE_DIR}/${DIRECTORY_TO_MOVE}" "${LOG_DIR}/"
#    rm -rf "${BRUTE_DIR}"
#    exit 1
#}

# Check that the script is executed with the correct number of arguments.
if [ "$#" -ne 1 ]; then
    # If not, display the correct usage and exit.
    echo "Usage: $0 <directory_to_move>"
    exit 1
fi

# Set variables for directory paths.
DIRECTORY_TO_MOVE=$1
LOG_DIR="logs"                            # The main logs directory.
FORMATTED_DIR="${LOG_DIR}/formatted_data" # The directory where formatted data will be stored.
BRUTE_DIR="${LOG_DIR}/brute_data"         # The directory where raw data will be moved.
PYTHON_SCRIPT="script/format/wattmeter_format.py" # Python script to format the data.

# Check if the logs directory exists.
if [ ! -d "$LOG_DIR" ]; then
    echo "The directory '$LOG_DIR' does not exist."
    exit 1
fi

# Create the brute_data directory if it doesn't exist and move the specified directory there.
mkdir -p "${BRUTE_DIR}"
if [ -d "${LOG_DIR}/${DIRECTORY_TO_MOVE}" ]; then
    mv "${LOG_DIR}/${DIRECTORY_TO_MOVE}" "${BRUTE_DIR}/" ## || error_exit
else
    echo "The directory '${DIRECTORY_TO_MOVE}' does not exist in '${LOG_DIR}'."
    exit 1
fi

# Create a directory with the same name in formatted_data.
DEST_DIR="${FORMATTED_DIR}/${DIRECTORY_TO_MOVE}"
if [ ! -d "${DEST_DIR}" ]; then
    mkdir -p "${DEST_DIR}" ## || error_exit
fi

# Function to create directory structure in formatted_data.
create_directory_structure() {
    local base_dir=$1
    local access_pattern=$2

    # Create small_size_io and big_size_io directories.
    mkdir -p "${base_dir}/small_size_io" # || error_exit
    mkdir -p "${base_dir}/big_size_io" # || error_exit

    # Add directories for different file sizes under small_size_io.
    for size in 1s 128k 16k 512k 8k; do
        local pattern_dir="${base_dir}/small_size_io/${size}/${access_pattern}"
        mkdir -p "${pattern_dir}" # || error_exit
        create_size_directories "${pattern_dir}"
    done

    # Add directories for different file sizes under big_size_io.
    for size in 1M 4M 2M 8M; do
        local pattern_dir="${base_dir}/big_size_io/${size}/${access_pattern}"
        mkdir -p "${pattern_dir}" # || error_exit
        create_size_directories "${pattern_dir}"
    done
}

# Function to create subdirectories for each file size.
create_size_directories() {
    local pattern_dir=$1
    for file_size in 256M 1G 4G; do
        mkdir -p "${pattern_dir}/${file_size}/energy" # Directory for energy data.
        mkdir -p "${pattern_dir}/${file_size}/perf" # Directory for performance data.
    done
    # Create the baseline directory for baseline data.
    mkdir -p "${pattern_dir}/baseline" # || error_exit
}

# Function to copy plot images to the appropriate directories.
copy_plots() {
    local base_dir=$1
    local access_pattern=$2
    local io_size=$3
    local file_size=$4

    # Define the source and destination paths for plot images.
    local plot_src="${BRUTE_DIR}/${DIRECTORY_TO_MOVE}/READ/${access_pattern}/plot/${io_size}/plot_io_${io_size}_${file_size}.png"
    local plot_dest="${base_dir}/${io_size}/${access_pattern}/${file_size}/plot_io_${io_size}_${file_size}.png"

    # If the source plot exists, copy it to the destination.
    if [ -f "${plot_src}" ]; then
        cp "${plot_src}" "${plot_dest}" # || error_exit
    fi

    # Copy the baseline plot to the appropriate baseline directories.
    local baseline_src="${BRUTE_DIR}/${DIRECTORY_TO_MOVE}/READ/${access_pattern}/plot/baseline/plot_baseline.png"
    local baseline_dest="${base_dir}/${io_size}/${access_pattern}/baseline/plot_baseline.png"

    if [ -f "${baseline_src}" ]; then
        cp "${baseline_src}" "${baseline_dest}" # || error_exit
    fi
}

# Function to copy boxplot images to the appropriate directories.
copy_boxplots() {
    local base_dir=$1
    local access_pattern=$2
    local io_size=$3
    local boxplot_name=$4

    # Define the source and destination paths for boxplot images.
    local boxplot_src="${BRUTE_DIR}/${DIRECTORY_TO_MOVE}/READ/${access_pattern}/box_plot/${boxplot_name}"
    local boxplot_dest="${base_dir}/${io_size}/${access_pattern}/${boxplot_name}"

    # If the source boxplot exists, copy it to the destination.
    if [ -f "${boxplot_src}" ]; then
        cp "${boxplot_src}" "${boxplot_dest}" # || error_exit
    fi
}

# Function to copy baseline boxplots to all appropriate directories.
copy_baseline_boxplot() {
    local base_dir=$1
    local access_pattern=$2

    # Define the source path for the baseline boxplot.
    local boxplot_src="${BRUTE_DIR}/${DIRECTORY_TO_MOVE}/READ/${access_pattern}/box_plot/boxplot_baseline.png"

    if [ -f "${boxplot_src}" ]; then
        # Copy the baseline boxplot to the small_size_io directories.
        for size in 1s 128k 16k 512k 8k; do
            local baseline_dest="${base_dir}/small_size_io/${size}/${access_pattern}/baseline/boxplot_baseline.png"
            if [ ! -d "$(dirname "${baseline_dest}")" ]; then
                mkdir -p "$(dirname "${baseline_dest}")"
            fi
            cp "${boxplot_src}" "${baseline_dest}"
        done

        # Copy the baseline boxplot to the big_size_io directories.
        for size in 1M 4M 2M 8M; do
            local baseline_dest="${base_dir}/big_size_io/${size}/${access_pattern}/baseline/boxplot_baseline.png"
            if [ ! -d "$(dirname "${baseline_dest}")" ]; then
                mkdir -p "$(dirname "${baseline_dest}")"
            fi
            cp "${boxplot_src}" "${baseline_dest}"
        done
    fi
}

# Function to generate CSV files from JSON data.
generate_csv() {
    local json_file=$1
    local csv_file=$2
    # Use a Python script to convert the JSON data to CSV format.
    python3 ${PYTHON_SCRIPT} "${json_file}" "${csv_file}" # || error_exit
}

# Function to recursively format subdirectories.
format_subdirectories() {
    local current_dir=$1
    for read_write in $(ls "${current_dir}"); do
        for access_pattern in $(ls "${current_dir}/${read_write}"); do
            base_dir="${DEST_DIR}/${read_write}"
            create_directory_structure("${base_dir}" "${access_pattern}")

            # Copy plots and generate CSVs for small_size_io.
            for size in 1s 128k 16k 512k 8k; do
                for file_size in 256M 1G 4G; do
                    copy_plots("${base_dir}/small_size_io" "${access_pattern}" "${size}" "${file_size}")
                    copy_boxplots("${base_dir}/small_size_io" "${access_pattern}" "${size}" "boxplot_${size}.png")
                    json_src="${current_dir}/${read_write}/${access_pattern}/small_size_io/READ_${size}/READ_${file_size}.json"
                    csv_dest="${base_dir}/small_size_io/${size}/${access_pattern}/${file_size}/energy/data.csv"
                    if [ -f "${json_src}" ]; then
                        generate_csv("${json_src}" "${csv_dest}")
                    fi
                done
            done

            # Copy plots and generate CSVs for big_size_io.
            for size in 1M 4M 2M 8M; do
                for file_size in 256M 1G 4G; do
                    copy_plots("${base_dir}/big_size_io" "${access_pattern}" "${size}" "${file_size}")
                    copy_boxplots("${base_dir}/big_size_io" "${access_pattern}" "${size}" "boxplot_${size}.png")
                    json_src="${current_dir}/${read_write}/${access_pattern}/big_size_io/READ_${size}/READ_${file_size}.json"
                    csv_dest="${base_dir}/big_size_io/${size}/${access_pattern}/${file_size}/energy/data.csv"
                    if [ -f "${json_src}" ]; then
                        generate_csv("${json_src}" "${csv_dest}")
                    fi
                done
            done
            # Copy the baseline boxplot to all directories.
            copy_baseline_boxplot("${base_dir}" "${access_pattern}")
        done
    done
}

# Call the function to format the subdirectories.
format_subdirectories("${BRUTE_DIR}/${DIRECTORY_TO_MOVE}")

# Call the generate_perf.sh script to generate performance CSV files.
script/format/generate_perf.sh "${DIRECTORY_TO_MOVE}" # || error_exit

# Call the process_baseline.sh script to process baseline data.
script/format/process_baseline.sh "${DIRECTORY_TO_MOVE}" # || error_exit

# Call the move_perf_files.sh script to move performance files to the correct locations.
script/format/move_perf_files.sh "${DIRECTORY_TO_MOVE}" # || error_exit

# Call the rename_csv_files.sh script to rename the CSV files.
script/format/rename_csv_files.sh "${DIRECTORY_TO_MOVE}" # || error_exit

# Call the copy_result_csv.sh script to copy the final result CSVs.
script/format/copy_result_csv.sh "${DIRECTORY_TO_MOVE}"

# Print a completion message indicating that the process is complete.
echo "The directory ${DIRECTORY_TO_MOVE} has been moved to 'brute_data'. The directory structure has been created in 'formatted_data', plots have been copied, and CSV files have been generated."

