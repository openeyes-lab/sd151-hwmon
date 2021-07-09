/*
 * sd151.c - Part of OPEN-EYES PI-POW HAT product, Linux kernel modules
 * for hardware monitoring
 * This driver handles the SD151 FW running on ATTINY817.
 * Author:
 * Massimiliano Negretti <massimiliano.negretti@open-eyes.it> 2021-07-4
 *
 * This driver handles 3 different features:
 * 1) HWMON
 * 2) WATCH DOG
 * 3) REAL TIME CLOCK
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

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/hwmon.h>
#include <linux/jiffies.h>
#include <linux/reboot.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/sched/signal.h>
#include <asm/irq.h>
#include <asm/gpio.h>

#include "sd151.h"

#define DRV_NAME	"sd151"

static struct sd151_private *pdata;

const struct regmap_config sd151_regmap_config = {
	.max_register = SD151_NUM_REGS - 1,
};

static const struct i2c_device_id sd151_id[] = {
	{ DRV_NAME, 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, sd151_id);

/*****************************************************************************
 * INPUT DEVICE
 *************************************************************************** */

static irqreturn_t sd151_irq(int irq, void *data)
{
	struct sd151_private *priv = data;
	schedule_work(&priv->irq_work);
	printk(KERN_INFO "interrupt");
  return IRQ_HANDLED;
}

static void sd151_irq_work_handler(struct work_struct *work)
{
	struct sd151_private *priv = container_of(work, struct sd151_private, irq_work);
	struct device *dev = &priv->client->dev;
	int ret;
	int val;
	int i,curr,prev,pwr;
//	char *shutdown_argv[] = { "/sbin/shutdown", "-h", "-P", "now", NULL };

	printk(KERN_INFO "interrupt on work");

	/* get the chip status */
	ret = regmap_read(priv->regmap, SD151_STATUS, &val);
	if (ret < 0) {
		dev_err(dev, "failed to read I2C chip status\n");
		return;
	}

	/* clear irq */
	ret = regmap_write(priv->regmap, SD151_COMMAND,	SD151_IRQ_ACKNOWLEDGE);
	if (ret < 0) {
		dev_err(dev, "failed to write I2C command\n");
	}

	if (val&SD151_STATUS_IRQ_BUTTONS) {
		ret = regmap_read(priv->regmap, SD151_BUTTONS, &val);
		if (ret < 0) {
			dev_err(dev, "failed to read I2C buttons\n");
			return;
		}

		curr = val;
		prev =  priv->inp.button;
		priv->inp.button = val;
		pwr = priv->inp.power;

		printk(KERN_INFO "input btn=%x prev=%x pwr=%x",curr,prev,pwr);

		for (i=0; i<NBUTTON; i++) {
			if ((curr&1)!=(prev&1)) {
				if (pwr&1) {
					//if(curr&1){
					//	set_current_state(TASK_INTERRUPTIBLE);
					//	call_usermodehelper(shutdown_argv[0], shutdown_argv, NULL, UMH_NO_WAIT);
					//}
					//kernel_power_off();
					//kill_cad_pid(SIGRTMIN + 4, 1);
					printk(KERN_INFO "send power signal");
					input_report_key(priv->inp.button_dev, KEY_POWER, curr&1);
					//kernel_power_off()
				} else {
					input_report_key(priv->inp.button_dev, BTN_0+i, curr&1);
					printk(KERN_INFO "send button");
				}
			}
			curr=curr>>1;
			prev=prev>>1;
			pwr=pwr>>1;
		}
		printk(KERN_INFO "input sync");
		input_sync(priv->inp.button_dev);
	}

}
/**
 * @brief HWMON function sd151 get voltage
 * @param [in] dev struct device pointer
 * @param [in] ch channel
 * @return voltage in millivolt
 * @details Returns the voltage of specific channel, from the given register,
 * in millivolts.
 */
static int sd151_get_voltage(struct device *dev, u8 ch)
{
	struct sd151_private *data = dev_get_drvdata(dev);
	int voltage=0;
	int ret;
	int reg;

	mutex_lock(&data->update_lock);

	if (ch<NUM_CH_VIN) {
		if (time_after(jiffies, data->volt_updated[ch] + HZ)
																						|| !data->volt_valid[ch]) {
			reg = SD151_VOLTAGE_5V_BOARD + ch*3;
			/* Get value */
			ret = regmap_read(data->regmap, reg, &voltage);
			if (ret < 0) {
				dev_err(dev, "failed to read I2C when get voltage\n");
				goto close;
			}
			data->volt[ch]=voltage;
			data->volt_updated[ch] = jiffies;
			data->volt_valid[ch] = true;
		} else {
			voltage = data->volt[ch];
		}

		mutex_unlock(&data->update_lock);
	}

close:
	mutex_unlock(&data->update_lock);
	return voltage;
}

