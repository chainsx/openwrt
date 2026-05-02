
#define _CONTACT(__STR_X__, __STR_Y__)		__STR_X__##__STR_Y__
#define CONTACT(__STR_X__, __STR_Y__)		_CONTACT(__STR_X__, __STR_Y__)

#define VAR_LOAD(_val, _addr)			do { _val = *_addr; } while (0)
#define VAR_LOAD_IDX(_idx, _val, _vals)		VAR_LOAD(CONTACT(_val, _idx), ((_vals) + (_idx)))
/* eq: _val_idx = _vals[_idx]; */

#define writel_fast_idx(_idx, _val, _reg)	writel_relaxed(CONTACT(_val, _idx), CONTACT(_reg, _idx))
/* eq: writel_relaxed(_val_idx, _reg_idx) */

#define writel_timing_idx(_idx, _val, _reg)	writel_timing(CONTACT(_val, _idx), CONTACT(_reg, _idx))
/* eq: writel_timing(_val_idx, _reg_idx) */

/* writel_relaxed may be 80ns in aw1912 */
#define writel_timing_each(c, val, reg)		writel_relaxed(val, reg)

#define REPEAT_OPS_0(_ops, ...)
#define REPEAT_OPS_1(_ops, ...)			_ops(0, __VA_ARGS__)
#define REPEAT_OPS_2(_ops, ...)			do { REPEAT_OPS_1(_ops, __VA_ARGS__); _ops(1, __VA_ARGS__); } while (0)
#define REPEAT_OPS_3(_ops, ...)			do { REPEAT_OPS_2(_ops, __VA_ARGS__); _ops(2, __VA_ARGS__); } while (0)
#define REPEAT_OPS_4(_ops, ...)			do { REPEAT_OPS_3(_ops, __VA_ARGS__); _ops(3, __VA_ARGS__); } while (0)
#define REPEAT_OPS_5(_ops, ...)			do { REPEAT_OPS_4(_ops, __VA_ARGS__); _ops(4, __VA_ARGS__); } while (0)
#define REPEAT_OPS_6(_ops, ...)			do { REPEAT_OPS_5(_ops, __VA_ARGS__); _ops(5, __VA_ARGS__); } while (0)
#define REPEAT_OPS_7(_ops, ...)			do { REPEAT_OPS_6(_ops, __VA_ARGS__); _ops(6, __VA_ARGS__); } while (0)
#define REPEAT_OPS_8(_ops, ...)			do { REPEAT_OPS_7(_ops, __VA_ARGS__); _ops(7, __VA_ARGS__); } while (0)
#define REPEAT_OPS_9(_ops, ...)			do { REPEAT_OPS_8(_ops, __VA_ARGS__); _ops(8, __VA_ARGS__); } while (0)
#define REPEAT_OPS_10(_ops, ...)		do { REPEAT_OPS_9(_ops, __VA_ARGS__); _ops(9, __VA_ARGS__); } while (0)
#define REPEAT_OPS_11(_ops, ...)		do { REPEAT_OPS_10(_ops, __VA_ARGS__); _ops(10, __VA_ARGS__); } while (0)
#define REPEAT_OPS_12(_ops, ...)		do { REPEAT_OPS_11(_ops, __VA_ARGS__); _ops(11, __VA_ARGS__); } while (0)
#define REPEAT_OPS_I(c, _ops, ...)		REPEAT_OPS_ ## c(_ops, __VA_ARGS__)
#define REPEAT_OPS(c, _ops, ...)		REPEAT_OPS_I(c, _ops, __VA_ARGS__)

/* request timing to 1us */
static inline void writel_timing(u32 val, void *reg)
{
	REPEAT_OPS(12, writel_timing_each, val, reg);
}

