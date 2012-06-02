/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2006-2007  Nokia Corporation
 *  Copyright (C) 2004-2008  Marcel Holtmann <marcel@holtmann.org>
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdint.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/prctl.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>

/* SS_BLUETOOTH(is80.hwang) 2012.02.10 */
// to switch role to Master
#ifdef GLOBALCONFIG_BLUETOOTH_USE_CSR
#include <bluetooth/hci.h>
#endif
/* SS_BLUETOOTH(is80.hwang) End */
#include "ipc.h"
#include "sbc.h"
/* SS_BLUETOOTH(changeon.park) 2012.02.07 */
#ifdef GLOBALCONFIG_BLUETOOTH_APT_X_SUPPORT
#include "aptXbtenc.h" //header file for btaptx
#endif
/* SS_BLUETOOTH(changeon.park) End */
#include "rtp.h"
#include "liba2dp.h"

#define LOG_NDEBUG 0
#define LOG_TAG "A2DP"
#include <utils/Log.h>

#define ENABLE_DEBUG
/* #define ENABLE_VERBOSE */
/* #define ENABLE_TIMING */

#define BUFFER_SIZE 2048

#ifdef ENABLE_DEBUG
#define DBG LOGD
#else
#define DBG(fmt, arg...)
#endif

#ifdef ENABLE_VERBOSE
#define VDBG LOGV
#else
#define VDBG(fmt, arg...)
#endif

#ifndef MIN
# define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#ifndef MAX
# define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

#define MAX_BITPOOL 64
#define MIN_BITPOOL 2

#define ERR LOGE

/* Number of packets to buffer in the stream socket */
#define PACKET_BUFFER_COUNT		10

/* timeout in milliseconds to prevent poll() from hanging indefinitely */
#define POLL_TIMEOUT			1000

/* milliseconds of unsucessfull a2dp packets before we stop trying to catch up
 * on write()'s and fall-back to metered writes */
#define CATCH_UP_TIMEOUT		200

/* timeout in milliseconds for a2dp_write */
#define WRITE_TIMEOUT			1000

/* timeout in seconds for command socket recv() */
#define RECV_TIMEOUT			5


typedef enum {
	A2DP_STATE_NONE = 0,
	A2DP_STATE_INITIALIZED,
	A2DP_STATE_CONFIGURING,
	A2DP_STATE_CONFIGURED,
	A2DP_STATE_STARTING,
	A2DP_STATE_STARTED,
	A2DP_STATE_STOPPING,
} a2dp_state_t;

typedef enum {
	A2DP_CMD_NONE = 0,
	A2DP_CMD_INIT,
	A2DP_CMD_CONFIGURE,
	A2DP_CMD_START,
	A2DP_CMD_STOP,
	A2DP_CMD_QUIT,
} a2dp_command_t;

/* SS_BLUETOOTH(changeon.park) 2012.02.07 */
#ifdef GLOBALCONFIG_BLUETOOTH_APT_X_SUPPORT

/*btaptx local data stracture 
btaptx_struct could be removed as we are not using it....however, it is just to keep symmetry with SBC implementation as much as possible.
*/
struct aptx_struct {
	unsigned long flags;
	int frequency;
	int mode;
	int endian;
}; 

typedef struct aptx_struct aptx_t;
char bt_a2dp_foundCodecs[3]={0};
int codec_priority; /* will make it an array to hold priorities of all found codec 0,1,2... */

#endif
/* SS_BLUETOOTH(changeon.park) End */

struct bluetooth_data {
	unsigned int link_mtu;			/* MTU for transport channel */
	struct pollfd stream;			/* Audio stream filedescriptor */
	struct pollfd server;			/* Audio daemon filedescriptor */
	a2dp_state_t state;				/* Current A2DP state */
	a2dp_command_t command;			/* Current command for a2dp_thread */
	pthread_t thread;
	pthread_mutex_t mutex;
	int started;
	pthread_cond_t thread_start;
	pthread_cond_t thread_wait;
	pthread_cond_t client_wait;

	sbc_capabilities_t sbc_capabilities;
	sbc_t sbc;				/* Codec data */
	int	frame_duration;			/* length of an SBC frame in microseconds */
	int codesize;				/* SBC codesize */
	int samples;				/* Number of encoded samples */
#ifdef GLOBALCONFIG_BT_SCMST_FEATURE
	size_t sizeof_scms_t;                   /* protection hdr */
	uint8_t scms_t_cp_header;		/* Protection header to use */
#endif
	uint8_t buffer[BUFFER_SIZE];		/* Codec transfer buffer */
	int count;				/* Codec transfer buffer counter */

	int nsamples;				/* Cumulative number of codec samples */
	uint16_t seq_num;			/* Cumulative packet sequence */
	int frame_count;			/* Current frames in buffer*/

	char	address[20];
	int	rate;
	int	channels;

	/* used for pacing our writes to the output socket */
	uint64_t	next_write;
	uint8_t		isEdrCapable;
/* SS_BLUETOOTH(changeon.park) 2012.02.07 */
#ifdef GLOBALCONFIG_BLUETOOTH_APT_X_SUPPORT
		aptx_capabilities_t aptx_capabilities; 	
		aptx_t aptx;	
		void	*aptxCodec;
#endif
/* SS_BLUETOOTH(changeon.park) End */

/* SS_BLUETOOTH(jiyoung93.hwang) 2012.04.27 -delay 200ms when a2dp_start */
        int mStandby;
};

#ifdef GLOBALCONFIG_BT_SCMST_FEATURE
#define SCMS_T_CP 0x0002
#define SCMS_T_COPY_ALLOWED 0x00
#define SCMS_T_COPY_NOT_ALLOWED 0x01
#endif

static uint64_t get_microseconds()
{
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return (now.tv_sec * 1000000UL + now.tv_nsec / 1000UL);
}

#ifdef ENABLE_TIMING
static void print_time(const char* message, uint64_t then, uint64_t now)
{
	DBG("%s: %lld us", message, now - then);
}
#endif

static int audioservice_send(struct bluetooth_data *data, const bt_audio_msg_header_t *msg);
static int audioservice_expect(struct bluetooth_data *data, bt_audio_msg_header_t *outmsg,
				int expected_type);
/* SS_BLUETOOTH(changeon.park) 2012.02.07 */
#ifdef GLOBALCONFIG_BLUETOOTH_APT_X_SUPPORT
/*
we have seprated the bluetooth_a2dp_hw_params(struct bluetooth_data *data); function for SBC and btaptx.
*/
static int bluetooth_a2dp_aptx_hw_params(struct bluetooth_data *data);
#endif
/* SS_BLUETOOTH(changeon.park) End */

static int bluetooth_a2dp_hw_params(struct bluetooth_data *data);
static void set_state(struct bluetooth_data *data, a2dp_state_t state);

/* SS_BLUETOOTH(is80.hwang) 2012.02.10 */
// to switch role to Master
#ifdef GLOBALCONFIG_BLUETOOTH_USE_CSR

int hci_open_dev(int dev_id);
int hci_close_dev(int dd);
int hci_switch_role(int dd, bdaddr_t *bdaddr, uint8_t role, int to);

#endif
/* SS_BLUETOOTH(is80.hwang) End */

static void bluetooth_close(struct bluetooth_data *data)
{
	DBG("bluetooth_close");
	if (data->server.fd >= 0) {
		/*SS_BLUETOOTH(junho1.kim) 2012.03.30 */
		// for bluetooth_parse_capabilities fail issue from QCT
		// sending BT_CLOSE to cleanup unix socket.
		char buf[BT_SUGGESTED_BUFFER_SIZE];
		struct bt_close_req *close_req = (void*) buf;
		struct bt_close_rsp *close_rsp = (void*) buf;
		int err;
		memset(close_req, 0, BT_SUGGESTED_BUFFER_SIZE);
		close_req->h.type = BT_REQUEST;
		close_req->h.name = BT_CLOSE;

		close_req->h.length = sizeof(*close_req);

		err = audioservice_send(data, &close_req->h);
		if (err < 0) {
			ERR("audioservice_send failed for BT_CLOSE_REQ\n");
		} else {
			close_rsp->h.length = 0;
			err = audioservice_expect(data, &close_rsp->h, BT_CLOSE);
			if (err < 0) {
				ERR("audioservice_expect failed for BT_CLOSE_RSP\n");
			}
		}
		bt_audio_service_close(data->server.fd);
		data->server.fd = -1;
	}

	if (data->stream.fd >= 0) {
		close(data->stream.fd);
		data->stream.fd = -1;
	}

	data->state = A2DP_STATE_NONE;

       /* SS_BLUETOOTH(jiyoung93.hwang) 2012.04.27 -delay 200ms when a2dp_start */
       data->mStandby = 0;
}

static int l2cap_set_flushable(int fd, int flushable)
{
	int flags;
	socklen_t len;

	len = sizeof(flags);
	if (getsockopt(fd, SOL_BLUETOOTH, BT_FLUSHABLE, &flags, &len) < 0)
		return -errno;

	if (flushable) {
		if (flags == BT_FLUSHABLE_ON)
			return 0;
		flags = BT_FLUSHABLE_ON;
	} else {
		if (flags == BT_FLUSHABLE_OFF)
			return 0;
		flags = BT_FLUSHABLE_OFF;
	}

	if (setsockopt(fd, SOL_BLUETOOTH, L2CAP_LM, &flags, sizeof(flags)) < 0)
		return -errno;

	return 0;
}