/**
 * @brief HWMON function sd151 get voltage MAX
 * @param [in] dev struct device pointer
 * @param [in] ch channel
 * @return voltage in millivolt
 * @details Returns the maximum voltage of specific channel, from the given
 * register, in millivolts.
 */
static int sd151_get_voltage_max(struct device *dev, u8 ch)
{
	struct sd151_private *data = dev_get_drvdata(dev);
	int voltage=0;
	int ret;
	int reg;

	mutex_lock(&data->update_lock);

	if (ch<NUM_CH_VIN) {
		if (time_after(jiffies, data->volt_max_updated[ch] + HZ)
																		|| !data->volt_max_valid[ch]) {
			reg = SD151_VOLTAGE_5V_BOARD_MAX + ch*3;
			/* Get value */
			ret = regmap_read(data->regmap, reg, &voltage);
			if (ret < 0) {
				dev_err(dev, "failed to read I2C when get MAX voltage\n");
				goto close;
			}

			data->volt_max[ch]=voltage;
			data->volt_max_updated[ch] = jiffies;
			data->volt_max_valid[ch] = true;
		} else {
			voltage = data->volt_max[ch];
		}

	}

close:
	mutex_unlock(&data->update_lock);
	return voltage;
}

/**
 * @brief HWMON function sd151 get voltage MIN
 * @param [in] dev struct device pointer
 * @param [in] ch channel
 * @return voltage in millivolt
 * @details Returns the minimum voltage of specific channel, from the given
 * register, in millivolts.
 */
static int sd151_get_voltage_min(struct device *dev, u8 ch)
{
	struct sd151_private *data = dev_get_drvdata(dev);
	int voltage=0;
	int ret;
	int reg;

	mutex_lock(&data->update_lock);

	if (ch<NUM_CH_VIN) {
		if (time_after(jiffies, data->volt_min_updated[ch] + HZ)
																			|| !data->volt_min_valid[ch]) {
			reg = SD151_VOLTAGE_5V_BOARD_MIN + ch*3;
			/* Get value */
			ret = regmap_read(data->regmap, reg, &voltage);
			if (ret < 0) {
				dev_err(dev, "failed to read I2C when get MIN voltage\n");
				goto close;
			}

			data->volt_min[ch]=voltage;
			data->volt_min_updated[ch] = jiffies;
			data->volt_min_valid[ch] = true;
		} else {
			voltage = data->volt_min[ch];
		}

	}

close:
	mutex_unlock(&data->update_lock);
	return voltage;
}

/**
 * @brief HWMON function input read method
 * @param [in] dev struct device pointer
 * @param [in] attr attribute
 * @param [in] channel
 * @param [out] val pointer
 * @return 9 if success.
 * @details Calls the right handler
 */
static int sd151_read_in(struct device *dev, u32 attr, int channel, long *val)
{
	switch (attr) {
		case hwmon_in_input:
			if (channel < NUM_CH_VIN)
				*val = sd151_get_voltage(dev,channel);
			else
				return -EOPNOTSUPP;
			return 0;
		case hwmon_in_max:
				if (channel < NUM_CH_VIN)
					*val = sd151_get_voltage_max(dev,channel);
				else
					return -EOPNOTSUPP;
				return 0;
		case hwmon_in_min:
				if (channel < NUM_CH_VIN)
					*val = sd151_get_voltage_min(dev,channel);
				else
					return -EOPNOTSUPP;
				return 0;
		default:
			return -EOPNOTSUPP;
	}
}

/**
 * @brief HWMON function read method
 * @param [in] dev struct device pointer
 * @param [in] type enum hwmon_sensor_types
 * @param [in] attr attribute
 * @param [in] channel
 * @param [out] val pointer
 * @return 0 if success.
 * @details Calls the right handler
 */
static int sd151_read(struct device *dev, enum hwmon_sensor_types type,
			u32 attr, int channel, long *val)
{
	switch (type) {
		case hwmon_in:
			return sd151_read_in(dev, attr, channel, val);
		default:
			return -EOPNOTSUPP;
	}
}

