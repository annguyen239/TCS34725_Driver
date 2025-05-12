#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>

#define DRIVER_NAME             "tcs34725_driver"

#define TCS34725_ADDR           0x29
#define TCS34725_ENABLE_REG     0x00
#define TCS34725_ID_REG         0x12
#define TCS34725_ID_VALUE       0x44
#define TCS34725_ENABLE_PON     0x01
#define TCS34725_ENABLE_AEN     0x02

#define TCS34725_CDATAL         0x14
#define TCS34725_RDATAL         0x16
#define TCS34725_GDATAL         0x18
#define TCS34725_BDATAL         0x1A
#define TCS34725_COMMAND_BIT    0x80

/* Driver state (currently unused beyond holding the client) */
struct tcs34725_data {
    struct i2c_client *client;
};

/* Container for one RGBC sample */
struct tcs34725_color {
    u16 clear;
    u16 red;
    u16 green;
    u16 blue;
};

/* Forward declarations */
static int tcs34725_enable(struct i2c_client *client);
static int tcs34725_read_data(struct i2c_client *client,
                              struct tcs34725_color *color);

/**
 * tcs34725_read_word() - read a 16-bit little-endian word from reg/ reg+1
 */
static int tcs34725_read_word(struct i2c_client *client, u8 reg)
{
    int low, high;

    low = i2c_smbus_read_byte_data(client,
               TCS34725_COMMAND_BIT | reg);
    if (low < 0)
        return low;

    high = i2c_smbus_read_byte_data(client,
                TCS34725_COMMAND_BIT | (reg + 1));
    if (high < 0)
        return high;

    return (high << 8) | low;
}

/**
 * tcs34725_enable() - power on the device and enable its ADC
 */
static int tcs34725_enable(struct i2c_client *client)
{
    int ret;

    /* Power-on (PON) */
    ret = i2c_smbus_write_byte_data(client,
               TCS34725_COMMAND_BIT | TCS34725_ENABLE_REG,
               TCS34725_ENABLE_PON);
    if (ret < 0)
        return ret;
    msleep(10);

    /* Power-on + ADC enable (AEN) */
    ret = i2c_smbus_write_byte_data(client,
               TCS34725_COMMAND_BIT | TCS34725_ENABLE_REG,
               TCS34725_ENABLE_PON | TCS34725_ENABLE_AEN);
    if (ret < 0)
        return ret;
    msleep(700);  /* wait for ADC to stabilize */

    return 0;
}

/**
 * tcs34725_read_data() - read all four 16-bit RGBC registers
 */
static int tcs34725_read_data(struct i2c_client *client,
                              struct tcs34725_color *color)
{
    int ret;

    ret = tcs34725_read_word(client, TCS34725_CDATAL);
    if (ret < 0) return ret;
    color->clear = ret;

    ret = tcs34725_read_word(client, TCS34725_RDATAL);
    if (ret < 0) return ret;
    color->red = ret;

    ret = tcs34725_read_word(client, TCS34725_GDATAL);
    if (ret < 0) return ret;
    color->green = ret;

    ret = tcs34725_read_word(client, TCS34725_BDATAL);
    if (ret < 0) return ret;
    color->blue = ret;

    return 0;
}

/**
 * tcs34725_probe() - bind callback: runs when the client is instantiated
 */
static int tcs34725_probe(struct i2c_client *client)
{
    struct tcs34725_color color;
    int ret, id;

    dev_info(&client->dev, "Probing TCS34725 on bus %d addr 0x%02x\n",
             client->adapter->nr, client->addr);

    /* Verify chip ID */
    id = i2c_smbus_read_byte_data(client,
         TCS34725_COMMAND_BIT | TCS34725_ID_REG);
    if (id < 0) {
        dev_err(&client->dev, "ID read failed: %d\n", id);
        return id;
    }
    /* accept 0x44 (TCS34721/25) or 0x4D (TCS34723/27) */
    if (id != 0x44 && id != 0x4D) {
        dev_err(&client->dev, "Unknown ID 0x%02x\n", id);
        return -ENODEV;
    }

    /* Power up + enable ADC */
    ret = tcs34725_enable(client);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to enable sensor: %d\n", ret);
        return ret;
    }

    /* Read and print one RGBC sample */
    ret = tcs34725_read_data(client, &color);
    if (ret < 0) {
        dev_err(&client->dev, "Read error: %d\n", ret);
        return ret;
    }
    dev_info(&client->dev,
             "TCS34725 Color Data — C:%u  R:%u  G:%u  B:%u\n",
             color.clear, color.red,
             color.green, color.blue);

    return 0;
}

/**
 * tcs34725_remove() - unbind callback: runs when the client is removed
 */
static void tcs34725_remove(struct i2c_client *client)
{
    /* Power down */
    i2c_smbus_write_byte_data(client,
        TCS34725_COMMAND_BIT | TCS34725_ENABLE_REG, 0x00);
    dev_info(&client->dev, "TCS34725 removed\n");
}

/* I2C ID table for legacy (new_device/sysfs) binding */
static const struct i2c_device_id tcs34725_id[] = {
    { "tcs34725", 0 },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(i2c, tcs34725_id);

/* I2C driver structure */
static struct i2c_driver tcs34725_driver = {
    .driver = {
        .name  = DRIVER_NAME,
        .owner = THIS_MODULE,
    },
    .probe    = tcs34725_probe,
    .remove   = tcs34725_remove,
    .id_table = tcs34725_id,
};

module_i2c_driver(tcs34725_driver);

MODULE_AUTHOR("NHOM_7");
MODULE_DESCRIPTION("TCS34725 Color Sensor I2C Driver");
MODULE_LICENSE("GPL");