static int bluetooth_start(struct bluetooth_data *data)
{
	char c = 'w';
	char buf[BT_SUGGESTED_BUFFER_SIZE];
	struct bt_start_stream_req *start_req = (void*) buf;
	struct bt_start_stream_rsp *start_rsp = (void*) buf;
	struct bt_new_stream_ind *streamfd_ind = (void*) buf;
	int opt_name, err, bytes;
/* SS_BLUETOOTH(is80.hwang) 2012.02.10 */
// to switch role to Master
#ifdef GLOBALCONFIG_BLUETOOTH_USE_CSR
        int dd, dev_id = 0;
        bdaddr_t bdaddr;
#endif
/* SS_BLUETOOTH(is80.hwang) End */

	DBG("bluetooth_start");
	data->state = A2DP_STATE_STARTING;
	/* send start */
	memset(start_req, 0, BT_SUGGESTED_BUFFER_SIZE);
	start_req->h.type = BT_REQUEST;
	start_req->h.name = BT_START_STREAM;
	start_req->h.length = sizeof(*start_req);


	err = audioservice_send(data, &start_req->h);
	if (err < 0)
		goto error;

	start_rsp->h.length = sizeof(*start_rsp);
	err = audioservice_expect(data, &start_rsp->h, BT_START_STREAM);
	if (err < 0)
		goto error;

	streamfd_ind->h.length = sizeof(*streamfd_ind);
	err = audioservice_expect(data, &streamfd_ind->h, BT_NEW_STREAM);
	if (err < 0)
		goto error;

	data->stream.fd = bt_audio_service_get_data_fd(data->server.fd);
	if (data->stream.fd < 0) {
		ERR("bt_audio_service_get_data_fd failed, errno: %d", errno);
		err = -errno;
		goto error;
	}
	l2cap_set_flushable(data->stream.fd, 1);
	data->stream.events = POLLOUT;

	/* set our socket buffer to the size of PACKET_BUFFER_COUNT packets */
	bytes = data->link_mtu * PACKET_BUFFER_COUNT;
	setsockopt(data->stream.fd, SOL_SOCKET, SO_SNDBUF, &bytes,
			sizeof(bytes));

#ifdef GLOBALCONFIG_BT_SCMST_FEATURE
	data->count = sizeof(struct rtp_header) + sizeof(struct rtp_payload) + data->sizeof_scms_t;
#else
	data->count = sizeof(struct rtp_header) + sizeof(struct rtp_payload);
#endif
	data->frame_count = 0;
	data->samples = 0;
	data->nsamples = 0;
	data->seq_num = 0;
	data->frame_count = 0;
	data->next_write = 0;

	set_state(data, A2DP_STATE_STARTED);

        /* SS_BLUETOOTH(jiyoung93.hwang) 2012.04.27 -delay 200ms when a2dp_start */
        data->mStandby = 1;

/* SS_BLUETOOTH(is80.hwang) 2012.02.10 */
// to switch role to Master
#ifdef GLOBALCONFIG_BLUETOOTH_USE_CSR

	str2ba(data->address, &bdaddr);

	DBG("bluetooth_start: hci_open_dev");
	
	dd = hci_open_dev(dev_id);
	if (dd < 0) {
		perror("HCI device open failed");
	}
	else {
		if (hci_switch_role(dd, &bdaddr, 0, 10000) < 0) {
			perror("Switch role request failed");
		} else
			DBG("bluetooth_start:: hci_switch_role");

		hci_close_dev(dd);
	}
/////////////////
#endif
/* SS_BLUETOOTH(is80.hwang) End */
	return 0;

error:
	/* close bluetooth connection to force reinit and reconfiguration */
	if (data->state == A2DP_STATE_STARTING) {
		bluetooth_close(data);
		/* notify client that thread is ready for next command */
		pthread_cond_signal(&data->client_wait);
        }
	return err;
}

static int bluetooth_stop(struct bluetooth_data *data)
{
	char buf[BT_SUGGESTED_BUFFER_SIZE];
	struct bt_stop_stream_req *stop_req = (void*) buf;
	struct bt_stop_stream_rsp *stop_rsp = (void*) buf;
	int err;

	DBG("bluetooth_stop");

	data->state = A2DP_STATE_STOPPING;
	l2cap_set_flushable(data->stream.fd, 0);
	if (data->stream.fd >= 0) {
		close(data->stream.fd);
		data->stream.fd = -1;
	}

	/* send stop request */
	memset(stop_req, 0, BT_SUGGESTED_BUFFER_SIZE);
	stop_req->h.type = BT_REQUEST;
	stop_req->h.name = BT_STOP_STREAM;
	stop_req->h.length = sizeof(*stop_req);

	err = audioservice_send(data, &stop_req->h);
	if (err < 0)
		goto error;

	stop_rsp->h.length = sizeof(*stop_rsp);
	err = audioservice_expect(data, &stop_rsp->h, BT_STOP_STREAM);
	if (err < 0)
		goto error;

error:
	if (data->state == A2DP_STATE_STOPPING)
		set_state(data, A2DP_STATE_CONFIGURED);
	return err;
}

static uint8_t high_quality_default_bitpool(uint8_t freq, uint8_t mode)
{
	switch (freq) {
	case BT_SBC_SAMPLING_FREQ_16000:
	case BT_SBC_SAMPLING_FREQ_32000:
		return 53;
	case BT_SBC_SAMPLING_FREQ_44100:
		switch (mode) {
		case BT_A2DP_CHANNEL_MODE_MONO:
		case BT_A2DP_CHANNEL_MODE_DUAL_CHANNEL:
			return 31;
		case BT_A2DP_CHANNEL_MODE_STEREO:
		case BT_A2DP_CHANNEL_MODE_JOINT_STEREO:
			return 53;
		default:
			ERR("Invalid channel mode %u", mode);
			return 53;
		}
	case BT_SBC_SAMPLING_FREQ_48000:
		switch (mode) {
		case BT_A2DP_CHANNEL_MODE_MONO:
		case BT_A2DP_CHANNEL_MODE_DUAL_CHANNEL:
			return 29;
		case BT_A2DP_CHANNEL_MODE_STEREO:
		case BT_A2DP_CHANNEL_MODE_JOINT_STEREO:
			return 51;
		default:
			ERR("Invalid channel mode %u", mode);
			return 51;
		}
	default:
		ERR("Invalid sampling freq %u", freq);
		return 53;
	}
}

static uint8_t medium_quality_default_bitpool(uint8_t freq, uint8_t mode)
{
	switch (freq) {
	case BT_SBC_SAMPLING_FREQ_16000:
	case BT_SBC_SAMPLING_FREQ_32000:
		return 35;
	case BT_SBC_SAMPLING_FREQ_44100:
		switch (mode) {
		case BT_A2DP_CHANNEL_MODE_MONO:
		case BT_A2DP_CHANNEL_MODE_DUAL_CHANNEL:
			return 19;
		case BT_A2DP_CHANNEL_MODE_STEREO:
		case BT_A2DP_CHANNEL_MODE_JOINT_STEREO:
			return 35;
		default:
			ERR("Invalid channel mode %u", mode);
			return 35;
		}
	case BT_SBC_SAMPLING_FREQ_48000:
		switch (mode) {
		case BT_A2DP_CHANNEL_MODE_MONO:
		case BT_A2DP_CHANNEL_MODE_DUAL_CHANNEL:
			return 18;
		case BT_A2DP_CHANNEL_MODE_STEREO:
		case BT_A2DP_CHANNEL_MODE_JOINT_STEREO:
			return 33;
		default:
			ERR("Invalid channel mode %u", mode);
			return 33;
		}
	default:
		ERR("Invalid sampling freq %u", freq);
		return 35;
	}
}

/* SS_BLUETOOTH(changeon.park) 2012.02.07 */
#ifdef GLOBALCONFIG_BLUETOOTH_APT_X_SUPPORT

/* bluetooth_a2dp_aptx_init(struct bluetooth_data *data) function for btaptx to initialized  

NOTE:  In this function we are not checking for vender and codec ids here as while initialization we set  frequency and mode only for btaptx 17.01.2011.
last edit 17.01.2011.

*/

static int bluetooth_a2dp_aptx_init(struct bluetooth_data *data)
{
	aptx_capabilities_t *cap = &data->aptx_capabilities;
	int dir;
switch (data->rate) {
	case 48000:
		cap->frequency = BT_APTX_SAMPLING_FREQ_48000;
		break;
	case 44100:
		cap->frequency = BT_APTX_SAMPLING_FREQ_44100;
		break;
	case 32000:
		cap->frequency = BT_APTX_SAMPLING_FREQ_32000;
		break;
	case 16000:
		cap->frequency = BT_APTX_SAMPLING_FREQ_16000;
		break;
	default:
		ERR("Rate %d not supported", data->rate);
		return -1;
	}
/*
aptX only supports STEREO mode, so no need to check for othe channel modes like SBC
*/

if (data->channels == 2) {
		if (cap->channel_mode & BT_A2DP_CHANNEL_MODE_JOINT_STEREO)
			cap->channel_mode = BT_A2DP_CHANNEL_MODE_STEREO;
		else if (cap->channel_mode & BT_A2DP_CHANNEL_MODE_STEREO)
		cap->channel_mode = BT_A2DP_CHANNEL_MODE_STEREO;
		else if (cap->channel_mode & BT_A2DP_CHANNEL_MODE_DUAL_CHANNEL)
			cap->channel_mode = BT_A2DP_CHANNEL_MODE_STEREO;
		}
	else
		{
		ERR("No supported channel modes");
		return -1;
		}

if (!cap->channel_mode) {
	ERR("No supported channel modes");
	return -1;
}

	return 0;
}

