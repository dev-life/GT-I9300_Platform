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

/* Enable: 1, Disable: 0 */
#define ENABLE_AVRCP_DEBUG 1

#ifdef ENABLE_AVRCP_DEBUG
#define FUNC_ENTER() 	avrcp_debug("--->: %s", __func__);
#define FUNC_LEAVE() 	avrcp_debug("<---: %s", __func__);
#else
#define FUNC_ENTER()
#define FUNC_LEAVE()
#endif

#define AVRCP_RESPONSE_PDU_LEN (sizeof(struct avrcp_response))
#define AVRCP_HEADER_LENGTH (sizeof(struct avrcp_header))
#define AVCTP_HEADER_LENGTH (sizeof(struct avctp_header))
#define AVCTP_AVRCP_HEADER_LEN (AVCTP_HEADER_LENGTH + AVRCP_HEADER_LENGTH)
#define AVRCP_COMPLETE_HEADER_LEN (AVRCP_RESPONSE_PDU_LEN + AVRCP_HEADER_LENGTH)

/* Control channel PSM */
#define AVCTP_PSM		23

/* A/V Control Max Frame size is 512 bytes */
#define AVRCP_MAX_FRAME_SIZE  512

#define AVRCP_MAX_PARAMETERS (AVRCP_MAX_FRAME_SIZE \
				 - AVRCP_HEADER_LENGTH \
				 - AVRCP_RESPONSE_PDU_LEN)

/* AVCTP Message types */
#define AVCTP_COMMAND	0
#define AVCTP_RESPONSE	1

/* Packet types */
#define AVCTP_PACKET_SINGLE		0
#define AVCTP_PACKET_START		1
#define AVCTP_PACKET_CONTINUE		2
#define AVCTP_PACKET_END		3

/* ctype entries */
#define CTYPE_CONTROL			0x00
#define CTYPE_STATUS			0x01
#define CTYPE_NOTIFY			0x03
#define CTYPE_NOT_IMPLEMENTED		0x08
#define CTYPE_ACCEPTED			0x09
#define CTYPE_REJECTED			0x0A
#define CTYPE_STABLE			0x0C
#define CTYPE_CHANGED			0x0D
#define CTYPE_INTERIM			0x0F

/*
 * This 24-bit value is expressed in a string as a
 * three-byte value with most significant byte first
 */
#define AVRCP_BLUETOOTH_COMPANY_ID "\x00\x19\x58"

/* opcodes */
#define OP_UNITINFO			0x30
#define OP_SUBUNITINFO			0x31
#define OP_PASSTHROUGH			0x7c
#define OP_VENDORDEPENDENT 	 	0x00

/* Vendor Dependent command opcodes */
#define AVRCP_GET_CAPABILITIES			0x10
#define AVRCP_LIST_PLAYER_SETTING_ATTRS		0x11
#define AVRCP_LIST_PLAYER_SETTING_VALUES	0x12
#define AVRCP_GET_PLAYER_SETTING_VALUE		0x13
#define AVRCP_SET_PLAYER_SETTING_VALUE		0x14
#define AVRCP_GET_PLAYER_SETTING_ATTR_TEXT	0x15
#define AVRCP_GET_PLAYER_SETTING_VALUE_TEXT	0x16
#define AVRCP_INFORM_DISP_CHAR_SET		0x17
#define AVRCP_INFORM_BATT_STATUS		0x18
#define AVRCP_GET_ELEMENT_ATTRIBUTES		0x20
#define AVRCP_GET_PLAY_STATUS			0x30
#define AVRCP_REGISTER_NOTIFY			0x31
#define AVRCP_REQUEST_CONT_RESP			0x40
#define AVRCP_ABORT_CONT_RESP			0x41

/*  Error code for AVRCP specific operations */
#define AVRCP_ERR_INVALID_CMD			0x00
#define AVRCP_ERR_INVALID_PARM			0x01
#define AVRCP_ERR_PARM_NOT_FOUND		0x02
#define AVRCP_ERR_INTERNAL_ERROR		0x03
#define AVRCP_ERR_NO_ERROR			0x04
#define AVRCP_ERR_UNKNOWN_ERROR			0x06
#define AVRCP_ERR_DOES_NOT_EXIST		0x09

/* Avrcp Get Capability ID */
#define AVRCP_COMPANY_ID			0x02
#define AVRCP_EVENTS_SUPPORTED			0x03

