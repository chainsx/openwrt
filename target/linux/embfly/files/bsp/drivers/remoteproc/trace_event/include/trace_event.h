/* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 *the the People's Republic of China and other countries.
 * All Allwinner Technology Co.,Ltd. trademarks are used with permission.
 *
 * DISCLAIMER
 * THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
 * IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
 * IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
 * ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
 * ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
 * COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
 * YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
 * PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
 * THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
 * OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _AW_COMPONENT_TRACE_EVENT_H_
#define _AW_COMPONENT_TRACE_EVENT_H_

#include "trace_event_parser_cfg.h"

#ifndef TRACE_EVENT_PARSER_ON_NATIVE_PLATFORM
#ifndef CONFIG_COMPONENTS_EVENTS
#define CONFIG_COMPONENTS_EVENTS
#endif
#endif

#ifdef CONFIG_COMPONENTS_EVENTS

#ifdef TRACE_EVENT_PARSER_ON_LINUX_KERNEL
#include <linux/types.h>
#include <linux/limits.h>
#define INT32_MIN S32_MIN
#define UINT32_MAX U32_MAX
#else
#include <stdint.h>
#endif

#define BUF_SPACE_NOT_ENOUGH INT32_MIN
#define NEW_TRACE_EVENT_NOT_EXIST UINT32_MAX

typedef int (*printf_func_t)(const char *fmt, ...);
typedef printf_func_t event_printf_t;

typedef int (*user_print_func)(void *priv, const char *fmt, ...);

typedef struct user_print_desc {
	uint8_t *print_buf;
	uint32_t print_buf_len;
	uint32_t print_buf_write_index;

	printf_func_t printf;
} user_print_desc_t;

typedef struct reader_info {
	int is_read_all;
	int is_read_to_buf;
	int is_panic_when_read;

	uint32_t read_data_len; /* the actual event data len after read operation complete */

	const struct aw_trace_event *trace_event_obj;

	/* printf member in single_event_pr_desc is generally NULL */
	user_print_desc_t single_event_pr_desc;

	user_print_func user_pr_func;
	void *user_pr_priv_data;

	/*
	 If the user want only read new data on next read operation,
	 this three member is used to record the info of last read opeation.
	 So the reader_info provided on next read opertion must be the previous reader_info.
	*/
	uint32_t read_pos;
	uint32_t last_event_pos;
	uint64_t last_event_timestamp;
} reader_info_t;

typedef struct writer_info {
	uint32_t write_pos;
	uint32_t overlay_cnt;
} writer_info_t;

typedef struct aw_trace_event_addr_info_32 {
	uint32_t writer_info_addr;
	uint32_t events_buf_addr;
	uint32_t subsys_class_addr;
	uint32_t pid_map_arr_addr;
} aw_trace_event_addr_info_32_t;

typedef struct aw_trace_event_addr_info_64 {
	uint64_t writer_info_addr;
	uint64_t events_buf_addr;
	uint64_t subsys_class_addr;
	uint64_t pid_map_arr_addr;
} aw_trace_event_addr_info_64_t;

typedef struct aw_trace_event_cfg {
	int is_64bit_pointer;
	uint32_t events_buf_len;
	uint32_t max_task_name_cnt;
	uint32_t pid_map_type_size;
	uint32_t os_event_type_size;
} aw_trace_event_cfg_t;

typedef struct aw_trace_event {
	aw_trace_event_cfg_t cfg;

	union {
		aw_trace_event_addr_info_32_t addr32;
		aw_trace_event_addr_info_64_t addr64;
	};

#ifdef TRACE_EVENT_PARSER_ON_NATIVE_PLATFORM
	writer_info_t *writer_info;
	uint32_t *events_buf;
#endif
} aw_trace_event_t;

struct aw_trace_event_parser;
typedef void *(*parser_addr_map_t)(const struct aw_trace_event_parser *parser, uint64_t da, size_t len);
typedef struct aw_trace_event_parser {
	parser_addr_map_t addr_map_func;
	void *priv;

#ifdef TRACE_EVENT_PARSER_ON_LINUX_KERNEL
	/*
	FIXME: The reader_info object and some buf doesn't depend on parser actually.
	We place them on here only for convenience. because currently only one reader on Linux kernel.
	*/
	reader_info_t reader_info;

	uint8_t kernel_buf[1024];
	uint8_t event_data_buf[512];
#endif
} aw_trace_event_parser_t;


#define ARG_N_(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
				_11, _12, _13, _14, _15, _16, _17, _18, _19, _20, \
				_21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
				_31, _32, _33, _34, _35, _36, _37, _38, _39, _40, \
				_41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
				_51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
				_61, _62, _63, _64, N, ...) N
#define _COUNT_ARG_HELPER(...)		ARG_N_(__VA_ARGS__)
#define _COUNT_ARG(...)		_COUNT_ARG_HELPER("ignore", ##__VA_ARGS__, \
								64, 63, 62, 61, \
								60, 59, 58, 57, 56, 55, 54, 53, 52, 51, \
								50, 49, 48, 47, 46, 45, 44, 43, 42, 41, \
								40, 39, 38, 37, 36, 35, 34, 33, 32, 31, \
								30, 29, 28, 27, 26, 25, 24, 23, 22, 21, \
								20, 19, 18, 17, 16, 15, 14, 13, 12, 11, \
								10,  9,  8,  7,  6,  5,  4,  3,  2,  1, 0)

#define FUNC_CAT2_1(a, b)			a##b
#define FUNC_CAT2(a, b)				FUNC_CAT2_1(a, b)
#define FUNC_VA_NAME(prefix, ...)	FUNC_CAT2(prefix, _COUNT_ARG(__VA_ARGS__))

#define EV_MACRO_IS_ENABLED3(ignore_this, val, ...)		val
#define EV_MACRO_IS_ENABLED2(one_or_two_args)			EV_MACRO_IS_ENABLED3(one_or_two_args 1, 0)
#define _XXXX1											_YYYY,
#define EV_MACRO_IS_ENABLED1(macro)						EV_MACRO_IS_ENABLED2(_XXXX##macro)
#define EV_MACRO_IS_ENABLED(macro)						EV_MACRO_IS_ENABLED1(macro)

/*
 * NOTE: the flow function can't insert trace_event stub.
 * use extern declare to avoid include head file.
 */
extern uint32_t hal_interrupt_get_nest(void);
extern unsigned long hal_interrupt_is_disable(void);
extern void *hal_thread_self(void);
extern int64_t ktime_get(void);
extern void xport_interrupt_enable(unsigned long flags);
extern unsigned long xport_interrupt_disable(void);
extern void *memcpy(void *dest, const void *src, size_t n);
extern size_t strlen(const char *s);
#define ev_get_irq_nest()		hal_interrupt_get_nest()
#define ev_get_irqoff()			hal_interrupt_is_disable()
#define ev_get_tcb()			hal_thread_self()
#define ev_get_time()			ktime_get()
#define events_buffer_lock()			xport_interrupt_disable()
#define events_buffer_unlock(flags)		xport_interrupt_enable(flags)
#define events_memcpy(dst, src, len)	memcpy(dst, src, len)

#define TRACE_EVENT_MAX_ARG				(8)
#define TRACE_EVENT_ARG_UNIT			uint32_t
#define TRACE_EVENT_ARG_SZ(sz)			((sz) & 0x3 ? ((sz) & ~0x3) + 4 : (sz)) /* align to 4 byte */
#define TRACE_EVENT_ARG_OFS()			(arg_ofs)
#define TRACE_EVENT_NEXT_ARG(ofs, sz)	(ofs += (TRACE_EVENT_ARG_SZ(sz) / 4))

#ifdef TRACE_EVENT_PARSER_ON_NATIVE_PLATFORM

typedef struct os_event {
	uint32_t magic;
	uint32_t stat;
	int pid;
	const char *name;
	uint64_t time;
	TRACE_EVENT_ARG_UNIT args[];
} __attribute__((packed)) os_event_t;

#else

typedef struct os_event_32 {
	uint32_t magic;
	uint32_t stat;
	int pid;
	//const char *name;
	uint32_t name_addr;
	uint64_t time;
	TRACE_EVENT_ARG_UNIT args[];
} __attribute__((packed)) os_event_32_t;

typedef struct os_event_64 {
	uint32_t magic;
	uint32_t stat;
	int pid;
	//const char *name;
	uint64_t name_addr;
	uint64_t time;
	TRACE_EVENT_ARG_UNIT args[];
} __attribute__((packed)) os_event_64_t;

/* FIXME: currently not support to parse the event which is generated on 64bit platform */
typedef os_event_32_t os_event_t;

#endif

#define EVENT_MAGIC			(0xf3f45e6b)

#define ARGCNT_SHIFT		(0)
#define ARGCNT_MASK			(0xF << ARGCNT_SHIFT)

