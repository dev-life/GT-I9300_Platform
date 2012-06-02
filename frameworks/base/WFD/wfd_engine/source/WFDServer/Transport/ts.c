/*
 * Utilities for working with H.222 Transport Stream packets
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
 *
 * It is modifed by ghryu for wi-fi display	
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>

#include "PSIDebug.h"

#include "compat.h"
#include "ts_fns.h"

#include "PSIApi.h"

//added for wifi display : ghryu 
#include "RTPPacket.h"

#ifdef ENABLE_WFD_HDCP
#include "EncryptFrame.h"
#endif

#define VIDEO_OUTPUT_BUFFER_SIZE 256 * 1024
#define AUDIO_OUTPUT_BUFFER_SIZE   4 * 1024

unsigned char g_vOutputbuffer[VIDEO_OUTPUT_BUFFER_SIZE];
unsigned char g_aOutputbuffer[AUDIO_OUTPUT_BUFFER_SIZE];


#define RTP_HEADER_SIZE						12 
#define ENCRYPTION_CHUNK_SIZE 				16
#define HDCP_REGISTRATION_DESCRIPTOR_LENGTH  7

#define ADD_RTP_DELAY

//#define A_LPCM

//for ts write
//#define TS_WRITE

#ifdef TS_WRITE
    FILE *fstream; 
#endif 

//type definition enumeraion 
typedef enum 
{
	ES_VIDEO 	=  0,  
	ES_AUDIO 
}ES_TYPE; 

//ts common context for av object 
typedef struct 
{
	void 		*id; 
	
	ES_TYPE		es_type; 

	int 		increment ; 
	int 		data_length ; 
#ifdef ADD_RTP_DELAY
    int         actual_data_length ;
    int         cnt_ts;
#endif
	byte  		*cur_data ;	
	
	int 		wfd_idx; 
	char		*buf0_ptr ;
	char		*pack_start ;

	//TS_writer_p 	output; 
	
	int 	end_marker;  	

	HObject 	mutex; 

	int        hdcpEnabled;
#ifdef ENABLE_WFD_HDCP
	void *hdcp_ctx;
#endif
	
	unsigned long long  mInputCounter;
	unsigned long mStrmCounter;
	int         codecType;
}TS_CTX_S; 

/**
	Method : swap_data 
	audio data conversion 
	little endian to big endian 
	this is only for cavium dongle only if interoperate with other than this api is useless 

	\param data 	[IN] data start pointer
	\param data_len [IN] data length 
	\return success or fail 
*/
extern int swap_data (char *data,  unsigned int data_len) 
{
	short 	*s; 
	int 	idx; 
	int 	count; 

	s = (short*)data; 

	if ( data_len == 0 ) 
	{
		PSIDebug(" erro audio frame length"); 
		return 1; 
	}	

	// check if a data_length is an odd or even 
	if (((data_len & 0x01) == 0))  // even 
		count = data_len >> 1;
	else
		count = (data_len - 1) >> 1;  // odd : check this again	

	for (idx = 0; idx < count + 1; idx ++) 
		s[idx] = (    ((s[idx] & 0xff) << 8)  |   ((s[idx] >> 8) & 0xff)  );
			
	return 0; 	
}

int f_rtcp;


/** 
	Method : A TS context creation
	
	\param type 	[IN] object type definition 
	\param HObject 	[IN] mutex 
	\return ts context
*/
extern void* ts_ctx_create(int type, HObject rtp_mutex)
{
	TS_CTX_S 	*ctx;
	ES_TYPE		es_type = ES_VIDEO;

	ctx = (TS_CTX_S*)PSIMalloc(sizeof(TS_CTX_S));
	PSIMemset(ctx, 0x0, sizeof(TS_CTX_S)); // 20120318_sinwon.seo, Add memset codes

	ctx->id = (void*)ctx;

	if (type == ES_VIDEO )
	{
		es_type = ES_VIDEO;
	}
	else if ( type == ES_AUDIO )
	{
		es_type = ES_AUDIO;
		//ctx->mStrmCounter = 1;
	}

	ctx->mStrmCounter = 0;

	ctx->es_type = es_type;
	ctx->buf0_ptr = NULL;
	ctx->data_length = 0;
#ifdef ADD_RTP_DELAY
    ctx->actual_data_length = 0;
    ctx->cnt_ts = 0;
#endif
	ctx->increment = 0;
	ctx->pack_start = NULL;
	ctx->wfd_idx = 0;
	ctx->end_marker = 0;
	ctx->mutex = rtp_mutex;
	ctx->hdcpEnabled = 0;
	ctx->mInputCounter = 0;

// for RTCP value setting 
	f_rtcp = 1;

#ifdef TS_WRITE
       if ( fstream == NULL )
            fstream = fopen ("/sdcard/Android/data/ts_output.ts", "wb");  
#endif 
	
	return (void*)ctx;
}

/** 
	Method : A TS context deletion 
	
	\param id 	[IN] object type definition 
	\return ts context  	
*/
extern int ts_ctx_delete(void *id)
{
	TS_CTX_S 	*ctx;

	ctx = (TS_CTX_S*)id;


	if ( ctx != NULL )
		PSIMemFree(ctx);

#ifdef TS_WRITE	
	if (fstream) 
		fclose(fstream); 
#endif 	

	return 0;
}

/**
	Method : A TS context initialization
	
	\param id 	[IN] object type identifier
	\return success or fail
*/
int ts_ctx_init(TS_CTX_S *ctx)
{

	ctx->wfd_idx = 0;
	ctx->buf0_ptr = NULL;
	ctx->pack_start = NULL;

	return 0;
}

/**
	Definition : Contiunity_counter used for TS packet linearity check up 
*/

static int continuity_counter[0x1fff+1] = {0};

/** 
	Return the next value of continuity_counter for the given pid
	
	\param pid 	[IN] program identifier 
*/
static inline int next_continuity_count(uint32_t pid)
{
  uint32_t next = (continuity_counter[pid] + 1) & 0x0f;
  continuity_counter[pid] = next;
  return next;
}

static unsigned char GetBitsFrom64Bits(unsigned long long input, int lsb, int msb)
{
	unsigned long long lMask = 0;
	unsigned long long lShiftedMask = 0;
	int lOutput = 0;
	int lMaskLen = msb - lsb + 1;
	int lIndex = 0;
	
	if (lMaskLen > 8)
	{
		PSIDebug("Requested msb:%d lsb:%d overflows with integer return", msb, lsb);
		return 0;
	}
	for ( ;lIndex < lMaskLen; lIndex++)
	    lMask = (lMask << 1) | 1;
	
	lShiftedMask = lMask << lsb;
	lOutput = (input & lShiftedMask) >> lsb;
	return lOutput;
}

static unsigned char GetBitsFrom32Bits(unsigned long  input, int lsb, int msb)
{
	unsigned long  lMask = 0;
	unsigned long  lShiftedMask = 0;
	int  lOutput = 0;
	int lMaskLen = msb - lsb + 1;
	int lIndex = 0;
	
	if (lMaskLen > 8)
	{
		PSIDebug("Requested msb:%d lsb:%d overflows with integer return", msb, lsb);
		return 0;
	}
	for (; lIndex < lMaskLen; lIndex++)
	    lMask = (lMask << 1) | 1;
	
	lShiftedMask = lMask << lsb;
	lOutput = (input & lShiftedMask) >> lsb;
	return lOutput;
}