#endif
/* SS_BLUETOOTH(changeon.park) End */

static int bluetooth_a2dp_init(struct bluetooth_data *data)
{
	sbc_capabilities_t *cap = &data->sbc_capabilities;
	unsigned int max_bitpool, min_bitpool;
	int dir, def_bitpool;

	switch (data->rate) {
	case 48000:
		cap->frequency = BT_SBC_SAMPLING_FREQ_48000;
		break;
	case 44100:
		cap->frequency = BT_SBC_SAMPLING_FREQ_44100;
		break;
	case 32000:
		cap->frequency = BT_SBC_SAMPLING_FREQ_32000;
		break;
	case 16000:
		cap->frequency = BT_SBC_SAMPLING_FREQ_16000;
		break;
	default:
		ERR("Rate %d not supported", data->rate);
		return -1;
	}

	if (data->channels == 2) {
		if (cap->channel_mode & BT_A2DP_CHANNEL_MODE_JOINT_STEREO)
			cap->channel_mode = BT_A2DP_CHANNEL_MODE_JOINT_STEREO;
		else if (cap->channel_mode & BT_A2DP_CHANNEL_MODE_STEREO)
			cap->channel_mode = BT_A2DP_CHANNEL_MODE_STEREO;
		else if (cap->channel_mode & BT_A2DP_CHANNEL_MODE_DUAL_CHANNEL)
			cap->channel_mode = BT_A2DP_CHANNEL_MODE_DUAL_CHANNEL;
	} else {
		if (cap->channel_mode & BT_A2DP_CHANNEL_MODE_MONO)
			cap->channel_mode = BT_A2DP_CHANNEL_MODE_MONO;
	}

	if (!cap->channel_mode) {
		ERR("No supported channel modes");
		return -1;
	}

	if (cap->block_length & BT_A2DP_BLOCK_LENGTH_16)
		cap->block_length = BT_A2DP_BLOCK_LENGTH_16;
	else if (cap->block_length & BT_A2DP_BLOCK_LENGTH_12)
		cap->block_length = BT_A2DP_BLOCK_LENGTH_12;
	else if (cap->block_length & BT_A2DP_BLOCK_LENGTH_8)
		cap->block_length = BT_A2DP_BLOCK_LENGTH_8;
	else if (cap->block_length & BT_A2DP_BLOCK_LENGTH_4)
		cap->block_length = BT_A2DP_BLOCK_LENGTH_4;
	else {
		ERR("No supported block lengths");
		return -1;
	}

	if (cap->subbands & BT_A2DP_SUBBANDS_8)
		cap->subbands = BT_A2DP_SUBBANDS_8;
	else if (cap->subbands & BT_A2DP_SUBBANDS_4)
		cap->subbands = BT_A2DP_SUBBANDS_4;
	else {
		ERR("No supported subbands");
		return -1;
	}

	if (cap->allocation_method & BT_A2DP_ALLOCATION_LOUDNESS)
		cap->allocation_method = BT_A2DP_ALLOCATION_LOUDNESS;
	else if (cap->allocation_method & BT_A2DP_ALLOCATION_SNR)
		cap->allocation_method = BT_A2DP_ALLOCATION_SNR;

		def_bitpool = (data->isEdrCapable) ?
				high_quality_default_bitpool(cap->frequency,
								 cap->channel_mode) :
				medium_quality_default_bitpool(cap->frequency,
								cap->channel_mode);

		min_bitpool = MAX(MIN_BITPOOL, cap->min_bitpool);
		max_bitpool = MIN(def_bitpool, cap->max_bitpool);

	cap->min_bitpool = min_bitpool;
	cap->max_bitpool = max_bitpool;

	return 0;
}

static void bluetooth_a2dp_setup(struct bluetooth_data *data)
{
	sbc_capabilities_t active_capabilities = data->sbc_capabilities;

	sbc_reinit(&data->sbc, 0);

	if (active_capabilities.frequency & BT_SBC_SAMPLING_FREQ_16000)
		data->sbc.frequency = SBC_FREQ_16000;

	if (active_capabilities.frequency & BT_SBC_SAMPLING_FREQ_32000)
		data->sbc.frequency = SBC_FREQ_32000;

	if (active_capabilities.frequency & BT_SBC_SAMPLING_FREQ_44100)
		data->sbc.frequency = SBC_FREQ_44100;

	if (active_capabilities.frequency & BT_SBC_SAMPLING_FREQ_48000)
		data->sbc.frequency = SBC_FREQ_48000;

	if (active_capabilities.channel_mode & BT_A2DP_CHANNEL_MODE_MONO)
		data->sbc.mode = SBC_MODE_MONO;

	if (active_capabilities.channel_mode & BT_A2DP_CHANNEL_MODE_DUAL_CHANNEL)
		data->sbc.mode = SBC_MODE_DUAL_CHANNEL;

	if (active_capabilities.channel_mode & BT_A2DP_CHANNEL_MODE_STEREO)
		data->sbc.mode = SBC_MODE_STEREO;

	if (active_capabilities.channel_mode & BT_A2DP_CHANNEL_MODE_JOINT_STEREO)
		data->sbc.mode = SBC_MODE_JOINT_STEREO;

	data->sbc.allocation = active_capabilities.allocation_method
				== BT_A2DP_ALLOCATION_SNR ? SBC_AM_SNR
				: SBC_AM_LOUDNESS;

	switch (active_capabilities.subbands) {
	case BT_A2DP_SUBBANDS_4:
		data->sbc.subbands = SBC_SB_4;
		break;
	case BT_A2DP_SUBBANDS_8:
		data->sbc.subbands = SBC_SB_8;
		break;
	}

	switch (active_capabilities.block_length) {
	case BT_A2DP_BLOCK_LENGTH_4:
		data->sbc.blocks = SBC_BLK_4;
		break;
	case BT_A2DP_BLOCK_LENGTH_8:
		data->sbc.blocks = SBC_BLK_8;
		break;
	case BT_A2DP_BLOCK_LENGTH_12:
		data->sbc.blocks = SBC_BLK_12;
		break;
	case BT_A2DP_BLOCK_LENGTH_16:
		data->sbc.blocks = SBC_BLK_16;
		break;
	}

	data->sbc.bitpool = active_capabilities.max_bitpool;
	data->codesize = sbc_get_codesize(&data->sbc);
	data->frame_duration = sbc_get_frame_duration(&data->sbc);
	DBG("frame_duration: %d us", data->frame_duration);
}

/* SS_BLUETOOTH(changeon.park) 2012.02.21 */
#ifdef GLOBALCONFIG_BLUETOOTH_APT_X_SUPPORT

/*
 bluetooth_a2dp_aptx_setup(struct bluetooth_data *data)

*/

static void bluetooth_a2dp_aptx_setup(struct bluetooth_data *data)
{
	aptx_capabilities_t active_capabilities = data->aptx_capabilities;

	if (active_capabilities.frequency & BT_APTX_SAMPLING_FREQ_16000)
			data->aptx.frequency = BT_APTX_SAMPLING_FREQ_16000;

		if (active_capabilities.frequency & BT_APTX_SAMPLING_FREQ_32000)
			data->aptx.frequency = BT_APTX_SAMPLING_FREQ_32000;

		if (active_capabilities.frequency & BT_APTX_SAMPLING_FREQ_44100)
			data->aptx.frequency = BT_APTX_SAMPLING_FREQ_44100;

		if (active_capabilities.frequency & BT_APTX_SAMPLING_FREQ_48000)
			data->aptx.frequency = BT_APTX_SAMPLING_FREQ_48000;


		if (active_capabilities.channel_mode == 2) {
			if (active_capabilities.channel_mode & BT_A2DP_CHANNEL_MODE_JOINT_STEREO)
				data->aptx.mode = BT_A2DP_CHANNEL_MODE_STEREO;
			else if (active_capabilities.channel_mode & BT_A2DP_CHANNEL_MODE_STEREO)
				data->aptx.mode = BT_A2DP_CHANNEL_MODE_STEREO;
		else if (active_capabilities.channel_mode & BT_A2DP_CHANNEL_MODE_DUAL_CHANNEL)
		data->aptx.mode = BT_A2DP_CHANNEL_MODE_STEREO;
			}
		else
			{
			ERR("No supported channel modes");
			}

	
}

#endif
/* SS_BLUETOOTH(changeon.park) End */

