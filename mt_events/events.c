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

static struct work_struct *stop_kthrd = NULL;


// ----------proc-fs-----------

#include <linux/proc_fs.h>
#include "time_proc.h"
static struct proc_dir_entry *root; // root directory for proc
static struct proc_dir_entry *timef; // time-settings file


// todo ------------------call initial sequence (for, lists, timers, others...) from proc_fs write function (syscall context)
// todo ------------------code work-function to end up the kthread work------------------
// todo ------------------check asyncronious exiting from kthread. (exiting after reschedule roma says)
static void stop_kthread_fn(struct work_struct *w) {
	pr_info("stopping message generator\n");
	kthread_stop(msg_generator);
	pr_info("message generator stopped\n");
	pr_info("starting message processing\n");
	process_events();
	pr_info("messages processed\n");
}



static void t_wrapper(struct timer_list *timer) {
	bool moded = false;
	bool canceled = false;

	pr_info("in t_wrapper\n");

	// kthread stop
	pr_info("setting kt_should_stop\n");
	kt_should_stop = true;
	// kthread_stop(msg_generator);
	pr_info("kt_should_stop set\n");

	pr_info("setting wc_should_stop\n");
	wc_should_stop = true;
	pr_info("wc_should_stop set\n");

	/*pr_info("moding dw\n");
	moded = mod_delayed_work(wq, &dw, 0);
	pr_info("dw moded: %s\n", moded ? "true" : "false");*/

	pr_info("canceling dw\n");
	canceled = cancel_delayed_work(&dw);
	pr_info("dw canceled: %s\n", canceled ? "true" : "false");
	
	// ---fix--- syncronize: process_events should start after ending of thread_fun

	DECLARE_WORK(&stop_kthrd, stop_kthread_fn)

	schedule_work(stop_kthrd);

	// process_events();
}

static void wq_wrapper(struct work_struct *w) {
	unsigned long delay = msecs_to_jiffies(2500) + get_random_int() % msecs_to_jiffies(10); // rand in [500, 1500] milisecs
	pr_info("in wq_wrapper\n");
	if (event_list_leng >= 60) {
		pr_info("in wq_wrapper. list >= 60\n");
		process_events();
	}
	pr_info("checking wc_should_stop\n");
	if (!wc_should_stop) {
		pr_info("queue dw\n");
		queue_delayed_work(wq, to_delayed_work(w), delay);
	}
	else {
		pr_info("chain wq_wrapper stopped\n");
	}
}

int thread_fun(void *data) {
	const unsigned long delay = msecs_to_jiffies(500) + get_random_int() % msecs_to_jiffies(1000); // rand in [500, 1500] milisecs (delay for wq)
	int added = 0;
	event_list_leng = 0;
	printk("i'm kthread\n");

	pr_info("enqueue delayed work\n");
	queue_delayed_work(wq, &dw, delay);


	while (!kthread_should_stop() && !kt_should_stop) {
		// may be better to use some less precise timer
		// usleep_range(200 * 1000, 2000 * 1000); // * 1000 - micro to milli
		usleep_range(200 * 1000, 1000 * 1000); // * 1000 - micro to milli

		if ( !kt_should_stop && (added = create_events()) == -1 ) { // fix mb not good idea "!kt_should_stop" to place here
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

	if (msg_generator == NULL) {
		pr_info("msg_generator = NULL\n");
	}
	else {
		pr_info("msg_generator: %p\n", msg_generator);
	}
	pr_info("checking kt_should_stop\n");
	if (!kt_should_stop) {
		pr_info("kt_should_stop isn't set. stopping kthread\n");
		kthread_stop(msg_generator);
		pr_info("kthread stopped\n");
	}

	del_timer(&timer);
	pr_info("timer deleted\n");


	pr_info("setting wc_should_stop\n");
	wc_should_stop = true;
	pr_info("wc_should_stop set\n");

	pr_info("flushing dw\n");
	flushed = flush_delayed_work(&dw);
	pr_info("delayed work flushed: %s\n", flushed ? "true" : "false");
	// need synchronization with checking this flag in wq_wrapper.
	// when still false, if-statment could pass, then destroy_workqueue, then queue-call in this workqueue

	/*pr_info("wc_should_stop setting\n");
	wc_should_stop = true;
	pr_info("wc_should_stop set\n");*/

	pr_info("delayed work pending: %s\n", delayed_work_pending(&dw) ? "true" : "false");
	// additionaly print wokr_busy func
	// usleep_range(200, 300);
	flush_workqueue(wq);
	// destroy_workqueue(wq);
	pr_info("workqueue flushed\n");

	remove_proc_entry("time", root);
	remove_proc_entry("event_list", NULL);
}

module_init(events_init);
module_exit(events_exit);