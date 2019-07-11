#include "BleServer.h"
#include "freertos/task.h"

static const char	*TAG = "\033[1;91mBleServer\033[0m";

static char			_running = false;
static BleServerConfig_t    *_config = NULL;

uint8_t char1_str[] = {0x11,0x22,0x33};
static uint8_t adv_config_done = 0;

static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

struct  gatt_char_s {
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
};

struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
    struct gatt_char_s  chars[CHAR_NUM];
};


static uint8_t adv_service_uuid128[32] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    //first uuid, 16bit, [12],[13] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xEE, 0x00, 0x00, 0x00,
    //second uuid, 32bit, [12], [13], [14], [15] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};

// The length of adv data must be less than 31 bytes
//static uint8_t test_manufacturer[TEST_MANUFACTURER_DATA_LEN] =  {0x12, 0x23, 0x45, 0x56};
//adv data
static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x0006, //slave connection min interval, Time = min_interval * 1.25 msec
    .max_interval = 0x0010, //slave connection max interval, Time = max_interval * 1.25 msec
    .appearance = 0x00,
    .manufacturer_len = 0, //TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data =  NULL, //&test_manufacturer[0],
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = 32,
    .p_service_uuid = adv_service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};
// scan response data
static esp_ble_adv_data_t scan_rsp_data = {
    .set_scan_rsp = true,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x0006,
    .max_interval = 0x0010,
    .appearance = 0x00,
    .manufacturer_len = 0, //TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data =  NULL, //&test_manufacturer[0],
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = 32,
    .p_service_uuid = adv_service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};


static esp_ble_adv_params_t adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x40,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    //.peer_addr            =
    //.peer_addr_type       =
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst gl_profile_tab = {
    .gatts_cb = gatts_profile_a_event_handler,
	.gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
};


typedef struct {
    uint8_t                 *prepare_buf;
    int                     prepare_len;
} prepare_type_env_t;

static prepare_type_env_t a_prepare_write_env = {.prepare_buf = NULL, .prepare_len = 0};

void example_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);
void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        adv_config_done &= (~adv_config_flag);
        if (adv_config_done == 0){
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
        adv_config_done &= (~scan_rsp_config_flag);
        if (adv_config_done == 0){
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        //advertising start complete event to indicate advertising start successfully or failed
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "Advertising start failed\n");
        }
        break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "Advertising stop failed\n");
        }
        else {
            ESP_LOGI(TAG, "Stop adv successfully\n");
        }
        break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
         ESP_LOGI(TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                  param->update_conn_params.status,
                  param->update_conn_params.min_int,
                  param->update_conn_params.max_int,
                  param->update_conn_params.conn_int,
                  param->update_conn_params.latency,
                  param->update_conn_params.timeout);
        break;
    default:
        break;
    }
}

void example_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param){
    esp_gatt_status_t status = ESP_GATT_OK;
    if (param->write.need_rsp){
        if (param->write.is_prep){
            if (prepare_write_env->prepare_buf == NULL) {
                prepare_write_env->prepare_buf = (uint8_t *)malloc(BUFF_SIZE*sizeof(uint8_t));
                prepare_write_env->prepare_len = 0;
                if (prepare_write_env->prepare_buf == NULL) {
                    ESP_LOGE(TAG, "Gatt_server prep no mem\n");
                    status = ESP_GATT_NO_RESOURCES;
                }
            } else {
                if(param->write.offset > BUFF_SIZE) {
                    status = ESP_GATT_INVALID_OFFSET;
                } else if ((param->write.offset + param->write.len) > BUFF_SIZE) {
                    status = ESP_GATT_INVALID_ATTR_LEN;
                }
            }

            esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));
            gatt_rsp->attr_value.len = param->write.len;
            gatt_rsp->attr_value.handle = param->write.handle;
            gatt_rsp->attr_value.offset = param->write.offset;
            gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
            memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);
            esp_err_t response_err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, gatt_rsp);
            if (response_err != ESP_OK){
               ESP_LOGE(TAG, "Send response error\n");
            }
            free(gatt_rsp);
            if (status != ESP_GATT_OK){
                return;
            }
            memcpy(prepare_write_env->prepare_buf + param->write.offset,
                   param->write.value,
                   param->write.len);
            prepare_write_env->prepare_len += param->write.len;

        }else{
            esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, NULL);
        }
    }
}