/**
 * @brief HWMON function return channel name
 * @param [in] dev struct device pointer
 * @param [in] type enum hwmon_sensor_types
 * @param [in] attr attribute
 * @param [in] channel
 * @param [out] str string pointer
 * @return 0 if success.
 * @details Returns the label of channel.
 */
static int sd151_read_string(struct device *dev, enum hwmon_sensor_types type,
		       u32 attr, int channel, const char **str)
{
	switch (type) {
		case hwmon_in:
			switch (attr) {
				case hwmon_in_label:
					switch (channel) {
						case 0:
							*str = "BOARD 5V";
							return 0;
						case 1:
							*str = "RPI 5V";
							return 0;
						case 2:
							*str = "RPI 3V3";
							return 0;
						default:
							*str = NULL;
							break;
					}
					return -EOPNOTSUPP;
				default:
					return -EOPNOTSUPP;
			}
		default:
			return -EOPNOTSUPP;
	}
	return -EOPNOTSUPP;
}

/**
 * @brief HWMON function return access attribute
 * @param [in] data unused
 * @param [in] type enum hwmon_sensor_types
 * @param [in] attr attribute
 * @param [in] channel
 * @return permission.
 * @details Returns file access permission
 */
static umode_t sd151_is_visible(const void *data, enum hwmon_sensor_types type,
			       u32 attr, int channel)
{
	switch (type) {
		case hwmon_in:
			switch (attr) {
				case hwmon_in_input:
					return S_IRUGO;
				case hwmon_in_label:
					return S_IRUGO;
 				case hwmon_in_max:
					return S_IRUGO;
				case hwmon_in_min:
					return S_IRUGO;
				default:
					break;
			}
			break;
		default:
			break;
	}
	return 0;
}

/****************************************************************************
 * HWMON STRUCTURES
 ****************************************************************************/
static const u32 sd151_in_config[] = {
	(HWMON_I_INPUT|HWMON_I_LABEL|HWMON_I_MAX|HWMON_I_MIN),
	(HWMON_I_INPUT|HWMON_I_LABEL|HWMON_I_MAX|HWMON_I_MIN),
	(HWMON_I_INPUT|HWMON_I_LABEL|HWMON_I_MAX|HWMON_I_MIN),
	0
};

static const struct hwmon_channel_info sd151_voltage = {
	.type = hwmon_in,
	.config = sd151_in_config,
};

static const struct hwmon_channel_info *sd151_info[] = {
	&sd151_voltage,
	NULL
};

static const struct hwmon_ops sd151_hwmon_ops = {
	.is_visible = sd151_is_visible,
	.read = sd151_read,
	.read_string = sd151_read_string,
};

static const struct hwmon_chip_info sd151_chip_info = {
	.ops = &sd151_hwmon_ops,
	.info = sd151_info,
};

/****************************************************************************
 * WATCHDOG OPS
 ****************************************************************************/

static int sd151_wdt_ping(struct watchdog_device *wdd)
{
	struct sd151_private *data = watchdog_get_drvdata(wdd);
	int ret = regmap_write(data->regmap, SD151_WDOG_REFRESH,
		SD151_WDOG_REFRESH_MAGIC_VALUE);

	return ret;
}

static int sd151_wdt_start(struct watchdog_device *wdd)
{
	struct sd151_private *data = watchdog_get_drvdata(wdd);
	int ret = regmap_write(data->regmap, SD151_COMMAND,	SD151_WDOG_ENABLE);

	return ret;
}

static int sd151_wdt_stop(struct watchdog_device *wdd)
{
	struct sd151_private *data = watchdog_get_drvdata(wdd);
	int	ret = regmap_write(data->regmap, SD151_COMMAND,	SD151_WDOG_DISABLE);

	return ret;
}

static int sd151_wdt_settimeout(struct watchdog_device *wdd, unsigned int to)
{
	struct sd151_private *data = watchdog_get_drvdata(wdd);
	int ret;
	unsigned int reg;

	if ((watchdog_timeout_invalid(wdd, to))||(to>255)) {
		return -EINVAL;
	}

	/* build up register value */
	reg = ((data->wdog_wait/5)<<SD151_WDOG_WAIT_POS)&SD151_WDOG_WAIT_MASK;
	reg |= (to&SD151_WDOG_TIMEOUT_MASK)<<SD151_WDOG_TIMEOUT_POS;

	ret = regmap_write(data->regmap, SD151_WDOG_TIMEOUT,reg);

	wdd->timeout = to;

	return 0;
}

