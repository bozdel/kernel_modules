#include <linux/seq_file.h>
#include <linux/uaccess.h> // copy_from_user copy_to_user
#include <linux/kernel.h> // sscanf
#include <linux/string.h>

#define BUFSIZE 4096

extern int limit;

int settings_show(struct seq_file *seq, void *data) {
	seq_printf(seq, "fibonacci sequence size - %d\n", limit);
	return 0;
}


// am i need a buffer (should i use single_open_size?)?

static int settings_open(struct inode *inode, struct file *file) {
	return single_open_size(file, settings_show, NULL, BUFSIZE);
}

static ssize_t settings_write(struct file *file, const char __user *buf, size_t size, loff_t *ppos) {
	struct seq_file *m = file->private_data;

	if (*ppos > 0 || size > BUFSIZE) {
		pr_info("invalid argument\n");
		return -EINVAL;
	}
	
	if (copy_from_user(m->buf, buf, size)) {
		pr_info("copy\n");
		return -EFAULT;
	}

	if (sscanf(m->buf, "%d\n", &limit) != 1) {
		pr_info("sscanf\n");
		return -EFAULT;
	}

	*ppos += size;

	return size;
}

const struct file_operations settings_file_ops = {
	.owner = THIS_MODULE,
	.open = settings_open,
	.read = seq_read,
	.write = settings_write,
	.llseek = seq_lseek,
	.release = single_release
};