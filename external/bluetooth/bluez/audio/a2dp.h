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

#define A2DP_CODEC_SBC			0x00

/* SS_BLUETOOTH(changeon.park) 2012.02.07 */
#ifdef GLOBALCONFIG_BLUETOOTH_APT_X_SUPPORT
#define NON_A2DP_CODEC_APTX		0xFF 
#endif
/* SS_BLUETOOTH(changeon.park) End */

#define A2DP_CODEC_MPEG12		0x01
#define A2DP_CODEC_MPEG24		0x02
#define A2DP_CODEC_ATRAC		0x03

#define SBC_SAMPLING_FREQ_16000		(1 << 3)
#define SBC_SAMPLING_FREQ_32000		(1 << 2)
#define SBC_SAMPLING_FREQ_44100		(1 << 1)
#define SBC_SAMPLING_FREQ_48000		1

/* SS_BLUETOOTH(changeon.park) 2012.02.07 */
#ifdef GLOBALCONFIG_BLUETOOTH_APT_X_SUPPORT
#define APTX_SAMPLING_FREQ_16000	(1 << 3)
#define APTX_SAMPLING_FREQ_32000	(1 << 2)
#define APTX_SAMPLING_FREQ_44100	(1 << 1)
#define APTX_SAMPLING_FREQ_48000	1
#endif
/* SS_BLUETOOTH(changeon.park) End */

#define SBC_CHANNEL_MODE_MONO		(1 << 3)
#define SBC_CHANNEL_MODE_DUAL_CHANNEL	(1 << 2)
#define SBC_CHANNEL_MODE_STEREO		(1 << 1)
#define SBC_CHANNEL_MODE_JOINT_STEREO	1

/* SS_BLUETOOTH(changeon.park) 2012.02.07 */
#ifdef GLOBALCONFIG_BLUETOOTH_APT_X_SUPPORT

/*#define APTX_CHANNEL_MODE_STEREO			2 */
/*
 * aptX codec for Bluetooth only supports stereo mode with value 2
 * But we do have sink devices programmed to send capabilities with other channel mode support.
 * So to handle the case and keeping codec symmetry with SBC etc., we do define other channel mode,
 * and we always make sure to set configuration with APTX_CHANNEL_MODE_STEREO only.
 *
 * */

#define APTX_CHANNEL_MODE_MONO			(1 << 3)
#define APTX_CHANNEL_MODE_DUAL_CHANNEL	(1 << 2)
#define APTX_CHANNEL_MODE_STEREO		(1 << 1)
#define APTX_CHANNEL_MODE_JOINT_STEREO	1


#define APTX_VENDER_ID0		0x4F //APTX codec ID 79
#define APTX_VENDER_ID1		0x0
#define APTX_VENDER_ID2		0x0
#define APTX_VENDER_ID3		0x0

#define APTX_CODEC_ID0		0x1
#define APTX_CODEC_ID1		0x0

#endif
/* SS_BLUETOOTH(changeon.park) End */

#define SBC_BLOCK_LENGTH_4		(1 << 3)
#define SBC_BLOCK_LENGTH_8		(1 << 2)
#define SBC_BLOCK_LENGTH_12		(1 << 1)
#define SBC_BLOCK_LENGTH_16		1

#define SBC_SUBBANDS_4			(1 << 1)
#define SBC_SUBBANDS_8			1

#define SBC_ALLOCATION_SNR		(1 << 1)
#define SBC_ALLOCATION_LOUDNESS		1

#define MPEG_CHANNEL_MODE_MONO		(1 << 3)
#define MPEG_CHANNEL_MODE_DUAL_CHANNEL	(1 << 2)
#define MPEG_CHANNEL_MODE_STEREO	(1 << 1)
#define MPEG_CHANNEL_MODE_JOINT_STEREO	1

#define MPEG_LAYER_MP1			(1 << 2)
#define MPEG_LAYER_MP2			(1 << 1)
#define MPEG_LAYER_MP3			1

#define MPEG_SAMPLING_FREQ_16000	(1 << 5)
#define MPEG_SAMPLING_FREQ_22050	(1 << 4)
#define MPEG_SAMPLING_FREQ_24000	(1 << 3)
#define MPEG_SAMPLING_FREQ_32000	(1 << 2)
#define MPEG_SAMPLING_FREQ_44100	(1 << 1)
#define MPEG_SAMPLING_FREQ_48000	1

#define MAX_BITPOOL 64
#define MIN_BITPOOL 2

#if __BYTE_ORDER == __LITTLE_ENDIAN

struct sbc_codec_cap {
	struct avdtp_media_codec_capability cap;
	uint8_t channel_mode:4;
	uint8_t frequency:4;
	uint8_t allocation_method:2;
	uint8_t subbands:2;
	uint8_t block_length:4;
	uint8_t min_bitpool;
	uint8_t max_bitpool;
} __attribute__ ((packed));

/* SS_BLUETOOTH(changeon.park) 2012.02.07 */
#ifdef GLOBALCONFIG_BLUETOOTH_APT_X_SUPPORT

