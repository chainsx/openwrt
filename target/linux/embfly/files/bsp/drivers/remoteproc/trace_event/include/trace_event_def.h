/* this file is generate by ./mk_event_def.sh, should not be modify directly */
#ifndef _TRACE_EVENT_DEF_H_
#define _TRACE_EVENT_DEF_H_

typedef enum {
	EV_SYS,
	EV_RT_SYS,
	EV_TICK,
	EV_IRQ,
	EV_MEM,
	EV_SCHE,
	EV_RT_TMR,
	EV_MUTEX,
	EV_SPIN,
	EV_SEM,
	EV_RT_NOTIFY,
	EV_QUEUE,
	EV_RT_QUEUE,
	EV_FS,
	EV_FATFS,
	EV_NTFS,
	EV_OPENAMP,
	EV_SUNXIAMP,
	EV_USR0,
	EV_RPD,
	EV_NUM_SUBSYS
} event_subsys;

#define EV_SYS_STRING                  "sys"
#define EV_RT_SYS_STRING               "rt_sys"
#define EV_TICK_STRING                 "tick"
#define EV_IRQ_STRING                  "irq"
#define EV_MEM_STRING                  "mem"
#define EV_SCHE_STRING                 "sche"
#define EV_RT_TMR_STRING               "rt-timer"
#define EV_MUTEX_STRING                "mutex"
#define EV_SPIN_STRING                 "spin"
#define EV_SEM_STRING                  "sem"
#define EV_RT_NOTIFY_STRING            "notify"
#define EV_QUEUE_STRING                "queue"
#define EV_RT_QUEUE_STRING             "rt_queue"
#define EV_FS_STRING                   "fs"
#define EV_FATFS_STRING                "fatfs"
#define EV_NTFS_STRING                 "ntfs"
#define EV_OPENAMP_STRING              "amp"
#define EV_SUNXIAMP_STRING             "amp"
#define EV_USR0_STRING                 "usr0"
#define EV_RPD_STRING                  "rpd"

#endif
