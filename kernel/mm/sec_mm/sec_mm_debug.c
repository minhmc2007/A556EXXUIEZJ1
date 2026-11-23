// SPDX-License-Identifier: GPL-2.0
/*
 * sec_mm/
 *
 * Copyright (C) 2024 Samsung Electronics
 *
 */

#include <linux/mm.h>
#include <linux/ratelimit.h>
#include <linux/vmalloc.h>
#include <linux/sec_mm.h>
#include <trace/hooks/mm.h>

DEFINE_RATELIMIT_STATE(dump_tasks_rs, 5 * HZ, 1);

static void sec_mm_show_mem(void *data, unsigned int filter, nodemask_t *nodes)
{
	pr_info("%s: %lu kB\n", "VmallocUsed", K(vmalloc_nr_pages()));

	if (in_interrupt())
		return;

	if (__ratelimit(&dump_tasks_rs))
		mm_debug_dump_tasks();
}

void init_sec_mm_debug(void)
{
	register_trace_prio_android_vh_show_mem(sec_mm_show_mem, NULL, 0);
}

void exit_sec_mm_debug(void)
{
	unregister_trace_android_vh_show_mem(sec_mm_show_mem, NULL);
}
