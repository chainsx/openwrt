#ifndef _AW_FREERTOS_TRACECONFIG_H
#define _AW_FREERTOS_TRACECONFIG_H

#include <trace_event.h>

#undef traceTASK_CREATE
#define traceTASK_CREATE(pxNewTCB)					trace_event(EV_RT_SYS, "cteate", ARG_PTR_RENAME(tcb, pxNewTCB))
#undef traceTASK_CREATE_FAILED
#define traceTASK_CREATE_FAILED(pxNewTCB)			trace_event(EV_RT_SYS, "cteate_fail", ARG_PTR_RENAME(tcb, xTask))
#undef traceTASK_DELAY
#define traceTASK_DELAY()							trace_event(EV_RT_SYS, "delay", \
														ARG_ULONG_RENAME(now, xTickCount), ARG_ULONG_RENAME(delay, xTicksToDelay))
#undef traceTASK_DELAY_UNTIL
#define traceTASK_DELAY_UNTIL(xTimeToWake)			trace_event(EV_RT_SYS, "until", \
														ARG_ULONG_RENAME(now, xTickCount), ARG_ULONG_RENAME(until, xTimeToWake))
#undef traceTASK_DELETE
#define traceTASK_DELETE(pxTCB)						trace_event(EV_RT_SYS, "delete", ARG_PTR_RENAME(tcb, pxTCB))
#undef traceTASK_RESUME
#define traceTASK_RESUME(pxTCB)						trace_event(EV_RT_SYS, "resume", ARG_PTR_RENAME(tcb, pxTCB))
#undef traceTASK_RESUME_FROM_ISR
#define traceTASK_RESUME_FROM_ISR(pxTCB)			trace_event(EV_RT_SYS, "resume", ARG_PTR_RENAME(tcb, pxTCB))
#undef traceTASK_SUSPEND
#define traceTASK_SUSPEND(pxTCB)					trace_event(EV_RT_SYS, "suspend", ARG_PTR_RENAME(tcb, pxTCB))

#undef traceTASK_INCREMENT_TICK
#define traceTASK_INCREMENT_TICK(xTickCount)		trace_event(EV_TICK, "inc", ARG_ULONG_RENAME(tick, xTickCount))
#undef traceINCREASE_TICK_COUNT
#define traceINCREASE_TICK_COUNT(xTicksToJump)		trace_event(EV_TICK, "jump", \
															ARG_ULONG_RENAME(tick, xTickCount), ARG_ULONG_RENAME(jump, xTicksToJump))

#undef traceTASK_NOTIFY
#define traceTASK_NOTIFY(uxIndexToNotify)			trace_event(EV_RT_NOTIFY, "notify", ARG_ULONG_RENAME(idx, uxIndexToNotify))
#undef traceTASK_NOTIFY_FROM_ISR
#define traceTASK_NOTIFY_FROM_ISR(uxIndexToNotify)	trace_event(EV_RT_NOTIFY, "notify", ARG_ULONG_RENAME(idx, uxIndexToNotify))
#undef traceTASK_NOTIFY_GIVE_FROM_ISR
#define traceTASK_NOTIFY_GIVE_FROM_ISR(uxIndexToNotify)	trace_event(EV_RT_NOTIFY, "give", ARG_ULONG_RENAME(idx, uxIndexToNotify))
#undef traceTASK_NOTIFY_TAKE
#define traceTASK_NOTIFY_TAKE(uxIndexToWait)		trace_event(EV_RT_NOTIFY, "take", ARG_ULONG_RENAME(idx, uxIndexToWait))
#undef traceTASK_NOTIFY_TAKE_BLOCK
#define traceTASK_NOTIFY_TAKE_BLOCK(uxIndexToWait)	trace_event(EV_RT_NOTIFY, "take_block", \
														ARG_ULONG_RENAME(idx, uxIndexToWait), ARG_ULONG_RENAME(wait, xTicksToWait))
#undef traceTASK_NOTIFY_WAIT
#define traceTASK_NOTIFY_WAIT(uxIndexToWait)		trace_event(EV_RT_NOTIFY, "wait", ARG_ULONG_RENAME(idx, uxIndexToWait))
#undef traceTASK_NOTIFY_WAIT_BLOCK
#define traceTASK_NOTIFY_WAIT_BLOCK(uxIndexToWait)	trace_event(EV_RT_NOTIFY, "wait_block", \
														ARG_ULONG_RENAME(idx, uxIndexToWait), ARG_ULONG_RENAME(wait, xTicksToWait))

#undef traceTASK_SWITCHED_IN
#define traceTASK_SWITCHED_IN()				trace_event(EV_SCHE, "in", ARG_PTR_RENAME(tcb, pxCurrentTCB))
#undef traceTASK_SWITCHED_OUT
#define traceTASK_SWITCHED_OUT()			trace_event(EV_SCHE, "out", ARG_PTR_RENAME(tcb, pxCurrentTCB))

#undef traceMALLOC
#define traceMALLOC(pvReturn, xWantedSize)				trace_event(EV_MEM, "malloc", \
															ARG_PTR_RENAME(ptr, pvReturn), ARG_ULONG_RENAME(size, xWantedSize))