static void AddPrivateData(byte *PES_hdr, TS_CTX_S* ctx, int lIndex)
{
	//Add the private data.
	PES_hdr[lIndex + 1] = 0x00; //Reserved bits
	PES_hdr[lIndex + 2] = 0x07 & ((GetBitsFrom32Bits(ctx->mStrmCounter, 30, 31) << 1) | 1); //5 bits reserved,31~30 bit and a marker bit.
	PES_hdr[lIndex + 3] = GetBitsFrom32Bits(ctx->mStrmCounter, 22, 29); //29~22 bit.
	PES_hdr[lIndex + 4] = (GetBitsFrom32Bits(ctx->mStrmCounter, 15, 21) << 1) | 1; //21~15 bit and a marker bit.
	PES_hdr[lIndex + 5] = GetBitsFrom32Bits(ctx->mStrmCounter, 7, 14); //14~7 bit.
	PES_hdr[lIndex + 6] = (GetBitsFrom32Bits(ctx->mStrmCounter, 0, 6) << 1) | 1; //6~0 bit and a marker bit.
	
	PES_hdr[lIndex + 7] = 0x00; //Reserved bits
	PES_hdr[lIndex + 8] = 0x0F & ((GetBitsFrom64Bits(ctx->mInputCounter, 60, 63) << 1) | 1); //4 bits reserved,63~60 bit and a marker bit.
	PES_hdr[lIndex + 9] = GetBitsFrom64Bits(ctx->mInputCounter, 52, 59); //59~52 bit.
	PES_hdr[lIndex + 10] = (GetBitsFrom64Bits(ctx->mInputCounter, 45, 51) << 1) | 1; //51~45 bit and a marker bit.
	PES_hdr[lIndex + 11] = GetBitsFrom64Bits(ctx->mInputCounter, 37, 44); //44~37 bit.
	PES_hdr[lIndex + 12] = (GetBitsFrom64Bits(ctx->mInputCounter, 30, 36) << 1) | 1; //36~30 bit and a marker bit.
	PES_hdr[lIndex + 13] = GetBitsFrom64Bits(ctx->mInputCounter, 22, 29); //29~22 bit.
	PES_hdr[lIndex + 14] = (GetBitsFrom64Bits(ctx->mInputCounter, 15, 21) << 1) | 1; //21~15 bit and a marker bit.
	PES_hdr[lIndex + 15] = GetBitsFrom64Bits(ctx->mInputCounter, 7, 14); //14~7 bit.
	PES_hdr[lIndex + 16] = (GetBitsFrom64Bits(ctx->mInputCounter, 0, 6) << 1) | 1; //6~0 bit and a marker bit.
	
	/*
	PSIDebug("PrivData: x%xx%xx%xx%xx%xx%xx%xx%xx%xx%xx%xx%xx%xx%xx%xx%x", PES_hdr[lIndex + 1], PES_hdr[lIndex + 2], PES_hdr[lIndex + 3],
			PES_hdr[lIndex + 4], PES_hdr[lIndex + 5], PES_hdr[lIndex + 6], PES_hdr[lIndex + 7], PES_hdr[lIndex + 8], PES_hdr[lIndex + 9], PES_hdr[lIndex + 10],
			PES_hdr[lIndex + 11], PES_hdr[lIndex + 12], PES_hdr[lIndex + 13], PES_hdr[lIndex + 14], PES_hdr[lIndex + 15], PES_hdr[lIndex + 16]);
	 */
}

/** 
	Create a PES header for video ES 
	
	\param data_len [IN] length of our ES data 
	 If this is too long to fit into 16 bits, then we will create a header
     with 0 as its length. Note this is only allowed (by the standard) for
     video data.	
	\param stream_id [IN] elementary stream id to use (see H.222 Table 2-18).
     If the stream id indicates an audio stream (as elucidated by Note 2 in
     that same table), then the data_alignment_indicator flag will be set
     in the PES header - i.e., we assume that the audio frame *starts*
     (has its syncword) at the start of the PES packet payload. 
	\param with_PTS [IN] it should be TRUE if the PTS value in `pts` should be written
	 to the PES header.
	\param with_DTS [IN] it should be TRUE if the DTS value in `dts` should be written
	 to the PES header. Note that if `with_DTS` is TRUE, then `with_PTS`
     must also be TRUE. If it is not, then the DTS value will be used for
     the PTS.
	\param PES_hdr[IN] it is the resultant PES packet header
	\param PES_hdr_len [IN]  its length (at the moment that's always the same, as
	 we're not yet outputting any timing information (PTS/DTS), and so
	 can get away with a minimal PES header).
	\param ctx [IN] ts context 
*/
extern void PES_header(uint32_t  data_len,
                       byte      stream_id,
                       int       with_PTS,
                       uint64_t  pts,
                       int       with_DTS,
                       uint64_t  dts,
                       byte     *PES_hdr,
                       int      *PES_hdr_len,
					   TS_CTX_S* ctx)
{	
	int  extra_len = 0;
	int lPES_Extn_Flag = 0;
    int lHdcp_Data_len = 0;
    int lExtnFlagPosn = 9; //Default case of not having PTS and DTS.

  // packet_start_code_prefix
  PES_hdr[0] = 0x00;
  PES_hdr[1] = 0x00;
  PES_hdr[2] = 0x01;

  PES_hdr[3] = 0xE0;//stream_id;

  // PES_packet_length comes next, but we'll actually sort it out
  // at the end, when we know what else we've put into our header

  // Flags: '10' then PES_scrambling_control .. original_or_copy
  // If it appears to be an audio stream, we set the data alignment indicator
  // flag, to indicate that the audio data starts with its syncword. For video
  // data, we leave the flag unset.
  PES_hdr[6] = 0x80;     // no flags set

  if (ctx->hdcpEnabled)
  {
	lPES_Extn_Flag = 1;
	/* 1 byte is to specify private data flag and other flags. 16 bytes are to
	 * specify the user defined private data.
	 */
	lHdcp_Data_len = 1 + 16;
  }
  
  // Flags: PTS_DTS_flags .. PES_extension_flag
  if (with_DTS && with_PTS)
    PES_hdr[7] = 0xC0;
  else if (with_PTS)
    PES_hdr[7] = 0x80;
  else
    PES_hdr[7] = 0x00;     // yet more unset flags (nb: no PTS/DTS info)

  PES_hdr[7] |= lPES_Extn_Flag;

  // PES_header_data_length
  if (with_DTS && with_PTS)
  {
    PES_hdr[8] = 0x0A;
    encode_pts_dts(&(PES_hdr[9]),3,pts);
    encode_pts_dts(&(PES_hdr[14]),1,dts);
    *PES_hdr_len = 9 + 10;
    extra_len = 3 + 10; // 3 bytes after the length field, plus our PTS & DTS
    lExtnFlagPosn = 19;
  }
  else if (with_PTS)
  {
    PES_hdr[8] = 0x05;
	//PES_hdr[8] = 0x0E;
    encode_pts_dts(&(PES_hdr[9]),2,pts);
    *PES_hdr_len = 9 + 5;
    extra_len = 3 + 5; // 3 bytes after the length field, plus our PTS
    lExtnFlagPosn = 14;
  }
  else
  {
    PES_hdr[8] = 0x00; // 0 means there is no more data
    *PES_hdr_len = 9;
    extra_len = 3; // just the basic 3 bytes after the length field
  }

  PES_hdr[8] += lHdcp_Data_len;
  *PES_hdr_len += lHdcp_Data_len;
  extra_len += lHdcp_Data_len;

  if (lPES_Extn_Flag)
  {
	//Extension flag is set. Add the byte for the extension flag.
	PES_hdr[lExtnFlagPosn] = 0x8E; //Private data flag set.
	
	//Add the private data.
	AddPrivateData(PES_hdr, ctx, lExtnFlagPosn);
  }
  // So now we can set the length field itself...
  if (data_len > 0xFFFF || (data_len + extra_len) > 0xFFFF)
  {
    // If the length is too great, we just set it "unset"
    // @@@ (this should only really be done for TS-wrapped video, so perhaps
    //     we should complain if this is not video?)
    PES_hdr[4] = 0;
    PES_hdr[5] = 0;
  }
  else
  {
    // The packet length doesn't include the bytes up to and including the
    // packet length field, but it *does* include any bytes of the PES header
    // after it.
    data_len += extra_len;
    PES_hdr[4] = (byte) ((data_len & 0xFF00) >> 8);
    PES_hdr[5] = (byte) ((data_len & 0x00FF));
  }
}

