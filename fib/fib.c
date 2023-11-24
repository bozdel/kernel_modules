#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("vyacheslav bondarev");

extern const struct file_operations fib_file_ops;
extern const struct file_operations settings_file_ops;

int limit = 10;

static struct proc_dir_entry *dir;
static struct proc_dir_entry *fib;
static struct proc_dir_entry *settings;

static int __init proc_init(void) {
	pr_info("module proc init\n");

	pr_info("call create_proc_entry\n");
	dir = proc_mkdir("fib", NULL);
	fib = proc_create("fib", 0, dir, &fib_file_ops);
	settings = proc_create("settings", S_IWUSR | S_IRUSR, dir, &settings_file_ops);


	return 0;
}

static void __exit proc_exit(void) {
	pr_info("module proc exit\n");

	// look at remove_proc_subtree. maybe it will be better to use (just one call)
	remove_proc_entry("fib", dir);
	remove_proc_entry("settings", dir);
	remove_proc_entry("fib", NULL);

}

module_init(proc_init);
module_exit(proc_exit);