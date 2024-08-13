# Energy Consumption Analysis for IO Operations

Welcome to the repository for my internship project focused on the analysis of energy consumption during input/output (IO) operations. This project includes a set of scripts and tools designed to calculate, visualize, and compare energy consumption using different methods. The main goal is to determine the most accurate approach for measuring energy consumed by IO operations, specifically focusing on projections and averages.

## Table of Contents

1. [Project Overview](#project-overview)
2. [Project Structure](#project-structure)
3. [Installation](#installation)
4. [Usage](#usage)
    - [Example: Compiling and Running `iotest.c`](#example-compiling-and-running-iotestc)
    - [Running the Benchmark](#running-the-benchmark)
    - [Plotting Results](#plotting-results)
5. [Scripts Explanation](#scripts-explanation)
    - [iotest.c and iotest.h](#iotestc-and-iotesth)
    - [benchmark.sh](#benchmarksh)
    - [plotting.sh](#plottingsh)
    - [format.sh](#formatsh)

## Project Overview

This project was developed during my internship and aims to analyze energy consumption during IO operations on storage devices. The focus is on calculating the exact energy consumed using wattmeter readings and comparing these calculations using different methods such as projection and average energy methods. The scripts automate the entire process from data collection to analysis and visualization.

## Project Structure

The project is organized as follows:

```plaintext
├── benchmark.sh            # Script for running IO benchmarks
├── format.sh               # Script for formatting and processing raw data
├── plotting.sh             # Script for plotting raw data
├── logs/                   # Directory containing raw and processed data files
├── scripts/                # Directory containing all Python and Shell scripts
│   ├── plot/               # Directory containing all Python and Shell scripts for plotting
        └── ...
│   ├── format/             # Directory containing all Python and Shell scripts for formating
        └── ...
│   ├── math/               # Directory containing all Python and Shell scripts for computing projection & line equation
│       └── ...             
├── README.md       
```

## Usage

### Example: Compiling and Running `iotest.c`

The `iotest.c` file is a C program used to simulate IO operations. The associated header file, `iotest.h`, contains the definitions and functions used within the `iotest.c` file.

#### Compiling `iotest.c`

To compile the `iotest.c` program, use the following command:

```bash
gcc -g iotest.c -o iotest -lm
```

This command compiles the C code and creates an executable named `iotest`.

### Running `iotest`

Once compiled, you can run the `iotest` program with various options. For example:

```bash
./iotest --mode read --nb_run 100 --nb_bloc 1 --sz_bloc 1M --filesize 256M
```

This command runs the `iotest` program in read mode with 100 iterations, a block size of 1M, and a file size of 256M.

### Running the Benchmark

The `benchmark.sh` script is used to run IO operations and measure energy consumption. Here's an example of how to run the benchmark:

```bash
sudo-g5k ./benchmark.sh READ RAND HDD
```

This script runs the IO benchmark with specified parameters (READ OR WRITE mode, RANDOM or SEQUENTIAL (RAND OR SEQ) access pattern, HDD OR SSD storage type) and stores the results in the `logs/` directory.

### Plotting Script (plotting.sh)

The `plotting.sh` script is used to generate various plots from the benchmark results. It supports different types of plots, such as baseline plots, boxplots, and IO energy consumption plots.

#### Usage Example:

```bash
./plotting.sh <log_dir> <type> [<optional_arg>]
```
- `log_dir`: The directory containing log files at the base of the logs (see example below).
- `type`: The type of plot to generate (e.g., `baseline` or `sz_bloc)`.
- `optional_arg`: An optional argument to specify additional options (e.g., `nb_run`).

For example, to generate ALL plot for all runs per iteration:

```bash
./plotting.sh logs/HDD/READ/RAND/ plot_all nb_run
```

### Scripts Explanation
## iotest.c and iotest.h

- `iotest.c`: This C program simulates IO operations by reading and writing data to a storage device. It is highly configurable with various command-line options that allow you to specify the mode (READ/WRITE), the number of iterations, block sizes, and file sizes.

- `iotest.h`: The header file contains function prototypes, macros, and structure definitions used in `iotest.c`. It helps in organizing the code and making the functions available across different files.

## benchmark.sh

- `benchmark.sh`: This shell script automates the process of running IO benchmarks on different storage devices. It compiles the iotest.c program, defines block sizes and file sizes, and runs multiple iterations of the IO operations while capturing the energy consumption data from a wattmeter. The results are stored in structured directories for later analysis.

## plotting.sh

- `plotting.sh`: This shell script is designed to generate visual plots of the energy consumption data collected during the benchmarks. It can produce different types of plots depending on the provided arguments. The script calls various Python scripts to generate baseline plots, boxplots, and IO-specific plots.

## format.sh


The `format.sh` script is used to organize and format raw data collected during IO tests by structuring it into a more manageable format for subsequent analysis and visualization.

### Usage

```bash
./format.sh <directory_to_move>
```

* `<directory_to_move>`: The name of the directory containing the raw data to be formatted. This directory will be moved into the `logs/brute_data` folder, in our case it will be either `SSD` or `HDD` (contained in `logs/`)

### Main Features

* **Moving Raw Data**: The script moves the specified directory containing raw data into `logs/brute_data`.
* **Creating Formatted Directory Structure**: A new directory structure is created in `logs/formatted_data` to organize data by block size and access type (sequential or random).
* **Formatting Energy Data**: JSON files containing energy measurements are converted to CSV files. The CSV files are then placed in the appropriate directories.
* **Copying Plots**: Generated plots and boxplots are copied into the corresponding directories under `formatted_data`.
* **Executing Additional Formatting Scripts**:
	+ `generate_perf.sh`: Generates performance CSV files using another python script (same name).
	+ `process_baseline.sh`: Formats baseline data using another python script.
	+ `move_perf_files.sh`: Moves and merges performance CSV files.
	+ `merge_csv_files.py` : Merges all the perf CSV file (each of them correspond to each iteration).
	+ `rename_csv_files.sh`: Renames CSV files for clear organization.
    + `move_perf_files.sh` : Moves the perf et energy file in the right directories.

### Example

To format raw data from the HDD benchmarking in the `logs/` directory:

```bash
./format.sh HDD
```

After running the script, the `HDD` directory will be moved to `logs/brute_data`, and the formatted data will be available in `logs/formatted_data/HDD`.
