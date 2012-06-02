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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// For the command line utilities
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>    // O_... flags

#include "compat.h"
#include "misc_fns.h"

static uint32_t crc_table[256];

/**
	Method make_crc_table
	It generates ctc information

	\return void 
 
*/
static void make_crc_table()
{
  int i, j;
  int already_done = 1;
  uint32_t crc;

  for (i = 0; i < 256; i++)
  {
    crc = i << 24;
    for (j = 0; j < 8; j++)
    {
      if (crc & 0x80000000L)
        crc = (crc << 1) ^ CRC32_POLY;
      else
        crc = ( crc << 1 );
    }
    crc_table[i] = crc;
  }

}

/**
	 Compute CRC32 over a block of data, by table method.
 
 	 Returns a working value, suitable for re-input for further blocks
 
 	 Notes: Input value should be 0xffffffff for the first block,
 	      else return value from previous call (not sure if that
 	      needs complementing before being passed back in).
 */
extern uint32_t crc32_block(uint32_t crc, byte *pData, int blk_len)
{
  static int table_made = FALSE;
  int i, j;

  if (!table_made) make_crc_table();
  
  for (j = 0; j < blk_len; j++)
  {
    i = ((crc >> 24) ^ *pData++) & 0xff;
    crc = (crc << 8) ^ crc_table[i];
  }
  return crc;
}

/**
	encode pts and dts value 
	insert to a specified packet 
 */
extern void encode_pts_dts(byte    data[],
                           int     guard_bits,
                           uint64_t value)
{
  int   pts1,pts2,pts3;

#define MAX_PTS_VALUE 0x1FFFFFFFFLL
  
  if (value > MAX_PTS_VALUE)
  {
    char        *what;
    uint64_t     temp = value;
    while (temp > MAX_PTS_VALUE)
      temp -= MAX_PTS_VALUE;
    switch (guard_bits)
    {
    case 2:  what = "PTS alone"; break;
    case 3:  what = "PTS before DTS"; break;
    case 1:  what = "DTS after PTS"; break;
    default: what = "PTS/DTS/???"; break;
    }
    //fprintf(stderr,"!!! value " LLU_FORMAT " for %s is more than " LLU_FORMAT            " - reduced to " LLU_FORMAT "\n",value,what,MAX_PTS_VALUE,temp);
    value = temp;
  }

  pts1 = (int)((value >> 30) & 0x07);
  pts2 = (int)((value >> 15) & 0x7FFF);
  pts3 = (int)( value        & 0x7FFF);

  data[0] =  (guard_bits << 4) | (pts1 << 1) | 0x01;
  data[1] =  (pts2 & 0x7F80) >> 7;
  data[2] = ((pts2 & 0x007F) << 1) | 0x01;
  data[3] =  (pts3 & 0x7F80) >> 7;
  data[4] = ((pts3 & 0x007F) << 1) | 0x01;

  //PSIDebug ("pts data = %x %x %x %x %x", data[0],data[1], data[2], data[3], data[4]  );
}

// vim: set tabstop=8 shiftwidth=2 expandtab:
