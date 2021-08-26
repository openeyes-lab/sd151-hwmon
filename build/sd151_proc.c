/*
 * sd151_gpio.c - Part of OPEN-EYES PI-POW HAT product, Linux kernel modules
 * for hardware monitoring
 * This driver handles the SD151 PROC module.
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
#include <linux/proc_fs.h>	/* Necessary because we use the proc fs */

#include "sd151.h"

struct sd151_private *pdata;

#define SD151_PROC_MSG_LEN               32
#define SD151_PROC_BUFSIZE               512

ssize_t sd151_proc_write( struct file *filp, const char __user *buff, size_t len, loff_t *data )
{
	char 			    cmd[SD151_PROC_MSG_LEN];
	int				    ret;

  if(len>(SD151_PROC_MSG_LEN-2))
  	return -EFAULT;

  memset(cmd,0,sizeof(cmd));

  if (copy_from_user( cmd, buff, len ))
  	return -EFAULT;

  if(strncmp(cmd,"buzzer-low",len-1)==0) {
    ret = regmap_write(pdata->regmap, SD151_COMMAND, SD151_BUZZER_LOW);
  } else if(strncmp(cmd,"buzzer-high",len-1)==0) {
		ret = regmap_write(pdata->regmap, SD151_COMMAND, SD151_BUZZER_HIGH);
	} else if(strncmp(cmd,"fan-on",len-1)==0) {
		ret = regmap_write(pdata->regmap, SD151_COMMAND, SD151_FAN_FORCE_ENABLE);
	} else if(strncmp(cmd,"fan-off",len-1)==0) {
		ret = regmap_write(pdata->regmap, SD151_COMMAND, SD151_FAN_RELASE_CONTROL);
	}
  else{
  	return -EFAULT;
	}
  return len;
}

int sd151_proc_read( struct file *filp, char __user *ubuf, size_t count, loff_t *ppos )
{
	char buf[SD151_PROC_BUFSIZE];
	int len=0;
	int ret;
	unsigned int status;

	printk( KERN_INFO "read handler %p %d\n",ppos,count);
	if(*ppos > 0 || count < SD151_PROC_BUFSIZE)
		return 0;

	len += sprintf(buf+len, "\nModule      : sd151-hwmon");
	len += sprintf(buf+len, "\nVersion     : %d",pdata->firmware_version);

	/* get status */
	ret = regmap_read(pdata->regmap, SD151_STATUS, &status);
	if (ret < 0) {
		dev_err(pdata->dev, "failed to read I2C\n");
		return -EFAULT;
	}
	if (status&SD151_STATUS_WDOG_EN) {
		len += sprintf(buf+len, "\nwdog        : enabled");
	} else {
		len += sprintf(buf+len, "\nwdog        : disabled");
	}

	if (status&SD151_STATUS_WDOG_EN) {
		len += sprintf(buf+len, "\nsys restart : from wake-up");
	} else {
		if ((status&SD151_STATUS_BOOT_MASK)==SD151_STATUS_POWERUP) {
			len += sprintf(buf+len, "\nsys restart : from power-up");
		} else if ((status&SD151_STATUS_BOOT_MASK)==SD151_STATUS_POWEROFF) {
			len += sprintf(buf+len, "\nsys restart : from power-down");
		} else if ((status&SD151_STATUS_BOOT_MASK)==SD151_STATUS_REBOOT) {
			len += sprintf(buf+len, "\nsys restart : from reboot");
		}
	}

	/* get buttons */
	ret = regmap_read(pdata->regmap, SD151_BUTTONS, &status);
	if (ret < 0) {
		dev_err(pdata->dev, "failed to read I2C\n");
		return -EFAULT;
	}
	if (status&1)
		len += sprintf(buf+len, "\nbutton-1    : enabled");
	if (status&2)
		len += sprintf(buf+len, "\nbutton-2    : enabled");

	/* get FAN */
	ret = regmap_read(pdata->regmap, SD151_FAN, &status);
	if (ret < 0) {
		dev_err(pdata->dev, "failed to read I2C\n");
		return -EFAULT;
	}
	if (status==0)
		len += sprintf(buf+len, "\nFAN         : OFF");
	else if (status==1)
		len += sprintf(buf+len, "\nFAN         : Enabled from PROC");
	else if (status==2)
		len += sprintf(buf+len, "\nFAN         : Enabled from TEMP");
	else if (status==3)
		len += sprintf(buf+len, "\nFAN         : Disabled");
	else
		len += sprintf(buf+len, "\nFAN         : bad value(%x)",status);

	len += sprintf(buf+len, "\nEnd of report.\n");

	if(copy_to_user(ubuf,buf,len))
		return -EFAULT;

	*ppos = len;
	return len;
}

static const struct proc_ops sd151_proc_fops = {
  .proc_read = sd151_proc_read,
  .proc_write = sd151_proc_write,
};

int sd151_proc_init(struct sd151_private *data)
{
  pdata = data;

	data->proc_entry = proc_create("sd151", 0666, NULL, &sd151_proc_fops);
	if (data->proc_entry == NULL) {
			dev_err(data->dev,"Couldn't create proc entry\n");
			return -EIO;
	}

  return 0;
}

EXPORT_SYMBOL_GPL(sd151_proc_init);

int sd151_proc_remove(struct sd151_private *data)
{
  pdata = NULL;
  if (data->proc_entry)
    proc_remove(data->proc_entry);

  return 0;
}

EXPORT_SYMBOL_GPL(sd151_proc_remove);