/** 
	Create a PES header for audio ES 
	
	\param data_len [IN] length of our ES data 
	 If this is too long to fit into 16 bits, then we will create a header
     with 0 as its length. Note this is only allowed (by the standard) for
     video data.	
	\param stream_id [IN] elementary stream id to use (see H.222 Table 2-18).
     If the stream id indicates an audio stream (as elucidated by Note 2 in
     that same table), then the data_alignment_indicator flag will be set
     in the PES header - i.e., we assume that the audio frame *starts*
     (has its syncword) at the start of the PES packet payload. 
	\param with_PTS [IN] it should be TRUE if the PTS value in `pts` should be written
	 to the PES header.
	\param with_DTS [IN] it should be TRUE if the DTS value in `dts` should be written
	 to the PES header. Note that if `with_DTS` is TRUE, then `with_PTS`
     must also be TRUE. If it is not, then the DTS value will be used for
     the PTS.
	\param PES_hdr[IN] it is the resultant PES packet header
	\param PES_hdr_len [IN]  its length (at the moment that's always the same, as
	 we're not yet outputting any timing information (PTS/DTS), and so
	 can get away with a minimal PES header).
	\param ctx [IN] ts context 
*/
extern void PES_A_header(uint32_t  data_len,
                       byte      stream_id,
                       int       with_PTS,
                       uint64_t  pts,
                       int       with_DTS,
                       uint64_t  dts,
                       byte     *PES_hdr,
                       int      *PES_hdr_len,
					   TS_CTX_S* ctx)
{
	int  extra_len = 0;

	int lPES_Extn_Flag = 0;
	int lHdcp_Data_len = 0;
	int lExtnFlagPosn = 14; //Default case
	
	
	// packet_start_code_prefix
	PES_hdr[0] = 0x00;
	PES_hdr[1] = 0x00;
	PES_hdr[2] = 0x01;

	if (ctx->codecType == 0) {
            PES_hdr[3] = 0xBD; //10111101
	
        } else if (ctx->codecType == 1) {
            PES_hdr[3] = 0xC0; //b1100000 
        }
	PES_hdr[6] = 0x81;     // flag set 

	if (ctx->hdcpEnabled)
	{
		lPES_Extn_Flag = 1;
		/* 1 byte is to specify private data flag and other flags. 16 bytes are to
		 * specify the user defined private data.
		 */
		lHdcp_Data_len = 1 + 16;
	}
	
	PES_hdr[7] = 0x80 | lPES_Extn_Flag;
	PES_hdr[8] = 0x07 + lHdcp_Data_len;

	if (with_PTS && !with_DTS)
	{
		//PSIDebug ("PES_A_header pts = %llu", pts); 
		encode_pts_dts(&(PES_hdr[9]),2,pts);

		//PSIDebug ("pts data = %x %x %x %x %x", PES_hdr[9],PES_hdr[10], PES_hdr[11], PES_hdr[12], PES_hdr[13]  );
		
		*PES_hdr_len = 20 + lHdcp_Data_len;
		extra_len = 3 + 5 + lHdcp_Data_len; // 3 bytes after the length field, plus our PTS
	}
	
	// So now we can set the length field itself...
	if (data_len > 0xFFFF || (data_len + extra_len) > 0xFFFF)
	{
		// If the length is too great, we just set it "unset"
		// @@@ (this should only really be done for TS-wrapped video, so perhaps
		//     we should complain if this is not video?)
		PES_hdr[4] = 0;
		PES_hdr[5] = 0;
	}
	else
	{
		// The packet length doesn't include the bytes up to and including the
		// packet length field, but it *does* include any bytes of the PES header
		// after it.

		if ( ctx->codecType == 0) 
		data_len = 1934 + lHdcp_Data_len;
	
		PES_hdr[4] = (byte) ((data_len & 0xFF00) >> 8);
		PES_hdr[5] = (byte) ((data_len & 0x00FF));
	}

	if (lPES_Extn_Flag)
	{
		//Extension flag is set. Add the byte for the extension flag.
		PES_hdr[lExtnFlagPosn] = 0x8E; //Private data flag set.

		//Add the private data.
		AddPrivateData(PES_hdr, ctx, lExtnFlagPosn);
		lExtnFlagPosn += 17;
	}
	// end of PES header 

	// additional header 
	PES_hdr[lExtnFlagPosn++] = (byte) ( (0xFF00) >> 8);
	PES_hdr[lExtnFlagPosn++] = (byte) (0x00FF);

	if (ctx->codecType == 0) {
            // sub stream id 	
            PES_hdr[lExtnFlagPosn++] = 0xa0; 
            PES_hdr[lExtnFlagPosn++] = 0x06; 
            //for cavium 0x01 otherwise 0x00
            PES_hdr[lExtnFlagPosn++] = 0x01;   //audio emphasis flag 
            // stero (2ch) 
            PES_hdr[lExtnFlagPosn++] = 0x11; 
        }
}

/** 
  	Write out TS packet, as composed from its parts.
 	`TS_hdr_len` + `pes_hdr_len` + `data_len` should equal 188.
 	`pes_hdr_len` may be 0 if there is no PES header data to be written
    (i.e., this is not the start of the PES packet's data, or the packet
    is not a PES packet).
 
    For real data, `data_len` should never be 0 (the exception is when
    writing NULL packets).
 
    `TS_hdr_len` must never be 0.

	\parma ctx [IN} ts context 
    \param ts_packet  [IN] `TS_packet` is the TS packet buffer
 	\param ts_hdr_len [IN] length of TS packet 
    \param pes_hdr    [IN] it is the PES header data, 
    \param  pes_hdr_len [IN] it is the length of PES header 
    \param data [IN} it is the actual ES data
    \param data_len 	[IN] it is the length data
    \param pid 	[IN] 	program id 
    \param got_pcr [IN] pcr flag 
    \param pcr [IN] pcr value 
 
  	\return 0 if it worked, 1 if something went wrong.
 */
