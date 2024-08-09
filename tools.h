#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <errno.h>
#include <time.h>

#define SECTOR_SIZE 512 // Defines the size of a disk sector in bytes

#define _MO (1ULL<<20) // Defines a megabyte in bytes
#define _GO (1ULL<<30) // Defines a gigabyte in bytes

// Global variables
size_t nb_run;        // Number of IO operations to perform
size_t nb_bloc;       // Number of blocks to read/write in each IO operation
size_t sz_bloc;       // Size of each block in bytes
size_t *times;        // Array to store the times taken for each IO operation

size_t filesize;      // Size of the file to be used for IO operations
size_t nb_skip;       // Number of initial IO operations to skip when calculating statistics
enum { READ_MODE, WRITE_MODE } mode; // Enumeration to define the mode (read or write)

struct timeval *time_epoch_start; // Array to store the start timestamps of each IO operation
struct timeval *time_epoch_end;   // Array to store the end timestamps of each IO operation

// Function prototypes
void measure_read(char *file_path, char *buffer);
void measure_write(char *file_path, char *buffer);
void parse_args(int argc, char **argv);
size_t get_val_arg(const char *arg);
void make_file_if_necessary(char *path, size_t filesize);
void print_mean_stdev(size_t *times, size_t n);
void log_times(const char *path, size_t *times, size_t n);
void log_timestamps(const char *path, struct timeval *timestamps, size_t n);
void format_timestamp(struct timeval *tv, char *buffer, size_t buffer_size);

// Function to compare two size_t values, used for sorting
int compare_size_t(const void *a, const void *b)
{
    size_t val1 = *(const size_t*)a;
    size_t val2 = *(const size_t*)b;
    return (val1 > val2) - (val1 < val2);
}

// Function to calculate quartiles (Q1, median, Q3) of the sorted times array
void calculate_quartiles(size_t *times, size_t n, size_t *q1, size_t *median, size_t *q3)
{
    // Sort the times array using qsort and the compare_size_t function
    qsort(times, n, sizeof(size_t), compare_size_t);

    // Calculate the first quartile (Q1), median, and third quartile (Q3)
    *q1 = times[n / 4];
    *median = times[n / 2];
    *q3 = times[3 * n / 4];
}

// Function to print the mean, standard deviation, and 95% confidence interval of the times array
void print_mean_stdev(size_t *times, size_t n)
{
    size_t _mean = 0, _stdev = 0;
    double mean, stdev;
    double interval95;
    size_t q1, median, q3;

    // Calculate the sum of times and sum of squared times
    for(size_t i = 0; i < n; i++){
        _mean += times[i];
        _stdev += times[i] * times[i];
    }
    
    // Calculate the mean and standard deviation
    mean = (double)_mean / (double)n;
    stdev = (double)_stdev / (double)n;
    stdev = stdev - mean * mean;
    stdev = sqrt(stdev);

    // Calculate the 95% confidence interval
    interval95 = 2 * stdev / sqrt(n);
    
    // Calculate the quartiles using the previously defined function
    calculate_quartiles(times, n, &q1, &median, &q3);

    // Print the mean, 95% confidence interval, and quartiles in milliseconds
    printf("Mean: %.7lf ms     95%% CI: ±%.7lf ms     Q1: %.7lf ms     Median: %.7lf ms     Q3: %.7lf ms\n",
           mean / 1e3, interval95 / 1e3, (double)q1 / 1e3, (double)median / 1e3, (double)q3 / 1e3);
}

// Function to log the times array to a file
void log_times(const char *path, size_t *times, size_t n)
{
    FILE *file = fopen(path, "w"); // Open the file for writing
    for(size_t i = 0; i < n; i++)
        fprintf(file, "%lu\n", times[i]); // Write each time value to the file
    fclose(file); // Close the file
}

// Function to log the timestamps array to a file
void log_timestamps(const char *path, struct timeval *timestamps, size_t n)
{
    FILE *file = fopen(path, "w"); // Open the file for writing
    char buffer[128]; // Buffer for formatted timestamps

    // Write each timestamp to the file after formatting it
    for (size_t i = 0; i < n; i++) {
        format_timestamp(&timestamps[i], buffer, sizeof(buffer));
        fprintf(file, "%s\n", buffer);
    }
    fclose(file); // Close the file
}

// Function to format a timestamp into a string
void format_timestamp(struct timeval *tv, char *buffer, size_t buffer_size) {
    struct tm *tm_info;
    char time_string[64];

    // Convert the timeval structure to a tm structure
    tm_info = localtime(&tv->tv_sec);
    // Format the tm structure into a string with seconds precision
    strftime(time_string, sizeof(time_string), "%Y-%m-%dT%H:%M:%S", tm_info);
    // Append the microseconds and timezone offset to the formatted string
    snprintf(buffer, buffer_size, "%s.%06ld%+03ld:00", time_string, tv->tv_usec, tm_info->tm_gmtoff / 3600);
}

