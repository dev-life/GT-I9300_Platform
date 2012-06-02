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

#include <bluetooth/bluetooth.h>
#include <bluetooth/sdp.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <dbus/dbus.h>
#include <glib.h>

#define ADAPTER_INTERFACE	"org.bluez.Adapter"

#define MODE_OFF		0x00
#define MODE_CONNECTABLE	0x01
#define MODE_DISCOVERABLE	0x02
#define MODE_UNKNOWN		0xff

#define MAX_NAME_LENGTH		248

/* Invalid SSP passkey value used to indicate negative replies */
#define INVALID_PASSKEY		0xffffffff
/* SS_BT::VEERA + HFP1.6 */
#define VENDOR_SPECIFIC_OGF 0x3f
#define ENABLE_WBS_OCF 0x7e
#define WRITE_I2SPCM_INTPARAM_OCF 0x6d
#define WRITE_SCOPCM_INTPARAM_OCF 0x1c

typedef enum {
	ADDR_TYPE_BREDR,
	ADDR_TYPE_LE_PUBLIC,
	ADDR_TYPE_LE_RANDOM,
} addr_type_t;

struct btd_adapter;

struct link_key_info {
	bdaddr_t bdaddr;
	unsigned char key[16];
	uint8_t type;
	uint8_t pin_len;
	uint8_t dlen;
	uint8_t data[10];
};

struct smp_ltk_info {
	bdaddr_t bdaddr;
	addr_type_t addr_type;
	uint8_t authenticated;
	uint8_t master;
	uint8_t enc_size;
	uint16_t ediv;
	uint8_t rand[8];
	uint8_t val[16];
};

struct remote_dev_info {
	bdaddr_t bdaddr;
	addr_type_t type;
	int8_t rssi;
	uint32_t class;
	char *name;
	char *alias;
	dbus_bool_t legacy;
	char **uuids;
	size_t uuid_count;
	GSList *services;
	uint8_t bdaddr_type;
	uint8_t flags;
};

/* SS_BT::VEERA + HFP1.6 */
/* brcm setting commands struct */
struct wbs_commands_param {
	uint8_t I2sEnable;
	uint8_t IsMaster;
	uint8_t ClockRate;
	uint8_t PcmInterfaceRate;
};

void btd_adapter_start(struct btd_adapter *adapter);

int btd_adapter_stop(struct btd_adapter *adapter);

void btd_adapter_get_mode(struct btd_adapter *adapter, uint8_t *mode,
					uint8_t *on_mode, gboolean *pairable);

struct btd_device *adapter_get_device(DBusConnection *conn,
				struct btd_adapter *adapter, const char *address);

struct btd_device *adapter_find_device(struct btd_adapter *adapter, const char *dest);

void adapter_remove_device(DBusConnection *conn, struct btd_adapter *adapter,
						struct btd_device *device,
						gboolean remove_storage);

struct btd_adapter *adapter_create(DBusConnection *conn, int id);
gboolean adapter_init(struct btd_adapter *adapter);
void adapter_remove(struct btd_adapter *adapter);
void adapter_set_allow_name_changes(struct btd_adapter *adapter,
						gboolean allow_name_changes);
void adapter_set_discovering(struct btd_adapter *adapter,
						gboolean discovering);
uint16_t adapter_get_dev_id(struct btd_adapter *adapter);
const gchar *adapter_get_path(struct btd_adapter *adapter);
void adapter_get_address(struct btd_adapter *adapter, bdaddr_t *bdaddr);
int adapter_get_discover_type(struct btd_adapter *adapter);
struct remote_dev_info *adapter_search_found_devices(struct btd_adapter *adapter,
							bdaddr_t *bdaddr);
void adapter_update_found_devices(struct btd_adapter *adapter,
					bdaddr_t *bdaddr, addr_type_t type,
					int8_t rssi, uint8_t confirm_name,
					uint8_t *data, uint8_t data_len);
void adapter_emit_device_found(struct btd_adapter *adapter,
						struct remote_dev_info *dev);
void adapter_mode_changed(struct btd_adapter *adapter, uint8_t scan_mode);
// SSBT :: KJH + (0215), to check le scan
void adapter_get_scanLe(struct btd_adapter *adapter, gboolean *mode);
int adapter_set_name(struct btd_adapter *adapter, const char *name);
void adapter_name_changed(struct btd_adapter *adapter, const char *name);
void adapter_service_insert(struct btd_adapter *adapter, void *rec);
void adapter_service_remove(struct btd_adapter *adapter, void *rec);
void btd_adapter_class_changed(struct btd_adapter *adapter,
							uint32_t new_class);