static int write_TS_packet_parts(TS_CTX_S *ctx,
                                 byte        TS_packet[TS_PACKET_SIZE],
                                 int         TS_hdr_len,
                                 byte        pes_hdr[],
                                 int         pes_hdr_len,
                                 byte        data[],
                                 int         data_len,
                                 uint32_t    pid,
                                 int         got_pcr,
                                 uint64_t    pcr)
{ 
	int result = 0; 
	int total_len  = TS_hdr_len + pes_hdr_len + data_len;
    static int count;

	if (total_len != TS_PACKET_SIZE)
	{
		PSIDebug (" TS packet length is %d, not 188 (composed of %d + %d + %d)\n",	    total_len,TS_hdr_len,pes_hdr_len,data_len); 
		return 1;
	}

	
        // Make the header copy to the packet. 
	if (pes_hdr_len > 0)
		memcpy(&(TS_packet[TS_hdr_len]),pes_hdr,pes_hdr_len);

        // Make the data copy to the packet. 
	if (data_len > 0 && data_len <= 184)
		memcpy(&(TS_packet[TS_hdr_len+pes_hdr_len]),data,data_len);

	if (  ((ctx->wfd_idx % 7) == 6)  || (ctx->end_marker == 1) ) 
	{
		// mutex lock for rtp object 
		PSILockMutex(ctx->mutex);

		if (ctx->end_marker)
			result = makeRTP_Packet(ctx->buf0_ptr, (TS_PACKET_SIZE * (ctx->wfd_idx + 1)), 1);  
		else
			result = makeRTP_Packet(ctx->buf0_ptr, (TS_PACKET_SIZE * (ctx->wfd_idx + 1)), 0);  

		if ( result ) 
		{
			PSIDebug("makeRTP_Packet err"); 
			PSIUnlockMutex(ctx->mutex);
			return result; 
		}

        //mutex unlock for rtp object 
		PSIUnlockMutex(ctx->mutex); 
		
#ifdef ADD_RTP_DELAY /* NOT USED with blocking socket */
        if (ctx->cnt_ts == 5)
        {
            ctx->cnt_ts = 0;
            usleep(66);
        }
        else
        {
            ctx->cnt_ts += 1;
        }
#endif

#ifdef 	TS_WRITE
        if (ctx->fstream) {
                    //PSIDebug ("Start Pnt : %x, Size : %ld", ctx->buf0_ptr+RTP_HEADER_SIZE, TS_PACKET_SIZE * (ctx->wfd_idx + 1));			  
		fwrite(ctx->buf0_ptr+RTP_HEADER_SIZE,1,(TS_PACKET_SIZE * (ctx->wfd_idx + 1)),ctx->fstream);
		PSIDebug("TS_WRITE to file"); 
        }
        else {
            PSIDebug ("File is not open.");
        }
#endif
		ts_ctx_init(ctx); 

	}
	else ctx->wfd_idx += 1;
	
   return result;
}

/** 
 	 Write data as a (series of) Transport Stream PES packets
 
 	\param pes_hdr [IN] it is NULL if the data to be written out is already PES, and is
 	 otherwise a PES header constructed with PES_header() 
 	\param pes_hdr_len [IN] it is the length of said PES header (or 0)
 	\param data [IN] it is (the remainder of) our ES data (e.g., a NAL unit) or PES packet
 	\param data_len [IN] it is the length of data 
 	\param start [IN] it is true if this is the first time we've called this function
     to output (part of) this data (in other words, this should be true
     when someone else calls this function, and false when the function
     calls itself). This is expected to be TRUE if a PES header is given...
 	\parma set_pusi [IN] it is TRUE if we should set the payload unit start indicator
    (generally true if `start` is TRUE). This is ignored if `start` is FALSE.
	\param pid [IN] it is the PID to use for this TS packet
 	\param stream_id [IN] it is the PES packet stream id to use (e.g.,DEFAULT_VIDEO_STREAM_ID)
 	\param got_PCR [IN] it is TRUE if we have a `PCR` value (this is only relevant when `start` is also TRUE).
 	\param PCR_base` and `PCR_extn` then encode that PCR value
 	
 	\Returns 0 if it worked, 1 if something went wrong.
*/
static int write_some_TS_PES_packet(TS_CTX_S  *ctx,
                                    byte        *pes_hdr,
                                    int          pes_hdr_len,
                                    byte        *data,
                                    int32_t     data_len,
                                    int          start,
                                    int          set_pusi,
                                    uint32_t     pid,
                                    byte         stream_id,
                                    int          got_PCR,
                                    uint64_t     PCR_base,
                                    uint32_t     PCR_extn
									//int          f_sEnd
                                    )
{
  //byte TS_packet[TS_PACKET_SIZE];
  char *TS_packet; 

  int     TS_hdr_len;
  uint32_t controls = 0;
  int32_t pes_data_len = 0;
  int     err;
  int     got_adaptation_field = FALSE;
  int32_t space_left;  // Bytes available for payload, after the TS header
  int 		idx = 0; 

  if ( ctx == NULL )
  {
	PSIDebug (" write_some_TS_PES_packet : ctx is NULL\n"); 
	return 1;
  }		

  if (pid < 0x0010 || pid > 0x1ffe)
  {
    
	PSIDebug(" pid error"); 
	return 1;
  }

  // If this is the first time we've "seen" this data, and it is not
  // already wrapped up as PES, then we need to remember its PES header
  if (pes_hdr)
    pes_data_len = data_len + pes_hdr_len;
  else
  {
    pes_hdr_len = 0;
    pes_data_len = data_len;
  }

  if ((ctx->wfd_idx % 0x07) == 0) 
  { 	
	ctx->buf0_ptr= getWFD_BufPtr(ctx->es_type);	
  	ctx->pack_start = ctx->buf0_ptr +  RTP_HEADER_SIZE; 
  	ctx->wfd_idx = 0;   
  }	

  TS_packet = ctx->pack_start + (TS_PACKET_SIZE * ctx->wfd_idx);  

 //PSIDebug ("TS_packet start\n");	
  // We always start with a sync_byte to identify this as a
  // Transport Stream packet
  *TS_packet  = 0x47;
  // Should we set the "payload_unit_start_indicator" bit?
  // Only for the first packet containing our data.W
  if (start && set_pusi)
    *(TS_packet +1 ) = (byte)(0x40 | ((pid & 0x1f00) >> 8));
  else
    *(TS_packet +1 ) = (byte)(0x00 | ((pid & 0x1f00) >> 8));
  *(TS_packet +2) = (byte)(pid & 0xff);

  // Sort out the adaptation field, if any
  if (start && got_PCR)
  {
    // This is the start of the data, and we have a PCR value to output,
    // so we know we have an adaptation field
    controls = 0x30;  // adaptation field control = '11' = both
    *(TS_packet+3) = (byte) (controls | next_continuity_count(pid));
    // And construct said adaptation field...
    *(TS_packet + 4)   = 7; // initial adaptation field length
    *(TS_packet + 5)  = 0x10;  // flag bits 0001 0000 -> got PCR
    *(TS_packet + 6)   = (byte)   (PCR_base >> 25);
    *(TS_packet + 7)  = (byte)  ((PCR_base >> 17) & 0xFF);
    *(TS_packet + 8)  = (byte)  ((PCR_base >>  9) & 0xFF);
    *(TS_packet + 9)  = (byte)  ((PCR_base >>  1) & 0xFF);
    *(TS_packet + 10) = (byte) (((PCR_base & 0x1) << 7) | 0x7E | (PCR_extn >> 8));
    *(TS_packet + 11) = (byte)  (PCR_extn >>  1);
    TS_hdr_len = 12;
    space_left = MAX_TS_PAYLOAD_SIZE - 8;
    got_adaptation_field = TRUE;

//	PSIDebug ("TS_packet start w PCR\n");	
  }
  else if (pes_data_len < MAX_TS_PAYLOAD_SIZE)
  {
    // Our data is less than 184 bytes long, which means it won't fill
    // the payload, so we need to pad it out with an (empty) adaptation
    // field, padded to the appropriate length

//	PSIDebug ("TS_packet start pes_data write\n");	
	controls = 0x30;  // adaptation field control = '11' = both
    *(TS_packet + 3 ) = (byte)(controls | next_continuity_count(pid));
    if (pes_data_len == (MAX_TS_PAYLOAD_SIZE - 1))  // i.e., 183
    {
      *(TS_packet + 4) = 0; // just the length used to pad
      TS_hdr_len = 5;
      space_left = MAX_TS_PAYLOAD_SIZE - 1;
    }
    else
    {
      *(TS_packet + 4) = 1; // initial length
      *(TS_packet + 5) = 0;  // unset flag bits
      TS_hdr_len = 6;
      space_left = MAX_TS_PAYLOAD_SIZE - 2;  // i.e., 182
    }
    got_adaptation_field = TRUE;
  }
  else
  {
    // The data either fits exactly, or is too long and will need to be
    // continued in further TS packets. In either case, we don't need an
    // adaptation field
    controls = 0x10;  // adaptation field control = '01' = payload only
    *(TS_packet + 3) = (byte)(controls | next_continuity_count(pid));
    TS_hdr_len = 4;
    space_left = MAX_TS_PAYLOAD_SIZE;
//	PSIDebug ("ts packet adaptation \n");
  }

  if (got_adaptation_field)
  {
    // Do we need to add stuffing bytes to allow for short PES data?
    if (pes_data_len < space_left)
    {
      int ii;
      int padlen = space_left - pes_data_len;
      for (ii = 0; ii < padlen; ii++)
        *(TS_packet + (TS_hdr_len+ii)) = 0xFF;
      *(TS_packet + 4 ) += padlen;
      TS_hdr_len   += padlen;
      space_left   -= padlen;
    }
  }	

  if (pes_data_len == space_left)
  {
    // Our data fits exactly
	// last packet of the frame, need end marker here : ghryu
    
	//if ( f_sEnd == 2)  
	ctx->end_marker = 1; 
	
	err = write_TS_packet_parts(ctx,
                                TS_packet,TS_hdr_len,
                                pes_hdr,pes_hdr_len,
                                data,data_len,
                                pid,got_PCR,(PCR_base*300)+PCR_extn);

    if (err) 
    {
		PSIDebug ("ts.c : TS_packet meets end marker err"); 
		return err;
    }	
  }
  else
  {
    ctx->increment = space_left - pes_hdr_len;  
	
	ctx->end_marker = 0; 
	
	//PSIDebug ("write_TS_packet_parts In\n");
	err = write_TS_packet_parts(ctx,
                                TS_packet,TS_hdr_len,
		                        pes_hdr,pes_hdr_len,
                                data,ctx->increment,
                                pid,got_PCR,(PCR_base*300)+PCR_extn);
	//PSIDebug ("write_TS_packet_parts Out\n");
    if (err) 
    {
		PSIDebug ("ts.c : TS_packet meets in the middle of frame write err"); 
		return err;
    }	
    
	ctx->data_length = data_len; 	
    ctx->cur_data = data; 
   
  }  
  return 0;
}