void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param){
    if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC){
        esp_log_buffer_hex(TAG, prepare_write_env->prepare_buf, prepare_write_env->prepare_len);
    }else{
        ESP_LOGI(TAG,"ESP_GATT_PREP_WRITE_CANCEL");
    }
    if (prepare_write_env->prepare_buf) {
        free(prepare_write_env->prepare_buf);
        prepare_write_env->prepare_buf = NULL;
    }
    prepare_write_env->prepare_len = 0;
}

static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    switch (event) {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(TAG, "REGISTER_APP_EVT, status %d, app_id %d\n", param->reg.status, param->reg.app_id);
        gl_profile_tab.service_id.is_primary = true;
        gl_profile_tab.service_id.id.inst_id = 0x00;
        gl_profile_tab.service_id.id.uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab.service_id.id.uuid.uuid.uuid16 = SERVICE_WIFI_CONFIGURATOR;

        esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(DEVICE_NAME);
        if (set_dev_name_ret){
            ESP_LOGE(TAG, "set device name failed, error code = %x", set_dev_name_ret);
        }
        //config adv data
        esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
        if (ret){
            ESP_LOGE(TAG, "config adv data failed, error code = %x", ret);
        }
        adv_config_done |= adv_config_flag;
        //config scan response data
        ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
        if (ret){
            ESP_LOGE(TAG, "config scan response data failed, error code = %x", ret);
        }
        adv_config_done |= scan_rsp_config_flag;
        esp_ble_gatts_create_service(gatts_if, &gl_profile_tab.service_id, GATTS_NUM_HANDLE_TEST_A);
        break;
    case ESP_GATTS_READ_EVT: {
        ESP_LOGI(TAG, "GATT_READ_EVT, conn_id %d, trans_id %d, handle %d\n", param->read.conn_id, param->read.trans_id, param->read.handle);
        esp_gatt_rsp_t rsp;
        memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
        rsp.attr_value.handle = param->read.handle;
        for (int i = 0;i < CHAR_NUM;i++)
        {
            if (gl_profile_tab.chars[i].char_handle != param->read.handle)
                continue;
            switch (gl_profile_tab.chars[i].char_uuid.uuid.uuid16){
                case CHAR_SSID:
                rsp.attr_value.len = strlen((char *)_config->wifiConfig->ssid);
                memcpy(rsp.attr_value.value, _config->wifiConfig->ssid, rsp.attr_value.len);
                ESP_LOGI(TAG, "Read ssid: %s", rsp.attr_value.value);
                break;
                case CHAR_PASSWORD:
                rsp.attr_value.len = strlen((char *)_config->wifiConfig->password);
                memcpy(rsp.attr_value.value, _config->wifiConfig->password, rsp.attr_value.len);
                ESP_LOGI(TAG, "Read password: %s", rsp.attr_value.value);
                break;
            }
        }
        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
                                    ESP_GATT_OK, &rsp);
        break;
    }
    case ESP_GATTS_WRITE_EVT: {
        ESP_LOGI(TAG, "GATT_WRITE_EVT, conn_id %d, trans_id %d, handle %d", param->write.conn_id, param->write.trans_id, param->write.handle);
        for (int i = 0;i < CHAR_NUM;i++)
        {
            if (gl_profile_tab.chars[i].char_handle != param->write.handle || !_config || !_config->wifiConfig)
                continue;
            switch (gl_profile_tab.chars[i].char_uuid.uuid.uuid16){
                case CHAR_SSID:
                ESP_LOGI(TAG, "Changing ssid: %s", param->write.value);
                if (_config->wifiConfig->ssid)
                    free(_config->wifiConfig->ssid);
                _config->wifiConfig->ssid = malloc(sizeof(char) * (param->write.len + 1));
                _config->wifiConfig->ssid[param->write.len] = 0;
                memcpy(_config->wifiConfig->ssid, param->write.value, param->write.len);
                break;

                case CHAR_PASSWORD:
                ESP_LOGI(TAG, "Changing password: %s", param->write.value);
                if (_config->wifiConfig->password)
                    free(_config->wifiConfig->password);
                _config->wifiConfig->password = malloc(sizeof(char) * (param->write.len + 1));
                _config->wifiConfig->password[param->write.len] = 0;
                memcpy(_config->wifiConfig->password, param->write.value, param->write.len);
                break;

                case CHAR_ACTION:
                ESP_LOGI(TAG, "Action: %d", param->write.value[0]);
                if (param->write.len == 1){
                    switch (param->write.value[0]){
                        case 0:
                        ESP_LOGI(TAG, "Restart wifi");
                        if (!wifiIsUsed()) {
                            startWifiClient(_config->wifiConfig);
                        } else {
                            restartWifiClient(_config->wifiConfig);
                        }
                        break;
                    }
                }
                break;
            }
            example_write_event_env(gatts_if, &a_prepare_write_env, param);
        }
        break;
    }
    case ESP_GATTS_EXEC_WRITE_EVT:
        ESP_LOGI(TAG,"ESP_GATTS_EXEC_WRITE_EVT");
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        example_exec_write_event_env(&a_prepare_write_env, param);
        break;
    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
        break;
    case ESP_GATTS_UNREG_EVT:
        break;
    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d\n", param->create.status, param->create.service_handle);
        gl_profile_tab.service_handle = param->create.service_handle;
        esp_ble_gatts_start_service(gl_profile_tab.service_handle);

        gl_profile_tab.chars[0].char_uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab.chars[1].char_uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab.chars[2].char_uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab.chars[0].char_uuid.uuid.uuid16 = CHAR_SSID;
        gl_profile_tab.chars[1].char_uuid.uuid.uuid16 = CHAR_PASSWORD;
        gl_profile_tab.chars[2].char_uuid.uuid.uuid16 = CHAR_ACTION;
        esp_gatt_char_prop_t property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
        esp_attr_value_t ssid =
        {
            .attr_max_len = sizeof(char) * BUFF_SIZE,
            .attr_len     = sizeof(_config->wifiConfig->ssid),
            .attr_value   = _config->wifiConfig->ssid
        };
        esp_attr_value_t password =
        {
            .attr_max_len = sizeof(char) * BUFF_SIZE,
            .attr_len     = sizeof(_config->wifiConfig->password),
            .attr_value   = _config->wifiConfig->password
        };
        esp_err_t add_char_ret = esp_ble_gatts_add_char(gl_profile_tab.service_handle, &gl_profile_tab.chars[0].char_uuid,
                                                        ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                                        property,
                                                        &ssid, NULL);
        if (add_char_ret){
            ESP_LOGE(TAG, "add char ssid failed, error code =%x",add_char_ret);
        }
        vTaskDelay(pdMS_TO_TICKS(30));
        add_char_ret = esp_ble_gatts_add_char(gl_profile_tab.service_handle, &gl_profile_tab.chars[1].char_uuid,
                                                        ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                                        property,
                                                        &password, NULL);
        if (add_char_ret){
            ESP_LOGE(TAG, "add char password failed, error code =%x",add_char_ret);
        }
        vTaskDelay(pdMS_TO_TICKS(30));
        add_char_ret = esp_ble_gatts_add_char(gl_profile_tab.service_handle, &gl_profile_tab.chars[2].char_uuid,
                                                        ESP_GATT_PERM_WRITE,
                                                        ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY,
                                                        NULL, NULL);
        if (add_char_ret){
            ESP_LOGE(TAG, "add char action failed, error code =%x",add_char_ret);
        }
        break;
    case ESP_GATTS_ADD_INCL_SRVC_EVT:
        break;
    case ESP_GATTS_ADD_CHAR_EVT: {
        ESP_LOGI(TAG, "ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d\n",
                param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);
        for (int i = 0;i < CHAR_NUM;i++){
            if (gl_profile_tab.chars[i].char_uuid.uuid.uuid16 != param->add_char.char_uuid.uuid.uuid16)
                continue;
            gl_profile_tab.chars[i].char_handle = param->add_char.attr_handle;
        }
        break;
    }
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
        break;
    case ESP_GATTS_DELETE_EVT:
        break;
    case ESP_GATTS_START_EVT:
        ESP_LOGI(TAG, "SERVICE_START_EVT, status %d, service_handle %d\n",
                 param->start.status, param->start.service_handle);
        break;
    case ESP_GATTS_STOP_EVT:
        break;
    case ESP_GATTS_CONNECT_EVT: {
        esp_ble_conn_update_params_t conn_params = {0};
        memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        /* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
        conn_params.latency = 0;
        conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
        conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
        conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms
        ESP_LOGI(TAG, "ESP_GATTS_CONNECT_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x:",
                 param->connect.conn_id,
                 param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
                 param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
        gl_profile_tab.conn_id = param->connect.conn_id;
        //start sent the update connection parameters to the peer device.
        esp_ble_gap_update_conn_params(&conn_params);
        break;
    }
    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_DISCONNECT_EVT, disconnect reason 0x%x", param->disconnect.reason);
        esp_ble_gap_start_advertising(&adv_params);
        break;
    case ESP_GATTS_CONF_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_CONF_EVT, status %d attr_handle %d", param->conf.status, param->conf.handle);
        if (param->conf.status != ESP_GATT_OK){
            esp_log_buffer_hex(TAG, param->conf.value, param->conf.len);
        }
        break;
    case ESP_GATTS_OPEN_EVT:
    case ESP_GATTS_CANCEL_OPEN_EVT:
    case ESP_GATTS_CLOSE_EVT:
    case ESP_GATTS_LISTEN_EVT:
    case ESP_GATTS_CONGEST_EVT:
    default:
        break;
    }
}

//Register app id with gatt id to handle action
static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gl_profile_tab.gatts_if = gatts_if;
        } else {
            ESP_LOGI(TAG, "Reg app failed, app_id %04x, status %d\n",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }

    /* If the gatts_if equal to profile A, call profile A cb handler,
     * so here call each profile's callback */
	if (gatts_if == ESP_GATT_IF_NONE || gatts_if == gl_profile_tab.gatts_if){
		if (gl_profile_tab.gatts_cb) {
			gl_profile_tab.gatts_cb(event, gatts_if, param);
		}
	}
}

