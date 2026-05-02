#include <linux/err.h>
#include <linux/list.h>

#include "regs_ops.c"

static LIST_HEAD(g_sync_group_list);
static DEFINE_SPINLOCK(g_sync_group_lock);

struct sync_group_t;
struct port_sync_t {
	u32 idx;
	u32 enable;
	struct sync_group_t *group;
};

struct sync_group_t {
	struct list_head list;
	struct device_node *np;
	u32 items;
	u32 timing;
	void (*writel_regs)(u32 *vals, void **regs, u32 count);
	void **regs;
	u32 *start_vals;
	u32 *stop_vals;
	struct port_sync_t sync[0];
};

#define CHECK_PTR(_ptr, _fail_ops)			do { if (IS_ERR_OR_NULL((_ptr))) { _fail_ops; } } while (0)
#define GET_SYNC_GROUP(_sync, _sync_group, _fail_ops)	do { \
	CHECK_PTR((sync), _fail_ops); \
	(_sync_group) = sync->group; \
	CHECK_PTR((_sync_group), _fail_ops); \
	if ((_sync) != &(_sync_group)->sync[(_sync)->idx]) { \
		_fail_ops; \
	} \
} while (0)

struct sync_group_t *sync_parse_dts(struct device_node *np, u32 *pidx)
{
	u32 timing, items, idx;
	struct sync_group_t *sync_group;
	struct device_node *sync_np;
	struct of_phandle_args args;
	size_t size;

	if (of_parse_phandle_with_args(np, "sync", "#sync-cells", 0, &args))
		return NULL;

	sync_np = args.np;
	idx = args.args[0];
	if (of_property_read_u32(sync_np, "items", &items))
		return ERR_PTR(-ENODATA);
	if (of_property_read_u32(sync_np, "timing", &timing))
		return ERR_PTR(-ENODATA);
	if (idx >= items)
		return ERR_PTR(-EINVAL);

	size = sizeof(*sync_group) + items * sizeof(sync_group->sync[0]);
	size += items * sizeof(sync_group->regs[0]);
	size += items * (sizeof(sync_group->start_vals[0]) + sizeof(sync_group->stop_vals[0]));
	sync_group = kzalloc(size, GFP_KERNEL);
	if (!sync_group)
		return ERR_PTR(-ENOMEM);

	INIT_LIST_HEAD(&sync_group->list);
	sync_group->np = sync_np;
	sync_group->items = items;
	sync_group->timing = timing;
	sync_group->writel_regs = timing ? writel_regs_timing : writel_regs_fast;
	sync_group->regs = (void **)&sync_group->sync[items];
	sync_group->start_vals = (u32 *)&sync_group->regs[items];
	sync_group->stop_vals = (u32 *)&sync_group->start_vals[items];
	*pidx = idx;
	return sync_group;
}

static struct port_sync_t *register_sync_group(struct device_node *np, void *reg, u32 enable_val, u32 disable_val)
{
	unsigned long flags;
	u32 idx;
	struct sync_group_t *sync_group, *tmp_sync_group, *alloc_sync_group;
	struct port_sync_t *sync;

	alloc_sync_group = sync_parse_dts(np, &idx);
	CHECK_PTR(alloc_sync_group, return ERR_PTR(PTR_ERR(alloc_sync_group)));

	spin_lock_irqsave(&g_sync_group_lock, flags);
	list_for_each_entry_safe(sync_group, tmp_sync_group, &g_sync_group_list, list) {
		/* found, using registerd */
		if ((alloc_sync_group->np == sync_group->np) && (alloc_sync_group->items == sync_group->items)) {
			if (sync_group->sync[idx].group)
				sync_group = ERR_PTR(-EALREADY);
			goto out;
		}
	}

	/* not found, register and using alloc */
	sync_group = alloc_sync_group;
	alloc_sync_group = NULL;
	list_add_tail(&sync_group->list, &g_sync_group_list);
out:
	if (!IS_ERR_OR_NULL(sync_group)) {
		sync = &sync_group->sync[idx];
		sync->group = sync_group;
		sync->idx = idx;
		sync_group->regs[idx] = reg;
		sync_group->start_vals[idx] = enable_val;
		sync_group->stop_vals[idx] = disable_val;
	}
	spin_unlock_irqrestore(&g_sync_group_lock, flags);
	if (alloc_sync_group)
		kfree(alloc_sync_group);

	CHECK_PTR(sync_group, return ERR_PTR(PTR_ERR(sync_group)));
	return sync;
}

static void unregister_sync_group(struct port_sync_t *sync)
{
	unsigned long flags;
	u32 idx, items;
	struct sync_group_t *sync_group;

	GET_SYNC_GROUP(sync, sync_group, return);

	spin_lock_irqsave(&g_sync_group_lock, flags);
	idx = sync->idx;
	sync_group->sync[idx].group = NULL;
	items = sync_group->items;
	for (idx = 0; idx < items; idx++) {
		/* someone used, not need to free */
		if (sync_group->sync[idx].group) {
			sync_group = NULL;
			goto out;
		}
	}

	/* nobody used */
	list_del(&sync_group->list);
out:
	spin_unlock_irqrestore(&g_sync_group_lock, flags);
	if (sync_group)
		kfree(sync_group);
}

static int sync_start(struct port_sync_t *sync)
{
	unsigned long flags;
	u32 idx, items;
	int ret = 1;
	struct sync_group_t *sync_group;

	GET_SYNC_GROUP(sync, sync_group, return -EINVAL);

	spin_lock_irqsave(&g_sync_group_lock, flags);
	idx = sync->idx;
	items = sync_group->items;
	if (sync_group->sync[idx].enable)
		goto out;

	sync_group->sync[idx].enable = 1;
	for (idx = 0; idx < sync_group->items; idx++) {
		/* someone not enable, not need to sync enable */
		if (!sync_group->sync[idx].enable)
			goto out;
	}

	sync_group->writel_regs(sync_group->stop_vals, sync_group->regs, sync_group->items);
	udelay(50);
	sync_group->writel_regs(sync_group->start_vals, sync_group->regs, sync_group->items);
	ret = 0;
out:
	spin_unlock_irqrestore(&g_sync_group_lock, flags);

	return ret;
}

static int sync_stop(struct port_sync_t *sync)
{
	unsigned long flags;
	u32 idx, items;
	int ret = 1;
	struct sync_group_t *sync_group;

	GET_SYNC_GROUP(sync, sync_group, return -EINVAL);

	spin_lock_irqsave(&g_sync_group_lock, flags);
	idx = sync->idx;
	items = sync_group->items;
	if (!sync_group->sync[idx].enable)
		goto out;

	sync_group->sync[idx].enable = 0;
	for (idx = 0; idx < sync_group->items; idx++) {
		if (!sync_group->sync[idx].enable)
			goto out;
	}

	sync_group->writel_regs(sync_group->stop_vals, sync_group->regs, sync_group->items);
	ret = 0;
out:
	spin_unlock_irqrestore(&g_sync_group_lock, flags);
	return ret;
}