void btd_adapter_pairable_changed(struct btd_adapter *adapter,
							gboolean pairable);

struct agent *adapter_get_agent(struct btd_adapter *adapter);
void adapter_add_connection(struct btd_adapter *adapter,
						struct btd_device *device);
void adapter_remove_connection(struct btd_adapter *adapter,
						struct btd_device *device);
gboolean adapter_has_discov_sessions(struct btd_adapter *adapter);
void adapter_suspend_discovery(struct btd_adapter *adapter);
//void adapter_resume_discovery(struct btd_adapter *adapter);

struct btd_adapter *btd_adapter_ref(struct btd_adapter *adapter);
void btd_adapter_unref(struct btd_adapter *adapter);

int btd_adapter_set_class(struct btd_adapter *adapter, uint8_t major,
							uint8_t minor);

struct btd_adapter_driver {
	const char *name;
	int (*probe) (struct btd_adapter *adapter);
	void (*remove) (struct btd_adapter *adapter);
};

typedef void (*service_auth_cb) (DBusError *derr, void *user_data);

int btd_register_adapter_driver(struct btd_adapter_driver *driver);
void btd_unregister_adapter_driver(struct btd_adapter_driver *driver);
int btd_request_authorization(const bdaddr_t *src, const bdaddr_t *dst,
		const char *uuid, service_auth_cb cb, void *user_data);
int btd_cancel_authorization(const bdaddr_t *src, const bdaddr_t *dst);

const char *adapter_any_get_path(void);

const char *btd_adapter_any_request_path(void);
void btd_adapter_any_release_path(void);
//gboolean adapter_is_pairable(struct btd_adapter *adapter);
gboolean adapter_powering_down(struct btd_adapter *adapter);

int btd_adapter_restore_powered(struct btd_adapter *adapter);
int btd_adapter_switch_online(struct btd_adapter *adapter);
int btd_adapter_switch_offline(struct btd_adapter *adapter);

typedef ssize_t (*btd_adapter_pin_cb_t) (struct btd_adapter *adapter,
					struct btd_device *dev, char *out);
void btd_adapter_register_pin_cb(struct btd_adapter *adapter,
						btd_adapter_pin_cb_t cb);
void btd_adapter_unregister_pin_cb(struct btd_adapter *adapter,
						btd_adapter_pin_cb_t cb);
ssize_t btd_adapter_get_pin(struct btd_adapter *adapter, struct btd_device *dev,
								char *pin_buf);

typedef void (*bt_hci_result_t) (uint8_t status, gpointer user_data);

struct btd_adapter_ops {
	int (*setup) (void);
	void (*cleanup) (void);
	int (*set_powered) (int index, gboolean powered);
	int (*set_discoverable) (int index, gboolean discoverable);
	int (*set_pairable) (int index, gboolean pairable);
	int (*set_limited_discoverable) (int index, gboolean limited);
	int (*start_discovery) (int index);
	int (*stop_discovery) (int index);
	int (*test_end_le) (int index);
	int (*set_name) (int index, const char *name);
	int (*set_dev_class) (int index, uint8_t major, uint8_t minor);
	int (*set_fast_connectable) (int index, gboolean enable);
	int (*read_clock) (int index, bdaddr_t *bdaddr, int which, int timeout,
					uint32_t *clock, uint16_t *accuracy);
	int (*read_bdaddr) (int index, bdaddr_t *bdaddr);
	int (*block_device) (int index, bdaddr_t *bdaddr, addr_type_t type);
	int (*unblock_device) (int index, bdaddr_t *bdaddr, addr_type_t type);
	int (*get_conn_list) (int index, GSList **conns);
	int (*read_local_features) (int index, uint8_t *features);
	int (*disconnect) (int index, bdaddr_t *bdaddr, addr_type_t type);
	int (*remove_bonding) (int index, bdaddr_t *bdaddr, addr_type_t type);
	int (*pincode_reply) (int index, bdaddr_t *bdaddr, const char *pin,
							size_t pin_len);
	int (*confirm_reply) (int index, bdaddr_t *bdaddr, addr_type_t type,
							gboolean success);
	int (*passkey_reply) (int index, bdaddr_t *bdaddr, addr_type_t type,
							uint32_t passkey);
	int (*enable_le) (int index);
	int (*encrypt_link) (int index, bdaddr_t *bdaddr, bt_hci_result_t cb,
							gpointer user_data);
	int (*set_did) (int index, uint16_t vendor, uint16_t product,
							uint16_t version);
	int (*add_uuid) (int index, uuid_t *uuid, uint8_t svc_hint);
	int (*remove_uuid) (int index, uuid_t *uuid);
	int (*disable_cod_cache) (int index);
	int (*restore_powered) (int index);
	int (*load_keys) (int index, GSList *keys, gboolean debug_keys);
	int (*set_io_capability) (int index, uint8_t io_capability);
	int (*create_bonding) (int index, bdaddr_t *bdaddr, uint8_t addr_type,
							uint8_t io_cap);
	int (*cancel_bonding) (int index, bdaddr_t *bdaddr);
	int (*read_local_oob_data) (int index);
	int (*add_remote_oob_data) (int index, bdaddr_t *bdaddr, uint8_t *hash,
							uint8_t *randomizer);
	int (*remove_remote_oob_data) (int index, bdaddr_t *bdaddr);
	int (*confirm_name) (int index, bdaddr_t *bdaddr, gboolean name_known);
	int (*set_link_timeout) (int index, bdaddr_t *bdaddr, uint32_t num_slots);
	int (*retry_authentication) (int index, bdaddr_t *bdaddr);
	int (*load_ltks) (int index, GSList *keys);
	/* monitoring of the RSSI of the link between two Bluetooth devices */
	int (*read_rssi) (int index, bdaddr_t *bdaddr);
	int (*set_sco_path) (int index, uint32_t sco_path);
};

