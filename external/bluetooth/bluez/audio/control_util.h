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

#if __BYTE_ORDER == __LITTLE_ENDIAN

#define _htobs(d)  bswap_16(d)
#define _htobl(d)  bswap_32(d)
#define _htobll(d) bswap_64(d)
#define _btohs(d)  bswap_16(d)
#define _btohl(d)  bswap_32(d)
#define _btohll(d) bswap_64(d)

#elif __BYTE_ORDER == __BIG_ENDIAN

#define _htobs(d)  (d)
#define _htobl(d)  (d)
#define _htobll(d) (d)
#define _btohs(d)  (d)
#define _btohl(d)  (d)
#define _btohll(d)  (d)

#else
#error "Unknown byte order"
#endif

void avrcp_debug(const char *format, ...);
uint32_t be_to_host32(const uint8_t *ptr);
uint16_t be_to_host16(const uint8_t *ptr);
void print_bufferdata(uint8_t *addr, uint32_t size);

void store_u8(uint8_t **buf, uint8_t val);
void store_u16(uint8_t **buf, uint16_t val);
void store_u32(uint8_t **buf, uint32_t val);
void store_u64(uint8_t **buf, uint64_t val);
void store_arr(uint8_t **buf, uint8_t *arr, int len);
uint8_t load_u8(uint8_t **buf);
uint16_t load_u16(uint8_t **buf);
uint32_t load_u32(uint8_t **buf);
uint64_t load_u64(uint8_t **buf);

