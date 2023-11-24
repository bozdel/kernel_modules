#include <linux/module.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/delay.h> // usleep_range
#include <linux/jiffies.h> // msecs_to_jiffies

MODULE_LICENSE("GPL");
MODULE_AUTHOR("v bondarev");

struct workqueue_struct *wq;
struct work_struct w;
struct delayed_work dw;

static int chain_size = 0;

static bool should_stop = false;

void work_fun(struct work_struct *w) {
	bool is_queued = 0;
	chain_size++;
	pr_info("i'm work function\n");
	usleep_range(500 * 1000, 1000 * 1000);
	if (chain_size < 15) {
		is_queued = queue_work(wq, w);
		pr_info("is_queued: %d\n", is_queued);
	}
	else {
		pr_info("finishing work\n");
	}
}

void work_fun_continiuos(struct work_struct *w) {
	bool is_queued = 0;
	int counter = 0;
	chain_size++;
	pr_info("i'm continious work function\n");

	while (counter < 5) {
		pr_info("local counter: %d/5\n", counter);
		usleep_range(200 * 1000, 201 * 1000);
		counter++;
	}

	if (chain_size < 15) {
		is_queued = queue_work(wq, w);
		pr_info("is_queued: %d\n", is_queued);
	}
	else {
		pr_info("finishing continiuos work\n");
	}
}

void work_fun_delayed(struct work_struct *w) {
	bool is_queued = 0;
	chain_size++;
	pr_info("i'm delayed work function\n");
	if (chain_size < 15) {
		is_queued = queue_delayed_work(wq, to_delayed_work(w), msecs_to_jiffies(1000));
		pr_info("is_queued: %s\n", is_queued ? "true" : "false");
	}
	else {
		pr_info("finishing delayed work\n");
	}
}

void work_fun_delayed_continious(struct work_struct *w) {
	bool is_queued = 0;
	int counter = 0;
	chain_size++;
	pr_info("i'm delayed continious work function\n");

	while (counter < 5) {
		pr_info("local counter: %d/5\n", counter);
		usleep_range(200 * 1000, 201 * 1000);
		counter++;
	}

	pr_info("\tfinishing cycle\n");

	// should_stop prevent queueing at destroyed queue
	// it can happen if module exiting (cancel_delayed_work called) at execution time of this func

	// maybe good idea to spinlock @should_stop before queue_delayed_work and unlock after this call.
	// it possible will allow to call it from irq safely.
	if (chain_size < 5 && !should_stop) {
		is_queued = queue_delayed_work(wq, to_delayed_work(w), msecs_to_jiffies(1000));
		pr_info("is_queued: %s\n", is_queued ? "true" : "false");
	}
	else {
		pr_info("finishing delayed work\n");
	}
}

void work_fun_delayed_continious2(struct work_struct *w) {
	bool is_queued = 0;
	int counter = 0;
	chain_size++;
	pr_info("i'm delayed continious work function\n");

	while (counter < 5) {
		pr_info("local counter: %d/5\n", counter);
		usleep_range(200 * 1000, 201 * 1000);
		counter++;
	}

	pr_info("\tfinishing cycle\n");

	if (chain_size < 5) {
		is_queued = queue_delayed_work(wq, to_delayed_work(w), msecs_to_jiffies(1000));
		pr_info("is_queued: %s\n", is_queued ? "true" : "false");
	}
	else {
		pr_info("finishing delayed work\n");
	}
}

static int __init init_workqueue(void) {
	pr_info("initing workqueue module\n");

	// INIT_WORK(&w, work_fun_continiuos);

	INIT_DELAYED_WORK(&dw, work_fun_delayed_continious2);



	wq = create_workqueue("my_wq");



	// queue_work(wq, &w);
	// should_stop = false;
	queue_delayed_work(wq, &dw, msecs_to_jiffies(1000));

	return 0;
}

static void __exit exit_workqueue(void) {
	bool canceled = false;
	pr_info("workqueue module exiting\n");

	// --------continiuos delayed work-------
	// should_stop = true; // should_stop = !cancel_delayed_work(&dw);
	// canceled = cancel_delayed_work(&dw);
	// pr_info("canceled: %s\n", canceled ? "true" : "false");


	// --------continiuos work---------
	/*pr_info("calling cancel_work_sync\n");
	cancel_work_sync(&w);
	pr_info("cancel_work_sync exited\n");*/


	// --------continious delayed work-----
	pr_info("calling cancel_delayed_work_sync\n");
	canceled = cancel_delayed_work_sync(&dw);
	pr_info("cancel_delayed_work_sync exited with return value: %s\n", canceled ? "true" : "false");


	pr_info("destroying workqueue\n");
	destroy_workqueue(wq);
	pr_info("workqueue destroyed\n");



	pr_info("end module exiting\n");
}

module_init(init_workqueue);
module_exit(exit_workqueue);