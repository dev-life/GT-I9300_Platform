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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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

#include <bluetooth/bluetooth.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#include <glib.h>
#include <dbus/dbus.h>
#include <gdbus.h>

#include "log.h"
#include "error.h"
#include "uinput.h"
#include "adapter.h"
#include "../src/device.h"
#include "device.h"
#include "manager.h"
#include "avdtp.h"
#include "control.h"
#include "sdpd.h"
#include "glib-helper.h"
#include "btio.h"
#include "dbus-common.h"
#include"control_types.h"
#include"control_util.h"
#include "syslog.h"
#include <android/log.h>


#define LOGE(x...) __android_log_print(ANDROID_LOG_ERROR, "control", x)
#define LOGD(x...) __android_log_print(ANDROID_LOG_DEBUG, "control", x)

// TODO: Remove this after tests
#undef debug
#undef error
#define debug LOGD
#define error LOGE


#define QUIRK_NO_RELEASE	1 << 0

static DBusConnection *connection = NULL;
static gchar *input_device_name = NULL;
static GSList *servers = NULL;
static gboolean avrcpmetadata = false;

struct avctp_state_callback {
	avctp_state_cb cb;
	void *user_data;
	unsigned int id;
};

struct avctp_server {
	bdaddr_t src;
	GIOChannel *io;
	uint32_t tg_record_id;
#ifndef ANDROID
	uint32_t ct_record_id;
#endif
};

struct control {
	struct audio_device *dev;

	avctp_state_t state;

	int uinput;

	GIOChannel *io;
	guint io_id;

	uint16_t mtu;

	gboolean target;
	/* Content info fo notification events from upper layers */
	struct playeragent *pa[AVRCP_NUM_EVENTS];
	/* Continuation response */
	struct avrcp_cont_resp *cont;

	uint8_t key_quirks[256];
};

/*
 *Global vairable shared between the application (media player) and
 * AVRCP profile this variable has to set by the media player
 */
static GSList *adapters = NULL;

struct media_player {
	char *agent_path;
	char *agent_name;
};

struct control_adapter {
	struct btd_adapter *btd_adapter;
	DBusConnection *conn;

	struct media_player *player;
};

static struct {
	const char *name;
	uint8_t avrcp;
	uint16_t uinput;
} key_map[] = {
	{ "VOLUME UP",		VOL_UP_OP,		KEY_VOLUMEUP },
	{ "VOLUME DOWN",	VOL_DOWN_OP,		KEY_VOLUMEDOWN },
	{ "MUTE",		MUTE_OP,		KEY_MUTE },
	{ "PLAY",		PLAY_OP,		KEY_PLAYCD },
	{ "STOP",		STOP_OP,		KEY_STOPCD },
	{ "PAUSE",		PAUSE_OP,		KEY_PAUSECD },
	{ "FORWARD",		FORWARD_OP,		KEY_NEXTSONG },
	{ "BACKWARD",		BACKWARD_OP,		KEY_PREVIOUSSONG },
	{ "REWIND",		REWIND_OP,		KEY_REWIND },
	{ "FAST FORWARD",	FAST_FORWARD_OP,	KEY_FASTFORWARD },
	/* operands in passthrough commands for the Cat 2 Panel */
	{ "KEY 0",	AVRCP_OP_0,		KEY_0 },
	{ "KEY 1",	AVRCP_OP_1,		KEY_1 },
	{ "KEY 2",	AVRCP_OP_2,		KEY_2 },
	{ "KEY 3",	AVRCP_OP_3,		KEY_3 },
	{ "KEY 4",	AVRCP_OP_4,		KEY_4 },
	{ "KEY 5",	AVRCP_OP_5,		KEY_5 },
	{ "KEY 6",	AVRCP_OP_6,		KEY_6 },
	{ "KEY 7",	AVRCP_OP_7,		KEY_7 },
	{ "KEY 8",	AVRCP_OP_8,		KEY_8 },
	{ "KEY 9",	AVRCP_OP_9,		KEY_9 },
	{ "DOT",	AVRCP_OP_DOT,		KEY_DOT },
	{ "ENTER",	AVRCP_OP_ENTER,		KEY_ENTER },
	{ "CLEAR",	AVRCP_OP_CLEAR,		KEY_CLEAR },

	{ NULL, 0, 0 }
};

static GSList *avctp_callbacks = NULL;

static void auth_cb(DBusError *derr, void *user_data);

static struct control_adapter *find_adapter(GSList *list,
					    struct btd_adapter *btd_adapter)
{
	GSList *l;
	struct control_adapter *adapter;

	for (l = list; l; l = l->next) {
		adapter = l->data;
		if (adapter->btd_adapter == btd_adapter)
			return adapter;
	}

	return NULL;
}

static sdp_record_t *avrcp_ct_record(void)
{
	sdp_list_t *svclass_id, *pfseq, *apseq, *root;
	uuid_t root_uuid, l2cap, avctp, avrct;
	sdp_profile_desc_t profile[1];
	sdp_list_t *aproto, *proto[2];
	sdp_record_t *record;
	sdp_data_t *psm, *version, *features;
	uint16_t lp = AVCTP_PSM;
	uint16_t avrcp_ver = 0x0100, avctp_ver = 0x0103, feat = 0x000f;

	record = sdp_record_alloc();
	if (!record)
		return NULL;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(0, &root_uuid);
	sdp_set_browse_groups(record, root);

	/* Service Class ID List */
	sdp_uuid16_create(&avrct, AV_REMOTE_SVCLASS_ID);
	svclass_id = sdp_list_append(0, &avrct);
	sdp_set_service_classes(record, svclass_id);

	/* Protocol Descriptor List */
	sdp_uuid16_create(&l2cap, L2CAP_UUID);
	proto[0] = sdp_list_append(0, &l2cap);
	psm = sdp_data_alloc(SDP_UINT16, &lp);
	proto[0] = sdp_list_append(proto[0], psm);
	apseq = sdp_list_append(0, proto[0]);

	sdp_uuid16_create(&avctp, AVCTP_UUID);
	proto[1] = sdp_list_append(0, &avctp);
	version = sdp_data_alloc(SDP_UINT16, &avctp_ver);
	proto[1] = sdp_list_append(proto[1], version);
	apseq = sdp_list_append(apseq, proto[1]);

	aproto = sdp_list_append(0, apseq);
	sdp_set_access_protos(record, aproto);

	/* Bluetooth Profile Descriptor List */
	sdp_uuid16_create(&profile[0].uuid, AV_REMOTE_PROFILE_ID);
	profile[0].version = avrcp_ver;
	pfseq = sdp_list_append(0, &profile[0]);
	sdp_set_profile_descs(record, pfseq);

	features = sdp_data_alloc(SDP_UINT16, &feat);
	sdp_attr_add(record, SDP_ATTR_SUPPORTED_FEATURES, features);

	sdp_set_info_attr(record, "AVRCP CT", 0, 0);

	free(psm);
	free(version);
	sdp_list_free(proto[0], 0);
	sdp_list_free(proto[1], 0);
	sdp_list_free(apseq, 0);
	sdp_list_free(pfseq, 0);
	sdp_list_free(aproto, 0);
	sdp_list_free(root, 0);
	sdp_list_free(svclass_id, 0);

	return record;
}

static sdp_record_t *avrcp_tg_record(void)
{
	sdp_list_t *svclass_id, *pfseq, *apseq, *root;
	uuid_t root_uuid, l2cap, avctp, avrtg;
	sdp_profile_desc_t profile[1];
	sdp_list_t *aproto, *proto[2];
	sdp_record_t *record;
	sdp_data_t *psm, *version, *features;
	uint16_t lp = AVCTP_PSM;
	uint16_t avrcp_ver = 0x0100;
	if (avrcpmetadata)
		avrcp_ver = 0x0103;
	uint16_t avctp_ver = 0x0103, feat = 0x000f;

#ifdef ANDROID
	if (avrcpmetadata)
		feat = 0x0011;
	else
		feat = 0x0001;
#endif
	record = sdp_record_alloc();
	if (!record)
		return NULL;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(0, &root_uuid);
	sdp_set_browse_groups(record, root);

	/* Service Class ID List */
	sdp_uuid16_create(&avrtg, AV_REMOTE_TARGET_SVCLASS_ID);
	svclass_id = sdp_list_append(0, &avrtg);
	sdp_set_service_classes(record, svclass_id);

	/* Protocol Descriptor List */
	sdp_uuid16_create(&l2cap, L2CAP_UUID);
	proto[0] = sdp_list_append(0, &l2cap);
	psm = sdp_data_alloc(SDP_UINT16, &lp);
	proto[0] = sdp_list_append(proto[0], psm);
	apseq = sdp_list_append(0, proto[0]);

	sdp_uuid16_create(&avctp, AVCTP_UUID);
	proto[1] = sdp_list_append(0, &avctp);
	version = sdp_data_alloc(SDP_UINT16, &avctp_ver);
	proto[1] = sdp_list_append(proto[1], version);
	apseq = sdp_list_append(apseq, proto[1]);

	aproto = sdp_list_append(0, apseq);
	sdp_set_access_protos(record, aproto);

	/* Bluetooth Profile Descriptor List */
	sdp_uuid16_create(&profile[0].uuid, AV_REMOTE_PROFILE_ID);
	profile[0].version = avrcp_ver;
	pfseq = sdp_list_append(0, &profile[0]);
	sdp_set_profile_descs(record, pfseq);

	features = sdp_data_alloc(SDP_UINT16, &feat);
	sdp_attr_add(record, SDP_ATTR_SUPPORTED_FEATURES, features);

	sdp_set_info_attr(record, "AVRCP TG", 0, 0);

	free(psm);
	free(version);
	sdp_list_free(proto[0], 0);
	sdp_list_free(proto[1], 0);
	sdp_list_free(apseq, 0);
	sdp_list_free(aproto, 0);
	sdp_list_free(pfseq, 0);
	sdp_list_free(root, 0);
	sdp_list_free(svclass_id, 0);

	return record;
}

static int send_event(int fd, uint16_t type, uint16_t code, int32_t value)
{
	struct uinput_event event;

	memset(&event, 0, sizeof(event));
	event.type	= type;
	event.code	= code;
	event.value	= value;

	return write(fd, &event, sizeof(event));
}

static void send_key(int fd, uint16_t key, int pressed)
{
	if (fd < 0)
		return;

	send_event(fd, EV_KEY, key, pressed);
	send_event(fd, EV_SYN, SYN_REPORT, 0);
}

struct avrcp_response *avrcp_init_response(uint8_t pdu_id, uint8_t packet_type,
						uint16_t p_len)
{
	struct avrcp_response *resp;
	uint32_t size = AVRCP_RESPONSE_PDU_LEN + p_len;
	resp = g_new0(struct avrcp_response, size);
	if (!resp)
		return NULL;

	/* Company ID */
	memcpy((uint8_t*)&resp->sig_company_id[0],
			(uint8_t*)AVRCP_BLUETOOTH_COMPANY_ID, 3);
	/* PDU ID */
	resp->pdu_id = pdu_id;
	/* Packet Type */
	resp->packet_type = packet_type;
	/*Param Len*/
	resp->param_len = p_len;

	return resp;
}

static void avrcp_send_response(struct control *control, uint8_t *packet,
				uint16_t len)
{
	int sock = 0;

	if (!control->io) {
		DBG("control->io is NULL, so return");
		return;
	}

	sock = g_io_channel_unix_get_fd(control->io);

	errno = 0;
	if (write(sock, packet, len) < 0) {
		LOGE("Can not write socket errno = %d!!", errno);
		avrcp_disconnect(control->dev);
	}
}

static uint16_t avrcp_cont_packet(struct control *control,
				uint8_t **packet,
				uint8_t *headers,
				struct avrcp_response *resp)
{
	struct avrcp_response *rsp;
	uint8_t *ptr = NULL;
	uint16_t bytes;

	control->cont->pdus--;

	if (control->cont->pdus) {
		bytes = AVRCP_MAX_PARAMETERS;
		resp->packet_type = AVCTP_PACKET_CONTINUE;
	} else {
		bytes = control->cont->total - control->cont->bytes;
		resp->packet_type = AVCTP_PACKET_END;
	}

	ptr = g_new(uint8_t, AVCTP_HEADER_LENGTH +
			AVRCP_COMPLETE_HEADER_LEN + bytes);

	/*Copy data to packet.*/
	memcpy(ptr, headers, AVCTP_AVRCP_HEADER_LEN);
	// Handle packet formation for PDU Length more than 512 bytes ..
	
	LOGD("avrcp_cont_packet packet formation -->");

	memcpy(ptr + AVCTP_AVRCP_HEADER_LEN, resp, AVRCP_RESPONSE_PDU_LEN);
	memcpy(ptr + AVCTP_AVRCP_HEADER_LEN + AVRCP_RESPONSE_PDU_LEN,((uint8_t *)resp) + AVRCP_RESPONSE_PDU_LEN +
					control->cont->bytes, bytes);
	

	/*Set param_len to the response pdu.*/
	rsp = (struct avrcp_response*)(ptr + AVCTP_AVRCP_HEADER_LEN);
	rsp->param_len = _htobs (bytes);

	control->cont->bytes += bytes;

	if (!control->cont->pdus) {
		/*Do cleaning. It was last packet.*/
		/*if (control->cont->resp)
			free(control->cont->resp);*/

		if (control->cont) {
			free(control->cont);
			control->cont = NULL;
		}
	}

