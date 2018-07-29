/* MMC PHY Registers */
#define PHYCTRL_CTRL1_REG	0x00
#define PHYCTRL_CTRL2_REG	0x04
#define PHYCTRL_CTRL3_REG	0x08
#define PHYCTRL_CTRL4_REG	0x0C
#define PHYCTRL_CTRL5_REG	0x10
#define PHYCTRL_CTRL6_REG	0x14
#define PHYCTRL_STAT1_REG	0x30
#define PHYCTRL_STAT2_REG	0x34

#define IOMUX_ENABLE_SHIFT	31
#define IOMUX_ENABLE_MASK	BIT(IOMUX_ENABLE_SHIFT)
#define OTAPDLYENA_SHIFT	20
#define OTAPDLYENA_MASK	BIT(OTAPDLYENA_SHIFT)
#define OTAPDLYSEL_SHIFT	12
#define OTAPDLYSEL_MASK	GENMASK(15, 12)
#define STRBSEL_SHIFT		24
#define STRBSEL_MASK		GENMASK(27, 24)
#define SEL50_SHIFT		8
#define SEL50_MASK		BIT(SEL50_SHIFT)
#define SEL100_SHIFT		9
#define SEL100_MASK		BIT(SEL100_SHIFT)
#define DLL_TRIM_ICP_SHIFT	4
#define DLL_TRIM_ICP_MASK	GENMASK(7, 4)
#define DR_TY_SHIFT		20
#define DR_TY_MASK		GENMASK(22, 20)
#define ENDLL_SHIFT		1
#define ENDLL_MASK		BIT(ENDLL_SHIFT)
#define DLLRDY_SHIFT		0
#define DLLRDY_MASK		BIT(DLLRDY_SHIFT)

#define DRIVER_STRENGTH_50_OHM	0x0
#define DRIVER_STRENGTH_33_OHM	0x1
#define DRIVER_STRENGTH_66_OHM	0x2
#define DRIVER_STRENGTH_100_OHM	0x3
#define DRIVER_STRENGTH_40_OHM	0x4

#include <clk.h>
#include <common.h>
#include <dm.h>
#include <generic-phy.h>
#include <regmap.h>

DECLARE_GLOBAL_DATA_PTR;

struct am654_mmc_phy {
	struct regmap *reg_base;
	u32 otap_del_sel;
	u32 trm_icp;
	u32 drv_strength;
};

static int am654_mmc_phy_init(struct phy *phy)
{
	struct am654_mmc_phy *mmc_phy = dev_get_priv(phy->dev);

	/* Enable pins by setting the IO mux to 0 */
	regmap_update_bits(mmc_phy->reg_base, PHYCTRL_CTRL1_REG,
			   IOMUX_ENABLE_MASK, 0);

	return 0;
}

static int am654_mmc_phy_power_on(struct phy *phy)
{
	struct am654_mmc_phy *mmc_phy = dev_get_priv(phy->dev);
	u32 mask, val;
	int sel50, sel100;
	int rate;

	if (!phy->priv) {
		dev_err(phy->dev, "No clock speed\n");
		return -EINVAL;
	}

	rate = *(unsigned int *)phy->priv;

	/* Setup DLL Output TAP delay */
	mask = OTAPDLYENA_MASK | OTAPDLYSEL_MASK;
	val = (1 << OTAPDLYENA_SHIFT) |
	      (mmc_phy->otap_del_sel << OTAPDLYSEL_SHIFT);
	regmap_update_bits(mmc_phy->reg_base, PHYCTRL_CTRL4_REG,
			   mask, val);

	switch (rate) {
	case 200000000:
		sel50 = 0;
		sel100 = 0;
		break;
	case 100000000:
		sel50 = 0;
		sel100 = 1;
		break;
	default:
		sel50 = 1;
		sel100 = 0;
	}

	/* Configure PHY DLL frequency */
	mask = SEL50_MASK | SEL100_MASK;
	val = (sel50 << SEL50_SHIFT) | (sel100 << SEL100_SHIFT);
	regmap_update_bits(mmc_phy->reg_base, PHYCTRL_CTRL5_REG,
			   mask, val);

	/* Configure DLL TRIM */
	mask = DLL_TRIM_ICP_MASK;
	val = mmc_phy->trm_icp << DLL_TRIM_ICP_SHIFT;

	/* Configure DLL driver strength */
	mask |= DR_TY_MASK;
	val |= mmc_phy->drv_strength << DR_TY_SHIFT;
	regmap_update_bits(mmc_phy->reg_base, PHYCTRL_CTRL1_REG, mask, val);

	/* Enable DLL */
	regmap_update_bits(mmc_phy->reg_base, PHYCTRL_CTRL1_REG,
			   ENDLL_MASK, 0x1 << ENDLL_SHIFT);

	/*
	 * Poll for DLL ready. Use a one second timeout.
	 * Works in all experiments done so far
	 */
	return regmap_read_poll_timeout(mmc_phy->reg_base, PHYCTRL_STAT1_REG,
					val, val & DLLRDY_MASK, 1000000);

}

static int am654_mmc_phy_power_off(struct phy *phy)
{
	struct am654_mmc_phy *mmc_phy = dev_get_priv(phy->dev);

	/* Disable DLL */
	regmap_update_bits(mmc_phy->reg_base, PHYCTRL_CTRL1_REG,
			   ENDLL_MASK, 0);

	return 0;
}

static const struct phy_ops am654_mmc_phy_ops = {
	.init = am654_mmc_phy_init,
	.power_on = am654_mmc_phy_power_on,
	.power_off = am654_mmc_phy_power_off,
};

int am654_mmc_phy_probe(struct udevice *dev)
{
	struct am654_mmc_phy *mmc_phy = dev_get_priv(dev);
	u32 drv_strength;
	int err;

	err = dev_read_u32(dev, "ti,otap-del-sel",
			   &mmc_phy->otap_del_sel);
	if (err)
		return err;

	err = dev_read_u32(dev, "ti,trm-icp",
			   &mmc_phy->trm_icp);
	if (err)
		return err;

	err = dev_read_u32(dev, "ti,drv-strength-ohm", &drv_strength);
	if (err)
		return err;

	switch (drv_strength) {
	case 50:
		mmc_phy->drv_strength = DRIVER_STRENGTH_50_OHM;
		break;
	case 33:
		mmc_phy->drv_strength = DRIVER_STRENGTH_33_OHM;
		break;
	case 66:
		mmc_phy->drv_strength = DRIVER_STRENGTH_66_OHM;
		break;
	case 100:
		mmc_phy->drv_strength = DRIVER_STRENGTH_100_OHM;
		break;
	case 40:
		mmc_phy->drv_strength = DRIVER_STRENGTH_40_OHM;
		break;
	default:
		dev_err(dev, "Invalid driver strength\n");
		return -EINVAL;
	}

	return regmap_init_mem(dev, &mmc_phy->reg_base);
}

static const struct udevice_id am654_mmc_phy_ids[] = {
	{.compatible = "ti,am654-mmc-phy"},
};

U_BOOT_DRIVER(am654_mmc_phy) = {
	.name	= "am654_mmc_phy",
	.id	= UCLASS_PHY,
	.of_match = am654_mmc_phy_ids,
	.ops = &am654_mmc_phy_ops,
	.probe = am654_mmc_phy_probe,
	.priv_auto_alloc_size = sizeof(struct am654_mmc_phy),
};
