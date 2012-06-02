/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2006-2010  Nokia Corporation
 *  Copyright (C) 2004-2010  Marcel Holtmann <marcel@holtmann.org>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>

#include "log.h"
#include "syslog.h"
#include <string.h>

#include <glib.h>
#include <dbus/dbus.h>
#include <gdbus.h>

#include <bluetooth/bluetooth.h>

#include "control_types.h"
#include "control_util.h"

void avrcp_debug(const char *format,  ...)
{
	va_list ap;

	if (!ENABLE_AVRCP_DEBUG)
		return;

	va_start(ap, format);

	vsyslog(LOG_DEBUG, format, ap);

	va_end(ap);
}

#if __BYTE_ORDER == __LITTLE_ENDIAN
uint32_t be_to_host32(const uint8_t *ptr)
{
	return (uint32_t)(((uint32_t) *ptr << 24)   | \
			((uint32_t) *(ptr+1) << 16) | \
			((uint32_t) *(ptr+2) << 8)  | \
			((uint32_t) *(ptr+3)));
}

uint16_t be_to_host16(const uint8_t *ptr)
{
    return (uint16_t)(((uint16_t) *ptr << 8) | ((uint16_t) *(ptr+1)));
}
#elif __BYTE_ORDER == __BIG_ENDIAN
uint32_t be_to_host32(const uint8_t *ptr) { return (uint32_t)(*ptr); }
uint16_t be_to_host16(const uint8_t *ptr) { return (uint16_t)(*ptr); }
#else
#error "Unknown byte order"
#endif

void print_bufferdata(uint8_t *addr, uint32_t size)
{
	uint32_t i;

	if (size <= 0)
		return;

	for (i = 0; i < size; i++)
		avrcp_debug("0x%02x ", addr[i]);

	return;
}

void store_u8(uint8_t **buf, uint8_t val)
{
	**buf = val;
	*buf += 1;
}

void store_u16(uint8_t **buf, uint16_t val)
{
	val = _htobs(val);
	memcpy(*buf, &val, sizeof(uint16_t));
	*buf += 2;
}

void store_u32(uint8_t **buf, uint32_t val)
{
	val = _htobl(val);
	memcpy(*buf, &val, sizeof(uint32_t));
	*buf += 4;
}

void store_u64(uint8_t **buf, uint64_t val)
{
	val = _htobll(val);
	memcpy(*buf, &val, sizeof(uint64_t));
	*buf += 8;
}

void store_arr(uint8_t **buf, uint8_t *arr, int len)
{
	memcpy(*buf, arr, len);
	*buf += len;
}

uint8_t load_u8(uint8_t **buf)
{
	uint8_t val = **buf;
	*buf += 1;
	return val;
}

uint16_t load_u16(uint8_t **buf)
{
	uint16_t val;

	memcpy(&val, *buf, sizeof(uint16_t));
	val = _btohs(val);
	*buf += 2;
	return val;
}

uint32_t load_u32(uint8_t **buf)
{
	uint32_t val;

	memcpy(&val, *buf, sizeof(uint32_t));
	val = _btohl(val);
	*buf += 4;
	return val;
}

uint64_t load_u64(uint8_t **buf)
{
	uint64_t val;

	memcpy(&val, *buf, sizeof(uint64_t));
	val = _btohll(val);
	*buf += 8;
	return val;
}
