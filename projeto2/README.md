# File System Operation Program

This C program processes file system commands (create, delete, move, lookup) using multithreading, with support for file and directory operations in a synchronized environment.

## Compilation and Usage

1. Compile:
   ```bash
   gcc -pthread -o fs_operation_program main.c fs/operations.c -I fs/


2. Run:
    ```bash
    ./fs_operation_program <input_file> <output_file> <num_threads>

    <input_file>: Command file (see format below).
<output_file>: File for the output tree.
<num_threads>: Number of threads to process commands.

## Command File Format

c <name> <type> - Create file (type f) or directory (type d)
l <name> - Lookup a file or directory
d <name> - Delete a file or directory
m <name> <new_path> - Move a file or directory
# - Comments

## Example Usage

c /home f
c /home/user d
l /home
m /home/user /home/guest
d /home