#define NEST_SHIFT			(4)
#define NEST_MASK			(0x7 << NEST_SHIFT)

#define IRQOFF_SHIFT		(7)
#define IRQOFF_MASK			(1 << IRQOFF_SHIFT)

#define EV_TYPE_SHIFT		(8)
#define EV_TYPE_MASK		(0xF << EV_TYPE_SHIFT)
#define EV_TYPE_BEGIN		(0)
#define EV_TYPE_END			(1)
#define EV_TYPE_MARK		(2)
#define EV_TYPE_CNT			(3)

#define SUBSYS_SHIFT		(12)
#define SUBSYS_MASK			(0x3FF << SUBSYS_SHIFT)

#define ARGSZ_SHIFT			(22)
#define ARGSZ_MASK			(0x3FF << ARGSZ_SHIFT)

#define _init_arg_type_int(ev, n, v, t)		\
	{ (ev)->args[TRACE_EVENT_ARG_OFS()] = (int)(v); TRACE_EVENT_NEXT_ARG(arg_ofs, sizeof(int)); }
#define _arg_type_int_sz(v)				(sizeof(int))

#define _init_arg_type_long(ev, n, v, t)	\
	{ (ev)->args[TRACE_EVENT_ARG_OFS()] = (long)(v); TRACE_EVENT_NEXT_ARG(arg_ofs, sizeof(long)); }
#define _arg_type_long_sz(v)				(sizeof(long))

#define _init_arg_type_char(ev, n, v, t)	\
	{ (ev)->args[TRACE_EVENT_ARG_OFS()] = (char)(v); TRACE_EVENT_NEXT_ARG(arg_ofs, sizeof(char)); }
#define _arg_type_char_sz(v)				(TRACE_EVENT_ARG_SZ(sizeof(char)))

#define _init_arg_type_str(ev, n, v, t)		\
	{ \
		unsigned int len = strlen((const char *)(v)) + 1; \
		events_memcpy(&((ev)->args[TRACE_EVENT_ARG_OFS()]), (void *)(v), len); \
		TRACE_EVENT_NEXT_ARG(arg_ofs, len); \
	}
#define _arg_type_str_sz(v)				(TRACE_EVENT_ARG_SZ(strlen((const char *)(v)) + 1))

#define ARG_PRINT_U			0	/* %u */
#define _init_arg_type_0(ev, n, v, t)		_init_arg_type_int(ev, n, v, t)
#define _arg_sz_type_0(v)					_arg_type_int_sz(v)

#define ARG_PRINT_LU		1	/* %lu */
#define _init_arg_type_1(ev, n, v, t)		_init_arg_type_long(ev, n, v, t)
#define _arg_sz_type_1(v)					_arg_type_long_sz(v)

#define ARG_PRINT_X			2	/* %x */
#define _init_arg_type_2(ev, n, v, t)		_init_arg_type_int(ev, n, v, t)
#define _arg_sz_type_2(v)					_arg_type_int_sz(v)

#define ARG_PRINT_LX		3	/* %lx */
#define _init_arg_type_3(ev, n, v, t)		_init_arg_type_long(ev, n, v, t)
#define _arg_sz_type_3(v)					_arg_type_long_sz(v)

#define ARG_PRINT_D			4	/* %d */
#define _init_arg_type_4(ev, n, v, t)		_init_arg_type_int(ev, n, v, t)
#define _arg_sz_type_4(v)					_arg_type_int_sz(v)

#define ARG_PRINT_LD		5	/* %ld */
#define _init_arg_type_5(ev, n, v, t)		_init_arg_type_long(ev, n, v, t)
#define _arg_sz_type_5(v)					_arg_type_long_sz(v)

#define ARG_PRINT_C			6	/* %c */
#define _init_arg_type_6(ev, n, v, t)		_init_arg_type_char(ev, n, v, t)
#define _arg_sz_type_6(v)					_arg_type_char_sz(v)

#define ARG_PRINT_P			7	/* %p */
#define _init_arg_type_7(ev, n, v, t)		_init_arg_type_long(ev, n, v, t)
#define _arg_sz_type_7(v)					_arg_type_long_sz(v)

#define ARG_PRINT_STR		8	/* %s */
#define _init_arg_type_8(ev, n, v, t)		_init_arg_type_str(ev, n, v, t)
#define _arg_sz_type_8(v)					_arg_type_str_sz(v)