/** 
 	Write out our ES data as a Transport Stream PES packet, with PTS and/or DTS
 	if we've got them, and some attempt to write out a sensible PCR.
 	If the data to be written is more than 65535 bytes long (i.e., the
 	length will not fit into 2 bytes), then the PES packet written will
 	have PES_packet_length set to zero (see ISO/IEC 13818-1 (H.222.0)
 	2.4.3.7, Semantic definitions of fields in PES packet). This is only
 	allowed for video streams.

 	\param id [IN] object id 
 	\param data [IN] ES data 
	\param data_len [IN] data_length 
	\parma pid[IN] it is the PID to use for this TS packet
 	\param stream_id [IN] it is the PES packet stream id to use (e.g.,
     DEFAULT_VIDEO_STREAM_ID)
 	\parma got_pts [IN] it is TRUE if we have a PTS value, in which case
     `pts` is said PTS value
 	\param got_dts [IN] it is TRUE if we also have DTS, in which case
     'dts` is said DTS value.

	\Returns 0 if it worked, 1 if something went wrong.
*/
extern int write_ES_as_TS_PES_packet_with_pts_dts(
												  void 			*id, 													  
                                                  byte        data[],
                                                  int         dataAddrType,
                                                  uint32_t    data_len,
                                                  uint32_t    pid,
                                                  byte        stream_id,
                                                  int         got_pts,
                                                  uint64_t    pts,
                                                  int         got_dts,
                                                  uint64_t    dts)
                                                  //int 		  f_sEnd)
{
	byte    pes_hdr[TS_PACKET_SIZE];  // better be more than long enough!
	int     pes_hdr_len = 0;

	int 	this_is_first = 1; 
	int 	err = 0; 

	int 	l_idx = 0; 
	int 	idx = 0; 

	TS_CTX_S		*ctx; 

	ctx =(TS_CTX_S*)id; 
#ifdef ADD_RTP_DELAY
    ctx->actual_data_length = 0;
    ctx->cnt_ts = 0;
#endif

	byte *encryptedData = NULL;
	int befCTX_hdcp = 0;
	befCTX_hdcp = ctx->hdcpEnabled;

	if( dataAddrType == 2 )
		ctx->hdcpEnabled = 0;
	
	if (ctx->es_type == ES_AUDIO) {
		PES_A_header(data_len,stream_id,got_pts,pts,0,0,pes_hdr,&pes_hdr_len, ctx);
		encryptedData = g_aOutputbuffer;
            if (ctx->codecType == 0) {
                //PSIDebug("Codec Type : L-PCM Codec.");
            }
            else if (ctx->codecType == 1) {
                //PSIDebug("Codec Type : AAC Codec.");
            }
        
	}
	else if (ctx->es_type == ES_VIDEO) {
		PES_header(data_len,stream_id,got_pts,pts,0,0,pes_hdr,&pes_hdr_len, ctx);
		encryptedData = g_vOutputbuffer;
	}

#ifdef ENABLE_WFD_HDCP
	if (ctx->hdcpEnabled)
	{
		//PSIDebug("Encrypting data of len:%u, strCtr:%lu inCtr:%llu ", data_len, ctx->mStrmCounter, ctx->mInputCounter);
		int ret = EncryptFrame(ctx->hdcp_ctx, data, dataAddrType, data_len, ctx->mStrmCounter, &(ctx->mInputCounter), encryptedData );
		if (ret < 0)
		{
			PSIDebug("EncryptFrame failed:%d", ret);
			return 1;
		}
		++(ctx->mStrmCounter);
	}
#endif
		
	ctx->data_length = data_len;

	if( ctx->hdcpEnabled ){
		ctx->cur_data =	encryptedData;
	}
	else{
		ctx->cur_data = data;   
	}
	ctx->increment = 0; 
	ctx->hdcpEnabled = befCTX_hdcp;

	if ( this_is_first ) 
	{
#ifdef ADD_RTP_DELAY
        ctx->actual_data_length = data_len;
        //PSIDebug("TS_CTX actual size = %d", ctx->actual_data_length);
        ctx->cnt_ts = 0;
#endif
		err = write_some_TS_PES_packet(ctx,pes_hdr,pes_hdr_len,
							  ctx->cur_data,ctx->data_length,TRUE,TRUE,pid,stream_id,
							  0,0,0);//, f_sEnd); 	
		 this_is_first = 0; 
		 if ( err ) 
		 {
			PSIDebug("ERROR : write_some_TS_PES_packet ===> 1"); 
			return err; 
		 }	
	
		 if (ctx->end_marker) 
		{		
			this_is_first = 1; 
			ctx->increment = 0; 
			ctx->data_length = 0; 
			ctx->cur_data = NULL; 
			return 0;
			//break;
		}
	}
	
	while (ctx->data_length - ctx->increment > 0) 	
	{
		err = write_some_TS_PES_packet(ctx,NULL,0,
                     &(ctx->cur_data[ctx->increment]),ctx->data_length - ctx->increment,
                     FALSE,FALSE,pid,stream_id,FALSE,0,0);//, f_sEnd);

		if (err) 
		{
			PSIDebug("ERROR : write_some_TS_PES_packet ===> 2"); 
			return err;
		}			

		if (ctx->end_marker) 
		{		
			this_is_first = 1; 
			ctx->increment = 0; 
			ctx->data_length = 0; 
			ctx->cur_data = NULL; 
			//return 0; 
			break;
		}

	}
	return 0; 
}

