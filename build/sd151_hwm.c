/*
 * sd151_hwmon.c - Part of OPEN-EYES PI-POW HAT product, Linux kernel modules
 * for hardware monitoring
 * This driver handles the SD151 HWMON module.
 * Author:
 * Massimiliano Negretti <massimiliano.negretti@open-eyes.it> 2021-07-4
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
#include <linux/hwmon.h>

#include "sd151.h"

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

const struct hwmon_chip_info sd151_chip_info = {
	.ops = &sd151_hwmon_ops,
	.info = sd151_info,
};

EXPORT_SYMBOL_GPL(sd151_chip_info);