#define ARG_TYPE_LONG		ARG_PRINT_LD
#define ARG_TYPE_ULONG		ARG_PRINT_LX
#define ARG_TYPE_PTR		ARG_PRINT_P
#define ARG_TYPE_STR		ARG_PRINT_STR
#define ARG_TYPE_INT		ARG_PRINT_D
#define ARG_TYPE_UINT		ARG_PRINT_U
#define ARG_TYPE_CHAR		ARG_PRINT_C
#define ARG_TYPE_BYTE		ARG_PRINT_X

/****************** private: arg macro **********************/
#define ARG_NAME(arg)		#arg,
#define COUNT_ARG(...)		(_COUNT_ARG(__VA_ARGS__) / 3) /* name, val, type */

#define _one_arg_(n)		n#n, v#n, t#n
#define __ev_args1__	_one_arg_(1)
#define __ev_args2__	_one_arg_(1),_one_arg_(2)
#define __ev_args3__	_one_arg_(1),_one_arg_(2),_one_arg_(3)
#define __ev_args4__	_one_arg_(1),_one_arg_(2),_one_arg_(3),_one_arg_(4)
#define __ev_args5__	_one_arg_(1),_one_arg_(2),_one_arg_(3),_one_arg_(4),_one_arg_(5)
#define __ev_args6__	_one_arg_(1),_one_arg_(2),_one_arg_(3),_one_arg_(4),_one_arg_(5),_one_arg_(6)
#define __ev_args7__	_one_arg_(1),_one_arg_(2),_one_arg_(3),_one_arg_(4),_one_arg_(5),_one_arg_(6),_one_arg_(7)
#define __ev_args8__	_one_arg_(1),_one_arg_(2),_one_arg_(3),_one_arg_(4),_one_arg_(5),_one_arg_(6),_one_arg_(7),_one_arg_(8)

/* arg: n = name, v = val, t = type */
#define _init_one_ev_arg(ev, n, v, t)			FUNC_CAT2(_init_arg_type_, t)(ev, n, v, t)

#define _init_ev_arg_0(...)
#define _init_ev_arg_3(ev, n1, v1, t1)        _init_one_ev_arg(ev, n1, v1, t1)

#define _init_ev_arg_6(ev, n1, v1, t1, n2, v2, t2) \
			{ \
				_init_one_ev_arg(ev, n1, v1, t1); \
				_init_one_ev_arg(ev, n2, v2, t2); \
			}

#define _init_ev_arg_9(ev, n1, v1, t1, n2, v2, t2, n3, v3, t3) \
			{ \
				_init_one_ev_arg(ev, n1, v1, t1); \
				_init_one_ev_arg(ev, n2, v2, t2); \
				_init_one_ev_arg(ev, n3, v3, t3); \
			}

#define _init_ev_arg_12(ev, n1, v1, t1, n2, v2, t2, n3, v3, t3, n4, v4, t4) \
			{ \
				_init_one_ev_arg(ev, n1, v1, t1); \
				_init_one_ev_arg(ev, n2, v2, t2); \
				_init_one_ev_arg(ev, n3, v3, t3); \
				_init_one_ev_arg(ev, n4, v4, t4); \
			}

#define _init_ev_arg_15(ev, n1, v1, t1, n2, v2, t2, n3, v3, t3, n4, v4, t4, n5, v5, t5) \
			{ \
				_init_one_ev_arg(ev, n1, v1, t1); \
				_init_one_ev_arg(ev, n2, v2, t2); \
				_init_one_ev_arg(ev, n3, v3, t3); \
				_init_one_ev_arg(ev, n4, v4, t4); \
				_init_one_ev_arg(ev, n5, v5, t5); \
			}

#define _init_ev_arg_18(ev, n1, v1, t1, n2, v2, t2, n3, v3, t3, n4, v4, t4, n5, v5, t5, n6, v6, t6) \
			{ \
				_init_one_ev_arg(ev, n1, v1, t1); \
				_init_one_ev_arg(ev, n2, v2, t2); \
				_init_one_ev_arg(ev, n3, v3, t3); \
				_init_one_ev_arg(ev, n4, v4, t4); \
				_init_one_ev_arg(ev, n5, v5, t5); \
				_init_one_ev_arg(ev, n6, v6, t6); \
			}

