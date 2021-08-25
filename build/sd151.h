/*
 * sd151.h - Part of OPEN-EYES PI-POW product, Linux kernel modules for hardware
 * monitoring
 *
 * Author:
 * Massimiliano Negretti <massimiliano.negretti@open-eyes.it> 2021-07-4
 *
 * Include file of sd151-hwmon Linux driver
 *
 * This file is part of sd151-hwmon distribution
 * https://github.com/openeyes-lab/sd151-hwmon
 *
 * Copyright (c) 2021 OPEN-EYES Srl
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _SD151_H
#define _SD151_H

#include <linux/regmap.h>
#include <linux/rtc.h>
#include <linux/time64.h>
#include <linux/watchdog.h>
#include <linux/i2c.h>
#include <asm/gpio.h>

struct device;

#define NUM_CH_VIN                      3
#define NBUTTON                         2

#define IRQ_GPIO                        23

struct sd151_input {
  struct input_dev     *button_dev;
  u16                  button;
  u16                  power;
};

struct sd151_private {
	struct device                 *dev;
  struct i2c_client             *client;
  struct regmap                 *regmap;
  struct watchdog_device        wdd;
  struct rtc_device             *rtc;
  struct work_struct            irq_work;
  struct sd151_input            inp;
	struct proc_dir_entry         *proc_entry;
  bool                          overlay_wdog_nowayout;
  int                           overlay_wdog_timeout;
  int                           overlay_wdog_wait;
  int                           device_wdog_timeout;
  int                           device_wdog_wait;
  int                           wdog_wait;
  struct mutex                  update_lock;
  u16                           firmware_version;
  bool                          alarm_enabled;
  bool                          alarm_pending;
  /* Voltage registers */
  bool                          volt_valid[NUM_CH_VIN];
  u16                           volt[NUM_CH_VIN];
  unsigned long                 volt_updated[NUM_CH_VIN];
  unsigned int                  irq;
  /* Voltage max registers */
  bool volt_max_valid[NUM_CH_VIN];
  u16 volt_max[NUM_CH_VIN];
  unsigned long volt_max_updated[NUM_CH_VIN];
  /* Voltage min registers */
  bool volt_min_valid[NUM_CH_VIN];
  u16 volt_min[NUM_CH_VIN];
  unsigned long volt_min_updated[NUM_CH_VIN];
};

#define SD151_NUM_REGS                  32

#define SD151_CHIP_ID_REG               0x00
#define SD151_CHIP_ID                   0xd151
#define SD151_CHIP_VER_REG              0x01

#define SD151_STATUS                    0x02
#define SD151_STATUS_POWERUP            0x0001
#define SD151_STATUS_POWEROFF           0x0002
#define SD151_STATUS_REBOOT             0x0003
#define SD151_STATUS_HALT               0x0004
#define SD151_STATUS_WAKEUP             0x0005
#define SD151_STATUS_BOOT_MASK          0x0007
#define SD151_STATUS_WDOG_EN            0x0008
#define SD151_STATUS_WAKEUP_EN          0x0010
#define SD151_STATUS_IRQ_BUTTONS        0x0100

#define SD151_COMMAND                   0x04
#define SD151_WDOG_ENABLE               0x01
#define SD151_WDOG_DISABLE              0x02
#define SD151_EXEC_POWEROFF             0x03
#define SD151_EXEC_REBOOT               0x04
#define SD151_EXEC_HALT                 0x05
#define SD151_PWOFFWAKEUP               0x06
#define SD151_IRQ_ACKNOWLEDGE           0x07
#define SD151_BUZZER_LOW                0x0A
#define SD151_BUZZER_HIGH               0x0B

#define SD151_WDOG_REFRESH              0x05
#define SD151_WDOG_REFRESH_MAGIC_VALUE  0x0d1e

#define SD151_WDOG_TIMEOUT              0x06
#define SD151_WDOG_TIMEOUT_MASK         0x00FF
#define SD151_WDOG_TIMEOUT_POS          0
#define SD151_WDOG_WAIT_MASK            0xFF00
#define SD151_WDOG_WAIT_POS             8

#define SD151_VOLTAGE_5V_BOARD          0x0A
#define SD151_VOLTAGE_5V_BOARD_MIN      0x0B
#define SD151_VOLTAGE_5V_BOARD_MAX      0x0C
#define SD151_VOLTAGE_5V_RPI            0x0D
#define SD151_VOLTAGE_3V3_RPI           0x10

#define SD151_BUTTONS                   0x14

#define SD151_RTC0                      0x1A
#define SD151_RTC1                      0x1B
#define SD151_RTC2                      0x1C
#define SD151_WAKEUP0                   0x1D
#define SD151_WAKEUP1                   0x1E
#define SD151_WAKEUP2                   0x1F

#define SD151_MIN_WDOG_WAIT             45

extern int sd151_proc_init(struct sd151_private *);
extern int sd151_proc_remove(struct sd151_private *);

#endif /* _SD151_H */
