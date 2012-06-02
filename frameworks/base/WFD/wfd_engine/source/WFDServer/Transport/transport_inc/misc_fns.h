/*
 * Miscellaneous useful functions.
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the MPEG TS, PS and ES tools.
 *
 * The Initial Developer of the Original Code is Amino Communications Ltd.
 * Portions created by the Initial Developer are Copyright (C) 2008
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Amino Communications Ltd, Swavesey, Cambridge UK
 *
 * ***** END LICENSE BLOCK *****
 */
#ifndef _misc_fns
#define _misc_fns
#include <stdint.h>
#endif


#define CRC32_POLY 0x04c11db7L

/*
 * Compute CRC32 over a block of data, by table method.
 *
 * Returns a working value, suitable for re-input for further blocks
 *
 * Notes: Input value should be 0xffffffff for the first block,
 *        else return value from previous call (not sure if that
 *        needs complementing before being passed back in).
 */
extern uint32_t crc32_block(uint32_t crc, byte *pData, int blk_len);

/**
	encode pts and dts value 
	insert to a specified packet 
 */

extern void encode_pts_dts(byte    data[],
                           int     guard_bits,
                           uint64_t value);