/****************************************************************************
 * WATCHDOG STRUCTURES
 ****************************************************************************/
static const struct watchdog_ops sd151_wdt_ops = {
	.owner = THIS_MODULE,
	.start = sd151_wdt_start,
	.stop = sd151_wdt_stop,
	.ping = sd151_wdt_ping,
	.set_timeout	= sd151_wdt_settimeout,
};

static struct watchdog_info sd151_wdt_info = {
	.options = WDIOF_KEEPALIVEPING | WDIOF_MAGICCLOSE | WDIOF_SETTIMEOUT,
	.identity = "OPEN-EYES sd151 Watchdog",
};

/****************************************************************************
 * WATCHDOG INITIALIZATION
 ****************************************************************************/
static int sd151_wdog_init(struct device *dev)
{
	struct sd151_private *data = dev_get_drvdata(dev);
	int ret;
	int tinfo;
	bool update_device=false;

	watchdog_set_drvdata(&data->wdd, data);

	/* get timeout info from device */
	ret = regmap_read(data->regmap, SD151_WDOG_TIMEOUT, &tinfo);
	if (ret < 0) {
		dev_err(dev, "failed to read I2C when init watchdog\n");
		return ret;
	}

	data->device_wdog_timeout = (tinfo & SD151_WDOG_TIMEOUT_MASK)>>
																										SD151_WDOG_TIMEOUT_POS;
	data->device_wdog_wait = ((tinfo & SD151_WDOG_WAIT_MASK)>>
																										SD151_WDOG_WAIT_POS)*5;

	if (data->overlay_wdog_timeout==-1) {
		/* If not defined in overlay get timeout value from device */
		data->wdd.timeout = data->device_wdog_timeout;
	} else {
		/* else overlay have priority */
		data->wdd.timeout = data->overlay_wdog_timeout;
		update_device = true;
	}

	if (data->overlay_wdog_wait<SD151_MIN_WDOG_WAIT ) {
		/* If not defined in overlay get timeout value from device */
		data->wdog_wait = data->device_wdog_wait;
	} else {
		/* else overlay have priority */
		data->wdog_wait = data->overlay_wdog_wait;
		update_device = true;
	}

	sd151_wdt_info.firmware_version = data->firmware_version;
	data->wdd.parent = dev;

	data->wdd.info = &sd151_wdt_info;
	data->wdd.ops = &sd151_wdt_ops;

	watchdog_set_nowayout(&data->wdd, data->overlay_wdog_nowayout);

	if (update_device)
		sd151_wdt_settimeout(&data->wdd,data->wdd.timeout);

	ret = watchdog_register_device(&data->wdd);
	if (ret)
		return ret;

	dev_info(dev, "Watchdog registered!\n");

	return 0;
}

/****************************************************************************
 * RTC OPS
 ****************************************************************************/

static int sd151_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
 	struct sd151_private *data = dev_get_drvdata(dev);
 	time64_t new_time = rtc_tm_to_time64(tm);
	int ret;
	unsigned int tick;

	if (new_time > 0x0000ffffffffffff)
 		return -EINVAL;

 	/*
 	 * The value to write is divided in 3 words of 16 bits
 	 * and written into the sd151 firmware
 	 */

	tick = new_time & 0xffff;
	ret = regmap_write(data->regmap, SD151_RTC0,	tick);
	if (ret) {
		dev_err(dev, "Unable to write RTC word 0 when setting time\n");
		return ret;
	}

	tick = (new_time>>16) & 0xffff;
	ret = regmap_write(data->regmap, SD151_RTC1,	tick);
	if (ret) {
		dev_err(dev, "Unable to write RTC word 1 when setting time\n");
		return ret;
	}

	tick = (new_time>>32) & 0xffff;
	ret = regmap_write(data->regmap, SD151_RTC2,	tick);
	if (ret) {
		dev_err(dev, "Unable to write RTC word 2 when setting time\n");
		return ret;
	}

 	return 0;
 }

static int sd151_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	struct sd151_private *data = dev_get_drvdata(dev);
	time64_t new_time,snap_time;
 	int ret;
 	unsigned int tick;

	/*
	* The value to read is divided in 3 words of 16 bits
	* and read from the sd151 firmware
	*/

	ret = regmap_read(data->regmap, SD151_RTC0,	&tick);
	if (ret) {
		dev_err(dev, "Unable to read RTC word 0 when getting time\n");
		return ret;
	}
	snap_time = tick&0xffff;
	new_time = snap_time;

	ret = regmap_read(data->regmap, SD151_RTC1,	&tick);
	if (ret) {
		dev_err(dev, "Unable to read RTC word 1 when getting time\n");
		return ret;
	}
	snap_time = tick&0xffff;
	new_time = new_time | (snap_time<<16);

	ret = regmap_read(data->regmap, SD151_RTC2,	&tick);
	if (ret) {
		dev_err(dev, "Unable to read RTC word 2 when getting time\n");
		return ret;
	}
	snap_time = tick&0xffff;
	new_time = new_time | (snap_time<<32);

	rtc_time64_to_tm(new_time,tm);
	return 0;
}

static int sd151_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct sd151_private *data = dev_get_drvdata(dev);
  time64_t alarm_time = rtc_tm_to_time64(&alrm->time);
 	int ret;
 	unsigned int tick;

	data->alarm_enabled = alrm->enabled;
	data->alarm_pending = alrm->pending;

 	if (alarm_time > 0x0000ffffffffffff)
  		return -EINVAL;

	/*
	 * The value to write is divided in 3 words of 16 bits
	 * and written into the sd151 firmware
	 */

 	tick = alarm_time & 0xffff;
 	ret = regmap_write(data->regmap, SD151_WAKEUP0,	tick);
 	if (ret) {
 		dev_err(dev, "Unable to write WAKEUP word 0 when setting alarm\n");
 		return ret;
 	}

 	tick = (alarm_time>>16) & 0xffff;
 	ret = regmap_write(data->regmap, SD151_WAKEUP1,	tick);
 	if (ret) {
 		dev_err(dev, "Unable to write WAKEUP word 1 when setting alarm\n");
 		return ret;
 	}

 	tick = (alarm_time>>32) & 0xffff;
 	ret = regmap_write(data->regmap, SD151_WAKEUP2,	tick);
 	if (ret) {
 		dev_err(dev, "Unable to write WAKEUP word 2 when setting alarm\n");
 		return ret;
 	}

  return 0;

}

static int sd151_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct sd151_private *data = dev_get_drvdata(dev);
 	time64_t alarm_time,snap_time;
  int ret;
  unsigned int tick;

 	/*
 	* The value to read is divided in 3 words of 16 bits
 	* and read from the sd151 firmware
 	*/

 	ret = regmap_read(data->regmap, SD151_WAKEUP0,	&tick);
 	if (ret) {
 		dev_err(dev, "Unable to read RTC word 0 when getting alarm\n");
 		return ret;
 	}
 	snap_time = tick&0xffff;
 	alarm_time = snap_time;

 	ret = regmap_read(data->regmap, SD151_WAKEUP1,	&tick);
 	if (ret) {
 		dev_err(dev, "Unable to read RTC word 1 when getting alarm\n");
 		return ret;
 	}
 	snap_time = tick&0xffff;
 	alarm_time = alarm_time | (snap_time<<16);

 	ret = regmap_read(data->regmap, SD151_WAKEUP2,	&tick);
 	if (ret) {
 		dev_err(dev, "Unable to read RTC word 2 when getting alarm\n");
 		return ret;
 	}
 	snap_time = tick&0xffff;
 	alarm_time = alarm_time | (snap_time<<32);

 	rtc_time64_to_tm(alarm_time,&alrm->time);
	alrm->enabled = data->alarm_enabled;
	alrm->pending = data->alarm_pending;

 	return 0;
}

 /*
  * Interface to RTC framework
  */

static int sd151_alarm_irq_enable(struct device *dev, unsigned int enabled)
{
	struct sd151_private *data = dev_get_drvdata(dev);

	if (!enabled) {
		/** When IRQ disabled clear wakeup timer */
		regmap_write(data->regmap, SD151_WAKEUP0,	0);
		regmap_write(data->regmap, SD151_WAKEUP1,	0);
		regmap_write(data->regmap, SD151_WAKEUP2,	0);
	}
 	return 0;
}

/****************************************************************************
 * RTC STRUCTURES
 ****************************************************************************/

