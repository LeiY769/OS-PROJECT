/*
 * Description:
 *   Utility functions for the procmon module.
 */
#ifndef UTIL_H
#define UTIL_H

#include "procmem_struct.h"

/*
 * Description:
 *  Compares two strings taking into account possible padding characters.
 * 
 * 
 * Parameters:
 *   - str1: The first string to compare.
 *   - str2: The second string to compare.
 * 
 * Returns:
 *   - 0 if the strings are equal up to the minimum length and any additional
 *     characters in the longer string are padding characters ('\0').
 *   - A value less than 0 if the first differing character in str1 is less
 *     than the corresponding character in str2.
 *   - A value greater than 0 if the first differing character in str1 is greater
 *     than the corresponding character in str2.
 *   - If the strings are equal up to the minimum length but additional characters
 *     in the longer string are not padding characters, the function returns -1.
 * 
 */
int compare_strings_with_padding(const char *str1, const char *str2);

/*
 * Description:
 *   Computes the hash value of a given key.
 *
 * Parameters:
 *   key: The key (process name) for which the hash value is computed.
 *
 * Returns:
 *   The computed hash value.
 */
unsigned long hash(const char *key);

/*
 * Description:
 *   Parses the input command string and determines the command type and process name.
 *
 * Parameters:
 *   input: The input command string to parse.
 *
 * Returns:
 *   A structure containing the parsed command type and process name.
 */
struct command_result parse_command(const char *input);

/*
 * Description:
 *   Prints information about a process to a seq_file.
 *
 * Parameters:
 *   a: Pointer to the seq_file structure to which the information is printed.
 *   process_info: Pointer to the processes_description structure containing the process information.
 */
void print_process_info(struct seq_file *a, struct processes_description *process_info);

/*
 * Description:
 *   Retrieves a page if it is valid (present in RAM) within a given virtual memory area.
 *
 * Parameters:
 *   vma: Pointer to the vm_area_struct representing the virtual memory area.
 *   address: The virtual address of the page to retrieve.
 *
 * Returns:
 *   Pointer to the page if it is valid, NULL otherwise.
 */
struct page *get_page_if_valid(struct vm_area_struct *vma, unsigned long address);

void write_error_message(struct seq_file *a, int error_code);

#endif