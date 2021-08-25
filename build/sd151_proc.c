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
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/gpio/consumer.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
//#include <sound/core.h>
//#include <sound/pcm.h>
//#include <sound/pcm_params.h>
//#include <sound/soc.h>
#include <sound/initval.h>
//#include <sound/tlv.h>
//#include <sound/soc-dapm.h>
#include <linux/proc_fs.h>	/* Necessary because we use the proc fs */

#include "sd151.h"

struct regmap *regmap_priv;

#define SD151_PROC_MSG_LEN               32
#define SD151_PROC_BUFSIZE               512

ssize_t sd151_proc_write( struct file *filp, const char __user *buff, size_t len, loff_t *data )
{
	char 			    cmd[SD151_PROC_MSG_LEN];
	int				    ret;
	//long			    reg,val;
	//char			    hex[8];

  if(len>(SD151_PROC_MSG_LEN-2))
  	return -EFAULT;

  memset(cmd,0,sizeof(cmd));

  if (copy_from_user( cmd, buff, len ))
  	return -EFAULT;

	printk(KERN_INFO"sd151_proc_write:{%s} len %d",cmd,len);

  if(strncmp(cmd,"buzzer-low",len-1)==0) {
	printk(KERN_INFO"sd151_proc_write:correct");
    ret = regmap_write(regmap_priv, SD151_COMMAND, SD151_BUZZER_LOW);
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
	unsigned int reg, val;

	printk( KERN_INFO "read handler %p %d\n",ppos,count);
	if(*ppos > 0 || count < SD151_PROC_BUFSIZE)
		return 0;

	for (reg=0;reg<SD151_NUM_REGS;reg++) {
		if (reg%16==0) {
			len += sprintf(buf+len, "\n%02x : ", reg);
		}
		ret = regmap_read(regmap_priv, reg, &val);
	  if (ret < 0) {
		  len += sprintf(buf+len, " xx");
	  } else {
		  len += sprintf(buf+len," %02x",val);
	  }
    if (len>(SD151_PROC_BUFSIZE-128))
      break;
  }

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
  regmap_priv = data->regmap;

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
  regmap_priv = NULL;
  if (data->proc_entry)
    proc_remove(data->proc_entry);

  return 0;
}

EXPORT_SYMBOL_GPL(sd151_proc_remove);