// Function to parse command-line arguments and set global variables
void parse_args(int argc, char **argv)
{
    int dry_run = 0;
    int percent_skip = -1;

    // Set default parameters
    mode = READ_MODE;
    nb_run  = 1;
    nb_bloc = 1;
    sz_bloc = SECTOR_SIZE;
    filesize = 1 * _GO;
    nb_skip = 0;

    // Loop through each command-line argument
    for(int i = 1; i < argc; i++){
        if(!strcmp(argv[i], "--mode")){
            if(++i > argc) goto usage;
            if(*argv[i] == 'r' || *argv[i] == 'R')
                mode = READ_MODE;
            else
                mode = WRITE_MODE;
        }
        else if(!strcmp(argv[i], "--nb_run")){
            if(++i > argc) goto usage;
            nb_run = get_val_arg(argv[i]);
        }
        else if(!strcmp(argv[i], "--nb_bloc")){
            if(++i > argc) goto usage;
            nb_bloc = get_val_arg(argv[i]);
        }
        else if(!strcmp(argv[i], "--sz_bloc")){
            if(++i > argc) goto usage;
            sz_bloc = get_val_arg(argv[i]);
        }
        else if(!strcmp(argv[i], "--filesize")){
            if(++i > argc) goto usage;
            filesize = get_val_arg(argv[i]);
        }
        else if(!strcmp(argv[i], "--skip")){
            if(++i > argc) goto usage;
            if(argv[i][strlen(argv[i]) - 1] == '%'){
                argv[i][strlen(argv[i]) - 1] = '\0';
                percent_skip = atoi(argv[i]);
            }
            else
                nb_skip = get_val_arg(argv[i]);
        }
        else if(!strcmp(argv[i], "--dry")){
            dry_run = 1;
        }
        else 
            goto usage;
    }

    // Calculate the number of runs to skip based on percentage if specified
    if(percent_skip > 0){
        nb_skip = nb_run * (double)percent_skip / 100.;
    }

    // Ensure that the number of skipped runs is less than the total number of runs
    if(nb_skip >= nb_run){
        fprintf(stderr, "to much skip: %ld/%ld\n", nb_skip, nb_run);
        exit(1);
    }

    // If dry run is specified, print the configuration and exit
    if(dry_run){
        fprintf(stderr, "mode: %s - run: %ld - bloc: %ld - sz_bloc: %ld - filesize: %ld - skip: %ld\n"
                , (mode == READ_MODE) ? "READ" : "WRITE"
                , nb_run, nb_bloc, sz_bloc, filesize, nb_skip);
        exit(0);
    }

    return;

    // If arguments are invalid, print usage information
    usage:
        fprintf(stderr, "usage: %s --mode <r|w> --nb_run <num> --nb_bloc <num> --sz_bloc <num> --filesize <num> --skip <num|%%>\n", argv[0]);
        fprintf(stderr, "\t's' = 512o, 'k' = 1Ko, 'M' = 1Mo, 'G' = 1Go \n");
        fprintf(stderr, "print le resultat dans stdout sous la forme: <mean> <stdev>, logs dans le fichier log.txt\n");
        
        exit(1);
}

// Function to parse and return a size value from a command-line argument
size_t get_val_arg(const char *arg)
{
    size_t value;
    char last_character;

    value = atoll(arg);
    last_character = arg[strlen(arg) - 1];
    switch (last_character)
    {
        case 's': case 'S':
            value *= SECTOR_SIZE;
            break;

        case 'k': case 'K':
            value *= 1<<10;
            break;

        case 'm': case 'M':
            value *= 1<<20;
            break;

        case 'G':
            value *= 1<<30;
            break;

        default:
            break;
    }
    return value;
}

// Function to create a file with random content if it does not already exist or is too small
void make_file_if_necessary(char *path, size_t filesize)
{
    const size_t _BUF_SIZE = 1<<22;
    char buffer[_BUF_SIZE];
    size_t total_written = 0;
    int fd, fdrand, fdcleancache;
    
    struct stat64 file_stat;
    if(stat64(path, &file_stat) < 0){
        if(errno != ENOENT) // if there is an error and file already exists
            goto lb_error;
    }
    // Exit if the file already exists and is of the correct size
    else 
        if(file_stat.st_size >= (off64_t)filesize) 
            return;

    // Create and write random content to the file
    fd = open64(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    fdrand = open("/dev/random", O_RDONLY);

    while(total_written < filesize){
        size_t buf_size = _BUF_SIZE;

        if(buf_size > filesize - total_written)
            buf_size = filesize - total_written;
        
        if(read(fdrand, buffer, buf_size) <= 0) 
            goto lb_error_rd;

        ssize_t written = write(fd, buffer, buf_size);
        if(written < 0) 
            goto lb_error_wr;
        total_written += written;
    }
    
    close(fd);
    close(fdrand);

    // Flush the cache to ensure data is written to disk
    sync();
    fdcleancache = open("/proc/sys/vm/drop_caches", O_WRONLY);
    if(write(fdcleancache, "3", 1) < 0)
        fprintf(stderr, "cache flush failed, need root\n");
    close(fdcleancache);

    return;

    lb_error_rd:
        perror("make_file_if_necessary -> read");
        exit(1);
    lb_error_wr:
        perror("make_file_if_necessary -> write");
        exit(1);
    lb_error:
        perror("make_file_if_necessary -> stat64");
        exit(1);
}