#define _init_ev_arg_21(ev, n1, v1, t1, n2, v2, t2, n3, v3, t3, n4, v4, t4, n5, v5, t5, n6, v6, t6, n7, v7, t7) \
			{ \
				_init_one_ev_arg(ev, n1, v1, t1); \
				_init_one_ev_arg(ev, n2, v2, t2); \
				_init_one_ev_arg(ev, n3, v3, t3); \
				_init_one_ev_arg(ev, n4, v4, t4); \
				_init_one_ev_arg(ev, n5, v5, t5); \
				_init_one_ev_arg(ev, n6, v6, t6); \
				_init_one_ev_arg(ev, n7, v7, t7); \
			}

#define _init_ev_arg_24(ev, n1, v1, t1, n2, v2, t2, n3, v3, t3, n4, v4, t4, n5, v5, t5, n6, v6, t6, n7, v7, t7, n8, v8, t8) \
			{ \
				_init_one_ev_arg(ev, n1, v1, t1); \
				_init_one_ev_arg(ev, n2, v2, t2); \
				_init_one_ev_arg(ev, n3, v3, t3); \
				_init_one_ev_arg(ev, n4, v4, t4); \
				_init_one_ev_arg(ev, n5, v5, t5); \
				_init_one_ev_arg(ev, n6, v6, t6); \
				_init_one_ev_arg(ev, n7, v7, t7); \
				_init_one_ev_arg(ev, n8, v8, t8); \
			}

#define _mk_ev_arg_n(ev, ...) FUNC_VA_NAME(_init_ev_arg_, ##__VA_ARGS__)(ev, ##__VA_ARGS__)

/******************************** count arg size *********************************/
#define _count_arg_type_sz(t, v) FUNC_CAT2(_arg_sz_type_, t)(v)

#define _count_ev_arg_0(...)                   (0)
#define _count_ev_arg_3(ev, n1, v1, t1)        _count_arg_type_sz(t1, v1)

#define _count_ev_arg_6(ev, n1, v1, t1, n2, v2, t2) \
				(_count_arg_type_sz(t1, v1) + _count_arg_type_sz(t2, v2))

#define _count_ev_arg_9(ev, n1, v1, t1, n2, v2, t2, n3, v3, t3) \
				(_count_arg_type_sz(t1, v1) + _count_arg_type_sz(t2, v2) + _count_arg_type_sz(t3, v3))

#define _count_ev_arg_12(ev, n1, v1, t1, n2, v2, t2, n3, v3, t3, n4, v4, t4) \
				(_count_arg_type_sz(t1, v1) + _count_arg_type_sz(t2, v2) + _count_arg_type_sz(t3, v3) + \
				_count_arg_type_sz(t4, v4))

#define _count_ev_arg_15(ev, n1, v1, t1, n2, v2, t2, n3, v3, t3, n4, v4, t4, n5, v5, t5) \
				(_count_arg_type_sz(t1, v1) + _count_arg_type_sz(t2, v2) + _count_arg_type_sz(t3, v3) + \
				_count_arg_type_sz(t4, v4) + _count_arg_type_sz(t5, v5))

#define _count_ev_arg_18(ev, n1, v1, t1, n2, v2, t2, n3, v3, t3, n4, v4, t4, n5, v5, t5, n6, v6, t6) \
				(_count_arg_type_sz(t1, v1) + _count_arg_type_sz(t2, v2) + _count_arg_type_sz(t3, v3) + \
				_count_arg_type_sz(t4, v4) + _count_arg_type_sz(t5, v5) + _count_arg_type_sz(t6, v6))

#define _count_ev_arg_21(ev, n1, v1, t1, n2, v2, t2, n3, v3, t3, n4, v4, t4, n5, v5, t5, n6, v6, t6, n7, v7, t7) \
				(_count_arg_type_sz(t1, v1) + _count_arg_type_sz(t2, v2) + _count_arg_type_sz(t3, v3) + \
				_count_arg_type_sz(t4, v4) + _count_arg_type_sz(t5, v5) + _count_arg_type_sz(t6, v6) + \
				_count_arg_type_sz(t7, v7))

#define _count_ev_arg_24(ev, n1, v1, t1, n2, v2, t2, n3, v3, t3, n4, v4, t4, n5, v5, t5, n6, v6, t6, n7, v7, t7, n8, v8, t8) \
				(_count_arg_type_sz(t1, v1) + _count_arg_type_sz(t2, v2) + _count_arg_type_sz(t3, v3) + \
				_count_arg_type_sz(t4, v4) + _count_arg_type_sz(t5, v5) + _count_arg_type_sz(t6, v6) + \
				_count_arg_type_sz(t7, v7) + _count_arg_type_sz(t8, v8))