	*packet = ptr;

	return AVCTP_HEADER_LENGTH + AVRCP_COMPLETE_HEADER_LEN + bytes;
}

static uint16_t avrcp_first_packet(struct control *control,
				uint8_t **packet,
				uint8_t *headers,
				struct avrcp_response *resp)
{
	struct avrcp_response *rsp;
	uint8_t *ptr = NULL;
	uint16_t len = resp->param_len;

	ptr = g_new(uint8_t, AVRCP_MAX_FRAME_SIZE + AVCTP_HEADER_LENGTH);

	/* Store information about sent data.*/
	control->cont = (struct avrcp_cont_resp *)
				malloc(sizeof(struct avrcp_cont_resp));
	resp->packet_type = AVCTP_PACKET_START;
	control->cont->resp = resp;
	control->cont->bytes = AVRCP_MAX_PARAMETERS;
	control->cont->pdu_id = resp->pdu_id;
	control->cont->total = len;

	/*Let's count how many pdu's we need.*/
	if (len % AVRCP_MAX_PARAMETERS)
		control->cont->pdus = len / AVRCP_MAX_PARAMETERS + 1;
	else
		control->cont->pdus = len / AVRCP_MAX_PARAMETERS;

	/*Copy data to packet.*/
	memcpy(ptr, headers, AVCTP_AVRCP_HEADER_LEN);
	memcpy(ptr + AVCTP_AVRCP_HEADER_LEN, resp, AVRCP_RESPONSE_PDU_LEN +
						AVRCP_MAX_PARAMETERS);

	/*Set param_len to the response pdu.*/
	rsp = (struct avrcp_response*)(ptr + AVCTP_AVRCP_HEADER_LEN);
	rsp->param_len = _htobs(control->cont->bytes);

	control->cont->pdus--;

	*packet = ptr;

	return AVRCP_MAX_FRAME_SIZE + AVCTP_HEADER_LENGTH;
}

static void avrcp_create_response(struct control *control,
				uint8_t *headers,
				struct avrcp_response *resp)
{
	uint8_t *packet = NULL;
	uint16_t len = resp->param_len;
	uint32_t total = AVRCP_COMPLETE_HEADER_LEN + len;

	if (total <= AVRCP_MAX_FRAME_SIZE) {
		packet = g_new(uint8_t, total + AVCTP_HEADER_LENGTH);
		resp->param_len = be_to_host16((uint8_t*)&(resp->param_len));

		if (packet) {
			memcpy(packet, headers, AVCTP_AVRCP_HEADER_LEN);
			memcpy(packet + AVCTP_AVRCP_HEADER_LEN, resp,
					AVRCP_RESPONSE_PDU_LEN + len);
		}

		total += AVCTP_HEADER_LENGTH;

	} else if (control->cont) {
		/*Continuation*/
		total = avrcp_cont_packet(control, &packet, headers, resp);
	} else {
		/*Start fragmentation*/
		total = avrcp_first_packet(control, &packet, headers, resp);
	}

	avrcp_send_response(control, packet, total);

	if (packet)
		g_free(packet);
}

static void set_ctype_code(void *ptr, uint8_t code)
{
	struct avrcp_header *header;

	header = (struct avrcp_header *)((uint8_t *)ptr + AVCTP_HEADER_LENGTH);

	header->code = code;
}

static void set_error_code(void *ptr, uint8_t err)
{
	uint8_t *buf;

	buf = (uint8_t*)(ptr) + AVRCP_RESPONSE_PDU_LEN;

	store_u8(&buf, err);
}

static void avrcp_error_response(struct control* control, uint8_t *headers,
				uint8_t pdu_id, uint8_t error_code)
{
	struct avrcp_response *resp;

	resp = avrcp_init_response(pdu_id, AVCTP_PACKET_SINGLE, 1);

	if (!resp)
		return;

	set_error_code(resp, error_code);
	set_ctype_code(headers, CTYPE_REJECTED);
	avrcp_create_response(control, headers, resp);
	g_free(resp);
}

static void avrcp_accept_response(struct control* control, uint8_t *headers,
				uint8_t pdu_id)
{
	struct avrcp_response *resp;

	resp = avrcp_init_response(pdu_id, AVCTP_PACKET_SINGLE, 0);

	if (!resp)
		return;

	set_ctype_code(headers, CTYPE_ACCEPTED);
	avrcp_create_response(control, headers, resp);
	g_free(resp);
}

static struct playeragent* avrcp_playeragent_new(struct control* control,
						uint8_t *buf)
{
	struct playeragent *pa;
	struct avrcp_query *query = (struct avrcp_query *)
					(buf + AVCTP_AVRCP_HEADER_LEN);
	pa = g_new0(struct playeragent, sizeof(struct playeragent));
	if (!pa)
		return NULL;

	pa->control = control;
	pa->headers = g_memdup(buf, AVCTP_AVRCP_HEADER_LEN);
	pa->pdu_id = query->pdu_id;
	pa->event_id = 0;

	return pa;
}

static void avrcp_playeragent_free(struct playeragent **pa)
{
	if (*pa) {
		if ((*pa)->headers)
			g_free((*pa)->headers);
		g_free(*pa);
		*pa = NULL;
	}
}

static void avrcp_supported_events_rsp(DBusPendingCall *call, void *user_data)
{
	DBusMessage *msg;
	DBusError err;
	DBusMessageIter iter;
	struct avrcp_response *resp = NULL;
	struct playeragent *pa = (struct playeragent *)user_data;
	uint8_t event_count;
	uint8_t *ptr;
	uint8_t i;

	/* steal_reply will always return non-NULL since the callback
	 * is only called after a reply has been received */
	msg = dbus_pending_call_steal_reply(call);

	LOGD("AVRCP_EVENTS_SUPPORTED RSP");
	dbus_error_init(&err);
	if (dbus_set_error_from_message(&err, msg)) {
		if (g_str_equal(DBUS_ERROR_UNKNOWN_METHOD, err.name) ||
				g_str_equal(DBUS_ERROR_NO_REPLY, err.name)) {
			dbus_error_free(&err);
			goto error;
		}
	} else {
		set_ctype_code(pa->headers, CTYPE_STABLE);

		dbus_message_iter_init(msg, &iter);
		dbus_message_iter_get_basic(&iter, &event_count);

		resp = avrcp_init_response(pa->pdu_id, AVCTP_PACKET_SINGLE,
						event_count + 2);
		if (!resp)
			goto error;

		ptr = (uint8_t*)(resp) + AVRCP_RESPONSE_PDU_LEN;
		store_u8(&ptr, AVRCP_EVENTS_SUPPORTED);
		store_u8(&ptr, event_count);
		dbus_message_iter_next(&iter);

		for (i = 0; i < event_count; i++) {
			dbus_message_iter_get_basic(&iter, ptr);
			dbus_message_iter_next(&iter);
			ptr++;
		}

		avrcp_create_response(pa->control, pa->headers, resp);
		goto clean;
	}

error:
	avrcp_error_response(pa->control, pa->headers, pa->pdu_id,
				AVRCP_ERR_INTERNAL_ERROR);
clean:
	if (resp)
		g_free(resp);
	if (msg)
		dbus_message_unref(msg);
	if (pa)
		dbus_pending_call_cancel(pa->call);

	dbus_pending_call_unref(call);
	avrcp_playeragent_free(&pa);
}

static void avrcp_list_player_setting_attrs_rsp(DBusPendingCall *call,
						void *user_data)
{
	DBusMessage *msg;
	DBusError err;
	DBusMessageIter iter;
	struct avrcp_response *resp = NULL;
	struct playeragent *pa = (struct playeragent *)user_data;
	uint8_t attr_count;
	uint8_t *ptr;
	uint8_t i;

	/* steal_reply will always return non-NULL since the callback
	 * is only called after a reply has been received */
	msg = dbus_pending_call_steal_reply(call);

	LOGD("AVRCP_LIST_PLAYER_SETTING_ATTRS RSP");
	dbus_error_init(&err);
	if (dbus_set_error_from_message(&err, msg)) {
		if (g_str_equal(DBUS_ERROR_UNKNOWN_METHOD, err.name) ||
				g_str_equal(DBUS_ERROR_NO_REPLY, err.name)) {
			dbus_error_free(&err);
			goto error;
		}
	} else {
		set_ctype_code(pa->headers, CTYPE_STABLE);

		dbus_message_iter_init(msg, &iter);
		dbus_message_iter_get_basic(&iter, &attr_count);

		resp = avrcp_init_response(pa->pdu_id, AVCTP_PACKET_SINGLE,
						attr_count + 1);
		if (!resp)
			goto error;

		ptr = (uint8_t*)(resp) + AVRCP_RESPONSE_PDU_LEN;
		store_u8(&ptr, attr_count);
		dbus_message_iter_next(&iter);

		for (i = 0; i < attr_count; i++) {
			dbus_message_iter_get_basic(&iter, ptr);
			dbus_message_iter_next(&iter);
			ptr++;
		}

		avrcp_create_response(pa->control, pa->headers, resp);
		goto clean;
	}

error:
	avrcp_error_response(pa->control, pa->headers, pa->pdu_id,
				AVRCP_ERR_INTERNAL_ERROR);
clean:
	if (resp)
		g_free(resp);
	if (msg)
		dbus_message_unref(msg);
	if (pa)
		dbus_pending_call_cancel(pa->call);
	dbus_pending_call_unref(call);
	avrcp_playeragent_free(&pa);
}

static void avrcp_list_player_setting_vals_rsp(DBusPendingCall *call,
						void *user_data)
{
	DBusMessage *msg;
	DBusError err;
	DBusMessageIter iter;
	struct avrcp_response *resp = NULL;
	struct playeragent *pa = (struct playeragent *)user_data;
	uint8_t error_code = AVRCP_ERR_NO_ERROR;
	uint8_t val_count;
	uint8_t *ptr;
	uint8_t i;

	/* steal_reply will always return non-NULL since the callback
	 * is only called after a reply has been received */
	msg = dbus_pending_call_steal_reply(call);

	LOGD("AVRCP_LIST_PLAYER_SETTING_VALS RSP");
	dbus_error_init(&err);
	if (dbus_set_error_from_message(&err, msg)) {
		if (g_str_equal(DBUS_ERROR_UNKNOWN_METHOD, err.name) ||
				g_str_equal(DBUS_ERROR_NO_REPLY, err.name)) {
			dbus_error_free(&err);
			goto error;
		}
	} else {
		set_ctype_code(pa->headers, CTYPE_STABLE);

		dbus_message_iter_init(msg, &iter);
		dbus_message_iter_get_basic(&iter, &val_count);

		resp = avrcp_init_response(pa->pdu_id, AVCTP_PACKET_SINGLE,
						val_count + 1);
		if (!resp) {
			error_code = AVRCP_ERR_INTERNAL_ERROR;
			goto error;
		}

		if (!val_count) {
			error_code = AVRCP_ERR_INVALID_PARM;
			goto error;
		}

		ptr = (uint8_t*)(resp) + AVRCP_RESPONSE_PDU_LEN;
		store_u8(&ptr, val_count);
		dbus_message_iter_next(&iter);

		for (i = 0; i < val_count; i++) {
			dbus_message_iter_get_basic(&iter, ptr);
			dbus_message_iter_next(&iter);
			ptr++;
		}

		avrcp_create_response(pa->control, pa->headers, resp);
		goto clean;
	}

error:
	avrcp_error_response(pa->control, pa->headers, pa->pdu_id,
				error_code);
clean:
	if (resp)
		g_free(resp);
	if (msg)
		dbus_message_unref(msg);
	if (pa)
		dbus_pending_call_cancel(pa->call);
	dbus_pending_call_unref(call);
	avrcp_playeragent_free(&pa);
}

static void avrcp_get_player_setting_value_rsp(DBusPendingCall *call,
						void *user_data)
{
	DBusMessage *msg;
	DBusError err;
	DBusMessageIter iter;
	struct avrcp_response *resp = NULL;
	struct playeragent *pa = (struct playeragent *)user_data;
	uint8_t error_code = AVRCP_ERR_NO_ERROR;
	uint8_t val_count;
	uint8_t attr_count;
	uint8_t *ptr;
	uint8_t i;

	/* steal_reply will always return non-NULL since the callback
	 * is only called after a reply has been received */
	msg = dbus_pending_call_steal_reply(call);

	LOGD("AVRCP_GET_PLAYER_SETTING_VAL RSP");
	dbus_error_init(&err);
	if (dbus_set_error_from_message(&err, msg)) {
		if (g_str_equal(DBUS_ERROR_UNKNOWN_METHOD, err.name) ||
				g_str_equal(DBUS_ERROR_NO_REPLY, err.name)) {
			dbus_error_free(&err);
			goto error;
		}
	} else {
		set_ctype_code(pa->headers, CTYPE_STABLE);

		dbus_message_iter_init(msg, &iter);
		dbus_message_iter_get_basic(&iter, &val_count);
		dbus_message_iter_init(msg, &iter);
		dbus_message_iter_get_basic(&iter, &attr_count);

		resp = avrcp_init_response(pa->pdu_id, AVCTP_PACKET_SINGLE,
						val_count * 2 + 1);
		if (!resp) {
			error_code = AVRCP_ERR_INTERNAL_ERROR;
			goto error;
		}

		if (!val_count) {
			error_code = AVRCP_ERR_INVALID_PARM;
			goto error;
		}

		ptr = (uint8_t*)(resp) + AVRCP_RESPONSE_PDU_LEN;
		store_u8(&ptr, val_count);
		dbus_message_iter_next(&iter);

		for (i = 0; i < val_count; i++) {
			dbus_message_iter_get_basic(&iter, ptr);
			dbus_message_iter_next(&iter);
			ptr++;
			dbus_message_iter_get_basic(&iter, ptr);
			dbus_message_iter_next(&iter);
			ptr++;
		}

		avrcp_create_response(pa->control, pa->headers, resp);
		goto clean;
	}

error:
	avrcp_error_response(pa->control, pa->headers, pa->pdu_id,
				error_code);
clean:
	if (resp)
		g_free(resp);
	if (msg)
		dbus_message_unref(msg);
	if (pa)
		dbus_pending_call_cancel(pa->call);
	dbus_pending_call_unref(call);
	avrcp_playeragent_free(&pa);
}