/** 
	Write out our ES data as a Transport Stream PES packet, with PCR
	This is only allowed for video streams.

	\param id [IN] object id 
	\param data [IN] ES data 
	\param data_len [IN] data_length 
	\parma pid[IN] it is the PID to use for this TS packet
	\param stream_id [IN] it is the PES packet stream id to use (e.g.,
	DEFAULT_VIDEO_STREAM_ID)
	\param pcr_base [IN]  PCR base data 
	\param pcr_extn [IN]  PCR extension data 
	
	\Returns 0 if it worked, 1 if something went wrong.
*/
extern int write_ES_as_TS_PES_packet_with_pcr(void 	*id,
                                              byte        data[],
                                              int         dataAddrType,
                                              uint32_t    data_len,
                                              uint32_t    pid,
                                              byte        stream_id,
                                              uint64_t    pcr_base,
                                              uint32_t    pcr_extn)
                                              //int f_sEnd)
{
	byte    pes_hdr[TS_PACKET_SIZE];  // better be more than long enough!
	int     pes_hdr_len = 0;
	int 	this_is_first = 1; 
	int 	err = 0; 
	int 	i = 0; 
	int 	l_idx = 0; 
	int 	idx = 0;

	byte *encryptedData = NULL;
	int befCTX_hdcp = 0;
	
	TS_CTX_S		*ctx;
	ctx = (TS_CTX_S*)id;

#ifdef ADD_RTP_DELAY
    ctx->actual_data_length = 0;
    ctx->cnt_ts = 0;
#endif


	befCTX_hdcp = ctx->hdcpEnabled;

	if( dataAddrType == 2 )
		ctx->hdcpEnabled = 0;
	
	if (ctx->es_type == ES_AUDIO) {
		PES_A_header(data_len,stream_id,1,pcr_base,0,0,pes_hdr,&pes_hdr_len, ctx);
		encryptedData = g_aOutputbuffer;
	}
	else if (ctx->es_type == ES_VIDEO) {
		PES_header(data_len,stream_id,1,pcr_base,0,0,pes_hdr,&pes_hdr_len, ctx);
		encryptedData = g_vOutputbuffer;
	}
	
#ifdef ENABLE_WFD_HDCP
	if (ctx->hdcpEnabled)
	{
		//PSIDebug("Encrypting data of len:%u, strCnt:%lu inCtr:%llu - 0x%x", data_len, ctx->mStrmCounter, ctx->mInputCounter, ctx->mInputCounter);
		int ret = EncryptFrame(ctx->hdcp_ctx, data, dataAddrType, data_len, ctx->mStrmCounter, &(ctx->mInputCounter), encryptedData);
		if (ret < 0)
		{
			PSIDebug("EncryptFrame failed:%d", ret);
			return 1;
		}
		++(ctx->mStrmCounter);
	}
#endif
	//PSIDebug (" after PES_header with pcr pes_hdr_len = %d", pes_hdr_len); 

	ctx->data_length = data_len; 
	if (ctx->hdcpEnabled) {
	    ctx->cur_data = encryptedData;
	}else{
	    ctx->cur_data = data; 
	}
	ctx->increment = 0; 
	
	ctx->hdcpEnabled = befCTX_hdcp;

	
	if ( this_is_first ) 
	{
#ifdef ADD_RTP_DELAY
        ctx->actual_data_length = data_len;
        //PSIDebug("TS_CTX actual size = %d", ctx->actual_data_length);
        ctx->cnt_ts = 0;
#endif
		err = write_some_TS_PES_packet(ctx,pes_hdr,pes_hdr_len,
							ctx->cur_data,ctx->data_length,TRUE,TRUE,pid,stream_id,
							TRUE, pcr_base, pcr_base * 300);	
		this_is_first = 0; 
		if (err) 
		{
			PSIDebug("ERROR : write_some_TS_PES_packet ===> 3"); 
			return err;
		}	
			
		if (ctx->end_marker)
		{		
			this_is_first = 1; 
			ctx->increment = 0; 
			ctx->data_length = 0; 
			ctx->cur_data = NULL; 
			return 0;
		}
	}

	while (ctx->data_length- ctx->increment > 0)
	{
		err = write_some_TS_PES_packet(ctx,NULL,0,
                     &(ctx->cur_data[ctx->increment]),ctx->data_length- ctx->increment,
                     FALSE,FALSE,pid,stream_id,FALSE,0,0);
		if (err) 
		{
			PSIDebug("ERROR : write_some_TS_PES_packet ===> 4"); 
			return err;
		}	

		if (ctx->end_marker) 
		{		
			this_is_first = 1; 
			ctx->increment = 0; 
			ctx->data_length = 0; 
			ctx->cur_data = NULL; 
			return 0;
		}
	}	
	return 0; 
			
}

/** 
  	Construct a Transport Stream packet header 
	The data is required to fit within a single TS packet - i.e., to be
	183 bytes or less.
	\param PID [IN] it is the PID to use for this packet.
 	\param data_len [IN] it is the length of the PAT or PMT data
 	\param TS_hdr [IN] it is a byte array into (the start of) which to write the TS header data.
	\param TS_hdr_len [IN] how much data we've written therein.
 
    \Returns 0 if it worked, 1 if something went wrong.
*/
static int TS_program_packet_hdr(uint32_t pid,
                                 int      data_len,
                                 byte     TS_hdr[TS_PACKET_SIZE],
                                 int     *TS_hdr_len)
{
  uint32_t controls = 0;
  int     pointer, ii;

  if (data_len > (TS_PACKET_SIZE - 5))  // i.e., 183
  {
    //;//fprintf(stderr,"### PMT/PAT data for PID %02x is too long (%d > 183)",            pid,data_len);
    return 1;
  }
  
  // We always start with a sync_byte to identify this as a
  // Transport Stream packet

  TS_hdr[0] = 0x47;
  // We want the "payload_unit_start_indicator" bit set
  TS_hdr[1] = (byte)(0x40 | ((pid & 0x1f00) >> 8));
  TS_hdr[2] = (byte)(pid & 0xff);
  // no adaptation field controls
  //controls = 0x01;
  
  //TS_hdr[3] = (byte)(controls | next_continuity_count(pid));
  TS_hdr[3] = 0x10; 

  TS_hdr[4] = 0x00; 		

  *TS_hdr_len = 5;

  return 0;
}
/** 
  	calling API for making PAT or PMT data.
	The data is required to fit within a single TS packet - i.e., to be
	183 bytes or less.
	\param PID [IN] it is the PID to use for this packet.
 	\param data_len [IN] it is the length of the PAT or PMT data
 	\param TS_hdr [IN] it is a byte array into (the start of) which to write the TS header data.
	\param TS_hdr_len [IN] how much data we've written therein.
 
    \Returns 0 if it worked, 1 if something went wrong.
*/
extern int write_TS_program_data3(void 	 *id, 
									  uint32_t	  transport_stream_id,
									  uint32_t	  program_number,
									  uint32_t	  pmt_pid,
									  uint32_t	  pcr_pid,
									  int		  num_progs)
									  /*		
									  , 
									  uint32_t	  prog_pid[],
									  byte		  prog_type[])
									  */