static esp_err_t	init()
{
	esp_err_t	ret;

    ESP_LOGI(TAG, "Initiating BLE...");
	ret = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
	if (ret != ESP_OK)
		return ret;

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return ret;
    }
    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(TAG, "%s init bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return ret;
    }
    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(TAG, "%s enable bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return ret;
    }

    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret){
        ESP_LOGE(TAG, "gatts register error, error code = %x", ret);
        return ret;
    }
    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret){
        ESP_LOGE(TAG, "gap register error, error code = %x", ret);
        return ret;
    }
    ret = esp_ble_gatts_app_register(0);
    if (ret){
        ESP_LOGE(TAG, "gatts app register error, error code = %x", ret);
        return ret;
    }
    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
    if (local_mtu_ret){
        ESP_LOGE(TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
    }
	return ret;
}

static esp_err_t	deinit()
{
	esp_err_t	ret;

    ESP_LOGI(TAG, "Deinitiating BLE...");
	ret = esp_ble_gatts_app_unregister(gl_profile_tab.gatts_if);
    if (ret){
        ESP_LOGE(TAG, "gatts app unregister error, error code = %x", ret);
        return ret;
    }
	ret = esp_bluedroid_disable();
    if (ret) {
        ESP_LOGE(TAG, "%s disable bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return ret;
    }
    ret = esp_bluedroid_deinit();
    if (ret) {
        ESP_LOGE(TAG, "%s deinit bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
		return ret;
    }    
	ret = esp_bt_controller_disable();
    if (ret) {
        ESP_LOGE(TAG, "%s disable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return ret;
    }
	ret = esp_bt_controller_deinit();
	if (ret) {
        ESP_LOGE(TAG, "%s deinitialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return ret;
    }
	return ret;
}

static void	taskBleServer(void *arg)
{
	BleServerConfig_t	*config = NULL;

    ESP_LOGI(TAG, "Initiating the task...");
	ESP_ERROR_CHECK(arg == NULL);
	config = (BleServerConfig_t *)arg;
    _config = config;
	init();
	while (_running){
        vTaskDelay(1);
	}
	deinit();
	vTaskDelete(NULL);
}

esp_err_t	startBleServer(BleServerConfig_t *config)
{
	BleServerConfig_t	*tmp = NULL;

	if (config == NULL)
		return ESP_FAIL;
	tmp = malloc(sizeof(BleServerConfig_t));
	if (tmp == NULL)
		return ESP_FAIL;
	memcpy(tmp, config, sizeof(BleServerConfig_t));
	_running = true;
    ESP_LOGI(TAG, "Starting the %s task...", TAG);
	return xTaskCreate(&taskBleServer, "taskBleServer", 10240, tmp, tskIDLE_PRIORITY, NULL);
}

esp_err_t	stopBleServer()
{
	if (_running == false)
		return ESP_FAIL;
    ESP_LOGI(TAG, "Stopping the %s task...", TAG);
	_running = false;
	return ESP_OK;
}