/* 
First added on APTX code 02-12-2010.
Amended on 14-01-2010 for frequency and channel mode 4-bits each 
*/

struct aptx_codec_cap {
	struct avdtp_media_codec_capability cap;
	uint8_t vender_id0;
	uint8_t vender_id1;
	uint8_t vender_id2;
	uint8_t vender_id3;
	uint8_t codec_id0;
	uint8_t codec_id1;
	uint8_t channel_mode:4;
	uint8_t frequency:4;
}__attribute__ ((packed));

#endif
/* SS_BLUETOOTH(changeon.park) End */

struct mpeg_codec_cap {
	struct avdtp_media_codec_capability cap;
	uint8_t channel_mode:4;
	uint8_t crc:1;
	uint8_t layer:3;
	uint8_t frequency:6;
	uint8_t mpf:1;
	uint8_t rfa:1;
	uint16_t bitrate;
} __attribute__ ((packed));

#elif __BYTE_ORDER == __BIG_ENDIAN

struct sbc_codec_cap {
	struct avdtp_media_codec_capability cap;
	uint8_t frequency:4;
	uint8_t channel_mode:4;
	uint8_t block_length:4;
	uint8_t subbands:2;
	uint8_t allocation_method:2;
	uint8_t min_bitpool;
	uint8_t max_bitpool;
} __attribute__ ((packed));

/* SS_BLUETOOTH(changeon.park) 2012.02.07 */
#ifdef GLOBALCONFIG_BLUETOOTH_APT_X_SUPPORT

/*
* First added -02-12-2010, amended on 14-01-2010 for frequency and channel mode 4-bits each
* the below structure need to be adjusted for __BIG_ENDIAN ness, at present it is same as  __LITTLE_ENDIAN, which is wrong - 9-02-2011
 * which is wrong - 9-02-2011.
 * Frequency and channel mode are reversed compared to Little_Endian@14-02-2012
 */

struct aptx_codec_cap {
	struct avdtp_media_codec_capability cap;
	uint8_t vender_id0;
	uint8_t vender_id1;
	uint8_t vender_id2;
	uint8_t vender_id3;
	uint8_t codec_id0;
	uint8_t codec_id1;
	uint8_t frequency:4;
	uint8_t channel_mode:4;
} __attribute__ ((packed));

#endif
/* SS_BLUETOOTH(changeon.park) End */

struct mpeg_codec_cap {
	struct avdtp_media_codec_capability cap;
	uint8_t layer:3;
	uint8_t crc:1;
	uint8_t channel_mode:4;
	uint8_t rfa:1;
	uint8_t mpf:1;
	uint8_t frequency:6;
	uint16_t bitrate;
} __attribute__ ((packed));

#else
#error "Unknown byte order"
#endif

struct a2dp_sep;


typedef void (*a2dp_select_cb_t) (struct avdtp *session,
					struct a2dp_sep *sep, GSList *caps,
					void *user_data);
typedef void (*a2dp_config_cb_t) (struct avdtp *session, struct a2dp_sep *sep,
					struct avdtp_stream *stream,
					struct avdtp_error *err,
					void *user_data);
typedef void (*a2dp_stream_cb_t) (struct avdtp *session,
					struct avdtp_error *err,
					void *user_data);

int a2dp_register(DBusConnection *conn, const bdaddr_t *src, GKeyFile *config);
void a2dp_unregister(const bdaddr_t *src);

struct a2dp_sep *a2dp_add_sep(const bdaddr_t *src, uint8_t type,
				uint8_t codec, gboolean delay_reporting,
				struct media_endpoint *endpoint, int *err);
void a2dp_remove_sep(struct a2dp_sep *sep);

struct a2dp_sep *a2dp_get(struct avdtp *session, struct avdtp_remote_sep *sep);

unsigned int a2dp_select_capabilities(struct avdtp *session,
					uint8_t type, const char *sender,
					a2dp_select_cb_t cb,
					void *user_data);
unsigned int a2dp_config(struct avdtp *session, struct a2dp_sep *sep,
				a2dp_config_cb_t cb, GSList *caps,
				void *user_data);
unsigned int a2dp_resume(struct avdtp *session, struct a2dp_sep *sep,
				a2dp_stream_cb_t cb, void *user_data);
unsigned int a2dp_suspend(struct avdtp *session, struct a2dp_sep *sep,
				a2dp_stream_cb_t cb, void *user_data);
gboolean a2dp_cancel(struct audio_device *dev, unsigned int id);

gboolean a2dp_sep_lock(struct a2dp_sep *sep, struct avdtp *session);
gboolean a2dp_sep_unlock(struct a2dp_sep *sep, struct avdtp *session);
gboolean a2dp_sep_get_lock(struct a2dp_sep *sep);
struct avdtp_stream *a2dp_sep_get_stream(struct a2dp_sep *sep);
struct a2dp_sep *a2dp_get_sep(struct avdtp *session,
				struct avdtp_stream *stream);
gboolean a2dp_read_edrcapability( bdaddr_t *src, bdaddr_t *dst);