static const struct rtc_class_ops sd151_rtc_ops = {
	.set_time	        = sd151_rtc_set_time,
	.read_time	      = sd151_rtc_read_time,
	.read_alarm	      = sd151_rtc_read_alarm,
	.set_alarm	      = sd151_rtc_set_alarm,
	.alarm_irq_enable = sd151_alarm_irq_enable,
};

/****************************************************************************
 * RTC INITIALIZATION
 ****************************************************************************/
static int sd151_rtc_init(struct device *dev)
{
	struct sd151_private *data = dev_get_drvdata(dev);

	device_init_wakeup(dev, 1);

	data->rtc = devm_rtc_device_register(dev, data->client->name,
					 &sd151_rtc_ops, THIS_MODULE);

	if (IS_ERR(data->rtc)) {
		dev_err(dev, "failed register RTC\n");
		return PTR_ERR(data->rtc);
	}

	return 0;
}

/****************************************************************************
 * REBOOT / SHUTDOWN NOTIFY
 ****************************************************************************/

static int sd151_notify_reboot(struct notifier_block *this,
			unsigned long code, void *x)
{
	struct sd151_private *data = pdata;
	int ret=0;

	switch (code) {
		case SYS_POWER_OFF:
			ret = regmap_write(data->regmap, SD151_COMMAND,	SD151_EXEC_POWEROFF);
			break;
		case SYS_RESTART:
			ret = regmap_write(data->regmap, SD151_COMMAND,	SD151_EXEC_REBOOT);
			break;
		case SYS_HALT:
			ret = regmap_write(data->regmap, SD151_COMMAND,	SD151_EXEC_HALT);
			break;
	}
	if (ret)
		printk(KERN_INFO "Unable to write shutdown command");

	return NOTIFY_DONE;
}

static struct notifier_block sd151_notifier = {
	.notifier_call	= sd151_notify_reboot,
	.next		= NULL,
	.priority	= 0,
};

int try_input_device_registration(struct device *dev,struct sd151_private *data)
{
	int ret;

	data->inp.button_dev = input_allocate_device();
	if (!data->inp.button_dev) {
		dev_err(dev,"Cannot allocate input device.\n");
		free_irq(data->irq, sd151_irq);
		return -ENOMEM;
	}
	data->inp.button_dev->name = DRV_NAME;

	//data->inp.button_dev->evbit[0] = BIT_MASK(EV_KEY);
	//data->inp.button_dev->keybit[BIT_WORD(BTN_0)] = BIT_MASK(BTN_0);

	__set_bit(EV_KEY, data->inp.button_dev->evbit);
	__set_bit(KEY_POWER, data->inp.button_dev->keybit);
	__set_bit(BTN_0, data->inp.button_dev->keybit);
	__set_bit(BTN_1, data->inp.button_dev->keybit);

	ret = input_register_device(data->inp.button_dev);
	if (ret) {
		dev_err(dev,"button.c: Failed to register device\n");
		free_irq(data->irq, sd151_irq);
		input_free_device(data->inp.button_dev);
		return -ENOMEM;
	}

	data->inp.power = 0x0001;

	return 0;
}

/****************************************************************************
 * SD151 PROBE
 ****************************************************************************/
