// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner USB3.0/PCIE/DisplayPort Combo Phy driver
 *
 * Copyright (C) 2024 Allwinner Electronics Co., Ltd.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/phy/phy.h>
#include <linux/reset.h>
#include <dt-bindings/phy/phy.h>

struct sunxi_cadence_combophy {
	const char *name;
	void __iomem *top_reg;
	void __iomem *phy_reg;
	struct clk *clk;
	struct clk *bus_clk;
	struct reset_control *reset;
	struct phy *phy;
	struct sunxi_cadence_phy *sunxi_cphy;
	atomic_t busy;
};

struct sunxi_cadence_phy {
	void __iomem *top_subsys_reg;
	void __iomem *top_combo_reg;
	struct device *dev;
	struct clk *serdes_clk;
	struct clk *dcxo_serdes1_clk;
	struct reset_control *serdes_reset;

	struct sunxi_cadence_combophy *combo0;
	struct sunxi_cadence_combophy *combo1;
	struct sunxi_cadence_combophy *aux_hpd;

	__u8 mode;
};

enum phy_type_e {
	COMBO_PHY0 = 0,
	COMBO_PHY1,
	AUX_HPD,
};

/* sysrtc */
#define DCXO_SERDES1_GATING			BIT(5)

/* serdes */
#define SUBSYS_PCIE_BGR				0x4
#define SUBSYS_PCIE_GATING			(BIT(16) | BIT(17) | BIT(18))
#define SUBSYS_DBG_CTL				0xf0
#define SUBSYS_DISABLE_COMBO1_AUTOGATING	BIT(29)
#define SUBSYS_COMB1_PIPE			0xc44
#define SUBSYS_COMB1_PIPE_PCIE			0x1

static int __maybe_unused sunxi_cadence_phy_suspend(struct device *dev)
{
	return 0;
}

static int __maybe_unused sunxi_cadence_phy_resume(struct device *dev)
{
	return 0;
}

static void sunxi_cadence_phy_reset_control_put(void *data)
{
	reset_control_put(data);
}

static struct dev_pm_ops sunxi_cadence_phy_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(sunxi_cadence_phy_suspend, sunxi_cadence_phy_resume)
};

static int sunxi_cadence_phy_combo1_pcie_init(struct sunxi_cadence_phy *sunxi_cphy)
{
	int ret;
	u32 val;

	ret = clk_set_rate(sunxi_cphy->serdes_clk, 100000000);
	if (ret)
		return ret;

	ret = clk_prepare_enable(sunxi_cphy->serdes_clk);
	if (ret)
		return ret;

	ret = clk_prepare_enable(sunxi_cphy->dcxo_serdes1_clk);
	if (ret)
		return ret;

	/* enable pcie */
	val = readl(sunxi_cphy->top_subsys_reg + SUBSYS_PCIE_BGR);
	writel(val | SUBSYS_PCIE_GATING, sunxi_cphy->top_subsys_reg + SUBSYS_PCIE_BGR);

	/* disable combo1 auto gating */
	val = readl(sunxi_cphy->top_subsys_reg + SUBSYS_DBG_CTL);
	writel(val | SUBSYS_DISABLE_COMBO1_AUTOGATING, sunxi_cphy->top_subsys_reg + SUBSYS_DBG_CTL);

	/* switch pipe to pcie */
	writel(SUBSYS_COMB1_PIPE_PCIE, sunxi_cphy->top_combo_reg + SUBSYS_COMB1_PIPE);

	return 0;
}

static void sunxi_cadence_phy_combo1_pcie_exit(struct sunxi_cadence_phy *sunxi_cphy)
{
	clk_disable_unprepare(sunxi_cphy->serdes_clk);
	clk_disable_unprepare(sunxi_cphy->dcxo_serdes1_clk);
}

static int sunxi_cadence_phy_combo1_init(struct phy *phy)
{
	struct sunxi_cadence_combophy *combo1 = phy_get_drvdata(phy);
	struct sunxi_cadence_phy *sunxi_cphy = combo1->sunxi_cphy;
	int ret = 0;

	if (sunxi_cphy->mode == PHY_TYPE_PCIE)
		ret = sunxi_cadence_phy_combo1_pcie_init(sunxi_cphy);

	return ret;
}

static int sunxi_cadence_phy_combo1_exit(struct phy *phy)
{
	struct sunxi_cadence_combophy *combo1 = phy_get_drvdata(phy);
	struct sunxi_cadence_phy *sunxi_cphy = combo1->sunxi_cphy;

	if (sunxi_cphy->mode == PHY_TYPE_PCIE)
		sunxi_cadence_phy_combo1_pcie_exit(sunxi_cphy);

	return 0;
}

static void sunxi_cadence_phy_combo1_release(struct phy *phy)
{
	struct sunxi_cadence_combophy *combo1 = phy_get_drvdata(phy);

	atomic_dec(&combo1->busy);
}

static const struct phy_ops combo_phy0_ops = {
	.owner		= THIS_MODULE,
};

