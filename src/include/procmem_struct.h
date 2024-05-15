
/*
 * Description:
 *   the structure and enumeration definitions for the procmon module.
 */
#ifndef PROCMEM_STRUCT_H
#define PROCMEM_STRUCT_H

#include <linux/fs.h>       // file operations
#include <linux/proc_fs.h>  // proc_create, proc_ops
#include <linux/uaccess.h>  // copy_from_user, copy_to_user
#include <linux/init.h>     // kernel initialization
#include <linux/seq_file.h> // seq_read, seq_lseek, single_open, single_release
#include <linux/module.h>   // all modules need this
#include <linux/slab.h>     // memory allocation (kmalloc/kzalloc)
#include <linux/kernel.h>   // kernel logging
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/hashtable.h>
#include <linux/list.h>
#include <linux/string.h>
#include <asm/pgtable.h>
#include <linux/highmem.h>

/*
 * Description:
 *   Structure representing a single PID item in a linked list.
 *   Contains the PID value and a pointer to the next item in the list.
 */
struct pid_item
{
    int pid;               // The PID value
    struct list_head next; // Pointer to the next item in the list
};

/*
 * Description:
 *   Structure representing a set of processes with similar characteristics.
 *   Stores information about the processes, such as their PIDs and memory usage.
 */
struct processes_description
{
    struct list_head pids;                // Head of the linked list containing the PIDs of all processes in the set
    int numbers_pids;                     // Number of PIDs in the set
    unsigned long nb_total_pages;         // Total number of pages used by all processes in the set
    unsigned long nb_valid_pages;         // Number of valid pages (present in RAM) used by all processes in the set
    unsigned long nb_invalid_pages;       // Number of invalid pages (not present in RAM) used by all processes in the set
    unsigned long nb_shareable_pages;     // Number of shareable read-only and valid pages between processes in the set
    unsigned long nb_group;               // Number of groups of identical read-only and valid pages in the set
    char process_name[16];                // Name of the process
    struct hlist_node hash_list_iterator; // Iterator for hashing the structure in a hash table
};

/*
 * Description:
 *   Enumeration for the possible input commands.
 */
enum command_type
{
    CMD_RESET,  // Command to reset the data structure
    CMD_ALL,    // Command to retrieve information about all processes
    CMD_FILTER, // Command to filter processes based on a specified criterion that is the name of the process
    CMD_DEL,    // Command to delete a specific process based on the name
    CMD_UNKNOWN // Command type is unknown or invalid
};

/*
 * Description:
 *   Structure representing information about an input command.
 *   Contains the command type and additional information, such as the process name.
 */
struct command_result
{
    enum command_type cmd; // The type of command
    char process_name[16]; // Name of the process associated with the command
};

#endif // PROCMEM_STRUCT_H