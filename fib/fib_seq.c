// #include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>

extern int limit;

struct fibs {
	int prev;
	int curr;
};

static void *fib_start(struct seq_file *seq, loff_t *pos) {
	struct fibs *fib = kmalloc(sizeof(struct fibs), GFP_KERNEL);

	if (!pos || *pos > limit) {
		return NULL;
	}
	
	fib->prev = 0;
	fib->curr = 1;

	return fib;
}

static void *fib_next(struct seq_file *seq, void *prev, loff_t *pos) {
	struct fibs *fib = prev;
	int fib_curr = 0;

	if (*pos > limit) {
		return NULL;
	}

	(*pos)++; // can i ignore this? and keep without changes. just use only prev. (cat I track position by adding "pos" in fibs struct?)

	fib_curr = fib->curr;
	fib->curr += fib->prev; // now it's next fib num
	fib->prev = fib_curr;

	return fib;
}

static void fib_stop(struct seq_file *seq, void *curr) {
	kfree(curr);
}

static int fib_show(struct seq_file *seq, void *curr) {
	struct fibs *fib = (struct fibs *)curr;
	seq_printf(seq, "%d ", fib->curr);
	return 0; // or error code
}

static const struct seq_operations fib_seq_ops = {
	.start = fib_start,
	.next = fib_next,
	.stop = fib_stop,
	.show = fib_show
};

static int fib_open(struct inode *inode, struct file *file) {
	return seq_open(file, &fib_seq_ops);
}

const struct file_operations fib_file_ops = {
	.owner = THIS_MODULE,
	.open = fib_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release
};