{
	int					err;	
	
	err = write_pat1(id, transport_stream_id);
	if (err) return 1;
	err = write_pmt1(id,pmt_pid);
	if (err) return 1;

	if (f_rtcp) 	
	{
                PSIDebug("starting RTCP timer");
		// RTCP enabling
		startServerRTCPTimer();
		f_rtcp = 0; 
	}

	return 0;

}

/** 
  	Construct a pat 
	The data is required to fit within a single TS packet - i.e., to be
	183 bytes or less.
	\param PID [IN] it is the PID to use for this packet.
 	\param transport_stream_id [IN] it is the stream id for delivering pat 
    \Returns 0 if it worked, 1 if something went wrong.
*/
extern int write_pat1(void 	*id,
		     uint32_t       transport_stream_id)

{
	int			ii;
	byte     	data[1021+3];
	char 	   *TS_packet; 

	int      TS_hdr_len;
	int      err;
	int      section_length;
	int      offset, data_length;
	uint32_t crc32 = 0;


	TS_CTX_S		*ctx; 

	ctx = (TS_CTX_S*)id; 

    //===================================================================================>
    // Create teh Header for PAT (Program Association Table)

	section_length = 9 + 4;//+ 4 * 1;
	if (section_length > 1021)
	{
		PSIDebug (" write pat1 : invalid section_length "); 
		return 1;
	}

        // 8 bit : Table ID (PAT (0x00), PMT (0x02))
	data[0] = 0x00;
    
	// The section length is fixed because our data is fixed
    // 0xb0 -> 1011 (SECTION LENGTH) 
    //  4bit : 1->section_syntax_ind, x-> (0 -> When PAT), 11-> Reserved. 
    //  12bit : Section Length (13) -> Informs how many programs are listed below by specifying the number of bytes of this section, 
    //                                              starting immediately following this field and including the CRC. First two bits must be zero. 
    //             --> 3 byte (table_id + section_syntax_ind + reserved + section length) + 13 byte (Other Info.)
	data[1] = (byte) (0xb0 | ((section_length & 0x0F00) >> 8));
	data[2] = (byte) (section_length & 0x0FF);

    // 
    // 16 bit : Network_IDdks
	data[3] = 0x00; 
	data[4] = 0x01; 
    
	// For simplicity, we'll have a version_id of 0
    // 0xc1 -> 11 00000 1
    // 2bit : '11' is configured. (Reserved ?)
    // 5 bit : '00000' is version ID (0). 
    // 1 bit : current_next_ind
	data[5] = 0xc1;
    
    // First section of the PAT has section number 0, and there is only that section
    // 8 bit : section_no
    // 8 bit : last section_no
	data[6] = 0x00;
	data[7] = 0x00;

	offset = 8;

    // 16bit : Program Num
	data[offset+0] = (byte) (1 & 0xFF00) >> 8;
	data[offset+1] = (byte) (1 & 0x00FF);

    // 16bit : Reserved. 
    // 0xE0 -> 0x111x 
    // 3 bit : Always set to binary '111'.
    // 13 bit : Packets with this PID are assumed to be PMT tables. 
	data[offset+2] = (byte) (0xE0 | (( 0x100& 0x1F00) >> 8)); // setting PMT_PID
	data[offset+3] = (byte) (0x100 & 0x00FF);
	
    // Create the CRC block. 
    // 32 bit : CRC information. 
	offset += 4;
	crc32 = crc32_block(0xffffffff,data,offset);
	data[12] = (byte) ((crc32 & 0xff000000) >> 24);
	data[13] = (byte) ((crc32 & 0x00ff0000) >> 16);
	data[14] = (byte) ((crc32 & 0x0000ff00) >>  8);
	data[15] = (byte)  (crc32 & 0x000000ff);
	data_length = offset+4;
    //===================================================================================>

    //===================================================================================>
    // Provide the empty packet buffer for A/V synchronization. 
	if ((ctx->wfd_idx % 0x07) == 0) 
	{ 	
		ctx->buf0_ptr= getWFD_BufPtr(0);	
		ctx->pack_start = ctx->buf0_ptr +  RTP_HEADER_SIZE; 
		ctx->wfd_idx = 0; 
	}	

	TS_packet = ctx->pack_start + (TS_PACKET_SIZE * ctx->wfd_idx);  
	ctx->end_marker = 1; 

    // Construct a Transport Stream Packet Header. 
	err = TS_program_packet_hdr(0x00,data_length,TS_packet,&TS_hdr_len);
	if (err)
	{
		PSIDebug ("### Error constructing PAT packet header");
		return 1;
	}

	for ( ii = 0; ii < 188 - TS_hdr_len - data_length ; ii ++) 
		data[data_length + ii] = 0xff; 

	data_length = 188 - TS_hdr_len; 
	
	err = write_TS_packet_parts(ctx,TS_packet,TS_hdr_len,NULL,0,
	                      data,data_length,0x00,FALSE,0);
	if (err)
	{
		PSIDebug("### Error writing PAT");
		return err;
	}

    //===================================================================================>
	return 0;
}

/** 
  	Construct a pmt 
	The data is required to fit within a single TS packet - i.e., to be
	183 bytes or less.
	\param PID [IN] it is the PID to use for this packet.
 	\param pmt_id [IN] it is the stream id for delivering pmt
    \Returns 0 if it worked, 1 if something went wrong.
*/

extern int write_pmt1(void  *id,
		     uint32_t    pmt_pid)