static void avrcp_set_player_setting_value_rsp(DBusPendingCall *call,
						void *user_data)
{
	DBusMessage *msg;
	DBusError err;
	DBusMessageIter iter;
	struct playeragent *pa = (struct playeragent *)user_data;
	uint8_t error_code = AVRCP_ERR_NO_ERROR;
	uint8_t val_count;

	/* steal_reply will always return non-NULL since the callback
	 * is only called after a reply has been received */
	msg = dbus_pending_call_steal_reply(call);

	LOGD("AVRCP_SET_PLAYER_SETTING_VAL RSP");
	dbus_error_init(&err);
	if (dbus_set_error_from_message(&err, msg)) {
		if (g_str_equal(DBUS_ERROR_UNKNOWN_METHOD, err.name) ||
				g_str_equal(DBUS_ERROR_NO_REPLY, err.name)) {
			dbus_error_free(&err);
			goto error;
		}
	} else {
		dbus_message_iter_init(msg, &iter);
		dbus_message_iter_get_basic(&iter, &val_count);

		if (!val_count) {
			error_code = AVRCP_ERR_INVALID_PARM;
			goto error;
		}
		avrcp_accept_response(pa->control, pa->headers, pa->pdu_id);

		goto clean;
	}

error:
	avrcp_error_response(pa->control, pa->headers, pa->pdu_id,
				error_code);
clean:
	if (msg)
		dbus_message_unref(msg);
	if (pa)
		dbus_pending_call_cancel(pa->call);
	dbus_pending_call_unref(call);
	avrcp_playeragent_free(&pa);
}

static void avrcp_register_notification_rsp(DBusPendingCall *call, void *user_data)
{
	DBusMessage *msg;
	DBusError err;
	struct avrcp_response *resp = NULL;
	struct playeragent *pa = (struct playeragent *)user_data;
	uint8_t status;
	uint64_t uid;
	uint8_t event;
	uint8_t error_code = AVRCP_ERR_NO_ERROR;
	uint8_t *ptr;
	uint8_t attr_count = 2; //There are two player application settings attributes
	uint8_t attrId1 = PLAYER_SET_REPEAT;
	uint8_t attrId2 = PLAYER_SET_SHUFFLE;
	uint8_t repeatValue;
	uint8_t shuffleValue;

	set_ctype_code(pa->headers, CTYPE_INTERIM);

	/* steal_reply will always return non-NULL since the callback
	 * is only called after a reply has been received */
	msg = dbus_pending_call_steal_reply(call);

	dbus_error_init(&err);
	if (dbus_set_error_from_message(&err, msg)) {
		if (g_str_equal(DBUS_ERROR_UNKNOWN_METHOD, err.name) ||
				g_str_equal(DBUS_ERROR_NO_REPLY, err.name)) {
			dbus_error_free(&err);
			goto error;
		}
	}

	dbus_error_init(&err);
	switch (pa->event_id) {
	case AVRCP_EVENT_PLAYBACK_STATUS_CHANGED:
		LOGD("AVRCP_EVENT_PLAYBACK_STATUS_CHANGED RSP");
		if (!dbus_message_get_args(msg, &err, DBUS_TYPE_BYTE, &event,
			DBUS_TYPE_BYTE, &status, DBUS_TYPE_INVALID)) {
			dbus_error_free(&err);
			goto error;
		}

		resp = avrcp_init_response(pa->pdu_id, AVCTP_PACKET_SINGLE, 2);
		if (!resp)
			goto error;
		ptr = (uint8_t*)(resp) + AVRCP_RESPONSE_PDU_LEN;
		store_u8(&ptr, event);
		store_u8(&ptr, status);

		if (status == 0xFF)
			set_ctype_code(pa->headers, CTYPE_REJECTED);

		break;
	case AVRCP_EVENT_TRACK_CHANGED:
		LOGD("AVRCP_EVENT_TRACK_CHANGED RSP");
		if (!dbus_message_get_args(msg, &err, DBUS_TYPE_BYTE, &event,
				DBUS_TYPE_UINT64, &uid, DBUS_TYPE_INVALID))
			goto error;

		resp = avrcp_init_response(pa->pdu_id, AVCTP_PACKET_SINGLE, 9);
		if (!resp)
			goto error;
		ptr = (uint8_t*)(resp) + AVRCP_RESPONSE_PDU_LEN;
		store_u8(&ptr, event);
		store_u64(&ptr, uid);

		break;
	case AVRCP_EVENT_PLAYER_APPLICATION_SETTING_CHANGED:
		DBG("AVRCP_EVENT_PLAYER_APPLICATION_SETTING_CHANGED RSP");
		if (!dbus_message_get_args(msg, &err, DBUS_TYPE_BYTE, &event,
				DBUS_TYPE_BYTE, &shuffleValue, 
				DBUS_TYPE_BYTE, &repeatValue,
				DBUS_TYPE_INVALID)) {
			dbus_error_free(&err);
			goto error;
		}
		resp = avrcp_init_response(pa->pdu_id, AVCTP_PACKET_SINGLE, attr_count * 2 + 2);
		ptr = (uint8_t*)(resp) + AVRCP_RESPONSE_PDU_LEN;
		event = AVRCP_EVENT_PLAYER_APPLICATION_SETTING_CHANGED;
		store_u8(&ptr, event);
		
		store_u8(&ptr, attr_count);
		
		store_u8(&ptr, attrId1);
		store_u8(&ptr, repeatValue);

		store_u8(&ptr, attrId2);
		store_u8(&ptr, shuffleValue);

		break;
	default:
		LOGE("Unknown/NotSupported Event Received");
		error_code = AVRCP_ERR_INVALID_PARM;
		goto error;
	}

	pa->control->pa[pa->event_id - 1] = pa; /* Remember event content */
	avrcp_create_response(pa->control, pa->headers, resp);

	goto clean;
error:
	avrcp_error_response(pa->control, pa->headers, pa->pdu_id, error_code);
	avrcp_playeragent_free(&pa); /* clear content in case of error */
clean:
	if (resp)
		g_free(resp);
	if (msg)
		dbus_message_unref(msg);
	if (pa)
		dbus_pending_call_cancel(pa->call);

	dbus_pending_call_unref(call);
}

static void avrcp_get_element_attr_rsp(DBusPendingCall *call, void *user_data)
{
	DBusMessage *msg;
	DBusError err;
	DBusMessageIter iter, struct_iter, array_iter;
	struct avrcp_response *resp = NULL;
	struct playeragent *pa = (struct playeragent *)user_data;
	struct attribute *attr;
	uint8_t num_of_attr;
	uint8_t i;
	GSList *attr_list = NULL;
	uint32_t payload_size = 0;
	uint8_t *ptr;

	/* steal_reply will always return non-NULL since the callback
	 * is only called after a reply has been received */
	msg = dbus_pending_call_steal_reply(call);

	dbus_error_init(&err);
	if (dbus_set_error_from_message(&err, msg)) {
		if (g_str_equal(DBUS_ERROR_UNKNOWN_METHOD, err.name) ||
				g_str_equal(DBUS_ERROR_NO_REPLY, err.name)) {
			dbus_error_free(&err);
			goto error;
		}
	}

	/* struct {u8, array{struct{u32,u16,u16,array(byte)},....}*/
	dbus_message_iter_init(msg, &iter);
	if (dbus_message_iter_get_arg_type(&iter)!= DBUS_TYPE_STRUCT)
		goto error;

	dbus_message_iter_recurse(&iter, &struct_iter);
	/*Take attribute count. */
	dbus_message_iter_get_basic(&struct_iter, &num_of_attr);

	if (!num_of_attr)
		goto error;

	payload_size += sizeof(num_of_attr);

	dbus_message_iter_next(&struct_iter);

	if (dbus_message_iter_get_arg_type(&struct_iter) != DBUS_TYPE_ARRAY ||
		dbus_message_iter_get_element_type(&struct_iter) != DBUS_TYPE_STRUCT)
		goto error;

	dbus_message_iter_recurse(&struct_iter, &array_iter);

	for (i = 0; i < num_of_attr; i++) {
		DBusMessageIter iter;
		uint8_t *value;

		if (dbus_message_iter_get_arg_type(&array_iter) != DBUS_TYPE_STRUCT)
			goto error;

		dbus_message_iter_recurse(&array_iter, &struct_iter);

		if (dbus_message_iter_get_arg_type(&struct_iter) != DBUS_TYPE_UINT32)
			goto error;

		attr = g_try_new(struct attribute, 1);
		if (!attr)
			goto error;

		dbus_message_iter_get_basic(&struct_iter, &attr->id);
		dbus_message_iter_next(&struct_iter);
		payload_size += sizeof(attr->id);

		dbus_message_iter_get_basic(&struct_iter, &attr->char_set);
		dbus_message_iter_next (&struct_iter);
		payload_size += sizeof(attr->char_set);

		dbus_message_iter_get_basic(&struct_iter, &attr->len);
		dbus_message_iter_next (&struct_iter);
		payload_size += sizeof(attr->len);

		payload_size += attr->len;

		dbus_message_iter_recurse(&struct_iter, &iter);
		dbus_message_iter_get_fixed_array(&iter, &value, (int*)&attr->len);

		attr->value = g_memdup(value, attr->len);

		dbus_message_iter_next (&array_iter);

		attr_list = g_slist_append(attr_list, attr);
		attr = NULL;
	}

	resp = avrcp_init_response(pa->pdu_id, AVCTP_PACKET_SINGLE, payload_size);
	if (!resp)
		goto error;

	ptr = (uint8_t*)(resp) + AVRCP_RESPONSE_PDU_LEN;
	store_u8(&ptr, num_of_attr);

	/*copy payload into resp*/
	for (i = 0; i < num_of_attr; i++) {
		attr = (struct attribute *)g_slist_nth_data(attr_list, i);
		store_u32(&ptr, attr->id);
		store_u16(&ptr, attr->char_set);
		store_u16(&ptr, attr->len);
		store_arr(&ptr, attr->value, (int)attr->len);
		g_free(attr);
	}

	g_slist_free(attr_list);

	set_ctype_code(pa->headers, CTYPE_STABLE);
	avrcp_create_response(pa->control, pa->headers, resp);
	goto clean;
error:
	avrcp_error_response(pa->control, pa->headers, pa->pdu_id,
			AVRCP_ERR_INTERNAL_ERROR);
clean:
	/*Clean if no fragmentation is ongoing.*/
	if (resp && !pa->control->cont)
		g_free(resp);
	if (msg)
		dbus_message_unref(msg);
	if (pa)
		dbus_pending_call_cancel(pa->call);
	dbus_pending_call_unref(call);
	avrcp_playeragent_free(&pa);
}

static void avrcp_get_play_status_rsp(DBusPendingCall *call, void *user_data)
{
	DBusMessage *msg;
	DBusError err;
	struct avrcp_response *resp = NULL;
	struct playeragent *pa = (struct playeragent *)user_data;
	uint8_t *ptr;
	uint8_t error_code = AVRCP_ERR_INTERNAL_ERROR;
	uint32_t song_len;
	uint32_t song_pos;
	uint8_t play_status;

	LOGD("AVRCP_GET_PLAY_STATUS_RSP");

	/* steal_reply will always return non-NULL since the callback
	 * is only called after a reply has been received */
	msg = dbus_pending_call_steal_reply(call);

	dbus_error_init(&err);
	if (dbus_set_error_from_message(&err, msg)) {
		if (g_str_equal(DBUS_ERROR_UNKNOWN_METHOD, err.name) ||
				g_str_equal(DBUS_ERROR_NO_REPLY, err.name)) {
			dbus_error_free(&err);
			goto error;
		}
	} else {
		resp = avrcp_init_response(pa->pdu_id, AVCTP_PACKET_SINGLE, 9);
		if (!resp)
			goto error;

		dbus_error_init(&err);
		if (!dbus_message_get_args(msg, &err,
					DBUS_TYPE_UINT32, &song_len,
					DBUS_TYPE_UINT32, &song_pos,
					DBUS_TYPE_BYTE, &play_status,
					DBUS_TYPE_INVALID)) {
			dbus_error_free(&err);
			goto error;
		}

		set_ctype_code(pa->headers, CTYPE_STABLE); // Changed ctype to STABLE. It was ACCEPTED before.

		ptr = (uint8_t*)(resp) + AVRCP_RESPONSE_PDU_LEN;
		/* Song Length in milliseconds */
		store_u32(&ptr, song_len);
		/* Song Position in milliseconds */
		store_u32(&ptr, song_pos);
		/* Play Status */
		store_u8(&ptr, play_status);

		avrcp_create_response(pa->control, pa->headers, resp);
		/*cleaning*/
		goto clean;
	}

error:
	avrcp_error_response(pa->control, pa->headers, pa->pdu_id, error_code);
clean:
	if (resp)
		free(resp);
	if (msg)
		dbus_message_unref(msg);
	if (pa)
		dbus_pending_call_cancel(pa->call);

	dbus_pending_call_unref(call);
	avrcp_playeragent_free(&pa);
}

static uint8_t avrcp_get_company(uint8_t *buf,
			struct avrcp_query *query,
			struct avrcp_response **resp)
{
	uint8_t *ptr;
	struct avrcp_response *r;

	LOGD("AVRCP_COMPANY_ID");
	r = avrcp_init_response(query->pdu_id, AVCTP_PACKET_SINGLE, 5);
	if (!r)
		return AVRCP_REPLY_ERR;

	ptr = (uint8_t*)(r) + AVRCP_RESPONSE_PDU_LEN;
	/*Capability ID: AVRCP Company ID */
	store_u8(&ptr, AVRCP_COMPANY_ID);
	/* Capability Count */
	store_u8(&ptr, 1);
	/* Company ID */
	memcpy(ptr, (uint8_t *)AVRCP_BLUETOOTH_COMPANY_ID, 3);

	set_ctype_code(buf, CTYPE_STABLE);
	*resp = r;

	return AVRCP_REPLY_OK;
}

static uint8_t avrcp_get_events_supported(struct control *control,
						struct avrcp_query *query,
						struct playeragent *pa)
{
	DBusMessage *msg;
	struct control_adapter *cadapter;

	LOGD("AVRCP_GET_EVENTS_SUPPORTED");
	/* Find adapter with media player attached */
	cadapter = find_adapter(adapters,
			device_get_adapter(control->dev->btd_dev));

	if (!cadapter)
		return AVRCP_REPLY_ERR;

	if (!cadapter->player)
		return AVRCP_REPLY_ERR;

	/* DBus interface Start*/
	msg = dbus_message_new_method_call(cadapter->player->agent_name,
					cadapter->player->agent_path,
					AVRCP_CONTROL_AVRCP_SRV,
					"GetSupportedEvents");

	if (!dbus_message_append_args(msg,
					DBUS_TYPE_OBJECT_PATH,
					&pa->control->dev->path,
					DBUS_TYPE_INVALID)) {
		LOGE("Could not add argument to msg.");
		dbus_message_unref(msg);
		return AVRCP_REPLY_ERR;
	}

	if (connection) {
		(void)dbus_connection_send_with_reply(connection, msg,
						 &pa->call, -1);
	} else {
		LOGE("DBUS connection problem");
		dbus_message_unref(msg);
		return AVRCP_REPLY_ERR;
	}

	if (!pa->call) {
		LOGE( "Pending Call Null");
		dbus_message_unref(msg);
		return AVRCP_REPLY_ERR;
	}

	dbus_pending_call_set_notify(pa->call,
			avrcp_supported_events_rsp, pa, NULL);

	dbus_message_unref(msg);
	return AVRCP_REPLY_NONE;
}

static uint8_t avrcp_list_player_setting_attrs(struct control *control,
					struct avrcp_query *query,
					struct playeragent *pa)
{
	/* Variables for DBus */
	DBusMessage *msg;
	struct control_adapter *cadapter;

	/* Find adapter with media player attached */
	cadapter = find_adapter(adapters,
			device_get_adapter(control->dev->btd_dev));

	if (!cadapter)
		return AVRCP_REPLY_ERR;

	if (!cadapter->player)
		return AVRCP_REPLY_ERR;

	/* DBus interface Start*/
	msg = dbus_message_new_method_call(cadapter->player->agent_name,
					cadapter->player->agent_path,
					AVRCP_CONTROL_AVRCP_SRV,
					"ListPlayerSettingAttrs");

	if (!dbus_message_append_args(msg,
					DBUS_TYPE_OBJECT_PATH,
					&pa->control->dev->path,
					DBUS_TYPE_INVALID)) {
		LOGE("Could not add argument to msg.");
		dbus_message_unref(msg);
		return AVRCP_REPLY_ERR;
	}

	if (connection) {
		(void)dbus_connection_send_with_reply(connection,
							msg, &pa->call, -1);
	} else {
		LOGE("DBUS connection problem");
		dbus_message_unref(msg);
		return AVRCP_REPLY_ERR;
	}

	if (!pa->call) {
		LOGE( "Pending Call Null");
		dbus_message_unref(msg);
		return AVRCP_REPLY_ERR;
	}

	dbus_pending_call_set_notify(pa->call,
			avrcp_list_player_setting_attrs_rsp, pa, NULL);

	dbus_message_unref(msg);
	return AVRCP_REPLY_NONE;
}

static uint8_t avrcp_list_player_setting_vals(struct control *control,
					struct avrcp_query *query,
					struct playeragent *pa)
{
	/* Variables for DBus */
	DBusMessage *msg;
	struct control_adapter *cadapter;
	uint8_t attr_id;
	uint8_t *ptr;

	query->param_len = be_to_host16((uint8_t *)&query->param_len);
	if (query->param_len != 1) {
		LOGE("Wrong param len = %d", query->param_len);
		return AVRCP_REPLY_ERR;
	}

	ptr = (uint8_t *)query->param;
	attr_id = load_u8(&ptr);

	/* Find adapter with media player attached */
	cadapter = find_adapter(adapters,
			device_get_adapter(control->dev->btd_dev));

	if (!cadapter)
		return AVRCP_REPLY_ERR;

	if (!cadapter->player)
		return AVRCP_REPLY_ERR;

	/* DBus interface Start*/
	msg = dbus_message_new_method_call(cadapter->player->agent_name,
					cadapter->player->agent_path,
					AVRCP_CONTROL_AVRCP_SRV,
					"ListPlayerSettingVals");

	if (!dbus_message_append_args(msg,
					DBUS_TYPE_OBJECT_PATH,
					&pa->control->dev->path,
					DBUS_TYPE_BYTE,
					&attr_id,
					DBUS_TYPE_INVALID)) {
		LOGE("Could not add argument to msg.");
		dbus_message_unref(msg);
		return AVRCP_REPLY_ERR;
	}
	/* Added this to report "rejected" to equilizer and scan listval commands */
	if ((attr_id == 0x01)||(attr_id == 0x04)) {
		LOGD("List_player_setting_vals param id not supported");
		return 100; //Internal return state to represent invalid param
	}
	if (connection) {
		(void)dbus_connection_send_with_reply(connection,
							msg, &pa->call, -1);
	} else {
		LOGE("DBUS connection problem");
		dbus_message_unref(msg);
		return AVRCP_REPLY_ERR;
	}

	if (!pa->call) {
		LOGE( "Pending Call Null");
		dbus_message_unref(msg);
		return AVRCP_REPLY_ERR;
	}

	dbus_pending_call_set_notify(pa->call,
			avrcp_list_player_setting_vals_rsp, pa, NULL);

	dbus_message_unref(msg);
	return AVRCP_REPLY_NONE;
}

static uint8_t avrcp_get_player_setting_value(struct control *control,
					struct avrcp_query *query,
					struct playeragent *pa)
{
	/* Variables for DBus */
	DBusMessage *msg;
	struct control_adapter *cadapter;
	uint8_t attr_count;
	uint8_t *ptr;

	query->param_len = be_to_host16((uint8_t *)&query->param_len);
	if (query->param_len < 1) {
		LOGE("Wrong param len = %d", query->param_len);
		return AVRCP_REPLY_ERR;
	}

	ptr = (uint8_t *)query->param;
	attr_count = load_u8(&ptr);

	/* Find adapter with media player attached */
	cadapter = find_adapter(adapters,
			device_get_adapter(control->dev->btd_dev));
	LOGD("AVRCP_GET_PLAYER_SETTING_VALUE");

	if (!cadapter)
		return AVRCP_REPLY_ERR;

	if (!cadapter->player)
		return AVRCP_REPLY_ERR;

	/* DBus interface Start*/
	msg = dbus_message_new_method_call(cadapter->player->agent_name,
					cadapter->player->agent_path,
					AVRCP_CONTROL_AVRCP_SRV,
					"GetPlayerSettingVal");

	if (!dbus_message_append_args(msg,
					DBUS_TYPE_OBJECT_PATH,
					&pa->control->dev->path,
					DBUS_TYPE_ARRAY,
					DBUS_TYPE_BYTE,
					&ptr,
					attr_count,
					DBUS_TYPE_INVALID)) {
		LOGE("Could not add argument to msg.");
		dbus_message_unref(msg);
		return AVRCP_REPLY_ERR;
	}

	if (connection) {
		(void)dbus_connection_send_with_reply(connection,
							msg, &pa->call, -1);
	} else {
		LOGE("DBUS connection problem");
		dbus_message_unref(msg);
		return AVRCP_REPLY_ERR;
	}

	if (!pa->call) {
		LOGE( "Pending Call Null");
		dbus_message_unref(msg);
		return AVRCP_REPLY_ERR;
	}

	dbus_pending_call_set_notify(pa->call,
			avrcp_get_player_setting_value_rsp, pa, NULL);

	dbus_message_unref(msg);
	return AVRCP_REPLY_NONE;
}

static uint8_t avrcp_set_player_setting_value(struct control *control,
					struct avrcp_query *query,
					struct playeragent *pa)
{
	/* Variables for DBus */
	DBusMessage *msg;
	struct control_adapter *cadapter;
	uint8_t attr_count;
	uint8_t *ptr;

	query->param_len = be_to_host16((uint8_t *)&query->param_len);
	if (query->param_len < 1) {
		LOGE("Wrong param len = %d", query->param_len);
		return AVRCP_REPLY_ERR;
	}

	ptr = (uint8_t *)query->param;
	attr_count = load_u8(&ptr);

	/* Find adapter with media player attached */
	cadapter = find_adapter(adapters,
			device_get_adapter(control->dev->btd_dev));

	if (!cadapter)
		return AVRCP_REPLY_ERR;

	if (!cadapter->player)
		return AVRCP_REPLY_ERR;

	/* DBus interface Start*/
	msg = dbus_message_new_method_call(cadapter->player->agent_name,
					cadapter->player->agent_path,
					AVRCP_CONTROL_AVRCP_SRV,
					"SetPlayerSettingVal");

	if (!dbus_message_append_args(msg,
					DBUS_TYPE_OBJECT_PATH,
					&pa->control->dev->path,
					DBUS_TYPE_ARRAY,
					DBUS_TYPE_BYTE,
					&ptr,
					attr_count * 2,
					DBUS_TYPE_INVALID)) {
		LOGE("Could not add argument to msg.");
		dbus_message_unref(msg);
		return AVRCP_REPLY_ERR;
	}

	if (connection) {
		(void)dbus_connection_send_with_reply(connection,
							msg, &pa->call, -1);
	} else {
		LOGE("DBUS connection problem");
		dbus_message_unref(msg);
		return AVRCP_REPLY_ERR;
	}

	if (!pa->call) {
		LOGE( "Pending Call Null");
		dbus_message_unref(msg);
		return AVRCP_REPLY_ERR;
	}

	dbus_pending_call_set_notify(pa->call,
			avrcp_set_player_setting_value_rsp, pa, NULL);

	dbus_message_unref(msg);
	return AVRCP_REPLY_NONE;
}

static uint8_t avrcp_inform_char_set(struct control *control,
				struct avrcp_query *query,
				 struct playeragent *pa)
{
	/* Variables for DBus */
	DBusMessage *msg;
	struct control_adapter *cadapter;
	uint8_t *ptr;
	uint8_t num_charset;
	uint16_t charset_id;

	query->param_len = be_to_host16((uint8_t *)&query->param_len);
	if (!query->param_len) {
		LOGE("Wrong param len = %d", query->param_len);
		return AVRCP_REPLY_ERR;
	}

	ptr = (uint8_t *)query->param;
	num_charset = load_u8(&ptr);
	charset_id = load_u16(&ptr);

	/* Find adapter with media player attached */
	cadapter = find_adapter(adapters,
			device_get_adapter(control->dev->btd_dev));

	if (!cadapter)
		return AVRCP_REPLY_ERR;

	if (!cadapter->player)
		return AVRCP_REPLY_ERR;

	/* DBus interface Start*/
	msg = dbus_message_new_method_call(cadapter->player->agent_name,
					   cadapter->player->agent_path,
					AVRCP_CONTROL_AVRCP_SRV,
					"DisplayCharacterSet");

	if (!dbus_message_append_args(msg,
					DBUS_TYPE_OBJECT_PATH,
					&pa->control->dev->path,
					DBUS_TYPE_BYTE,
					&num_charset,
					DBUS_TYPE_UINT16,
					&charset_id,
					DBUS_TYPE_INVALID)) {
		LOGE("Could not add argument to msg.");
		dbus_message_unref(msg);
		return AVRCP_REPLY_ERR;
	}

	if (connection) {
		g_dbus_send_message(connection, msg);
	} else {
		LOGE("DBUS connection problem");
		dbus_message_unref(msg);
		return AVRCP_REPLY_ERR;
	}

