#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/list.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/jiffies.h> // msecs_to_jiffies
#include <linux/workqueue.h>
#include "event_list.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("vyacheslav bondarev");

extern struct list_head event_list;
extern int event_list_leng;

static struct timer_list timer;

static struct delayed_work dw;
static struct workqueue_struct *wq;
static bool wc_should_stop = false; // (Work Chain) for correct stoping wq_wrapper. see wq module

static bool kt_should_stop = false; // (KThread) for stopping from t_wrapper (atomic context?)

static struct task_struct *msg_generator = NULL;

// static struct work_struct stop_work;
static void stop_fn(struct work_struct *w);
DECLARE_WORK(stop_work, stop_fn);


// ----------proc-fs-----------

#include <linux/proc_fs.h>
#include "time_proc.h"
static struct proc_dir_entry *root; // root directory for proc
static struct proc_dir_entry *timef; // time-settings file

extern int cmd; // variable from time_proc

// ----------proc-fs-----------


// todo ------------------call initial sequence (for, lists, timers, others...) from proc_fs write function (syscall context)
// todo ------------------code work-function to end up the kthread work------------------
// todo ------------------check asyncronious exiting from kthread. (exiting after reschedule roma says)
static void stop_fn(struct work_struct *w) {
	bool canceled = false;
	int dbg_counter = 0;

	pr_info("in stop_fn\n");

	pr_info("canceling delayed work\n");
	canceled = cancel_delayed_work_sync(&dw);
	pr_info("delayed work canceled: %s\n", canceled ? "true" : "false");

	pr_info("stopping message generator\n");
	if (!kt_should_stop) {
		kt_should_stop = true; // bool var for enshureing that kthread_stop will be called only once
		pr_info("generator isn't stopped yet. stopping\n");
		kthread_stop(msg_generator);
	}
	pr_info("message generator stopped\n");

	pr_info("starting message processing\n");
	process_events();
	pr_info("messages processed\n");

	while (dbg_counter < 10) {
		pr_info("still in stop_fn\n");
		usleep_range(200 * 1000, 201 * 1000);
		dbg_counter++;
	}
	pr_info("exiting from stop_fn\n");
	cmd = STOP;
}



static void t_wrapper(struct timer_list *timer) {
	pr_info("in t_wrapper\n");

	schedule_work(&stop_work);
}

static void wq_wrapper(struct work_struct *w) {
	bool is_queued = false;
	unsigned long delay = msecs_to_jiffies(2500) + get_random_int() % msecs_to_jiffies(10); // rand in [500, 1500] milisecs
	pr_info("in wq_wrapper\n");
	if (event_list_leng >= 60) {
		pr_info("in wq_wrapper. list >= 60\n");
		process_events();
	}

	pr_info("queue dw\n");
	is_queued = queue_delayed_work(wq, to_delayed_work(w), delay);
	pr_info("wq_wrapper queued: %s\n", is_queued ? "true" : "false");
}

int thread_fun(void *data) {
	const unsigned long delay = msecs_to_jiffies(500) + get_random_int() % msecs_to_jiffies(1000); // rand in [500, 1500] milisecs (delay for wq)
	int added = 0;
	event_list_leng = 0;
	printk("i'm kthread\n");

	pr_info("enqueue delayed work\n");
	queue_delayed_work(wq, &dw, delay);


	while (!kthread_should_stop()) {
		// may be better to use some less precise timer
		// usleep_range(200 * 1000, 2000 * 1000); // * 1000 - micro to milli
		usleep_range(200 * 1000, 1000 * 1000); // * 1000 - micro to milli

		if ( (added = create_events()) == -1 ) { // fix mb not good idea "!kt_should_stop" to place here
			pr_info("create_events failed\n");
			// return -1;
			// break; // mb good solution
		}

		event_list_leng += added;
		pr_info("list leng: %d\n", event_list_leng);
	}

	// kthread_park // mb good solution
	// while (!kthread_should_stop()) ; bad solution

	return 0;
}




static int __init events_init(void) {

	// -------------proc----------------

	root = proc_mkdir("event_list", NULL);
	timef = proc_create("time", S_IROTH | S_IWOTH, root, &time_file_ops);




	// -------------list----------------

	INIT_LIST_HEAD(&event_list);

	// -------------timer---------------

	timer_setup(&timer, t_wrapper, 0);

	// -----------workqueue-------------

	INIT_DELAYED_WORK(&dw, wq_wrapper);
	pr_info("creating workqueue\n");
	wq = create_workqueue("events_module_queue");

	// -----------kthread---------------

	kt_should_stop = false;
	pr_info("kthread starting\n");
	msg_generator = kthread_run(thread_fun, NULL, "thrd_1");

	// ---------timer setting-----------

	pr_info("setting timer\n");
	timer.expires = jiffies + msecs_to_jiffies(time * 1000); // * 1000 - msecs to secs
	add_timer(&timer);
	pr_info("timer set\n");

	return 0;
}

static void __exit events_exit(void) {
	bool flushed = false;


	schedule_work(&stop_work);
	flush_work(&stop_work);

	del_timer(&timer);
	pr_info("timer deleted\n");


	remove_proc_entry("time", root);
	remove_proc_entry("event_list", NULL);
}

module_init(events_init);
module_exit(events_exit);