static int bluetooth_a2dp_hw_params(struct bluetooth_data *data)
{
	char buf[BT_SUGGESTED_BUFFER_SIZE];
	struct bt_open_req *open_req = (void *) buf;
	struct bt_open_rsp *open_rsp = (void *) buf;
	struct bt_set_configuration_req *setconf_req = (void*) buf;
	struct bt_set_configuration_rsp *setconf_rsp = (void*) buf;
	int err;

	memset(open_req, 0, BT_SUGGESTED_BUFFER_SIZE);
	open_req->h.type = BT_REQUEST;
	open_req->h.name = BT_OPEN;
	open_req->h.length = sizeof(*open_req);
	strncpy(open_req->destination, data->address, 18);
	open_req->seid = data->sbc_capabilities.capability.seid;
	open_req->lock = BT_WRITE_LOCK;

	err = audioservice_send(data, &open_req->h);
	if (err < 0)
		return err;

	open_rsp->h.length = sizeof(*open_rsp);
	err = audioservice_expect(data, &open_rsp->h, BT_OPEN);
	if (err < 0)
		return err;

	err = bluetooth_a2dp_init(data);
	if (err < 0)
		return err;


	memset(setconf_req, 0, BT_SUGGESTED_BUFFER_SIZE);
	setconf_req->h.type = BT_REQUEST;
	setconf_req->h.name = BT_SET_CONFIGURATION;
	setconf_req->h.length = sizeof(*setconf_req);
	memcpy(&setconf_req->codec, &data->sbc_capabilities,
						sizeof(data->sbc_capabilities));

	setconf_req->codec.transport = BT_CAPABILITIES_TRANSPORT_A2DP;
	setconf_req->codec.length = sizeof(data->sbc_capabilities);
	setconf_req->h.length += setconf_req->codec.length - sizeof(setconf_req->codec);

	DBG("bluetooth_a2dp_hw_params sending configuration:\n");
	switch (data->sbc_capabilities.channel_mode) {
		case BT_A2DP_CHANNEL_MODE_MONO:
			DBG("\tchannel_mode: MONO\n");
			break;
		case BT_A2DP_CHANNEL_MODE_DUAL_CHANNEL:
			DBG("\tchannel_mode: DUAL CHANNEL\n");
			break;
		case BT_A2DP_CHANNEL_MODE_STEREO:
			DBG("\tchannel_mode: STEREO\n");
			break;
		case BT_A2DP_CHANNEL_MODE_JOINT_STEREO:
			DBG("\tchannel_mode: JOINT STEREO\n");
			break;
		default:
			DBG("\tchannel_mode: UNKNOWN (%d)\n",
				data->sbc_capabilities.channel_mode);
	}
	switch (data->sbc_capabilities.frequency) {
		case BT_SBC_SAMPLING_FREQ_16000:
			DBG("\tfrequency: 16000\n");
			break;
		case BT_SBC_SAMPLING_FREQ_32000:
			DBG("\tfrequency: 32000\n");
			break;
		case BT_SBC_SAMPLING_FREQ_44100:
			DBG("\tfrequency: 44100\n");
			break;
		case BT_SBC_SAMPLING_FREQ_48000:
			DBG("\tfrequency: 48000\n");
			break;
		default:
			DBG("\tfrequency: UNKNOWN (%d)\n",
				data->sbc_capabilities.frequency);
	}
	switch (data->sbc_capabilities.allocation_method) {
		case BT_A2DP_ALLOCATION_SNR:
			DBG("\tallocation_method: SNR\n");
			break;
		case BT_A2DP_ALLOCATION_LOUDNESS:
			DBG("\tallocation_method: LOUDNESS\n");
			break;
		default:
			DBG("\tallocation_method: UNKNOWN (%d)\n",
				data->sbc_capabilities.allocation_method);
	}
	switch (data->sbc_capabilities.subbands) {
		case BT_A2DP_SUBBANDS_4:
			DBG("\tsubbands: 4\n");
			break;
		case BT_A2DP_SUBBANDS_8:
			DBG("\tsubbands: 8\n");
			break;
		default:
			DBG("\tsubbands: UNKNOWN (%d)\n",
				data->sbc_capabilities.subbands);
	}
	switch (data->sbc_capabilities.block_length) {
		case BT_A2DP_BLOCK_LENGTH_4:
			DBG("\tblock_length: 4\n");
			break;
		case BT_A2DP_BLOCK_LENGTH_8:
			DBG("\tblock_length: 8\n");
			break;
		case BT_A2DP_BLOCK_LENGTH_12:
			DBG("\tblock_length: 12\n");
			break;
		case BT_A2DP_BLOCK_LENGTH_16:
			DBG("\tblock_length: 16\n");
			break;
		default:
			DBG("\tblock_length: UNKNOWN (%d)\n",
				data->sbc_capabilities.block_length);
	}
	DBG("\tmin_bitpool: %d\n", data->sbc_capabilities.min_bitpool);
	DBG("\tmax_bitpool: %d\n", data->sbc_capabilities.max_bitpool);

	err = audioservice_send(data, &setconf_req->h);
	if (err < 0)
		return err;

	err = audioservice_expect(data, &setconf_rsp->h, BT_SET_CONFIGURATION);
	if (err < 0)
		return err;

	data->link_mtu = setconf_rsp->link_mtu;
#ifdef GLOBALCONFIG_BT_SCMST_FEATURE
	if (setconf_rsp->content_protection == SCMS_T_CP) {
		data->sizeof_scms_t = 1;
		data->scms_t_cp_header = SCMS_T_COPY_NOT_ALLOWED;
	} else {
		data->sizeof_scms_t = 0;
		data->scms_t_cp_header = SCMS_T_COPY_ALLOWED;
	}
	DBG("MTU: %d -- SCMS-T Enabled: %d", data->link_mtu, setconf_rsp->content_protection);
#else
	DBG("MTU: %d", data->link_mtu);
#endif
	/* Setup SBC encoder now we agree on parameters */
	bluetooth_a2dp_setup(data);

	DBG("\tallocation=%u\n\tsubbands=%u\n\tblocks=%u\n\tbitpool=%u\n",
		data->sbc.allocation, data->sbc.subbands, data->sbc.blocks,
		data->sbc.bitpool);

	return 0;
}

/* SS_BLUETOOTH(changeon.park) 2012.02.07 */
#ifdef GLOBALCONFIG_BLUETOOTH_APT_X_SUPPORT
/*
bluetooth_a2dp_aptx_hw_params(struct bluetooth_data *data)
*/
static int bluetooth_a2dp_aptx_hw_params(struct bluetooth_data *data)
{
	char buf[BT_SUGGESTED_BUFFER_SIZE];
	struct bt_open_req *open_req = (void *) buf;
	struct bt_open_rsp *open_rsp = (void *) buf;
	struct bt_set_configuration_req *setconf_req = (void*) buf;
	struct bt_set_configuration_rsp *setconf_rsp = (void*) buf;
	int err;

	LOGD("We are in bluetooth_a2dp_aptx_hw_params()\n");

	memset(open_req, 0, BT_SUGGESTED_BUFFER_SIZE);
	open_req->h.type = BT_REQUEST;
	open_req->h.name = BT_OPEN;
	open_req->h.length = sizeof(*open_req);
	strncpy(open_req->destination, data->address, 18);


	open_req->seid = data->aptx_capabilities.capability.seid;
	open_req->lock = BT_WRITE_LOCK;
			
		
	err = audioservice_send(data, &open_req->h);
	if (err < 0)
		return err;

	open_rsp->h.length = sizeof(*open_rsp);
	err = audioservice_expect(data, &open_rsp->h, BT_OPEN);
	if (err < 0)
		return err;

	err = bluetooth_a2dp_aptx_init(data);
	if (err < 0)
		return err;


	memset(setconf_req, 0, BT_SUGGESTED_BUFFER_SIZE);
	setconf_req->h.type = BT_REQUEST;
	setconf_req->h.name = BT_SET_CONFIGURATION;
	setconf_req->h.length = sizeof(*setconf_req);

	/* SS_BLUETOOTH 2012.04.17 prevent [OVERRUN_STATIC] */
	//memcpy(&setconf_req->codec, &data->aptx_capabilities,
	//					sizeof(aptx_capabilities_t));
	memcpy(&setconf_req->codec, &data->aptx_capabilities,
					sizeof(data->aptx_capabilities));
	/* SS_BLUETOOTH end */
	
	setconf_req->codec.transport = BT_CAPABILITIES_TRANSPORT_A2DP;
	setconf_req->codec.length = sizeof(data->aptx_capabilities);
	setconf_req->h.length += setconf_req->codec.length - sizeof(setconf_req->codec);


 DBG("bluetooth_a2dp_aptx_hw_params sending configuration:\n");
	switch (data->aptx_capabilities.channel_mode) {
		case BT_A2DP_CHANNEL_MODE_MONO:
			LOGD("\tchannel_mode: MONO\n");
			break;
		case BT_A2DP_CHANNEL_MODE_DUAL_CHANNEL:
			LOGD("\tchannel_mode: DUAL CHANNEL\n");
			break;
		case BT_A2DP_CHANNEL_MODE_STEREO:
			LOGD("\tchannel_mode: STEREO\n");
			break;
		case BT_A2DP_CHANNEL_MODE_JOINT_STEREO:
			LOGD("\tchannel_mode: JOINT STEREO\n");
			break;
		default:
			LOGD("\tchannel_mode: UNKNOWN (%d)\n",
				data->aptx_capabilities.channel_mode);
	}
	switch (data->aptx_capabilities.frequency) {
		case BT_APTX_SAMPLING_FREQ_16000:
			LOGD("\tfrequency: 16000\n");
			break;
		case BT_APTX_SAMPLING_FREQ_32000:
			LOGD("\tfrequency: 32000\n");
			break;
		case BT_APTX_SAMPLING_FREQ_44100:
			LOGD("\tfrequency: 44100\n");
			break;
		case BT_APTX_SAMPLING_FREQ_48000:
			LOGD("\tfrequency: 48000\n");
			break;
		default:
			LOGD("\tfrequency: UNKNOWN (%d)\n",
				data->aptx_capabilities.frequency);
	}
	
				
	err = audioservice_send(data, &setconf_req->h);
	
	LOGD("bluetooth_a2dp_aptx_hw_params called audioservice_send() from \n");

	if (err < 0)
		return err;

	err = audioservice_expect(data, &setconf_rsp->h, BT_SET_CONFIGURATION);
	LOGD("bluetooth_a2dp_aptx_hw_params called audioservice_expect()\n");
	if (err < 0)
		return err;

	data->link_mtu = setconf_rsp->link_mtu;
	LOGD("MTU: %d", data->link_mtu);

	/* Setup APTX encoder now we agree on parameters */

	LOGD("bluetooth_a2dp_aptx_hw_params called bluetooth_a2dp_aptx_setup(data)\n");

	bluetooth_a2dp_aptx_setup(data);
	/*
	DBG("\taptX Frequency=%u\n\taptX Channel mode=%u\n",
			data->aptx.frequency, data->aptx.mode);
	*/
	LOGD("end of bluetooth_a2dp_aptx_hw_btaptx_params()\n");
		
	return 0;
}
#endif
/* SS_BLUETOOTH(changeon.park) End */

/* SS_BLUETOOTH(changeon.park) 2012.02.07 */
#ifdef GLOBALCONFIG_BLUETOOTH_APT_X_SUPPORT
/*
avdtp_write_aptx(struct bluetooth_data *data)
*/
static int avdtp_write_aptx(struct bluetooth_data *data)
	{
		int ret = 0;
		uint64_t now;
		
	long duration = (10000.0*4.0 * (long)data->frame_count) / 441.0;

#ifdef ENABLE_TIMING
		uint64_t begin, end, begin2, end2;
		begin = get_microseconds();
#endif
		data->stream.revents = 0;
#ifdef ENABLE_TIMING
		begin2 = get_microseconds();
#endif
		ret = poll(&data->stream, 1, POLL_TIMEOUT);
#ifdef ENABLE_TIMING
		end2 = get_microseconds();
		print_time("poll", begin2, end2);
#endif
		if (ret == 1 && data->stream.revents == POLLOUT) {
			long ahead = 0;
			now = get_microseconds();
	
			if (data->next_write) {
				ahead = data->next_write - now;
#ifdef ENABLE_TIMING
				DBG("duration: %ld, ahead: %ld", duration, ahead);
#endif
				if (ahead > 0) {
					/* too fast, need to throttle */
					usleep(ahead);
				}
			} else {
				data->next_write = now;
			}
			if (ahead <= -CATCH_UP_TIMEOUT * 1000) {
				/* fallen too far behind, don't try to catch up */
				VDBG("ahead < %d, reseting next_write timestamp", -CATCH_UP_TIMEOUT * 1000);
				data->next_write = 0;
			} else {
				data->next_write += duration;
			}
	
#ifdef ENABLE_TIMING
			begin2 = get_microseconds();
#endif
			ret = send(data->stream.fd, data->buffer, data->count, MSG_NOSIGNAL);

#ifdef ENABLE_TIMING
			end2 = get_microseconds();
			print_time("send", begin2, end2);
#endif
			if (ret < 0) {
				/* can happen during normal remote disconnect */
				VDBG("send() failed: %d (errno %s)", ret, strerror(errno));
			}
			if (ret == -EPIPE) {
				bluetooth_close(data);
			}
		} else {
			/* can happen during normal remote disconnect */
			VDBG("poll() failed: %d (revents = %d, errno %s)",
					ret, data->stream.revents, strerror(errno));
			data->next_write = 0;
		}
	
		/* Reset buffer of data to send */
		data->count = 0; // btaptx has no header....
		data->frame_count = 0;
		data->samples = 0;
		data->seq_num++;
	
#ifdef ENABLE_TIMING
		end = get_microseconds();
		print_time("avdtp_aptx_write", begin, end);
#endif
		return 0; /* always return success */
	}

#endif
/* SS_BLUETOOTH(changeon.park) End */

static int avdtp_write(struct bluetooth_data *data)
{
	int ret = 0;
	struct rtp_header *header;
	struct rtp_payload *payload;

	uint64_t now;
	long duration = data->frame_duration * data->frame_count;
#ifdef ENABLE_TIMING
	uint64_t begin, end, begin2, end2;
	begin = get_microseconds();
#endif

	header = (struct rtp_header *)data->buffer;
#ifdef GLOBALCONFIG_BT_SCMST_FEATURE
	payload = (struct rtp_payload *)(data->buffer + sizeof(*header) + data->sizeof_scms_t);
	memset(data->buffer, 0, sizeof(*header) + sizeof(*payload) + data->sizeof_scms_t);
#else
	payload = (struct rtp_payload *)(data->buffer + sizeof(*header));

	memset(data->buffer, 0, sizeof(*header) + sizeof(*payload));
#endif

#ifdef GLOBALCONFIG_BT_SCMST_FEATURE
	if (data->sizeof_scms_t) {
		data->buffer[sizeof(*header)] = SCMS_T_COPY_NOT_ALLOWED;
	}else
		data->buffer[sizeof(*header)] = SCMS_T_COPY_ALLOWED;
#endif
	payload->frame_count = data->frame_count;
	header->v = 2;
	header->pt = 1;
	header->sequence_number = htons(data->seq_num);
	header->timestamp = htonl(data->nsamples);
	header->ssrc = htonl(1);

	data->stream.revents = 0;
#ifdef ENABLE_TIMING
	begin2 = get_microseconds();
#endif
	ret = poll(&data->stream, 1, POLL_TIMEOUT);
#ifdef ENABLE_TIMING
	end2 = get_microseconds();
	print_time("poll", begin2, end2);
#endif
	if (ret == 1 && data->stream.revents == POLLOUT) {
		long ahead = 0;
		now = get_microseconds();

		if (data->next_write) {
			ahead = data->next_write - now;
#ifdef ENABLE_TIMING
			DBG("duration: %ld, ahead: %ld", duration, ahead);
#endif
			if (ahead > 0) {
				/* too fast, need to throttle */
				usleep(ahead);
			}
		} else {
			data->next_write = now;
		}
		if (ahead <= -CATCH_UP_TIMEOUT * 1000) {
			/* fallen too far behind, don't try to catch up */
			VDBG("ahead < %d, reseting next_write timestamp", -CATCH_UP_TIMEOUT * 1000);
			data->next_write = 0;
		} else {
			data->next_write += duration;
		}

#ifdef ENABLE_TIMING
		begin2 = get_microseconds();
#endif
		ret = send(data->stream.fd, data->buffer, data->count, MSG_NOSIGNAL);
#ifdef ENABLE_TIMING
		end2 = get_microseconds();
		print_time("send", begin2, end2);
#endif
		if (ret < 0) {
			/* can happen during normal remote disconnect */
			VDBG("send() failed: %d (errno %s)", ret, strerror(errno));
		}
		if (ret == -EPIPE) {
			bluetooth_close(data);
		}
	} else {
		/* can happen during normal remote disconnect */
		VDBG("poll() failed: %d (revents = %d, errno %s)",
				ret, data->stream.revents, strerror(errno));
		data->next_write = 0;
	}

	/* Reset buffer of data to send */
#ifdef GLOBALCONFIG_BT_SCMST_FEATURE
	data->count = sizeof(struct rtp_header) + sizeof(struct rtp_payload) + data->sizeof_scms_t;
#else
	data->count = sizeof(struct rtp_header) + sizeof(struct rtp_payload);
#endif
	data->frame_count = 0;
	data->samples = 0;
	data->seq_num++;

#ifdef ENABLE_TIMING
	end = get_microseconds();
	print_time("avdtp_write", begin, end);
#endif
	return 0; /* always return success */
}

static int audioservice_send(struct bluetooth_data *data,
		const bt_audio_msg_header_t *msg)
{
	int err;
	uint16_t length;

	length = msg->length ? msg->length : BT_SUGGESTED_BUFFER_SIZE;

	VDBG("sending %s", bt_audio_strtype(msg->type));
	if (send(data->server.fd, msg, length,
			MSG_NOSIGNAL) > 0)
		err = 0;
	else {
		err = -errno;
		ERR("Error sending data to audio service: %s(%d)",
			strerror(errno), errno);
		if (err == -EPIPE)
			bluetooth_close(data);
	}

	return err;
}

static int audioservice_recv(struct bluetooth_data *data,
		bt_audio_msg_header_t *inmsg)
{
	int err, ret;
	const char *type, *name;
	uint16_t length;

	length = inmsg->length ? inmsg->length : BT_SUGGESTED_BUFFER_SIZE;

	ret = recv(data->server.fd, inmsg, length, 0);
	if (ret < 0) {
		err = -errno;
		ERR("Error receiving IPC data from bluetoothd: %s (%d)",
						strerror(errno), errno);
		if (err == -EPIPE)
			bluetooth_close(data);
	} else if ((size_t) ret < sizeof(bt_audio_msg_header_t)) {
		ERR("Too short (%d bytes) IPC packet from bluetoothd", ret);
		err = -EINVAL;
	} else if (inmsg->type == BT_ERROR) {
		bt_audio_error_t *error = (bt_audio_error_t *)inmsg;
		ret = recv(data->server.fd, &error->posix_errno,
				sizeof(error->posix_errno), 0);
		if (ret < 0) {
			err = -errno;
			ERR("Error receiving error code for BT_ERROR: %s (%d)",
						strerror(errno), errno);
			if (err == -EPIPE)
				bluetooth_close(data);
		} else {
			ERR("%s failed : %s(%d)",
					bt_audio_strname(error->h.name),
					strerror(error->posix_errno),
					error->posix_errno);
			err = -error->posix_errno;
		}
	} else {
		type = bt_audio_strtype(inmsg->type);
		name = bt_audio_strname(inmsg->name);
		if (type && name) {
			DBG("Received %s - %s", type, name);
			err = 0;
		} else {
			err = -EINVAL;
			ERR("Bogus message type %d - name %d"
					" received from audio service",
					inmsg->type, inmsg->name);
		}

	}
	return err;
}

static int audioservice_expect(struct bluetooth_data *data,
		bt_audio_msg_header_t *rsp_hdr, int expected_name)
{
	int err = audioservice_recv(data, rsp_hdr);

	if (err != 0)
		return err;

	if (rsp_hdr->name != expected_name) {
		err = -EINVAL;
		ERR("Bogus message %s received while %s was expected",
				bt_audio_strname(rsp_hdr->name),
				bt_audio_strname(expected_name));
	}
	return err;

}