	dbus_message_unref(msg);
	return AVRCP_REPLY_NONE;
}

static uint8_t avrcp_inform_batt_stat(struct control *control,
				struct avrcp_query *query,
				struct playeragent *pa)
{
	/* Variables for DBus */
	DBusMessage *msg;
	struct control_adapter *cadapter;

	if (query->param[0] >= 5)
		return AVRCP_REPLY_ERR;

	/* Find adapter with media player attached */
	cadapter = find_adapter(adapters,
			device_get_adapter(control->dev->btd_dev));

	if (!cadapter)
		return AVRCP_REPLY_ERR;

	if (!cadapter->player)
		return AVRCP_REPLY_ERR;

	/* DBus interface Start*/
	msg = dbus_message_new_method_call(cadapter->player->agent_name,
					cadapter->player->agent_path,
					AVRCP_CONTROL_AVRCP_SRV,
					"BatteryStatus");

	if (!dbus_message_append_args(msg,
					DBUS_TYPE_OBJECT_PATH,
					&pa->control->dev->path,
					DBUS_TYPE_BYTE,
					&query->param[0],
					DBUS_TYPE_INVALID)) {
		LOGE("Could not add argument to msg.");
		dbus_message_unref(msg);
		return AVRCP_REPLY_ERR;
	}

	if (connection) {
		g_dbus_send_message(connection, msg);
	} else {
		LOGE("DBUS connection problem");
		dbus_message_unref(msg);
		return AVRCP_REPLY_ERR;
	}

	dbus_message_unref(msg);
	return AVRCP_REPLY_NONE;
}

static uint8_t avrcp_get_element_attr(struct control *control,
				struct avrcp_query *query,
				struct playeragent *pa)
{
	/* Variables for DBus */
	DBusMessage *msg;
	struct control_adapter *cadapter;
	uint8_t i;
	uint8_t *ptr;
	uint64_t identifier;
	uint8_t attr_count;
	uint32_t *pattr;

	query->param_len = be_to_host16((uint8_t *)&query->param_len);

	if (query->param_len < 9) {
		LOGE("Wrong param len = %d", query->param_len);
		return AVRCP_REPLY_ERR;
	}

	ptr = (uint8_t *)query->param;
	identifier = load_u64(&ptr);
	attr_count = load_u8(&ptr);

	pattr = g_malloc(attr_count * sizeof(uint32_t));
	if (!pattr && attr_count) {
		LOGE("memory not available");
		return AVRCP_REPLY_ERR;
	}

	LOGD("AVRCP_ELEMENT_ATTRIBUTES");
	if (identifier != 0x00) {
		LOGE("Only Playing is supported");
		return AVRCP_REPLY_ERR;
	}

	/* Find adapter with media player attached */
	cadapter = find_adapter(adapters,
			device_get_adapter(control->dev->btd_dev));

	if (!cadapter)
		return AVRCP_REPLY_ERR;

	if (!cadapter->player)
		return AVRCP_REPLY_ERR;

	/* DBus interface Start*/
	msg = dbus_message_new_method_call(cadapter->player->agent_name,
					cadapter->player->agent_path,
					AVRCP_CONTROL_AVRCP_SRV,
					"GetElementAttributes");

	memcpy(pattr, ptr, attr_count * sizeof(uint32_t));
	for (i = 0; i < attr_count; i++) {
		*(pattr+i) = be_to_host32((const uint8_t *)(pattr+i));
	}

	/*Add all atributes do DBUS msg.*/
	if (!dbus_message_append_args(msg,
					DBUS_TYPE_OBJECT_PATH,
					&pa->control->dev->path,
					DBUS_TYPE_ARRAY,
					DBUS_TYPE_UINT32,
					&pattr,
					attr_count,
					DBUS_TYPE_INVALID)) {
		LOGE("Could not add argument to msg.");
		dbus_message_unref(msg);
		g_free(pattr);
		return AVRCP_REPLY_ERR;
	}
	g_free(pattr);

	if (connection) {
		(void)dbus_connection_send_with_reply(connection,
						msg, &pa->call, -1);
	} else {
		LOGE("DBUS connection problem");
		dbus_message_unref(msg);
		return AVRCP_REPLY_ERR;
	}

	if (!pa->call) {
		LOGE( "Pending Call Null");
		dbus_message_unref(msg);
		return AVRCP_REPLY_ERR;
	}

	dbus_pending_call_set_notify(pa->call,
			avrcp_get_element_attr_rsp, pa, NULL);

	dbus_message_unref(msg);
	return AVRCP_REPLY_NONE;
}

static uint8_t avrcp_get_play_status(struct control *control,
					struct avrcp_query *query,
					struct playeragent *pa)
{
	/* Variables for DBus */
	DBusMessage *msg;
	struct control_adapter *cadapter;

	/* Find adapter with media player attached */
	cadapter = find_adapter(adapters,
			device_get_adapter(control->dev->btd_dev));

	if (!cadapter)
		return AVRCP_REPLY_ERR;

	if (!cadapter->player)
		return AVRCP_REPLY_ERR;

	/* DBus interface Start*/
	msg = dbus_message_new_method_call(cadapter->player->agent_name,
					cadapter->player->agent_path,
					AVRCP_CONTROL_AVRCP_SRV,
					"GetPlayStatus");

	if (!dbus_message_append_args(msg,
					DBUS_TYPE_OBJECT_PATH,
					&pa->control->dev->path,
					DBUS_TYPE_INVALID)) {
		LOGE("Could not add argument to msg.");
		dbus_message_unref(msg);
		return AVRCP_REPLY_ERR;
	}

	if (connection) {
		(void)dbus_connection_send_with_reply(connection,
							msg, &pa->call, -1);
	} else {
		LOGE("DBUS connection problem");
		dbus_message_unref(msg);
		return AVRCP_REPLY_ERR;
	}

	if (!pa->call) {
		LOGE( "Pending Call Null");
		dbus_message_unref(msg);
		return AVRCP_REPLY_ERR;
	}

	dbus_pending_call_set_notify(pa->call,
			avrcp_get_play_status_rsp, pa, NULL);

	return AVRCP_REPLY_NONE;
}

static uint8_t avrcp_register_notification(struct control *control,
						struct avrcp_query *query,
						struct playeragent *pa)
{
	/* Variables for DBus */
	DBusMessage *msg;
	struct control_adapter *cadapter;
	uint8_t *ptr;
	uint8_t event;
	uint32_t playback;

	query->param_len = be_to_host16((uint8_t *)&query->param_len);

	if (query->param_len != 5) {
		LOGE("Wrong param len = %d", query->param_len);
		return AVRCP_REPLY_ERR;
	}

	ptr = (uint8_t *)query->param;
	event = load_u8(&ptr);
	playback = load_u32(&ptr);

	/* Find adapter with media player attached */
	cadapter = find_adapter(adapters,
			device_get_adapter(control->dev->btd_dev));

	if (!cadapter)
		return AVRCP_REPLY_ERR;

	if (!cadapter->player)
		return AVRCP_REPLY_ERR;

	/* DBus interface Start*/
	msg = dbus_message_new_method_call(cadapter->player->agent_name,
					cadapter->player->agent_path,
					AVRCP_CONTROL_AVRCP_SRV,
					"RegisterNotification");

	/*Add parameters.*/
	if (!dbus_message_append_args(msg,
					DBUS_TYPE_OBJECT_PATH,
					&pa->control->dev->path,
					DBUS_TYPE_BYTE,
					&event,
					DBUS_TYPE_UINT32,
					&playback,
					DBUS_TYPE_INVALID)) {
		LOGE("Could not add argument to msg.");
		dbus_message_unref(msg);
		return AVRCP_REPLY_ERR;
	}

	if (connection) {
		(void)dbus_connection_send_with_reply(connection,
						msg, &pa->call, -1);
	} else {
		LOGE("DBUS connection problem");
		dbus_message_unref(msg);
		return AVRCP_REPLY_ERR;
	}

	if (!pa->call) {
		LOGE( "Pending Call Null");
		dbus_message_unref(msg);
		return AVRCP_REPLY_ERR;
	}

	/*Remember evet_id*/
	pa->event_id = event;
	dbus_pending_call_set_notify(pa->call, avrcp_register_notification_rsp,
					 pa, NULL);

	dbus_message_unref(msg);
	return AVRCP_REPLY_NONE;
}

static uint8_t avrcp_request_cont_resp(uint8_t *buf,
					struct control *control,
					struct avrcp_query *query,
					struct avrcp_response **resp)
{
	LOGD("AVRCP_REQUEST_CONT_RESP");

	if (control->cont) {
		set_ctype_code(buf, CTYPE_STABLE);
		if (control->cont->resp) {
			*resp = control->cont->resp;
			return AVRCP_REPLY_OK;
		} else
			return AVRCP_REPLY_ERR;
	}

	return AVRCP_REPLY_ERR;
}

static uint8_t avrcp_abort_cont_resp(uint8_t *buf,
					struct control *control,
					struct avrcp_query *query,
					struct avrcp_response **resp)
{
	struct avrcp_response *r;
	uint8_t code;
	uint16_t len;

	LOGD("AVRCP_ABORT_CONT_RESP");

	r = avrcp_init_response(query->pdu_id, AVCTP_PACKET_SINGLE, 0);
	if (!r)
		return AVRCP_REPLY_ERR;

	len = be_to_host16((uint8_t *)&(query->param_len));

	if (len == 1) {
		if (control->cont &&
			/* query->param[0] is byte */
			(query->param[0] == control->cont->pdu_id)) {

			if (control->cont->resp)
				free(control->cont->resp);

			free(control->cont);
			control->cont = NULL;
			code = CTYPE_ACCEPTED;
		} else {
			code = CTYPE_REJECTED;
		}
	} else {
		code = CTYPE_REJECTED;
	}

	set_ctype_code(buf, code);

	*resp = r;
	return AVRCP_REPLY_OK;
}

static void handle_panel_vendordependent(struct control *control,
					uint8_t *buf,
					uint16_t packet_size)
{
	struct avrcp_query *query;
	struct avrcp_response *resp = NULL;
	struct playeragent *pa;
	uint16_t len;
	int status;
	int err = AVRCP_ERR_INTERNAL_ERROR;

	query = (struct avrcp_query *) (buf + AVCTP_AVRCP_HEADER_LEN);

	len = be_to_host16((uint8_t *)&(query->param_len));

	pa = avrcp_playeragent_new(control, buf);
	if (!pa) {
		status = AVRCP_REPLY_ERR;
		goto finish;
	}

	switch (query->pdu_id) {
	case AVRCP_GET_CAPABILITIES:
		if (len != 1) {
			status = AVRCP_REPLY_ERR;
			err = AVRCP_ERR_PARM_NOT_FOUND;
			break;
		}

		switch (query->param[0]) {
		case AVRCP_COMPANY_ID:
			status = avrcp_get_company(buf, query, &resp);
			break;
		case AVRCP_EVENTS_SUPPORTED:
			status = avrcp_get_events_supported(control, query, pa);
			break;
		default:/* Error: capability asked is not valid */
			LOGE("capability asked is invalid 0x%02x",
							query->param[0]);
			status = AVRCP_REPLY_ERR;
			err = AVRCP_ERR_INVALID_PARM;
			break;
		}
		break;
	case AVRCP_LIST_PLAYER_SETTING_ATTRS:
		LOGD("AVRCP_LIST_PLAYER_SETTING_ATTRS");
		status = avrcp_list_player_setting_attrs(control, query, pa);
		break;

	case AVRCP_LIST_PLAYER_SETTING_VALUES:
		LOGD("AVRCP_LIST_PLAYER_SETTING_VALUES");	
		status = avrcp_list_player_setting_vals(control, query, pa);
		/* Adding this check to send Rejected response if Equilizer or scan's values
		* are asked since we do not support it, but still this command was sent
		* by CT
		*/
		if (status == 100) {
			status = AVRCP_REPLY_ERR;
			err = AVRCP_ERR_INVALID_PARM;
		}
		break;

	case AVRCP_GET_PLAYER_SETTING_VALUE:
		LOGD("AVRCP_GET_PLAYER_SETTING_VALUE");
		status = avrcp_get_player_setting_value(control, query, pa);
		break; 
		
	case AVRCP_SET_PLAYER_SETTING_VALUE:
		LOGD("AVRCP_SET_PLAYER_SETTING_VALUE");
		status = avrcp_set_player_setting_value(control, query, pa);
		break;
	case AVRCP_GET_PLAYER_SETTING_ATTR_TEXT:
	case AVRCP_GET_PLAYER_SETTING_VALUE_TEXT:
		resp = avrcp_init_response(query->pdu_id,
					AVCTP_PACKET_SINGLE , 0);
		LOGE("PDU_ID 0x%x  NOT SUPPORTED", query->pdu_id);
		set_ctype_code(buf, CTYPE_NOT_IMPLEMENTED);
		status = AVRCP_REPLY_OK;
		break;
	case AVRCP_INFORM_DISP_CHAR_SET:
		LOGD("AVRCP_INFORM_DISP_CHAR_SET");
		status = avrcp_inform_char_set(control, query, pa);
		break;
	case AVRCP_INFORM_BATT_STATUS:
		LOGD("AVRCP_INFORM_BATT_STATUS");
		status = avrcp_inform_batt_stat(control, query, pa);
		break;
	case AVRCP_GET_ELEMENT_ATTRIBUTES:
		status = avrcp_get_element_attr(control, query, pa);
		break;
	case AVRCP_GET_PLAY_STATUS:
		LOGD("AVRCP_GET_PLAY_STATUS");
		status = avrcp_get_play_status(control, query, pa);
		break;
	case AVRCP_REGISTER_NOTIFY:
		LOGD("AVRCP_REGISTER_NOTIFY");
		status = avrcp_register_notification(control, query, pa);
		break;
	case AVRCP_REQUEST_CONT_RESP:
		status = avrcp_request_cont_resp(buf, control, query, &resp);
		break;
	case AVRCP_ABORT_CONT_RESP:
		status = avrcp_abort_cont_resp(buf, control, query, &resp);
		break;
	default:
		LOGE("PDU ID Not Found");
		status = AVRCP_REPLY_ERR;
		err = AVRCP_ERR_INVALID_CMD;
		break;
	}

finish:
	if (status == AVRCP_REPLY_OK) {
		avrcp_create_response(control, buf, resp);
	} else if (status == AVRCP_REPLY_ERR) {
		avrcp_error_response(control, pa->headers, pa->pdu_id,
				err);
	}

	if (status != AVRCP_REPLY_NONE) {
		avrcp_playeragent_free(&pa);
		if (resp && !control->cont) {
			g_free(resp);
		}
	}

	return;
}

