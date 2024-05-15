#include <linux/string.h>
#include <linux/kernel.h> // kernel logging
#include <linux/errno.h>

#include "util.h"

int compare_strings_with_padding(const char *str1, const char *str2)
{
    size_t length1 = strlen(str1);
    size_t length2 = strlen(str2);
    size_t min_length = (length1 < length2) ? length1 : length2;
    size_t max_length = (length1 < length2) ? length2 : length1;

    int result = strncmp(str1, str2, min_length);

    if (result == 0)
    {
        for (size_t i = min_length; i < max_length; i++)
        {
            if (str1[i] != '\0' && str2[i] != '\0')
            {
                return -1;
            }
        }
    }

    return result;
}

unsigned long hash(const char *key)
{
    size_t length = strnlen(key, 16);
    unsigned long sum = 0;
    unsigned long base = 1;
    unsigned long mod = 1000000007;

    if (!key)
    {
        printk(KERN_ERR "[ERROR] Invalid argument: key is NULL\n");
        return -EINVAL;
    }

    for (size_t i = 0; i < length; i++)
    {
        sum = (sum + base * key[i]) % mod;
        base = (base * 256) % mod;
    }

    return sum;
}

struct command_result parse_command(const char *input)
{
    struct command_result result;
    char *delimiter;

    // Initialize the result
    result.cmd = CMD_UNKNOWN;

    // Check each commands
    if (compare_strings_with_padding(input, "RESET") == 0)
    {
        result.cmd = CMD_RESET;
    }
    else if (compare_strings_with_padding(input, "ALL") == 0)
    {
        result.cmd = CMD_ALL;
    }
    else if ((delimiter = strstr(input, "|")) != NULL)
    {
        if (strncmp(input, "FILTER", delimiter - input) == 0 && (delimiter - input) == strlen("FILTER"))
        {
            result.cmd = CMD_FILTER;
        }
        else if (strncmp(input, "DEL", delimiter - input) == 0 && (delimiter - input) == strlen("DEL"))
        {
            result.cmd = CMD_DEL;
        }

        // copy process name
        strncpy(result.process_name, delimiter + 1, sizeof(result.process_name) - 1);
    }

    return result;
}

void print_process_info(struct seq_file *a, struct processes_description *process_info)
{
    int first = 1; // Control variable to indicate the first element
    struct pid_item *pid_item;

    seq_printf(a, "%s, ", process_info->process_name);
    seq_printf(a, "total: %lu, ", process_info->nb_total_pages);
    seq_printf(a, "valid: %lu, ", process_info->nb_valid_pages);
    seq_printf(a, "invalid: %lu, ", process_info->nb_invalid_pages);
    seq_printf(a, "may_be_shared: %lu, ", process_info->nb_shareable_pages);
    seq_printf(a, "nb_group: %lu, ", process_info->nb_group);
    seq_printf(a, "pid(%d): ", process_info->numbers_pids);
    list_for_each_entry(pid_item, &process_info->pids, next)
    {
        if (first)
        {
            seq_printf(a, "%d", pid_item->pid);
            first = 0; // Set the control variable to 0 after printing the first item
        }
        else
        {
            seq_printf(a, "; %d", pid_item->pid);
        }
    }

    seq_printf(a, "\n");
}

struct page *get_page_if_valid(struct vm_area_struct *vma, unsigned long address)
{
    pgd_t *pgd;
    p4d_t *p4d;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
    struct page *page = NULL;
    struct mm_struct *mm = vma->vm_mm;

    pgd = pgd_offset(mm, address);
    if (pgd_none(*pgd) || unlikely(pgd_bad(*pgd)))
    {
        return NULL;
    }

    p4d = p4d_offset(pgd, address);
    if (p4d_none(*p4d) || unlikely(p4d_bad(*p4d)))
    {
        return NULL;
    }

    pud = pud_offset(p4d, address);
    if (pud_none(*pud) || unlikely(pud_bad(*pud)))
    {
        return NULL;
    }

    pmd = pmd_offset(pud, address);
    if (pmd_none(*pmd) || unlikely(pmd_bad(*pmd)))
    {
        return NULL;
    }

    pte = pte_offset_map(pmd, address);
    if (!pte)
    {
        return NULL;
    }

    if (pte_present(*pte))
    {
        page = pte_page(*pte);
    }
    pte_unmap(pte);

    return page;
}

void write_error_message(struct seq_file *a, int error_code) {
    switch (error_code) {
        case -EINVAL:
            seq_printf(a, "[ERROR]: Invalid argument\n");
            break;
        case -ENOENT:
            seq_printf(a, "[ERROR]: No such file or directory\n");
            break;
        case -EFAULT:
            seq_printf(a, "[ERROR]: Bad address\n");
            break;
        case -ESRCH:
            seq_printf(a, "[ERROR]: No such process\n");
            break;
        case -ENOMEM:
            seq_printf(a, "[ERROR]: Memory allocation error\n");
            break;
        default:
            seq_printf(a, "[ERROR]: Unknown error\n");
            break;
    }
    return;
}