{
	int      	ii;
	byte     	data[3+1021];	// maximum PMT size
	char		*TS_packet; 
	int      	TS_hdr_len;
	int      	err;
	int      	section_length;
	int      	offset, data_length;
	uint32_t 	crc32;
	uint32_t 	pid, len; 
	
	TS_CTX_S	*ctx; 

	ctx = (TS_CTX_S*)id; 

	section_length = 9 + 10 + 4;
	
	if (ctx->hdcpEnabled)
	    section_length += HDCP_REGISTRATION_DESCRIPTOR_LENGTH;
		
	if (section_length > 1021)
	{
		PSIDebug("### PMT data is too long - will not fit in 1021 bytes ");
	
		return 1;
	}

	data[0] = 0x02;
	data[1] = (byte) (0xb0 | ((section_length & 0x0F00) >> 8));
	data[2] = (byte) (section_length & 0x0FF);
	data[3] = (byte) (1 & 0xFF00) >> 8;
	data[4] = (byte) (1 & 0x00FF);
	data[5] = 0xc1;
	data[6] = 0x00; // section number
	data[7] = 0x00; // last section number	
	
	data[8] = (byte) (0xE0 | ((0x1011& 0x1F00) >> 8) );
	data[9] = (byte) (0x1011 & 0x00FF);
	data[10] = 0xF0;
	data[11] = (byte)0x00;
	offset = 12;

	if (ctx->hdcpEnabled)
	{
		data[11] = (byte)HDCP_REGISTRATION_DESCRIPTOR_LENGTH;
		data[offset+0] = 0x05; //registration_descriptor()
		data[offset+1] = 0x05; //length field.
		data[offset+2] = 'H';
		data[offset+3] = 'D';
		data[offset+4] = 'C';
		data[offset+5] = 'P';
		data[offset+6] = 0x20; //HDCP_version
		offset += 7;
	}

	for (ii=0; ii < 2; ii++)
	{
		if ( ii == 0 ) 
		{	
			//offset = 19 ;
			pid =  0x1011; //default video pid : H264
			len = 0;
			data[offset+0] = 0x1b;
			data[offset+1] = (byte) (0xE0 | ((0x1011 & 0x1F00) >> 8));
			data[offset+2] = (byte) (0x1011 & 0x00FF);
			data[offset+3] = ((len & 0xFF00) >> 8) | 0xF0;
			data[offset+4] =   len & 0x00FF;
			//memcpy(data+offset+5,g_pmt.streams[ii].ES_info,len);
			offset += 5 + len;
		}else if ( ii == 1) 
		{
			//offset = 24;
			pid = 0x1100;	//default audio pid :LPCM
			len = 0;

//for temp 
			//ctx->codecType = 1; 

                        if (ctx->codecType == 0) {
		   	    data[offset+0] = 0x83; //LPCM
                        } else if (ctx->codecType == 1) {
			    data[offset+0] = 0x0F; //AAC
                        }
			data[offset+1] = (byte) (0xE0 | ((0x1100 & 0x1F00) >> 8));
			data[offset+2] = (byte) (0x1100 & 0x00FF);
			data[offset+3] = ((len & 0xFF00) >> 8) | 0xF0;
			data[offset+4] =   len & 0x00FF;
			//memcpy(data+offset+5,g_pmt.streams[ii].ES_info,len);
			offset += 5 + len;
		}
	}

	crc32 = crc32_block(0xffffffff,data,offset);
	data[offset+0] = (byte) ((crc32 & 0xff000000) >> 24);
	data[offset+1] = (byte) ((crc32 & 0x00ff0000) >> 16);
	data[offset+2] = (byte) ((crc32 & 0x0000ff00) >>  8);
	data[offset+3] = (byte)  (crc32 & 0x000000ff);
	data_length = offset + 4;

	if (data_length != section_length + 3)
	{
		PSIDebug("### PMT length %d, section length+3 %d ",
	    data_length,section_length+3);
		return 1;
	}

	crc32 = crc32_block(0xffffffff,data,data_length);
	if (crc32 != 0)
	{
		PSIDebug("### PMT CRC does not self-cancel ");
		return 1;
	}

	if ( (ctx->wfd_idx % 0x07) == 0) 
	{ 	
		ctx->buf0_ptr= getWFD_BufPtr(0);	 
		ctx->pack_start = ctx->buf0_ptr +  RTP_HEADER_SIZE; 
		ctx->wfd_idx = 0; 
	}	

	TS_packet = ctx->pack_start + (TS_PACKET_SIZE * ctx->wfd_idx);  
	ctx->end_marker = 1; 

	err = TS_program_packet_hdr(pmt_pid,data_length,TS_packet,&TS_hdr_len);
	if (err)
	{
		PSIDebug("### Error constructing PMT packet header ");
		return err;
	}

	for ( ii = 0; ii < 188 - TS_hdr_len - data_length ; ii ++) 
		data[data_length + ii] = 0xff; 

	data_length = 188 - TS_hdr_len; 
	
	err = write_TS_packet_parts(ctx,TS_packet,TS_hdr_len,NULL,0,
	                          data,data_length,0x02,FALSE,0);
	if (err)
	{
		PSIDebug("### Error writing PMT ");
		return err;
	}

	return 0;
}

void setHDCPmode(void* id, int* tData)
{
	TS_CTX_S		*ctx; 

	ctx =(TS_CTX_S*)id; 
	if( ctx->es_type == ES_AUDIO ){
		ctx->hdcpEnabled = 0;
	}
	else{
		ctx->hdcpEnabled = *tData;
	}
	PSIDebug("set %s HDCP mode:%d", (ctx->es_type?"ES_AUDIO":"ES_VIDEO"), ctx->hdcpEnabled);
}

void setAudioCodecType(void* id, int codecType)
{
	TS_CTX_S		*ctx; 

	ctx =(TS_CTX_S*)id; 
	if( ctx->es_type == ES_AUDIO ){
		ctx->codecType = codecType;
	}
}



/**
	Write out a Transport Stream Null packet.
	\param id [IN] object id 

	Returns 0 if it worked, 1 if something went wrong.
*/
extern int write_TS_null_packet(void *id)
{
  char 	*TS_packet; 
  int    err, ii;

  TS_CTX_S			*ctx; 

  ctx = (TS_CTX_S*)id; 

  // write data directly to RTP buffer 
  if ( (ctx->wfd_idx % 0x07) == 0) 
  {   
	  ctx->pack_start = NULL; 
	  
	  ctx->buf0_ptr= getWFD_BufPtr(0);	
	  ctx->pack_start = ctx->buf0_ptr +  RTP_HEADER_SIZE; 
	  ctx->wfd_idx = 0; 

  }	

  TS_packet = ctx->pack_start + (TS_PACKET_SIZE * ctx->wfd_idx); 


  *(TS_packet)  = 0x47;
  *(TS_packet+1) = 0x1F;  // PID is 0x1FFF
  *(TS_packet+2) = 0xFF;
  *(TS_packet+3) = 0x01;  // payload only
  for (ii=4; ii<TS_PACKET_SIZE; ii++)
     *(TS_packet+ii) = 0xFF;

  err = write_TS_packet_parts(ctx,TS_packet,TS_PACKET_SIZE,NULL,0,NULL,0,
                              0x1FF,FALSE,0);
  if (err)
  { 
    PSIDebug (" err:  writing null TS packet\n");
    return err;
  }
  return 0;
}

/**
	Call RTP initialization module. 
	\param localPort [IN] local port 
	\param primaryPort [IN] primary port
 	\param secondaryPort [IN] secondary port
 	\param remote IP [IN] remote IP 

	Returns 0 if it worked, 1 if something went wrong.
*/
extern int set_RTPinit (unsigned int localPort, unsigned int primaryPort, unsigned int secondaryPort, char *remoteIP
#ifdef RTCP_RR
						, RTCP_Noti_sig aNotiCallback, void *transArg
#endif
						) {

	int ret = 0; 
	ret = setRTP_Params(localPort, primaryPort
						, secondaryPort, remoteIP
#ifdef RTCP_RR
						, aNotiCallback, transArg
#endif
						);
	return ret; 
}

#ifdef BUFFER_CONTROL
/**
	Call RTP socket buffer size change module. 
	Returns 0 if it worked, 1 if something went wrong.
*/
extern int change_TS_RTPSocketBufferSize(int aIncrease)
{
	int ret = 0; 
	ret = ChangeRTP_Socket_Buffer_Size(aIncrease); 
	return ret; 
}
#endif

/**
	Call RTP deinitialization module. 
	Returns 0 if it worked, 1 if something went wrong.
*/
extern int set_RTPdeinit ()
{
	int ret = 0; 
	ret = ClearRTP_Params(); 
	return ret; 
}

#ifdef ENABLE_WFD_HDCP
extern void set_HDCPctx_to_TSctx( void *hdcp, void *id )
{
	TS_CTX_S *ts_ctx = (TS_CTX_S*)id;
	ts_ctx->hdcp_ctx  = hdcp;
}
#endif