static const struct phy_ops combo_phy1_ops = {
	.init		= sunxi_cadence_phy_combo1_init,
	.exit		= sunxi_cadence_phy_combo1_exit,
	.release	= sunxi_cadence_phy_combo1_release,
	.owner		= THIS_MODULE,
};

static const struct phy_ops aux_hpd_ops = {
	.owner		= THIS_MODULE,
};


int sunxi_cadence_phy_create(struct device *dev, struct device_node *np,
			     struct sunxi_cadence_combophy *combophy, enum phy_type_e type)
{
	struct sunxi_cadence_phy *sunxi_cphy = dev_get_drvdata(dev);
	const struct phy_ops *ops;
	int ret;

	switch (type) {
	case COMBO_PHY0:
		combophy->name = kstrdup_const("combophy0", GFP_KERNEL);
		ops = &combo_phy0_ops;
		break;
	case COMBO_PHY1:
		combophy->name = kstrdup_const("combophy1", GFP_KERNEL);
		ops = &combo_phy1_ops;
		break;
	case AUX_HPD:
		combophy->name = kstrdup_const("aux_hpd", GFP_KERNEL);
		ops = &aux_hpd_ops;
		break;
	default:
		pr_err("not support phy type (%d)\n", type);
		return -EINVAL;
	}

	combophy->clk = devm_get_clk_from_child(dev, np, "phy-clk");
	if (IS_ERR(combophy->clk)) {
		combophy->clk = NULL;
		pr_debug("Maybe there is no clk for phy (%s)\n", combophy->name);
	}

	combophy->bus_clk = devm_get_clk_from_child(dev, np, "phy-bus-clk");
	if (IS_ERR(combophy->bus_clk)) {
		combophy->bus_clk = NULL;
		pr_debug("Maybe there is no bus clk for phy (%s)\n", combophy->name);
	}

	combophy->reset = of_reset_control_get(np, "phy_reset");
	if (IS_ERR(combophy->reset)) {
		combophy->reset = NULL;
		pr_debug("Maybe there is no reset for phy (%s)\n", combophy->name);
	}

	if (combophy->reset) {
		ret = devm_add_action_or_reset(dev, sunxi_cadence_phy_reset_control_put,
					       combophy->reset);
		if (ret)
			return ret;
	}

	combophy->top_reg = of_iomap(np, 0);
	if (!combophy->top_reg)
		return -ENOMEM;

	combophy->phy_reg = of_iomap(np, 1);
	if (!combophy->phy_reg) {
		combophy->phy_reg = NULL;
		pr_debug("Maybe there is no phy reg for %s\n", combophy->name);
	}

	combophy->phy = devm_phy_create(dev, np, ops);
	if (IS_ERR(combophy->phy)) {
		ret = PTR_ERR(combophy->phy);
		dev_err(dev, "failed to create phy for %s, ret:%d\n", combophy->name, ret);
		return ret;
	}

	atomic_set(&combophy->busy, 0);
	combophy->sunxi_cphy = sunxi_cphy;
	phy_set_drvdata(combophy->phy, combophy);

	return 0;
}

static struct phy *sunxi_cadence_phy_xlate(struct device *dev,
					  struct of_phandle_args *args)
{
	struct sunxi_cadence_phy *sunxi_cphy = dev_get_drvdata(dev);
	struct phy *phy = NULL;

	if (args->args_count != 1) {
		dev_err(dev, "invalid number of arguments\n");
		return ERR_PTR(-EINVAL);
	}

	if (sunxi_cphy->mode != PHY_NONE && sunxi_cphy->mode != args->args[0])
		dev_warn(dev, "phy type select %d overwriting type %d\n",
			 args->args[0], sunxi_cphy->mode);

	sunxi_cphy->mode = args->args[0];

	if (!atomic_read(&sunxi_cphy->combo0->busy) && sunxi_cphy->mode != PHY_TYPE_PCIE) {
		atomic_inc(&sunxi_cphy->combo0->busy);
		phy = sunxi_cphy->combo0->phy;
	}

	if (!atomic_read(&sunxi_cphy->combo1->busy) && sunxi_cphy->mode != PHY_TYPE_DP) {
		atomic_inc(&sunxi_cphy->combo1->busy);
		phy = sunxi_cphy->combo1->phy;
	}

	return phy;
}

static int sunxi_cadence_phy_serdes_init(struct sunxi_cadence_phy *sunxi_cphy)
{
	int ret;

	ret = reset_control_deassert(sunxi_cphy->serdes_reset);
	if (ret)
		return ret;

	return 0;
}

static void sunxi_cadence_phy_serdes_exit(struct sunxi_cadence_phy *sunxi_cphy)
{
	reset_control_assert(sunxi_cphy->serdes_reset);
}

