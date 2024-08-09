#include "tools.h"

/// MAIN
int main(int argc, char **argv)
{
    char *buffer; // Buffer used to store data that will be read or written
    char timestamp_buffer[128]; // Buffer used to store formatted timestamps

    char file_path[] = "/tmp/test.file"; // Path to the test file where IO operations will be performed

    // Ensure that the test file exists and has the required size
    make_file_if_necessary(file_path, 4*_GO);

    // Parse command-line arguments to set up the IO parameters
    parse_args(argc, argv);

    // Allocate memory for storing the times and timestamps of each IO operation
    times = malloc(nb_run * nb_bloc * sizeof(size_t));
    time_epoch_start = malloc(nb_run * nb_bloc * sizeof(struct timeval));
    time_epoch_end = malloc(nb_run * nb_bloc * sizeof(struct timeval));

    // Align the buffer to the sector size to ensure optimal IO performance
    buffer = aligned_alloc(SECTOR_SIZE, sz_bloc);

    // LAUNCH MEASURE
    // Perform the IO operations (either reading or writing) based on the selected mode
    if (mode == READ_MODE)
        measure_read(file_path, buffer);
    else
        measure_write(file_path, buffer);

    // Calculate and print the mean and standard deviation of the IO times
    print_mean_stdev(times + nb_skip, nb_run - nb_skip);

    // Log the times and timestamps of each IO operation to files
    log_times("log.txt", times, nb_run * nb_bloc);
    log_timestamps("log_epoch_start.txt", time_epoch_start, nb_run * nb_bloc);
    log_timestamps("log_epoch_end.txt", time_epoch_end, nb_run * nb_bloc);

    // Free the allocated memory
    free(times);
    free(time_epoch_start);
    free(time_epoch_end);
    free(buffer);

    return 0; // Exit the program
}

void measure_read(char *file_path, char *buffer)
{
    struct timeval start, end; // Structures to hold the start and end times of each IO operation
    size_t randnum; // Random number used to determine the offset for reading
    loff_t offset; // Offset within the file where the read operation will start
    int fd, fdrand, fdcleancache; // File descriptors for the test file, random number generator, and cache cleaner
    char timestamp_buffer[128]; // Buffer for formatting timestamps

    // Open the test file for reading, with flags for synchronous and direct IO
    fd = open64(file_path, O_RDONLY | O_SYNC | O_DIRECT);
    if (fd < 0) {
        perror("open"); // Print an error message if the file cannot be opened
        exit(1); // Exit the program with an error code
    }
    
    // Open the random number generator to create random offsets
    fdrand = open("/dev/urandom", O_RDONLY);
    // Open the cache cleaner to ensure that the cache is flushed between IO operations
    fdcleancache = open("/proc/sys/vm/drop_caches", O_WRONLY);

    // Perform the specified number of read operations
    for (size_t i = 0; i < nb_run; i++) {
        // Generate a random offset within the file
        read(fdrand, &randnum, sizeof(size_t));
        offset = SECTOR_SIZE * (loff_t)(randnum % (filesize / SECTOR_SIZE + 1 - nb_bloc * sz_bloc / SECTOR_SIZE));
        lseek64(fd, offset, SEEK_SET); // Seek to the calculated offset

        // Perform the read operation for each block
        for (size_t j = 0; j < nb_bloc; j++) {
            gettimeofday(&start, NULL); // Record the start time
            time_epoch_start[i * nb_bloc + j] = start;

            if (read(fd, buffer, sz_bloc) < 0) { // Perform the read operation
                perror("read");
                exit(1);
            }
            gettimeofday(&end, NULL); // Record the end time
            time_epoch_end[i * nb_bloc + j] = end;

            // Calculate the time taken for the read operation and store it
            times[i * nb_bloc + j] = (end.tv_sec - start.tv_sec) * (size_t)(1e6) + (end.tv_usec - start.tv_usec);

            sync(); // Synchronize the filesystem state with storage

            // Flush the cache to ensure that subsequent IO operations are not affected by caching
            if (write(fdcleancache, "3", 1) < 0)
                fprintf(stderr, "cache flush failed, need root\n");
        }
    }

    close(fdrand);
    close(fdcleancache);
    close(fd); // Close the file descriptors
}

void measure_write(char *file_path, char *buffer)
{
    struct timeval start, end;
    size_t randnum;
    loff_t offset;
    int fd, fdrand;
    char timestamp_buffer[128];

    fd = open64(file_path, O_WRONLY | O_SYNC | O_DIRECT);
    if (fd < 0) {
        perror("open");
        exit(1);
    }
    fdrand = open("/dev/urandom", O_RDONLY);

    read(fdrand, buffer, sz_bloc); // Fill the buffer with random data

    for (size_t i = 0; i < nb_run; i++) {
        read(fdrand, &randnum, sizeof(size_t));
        offset = SECTOR_SIZE * (loff_t)(randnum % (filesize / SECTOR_SIZE + 1 - nb_bloc * sz_bloc / SECTOR_SIZE));
        lseek64(fd, offset, SEEK_SET);

        for (size_t j = 0; j < nb_bloc; j++) {
            gettimeofday(&start, NULL);
            time_epoch_start[i * nb_bloc + j] = start;

            if (write(fd, buffer, sz_bloc) < 0) {
                perror("write");
                exit(1);
            }
            gettimeofday(&end, NULL);
            time_epoch_end[i * nb_bloc + j] = end;

            times[i * nb_bloc + j] = (end.tv_sec - start.tv_sec) * (size_t)(1e6) + (end.tv_usec - start.tv_usec);

            sync();
        }
    }

    close(fdrand);
    close(fd);
}