static DBusMessage *register_player(DBusConnection *conn,
					DBusMessage *msg,
					void *data)
{
	DBusMessageIter iter;
	struct control_adapter *adapter = data;
	struct media_player *player;
	char *path;

	if (adapter->player)
		return g_dbus_create_error(msg, ERROR_INTERFACE ".AlreadyRegistered",
					"Player already registered");

	player = g_try_new0(struct media_player, 1);
	if (!player)
		return g_dbus_create_error(msg, ERROR_INTERFACE ".OutOfMemory",
					"Cannot allocate media player");

	dbus_message_iter_init(msg, &iter);

	if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_OBJECT_PATH) {
		g_free(player);
		return g_dbus_create_error(msg, ERROR_INTERFACE ".InvalidArguments",
					 "Invalid arguments in method call");
	}

	dbus_message_iter_get_basic(&iter, &path);
	dbus_message_iter_next(&iter);

	player->agent_path = g_strdup(path);
	player->agent_name = g_strdup(dbus_message_get_sender(msg));

	adapter->player = player;

	return dbus_message_new_method_return(msg);
}

static DBusMessage *unregister_player(DBusConnection *conn,
					DBusMessage *msg,
					void *data)
{
	DBusMessageIter iter;
	struct control_adapter *adapter = data;
	char *path;

	if (!adapter->player)
		return g_dbus_create_error(msg, ERROR_INTERFACE ".NotRegistered",
						 "Player not registered");

	dbus_message_iter_init(msg, &iter);

	if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_OBJECT_PATH)
		return g_dbus_create_error(msg, ERROR_INTERFACE ".InvalidArguments",
						 "Invalid arguments in method call");

	dbus_message_iter_get_basic(&iter, &path);

	if (strcmp(path, adapter->player->agent_path))
		return g_dbus_create_error(msg, ERROR_INTERFACE ".InvalidArguments",
				 "Cannot unregister player which is not registered");

	g_free(adapter->player->agent_path);
	g_free(adapter->player->agent_name);
	g_free(adapter->player);
	adapter->player = NULL;

	return dbus_message_new_method_return(msg);
}

static GDBusMethodTable controladapter_methods[] = {
	{ "RegisterPlayer",	"o",	"",	register_player },
	{ "UnregisterPlayer",	"o",	"",	unregister_player },
	{ NULL, NULL, NULL, NULL }
};

int avrcp_adapter_register(DBusConnection *conn, struct btd_adapter *btd_adapter, GKeyFile *config)
{
	GError *err = NULL;

	if (config) {
		avrcpmetadata = g_key_file_get_boolean(config,
					"AVRCP", "MetaDataEnable", &err);
		if (err) {
			LOGD("audio.conf: %s", err->message);
			avrcpmetadata = false;
			g_error_free(err);
		}
	}
	
	if (!avrcpmetadata)	return 0;		

	const char *path = adapter_get_path(btd_adapter);
	struct control_adapter *adapter;

	adapter = g_try_new0(struct control_adapter, 1);

	if (!g_dbus_register_interface(conn, path, AVRCP_CONTROL_ADAPTER,
					controladapter_methods, NULL, NULL,
					adapter, NULL)) {
		error("D-Bus failed to register %s interface",
					AVRCP_CONTROL_ADAPTER);
		g_free(adapter);
		return -EIO;
	}

	adapter->btd_adapter = btd_adapter_ref(btd_adapter);
	adapter->conn = dbus_connection_ref(conn);

	adapters = g_slist_prepend(adapters, adapter);

	debug("Registered interface %s on path %s", AVRCP_CONTROL_ADAPTER, path);

	return 0;
}

void avrcp_adapter_unregister(struct btd_adapter *btd_adapter)
{
	if (!avrcpmetadata)
		return ;
	struct control_adapter *adapter;

	adapter = find_adapter(adapters, btd_adapter);
	if (!adapter)
		return;

	g_dbus_unregister_interface(adapter->conn,
		adapter_get_path(btd_adapter), AVRCP_CONTROL_ADAPTER);
	dbus_connection_unref(adapter->conn);
	btd_adapter_unref(adapter->btd_adapter);
}

static void handle_panel_passthrough(struct control *control,
					const unsigned char *operands,
					int operand_count)
{
	const char *status;
	int pressed, i;

	if (operand_count == 0)
		return;

	if (operands[0] & 0x80) {
		status = "released";
		pressed = 0;
	} else {
		status = "pressed";
		pressed = 1;
	}

	for (i = 0; key_map[i].name != NULL; i++) {
		uint8_t key_quirks;

		if ((operands[0] & 0x7F) != key_map[i].avrcp)
			continue;

		DBG("AVRCP: %s %s", key_map[i].name, status);

		key_quirks = control->key_quirks[key_map[i].avrcp];

		if (key_quirks & QUIRK_NO_RELEASE) {
			if (!pressed) {
				DBG("AVRCP: Ignoring release");
				break;
			}

			DBG("AVRCP: treating key press as press + release");
			send_key(control->uinput, key_map[i].uinput, 1);
			send_key(control->uinput, key_map[i].uinput, 0);
			break;
		}

		send_key(control->uinput, key_map[i].uinput, pressed);
		break;
	}

	if (key_map[i].name == NULL)
		DBG("AVRCP: unknown button 0x%02X %s",
						operands[0] & 0x7F, status);
}

static void avctp_disconnected(struct audio_device *dev)
{
	struct control *control = dev->control;

	if (!control)
		return;

	if (control->io) {
		g_io_channel_shutdown(control->io, TRUE, NULL);
		g_io_channel_unref(control->io);
		control->io = NULL;
	}

	if (control->io_id) {
		g_source_remove(control->io_id);
		control->io_id = 0;

		if (control->state == AVCTP_STATE_CONNECTING)
			audio_device_cancel_authorization(dev, auth_cb,
								control);
	}

	if (control->uinput >= 0) {
		char address[18];

		ba2str(&dev->dst, address);
		DBG("AVRCP: closing uinput for %s", address);

		ioctl(control->uinput, UI_DEV_DESTROY);
		close(control->uinput);
		control->uinput = -1;
	}
	
	if (control->cont) {
		if (control->cont->resp)
			free(control->cont->resp);
		free(control->cont);
		control->cont = NULL;
	}
}

static void avctp_set_state(struct control *control, avctp_state_t new_state)
{
	GSList *l;
	struct audio_device *dev = control->dev;
	avctp_state_t old_state = control->state;
	gboolean value;

	switch (new_state) {
	case AVCTP_STATE_DISCONNECTED:
		DBG("AVCTP Disconnected");

		avctp_disconnected(control->dev);

		if (old_state != AVCTP_STATE_CONNECTED)
			break;

		value = FALSE;
		g_dbus_emit_signal(dev->conn, dev->path,
					AUDIO_CONTROL_INTERFACE,
					"Disconnected", DBUS_TYPE_INVALID);
		emit_property_changed(dev->conn, dev->path,
					AUDIO_CONTROL_INTERFACE, "Connected",
					DBUS_TYPE_BOOLEAN, &value);

		if (!audio_device_is_active(dev, NULL))
			audio_device_set_authorized(dev, FALSE);

		break;
	case AVCTP_STATE_CONNECTING:
		DBG("AVCTP Connecting");
		break;
	case AVCTP_STATE_CONNECTED:
		DBG("AVCTP Connected");
		value = TRUE;
		g_dbus_emit_signal(control->dev->conn, control->dev->path,
				AUDIO_CONTROL_INTERFACE, "Connected",
				DBUS_TYPE_INVALID);
		emit_property_changed(control->dev->conn, control->dev->path,
				AUDIO_CONTROL_INTERFACE, "Connected",
				DBUS_TYPE_BOOLEAN, &value);
		break;
	default:
		error("Invalid AVCTP state %d", new_state);
		return;
	}

	control->state = new_state;

	for (l = avctp_callbacks; l != NULL; l = l->next) {
		struct avctp_state_callback *cb = l->data;
		cb->cb(control->dev, old_state, new_state, cb->user_data);
	}
}


//WTL_EDM_START
#ifdef SAMSUNG_EDM
char* getPropValue (const char* propName, FILE* fp)
{
    char line[128];
	
    //start reading stream
    while (fgets(line, sizeof(line), fp)) {
        char *pos = strstr(line, propName);
        if (NULL != pos) {
            //move pointer to value location for this just skip strlen (propName)+strlen(=)
            pos +=strlen (propName)+1;
            char *end = pos;
            //in order to get value reach at the end of line
            while ( *end++ != '\n');
            int len = end-pos;
            //just copy the value in to temp buffer
            char * propValue = malloc (len + 1);
            if (NULL != strncpy( propValue, pos, len)) {
                return propValue;
            }
            else {
                return NULL;
            }
        }
    }
    return NULL;
}
#endif
//WTL_EDM_END

static gboolean control_cb(GIOChannel *chan, GIOCondition cond,
				gpointer data)
{
	struct control *control = data;
	unsigned char buf[1024], *operands;
	struct avctp_header *avctp;
	struct avrcp_header *avrcp;
	int ret, packet_size, operand_count, sock;
	gboolean resp_ready = TRUE;

	if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL))
		goto failed;

	sock = g_io_channel_unix_get_fd(control->io);

	ret = read(sock, buf, sizeof(buf));
	if (ret <= 0)
		goto failed;

	DBG("Got %d bytes of data for AVCTP session %p", ret, control);

	if ((unsigned int) ret < sizeof(struct avctp_header)) {
		error("Too small AVCTP packet");
		goto failed;
	}

	packet_size = ret;

	avctp = (struct avctp_header *) buf;

	DBG("AVCTP transaction %u, packet type %u, C/R %u, IPID %u, "
			"PID 0x%04X",
			avctp->transaction, avctp->packet_type,
			avctp->cr, avctp->ipid, ntohs(avctp->pid));

	ret -= sizeof(struct avctp_header);
	if ((unsigned int) ret < sizeof(struct avrcp_header)) {
		error("Too small AVRCP packet");
		goto failed;
	}

	avrcp = (struct avrcp_header *) (buf + sizeof(struct avctp_header));

	ret -= sizeof(struct avrcp_header);

	operands = buf + sizeof(struct avctp_header) + sizeof(struct avrcp_header);
	operand_count = ret;

	DBG("AVRCP %s 0x%01X, subunit_type 0x%02X, subunit_id 0x%01X, "
			"opcode 0x%02X, %d operands",
			avctp->cr ? "response" : "command",
			avrcp->code, avrcp->subunit_type, avrcp->subunit_id,
			avrcp->opcode, operand_count);

//WTL_EDM_START
#ifdef SAMSUNG_EDM
	int avrcp_block = 0;
	FILE *fp = fopen("/data/system/enterprise.conf", "r");
	if (NULL != fp )  {
		char* propValue = getPropValue ("avrcpBlocked",  fp);
		if(NULL != propValue) {	 	    
	 	      avrcp_block = atoi (propValue);	
		      free(propValue);
	 	 }
	        fclose (fp);
	        fp = NULL;		
	}