#define _count_ev_arg_sz(ev, ...) FUNC_VA_NAME(_count_ev_arg_, ##__VA_ARGS__)(ev, ##__VA_ARGS__)

/****************** private: name macro **********************/
/*
 * name formats: event_name:argN_type:argN_name:....:
 *
 * such as: trace_event(EV_FS, vfs_open, ARG_STR(path), ARG_INT(flags));
 * will generate name: vfs_open:2:path:5:flags:
 */
/* n = name, v = val, t = type */
#define _init_ev_arg_name_0()
#define _init_ev_arg_name_3(n1, v1, t1) \
				#t1":"n1":"
#define _init_ev_arg_name_6(n1, v1, t1, n2, v2, t2) \
				#t1":"n1":"#t2":"n2":"
#define _init_ev_arg_name_9(n1, v1, t1, n2, v2, t2, n3, v3, t3) \
				#t1":"n1":"#t2":"n2":"#t3":"n3":"
#define _init_ev_arg_name_12(n1, v1, t1, n2, v2, t2, n3, v3, t3, n4, v4, t4) \
				#t1":"n1":"#t2":"n2":"#t3":"n3":"#t4":"n4":"
#define _init_ev_arg_name_15(n1, v1, t1, n2, v2, t2, n3, v3, t3, n4, v4, t4, n5, v5, t5) \
				#t1":"n1":"#t2":"n2":"#t3":"n3":"#t4":"n4":"#t5":"n5":"
#define _init_ev_arg_name_18(n1, v1, t1, n2, v2, t2, n3, v3, t3, n4, v4, t4, n5, v5, t5, n6, v6, t6) \
				#t1":"n1":"#t2":"n2":"#t3":"n3":"#t4":"n4":"#t5":"n5":"#t6":"n6":"
#define _init_ev_arg_name_21(n1, v1, t1, n2, v2, t2, n3, v3, t3, n4, v4, t4, n5, v5, t5, n6, v6, t6, n7, v7, t7) \
				#t1":"n1":"#t2":"n2":"#t3":"n3":"#t4":"n4":"#t5":"n5":"#t6":"n6":"#t7":"n7":"
#define _init_ev_arg_name_24(n1, v1, t1, n2, v2, t2, n3, v3, t3, n4, v4, t4, n5, v5, t5, n6, v6, t6, n7, v7, t7, n8, v8, t8) \
				#t1":"n1":"#t2":"n2":"#t3":"n3":"#t4":"n4":"#t5":"n5":"#t6":"n6":"#t7":"n7":"#t8":"n8":"

#define __mk_ev_name(...)						FUNC_VA_NAME(_init_ev_arg_name_, ##__VA_ARGS__)(__VA_ARGS__)
#define __mk_ev_name2(sys, name)				name ":"
#define __mk_ev_name1(sys, name, ...)			__mk_ev_name2(sys, name) __mk_ev_name(__VA_ARGS__)
#define trace_mk_ev_name(sys, name, ...)		__mk_ev_name1(sys##_STRING, name, ##__VA_ARGS__)

/****************** private: stat macro **********************/
#define _mk_ev_stat(arg_cnt, subsys, irqoff, type, argsz) \
		({ \
			uint32_t stat = 0; \
			stat |= (((arg_cnt) << ARGCNT_SHIFT) & ARGCNT_MASK); \
			stat |= (((subsys) << SUBSYS_SHIFT) & SUBSYS_MASK); \
			stat |= (((ev_get_irq_nest()) << NEST_SHIFT) & NEST_MASK); \
			stat |= (((irqoff) << IRQOFF_SHIFT) & IRQOFF_MASK); \
			stat |= (((type) << EV_TYPE_SHIFT) & EV_TYPE_MASK); \
			stat |= ((((argsz) / sizeof(TRACE_EVENT_ARG_UNIT)) << ARGSZ_SHIFT) & ARGSZ_MASK); \
			stat; \
		 })

/********************************* publish *****************************************/

#include "trace_event_def.h"

/* SUBSYS */
/*
 * how to add new subsys?
 *
 * e.g. add new subsys XXX
 *   method 1:
 *     edit envent_list.txt, add new line EV_XXX     "xxx"       "Enable xxx Event"
 *     run script: ./mk_kconfig.sh
 *   method 2:
 *     step1: add nuw subsys EV_XXX at enum event_subsys witch CONFIG_EVENTS_TRACE_EV_XXX condition
 *     step2: add define #define EV_SYS_XXX  "xxx"
 *     step3: add Kocnfig at Kconfig file, format: config EVENTS_TRACE_EV_XXX
 */

#ifdef TRACE_EVENT_PARSER_ON_NATIVE_PLATFORM
struct subsys_entry {
	const char *name;
	unsigned char enable;
	unsigned char buildin;
};
extern struct subsys_entry g_subsys_class[EV_NUM_SUBSYS];
#else
typedef struct subsys_entry_32 {
	//const char *name;
	uint32_t name_addr;
	unsigned char enable;
	unsigned char buildin;
} subsys_entry_32_t;

typedef struct subsys_entry_64 {
	//const char *name;
	uint64_t name_addr;
	unsigned char enable;
	unsigned char buildin;
} subsys_entry_64_t;

typedef union subsys_entry {
	subsys_entry_32_t p32;
	subsys_entry_64_t p64;
} subsys_entry_t;

#endif

extern int event_tracing;

/* ARG MICRO */
#define _ARG_TYPE(t)			t
#define ARG_PTR(p)				ARG_NAME(p) (unsigned long)(p), _ARG_TYPE(ARG_TYPE_PTR)
#define ARG_STR(p)				ARG_NAME(p) (unsigned long)(p), _ARG_TYPE(ARG_TYPE_STR)
#define ARG_CHAR(p)				ARG_NAME(p) (unsigned long)(p), _ARG_TYPE(ARG_TYPE_CHAR)
#define ARG_BYTE(p)				ARG_NAME(p) (unsigned long)(p), _ARG_TYPE(ARG_TYPE_BYTE)
#define ARG_INT(p)				ARG_NAME(p) (unsigned long)(p), _ARG_TYPE(ARG_TYPE_INT)
#define ARG_UINT(p)				ARG_NAME(p) (unsigned long)(p), _ARG_TYPE(ARG_TYPE_UINT)
#define ARG_LONG(p)				ARG_NAME(p) (unsigned long)(p), _ARG_TYPE(ARG_TYPE_LONG)
#define ARG_ULONG(p)			ARG_NAME(p) (unsigned long)(p), _ARG_TYPE(ARG_TYPE_ULONG)

#define ARG_PTR_RENAME(name, p)			ARG_NAME(name) (unsigned long)(p), _ARG_TYPE(ARG_TYPE_PTR)
#define ARG_STR_RENAME(name, p)			ARG_NAME(name) (unsigned long)(p), _ARG_TYPE(ARG_TYPE_STR)
#define ARG_CHAR_RENAME(name, p)		ARG_NAME(name) (unsigned long)(p), _ARG_TYPE(ARG_TYPE_CHAR)
#define ARG_BYTE_RENAME(name, p)		ARG_NAME(name) (unsigned long)(p), _ARG_TYPE(ARG_TYPE_BYTE)
#define ARG_INT_RENAME(name, p)			ARG_NAME(name) (unsigned long)(p), _ARG_TYPE(ARG_TYPE_INT)
#define ARG_UINT_RENAME(name, p)		ARG_NAME(name) (unsigned long)(p), _ARG_TYPE(ARG_TYPE_UINT)
#define ARG_LONG_RENAME(name, p)		ARG_NAME(name) (unsigned long)(p), _ARG_TYPE(ARG_TYPE_LONG)
#define ARG_ULONG_RENAME(name, p)		ARG_NAME(name) (unsigned long)(p), _ARG_TYPE(ARG_TYPE_ULONG)

#define RET_ARG(p)			ARG_NAME(ret) (unsigned long)(p), _ARG_TYPE(ARG_TYPE_UINT)

#ifdef TRACE_EVENT_PARSER_ON_NATIVE_PLATFORM
#define event_size(argsz)		((sizeof(os_event_t) + argsz) / sizeof(uint32_t))
#define event_byte_size(argsz)		(sizeof(os_event_t) + argsz)

#ifndef __ALIGN_KERNEL
#define __ALIGN_KERNEL(x, a) __ALIGN_KERNEL_MASK(x, (typeof(x))(a) - 1)
#endif
#ifndef __ALIGN_KERNEL_MASK
#define __ALIGN_KERNEL_MASK(x, mask) (((x) + (mask)) & ~(mask))
#endif

#ifndef ALIGN_UP
#define ALIGN_UP(x, a) __ALIGN_KERNEL((x), (a))
#endif

#ifndef ALIGN_DOWN
#define ALIGN_DOWN(x, a) __ALIGN_KERNEL((x) - ((a) - 1), (a))
#endif

