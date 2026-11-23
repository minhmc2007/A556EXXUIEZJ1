/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _SEC_MM_H
#define _SEC_MM_H

#include <linux/mm.h>

#define GB_TO_PAGES(x) ((x) << (30 - PAGE_SHIFT))
#define MB_TO_PAGES(x) ((x) << (20 - PAGE_SHIFT))
#define K(x) ((x) << (PAGE_SHIFT-10))

extern struct ratelimit_state dump_tasks_rs;

void mm_debug_dump_tasks(void);

void init_lowfile_detect(void);
void exit_lowfile_detect(void);

void init_panic_handler(void);
void exit_panic_handler(void);

void init_sec_mm_debug(void);
void exit_sec_mm_debug(void);

void init_sec_mm_tune(void);
void exit_sec_mm_tune(void);

void init_sec_mm_rss_stat(void);
void exit_sec_mm_rss_stat(void);

#endif /* _SEC_MM_H */