#endif
//WTL_EDM_END

	if (avctp->packet_type != AVCTP_PACKET_SINGLE) {
		avctp->cr = AVCTP_RESPONSE;
		avrcp->code = CTYPE_NOT_IMPLEMENTED;
	} else if (avctp->pid != htons(AV_REMOTE_SVCLASS_ID)) {
		avctp->ipid = 1;
		avctp->cr = AVCTP_RESPONSE;
		avrcp->code = CTYPE_REJECTED;
	} else if (avctp->cr == AVCTP_COMMAND &&
			avrcp->code == CTYPE_CONTROL &&
			avrcp->subunit_type == SUBUNIT_PANEL &&
//WTL_EDM_START
#ifdef SAMSUNG_EDM
			!avrcp_block &&
#endif
//WTL_EDM_END
			avrcp->opcode == OP_PASSTHROUGH) {
		handle_panel_passthrough(control, operands, operand_count);
		avctp->cr = AVCTP_RESPONSE;
		avrcp->code = CTYPE_ACCEPTED;
	} else if (avctp->cr == AVCTP_COMMAND &&
			avrcp->code == CTYPE_STATUS &&
			(avrcp->opcode == OP_UNITINFO
			|| avrcp->opcode == OP_SUBUNITINFO)) {
		avctp->cr = AVCTP_RESPONSE;
		avrcp->code = CTYPE_STABLE;
		/* The first operand should be 0x07 for the UNITINFO response.
		 * Neither AVRCP (section 22.1, page 117) nor AVC Digital
		 * Interface Command Set (section 9.2.1, page 45) specs
		 * explain this value but both use it */
		if (operand_count >= 1 && avrcp->opcode == OP_UNITINFO)
			operands[0] = 0x07;
		if (operand_count >= 2)
			operands[1] = SUBUNIT_PANEL << 3;
		DBG("reply to %s", avrcp->opcode == OP_UNITINFO ?
				"OP_UNITINFO" : "OP_SUBUNITINFO");
	} else if (avctp->cr == AVCTP_COMMAND &&
			(avrcp->code == CTYPE_STATUS
			|| avrcp->code == CTYPE_CONTROL
			|| avrcp->code == CTYPE_NOTIFY) &&
//WTL_EDM_START
#ifdef SAMSUNG_EDM
			!avrcp_block &&
#endif
//WTL_EDM_END
			(avrcp->opcode == OP_VENDORDEPENDENT) &&
			avrcpmetadata) {
		avctp->cr = AVCTP_RESPONSE;
		avctp->packet_type = AVCTP_PACKET_SINGLE;
		handle_panel_vendordependent(control, buf, packet_size);
		resp_ready = FALSE;
	} else {
		avctp->cr = AVCTP_RESPONSE;		
		avrcp->code = CTYPE_REJECTED;
	}

	if (resp_ready) {
		ret = write(sock, buf, packet_size);
		if (ret != packet_size)
			goto failed;
	}

	return TRUE;

failed:
	DBG("AVCTP session %p got disconnected", control);
	avctp_set_state(control, AVCTP_STATE_DISCONNECTED);
	return FALSE;
}

static int uinput_create(char *name)
{
	struct uinput_dev dev;
	int fd, err, i;

	fd = open("/dev/uinput", O_RDWR);
	if (fd < 0) {
		fd = open("/dev/input/uinput", O_RDWR);
		if (fd < 0) {
			fd = open("/dev/misc/uinput", O_RDWR);
			if (fd < 0) {
				err = errno;
				error("Can't open input device: %s (%d)",
							strerror(err), err);
				return -err;
			}
		}
	}

	memset(&dev, 0, sizeof(dev));
	if (name)
		strncpy(dev.name, name, UINPUT_MAX_NAME_SIZE - 1);

	dev.id.bustype = BUS_BLUETOOTH;
	dev.id.vendor  = 0x0000;
	dev.id.product = 0x0000;
	dev.id.version = 0x0000;

	if (write(fd, &dev, sizeof(dev)) < 0) {
		err = errno;
		error("Can't write device information: %s (%d)",
						strerror(err), err);
		close(fd);
		errno = err;
		return -err;
	}

	ioctl(fd, UI_SET_EVBIT, EV_KEY);
	ioctl(fd, UI_SET_EVBIT, EV_REL);
	/*SS_BLUETOOTH(chun.ho.park) 2012.02.08*/
	// Remove repeat event, since it automatically generated when pressed key events are recieved continuously
	//ioctl(fd, UI_SET_EVBIT, EV_REP); 
	/*SS_BLUETOOTH(chun.ho.park) End*/
	ioctl(fd, UI_SET_EVBIT, EV_SYN);

	for (i = 0; key_map[i].name != NULL; i++)
		ioctl(fd, UI_SET_KEYBIT, key_map[i].uinput);

	if (ioctl(fd, UI_DEV_CREATE, NULL) < 0) {
		err = errno;
		error("Can't create uinput device: %s (%d)",
						strerror(err), err);
		close(fd);
		errno = err;
		return -err;
	}

	return fd;
}

static void init_uinput(struct control *control)
{
	struct audio_device *dev = control->dev;
	char address[18], name[248 + 1], *uinput_dev_name;

	device_get_name(dev->btd_dev, name, sizeof(name));
	if (g_str_equal(name, "Nokia CK-20W")) {
		control->key_quirks[FORWARD_OP] |= QUIRK_NO_RELEASE;
		control->key_quirks[BACKWARD_OP] |= QUIRK_NO_RELEASE;
		control->key_quirks[PLAY_OP] |= QUIRK_NO_RELEASE;
		control->key_quirks[PAUSE_OP] |= QUIRK_NO_RELEASE;
	}

	ba2str(&dev->dst, address);

	/* Use device name from config file if specified */
	uinput_dev_name = input_device_name;
	if (!uinput_dev_name)
		uinput_dev_name = address;

	control->uinput = uinput_create(uinput_dev_name);
	if (control->uinput < 0)
		error("AVRCP: failed to init uinput for %s", address);
	else
		DBG("AVRCP: uinput initialized for %s", address);
}

static void avctp_connect_cb(GIOChannel *chan, GError *err, gpointer data)
{
	struct control *control = data;
	char address[18];
	uint16_t imtu;
	GError *gerr = NULL;

	if (err) {
		avctp_set_state(control, AVCTP_STATE_DISCONNECTED);
		error("%s", err->message);
		return;
	}

	bt_io_get(chan, BT_IO_L2CAP, &gerr,
			BT_IO_OPT_DEST, &address,
			BT_IO_OPT_IMTU, &imtu,
			BT_IO_OPT_INVALID);
	if (gerr) {
		avctp_set_state(control, AVCTP_STATE_DISCONNECTED);
		error("%s", gerr->message);
		g_error_free(gerr);
		return;
	}

	DBG("AVCTP: connected to %s", address);

	if (!control->io)
		control->io = g_io_channel_ref(chan);

	init_uinput(control);

	avctp_set_state(control, AVCTP_STATE_CONNECTED);
	control->mtu = imtu;
	control->io_id = g_io_add_watch(chan,
				G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL,
				(GIOFunc) control_cb, control);
}

static void auth_cb(DBusError *derr, void *user_data)
{
	struct control *control = user_data;
	GError *err = NULL;

	if (control->io_id) {
		g_source_remove(control->io_id);
		control->io_id = 0;
	}

	if (derr && dbus_error_is_set(derr)) {
		error("Access denied: %s", derr->message);
		avctp_set_state(control, AVCTP_STATE_DISCONNECTED);
		return;
	}

	if (!bt_io_accept(control->io, avctp_connect_cb, control,
								NULL, &err)) {
		error("bt_io_accept: %s", err->message);
		g_error_free(err);
		avctp_set_state(control, AVCTP_STATE_DISCONNECTED);
	}
}

static void avctp_confirm_cb(GIOChannel *chan, gpointer data)
{
	struct control *control = NULL;
	struct audio_device *dev;
	char address[18];
	bdaddr_t src, dst;
	GError *err = NULL;

	bt_io_get(chan, BT_IO_L2CAP, &err,
			BT_IO_OPT_SOURCE_BDADDR, &src,
			BT_IO_OPT_DEST_BDADDR, &dst,
			BT_IO_OPT_DEST, address,
			BT_IO_OPT_INVALID);
	if (err) {
		error("%s", err->message);
		g_error_free(err);
		g_io_channel_shutdown(chan, TRUE, NULL);
		return;
	}

	dev = manager_get_device(&src, &dst, TRUE);
	if (!dev) {
		error("Unable to get audio device object for %s", address);
		goto drop;
	}

	if (!dev->control) {
		btd_device_add_uuid(dev->btd_dev, AVRCP_REMOTE_UUID);
		if (!dev->control)
			goto drop;
	}

	control = dev->control;

	if (control->io) {
		error("Refusing unexpected connect from %s", address);
		goto drop;
	}

	avctp_set_state(control, AVCTP_STATE_CONNECTING);
	control->io = g_io_channel_ref(chan);

	if (audio_device_request_authorization(dev, AVRCP_TARGET_UUID,
						auth_cb, dev->control) < 0)
		goto drop;

	control->io_id = g_io_add_watch(chan, G_IO_ERR | G_IO_HUP | G_IO_NVAL,
							control_cb, control);
	return;

drop:
	if (!control || !control->io)
		g_io_channel_shutdown(chan, TRUE, NULL);
	if (control)
		avctp_set_state(control, AVCTP_STATE_DISCONNECTED);
}

static GIOChannel *avctp_server_socket(const bdaddr_t *src, gboolean master)
{
	GError *err = NULL;
	GIOChannel *io;

	io = bt_io_listen(BT_IO_L2CAP, NULL, avctp_confirm_cb, NULL,
				NULL, &err,
				BT_IO_OPT_SOURCE_BDADDR, src,
				BT_IO_OPT_PSM, AVCTP_PSM,
				BT_IO_OPT_SEC_LEVEL, BT_IO_SEC_MEDIUM,
				BT_IO_OPT_MASTER, master,
				BT_IO_OPT_INVALID);
	if (!io) {
		error("%s", err->message);
		g_error_free(err);
	}

	return io;
}

gboolean avrcp_connect(struct audio_device *dev)
{
	struct control *control = dev->control;
	GError *err = NULL;
	GIOChannel *io;

	if (control->state > AVCTP_STATE_DISCONNECTED)
		return TRUE;

	avctp_set_state(control, AVCTP_STATE_CONNECTING);

	io = bt_io_connect(BT_IO_L2CAP, avctp_connect_cb, control, NULL, &err,
				BT_IO_OPT_SOURCE_BDADDR, &dev->src,
				BT_IO_OPT_DEST_BDADDR, &dev->dst,
				BT_IO_OPT_PSM, AVCTP_PSM,
				BT_IO_OPT_INVALID);
	if (err) {
		avctp_set_state(control, AVCTP_STATE_DISCONNECTED);
		error("%s", err->message);
		g_error_free(err);
		return FALSE;
	}

	control->io = io;

	return TRUE;
}

void avrcp_disconnect(struct audio_device *dev)
{
	struct control *control = dev->control;

	if (!(control && control->io))
		return;

	avctp_set_state(control, AVCTP_STATE_DISCONNECTED);
}

int avrcp_register(DBusConnection *conn, const bdaddr_t *src, GKeyFile *config)
{
	sdp_record_t *record;
	gboolean tmp, master = TRUE;
	GError *err = NULL;
	struct avctp_server *server;

	if (config) {
		tmp = g_key_file_get_boolean(config, "General",
							"Master", &err);
		if (err) {
			DBG("audio.conf: %s", err->message);
			g_error_free(err);
		} else
			master = tmp;
		err = NULL;
		input_device_name = g_key_file_get_string(config,
			"AVRCP", "InputDeviceName", &err);
		if (err) {
			DBG("audio.conf: %s", err->message);
			input_device_name = NULL;
			g_error_free(err);
		}		
	}

	server = g_new0(struct avctp_server, 1);
	if (!server)
		return -ENOMEM;

	if (!connection)
		connection = dbus_connection_ref(conn);

	record = avrcp_tg_record();
	if (!record) {
		error("Unable to allocate new service record");
		g_free(server);
		return -1;
	}

	if (add_record_to_server(src, record) < 0) {
		error("Unable to register AVRCP target service record");
		g_free(server);
		sdp_record_free(record);
		return -1;
	}
	server->tg_record_id = record->handle;

#ifndef ANDROID
	record = avrcp_ct_record();
	if (!record) {
		error("Unable to allocate new service record");
		g_free(server);
		return -1;
	}

	if (add_record_to_server(src, record) < 0) {
		error("Unable to register AVRCP controller service record");
		sdp_record_free(record);
		g_free(server);
		return -1;
	}
	server->ct_record_id = record->handle;
#endif

	server->io = avctp_server_socket(src, master);
	if (!server->io) {
#ifndef ANDROID
		remove_record_from_server(server->ct_record_id);
#endif
		remove_record_from_server(server->tg_record_id);
		g_free(server);
		return -1;
	}

	bacpy(&server->src, src);

	servers = g_slist_append(servers, server);

	return 0;
}

static struct avctp_server *find_server(GSList *list, const bdaddr_t *src)
{
	for (; list; list = list->next) {
		struct avctp_server *server = list->data;

		if (bacmp(&server->src, src) == 0)
			return server;
	}

	return NULL;
}

void avrcp_unregister(const bdaddr_t *src)
{
	struct avctp_server *server;

	server = find_server(servers, src);
	if (!server)
		return;

	servers = g_slist_remove(servers, server);

#ifndef ANDROID
	remove_record_from_server(server->ct_record_id);
#endif
	remove_record_from_server(server->tg_record_id);

	g_io_channel_shutdown(server->io, TRUE, NULL);
	g_io_channel_unref(server->io);
	g_free(server);

	if (servers)
		return;

	dbus_connection_unref(connection);
	connection = NULL;
}

static DBusMessage *control_is_connected(DBusConnection *conn,
						DBusMessage *msg,
						void *data)
{
	struct audio_device *device = data;
	struct control *control = device->control;
	DBusMessage *reply;
	dbus_bool_t connected;

	reply = dbus_message_new_method_return(msg);
	if (!reply)
		return NULL;

	connected = (control->state == AVCTP_STATE_CONNECTED);

	dbus_message_append_args(reply, DBUS_TYPE_BOOLEAN, &connected,
					DBUS_TYPE_INVALID);

	return reply;
}