static int bluetooth_init(struct bluetooth_data *data)
{
	int sk, err;
	struct timeval tv = {.tv_sec = RECV_TIMEOUT};

	DBG("bluetooth_init");

	sk = bt_audio_service_open();
	if (sk < 0) {
		ERR("bt_audio_service_open failed\n");
		return -errno;
	}

	err = setsockopt(sk, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	if (err < 0) {
		ERR("bluetooth_init setsockopt(SO_RCVTIMEO) failed %d", err);
		return err;
	}

	data->server.fd = sk;
	data->server.events = POLLIN;
	data->state = A2DP_STATE_INITIALIZED;

	return 0;
}

/* SS_BLUETOOTH(changeon.park) 2012.02.07 */
#ifdef GLOBALCONFIG_BLUETOOTH_APT_X_SUPPORT	
static int bluetooth_parse_capabilities(struct bluetooth_data *data,
					struct bt_get_capabilities_rsp *rsp)
{
	int bytes_left = rsp->h.length - sizeof(*rsp);
	codec_capabilities_t *codec = (void *) rsp->data;

	codec_capabilities_t *psbccodec = NULL;
	codec_capabilities_t *paptxcodec = NULL;
	codec_priority = 0;
	memset(bt_a2dp_foundCodecs, 0,3);

	if (codec->transport != BT_CAPABILITIES_TRANSPORT_A2DP) {
		ERR("bluetooth_parse_capabilities() invalid codec->transport : %d",codec->transport);
		return -EINVAL;
	}

	while (bytes_left > 0) {
		if ((codec->type == BT_A2DP_SBC_SINK) &&
				!(codec->lock & BT_WRITE_LOCK)) {
			psbccodec = codec;
			LOGD("bluetooth_parse_capabilities:: detect SBC");
		}

		if ((codec->type == BT_A2DP_APTX_SINK) &&
				!(codec->lock & BT_WRITE_LOCK)) {
			paptxcodec = codec;
			LOGD("bluetooth_parse_capabilities:: detect APTX");
		}

		bytes_left -= codec->length;
		codec = (void *) ((uint8_t *)codec + codec->length);
		if (codec->length == 0) {
			ERR("bluetooth_parse_capabilities() invalid codec capabilities length");
			break;
		}
	}

   if (paptxcodec && paptxcodec->length == sizeof(data->aptx_capabilities)) {
      LOGD("~~~~~~~~~~~~~ codec type set to aptx");
	  memset(bt_a2dp_foundCodecs, 0,3);
	  memcpy(bt_a2dp_foundCodecs, "aptx", 3);
	  codec_priority=1;

        memcpy(&data->aptx_capabilities, paptxcodec, paptxcodec->length);
		LOGD ("paptxcodec->length=%u\n",paptxcodec->length);
		LOGD("aptx codec ID0 = %x ",  data->aptx_capabilities.codec_id0); 
		LOGD("aptx codec ID1 = %x ",  data->aptx_capabilities.codec_id1); 
		LOGD("aptx Vendor ID0 = %x ", data->aptx_capabilities.vender_id0); 
		LOGD("aptx Vendor ID1 = %x ", data->aptx_capabilities.vender_id1); 
		LOGD("aptx Vendor ID2 = %x ", data->aptx_capabilities.vender_id2); 
		LOGD("aptx Vendor ID3 = %x ", data->aptx_capabilities.vender_id3); 
		LOGD("aptx channel mode = %u", data->aptx_capabilities.channel_mode); 
		LOGD("aptx frequency = %u ",  data->aptx_capabilities.frequency); 

		data->isEdrCapable = rsp->isEdrCapable;
		return 0;
   }

   if (psbccodec && psbccodec->length == sizeof(data->sbc_capabilities)) {
      	LOGD("~~~~~~~~~~~~~ codec type set to sbc");
		memset(bt_a2dp_foundCodecs, 0,3);
		memcpy(bt_a2dp_foundCodecs, "sbc", 2);
		codec_priority=0;
        memcpy(&data->sbc_capabilities, psbccodec, psbccodec->length);
		LOGD("psbccodec->length = %u\n\n ",	psbccodec->length); 
		LOGD("SBC channel mode = %u\n ",	data->sbc_capabilities.channel_mode); 
		LOGD("SBC frequency = %u\n ",		data->sbc_capabilities.frequency); 
		LOGD("SBC allocation_method = %u\n ", data->sbc_capabilities.allocation_method); 
		LOGD("SBC subbands = %u\n ",	data->sbc_capabilities.subbands); 
		LOGD("SBC block_length = %u\n ", data->sbc_capabilities.block_length); 
		LOGD("SBC min_bitpool = %u\n ", data->sbc_capabilities.min_bitpool); 
		LOGD("SBC max_bitpool = %u\n ", data->sbc_capabilities.max_bitpool);

		data->isEdrCapable = rsp->isEdrCapable;
		return 0;
   }

   	return -EINVAL;
}

#else
static int bluetooth_parse_capabilities(struct bluetooth_data *data,
					struct bt_get_capabilities_rsp *rsp)
{
	int bytes_left = rsp->h.length - sizeof(*rsp);
	codec_capabilities_t *codec = (void *) rsp->data;

	if (codec->transport != BT_CAPABILITIES_TRANSPORT_A2DP){
		ERR("bluetooth_parse_capabilities() invalid codec->transport : %d",codec->transport);  
		return -EINVAL;
	}

	while (bytes_left > 0) {
		if ((codec->type == BT_A2DP_SBC_SINK) &&
			!(codec->lock & BT_WRITE_LOCK))
			break;

		if (codec->length == 0) {
			ERR("bluetooth_parse_capabilities() invalid codec capabilities length");
			return -EINVAL;
		}
		bytes_left -= codec->length;
		codec = (codec_capabilities_t *)((char *)codec + codec->length);
	}

	if (bytes_left <= 0 ||
			codec->length != sizeof(data->sbc_capabilities)){
		ERR("bluetooth_parse_capabilities() error bytes_left : %d",bytes_left);
		ERR("bluetooth_parse_capabilities() error codec->length : %d data->sbc_capabilities size :%d ",
			codec->length,sizeof(data->sbc_capabilities));
		return -EINVAL;
	}

	memcpy(&data->sbc_capabilities, codec, codec->length);

	data->isEdrCapable = rsp->isEdrCapable;
	return 0;
}
#endif
/* SS_BLUETOOTH(changeon.park) End */

static int bluetooth_configure(struct bluetooth_data *data)
{
	char buf[BT_SUGGESTED_BUFFER_SIZE];
	struct bt_get_capabilities_req *getcaps_req = (void*) buf;
	struct bt_get_capabilities_rsp *getcaps_rsp = (void*) buf;
	int err;

	DBG("bluetooth_configure");

	data->state = A2DP_STATE_CONFIGURING;
	memset(getcaps_req, 0, BT_SUGGESTED_BUFFER_SIZE);
	getcaps_req->h.type = BT_REQUEST;
	getcaps_req->h.name = BT_GET_CAPABILITIES;

	getcaps_req->flags = 0;
	getcaps_req->flags |= BT_FLAG_AUTOCONNECT;
	strncpy(getcaps_req->destination, data->address, 18);
	getcaps_req->transport = BT_CAPABILITIES_TRANSPORT_A2DP;
	getcaps_req->h.length = sizeof(*getcaps_req);

	err = audioservice_send(data, &getcaps_req->h);
	if (err < 0) {
		ERR("audioservice_send failed for BT_GETCAPABILITIES_REQ\n");
		goto error;
	}

	getcaps_rsp->h.length = 0;
	err = audioservice_expect(data, &getcaps_rsp->h, BT_GET_CAPABILITIES);
	if (err < 0) {
		ERR("audioservice_expect failed for BT_GETCAPABILITIES_RSP\n");
		goto error;
	}
/* SS_BLUETOOTH(changeon.park) 2012.02.07 */
#ifdef GLOBALCONFIG_BLUETOOTH_APT_X_SUPPORT	
	memset(bt_a2dp_foundCodecs, 0,3);
#endif
/* SS_BLUETOOTH(changeon.park) End */
	err = bluetooth_parse_capabilities(data, getcaps_rsp);
	if (err < 0) {
		ERR("bluetooth_parse_capabilities failed err: %d", err);
		goto error;
	}

/* SS_BLUETOOTH(changeon.park) 2012.02.07 */
#ifdef GLOBALCONFIG_BLUETOOTH_APT_X_SUPPORT
// If aptx is seen configure it...
	if((strncmp(bt_a2dp_foundCodecs,"aptx",3))==0 && (codec_priority==1) ) {
		
		err = bluetooth_a2dp_aptx_hw_params(data);
		if (err < 0) {
			ERR("bluetooth_a2dp_aptx_hw_params failed err: %d", err);
			goto error;
		}
	}
	else 

		if((strncmp(bt_a2dp_foundCodecs,"sbc",2))==0 && (codec_priority==0) ) {
		err = bluetooth_a2dp_hw_params(data);
		if (err < 0) {
			ERR("bluetooth_a2dp_sbc_hw_params failed err: %d", err);
			goto error;
		}
	}
#else
	err = bluetooth_a2dp_hw_params(data);
	if (err < 0) {
		ERR("bluetooth_a2dp_hw_params failed err: %d", err);
		goto error;
	}
#endif
/* SS_BLUETOOTH(changeon.park) End */

	set_state(data, A2DP_STATE_CONFIGURED);
	return 0;

error:

	if (data->state == A2DP_STATE_CONFIGURING) {
		bluetooth_close(data);
		/* notify client that thread is ready for next command */
		pthread_cond_signal(&data->client_wait);
        }
	return err;
}

static void set_state(struct bluetooth_data *data, a2dp_state_t state)
{
	data->state = state;
	pthread_cond_signal(&data->client_wait);
}

static void __set_command(struct bluetooth_data *data, a2dp_command_t command)
{
	VDBG("set_command %d\n", command);
	data->command = command;
	pthread_cond_signal(&data->thread_wait);
	return;
}

static void set_command(struct bluetooth_data *data, a2dp_command_t command)
{
	pthread_mutex_lock(&data->mutex);
	__set_command(data, command);
	pthread_mutex_unlock(&data->mutex);
}

/* timeout is in milliseconds */
static int wait_for_start(struct bluetooth_data *data, int timeout)
{
	a2dp_state_t state = data->state;
	struct timeval tv;
	struct timespec ts;
	int err = 0;

#ifdef ENABLE_TIMING
	uint64_t begin, end;
	begin = get_microseconds();
#endif

	gettimeofday(&tv, (struct timezone *) NULL);
	ts.tv_sec = tv.tv_sec + (timeout / 1000);
	ts.tv_nsec = (tv.tv_usec + (timeout % 1000) * 1000L ) * 1000L;

	pthread_mutex_lock(&data->mutex);
	while (state != A2DP_STATE_STARTED) {
		if (state == A2DP_STATE_NONE)
			__set_command(data, A2DP_CMD_INIT);
		else if (state == A2DP_STATE_INITIALIZED)
			__set_command(data, A2DP_CMD_CONFIGURE);
		else if (state == A2DP_STATE_CONFIGURED) {
			__set_command(data, A2DP_CMD_START);
		}
again:
		err = pthread_cond_timedwait(&data->client_wait, &data->mutex, &ts);
		if (err) {
			/* don't timeout if we're done */
			if (data->state == A2DP_STATE_STARTED) {
				err = 0;
				break;
			}
			if (err == ETIMEDOUT)
				break;
			goto again;
		}

		if (state == data->state)
			goto again;

		state = data->state;

		if (state == A2DP_STATE_NONE) {
			err = ENODEV;
			break;
		}
	}
	pthread_mutex_unlock(&data->mutex);

#ifdef ENABLE_TIMING
	end = get_microseconds();
	print_time("wait_for_start", begin, end);
#endif

	/* pthread_cond_timedwait returns positive errors */
	return -err;
}

/* SS_BLUETOOTH(changeon.park) 2012.02.07 */
#ifdef GLOBALCONFIG_BLUETOOTH_APT_X_SUPPORT
void aptx_finish(struct bluetooth_data *data)
{
	memset(&data->aptx, 0 , sizeof(data->aptx));
	memset(bt_a2dp_foundCodecs, 0,3);
	if(data->aptxCodec!=NULL){
		free(data->aptxCodec);
		data->aptxCodec=NULL;
	}
}
#endif
/* SS_BLUETOOTH(changeon.park) End */

static void a2dp_free(struct bluetooth_data *data)
{
	pthread_cond_destroy(&data->client_wait);
	pthread_cond_destroy(&data->thread_wait);
	pthread_cond_destroy(&data->thread_start);
	pthread_mutex_destroy(&data->mutex);
/* SS_BLUETOOTH(changeon.park) 2012.02.07 */
#ifdef GLOBALCONFIG_BLUETOOTH_APT_X_SUPPORT
	aptx_finish(data);
#endif
/* SS_BLUETOOTH(changeon.park) End */
	free(data);
	return;
}

static void* a2dp_thread(void *d)
{
	struct bluetooth_data* data = (struct bluetooth_data*)d;
	a2dp_command_t command = A2DP_CMD_NONE;
	int err = 0;

	DBG("a2dp_thread started");
	prctl(PR_SET_NAME, (int)"a2dp_thread", 0, 0, 0);

	pthread_mutex_lock(&data->mutex);

	data->started = 1;
	pthread_cond_signal(&data->thread_start);

	while (1)
	{
		while (1) {
			pthread_cond_wait(&data->thread_wait, &data->mutex);

			/* Initialization needed */
			if (data->state == A2DP_STATE_NONE &&
				data->command != A2DP_CMD_QUIT) {
				err = bluetooth_init(data);
			}

			/* New state command signaled */
			if (command != data->command) {
				command = data->command;
				break;
			}
		}

		switch (command) {
			case A2DP_CMD_CONFIGURE:
				if (data->state != A2DP_STATE_INITIALIZED)
					break;
				err = bluetooth_configure(data);
				break;

			case A2DP_CMD_START:
				if (data->state != A2DP_STATE_CONFIGURED)
					break;
				err = bluetooth_start(data);
				break;

			case A2DP_CMD_STOP:
				if (data->state != A2DP_STATE_STARTED)
					break;
				err = bluetooth_stop(data);
				break;

			case A2DP_CMD_QUIT:
				bluetooth_close(data);
				sbc_finish(&data->sbc);
/* SS_BLUETOOTH(changeon.park) 2012.02.07 */
#ifdef GLOBALCONFIG_BLUETOOTH_APT_X_SUPPORT
				aptx_finish(data);
#endif
/* SS_BLUETOOTH(changeon.park) End */
				pthread_mutex_unlock(&data->mutex); // SS_BLUETOOTH(is80.hwang) 2012.02.10 : code review
				a2dp_free(data);
				return NULL; // SS_BLUETOOTH(is80.hwang) 2012.02.10 : code review

			case A2DP_CMD_INIT:
				/* already called bluetooth_init() */
			default:
				break;
		}
		// reset last command in case of error to allow
		// re-execution of the same command
		if (err < 0) {
			command = A2DP_CMD_NONE;
		}
	}

done:
	pthread_mutex_unlock(&data->mutex);
	DBG("a2dp_thread finished");
	return NULL;
}

int a2dp_init(int rate, int channels, a2dpData* dataPtr)
{
	struct bluetooth_data* data;
	pthread_attr_t attr;
	int err;

	DBG("a2dp_init rate: %d channels: %d", rate, channels);
	*dataPtr = NULL;
	data = malloc(sizeof(struct bluetooth_data));
	if (!data)
		return -1;

	memset(data, 0, sizeof(struct bluetooth_data));
	data->server.fd = -1;
	data->stream.fd = -1;
	data->state = A2DP_STATE_NONE;
	data->command = A2DP_CMD_NONE;

	strncpy(data->address, "00:00:00:00:00:00", 18);
	data->rate = rate;
	data->channels = channels;

        /* SS_BLUETOOTH(jiyoung93.hwang) 2012.04.27 -delay 200ms when a2dp_start */
        data->mStandby = 0;

/* SS_BLUETOOTH(changeon.park) 2012.02.07 */
#ifdef GLOBALCONFIG_BLUETOOTH_APT_X_SUPPORT
	data->aptxCodec = malloc((size_t)SizeofAptxbtenc());
	aptxbtenc_init(data->aptxCodec, 0); //check what is best 0 or 1? for aptX init
	memset(bt_a2dp_foundCodecs, 0,3);
#endif
/* SS_BLUETOOTH(changeon.park) End */

	sbc_init(&data->sbc, 0);

	pthread_mutex_init(&data->mutex, NULL);
	pthread_cond_init(&data->thread_start, NULL);
	pthread_cond_init(&data->thread_wait, NULL);
	pthread_cond_init(&data->client_wait, NULL);

	pthread_mutex_lock(&data->mutex);
	data->started = 0;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	err = pthread_create(&data->thread, &attr, a2dp_thread, data);
	if (err) {
		/* If the thread create fails we must not wait */
		pthread_mutex_unlock(&data->mutex);
		err = -err;
		goto error;
	}

	/* Make sure the state machine is ready and waiting */
	while (!data->started) {
		pthread_cond_wait(&data->thread_start, &data->mutex);
	}

	/* Poke the state machine to get it going */
	pthread_cond_signal(&data->thread_wait);

	pthread_mutex_unlock(&data->mutex);
	pthread_attr_destroy(&attr);

	*dataPtr = data;
	return 0;
error:
	bluetooth_close(data);
	sbc_finish(&data->sbc);
/* SS_BLUETOOTH(changeon.park) 2012.02.07 */
#ifdef GLOBALCONFIG_BLUETOOTH_APT_X_SUPPORT
	aptx_finish(data);
#endif
/* SS_BLUETOOTH(changeon.park) End */
	pthread_attr_destroy(&attr);
	a2dp_free(data);

	return err;
}

void a2dp_set_sink(a2dpData d, const char* address)
{
	struct bluetooth_data* data = (struct bluetooth_data*)d;
	if (strncmp(data->address, address, 18)) {
		strncpy(data->address, address, 18);
		set_command(data, A2DP_CMD_INIT);
	}
}

#ifdef GLOBALCONFIG_BT_SCMST_FEATURE
void a2dp_set_cp_header(a2dpData d, uint8_t cpHeader)
{
	struct bluetooth_data* data = (struct bluetooth_data*)d;

	if (data) {
		VDBG("a2dp_set_cp_header called.  current: %x, new: %x", data->scms_t_cp_header, cpHeader);

		/* For SCMS-T least significant two bits matter, mask out the rest */
		data->scms_t_cp_header = (cpHeader & 0x03);
	}
}
#endif

/* SS_BLUETOOTH(changeon.park) 2012.02.07 */
#ifdef GLOBALCONFIG_BLUETOOTH_APT_X_SUPPORT
int a2dp_write(a2dpData d, const void* buffer, int count)
{
	struct bluetooth_data* data = (struct bluetooth_data*)d;
	uint8_t* src = (uint8_t *)buffer;
	int codesize;
	int err, ret = 0;
	long frames_left = count;
	int encoded;
	unsigned int written;
	const char *buff;
	int did_configure = 0;

	/* aptX local variables */

 uint16_t* srcptr = (uint16_t *)buffer;
 uint32_t pcmL[4];
 uint32_t pcmR[4];
 uint16_t encodedSample[2];
 uint8_t t;

#ifdef ENABLE_TIMING
	uint64_t begin, end;
	DBG("********** a2dp_write **********");
	begin = get_microseconds();
#endif

	err = wait_for_start(data, WRITE_TIMEOUT);
	if (err < 0)
		return err;
	codesize = data->codesize;

        /* SS_BLUETOOTH(jiyoung93.hwang) 2012.04.27 -delay 200ms when a2dp_start */
        if (data->mStandby) {
            usleep(200000);
            data->mStandby=0;
        }

	//start aptx
  if((strncmp(bt_a2dp_foundCodecs,"aptx",3))==0 && (codec_priority==1))  {
	  codesize = 16;
      written = 4;
      encoded = 8;
//      LOGD("aptx encoding");
      while (frames_left >= codesize) {
         // get the next 4 PCM samples per channel from the buffer
         for (t = 0; t < 4; t++) {
            pcmL[t] = (uint32_t*)(srcptr[2*t]);
            pcmR[t] = (uint32_t*)(srcptr[2*t+1]);
         }
     	   
	/* 
	Call to aptx encoder function can be made directly like this, 
	aptxbtenc_encodestereo(data->aptxCodec, pcmL, pcmR,  data->buffer + data->count); 
	*/
       aptxbtenc_encodestereo(data->aptxCodec, pcmL, pcmR,  &encodedSample);

		 data->buffer[data->count]   = (uint8_t)((encodedSample[0] >>8) & 0xff);	 
		 data->buffer[data->count+1] = (uint8_t)((encodedSample[0] >>0) & 0xff);	 
		 data->buffer[data->count+2] = (uint8_t)((encodedSample[1] >>8) & 0xff);	 
		 data->buffer[data->count+3] = (uint8_t)((encodedSample[1] >>0) & 0xff);

         srcptr += encoded;
         data->count += written;
         data->frame_count++;
         data->samples += encoded;
         data->nsamples += encoded;
		 
		  /* No space left for another frame then send 
		      in case of btaptx using max of mtu approx 892 of data->link_mtu (approx 895) 
		  */
		  
			if( (data->count + 4 >= 370) || (data->count + 4 >= data->link_mtu) ||
             (data->count + 4 >= BUFFER_SIZE)) {

//		      LOGD("from btaptx sending packet %d, data->samples=%d, count %d, link_mtu %u",
//                  data->seq_num, data->samples, data->count,
//                  data->link_mtu);
	   
            err = avdtp_write_aptx(data);
            if (err < 0)
               return err;
         }

    	ret += codesize; 
        frames_left -= codesize; 
        
      }
		/* end aptx */
	  
  } 
  //else if ((strncmp(bt_a2dp_foundCodecs,"sbc",2))==0 && (codec_priority==0)) {
  else {  /* SS_BLUETOOTH(changeon.park) 2012.04.23 - changed for SBC legacy */
//	LOGD("SBC encoding");

	while (frames_left >= codesize) {
		/* Enough data to encode (sbc wants 512 byte blocks) */
		encoded = sbc_encode(&(data->sbc), src, codesize,
					data->buffer + data->count,
					sizeof(data->buffer) - data->count,
					&written);
		if (encoded <= 0) {
			ERR("Encoding error %d", encoded);
			goto done;
		}
//		LOGD("aptX enabled but sbc_encode is used, returned %d, codesize: %d, written: %d\n",
//			encoded, codesize, written);

		src += encoded;
		data->count += written;
		data->frame_count++;
		data->samples += encoded;
		data->nsamples += encoded;

		/* No space left for another frame then send */
		if ((data->count + written >= data->link_mtu) ||
				(data->count + written >= BUFFER_SIZE)) {
//			LOGD("sending packet %d, count %d, link_mtu %u",
//					data->seq_num, data->count,
//					data->link_mtu);
			err = avdtp_write(data);
			if (err < 0)
				return err;
		}

		ret += encoded;
		frames_left -= encoded;
	}
    }
//else {
//LOGD ("No suported codec encoder\n");
//	}

	if (frames_left > 0)
		ERR("%ld bytes left at end of a2dp_write\n", frames_left);

done:
#ifdef ENABLE_TIMING
	end = get_microseconds();
	print_time("a2dp_write total", begin, end);
#endif
	return ret;
}
#else /*in case of SBC only, an original version of a2dp_write() function is called  */
int a2dp_write(a2dpData d, const void* buffer, int count)
{
	struct bluetooth_data* data = (struct bluetooth_data*)d;
	uint8_t* src = (uint8_t *)buffer;
	int codesize;
	int err, ret = 0;
	long frames_left = count;
	int encoded;
	unsigned int written;
	const char *buff;
	int did_configure = 0;
#ifdef ENABLE_TIMING
	uint64_t begin, end;
	DBG("********** a2dp_write **********");
	begin = get_microseconds();
#endif

	err = wait_for_start(data, WRITE_TIMEOUT);
	if (err < 0)
		return err;

	codesize = data->codesize;

        /* SS_BLUETOOTH(jiyoung93.hwang) 2012.04.27 -delay 200ms when a2dp_start */
        if (data->mStandby) {
            usleep(200000);
            data->mStandby=0;
        }

	while (frames_left >= codesize) {
		/* Enough data to encode (sbc wants 512 byte blocks) */
		encoded = sbc_encode(&(data->sbc), src, codesize,
					data->buffer + data->count,
					sizeof(data->buffer) - data->count,
					&written);
		if (encoded <= 0) {
			ERR("Encoding error %d", encoded);
			goto done;
		}
		VDBG("sbc_encode returned %d, codesize: %d, written: %d\n",
			encoded, codesize, written);

		src += encoded;
		data->count += written;
		data->frame_count++;
		data->samples += encoded;
		data->nsamples += encoded;

		/* No space left for another frame then send */
		if ((data->count + written >= data->link_mtu) ||
				(data->count + written >= BUFFER_SIZE)) {
			VDBG("sending packet %d, count %d, link_mtu %u",
					data->seq_num, data->count,
					data->link_mtu);
			err = avdtp_write(data);
			if (err < 0)
				return err;
		}

		ret += encoded;
		frames_left -= encoded;
	}

	if (frames_left > 0)
		ERR("%ld bytes left at end of a2dp_write\n", frames_left);

done:
#ifdef ENABLE_TIMING
	end = get_microseconds();
	print_time("a2dp_write total", begin, end);
#endif
	return ret;
}
#endif
/* SS_BLUETOOTH(changeon.park) End */

int a2dp_stop(a2dpData d)
{
	struct bluetooth_data* data = (struct bluetooth_data*)d;
	DBG("a2dp_stop\n");
	if (!data)
		return 0;

	set_command(data, A2DP_CMD_STOP);
	return 0;
}

void a2dp_cleanup(a2dpData d)
{
	struct bluetooth_data* data = (struct bluetooth_data*)d;
	DBG("a2dp_cleanup\n");
	set_command(data, A2DP_CMD_QUIT);
}
