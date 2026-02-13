#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/timer.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>

#define DEVICE_NAME "led_driver"
#define CLASS_NAME "led"

static int led_gpio = -1;
static int major;
static struct class *led_class = NULL;
static struct device *led_device = NULL;

static struct timer_list blink_timer;
static bool led_on = false;
static bool led_enabled = false;
static unsigned int blink_interval = 0;

static void blink_timer_callback(struct timer_list *t)
{
    led_on = !led_on;
    gpio_set_value(led_gpio, led_on);

    if (blink_interval > 0) {
        mod_timer(&blink_timer, jiffies + msecs_to_jiffies(blink_interval * 1000));
    }
}

static ssize_t led_write(struct file *file, const char __user *buf, size_t len, loff_t *off)
{
    char kbuf[32];
    unsigned int val_on, val_interval;

    if (len > sizeof(kbuf) - 1)
        return -EINVAL;

    if (copy_from_user(kbuf, buf, len))
        return -EFAULT;

    kbuf[len] = '\0';

    if (sscanf(kbuf, "%u;%u", &val_on, &val_interval) != 2)
        return -EINVAL;

    if (val_on) {
        led_enabled = true;
        blink_interval = val_interval;

        if (blink_interval > 0) {
            led_on = true;
            gpio_set_value(led_gpio, 1);
            mod_timer(&blink_timer, jiffies + msecs_to_jiffies(blink_interval * 1000));
        } else {
            led_on = true;
            gpio_set_value(led_gpio, 1);
            del_timer_sync(&blink_timer);
        }
    } else {
        led_enabled = false;
        led_on = false;
        gpio_set_value(led_gpio, 0);
        blink_interval = 0;
        del_timer_sync(&blink_timer);
    }

    return len;
}

static ssize_t led_read(struct file *file, char __user *buf, size_t len, loff_t *off)
{
    char kbuf[64];
    int ret;

    if (*off > 0)
        return 0;

    ret = snprintf(kbuf, sizeof(kbuf),
                   "State: %s\nInterval: %u\n",
                   led_enabled ? (blink_interval > 0 ? "blinking" : "on") : "off",
                   blink_interval);

    if (copy_to_user(buf, kbuf, ret))
        return -EFAULT;

    *off = ret;
    return ret;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = led_read,
    .write = led_write,
};

static int __init led_init(void)
{
    struct device_node *np;

    pr_info("LED: Initializing...\n");

    np = of_find_compatible_node(NULL, NULL, "custom,led");
    if (!np) {
        pr_err("LED: Device tree node not found\n");
        return -ENODEV;
    }

    led_gpio = of_get_named_gpio(np, "gpios", 0);
    if (!gpio_is_valid(led_gpio)) {
        pr_err("LED: Invalid GPIO number from device tree\n");
        return -EINVAL;
    }

    if (gpio_request(led_gpio, "led_gpio")) {
        pr_err("LED: Failed to request GPIO %d\n", led_gpio);
        return -EBUSY;
    }

    gpio_direction_output(led_gpio, 0);
    gpio_set_value(led_gpio, 0);

    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) {
        pr_err("LED: Failed to register character device\n");
        gpio_free(led_gpio);
        return major;
    }

    led_class = class_create(CLASS_NAME);
    if (IS_ERR(led_class)) {
        unregister_chrdev(major, DEVICE_NAME);
        gpio_free(led_gpio);
        return PTR_ERR(led_class);
    }

    led_device = device_create(led_class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    if (IS_ERR(led_device)) {
        class_destroy(led_class);
        unregister_chrdev(major, DEVICE_NAME);
        gpio_free(led_gpio);
        return PTR_ERR(led_device);
    }

    timer_setup(&blink_timer, blink_timer_callback, 0);

    pr_info("LED: Driver loaded. Use /dev/%s to control.\n", DEVICE_NAME);
    return 0;
}

static void __exit led_exit(void)
{
    del_timer_sync(&blink_timer);
    gpio_set_value(led_gpio, 0);
    gpio_free(led_gpio);

    device_destroy(led_class, MKDEV(major, 0));
    class_destroy(led_class);
    unregister_chrdev(major, DEVICE_NAME);

    pr_info("LED: Driver unloaded\n");
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tadas");
MODULE_DESCRIPTION("GPIO LED Driver with blinking support");
MODULE_VERSION("1.0");