static int sunxi_cadence_phy_parse_dt(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sunxi_cadence_phy *sunxi_cphy = dev_get_drvdata(dev);
	struct device_node *child;
	int ret;

	/* parse top register, which determide general configuration such as mode */
	sunxi_cphy->top_subsys_reg = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(sunxi_cphy->top_subsys_reg))
		return PTR_ERR(sunxi_cphy->top_subsys_reg);

	sunxi_cphy->top_combo_reg = devm_platform_ioremap_resource(pdev, 1);
	if (IS_ERR(sunxi_cphy->top_combo_reg))
		return PTR_ERR(sunxi_cphy->top_combo_reg);

	sunxi_cphy->serdes_clk = devm_clk_get(dev, "serdes-clk");
	if (IS_ERR(sunxi_cphy->serdes_clk)) {
		return dev_err_probe(dev, PTR_ERR(sunxi_cphy->serdes_clk),
			"failed to get serdes clock for sunxi cadence phy\n");
	}

	sunxi_cphy->serdes_reset = devm_reset_control_get_shared(dev, "serdes-reset");
	if (IS_ERR(sunxi_cphy->serdes_reset)) {
		return dev_err_probe(dev, PTR_ERR(sunxi_cphy->serdes_reset),
			"failed to get serdes reset for sunxi cadence phy\n");
	}

	sunxi_cphy->dcxo_serdes1_clk = devm_clk_get(dev, "dcxo-serdes1-clk");
	if (IS_ERR(sunxi_cphy->dcxo_serdes1_clk)) {
		return dev_err_probe(dev, PTR_ERR(sunxi_cphy->dcxo_serdes1_clk),
			"failed to get dcxo serdes1 clock for sunxi cadence phy\n");
	}

	for_each_available_child_of_node(dev->of_node, child) {
		if (of_node_name_eq(child, "combo-phy0")) {
			sunxi_cphy->combo0 = devm_kzalloc(dev, sizeof(*sunxi_cphy->combo0), GFP_KERNEL);
			if (!sunxi_cphy->combo0)
				return -ENOMEM;

			/* create combophy0 */
			ret = sunxi_cadence_phy_create(dev, child, sunxi_cphy->combo0, COMBO_PHY0);
			if (ret) {
				dev_err(dev, "failed to create cadence combophy0, ret:%d\n", ret);
				goto err_node_put;
			}

		} else if (of_node_name_eq(child, "combo-phy1")) {
			sunxi_cphy->combo1 = devm_kzalloc(dev, sizeof(*sunxi_cphy->combo1), GFP_KERNEL);
			if (!sunxi_cphy->combo1)
				return -ENOMEM;

			/* create combophy1 */
			ret = sunxi_cadence_phy_create(dev, child, sunxi_cphy->combo1, COMBO_PHY1);
			if (ret) {
				dev_err(dev, "failed to create cadence combophy1, ret:%d\n", ret);
				goto err_node_put;
			}

		} else if (of_node_name_eq(child, "aux-hpd")) {
			sunxi_cphy->aux_hpd = devm_kzalloc(dev, sizeof(*sunxi_cphy->aux_hpd), GFP_KERNEL);
			if (!sunxi_cphy->aux_hpd)
				return -ENOMEM;

			/* create aux phy */
			ret = sunxi_cadence_phy_create(dev, child, sunxi_cphy->aux_hpd, AUX_HPD);
			if (ret) {
				dev_err(dev, "failed to create cadence aux-hpd phy, ret:%d\n", ret);
				goto err_node_put;
			}
		}
	}

	return 0;

err_node_put:
	of_node_put(child);

	return ret;
}

static int sunxi_cadence_phy_probe(struct platform_device *pdev)
{
	struct sunxi_cadence_phy *sunxi_cphy;
	struct device *dev = &pdev->dev;
	struct phy_provider *phy_provider;
	int ret;

	sunxi_cphy = devm_kzalloc(dev, sizeof(*sunxi_cphy), GFP_KERNEL);
	if (!sunxi_cphy)
		return -ENOMEM;

	sunxi_cphy->dev = dev;
	dev_set_drvdata(dev, sunxi_cphy);

	ret = sunxi_cadence_phy_parse_dt(pdev);
	if (ret)
		return -EINVAL;

	phy_provider = devm_of_phy_provider_register(dev, sunxi_cadence_phy_xlate);
	if (IS_ERR(phy_provider))
		return PTR_ERR_OR_ZERO(phy_provider);

	ret = sunxi_cadence_phy_serdes_init(sunxi_cphy);
	if (ret)
		return -EINVAL;

	return 0;
}

static int sunxi_cadence_phy_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sunxi_cadence_phy *sunxi_cphy = dev_get_drvdata(dev);

	sunxi_cadence_phy_serdes_exit(sunxi_cphy);

	return 0;
}

static const struct of_device_id sunxi_cadence_phy_of_match_table[] = {
	{
		.compatible = "allwinner,cadence-combophy",
	},
};

static struct platform_driver sunxi_cadence_phy_driver = {
	.probe		= sunxi_cadence_phy_probe,
	.remove		= sunxi_cadence_phy_remove,
	.driver = {
		.name	= "sunxi-cadence-combophy",
		.pm	= &sunxi_cadence_phy_pm_ops,
		.of_match_table = sunxi_cadence_phy_of_match_table,
	},
};
module_platform_driver(sunxi_cadence_phy_driver);

MODULE_AUTHOR("huangyongxing@allwinnertech.com");
MODULE_DESCRIPTION("Allwinner CADENCE COMBOPHY driver");
MODULE_VERSION("0.0.2");
MODULE_LICENSE("GPL v2");
