// SPDX-License-Identifier: GPL-2.0
/*
 * sec_mm_rss_stat
 */

#include <linux/hashtable.h>
#include <linux/slab.h>
#include <linux/sec_mm.h>
#include <linux/sec_debug.h>
#include <trace/events/kmem.h>
#include <trace/hooks/mm.h>

#define NR_SEC_MM_HASH_BITS 10
#define NR_SEC_MM_HASH_INDICES (1 << NR_SEC_MM_HASH_BITS)

static DEFINE_HASHTABLE(sec_mm_table, NR_SEC_MM_HASH_BITS);
static rwlock_t sec_mm_lock[NR_SEC_MM_HASH_INDICES];

struct sec_mm_entry {
	struct hlist_node node;
	struct mm_struct *mm;
	u64 max_rss_file;
};

static bool is_sec_mm_rss_stat_enabled(void)
{
	return secdbg_mode_enter_upload();
}

static struct sec_mm_entry *mm_search(struct mm_struct *mm, u32 idx)
{
	struct sec_mm_entry *entry;

	hlist_for_each_entry(entry, &sec_mm_table[idx], node) {
		if (entry->mm == mm)
			return entry;
	}
	return NULL;
}

static struct sec_mm_entry *mm_add(struct mm_struct *mm, u32 idx)
{
	struct sec_mm_entry *new_entry, *entry;

	new_entry = kmalloc(sizeof(struct sec_mm_entry),
			in_atomic() ? GFP_ATOMIC : GFP_KERNEL);
	if (!new_entry)
		return NULL;

	write_lock(&sec_mm_lock[idx]);
	entry = mm_search(mm, idx);
	if (entry) {
		kfree(new_entry);
	} else {
		entry = new_entry;
		entry->mm = mm;
		entry->max_rss_file = get_mm_counter(mm, MM_FILEPAGES);
		hlist_add_head(&entry->node, &sec_mm_table[idx]);
	}
	write_unlock(&sec_mm_lock[idx]);

	return entry;
}

static struct sec_mm_entry *mm_search_add(struct mm_struct *mm)
{
	struct sec_mm_entry *entry;
	u32 idx = hash_ptr(mm, 10);

	read_lock(&sec_mm_lock[idx]);
	entry = mm_search(mm, idx);
	read_unlock(&sec_mm_lock[idx]);

	return entry ? entry : mm_add(mm, idx);
}

static void sec_mm_add_entry(void *data, struct mm_struct *mm)
{
	mm_add(mm, hash_ptr(mm, 10));
}

static void sec_mm_del_entry(void *data, struct mm_struct *mm)
{
	struct sec_mm_entry *entry;
	u32 idx;

	idx = hash_ptr(mm, 10);
	write_lock(&sec_mm_lock[idx]);
	entry = mm_search(mm, idx);
	if (entry) {
		hlist_del(&entry->node);
		kfree(entry);
	}
	write_unlock(&sec_mm_lock[idx]);
}

static void sec_mm_show_entry(void *data, struct seq_file *m, struct mm_struct *mm)
{
	struct sec_mm_entry *entry;

	entry = mm_search_add(mm);
	seq_printf(m, "MaxRssFile:\t%llu kB\n", K(entry ?
		entry->max_rss_file : get_mm_counter(mm, MM_FILEPAGES)));
}

static void sec_mm_update_entry(void *data, struct mm_struct *mm, int member, long count)
{
	struct sec_mm_entry *entry;

	if (member != MM_FILEPAGES)
		return;
	entry = mm_search_add(mm);
	if (entry)
		entry->max_rss_file = max_t(u64, entry->max_rss_file,
				get_mm_counter(mm, MM_FILEPAGES));
}

void init_sec_mm_rss_stat(void)
{
	int idx;

	if (!is_sec_mm_rss_stat_enabled())
		return;

	for (idx = 0; idx < NR_SEC_MM_HASH_INDICES; idx++)
		rwlock_init(&sec_mm_lock[idx]);

	register_trace_android_vh_mm_init(sec_mm_add_entry, NULL);
	register_trace_android_vh_mm_free(sec_mm_del_entry, NULL);
	register_trace_android_vh_task_mem(sec_mm_show_entry, NULL);
	register_trace_rss_stat(sec_mm_update_entry, NULL);
}

void exit_sec_mm_rss_stat(void)
{
	struct sec_mm_entry *entry;
	struct hlist_node *tmp;
	int idx;

	if (!is_sec_mm_rss_stat_enabled())
		return;

	unregister_trace_android_vh_mm_init(sec_mm_add_entry, NULL);
	unregister_trace_android_vh_mm_free(sec_mm_del_entry, NULL);
	unregister_trace_android_vh_task_mem(sec_mm_show_entry, NULL);
	unregister_trace_rss_stat(sec_mm_update_entry, NULL);

	for (idx = 0; idx < NR_SEC_MM_HASH_INDICES; idx++) {
		write_lock(&sec_mm_lock[idx]);
		hlist_for_each_entry_safe(entry, tmp,
				&sec_mm_table[idx], node) {
			hlist_del(&entry->node);
			kfree(entry);
		}
		write_unlock(&sec_mm_lock[idx]);
	}
}