#if defined(CONFIG_ARCH_RISCV_E906) || defined(CONFIG_ARCH_RISCV_E907)
#define L1_CACHE_LINE_SIZE 32
#else
/* FIXME: possibly the cache line size of other cpu is not 64*/
#define L1_CACHE_LINE_SIZE 64
#endif

#define CACHE_LINE_SIZE L1_CACHE_LINE_SIZE

#define CACHE_LINE_ALIGN_ATTR __attribute__((__aligned__(CACHE_LINE_SIZE)))

#define CACHE_LINE_ALIGN_UP(x) ALIGN_UP((x), (CACHE_LINE_SIZE))
#define CACHE_LINE_ALIGN_DOWN(x) ALIGN_DOWN((x), (CACHE_LINE_SIZE))

extern void hal_dcache_clean(unsigned long vaddr_start, unsigned long size);

#define _trace_event(type, subsys, _name, ...) \
		do { \
			if (EV_MACRO_IS_ENABLED(CONFIG_EVENTS_TRACE_##subsys) && \
							g_subsys_class[subsys].enable && !event_tracing) { \
				__attribute__((__unused__))unsigned long arg_ofs = 0; \
				unsigned long irqoff, flags, arg_sz, start_addr, end_addr, clean_size; \
				arg_sz = _count_ev_arg_sz(event, __VA_ARGS__); \
				irqoff = ev_get_irqoff(); \
				flags = events_buffer_lock(); \
				event_tracing++; \
				os_event_t *event = trace_init_event(arg_sz); \
				event->name = trace_mk_ev_name(subsys, _name, ##__VA_ARGS__); \
				event->stat = _mk_ev_stat(COUNT_ARG(__VA_ARGS__), subsys, irqoff, type, arg_sz); \
				_mk_ev_arg_n(event, __VA_ARGS__); \
				event_tracing--; \
				start_addr = (unsigned long)event;\
				end_addr = start_addr + event_byte_size(arg_sz) - 1;\
				clean_size = CACHE_LINE_ALIGN_UP(end_addr)- CACHE_LINE_ALIGN_DOWN(start_addr); \
				hal_dcache_clean(CACHE_LINE_ALIGN_DOWN(start_addr), clean_size); \
				events_buffer_unlock(flags); \
			} \
		} while (0)

#define trace_event(subsys, _name, ...)             _trace_event(EV_TYPE_MARK, subsys, _name, ##__VA_ARGS__)
#define trace_event_begin(subsys, _name, ...)       _trace_event(EV_TYPE_BEGIN, subsys, _name, ##__VA_ARGS__)
#define trace_event_end(subsys, _name, ...)         _trace_event(EV_TYPE_END, subsys, _name, ##__VA_ARGS__)
#define trace_event_count(subsys, _name, p, v)      _trace_event(EV_TYPE_CNT, subsys, "", ARG_ULONG(p), v)

os_event_t *trace_init_event(unsigned long argsz);

int trace_event_dump(event_printf_t pr);
int trace_event_panic_dump(event_printf_t pr);
#endif

int aw_trace_event_parser_init(aw_trace_event_parser_t *parser, parser_addr_map_t map_func, void *priv);

int aw_trace_event_read(const aw_trace_event_parser_t *parser, reader_info_t *reader_info);

int aw_trace_event_read_to_buf(aw_trace_event_parser_t *parser,
							   reader_info_t *reader_info, void *buf, uint32_t buf_len);

int aw_trace_event_read_all_to_buf(aw_trace_event_parser_t *parser,
								   const aw_trace_event_t *trace_event_obj, int is_panic_read, void *buf, uint32_t buf_len);

int aw_trace_event_dump(aw_trace_event_parser_t *parser,
						reader_info_t *reader_info, event_printf_t event_pr_func);

int aw_trace_event_dump_all(aw_trace_event_parser_t *parser,
							const aw_trace_event_t *trace_event_obj, int is_panic_dump, event_printf_t event_pr_func);

int aw_trace_event_info_dump(const aw_trace_event_t *obj);

#else
#define trace_event(subsys, _name, ...)
#define trace_event_begin(subsys, _name, ...)
#define trace_event_end(subsys, _name, ...)
#define trace_event_count(subsys, _name, ...)

#define trace_event_dump(...)
#define trace_event_panic_dump(...)

#endif /* CONFIG_COMPONENTS_EVENTS */

#endif /* _AW_COMPONENT_TRACE_EVENT_H_ */

