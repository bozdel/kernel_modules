#include <linux/seq_file.h>
#include <linux/uaccess.h> // copy_from_user copy_to_user
#include <linux/workqueue.h>
#include "time_proc.h"

#define BUFSIZE 1000 // move to header

extern struct work_struct stop_work;

int time = 13; // default value for timer expiration
int cmd = RUN;

static const char *cmds_strings[3] = {
	"stop",
	"run",
	"err"
};

int time_show(struct seq_file *seq, void *data) {
	seq_printf(seq, "time: %d\n", time);
	seq_printf(seq, "state: %s\n", cmds_strings[cmd]);
	return 0;
}

static int time_open(struct inode *inode, struct file *file) {
	return single_open_size(file, time_show, NULL, BUFSIZE);
}

static ssize_t time_write(struct file *file, const char __user *buf, size_t size, loff_t *ppos) {
	struct seq_file *m = file->private_data;
	char cmd_buff[20] = { 0 };

	if (*ppos > 0 || size > BUFSIZE) {
		pr_info("invalid argument\n");
		return -EINVAL;
	}
	
	if (copy_from_user(m->buf, buf, size)) {
		return -EFAULT;
	}

	if (sscanf(m->buf, "%s %d\n", cmd_buff, &time) < 1) {
		return -EFAULT;
	}

	if (strcmp(cmd_buff, cmds_strings[STOP]) == 0) {
		cmd = STOP;
		schedule_work(&stop_work);
	}
	else if (strcmp(cmd_buff, cmds_strings[RUN]) == 0) {
		cmd = RUN;
	}
	else {
		cmd = ERR;
	}

	*ppos += size;

	return size;
}



const struct file_operations time_file_ops = {
	.owner = THIS_MODULE,
	.open = time_open,
	.read = seq_read,
	.write = time_write,
	.llseek = seq_lseek,
	.release = single_release
};