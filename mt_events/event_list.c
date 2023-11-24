#include <linux/list.h>
#include <linux/slab.h>
#include <linux/random.h>
#include "event_list.h"

struct list_head event_list;
int event_list_leng;



/*void event_list_run(struct test_event *event) {
	struct list_head *node = NULL;

	list_for_each(node, event_list) {

	}
}*/

void event_list_add(struct test_event *event) {
	list_add_tail(&event->list, &event_list);
}

void event_list_del(struct test_event *event) {
	list_del(&event->list);
	kfree(event);
}

int create_events(void) {
	int events_amo = get_random_int() % 5 + 1;
	int i = 0;
	for (i = 0; i < events_amo; i++) {
		struct test_event *event = kmalloc(sizeof(struct test_event), GFP_KERNEL);
		if (!event) {
			pr_info("kmalloc failed\n");
			return -1;
		}
		event->data = get_random_int() % 100;
		event_list_add(event);
		pr_info("one added\n");
	}
	return events_amo;
}

// returns sum of data's
void process_events(void) {
	struct list_head *ptr = NULL;
	struct list_head *tmp = NULL;
	struct test_event *entry;

	int sum = 0;

	// add list node removing
	list_for_each_safe(ptr, tmp, &event_list) {
		entry = list_entry(ptr, struct test_event, list);
		sum += (int)(entry->data);
		event_list_del(entry);
	}

	pr_info("list processed\n");
	pr_info("sum: %d\n", sum);

	event_list_leng = 0;
}