#undef traceFREE
#define traceFREE(pvReturn, xWantedSize)				trace_event(EV_MEM, "free", \
															ARG_PTR_RENAME(ptr, pvReturn), ARG_ULONG_RENAME(size, xWantedSize))

#undef traceQUEUE_SEND
#define traceQUEUE_SEND(pxQueue)					trace_event(EV_RT_QUEUE, "send", ARG_PTR(pxQueue))
#undef traceQUEUE_SEND_FROM_ISR
#define traceQUEUE_SEND_FROM_ISR(pxQueue)			trace_event(EV_RT_QUEUE, "send", ARG_PTR(pxQueue))

#undef traceQUEUE_SEND_FAILED
#define traceQUEUE_SEND_FAILED(pxQueue)				trace_event(EV_RT_QUEUE, "send_fail", ARG_PTR(pxQueue))
#undef traceQUEUE_SEND_FROM_ISR_FAILED
#define traceQUEUE_SEND_FROM_ISR_FAILED(pxQueue)	trace_event(EV_RT_QUEUE, "send_fail", ARG_PTR(pxQueue))

#undef traceBLOCKING_ON_QUEUE_SEND
#define traceBLOCKING_ON_QUEUE_SEND(pxQueue)		trace_event(EV_RT_QUEUE, "send_block", ARG_PTR(pxQueue))

#undef traceQUEUE_RECEIVE
#define traceQUEUE_RECEIVE(pxQueue)					trace_event(EV_RT_QUEUE, "recv", ARG_PTR(pxQueue))
#undef traceQUEUE_RECEIVE_FROM_ISR
#define traceQUEUE_RECEIVE_FROM_ISR(pxQueue)		trace_event(EV_RT_QUEUE, "recv", ARG_PTR(pxQueue))

#undef traceQUEUE_RECEIVE_FAILED
#define traceQUEUE_RECEIVE_FAILED(pxQueue)			trace_event(EV_RT_QUEUE, "recv_fail", ARG_PTR(pxQueue))
#undef traceQUEUE_RECEIVE_FROM_ISR_FAILED
#define traceQUEUE_RECEIVE_FROM_ISR_FAILED(pxQueue)	trace_event(EV_RT_QUEUE, "recv_fail", ARG_PTR(pxQueue))

#undef traceBLOCKING_ON_QUEUE_RECEIVE
#define traceBLOCKING_ON_QUEUE_RECEIVE(pxQueue)		trace_event(EV_RT_QUEUE, "recv_block", ARG_PTR(pxQueue))

#undef traceQUEUE_PEEK
#define traceQUEUE_PEEK(pxQueue)					trace_event(EV_RT_QUEUE, "peek", ARG_PTR(pxQueue))
#undef traceQUEUE_PEEK_FROM_ISR
#define traceQUEUE_PEEK_FROM_ISR(pxQueue)			trace_event(EV_RT_QUEUE, "peek", ARG_PTR(pxQueue))

#undef traceTIMER_CREATE
#define traceTIMER_CREATE(pxNewTimer)				trace_event(EV_RT_TMR, "create", ARG_PTR_RENAME(timer, pxNewTimer))

#undef traceTIMER_CREATE_FAILED
#define traceTIMER_CREATE_FAILED(pxNewTimer)		trace_event(EV_RT_TMR, "create_fail", ARG_PTR_RENAME(timer, pxNewTimer))

#undef traceTIMER_COMMAND_RECEIVED
#define traceTIMER_COMMAND_RECEIVED(pxTimer, xMessageID, xMessageValue) \
													trace_event(EV_RT_TMR, "rx_cmd", \
														ARG_PTR_RENAME(timer, pxTimer), ARG_INT_RENAME(id, xMessageID), \
														ARG_ULONG_RENAME(val, xMessageValue))

#undef traceTIMER_COMMAND_SEND
#define traceTIMER_COMMAND_SEND(pxTimer, xMessageID, xMessageValue, xReturn) \
													trace_event(EV_RT_TMR, "tx_cmd", ARG_PTR_RENAME(timer, pxTimer), \
														ARG_INT_RENAME(id, xMessageID), ARG_ULONG_RENAME(val, xMessageValue), \
														RET_ARG(xReturn))
#undef traceTIMER_EXPIRED
#define traceTIMER_EXPIRED(pxTimer)				trace_event(EV_RT_TMR, "expired", ARG_PTR_RENAME(timer, pxTimer))

#undef tracePEND_FUNC_CALL
#define tracePEND_FUNC_CALL(xFunctionToPend, pvParameter1, ulParameter2, xReturn) \
												trace_event(EV_RT_TMR, "func", ARG_PTR_RENAME(func, xFunctionToPend), \
														ARG_PTR_RENAME(arg1, pvParameter1), \
														ARG_ULONG_RENAME(arg2, ulParameter2), RET_ARG(xReturn))
#undef tracePEND_FUNC_CALL_FROM_ISR
#define tracePEND_FUNC_CALL_FROM_ISR(xFunc, arg1, arg2, ret)		tracePEND_FUNC_CALL(xFunc, arg1, arg2, ret)

#endif