/* Total No. of AVRCP EVENTs */
#define AVRCP_NUM_EVENTS				8
/* AVRCP EVENTs */
#define AVRCP_EVENT_PLAYBACK_STATUS_CHANGED		0x01
#define AVRCP_EVENT_TRACK_CHANGED			0x02
#define AVRCP_EVENT_TRACK_REACHED_END 			0x03
#define AVRCP_EVENT_TRACK_REACHED_START			0x04
#define AVRCP_EVENT_PLAYBACK_POS_CHANGED		0x05
#define AVRCP_EVENT_BATT_STATUS_CHANGED			0x06
#define AVRCP_EVENT_SYSTEM_STATUS_CHANGED		0x07
#define AVRCP_EVENT_PLAYER_APPLICATION_SETTING_CHANGED	0x08

/* subunits of interest */
#define SUBUNIT_PANEL			0x09

/* operands in passthrough commands */
#define VOL_UP_OP			0x41
#define VOL_DOWN_OP			0x42
#define MUTE_OP				0x43
#define PLAY_OP				0x44
#define STOP_OP				0x45
#define PAUSE_OP			0x46
#define RECORD_OP			0x47
#define REWIND_OP			0x48
#define FAST_FORWARD_OP			0x49
#define EJECT_OP			0x4a
#define FORWARD_OP			0x4b
#define BACKWARD_OP			0x4c

/* operands in passthrough commands for the Cat 2 Panel */
#define AVRCP_OP_0		0x20
#define AVRCP_OP_1		0x21
#define AVRCP_OP_2		0x22
#define AVRCP_OP_3		0x23
#define AVRCP_OP_4		0x24
#define AVRCP_OP_5		0x25
#define AVRCP_OP_6		0x26
#define AVRCP_OP_7		0x27
#define AVRCP_OP_8		0x28
#define AVRCP_OP_9		0x29
#define AVRCP_OP_DOT		0x2A
#define AVRCP_OP_ENTER		0x2B
#define AVRCP_OP_CLEAR		0x2C

#define QUIRK_NO_RELEASE	1 << 0

/* Internal state machine status */
#define AVRCP_REPLY_ERR		0
#define AVRCP_REPLY_OK		1
#define AVRCP_REPLY_NONE	2

/* Player setting attributes */
#define PLAYER_SET_REPEAT   0x02;
#define PLAYER_SET_SHUFFLE  0x03;


/* Byte Order code */
#if __BYTE_ORDER == __LITTLE_ENDIAN

struct avctp_header {
	uint8_t		ipid:1;
	uint8_t		cr:1;
	uint8_t		packet_type:2;
	uint8_t		transaction:4;
	uint16_t	pid;
} __attribute__ ((packed));

struct avrcp_header {
	uint8_t		code:4;
	uint8_t		_hdr0:4;
	uint8_t		subunit_id:3;
	uint8_t		subunit_type:5;
	uint8_t		opcode;
} __attribute__ ((packed));

#elif __BYTE_ORDER == __BIG_ENDIAN

struct avctp_header {
	uint8_t		transaction:4;
	uint8_t		packet_type:2;
	uint8_t		cr:1;
	uint8_t		ipid:1;
	uint16_t	pid;
} __attribute__ ((packed));

struct avrcp_header {
	uint8_t		_hdr0:4;
	uint8_t		code:4;
	uint8_t		subunit_type:5;
	uint8_t		subunit_id:3;
	uint8_t		opcode;
} __attribute__ ((packed));

#else
#error "Unknown byte order"
#endif

struct control;

struct avrcp_response {
	uint8_t		sig_company_id[3];
	uint8_t		pdu_id;
	uint8_t		packet_type:2;
	uint8_t		reserved:6;
	uint16_t	param_len;
} __attribute__ ((packed));

struct avrcp_query {
	uint8_t		sig_company_id[3];
	uint8_t		pdu_id;
	uint8_t		packet_type:2;
	uint8_t		reserved:6;
	uint16_t	param_len;
	uint8_t		param[AVRCP_MAX_PARAMETERS];
} __attribute__ ((packed));

struct avrcp_cont_resp {
	uint16_t 	pdus;
	struct 		avrcp_response *resp;
	uint16_t 	bytes;
	uint32_t 	total;
	uint8_t 	pdu_id;
};

struct playeragent {
	struct 		control *control;
	uint8_t 	*headers;
	uint8_t 	pdu_id;
	uint8_t 	event_id;
	DBusPendingCall *call;
};

struct attribute {
	uint32_t 	id;
	uint16_t 	char_set;
	uint16_t 	len;
	uint8_t 	*value;
};
