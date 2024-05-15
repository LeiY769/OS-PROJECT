#include "util.h"
#include "procmem_struct.h"

/* define constante */

#define PSEUDO_FILE_NAME "memory_info" // name of the proc entry
#define HASHTABLE_SIZE 3

/* module parameters */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dario Rinallo");
MODULE_AUTHOR("Lei Yang");
MODULE_DESCRIPTION("The procmon module enables monitoring and collecting information about running processes on the Linux system. It creates and populates an in-memory data structure with the names of all currently running processes. For each set of processes with the same name, the module stores memory-related information, providing insights into the system resource usage by different processes.");

/* global variables */

static char *command_string = NULL;
DEFINE_HASHTABLE(processes_hash_table, HASHTABLE_SIZE);

/* read the process information and put them on the hashtable */
static int fill_dictionary(void)
{
    struct task_struct *task;
    unsigned long key;

    for_each_process(task)
    {
        struct mm_struct *mm = task->mm;
        if (task && mm)
        {
            struct processes_description *existing_process_info = NULL;
            struct pid_item *pid_item;
            char process_name[16];
            struct vm_area_struct *vma;
            unsigned long page_address;
            int valid_page_count = 0;
            int unvalid_page_count = 0;

            snprintf(process_name, sizeof(process_name), task->comm);
            key = hash(process_name);

            // Check if the process name is already in the hash table
            hash_for_each_possible(processes_hash_table, existing_process_info, hash_list_iterator, key)
            {
                //if (strncmp(existing_process_info->process_name, process_name, sizeof(process_name)) == 0)
                if (compare_strings_with_padding(existing_process_info->process_name, process_name) == 0)
                {
                    break;
                }
            }

            if (existing_process_info)
            {
                // If the process name is already in the hash table, add the PID to the PID list
                // and add the total number of pages
                pid_item = kmalloc(sizeof(*pid_item), GFP_KERNEL);
                if (!pid_item)
                {
                    printk(KERN_ERR "[ERROR]: Memory allocation error\n");
                    return -ENOMEM;
                }

                pid_item->pid = task->pid;
                list_add(&pid_item->next, &existing_process_info->pids);

                existing_process_info->numbers_pids++;

                existing_process_info->nb_total_pages += mm->total_vm;

                for (vma = mm->mmap; vma; vma = vma->vm_next)
                {
                    for (page_address = vma->vm_start; page_address < vma->vm_end; page_address += PAGE_SIZE)
                    {
                        struct page *page = get_page_if_valid(vma, page_address);
                        if (page)
                        {
                            valid_page_count++;
                        }
                        else
                        {
                            unvalid_page_count++;
                        }
                    }
                }

                existing_process_info->nb_valid_pages += valid_page_count;
                existing_process_info->nb_invalid_pages += unvalid_page_count;
            }
            else
            {
                // If the process name is not in the hash table, add a new process
                struct processes_description *process_info = kmalloc(sizeof(*process_info), GFP_KERNEL);
                pid_item = kmalloc(sizeof(*pid_item), GFP_KERNEL);
                if (!process_info)
                {
                    printk(KERN_ERR "[ERROR]: Memory allocation error\n");
                    return -ENOMEM;
                }

                INIT_LIST_HEAD(&process_info->pids); // Initialize the PID list

                process_info->numbers_pids = 0;

                if (!pid_item)
                {
                    kfree(process_info);
                    printk(KERN_ERR "[ERROR]: Memory allocation error\n");
                    return -ENOMEM;
                }

                pid_item->pid = task->pid;
                list_add(&pid_item->next, &process_info->pids); // Add PID to list
                process_info->numbers_pids++;

                process_info->nb_total_pages = mm->total_vm;

                for (vma = mm->mmap; vma; vma = vma->vm_next)
                {
                    for (page_address = vma->vm_start; page_address < vma->vm_end; page_address += PAGE_SIZE)
                    {
                        struct page *page = get_page_if_valid(vma, page_address);
                        if (page)
                        {
                            valid_page_count++;
                        }
                        else
                        {
                            unvalid_page_count++;
                        }
                    }
                }

                process_info->nb_valid_pages = valid_page_count;
                process_info->nb_invalid_pages = unvalid_page_count;
                process_info->nb_shareable_pages = 0;
                process_info->nb_group = 0;
                snprintf(process_info->process_name, sizeof(process_info->process_name), task->comm);
                INIT_HLIST_NODE(&process_info->hash_list_iterator);

                key = hash(process_info->process_name);
                hash_add(processes_hash_table, &process_info->hash_list_iterator, key);
            }
        }
    }

    return 0;
}

/* Function to clear the dictionary */
static void clear_dictionary(void)
{
    struct processes_description *tmp_process;
    struct hlist_node *tmp2;
    int bkt;

    // Delete all remaining items
    hash_for_each_safe(processes_hash_table, bkt, tmp2, tmp_process, hash_list_iterator)
    {
        struct pid_item *pid_item, *tmp_pid;

        // Release all PID from the list
        list_for_each_entry_safe(pid_item, tmp_pid, &tmp_process->pids, next)
        {
            list_del(&pid_item->next);
            kfree(pid_item);
        }

        hash_del(&tmp_process->hash_list_iterator);
        kfree(tmp_process);
    }
}