int btd_register_adapter_ops(struct btd_adapter_ops *ops, gboolean priority);
void btd_adapter_cleanup_ops(struct btd_adapter_ops *btd_adapter_ops);
int adapter_ops_setup(void);

typedef void (*btd_adapter_powered_cb) (struct btd_adapter *adapter,
						gboolean powered);
void btd_adapter_register_powered_callback(struct btd_adapter *adapter,
						btd_adapter_powered_cb cb);
void btd_adapter_unregister_powered_callback(struct btd_adapter *adapter,
						btd_adapter_powered_cb cb);

/* If TRUE, enables fast connectabe, i.e. reduces page scan interval and changes
 * type. If FALSE, disables fast connectable, i.e. sets page scan interval and
 * type to default values. Valid for both connectable and discoverable modes. */
int btd_adapter_set_fast_connectable(struct btd_adapter *adapter,
							gboolean enable);

int btd_adapter_read_clock(struct btd_adapter *adapter, bdaddr_t *bdaddr,
				int which, int timeout, uint32_t *clock,
				uint16_t *accuracy);

int btd_adapter_block_address(struct btd_adapter *adapter, bdaddr_t *bdaddr,
							addr_type_t type);
int btd_adapter_unblock_address(struct btd_adapter *adapter, bdaddr_t *bdaddr,
							addr_type_t type);

int btd_adapter_disconnect_device(struct btd_adapter *adapter,
					bdaddr_t *bdaddr, addr_type_t type);

int btd_adapter_remove_bonding(struct btd_adapter *adapter, bdaddr_t *bdaddr,
							addr_type_t type);

int btd_adapter_pincode_reply(struct btd_adapter *adapter, bdaddr_t *bdaddr,
					const char *pin, size_t pin_len);
int btd_adapter_confirm_reply(struct btd_adapter *adapter, bdaddr_t *bdaddr,
					addr_type_t type, gboolean success);
int btd_adapter_passkey_reply(struct btd_adapter *adapter, bdaddr_t *bdaddr,
					addr_type_t type, uint32_t passkey);

int btd_adapter_encrypt_link(struct btd_adapter *adapter, bdaddr_t *bdaddr,
				bt_hci_result_t cb, gpointer user_data);

int btd_adapter_retry_authentication(struct btd_adapter *adapter,
				bdaddr_t *bdaddr);

int btd_adapter_set_did(struct btd_adapter *adapter, uint16_t vendor,
					uint16_t product, uint16_t version);

int adapter_create_bonding(struct btd_adapter *adapter, bdaddr_t *bdaddr,
				uint8_t addr_type, uint8_t io_cap);

int adapter_cancel_bonding(struct btd_adapter *adapter, bdaddr_t *bdaddr);

void adapter_bonding_complete(struct btd_adapter *adapter, bdaddr_t *bdaddr,
							uint8_t status);

int btd_adapter_read_local_oob_data(struct btd_adapter *adapter);

int btd_adapter_add_remote_oob_data(struct btd_adapter *adapter,
			bdaddr_t *bdaddr, uint8_t *hash, uint8_t *randomizer);

int btd_adapter_remove_remote_oob_data(struct btd_adapter *adapter,
							bdaddr_t *bdaddr);

/* monitoring of the RSSI of the link between two Bluetooth devices */
int btd_adapter_read_rssi(struct btd_adapter *adapter, bdaddr_t *bdaddr);