static int avctp_send_passthrough(struct control *control, uint8_t op)
{
	unsigned char buf[AVCTP_HEADER_LENGTH + AVRCP_HEADER_LENGTH + 2];
	struct avctp_header *avctp = (void *) buf;
	struct avrcp_header *avrcp = (void *) &buf[AVCTP_HEADER_LENGTH];
	uint8_t *operands = &buf[AVCTP_HEADER_LENGTH + AVRCP_HEADER_LENGTH];
	int sk = g_io_channel_unix_get_fd(control->io);
	static uint8_t transaction = 0;

	memset(buf, 0, sizeof(buf));

	avctp->transaction = transaction++;
	avctp->packet_type = AVCTP_PACKET_SINGLE;
	avctp->cr = AVCTP_COMMAND;
	avctp->pid = htons(AV_REMOTE_SVCLASS_ID);

	avrcp->code = CTYPE_CONTROL;
	avrcp->subunit_type = SUBUNIT_PANEL;
	avrcp->opcode = OP_PASSTHROUGH;

	operands[0] = op & 0x7f;
	operands[1] = 0;

	if (write(sk, buf, sizeof(buf)) < 0)
		return -errno;

	/* Button release */
	avctp->transaction = transaction++;
	operands[0] |= 0x80;

	if (write(sk, buf, sizeof(buf)) < 0)
		return -errno;

	return 0;
}

static DBusMessage *volume_up(DBusConnection *conn, DBusMessage *msg,
								void *data)
{
	struct audio_device *device = data;
	struct control *control = device->control;
	DBusMessage *reply;
	int err;

	reply = dbus_message_new_method_return(msg);
	if (!reply)
		return NULL;

	if (control->state != AVCTP_STATE_CONNECTED)
		return btd_error_not_connected(msg);

	if (!control->target)
		return btd_error_not_supported(msg);

	err = avctp_send_passthrough(control, VOL_UP_OP);
	/* Added %s in g_dbus_create_error since it only accepts string literal. Otherwise this results in build error. */
	if (err < 0)
		return btd_error_failed(msg, strerror(-err));

	return dbus_message_new_method_return(msg);
}

static DBusMessage *volume_down(DBusConnection *conn, DBusMessage *msg,
								void *data)
{
	struct audio_device *device = data;
	struct control *control = device->control;
	DBusMessage *reply;
	int err;

	reply = dbus_message_new_method_return(msg);
	if (!reply)
		return NULL;

	if (control->state != AVCTP_STATE_CONNECTED)
		return btd_error_not_connected(msg);

	if (!control->target)
		return btd_error_not_supported(msg);

	err = avctp_send_passthrough(control, VOL_DOWN_OP);
	/* Added %s in g_dbus_create_error since it only accepts string literal. Otherwise this results in build error. */
	if (err < 0)
		return btd_error_failed(msg, strerror(-err));

	return dbus_message_new_method_return(msg);
}

static DBusMessage *control_get_properties(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	struct audio_device *device = data;
	DBusMessage *reply;
	DBusMessageIter iter;
	DBusMessageIter dict;
	gboolean value;

	reply = dbus_message_new_method_return(msg);
	if (!reply)
		return NULL;

	dbus_message_iter_init_append(reply, &iter);

	dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY,
			DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
			DBUS_TYPE_STRING_AS_STRING DBUS_TYPE_VARIANT_AS_STRING
			DBUS_DICT_ENTRY_END_CHAR_AS_STRING, &dict);

	/* Connected */
	value = (device->control->state == AVCTP_STATE_CONNECTED);
	dict_append_entry(&dict, "Connected", DBUS_TYPE_BOOLEAN, &value);

	dbus_message_iter_close_container(&iter, &dict);

	return reply;
}

static DBusMessage *notify_playback_status_changed(DBusConnection *conn,
				DBusMessage *msg,
				void *data)
{
	DBusMessage *reply;
	DBusError err;
	struct avrcp_response *resp = NULL;
	struct audio_device *device = data;
	struct control *control = device->control;
	uint8_t event;
	uint8_t playstatus;
	uint8_t *ptr;
	uint8_t idx;

	reply = dbus_message_new_method_return(msg);
	if (!reply)
		return NULL;

	if (control->state != AVCTP_STATE_CONNECTED)
		return g_dbus_create_error(msg,
					ERROR_INTERFACE ".NotConnected",
					"Device not Connected");

	dbus_error_init(&err);
	if (!dbus_message_get_args(msg, &err,
				DBUS_TYPE_BYTE,
				&playstatus,
				DBUS_TYPE_INVALID)) {
		dbus_error_free(&err);
		return g_dbus_create_error(msg,
					ERROR_INTERFACE ".WrongArgs",
					"Wrong parameters passed");
	}

	idx = AVRCP_EVENT_PLAYBACK_STATUS_CHANGED - 1;

	if (!control->pa[idx])
		return g_dbus_create_error(msg,
					ERROR_INTERFACE ".NotRegistred",
					"Notification not registered");

	set_ctype_code(control->pa[idx]->headers, CTYPE_CHANGED);

	resp = avrcp_init_response(control->pa[idx]->pdu_id, AVCTP_PACKET_SINGLE, 2);
	ptr = (uint8_t*)(resp) + AVRCP_RESPONSE_PDU_LEN;
	event = AVRCP_EVENT_PLAYBACK_STATUS_CHANGED;
	store_u8(&ptr, event);
	store_u8(&ptr, playstatus);

	avrcp_create_response(control, control->pa[idx]->headers, resp);
	avrcp_playeragent_free(&control->pa[idx]);

	return dbus_message_new_method_return(msg);
}

static DBusMessage *notify_application_settings_changed(DBusConnection *conn,
				DBusMessage *msg,
				void *data)
{
	DBusMessage *reply;
	DBusError err;
	struct avrcp_response *resp = NULL;
	struct audio_device *device = data;
	struct control *control = device->control;
	uint8_t event;
	uint8_t *ptr;
	uint8_t idx;
	uint8_t shuffleValue;
	uint8_t repeatValue;
	uint8_t attr_count = 2; //There are two player application settings attributes
	uint8_t attrId1 = PLAYER_SET_REPEAT;
	uint8_t attrId2 = PLAYER_SET_SHUFFLE;
	
	reply = dbus_message_new_method_return(msg);
	if (!reply)
		return NULL;

	if (control->state != AVCTP_STATE_CONNECTED)
		return g_dbus_create_error(msg,
					ERROR_INTERFACE ".NotConnected",
					"Device not Connected");

	dbus_error_init(&err);
	
	idx = AVRCP_EVENT_PLAYER_APPLICATION_SETTING_CHANGED - 1;

	if (!control->pa[idx])
		return g_dbus_create_error(msg,
					ERROR_INTERFACE ".NotRegistred",
					"Notification not registered");

	set_ctype_code(control->pa[idx]->headers, CTYPE_CHANGED);
	
	if (!dbus_message_get_args(msg, &err,
				DBUS_TYPE_BYTE, &shuffleValue,
				DBUS_TYPE_BYTE, &repeatValue,
				DBUS_TYPE_INVALID)) {
		dbus_error_free(&err);
		return g_dbus_create_error(msg,
					ERROR_INTERFACE ".WrongArgs",
					"Wrong parameters passed");
	}
			
	resp = avrcp_init_response(control->pa[idx]->pdu_id, AVCTP_PACKET_SINGLE, attr_count * 2 + 2);
	ptr = (uint8_t*)(resp) + AVRCP_RESPONSE_PDU_LEN;
	event = AVRCP_EVENT_PLAYER_APPLICATION_SETTING_CHANGED;
	
	store_u8(&ptr, event);
	
	store_u8(&ptr, attr_count);

	store_u8(&ptr, attrId1);
	store_u8(&ptr, repeatValue);

	store_u8(&ptr, attrId2);
	store_u8(&ptr, shuffleValue);

	avrcp_create_response(control, control->pa[idx]->headers, resp);
	avrcp_playeragent_free(&control->pa[idx]);

	return dbus_message_new_method_return(msg);
}

static DBusMessage *notify_track_changed(DBusConnection *conn,
				DBusMessage *msg,
				void *data)
{
	DBusMessage *reply;
	DBusError err;
	struct avrcp_response *resp = NULL;
	struct audio_device *device = data;
	struct control *control = device->control;
	uint8_t event;
	uint64_t uid;
	uint8_t *ptr;
	uint8_t idx;

	reply = dbus_message_new_method_return(msg);
	if (!reply)
		return NULL;

	if (control->state != AVCTP_STATE_CONNECTED)
		return g_dbus_create_error(msg,
					ERROR_INTERFACE ".NotConnected",
					"Device not Connected");

	dbus_error_init(&err);
	if (!dbus_message_get_args(msg, &err,
				DBUS_TYPE_UINT64,
				&uid,
				DBUS_TYPE_INVALID)) {
		dbus_error_free(&err);
		return g_dbus_create_error(msg,
					ERROR_INTERFACE ".WrongArgs",
					"Wrong parameters passed");
	}

	idx = AVRCP_EVENT_TRACK_CHANGED - 1;

	if (!control->pa[idx])
		return g_dbus_create_error(msg,
					ERROR_INTERFACE ".NotRegistred",
					"Notification not registered");

	set_ctype_code(control->pa[idx]->headers, CTYPE_CHANGED);

	resp = avrcp_init_response(control->pa[idx]->pdu_id, AVCTP_PACKET_SINGLE, 9);
	ptr = (uint8_t*)(resp) + AVRCP_RESPONSE_PDU_LEN;
	event = AVRCP_EVENT_TRACK_CHANGED;
	store_u8(&ptr, event);
	/* Unique Identifier to Identify an element */
	store_u64(&ptr, uid);

	avrcp_create_response(control, control->pa[idx]->headers, resp);
	avrcp_playeragent_free(&control->pa[idx]);

	return dbus_message_new_method_return(msg);
}

static GDBusMethodTable control_methods[] = {
	{ "IsConnected",	"",	"b",	control_is_connected,
						G_DBUS_METHOD_FLAG_DEPRECATED },
	{ "GetProperties",	"",	"a{sv}",control_get_properties },
	{ "VolumeUp",		"",	"",	volume_up },
	{ "VolumeDown",		"",	"",	volume_down },
	{ "NotifyPlaybackStatusChanged",	"y",	"",
					notify_playback_status_changed },
	{ "NotifyTrackChanged",			"t",	"",
					notify_track_changed },
	{ "NotifyApplicationSettingsChanged",			"yy",	"",
					notify_application_settings_changed },
	{ NULL, NULL, NULL, NULL }
};

static GDBusSignalTable control_signals[] = {
	{ "Connected",			"",	G_DBUS_SIGNAL_FLAG_DEPRECATED},
	{ "Disconnected",		"",	G_DBUS_SIGNAL_FLAG_DEPRECATED},
	{ "PropertyChanged",		"sv"	},
	{ NULL, NULL }
};

static void path_unregister(void *data)
{
	struct audio_device *dev = data;
	struct control *control = dev->control;

	DBG("Unregistered interface %s on path %s",
		AUDIO_CONTROL_INTERFACE, dev->path);

	if (control->state != AVCTP_STATE_DISCONNECTED)
		avctp_disconnected(dev);

	g_free(control);
	dev->control = NULL;
}

void control_unregister(struct audio_device *dev)
{
	g_dbus_unregister_interface(dev->conn, dev->path,
		AUDIO_CONTROL_INTERFACE);
}

void control_update(struct audio_device *dev, uint16_t uuid16)
{
	struct control *control = dev->control;

	if (uuid16 == AV_REMOTE_TARGET_SVCLASS_ID)
		control->target = TRUE;
}

struct control *control_init(struct audio_device *dev, uint16_t uuid16)
{
	struct control *control;

	if (!g_dbus_register_interface(dev->conn, dev->path,
					AUDIO_CONTROL_INTERFACE,
					control_methods, control_signals, NULL,
					dev, path_unregister))
		return NULL;

	DBG("Registered interface %s on path %s",
		AUDIO_CONTROL_INTERFACE, dev->path);

	control = g_new0(struct control, 1);
	control->dev = dev;
	control->state = AVCTP_STATE_DISCONNECTED;
	control->uinput = -1;

	if (uuid16 == AV_REMOTE_TARGET_SVCLASS_ID)
		control->target = TRUE;

	return control;
}

gboolean control_is_active(struct audio_device *dev)
{
	struct control *control = dev->control;

	if (control && control->state != AVCTP_STATE_DISCONNECTED)
		return TRUE;

	return FALSE;
}

unsigned int avctp_add_state_cb(avctp_state_cb cb, void *user_data)
{
	struct avctp_state_callback *state_cb;
	static unsigned int id = 0;

	state_cb = g_new(struct avctp_state_callback, 1);
	state_cb->cb = cb;
	state_cb->user_data = user_data;
	state_cb->id = ++id;

	avctp_callbacks = g_slist_append(avctp_callbacks, state_cb);

	return state_cb->id;
}

gboolean avctp_remove_state_cb(unsigned int id)
{
	GSList *l;

	for (l = avctp_callbacks; l != NULL; l = l->next) {
		struct avctp_state_callback *cb = l->data;
		if (cb && cb->id == id) {
			avctp_callbacks = g_slist_remove(avctp_callbacks, cb);
			g_free(cb);
			return TRUE;
		}
	}

	return FALSE;
}