/* this function writes a message to the pseudo file system */
static ssize_t write_msg(struct file *file, const char __user *buff, size_t cnt, loff_t *f_pos)
{
    // allocate memory, (size and flag) - flag: type of memory (kernel memory)
    char *tmp = kzalloc(cnt + 1, GFP_KERNEL);
    if (!tmp)
    {
        printk(KERN_ERR "[ERROR]: Memory allocation error\n");
        return -ENOMEM;
    }

    // copy data from user space to kernel space by using copy_from_user
    if (copy_from_user(tmp, buff, cnt))
    {
        kfree(tmp);
        printk(KERN_ERR "[ERROR]: Bad address - copy_from_user\n");
        return -EFAULT;
    }

    if (command_string)
    {
        kfree(command_string);
    }
    command_string = tmp;
    return cnt;
}

/* this function reads a message from the pseudo file system via the seq_printf function */
static int show_the_proc(struct seq_file *a, void *v)
{
    struct command_result result = parse_command(command_string);
    struct processes_description *process_info;
    int bkt;
    unsigned long key;
    bool found = false;

    // Afficher le résultat
    switch (result.cmd)
    {
    case CMD_RESET:
        // Clear the existing entries in the hashtable
        clear_dictionary();
        // Repopulate the hashtable with process information
        if (fill_dictionary() != 0)
        {
            seq_printf(a, "[ERROR]: Failed to reset the process information\n");
        }
        else
        {
            seq_printf(a, "[SUCCESS]\n");
        }
        break;
    case CMD_ALL:
        hash_for_each(processes_hash_table, bkt, process_info, hash_list_iterator)
        {
            print_process_info(a, process_info);
        }

        break;
    case CMD_FILTER:

        key = hash(result.process_name);

        hash_for_each_possible(processes_hash_table, process_info, hash_list_iterator, key)
        {
            if (compare_strings_with_padding(process_info->process_name, result.process_name) == 0)
            {
                found = true;
                print_process_info(a, process_info);
            }
        }

        if (!found)
        {
            seq_printf(a, "[ERROR]: No such process\n");
            printk(KERN_ERR "[ERROR]: No such process\n");
            //return -ENOENT;
        }
        break;
    case CMD_DEL:

        key = hash(result.process_name);

        hash_for_each_possible(processes_hash_table, process_info, hash_list_iterator, key)
        {
            if (compare_strings_with_padding(process_info->process_name, result.process_name) == 0)
            {
                found = true;
                hash_del(&process_info->hash_list_iterator);
                kfree(process_info);

                // Write success message to pseudo file
                seq_printf(a, "[SUCCESS]\n");
                break;
            }
        }

        if (!found)
        {
            // Write error message to pseudo file
            seq_printf(a, "[ERROR]: No such process\n");
            printk(KERN_ERR "[ERROR]: No such process\n");
            //return -ENOENT;
        }
        break;
    default:
        seq_printf(a, "[ERROR]: Invalid argument\n");
        printk(KERN_ERR "[ERROR]: Invalid argument\n");
        //return -EINVAL;
        break;
    }

    return 0;
}

/* this function opens the proc entry by calling the show_the_proc function */
static int open_the_proc(struct inode *inode, struct file *file)
{
    if (!file)
    {
        printk(KERN_ERR "[ERROR]: No such file or directory\n");
        return -ENOENT;
    }
    return single_open(file, show_the_proc, NULL);
}

/*-----------------------------------------------------------------------*/
// Structure that associates a set of function pointers (e.g., device_open)
// that implement the corresponding file operations (e.g., open).
/*-----------------------------------------------------------------------*/
static struct file_operations new_fops = {
    // defined in linux/fs.h
    .owner = THIS_MODULE,
    .open = open_the_proc, // open callback
    .release = single_release,
    .read = seq_read,   // read
    .write = write_msg, // write callback
    .llseek = seq_lseek,
};

static int __init procmeminfo_init(void)
{
    // create proc entry with read/write functionality
    struct proc_dir_entry *entry = proc_create(PSEUDO_FILE_NAME, 0777, NULL, &new_fops);

    if (!entry)
    {
        return -1;
    }
    else
    {
        printk(KERN_INFO "Init Module [OK]\n");
    }

    /* read the process information and put them on the hashtable */
    return fill_dictionary();
}

static void __exit procmeminfo_exit(void)
{
    if (command_string)
    {
        kfree(command_string);
    }

    clear_dictionary();

    //  remove proc entry
    remove_proc_entry(PSEUDO_FILE_NAME, NULL);
    printk(KERN_INFO "Exit Module [OK]\n");
}

module_init(procmeminfo_init);
module_exit(procmeminfo_exit);