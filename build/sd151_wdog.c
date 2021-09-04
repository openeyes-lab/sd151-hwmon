/*
 * sd151_wdog.c - Part of OPEN-EYES PI-POW HAT product, Linux kernel modules
 * for hardware monitoring
 * This driver handles the SD151 hardware watchdog module.
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

#include "sd151.h"


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

int sd151_wdog_init(struct sd151_private *data)
{
	//struct sd151_private *data = dev_get_drvdata(dev);
	int ret;
	int tinfo;
	bool update_device=false;

	watchdog_set_drvdata(&data->wdd, data);

	/* get timeout info from device */
	ret = regmap_read(data->regmap, SD151_WDOG_TIMEOUT, &tinfo);
	if (ret < 0) {
		dev_err(data->dev, "failed to read I2C when init watchdog\n");
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
	data->wdd.parent = data->dev;

	data->wdd.info = &sd151_wdt_info;
	data->wdd.ops = &sd151_wdt_ops;

	watchdog_set_nowayout(&data->wdd, data->overlay_wdog_nowayout);

	if (update_device)
		sd151_wdt_settimeout(&data->wdd,data->wdd.timeout);

	ret = watchdog_register_device(&data->wdd);
	if (ret)
		return ret;

	return 0;
}

EXPORT_SYMBOL_GPL(sd151_wdog_init);
