/*
 * Functions for working with H.222 Transport Stream packets - in particular,
 * for writing PES packets.
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

#ifndef _ts_fns
#define _ts_fns

#include "compat.h"
//#include "h222_defns.h"
//#include "tswrite_defns.h"
//#include "pidint_defns.h"
#include "ts_defns.h"


#include "PSIApi.h"


#include "misc_fns.h"

#ifdef RTCP_RR
#include "rtp_api.h"
#endif


// ============================================================
// Writing a Transport Stream
// ============================================================

#ifdef ENABLE_WFD_HDCP
extern void set_HDCPctx_to_TSctx( void* hdcp, void *id );
#endif


extern void* ts_ctx_create(int type, HObject mutex); 
extern int ts_ctx_delete(void *id); 

// little endian to big endia
extern int swap_data (char *data,  unsigned int data_len);  


/*
 * Write out a Transport Stream PAT and PMT.
 * 
 * - `output` is the TS output context returned by `tswrite_open`
 * - `transport_stream_id` is the id for this particular transport stream.
 * - `program_number` is the program number to use for the PID.
 * - `pmt_pid` is the PID for the PMT.
 * - `pid` is the PID of the stream to enter in the tables. This is also
 *    used as the PCR PID.
 * - `stream_type` is the type of stream. MPEG-2 video is 0x01,
 *   MPEG-4/AVC (H.264) is 0x1b.
 *
 * Since we're outputting a TS representing a single ES, we only need to
 * support a single program stream, containing a single PID.
 *
 * Returns 0 if it worked, 1 if something went wrong.
 */

extern int write_TS_program_data3(void 	*id,
                                  uint32_t    transport_stream_id,
                                  uint32_t    program_number,
                                  uint32_t    pmt_pid,
                                  uint32_t    pcr_pid,
                                  int         num_progs); 
/*
 * Write out a Transport Stream PAT.
 * 
 * - `output` is the TS output context returned by `tswrite_open`
 * - `transport_stream_id` is the id for this particular transport stream.
 * - `prog_list` is a PIDINT list of program number / PID pairs.
 *
 * Returns 0 if it worked, 1 if something went wrong.
 */
extern int write_pat1(void	    *id,
		     uint32_t       transport_stream_id);

/*
 * Write out a Transport Stream PMT, given a PMT datastructure
 * 
 * - `output` is the TS output context returned by `tswrite_open`
 * - `pmt_pid` is the PID for the PMT.
 * - 'pmt' is the datastructure containing the PMT information
 *
 * Returns 0 if it worked, 1 if something went wrong.
 */
extern int write_pmt1(void *id,
		     uint32_t    pmt_pid);

/*
 * Write out our ES data as a Transport Stream PES packet, with PTS and/or DTS
 * if we've got them, and some attempt to write out a sensible PCR.
 *
 * - `output` is the TS output context returned by `tswrite_open`
 * - `data` is our ES data (e.g., a NAL unit)
 * - `data_len` is its length
 * - `pid` is the PID to use for this TS packet
 * - `stream_id` is the PES packet stream id to use (e.g.,
 *    DEFAULT_VIDEO_STREAM_ID)
 * - `got_pts` is TRUE if we have a PTS value, in which case
 * - `pts` is said PTS value
 * - `got_dts` is TRUE if we also have DTS, in which case
 * - `dts` is said DTS value.
 *
 * We also want to try to write out a sensible PCR value.
 *
 * PTS can go up as well as down (it is the time at which the next frame
 * should be presented to the user, but frames do not necessarily occur
 * in presentation order).
 *
 * DTS only goes up, since it is the time that the frame should be decoded.
 *
 * Thus, if we have it, the DTS is sensible to use for the PCR...
 *
 * If the data to be written is more than 65535 bytes long (i.e., the
 * length will not fit into 2 bytes), then the PES packet written will
 * have PES_packet_length set to zero (see ISO/IEC 13818-1 (H.222.0)
 * 2.4.3.7, Semantic definitions of fields in PES packet). This is only
 * allowed for video streams.
 *
 * Returns 0 if it worked, 1 if something went wrong.
 */
extern int write_ES_as_TS_PES_packet_with_pts_dts(void		*id,
                                                  byte        data[],
                                                  int         dataAddrType,
                                                  uint32_t    data_len,
                                                  uint32_t    pid,
                                                  byte        stream_id,
                                                  int         got_pts,
                                                  uint64_t    pts,
                                                  int         got_dts,
                                                  uint64_t    dts);
												  
extern void setHDCPmode(void* id, int* tData);

extern void setAudioCodecType(void* id, int codecType);

/*
 * Write out our ES data as a Transport Stream PES packet, with PCR.
 *
 * - `output` is the TS output context returned by `tswrite_open`
 * - `data` is our ES data (e.g., a NAL unit)
 * - `data_len` is its length
 * - `pid` is the PID to use for this TS packet
 * - `stream_id` is the PES packet stream id to use (e.g.,
 *    DEFAULT_VIDEO_STREAM_ID)
 * - `pcr_base` and `pcr_extn` encode the PCR value.
 *
 * If the data to be written is more than 65535 bytes long (i.e., the
 * length will not fit into 2 bytes), then the PES packet written will
 * have PES_packet_length set to zero (see ISO/IEC 13818-1 (H.222.0)
 * 2.4.3.7, Semantic definitions of fields in PES packet). This is only
 * allowed for video streams.
 *
 * Returns 0 if it worked, 1 if something went wrong.
 */
extern int write_ES_as_TS_PES_packet_with_pcr(void *id,
                                              byte        data[],
                                              int         dataAddrType,
                                              uint32_t    data_len,
                                              uint32_t    pid,
                                              byte        stream_id,
                                              uint64_t    pcr_base,
                                              uint32_t    pcr_extn);
/*
 * Write out a Transport Stream Null packet.
 *
 * - `output` is the TS output context returned by `tswrite_open`
 * 
 * Returns 0 if it worked, 1 if something went wrong.
 */
extern int write_TS_null_packet(void *id);

// set RTP init
extern int set_RTPinit (unsigned int localPort, unsigned int primaryPort, unsigned int secondaryPort, char *remoteIP
#ifdef RTCP_RR
						, RTCP_Noti_sig aNotiCallback, void *transArg
#endif
						);

#ifdef BUFFER_CONTROL
extern int change_TS_RTPSocketBufferSize(int aIncrease);
#endif

extern int set_RTPdeinit (); //ygkim
#endif // _ts_fns



// Local Variables:
// tab-width: 8
// indent-tabs-mode: nil
// c-basic-offset: 2
// End:
// vim: set tabstop=8 shiftwidth=2 expandtab:
