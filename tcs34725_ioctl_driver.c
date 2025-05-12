#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/delay.h>

#define DRIVER_NAME         "tcs34725_driver"
#define CLASS_NAME          "tcs34725"
#define DEVICE_NAME         "tcs34725"

/* I2C register and command definitions */
#define TCS34725_CMD_BIT      0x80
#define TCS34725_REG_ENABLE   0x00
#define TCS34725_PON          0x01
#define TCS34725_AEN          0x02
#define TCS34725_REG_ATIME    0x01
#define TCS34725_REG_CONTROL  0x0F
#define TCS34725_REG_CDATAL   0x14
#define TCS34725_REG_RDATAL   0x16
#define TCS34725_REG_GDATAL   0x18
#define TCS34725_REG_BDATAL   0x1A

/* IOCTL commands */
#define TCS34725_IOCTL_MAGIC     't'
#define TCS34725_IOCTL_READ_C    _IOR(TCS34725_IOCTL_MAGIC, 1, u16)
#define TCS34725_IOCTL_READ_R    _IOR(TCS34725_IOCTL_MAGIC, 2, u16)
#define TCS34725_IOCTL_READ_G    _IOR(TCS34725_IOCTL_MAGIC, 3, u16)
#define TCS34725_IOCTL_READ_B    _IOR(TCS34725_IOCTL_MAGIC, 4, u16)
#define TCS34725_IOCTL_SET_GAIN  _IOW(TCS34725_IOCTL_MAGIC, 5, u8)
#define TCS34725_IOCTL_SET_ATIME _IOW(TCS34725_IOCTL_MAGIC, 6, u8)

struct tcs34725drv {
    struct i2c_client *client;
    struct cdev cdev;
    dev_t devt;
    struct class *class;
    struct device *device;
};
static struct tcs34725drv *tcs34725dev;

/* Helper: read a 16-bit little-endian value from two registers */
static int tcs34725_read_word(struct i2c_client *c, u8 reg, u16 *val)
{
    int lo = i2c_smbus_read_byte_data(c, TCS34725_CMD_BIT | reg);
    int hi;
    if (lo < 0)
        return lo;
    hi = i2c_smbus_read_byte_data(c, TCS34725_CMD_BIT | (reg + 1));
    if (hi < 0)
        return hi;
    *val = (hi << 8) | lo;
    return 0;
}

/* Helper: write a single byte */
static int tcs34725_write_byte(struct i2c_client *c, u8 reg, u8 data)
{
    return i2c_smbus_write_byte_data(c, TCS34725_CMD_BIT | reg, data);
}

/* File operations: open and release */
static int tcs34725_open(struct inode *inode, struct file *file)
{
    file->private_data = tcs34725dev;
    return 0;
}
static int tcs34725_release(struct inode *inode, struct file *file)
{
    return 0;
}

/* IOCTL handler */
static long tcs34725_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct tcs34725drv *d = file->private_data;
    struct i2c_client *c = d->client;
    int ret = 0;
    u16 val16;
    u8  val8;

    switch (cmd) {
    case TCS34725_IOCTL_READ_C:
        ret = tcs34725_read_word(c, TCS34725_REG_CDATAL, &val16);
        if (!ret && copy_to_user((u16 __user *)arg, &val16, sizeof(val16)))
            ret = -EFAULT;
        break;
    case TCS34725_IOCTL_READ_R:
        ret = tcs34725_read_word(c, TCS34725_REG_RDATAL, &val16);
        if (!ret && copy_to_user((u16 __user *)arg, &val16, sizeof(val16)))
            ret = -EFAULT;
        break;
    case TCS34725_IOCTL_READ_G:
        ret = tcs34725_read_word(c, TCS34725_REG_GDATAL, &val16);
        if (!ret && copy_to_user((u16 __user *)arg, &val16, sizeof(val16)))
            ret = -EFAULT;
        break;
    case TCS34725_IOCTL_READ_B:
        ret = tcs34725_read_word(c, TCS34725_REG_BDATAL, &val16);
        if (!ret && copy_to_user((u16 __user *)arg, &val16, sizeof(val16)))
            ret = -EFAULT;
        break;
    case TCS34725_IOCTL_SET_GAIN:
        if (copy_from_user(&val8, (u8 __user *)arg, sizeof(val8)))
            return -EFAULT;
        ret = tcs34725_write_byte(c, TCS34725_REG_CONTROL, val8);
        break;
    case TCS34725_IOCTL_SET_ATIME:
        if (copy_from_user(&val8, (u8 __user *)arg, sizeof(val8)))
            return -EFAULT;
        ret = tcs34725_write_byte(c, TCS34725_REG_ATIME, val8);
        break;
    default:
        ret = -EINVAL;
        break;
    }

    return ret;
}

