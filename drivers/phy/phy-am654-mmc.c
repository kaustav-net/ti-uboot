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

#include <common.h>
#include <dm.h>
#include <generic-phy.h>
#include <regmap.h>

struct am654_mmc_phy {
	struct regmap *reg_base;
};

static int am654_mmc_phy_init(struct phy *phy)
{
	struct am654_mmc_phy *mmc_phy = dev_get_priv(phy->dev);

	/* Enable pins by setting the IO mux to 0 */
	regmap_update_bits(mmc_phy->reg_base, PHYCTRL_CTRL1_REG,
			   IOMUX_ENABLE_MASK, 0);

	return 0;
}

static const struct phy_ops am654_mmc_phy_ops = {
	.init = am654_mmc_phy_init,
};

int am654_mmc_phy_probe(struct udevice *dev)
{
	struct am654_mmc_phy *mmc_phy = dev_get_priv(dev);

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
