/*
 * sd151_gpio.c - Part of OPEN-EYES PI-POW HAT product, Linux kernel modules
 * for hardware monitoring
 * This driver handles the SD151 GPIO module.
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

 /**
 * GPIO CALLBACK
 */

 /**
  * @brief GPIO function sd151 get value
  * @param [in] chip struct gpio_chip pointer
  * @param [in] off channel
  * @return gpio status
  * @details Returns always 0 because only output is supported.
  */
static int sd151_gpio_get_value(struct gpio_chip *chip, unsigned off)
{
	//struct sd151_private *data = gpiochip_get_data(chip);
	//int val=0;
	//int ret;

	if (off<SD151_MAXGPIO) {
		//mutex_lock(&data->update_lock);

		/* Get GPIO value */
		//ret = regmap_read(data->regmap, SD151_CHIP_GPIO_VALUE+off, &val);
		//if (ret < 0) {
		//	mutex_unlock(&data->update_lock);
		//	dev_err(data->dev, "sd108_gpio_get_value failed to read I2C\n");
		//	return ret;
		//}

		//data->gpio.gpio_val = val;

		//mutex_unlock(&data->update_lock);

		return 0;
	}

	return -ENODEV;
}

static void sd151_gpio_set_value(struct gpio_chip *chip, unsigned off, int val)
{
	struct sd151_private *data = gpiochip_get_data(chip);
	int ret;

  if (off<SD151_MAXGPIO) {
		if (val == 0) // nothing to do
			return;

		mutex_lock(&data->update_lock);

		/* Set GPIO value */
		ret = regmap_write(data->regmap, SD151_CHIP_GPIO_VALUE+off, val);

    mutex_unlock(&data->update_lock);

		if (ret < 0) {
			dev_err(data->dev, "sd108_gpio_set_value failed to read I2C\n");
		}

		//data->gpio.gpio_val = val;
	}
}

static int sd151_gpio_direction_input(struct gpio_chip *chip, unsigned off)
{
//	struct sd151_private *data = gpiochip_get_data(chip);
//	int direction;
//	int ret;

//	if (off<SD151_MAXGPIO) {
//		mutex_lock(&data->update_lock);

		/* Set GPIO as input  */
		//direction = data->gpio.gpio_dir & ~(0xff<<(8*off));

//		ret = regmap_write(data->regmap, SD151_CHIP_GPIO_DIRECTION, direction);
//		if (ret < 0) {
//			mutex_unlock(&data->update_lock);
//			dev_err(data->dev, "sd108_gpio_set_value failed to read I2C\n");
//			return ret;
//		}
//		data->gpio.gpio_dir = direction;
//		mutex_unlock(&data->update_lock);
//		return 0;
//	}

	return -ENODEV;
}

static int sd151_gpio_direction_output(struct gpio_chip *chip,
					 unsigned off, int val)
{
//	struct sd151_private *data = gpiochip_get_data(chip);
//	int direction;
//	int ret;

	if (off<SD151_MAXGPIO) {
	//	sd151_gpio_set_value(chip,off,val);

//		mutex_lock(&data->update_lock);
	//	direction = data->gpio.gpio_dir & ~(0xff<<(8*off));
//		direction = direction | (1<<(8*off));
		/* Set GPIO as output  */
//		ret = regmap_write(data->regmap, SD151_CHIP_GPIO_DIRECTION, direction);
//		if (ret < 0) {
	//		mutex_unlock(&data->update_lock);
//			dev_err(data->dev, "sd108_gpio_set_value failed to read I2C\n");
//			return ret;
//		}
//		data->gpio.gpio_dir = direction;
//		mutex_unlock(&data->update_lock);
		return 0;
	}

	return -ENODEV;
}

int sd151_gpio_init(struct sd151_private *data)
{
  int ret;

  /* GPIO setup */

  data->gpio.gpio_dev.direction_input = sd151_gpio_direction_input;
	data->gpio.gpio_dev.direction_output = sd151_gpio_direction_output;
	data->gpio.gpio_dev.get = sd151_gpio_get_value;
	data->gpio.gpio_dev.set = sd151_gpio_set_value;
	data->gpio.gpio_dev.can_sleep = true;
	data->gpio.gpio_dev.base = -1;
	data->gpio.gpio_dev.parent = data->dev;
	data->gpio.gpio_dev.ngpio = SD151_MAXGPIO;
	data->gpio.gpio_dev.label = data->client->name;
	data->gpio.gpio_dev.owner = THIS_MODULE;

	ret = devm_gpiochip_add_data(data->dev, &data->gpio.gpio_dev, data);
	if (ret < 0) {
		dev_err(data->dev, "failed add GPIO\n");
		return ret;
	}

  return 0;
}

EXPORT_SYMBOL_GPL(sd151_gpio_init);
