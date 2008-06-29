/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 *   Copyright (C) 2007 John Crispin <blogic@openwrt.org> 
 */

#include <linux/autoconf.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/mtd/physmap.h>
#include <linux/kernel.h>
#include <linux/reboot.h>
#include <linux/platform_device.h>
#include <asm/bootinfo.h>
#include <asm/reboot.h>
#include <asm/time.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/etherdevice.h>
#include <asm/ifxmips/ifxmips.h>
#include <asm/ifxmips/ifxmips_mii0.h>

#define MAX_IFXMIPS_DEVS		9

#define BOARD_DANUBE			"Danube"
#define BOARD_DANUBE_CHIPID		0x10129083

#define BOARD_TWINPASS			"Twinpass"
#define BOARD_TWINPASS_CHIPID	0x3012D083

static unsigned int chiprev;
static struct platform_device *ifxmips_devs[MAX_IFXMIPS_DEVS];
static int cmdline_mac = 0;

spinlock_t ebu_lock = SPIN_LOCK_UNLOCKED;
EXPORT_SYMBOL_GPL(ebu_lock);

static struct ifxmips_mac ifxmips_mii_mac;

static struct platform_device
ifxmips_led[] =
{
	{
		.id = 0,
		.name = "ifxmips_led",
	},
};

static struct platform_device
ifxmips_gpio[] =
{
	{
		.id = 0,
		.name = "ifxmips_gpio",
	},
};

static struct platform_device
ifxmips_mii[] =
{
	{
		.id = 0,
		.name = "ifxmips_mii0",
		.dev = {
			.platform_data = &ifxmips_mii_mac,
		}
	},
};

static struct platform_device
ifxmips_wdt[] =
{
	{
		.id = 0,
		.name = "ifxmips_wdt",
	},
};

static struct resource
ifxmips_mtd_resource = {
	.start  = IFXMIPS_FLASH_START,
	.end    = IFXMIPS_FLASH_START + IFXMIPS_FLASH_MAX - 1,
	.flags  = IORESOURCE_MEM,
};

static struct platform_device
ifxmips_mtd[] =
{
	{
		.id = 0,
		.name = "ifxmips_mtd",
		.num_resources  = 1,
		.resource   = &ifxmips_mtd_resource,
	},
};

#ifdef CONFIG_GPIO_DEVICE
static struct resource
ifxmips_gpio_dev_resources[] = {
	{
		.name = "gpio",
		.flags  = 0,
		.start = (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) |
			(1 << 4) | (1 << 5) | (1 << 8) | (1 << 9) | (1 << 12),
		.end = (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) |
			(1 << 4) | (1 << 5) | (1 << 8) | (1 << 9) | (1 << 12),
	},
};

static struct platform_device
ifxmips_gpio_dev[] = {
	{
		.name     = "GPIODEV",
		.id     = -1,
		.num_resources    = ARRAY_SIZE(ifxmips_gpio_dev_resources),
		.resource   = ifxmips_gpio_dev_resources,
	}
};
#endif

const char*
get_system_type(void)
{
	chiprev = ifxmips_r32(IFXMIPS_MPS_CHIPID);
	switch(chiprev)
	{
	case BOARD_DANUBE_CHIPID:
		return BOARD_DANUBE;

	case BOARD_TWINPASS_CHIPID:
		return BOARD_TWINPASS;
	}

	return BOARD_SYSTEM_TYPE;
}

#define IS_HEX(x)	(((x >='0' && x <= '9') || (x >='a' && x <= 'f') || (x >='A' && x <= 'F'))?(1):(0))
static int __init
ifxmips_set_mii0_mac(char *str)
{
	int i;
	str = strchr(str, '=');
	if(!str)
		goto out;
	str++;
	if(strlen(str) != 17)
		goto out;
	for(i = 0; i < 6; i++)
	{
		if(!IS_HEX(str[3 * i]) || !IS_HEX(str[(3 * i) + 1]))
			goto out;
		if((i != 5) && (str[(3 * i) + 2] != ':'))
			goto out;
		ifxmips_mii_mac.mac[i] = simple_strtoul(&str[3 * i], NULL, 16);
	}
	if(is_valid_ether_addr(ifxmips_mii_mac.mac))
		cmdline_mac = 1;
out:
	return 1;
}
__setup("mii0_mac", ifxmips_set_mii0_mac);

int __init
ifxmips_init_devices(void)
{
	int dev = 0;

	if(!cmdline_mac)
		random_ether_addr(ifxmips_mii_mac.mac);

	ifxmips_devs[dev++] = ifxmips_led;
	ifxmips_devs[dev++] = ifxmips_gpio;
	ifxmips_devs[dev++] = ifxmips_mii;
	ifxmips_devs[dev++] = ifxmips_mtd;
	ifxmips_devs[dev++] = ifxmips_wdt;
#ifdef CONFIG_GPIO_DEVICE
	ifxmips_devs[dev++] = ifxmips_gpio_dev;
#endif
	return platform_add_devices(ifxmips_devs, dev);
}

arch_initcall(ifxmips_init_devices);