static const struct file_operations tcs34725_fops = {
    .owner          = THIS_MODULE,
    .open           = tcs34725_open,
    .release        = tcs34725_release,
    .unlocked_ioctl = tcs34725_ioctl,
};

/* I2C probe: register char device and initialize sensor */
static int tcs34725_probe(struct i2c_client *client)
{
    int ret;
    dev_t devt;
    struct device *dev;

    tcs34725dev = kzalloc(sizeof(*tcs34725dev), GFP_KERNEL);
    if (!tcs34725dev)
        return -ENOMEM;
    tcs34725dev->client = client;
    i2c_set_clientdata(client, tcs34725dev);

    /* allocate device number */
    ret = alloc_chrdev_region(&devt, 0, 1, DEVICE_NAME);
    if (ret)
        goto err_free;
    tcs34725dev->devt = devt;

    /* create class (name only) */
    tcs34725dev->class = class_create(CLASS_NAME);
    if (IS_ERR(tcs34725dev->class)) {
        ret = PTR_ERR(tcs34725dev->class);
        goto err_chrdev;
    }

    /* init and add cdev */
    cdev_init(&tcs34725dev->cdev, &tcs34725_fops);
    tcs34725dev->cdev.owner = THIS_MODULE;
    ret = cdev_add(&tcs34725dev->cdev, devt, 1);
    if (ret)
        goto err_class;

    /* create device node */
    dev = device_create(tcs34725dev->class, &client->dev, devt,
                        NULL, DEVICE_NAME);
    if (IS_ERR(dev)) {
        ret = PTR_ERR(dev);
        goto err_cdev;
    }
    tcs34725dev->device = dev;

    /* power on sensor */
    ret = tcs34725_write_byte(client, TCS34725_REG_ENABLE, TCS34725_PON);
    if (ret < 0)
        goto err_device;
    msleep(10);
    ret = tcs34725_write_byte(client, TCS34725_REG_ENABLE,
                              TCS34725_PON | TCS34725_AEN);
    if (ret < 0)
        goto err_device;
    msleep(200);

    dev_info(&client->dev, "TCS34725 initialized, major=%d\n", MAJOR(devt));
    return 0;

err_device:
    device_destroy(tcs34725dev->class, devt);
err_cdev:
    cdev_del(&tcs34725dev->cdev);
err_class:
    class_destroy(tcs34725dev->class);
err_chrdev:
    unregister_chrdev_region(devt, 1);
err_free:
    kfree(tcs34725dev);
    return ret;
}

static void tcs34725_remove(struct i2c_client *client)
{
    struct tcs34725drv *d = i2c_get_clientdata(client);
    dev_t devt = d->devt;

    /* power down */
    tcs34725_write_byte(client, TCS34725_REG_ENABLE, 0x00);

    device_destroy(d->class, devt);
    cdev_del(&d->cdev);
    class_destroy(d->class);
    unregister_chrdev_region(devt, 1);
    kfree(d);

    dev_info(&client->dev, "TCS34725 removed\n");
}

static const struct i2c_device_id tcs34725_id[] = {
    { "tcs34725", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, tcs34725_id);

static struct i2c_driver tcs34725_i2c_driver = {
    .driver = {
        .name  = DRIVER_NAME,
        .owner = THIS_MODULE,
    },
    .probe    = tcs34725_probe,
    .remove   = tcs34725_remove,
    .id_table = tcs34725_id,
};

module_i2c_driver(tcs34725_i2c_driver);

MODULE_AUTHOR("NHOM_7");
MODULE_DESCRIPTION("TCS34725 I2C Driver with IOCTL Interface");
MODULE_LICENSE("GPL");
