// I2C functions on AVR
//
// Copyright (C) 2018  Kevin O'Connor <kevin@koconnor.net>
//
// This file may be distributed under the terms of the GNU GPLv3 license.

#include <avr/io.h> // TWCR
#include "autoconf.h" // CONFIG_CLOCK_FREQ
#include "board/misc.h" // timer_is_before
#include "command.h" // shutdown
#include "gpio.h" // i2c_setup
#include "internal.h" // GPIO
#include "sched.h" // sched_shutdown

#if CONFIG_MACH_atmega168 || CONFIG_MACH_atmega328 || CONFIG_MACH_atmega328p
static const uint8_t SCL = GPIO('C', 5), SDA = GPIO('C', 4);
#elif CONFIG_MACH_atmega644p || CONFIG_MACH_atmega1284p
static const uint8_t SCL = GPIO('C', 0), SDA = GPIO('C', 1);
#elif CONFIG_MACH_at90usb1286 || CONFIG_MACH_at90usb646 || CONFIG_MACH_atmega32u4 || CONFIG_MACH_atmega1280 || CONFIG_MACH_atmega2560
static const uint8_t SCL = GPIO('D', 0), SDA = GPIO('D', 1);
#endif

static void
i2c_init(void)
{
    if (TWCR & (1<<TWEN))
        // Already setup
        return;

    // Setup output pins and enable pullups
    gpio_out_setup(SDA, 1);
    gpio_out_setup(SCL, 1);

    // Set 100Khz frequency
    TWSR = 0;
    TWBR = ((CONFIG_CLOCK_FREQ / 100000) - 16) / 2;

    // Enable interface
    TWCR = (1<<TWEN);
}

struct i2c_config
i2c_setup(uint32_t bus, uint32_t rate, uint8_t addr)
{
    if (bus)
        shutdown("Unsupported i2c bus");
    i2c_init();
    return (struct i2c_config){ .addr=addr<<1 };
}

static void
i2c_wait(uint32_t timeout)
{
    for (;;) {
        if (TWCR & (1<<TWINT))
            break;
        if (!timer_is_before(timer_read_time(), timeout))
            shutdown("i2c timeout");
    }
}

static void
i2c_start(uint32_t timeout)
{
    TWCR = (1<<TWEN) | (1<<TWINT) | (1<<TWSTA);
    i2c_wait(timeout);
    uint32_t status = TWSR;
    if (status != 0x10 && status != 0x08)
        shutdown("Failed to send i2c start");
}

static void
i2c_send_byte(uint8_t b, uint32_t timeout)
{
    TWDR = b;
    TWCR = (1<<TWEN) | (1<<TWINT);
    i2c_wait(timeout);
}

static void
i2c_stop(uint32_t timeout)
{
    TWCR = (1<<TWEN) | (1<<TWINT) | (1<<TWSTO);
}

void
i2c_write(struct i2c_config config, uint8_t write_len, uint8_t *write)
{
    uint32_t timeout = timer_read_time() + timer_from_us(5000);

    i2c_start(timeout);
    i2c_send_byte(config.addr, timeout);
    while (write_len--)
        i2c_send_byte(*write++, timeout);
    i2c_stop(timeout);
}

void
i2c_read(struct i2c_config config, uint8_t reg_len, uint8_t *reg
         , uint8_t read_len, uint8_t *read)
{
    shutdown("i2c_read not supported on avr");
}