void writel_2regs_timing(u32 *vals, void **regs)
{
	u32 val0, val1;
	void *reg0, *reg1;

	REPEAT_OPS(2, VAR_LOAD_IDX, val, vals);
	REPEAT_OPS(2, VAR_LOAD_IDX, reg, regs);

	__iowmb();
	REPEAT_OPS(2, writel_timing_idx, val, reg);
	__iowmb();
}
EXPORT_SYMBOL_GPL(writel_2regs_timing);

void writel_3regs_timing(u32 *vals, void **regs)
{
	u32 val0, val1, val2;
	void *reg0, *reg1, *reg2;

	REPEAT_OPS(3, VAR_LOAD_IDX, val, vals);
	REPEAT_OPS(3, VAR_LOAD_IDX, reg, regs);

	__iowmb();
	REPEAT_OPS(3, writel_timing_idx, val, reg);
	__iowmb();
}
EXPORT_SYMBOL_GPL(writel_3regs_timing);

void writel_4regs_timing(u32 *vals, void **regs)
{
	u32 val0, val1, val2, val3;
	void *reg0, *reg1, *reg2, *reg3;

	REPEAT_OPS(4, VAR_LOAD_IDX, val, vals);
	REPEAT_OPS(4, VAR_LOAD_IDX, reg, regs);

	__iowmb();
	REPEAT_OPS(4, writel_timing_idx, val, reg);
	__iowmb();
}

void writel_regs_timing(u32 *vals, void **regs, u32 count)
{
	u32 idx;

	switch (count) {
	case 2:
		writel_2regs_timing(&vals[0], &regs[0]);
		break;
	case 3:
		writel_3regs_timing(&vals[0], &regs[0]);
		break;
	case 4:
		writel_4regs_timing(&vals[0], &regs[0]);
		break;
	default:
		__iowmb();
		for (idx = 0; idx < count; idx++)
			writel_timing(vals[idx], regs[idx]);
		__iowmb();
		break;
	}
}
EXPORT_SYMBOL_GPL(writel_regs_timing);

void writel_2regs_fast(u32 *vals, void **regs)
{
	u32 val0, val1;
	void *reg0, *reg1;

	REPEAT_OPS(2, VAR_LOAD_IDX, val, vals);
	REPEAT_OPS(2, VAR_LOAD_IDX, reg, regs);

	__iowmb();
	REPEAT_OPS(2, writel_fast_idx, val, reg);
	__iowmb();
}
EXPORT_SYMBOL_GPL(writel_2regs_fast);

void writel_3regs_fast(u32 *vals, void **regs)
{
	u32 val0, val1, val2;
	void *reg0, *reg1, *reg2;

	REPEAT_OPS(3, VAR_LOAD_IDX, val, vals);
	REPEAT_OPS(3, VAR_LOAD_IDX, reg, regs);

	__iowmb();
	REPEAT_OPS(3, writel_fast_idx, val, reg);
	__iowmb();
}
EXPORT_SYMBOL_GPL(writel_3regs_fast);

void writel_4regs_fast(u32 *vals, void **regs)
{
	u32 val0, val1, val2, val3;
	void *reg0, *reg1, *reg2, *reg3;

	REPEAT_OPS(4, VAR_LOAD_IDX, val, vals);
	REPEAT_OPS(4, VAR_LOAD_IDX, reg, regs);

	__iowmb();
	REPEAT_OPS(4, writel_fast_idx, val, reg);
	__iowmb();
}

void writel_regs_fast(u32 *vals, void **regs, u32 count)
{
	u32 idx;

	switch (count) {
	case 2:
		writel_2regs_fast(&vals[0], &regs[0]);
		break;
	case 3:
		writel_3regs_fast(&vals[0], &regs[0]);
		break;
	case 4:
		writel_4regs_fast(&vals[0], &regs[0]);
		break;
	default:
		__iowmb();
		for (idx = 0; idx < count; idx++)
			writel_relaxed(vals[idx], regs[idx]);
		__iowmb();
		break;
	}
}
EXPORT_SYMBOL_GPL(writel_regs_fast);
