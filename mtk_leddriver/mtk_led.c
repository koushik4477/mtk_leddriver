#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/regmap.h>
#include <linux/io.h>
#include <linux/pm_runtime.h>
#include <linux/spinlock.h>
#include <linux/ktime.h>
#include "mtk_led.h"

struct mtk_led {
    struct device *dev;
    void __iomem *base;
    struct regmap *regmap;
    struct clk *clk;
    int irq;
    spinlock_t lock;

    /* performance stats */
    u64 toggle_count;
    u64 total_latency_ns;
};

static const struct regmap_config mtk_led_regmap_config = {
    .reg_bits = 32,
    .val_bits = 32,
    .reg_stride = 4,
    .max_register = 0x20,
};

static irqreturn_t mtk_led_irq(int irq, void *data)
{
    struct mtk_led *led = data;
    unsigned long flags;
    u32 status;

    spin_lock_irqsave(&led->lock, flags);

    regmap_read(led->regmap, LED_INT_STATUS, &status);
    if (status & LED_INT_BIT) {
        regmap_write(led->regmap, LED_INT_STATUS, LED_INT_BIT); // clear
    }

    spin_unlock_irqrestore(&led->lock, flags);

    return IRQ_HANDLED;
}

static ssize_t toggle_show(struct device *dev,
                           struct device_attribute *attr, char *buf)
{
    struct mtk_led *led = dev_get_drvdata(dev);
    return sprintf(buf, "Toggles: %llu\nAvg latency(ns): %llu\n",
                   led->toggle_count,
                   led->toggle_count ?
                   led->total_latency_ns / led->toggle_count : 0);
}

static ssize_t toggle_store(struct device *dev,
                            struct device_attribute *attr,
                            const char *buf, size_t count)
{
    struct mtk_led *led = dev_get_drvdata(dev);
    ktime_t start, end;
    unsigned long flags;

    pm_runtime_get_sync(dev);

    start = ktime_get();

    spin_lock_irqsave(&led->lock, flags);
    regmap_write(led->regmap, LED_TOGGLE_REG, 1);
    spin_unlock_irqrestore(&led->lock, flags);

    end = ktime_get();

    led->toggle_count++;
    led->total_latency_ns += ktime_to_ns(ktime_sub(end, start));

    pm_runtime_put(dev);

    return count;
}

static DEVICE_ATTR_RW(toggle);

static int mtk_led_probe(struct platform_device *pdev)
{
    struct mtk_led *led;
    struct resource *res;

    led = devm_kzalloc(&pdev->dev, sizeof(*led), GFP_KERNEL);
    if (!led)
        return -ENOMEM;

    led->dev = &pdev->dev;
    spin_lock_init(&led->lock);

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    led->base = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(led->base))
        return PTR_ERR(led->base);

    led->regmap = devm_regmap_init_mmio(&pdev->dev, led->base,
                                        &mtk_led_regmap_config);
    if (IS_ERR(led->regmap))
        return PTR_ERR(led->regmap);

    led->clk = devm_clk_get(&pdev->dev, NULL);
    if (IS_ERR(led->clk))
        return PTR_ERR(led->clk);

    clk_prepare_enable(led->clk);

    led->irq = platform_get_irq(pdev, 0);
    if (led->irq < 0)
        return led->irq;

    if (devm_request_irq(&pdev->dev, led->irq, mtk_led_irq,
                         0, "mtk_led_irq", led))
        return -EINVAL;

    regmap_write(led->regmap, LED_INT_ENABLE, LED_INT_BIT);

    platform_set_drvdata(pdev, led);
    device_create_file(&pdev->dev, &dev_attr_toggle);

    pm_runtime_enable(&pdev->dev);

    dev_info(&pdev->dev, "MTK-style LED driver loaded\n");
    return 0;
}

static int mtk_led_remove(struct platform_device *pdev)
{
    struct mtk_led *led = platform_get_drvdata(pdev);

    device_remove_file(&pdev->dev, &dev_attr_toggle);
    clk_disable_unprepare(led->clk);
    pm_runtime_disable(&pdev->dev);

    return 0;
}

/* Power management */

static int mtk_led_suspend(struct device *dev)
{
    struct mtk_led *led = dev_get_drvdata(dev);
    clk_disable_unprepare(led->clk);
    return 0;
}

static int mtk_led_resume(struct device *dev)
{
    struct mtk_led *led = dev_get_drvdata(dev);
    return clk_prepare_enable(led->clk);
}

static const struct dev_pm_ops mtk_led_pm_ops = {
    .suspend = mtk_led_suspend,
    .resume  = mtk_led_resume,
};

static const struct of_device_id mtk_led_of_match[] = {
    { .compatible = "mediatek,my-led" },
    {},
};
MODULE_DEVICE_TABLE(of, mtk_led_of_match);

static struct platform_driver mtk_led_driver = {
    .probe  = mtk_led_probe,
    .remove = mtk_led_remove,
    .driver = {
        .name           = "mtk_led",
        .of_match_table = mtk_led_of_match,
        .pm             = &mtk_led_pm_ops,
    },
};

module_platform_driver(mtk_led_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Krishna");
MODULE_DESCRIPTION("Register-Level Platform Driver");