int sd151_probe(struct i2c_client *client, struct regmap *regmap)
{
	struct device *dev = &client->dev;
	struct sd151_private *data;
	struct device *hwmon_dev;
	unsigned int val;
	int ret;

	if (IS_ERR(regmap))
		return PTR_ERR(regmap);

	data = devm_kzalloc(dev, sizeof(struct sd151_private),GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	pdata = data;

	data->client = client;
	data->regmap = regmap;


	if (gpio_is_valid(IRQ_GPIO)) {
		if(gpio_request(IRQ_GPIO,"SD151_IRQ") < 0){
    	dev_err(dev,"ERROR: GPIO %d request\n", IRQ_GPIO);
    	return -EBUSY;
  	}
		gpio_direction_input(IRQ_GPIO);
		//if(gpio_set_debounce(IRQ_GPIO, 200) < 0){
		//	dev_err(dev,"ERROR: gpio_set_debounce - %d\n", IRQ_GPIO);
		//	return -EBUSY;
		//}
		data->irq = gpio_to_irq(IRQ_GPIO);
		if (request_irq(data->irq, sd151_irq, IRQF_TRIGGER_FALLING, DRV_NAME, data)) {
			dev_err(dev, "Can't allocate irq %d",data->irq);
			return -EBUSY;
		}

		INIT_WORK(&data->irq_work, sd151_irq_work_handler);

	} else {
		dev_err(dev, "Invalid GPIO %d",IRQ_GPIO);
		return -EBUSY;
	}



	dev_set_drvdata(dev, data);

	mutex_init(&data->update_lock);

	/* Verify that we have a sd151 */
	ret = regmap_read(regmap, SD151_CHIP_ID_REG, &val);
	if (ret < 0) {
		dev_err(dev, "failed to read I2C chip Id\n");
		goto error;
	}

	if (val!=SD151_CHIP_ID) {
		dev_err(dev, "Invalid chip id: %x\n", val);
		ret = -ENODEV;
		goto error;
	}

	/* Get version */
	ret = regmap_read(regmap, SD151_CHIP_VER_REG, &val);
	if (ret < 0) {
		dev_err(dev, "failed to read I2C firmware version\n");
		goto error;
	}


	data->firmware_version = val;

	/* Get status */
	ret = regmap_read(regmap, SD151_STATUS, &val);
	if (ret < 0) {
		dev_err(dev, "failed to access device when reading status\n");
		goto error;
	}
	switch (val&SD151_STATUS_BOOT_MASK) {
		case SD151_STATUS_POWERUP:
			dev_info(dev, "start from POWER-UP");
			break;
		case SD151_STATUS_POWEROFF:
			dev_info(dev, "start from POWER-OFF");
			break;
		case SD151_STATUS_REBOOT:
			dev_info(dev, "start from REBOOT");
			break;
		case SD151_STATUS_HALT:
			dev_info(dev, "start from HALT");
			break;
		case SD151_STATUS_WAKEUP:
			dev_info(dev, "start from WAKEUP");
			break;
		default:
			dev_err(dev, "start from unknown %x",val);
			break;
	}

	/* HWMON register */
	hwmon_dev = devm_hwmon_device_register_with_info(dev, client->name,
							 data, &sd151_chip_info, NULL);

	if (IS_ERR(hwmon_dev)) {
		ret = PTR_ERR(hwmon_dev);
		goto error;
	}

	dev_info(dev, "HWMON registered as %s\n",dev_name(hwmon_dev));

	if (device_property_read_bool(dev, "wdog_enabled")) {
		if (device_property_read_bool(dev, "wdog_nowayout"))
			data->overlay_wdog_nowayout = true;

		if (device_property_read_u32(dev, "wdog_timeout", &val))
			data->overlay_wdog_timeout = -1;
		else
			data->overlay_wdog_timeout = val;

		if (device_property_read_u32(dev, "wdog_wait", &val))
			data->overlay_wdog_wait = -1;
		else
			data->overlay_wdog_wait = val;

		ret = sd151_wdog_init(dev);
		if (ret)
			goto error;
	}

	if (device_property_read_bool(dev, "rtc_enabled")) {
		sd151_rtc_init(dev);
	}


	try_input_device_registration(dev,data);

	/*
	 * Register the tts_notifier to reboot notifier list so that the _TTS
	 * object can also be evaluated when the system enters S5.
	 */
	register_reboot_notifier(&sd151_notifier);

	return 0;

error:
	return ret;
}

static int sd151_i2c_probe(struct i2c_client *client,
			    const struct i2c_device_id *id)
{
	struct regmap_config config;

	config = sd151_regmap_config;
	config.val_bits = 16;
	config.reg_bits = 8;
	config.cache_type = REGCACHE_NONE;

	return sd151_probe(client,devm_regmap_init_i2c(client, &config));
}

static int sd151_remove(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct sd151_private *data = dev_get_drvdata(dev);

	watchdog_unregister_device(&data->wdd);
	free_irq(data->irq, sd151_irq);
	input_free_device(data->inp.button_dev);
	unregister_reboot_notifier(&sd151_notifier);
	return 0;
}

static struct i2c_driver sd151_i2c_driver = {
	.class		= I2C_CLASS_HWMON,
	.driver = {
		.name = DRV_NAME,
	},
	.probe    = sd151_i2c_probe,
	.remove	  = sd151_remove,
	.id_table = sd151_id,
};
module_i2c_driver(sd151_i2c_driver);

MODULE_DESCRIPTION("HWMON SD151 driver");
MODULE_AUTHOR("Massimiliano Negretti <massimiliano.negretti@open-eyes.it>");
MODULE_LICENSE("GPL");
