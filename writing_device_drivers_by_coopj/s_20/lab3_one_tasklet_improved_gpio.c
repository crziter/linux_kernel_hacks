/* **************** LDD:2.0 s_20/lab3_one_tasklet_improved_gpio.c **************** */
/*
 * The code herein is: Copyright Jerry Cooperstein, 2012
 *
 * This Copyright is retained for the purpose of protecting free
 * redistribution of source.
 *
 *     URL:    http://www.coopj.com
 *     email:  coop@coopj.com
 *
 * The primary maintainer for this code is Jerry Cooperstein
 * The CONTRIBUTORS file (distributed with this
 * file) lists those known to have contributed to the source.
 *
 * This code is distributed under Version 2 of the GNU General Public
 * License, which you should have received with the source.
 *
 */
/*
 * Producer/Consumer (tasklet solution)
 *
 * You may have noticed that
 * you lost some bottom halves. This will happen when more than one
 * interrupt arrives before bottom halves are accomplished. For
 * instance, the same tasklet can only be queued up twice.
 *
 * Write a bottom half that can "catch up"; i.e., consume more than
 * one event when it is called, cleaning up the pending queue.  Do
 * this for at least one of the previous solutions.
 @*/

#include <linux/module.h>
#include "lab_one_interrupt_gpio.h"
#include <linux/gpio.h>

static atomic_t nevents;	/* number of events to deal with */
static atomic_t catchup;	/* number of 'missed events' */

static void t_fun(unsigned long t_arg)
{
	struct my_dat *data = (struct my_dat *)t_arg;

	/* did we get a spurious interrupt, or was it queued too late? */
	if (atomic_read(&nevents) <= 0)
		return;

	for (;;) {
		atomic_inc(&counter_bh);
		pr_info
		    ("In BH: counter_th = %d, counter_bh = %d, jiffies=%ld, %ld\n",
		     atomic_read(&counter_th), atomic_read(&counter_bh),
		     data->jiffies, jiffies);
		if (atomic_dec_and_test(&nevents))
			break;
		atomic_inc(&catchup);
		pr_info("****** nevents > 0, catchup=%d\n",
			atomic_read(&catchup));
	}
}

/* initialize tasklet */
static DECLARE_TASKLET(t_name, t_fun, (unsigned long)&my_data);

static irqreturn_t my_interrupt(int irq, void *dev_id)
{
	struct my_dat *data = (struct my_dat *)dev_id;
	atomic_inc(&counter_th);
	atomic_inc(&nevents);
	data->jiffies = jiffies;
	tasklet_schedule(&t_name);
	mdelay(delay);		/* hoke up a delay to try to cause pileup */
	return IRQ_NONE;	/* we return IRQ_NONE because we are just observing */
}

static int __init my_init(void)
{
	int ret;

	atomic_set(&catchup, 0);
	atomic_set(&nevents, 0);

	ret = gpio_request_one(switch_gpio, GPIOF_IN, "test button");
	if (ret < 0) { 
		pr_info("failed to request GPIO %d, error %d\n", switch_gpio, ret);
		return -1;
	}

#if 0
	ret = gpio_direction_input(switch_gpio);
	if (ret < 0) { 
		pr_info("failed to input GPIO %d, error %d\n", switch_gpio, ret);
		return -1;
	}
#endif

	irq = gpio_to_irq(switch_gpio);
	if (irq < 0) { 
		pr_info("failed to gpio_to_irq() %d\n", irq);
		return -1;
	}

	return my_generic_init();
}

static void __exit my_exit(void)
{
	my_generic_exit();
	gpio_free(switch_gpio);
	pr_info("Final statistics:   catchup = %d\n", atomic_read(&catchup));
}

module_init(my_init);
module_exit(my_exit);

MODULE_AUTHOR("Jerry Cooperstein");
MODULE_AUTHOR("Chunghan Yi");
MODULE_DESCRIPTION("LDD:2.0 s_20/lab3_one_tasklet_improved_gpio.c");
MODULE_LICENSE("GPL v2");
