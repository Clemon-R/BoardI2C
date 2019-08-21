#include "BleServer.h"
#include "freertos/task.h"

#include "../lcd/Sensors.h"
#include "../sensors/SensorClient.h"
#include "../wifi/Ota.h"

static const char	*TAG = "\033[1;91mBleServer\033[0m";

static char			_running = false;
static BleServerConfig_t    *_config = NULL;
static uint32_t             _passkey;

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
    esp_gatt_srvc_id_t services[SERVICE_NUM];
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
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "Advertising start failed\n");
        }
        break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "Advertising stop failed\n");
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
    case ESP_GAP_BLE_SEC_REQ_EVT:
        /* send the positive(true) security response to the peer device to accept the security request.
        If not accept the security request, should sent the security response with negative(false) accept value*/
        ESP_LOGI(TAG, "Security gap");
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
        break;
    case ESP_GAP_BLE_PASSKEY_NOTIF_EVT:  ///the app will receive this evt when the IO  has Output capability and the peer device IO has Input capability.
        ///show the passkey number to the user to input it in the peer deivce.
        ESP_LOGI(TAG, "The passkey Notify number:%06d", param->ble_security.key_notif.passkey);
        showContainer(param->ble_security.key_notif.passkey, true);
        break;
    case ESP_GAP_BLE_AUTH_CMPL_EVT:
        showContainer(0, false);
        _passkey = (uint32_t)((rand() / (float)RAND_MAX) * UINT32_MAX);
        break;
    default:
        ESP_LOGW(TAG, "Unknow event id %d", event);
        break;
    }
}

static void write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param){
    esp_gatt_status_t status = ESP_GATT_OK;
    if (!param->write.need_rsp)
        return;
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

static void exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param){
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

static void createSensorsService()
{
    esp_gatt_char_prop_t property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE;

    gl_profile_tab.chars[SENSORS_CHAR_OFFSET].char_uuid.len = ESP_UUID_LEN_16;
    gl_profile_tab.chars[SENSORS_CHAR_OFFSET + 1].char_uuid.len = ESP_UUID_LEN_16;
    gl_profile_tab.chars[SENSORS_CHAR_OFFSET + 2].char_uuid.len = ESP_UUID_LEN_16;
    gl_profile_tab.chars[SENSORS_CHAR_OFFSET + 3].char_uuid.len = ESP_UUID_LEN_16;
    gl_profile_tab.chars[SENSORS_CHAR_OFFSET + 4].char_uuid.len = ESP_UUID_LEN_16;
    gl_profile_tab.chars[SENSORS_CHAR_OFFSET + 5].char_uuid.len = ESP_UUID_LEN_16;
    gl_profile_tab.chars[SENSORS_CHAR_OFFSET].char_uuid.uuid.uuid16 = SENSORS_CHAR_STATE;
    gl_profile_tab.chars[SENSORS_CHAR_OFFSET + 1].char_uuid.uuid.uuid16 = SENSORS_CHAR_DELAI;
    gl_profile_tab.chars[SENSORS_CHAR_OFFSET + 2].char_uuid.uuid.uuid16 = SENSORS_CHAR_TEMP;
    gl_profile_tab.chars[SENSORS_CHAR_OFFSET + 3].char_uuid.uuid.uuid16 = SENSORS_CHAR_HUMIDITY;
    gl_profile_tab.chars[SENSORS_CHAR_OFFSET + 4].char_uuid.uuid.uuid16 = SENSORS_CHAR_PRESSURE;
    gl_profile_tab.chars[SENSORS_CHAR_OFFSET + 5].char_uuid.uuid.uuid16 = SENSORS_CHAR_COLOR;

    for (int i = SENSORS_CHAR_OFFSET;i < SENSORS_CHAR_OFFSET + 6;i++){
        esp_err_t add_char_ret = esp_ble_gatts_add_char(gl_profile_tab.service_handle, &gl_profile_tab.chars[i].char_uuid,
                                                        ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED,
                                                        property,
                                                        NULL, NULL);
        if (add_char_ret){
            ESP_LOGE(TAG, "add char failed, error code =%x",add_char_ret);
        }
    }
}

static void createMqttService()
{
    esp_gatt_char_prop_t property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE;

    gl_profile_tab.chars[MQTT_CHAR_OFFSET].char_uuid.len = ESP_UUID_LEN_16;
    gl_profile_tab.chars[MQTT_CHAR_OFFSET + 1].char_uuid.len = ESP_UUID_LEN_16;
    gl_profile_tab.chars[MQTT_CHAR_OFFSET + 2].char_uuid.len = ESP_UUID_LEN_16;
    gl_profile_tab.chars[MQTT_CHAR_OFFSET].char_uuid.uuid.uuid16 = MQTT_CHAR_URL;
    gl_profile_tab.chars[MQTT_CHAR_OFFSET + 1].char_uuid.uuid.uuid16 = MQTT_CHAR_PORT;
    gl_profile_tab.chars[MQTT_CHAR_OFFSET + 2].char_uuid.uuid.uuid16 = MQTT_CHAR_ACTION;

    for (int i = MQTT_CHAR_OFFSET;i < MQTT_CHAR_OFFSET + 3;i++){
        esp_err_t add_char_ret = esp_ble_gatts_add_char(gl_profile_tab.service_handle, &gl_profile_tab.chars[i].char_uuid,
                                                        ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED,
                                                        property,
                                                        NULL, NULL);
        if (add_char_ret){
            ESP_LOGE(TAG, "add char failed, error code =%x",add_char_ret);
        }
    }
}

static void createWifiService()
{
    gl_profile_tab.chars[WIFI_CHAR_OFFSET].char_uuid.len = ESP_UUID_LEN_16;
    gl_profile_tab.chars[WIFI_CHAR_OFFSET + 1].char_uuid.len = ESP_UUID_LEN_16;
    gl_profile_tab.chars[WIFI_CHAR_OFFSET + 2].char_uuid.len = ESP_UUID_LEN_16;
    gl_profile_tab.chars[WIFI_CHAR_OFFSET].char_uuid.uuid.uuid16 = WIFI_CHAR_SSID;
    gl_profile_tab.chars[WIFI_CHAR_OFFSET + 1].char_uuid.uuid.uuid16 = WIFI_CHAR_PASSWORD;
    gl_profile_tab.chars[WIFI_CHAR_OFFSET + 2].char_uuid.uuid.uuid16 = WIFI_CHAR_ACTION;
    esp_gatt_char_prop_t property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE;

    for (int i = WIFI_CHAR_OFFSET;i < WIFI_CHAR_OFFSET + 3;i++){
        esp_err_t add_char_ret = esp_ble_gatts_add_char(gl_profile_tab.service_handle, &gl_profile_tab.chars[i].char_uuid,
                                                    ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED,
                                                    property,
                                                    NULL, NULL);
        if (add_char_ret){
            ESP_LOGE(TAG, "add char ssid failed, error code =%x",add_char_ret);
        }
    }
}

static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    TickType_t  delai;
    static int16_t  readTruncated = -1;

    switch (event) {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(TAG, "REGISTER_APP_EVT, status %d, app_id %d\n", param->reg.status, param->reg.app_id);
        gl_profile_tab.services[0].is_primary = true;
        gl_profile_tab.services[0].id.inst_id = 0x00;
        gl_profile_tab.services[0].id.uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab.services[0].id.uuid.uuid.uuid16 = SERVICE_WIFI;

        gl_profile_tab.services[1].is_primary = true;
        gl_profile_tab.services[1].id.inst_id = 0x00;
        gl_profile_tab.services[1].id.uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab.services[1].id.uuid.uuid.uuid16 = SERVICE_MQTT;

        gl_profile_tab.services[2].is_primary = true;
        gl_profile_tab.services[2].id.inst_id = 0x00;
        gl_profile_tab.services[2].id.uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab.services[2].id.uuid.uuid.uuid16 = SERVICE_SENSORS;

        esp_ble_gap_config_local_privacy(true);
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
        esp_ble_gatts_create_service(gatts_if, &gl_profile_tab.services[0], GATTS_NUM_HANDLE_TEST_A);
        esp_ble_gatts_create_service(gatts_if, &gl_profile_tab.services[1], GATTS_NUM_HANDLE_TEST_A);
        esp_ble_gatts_create_service(gatts_if, &gl_profile_tab.services[2], GATTS_NUM_HANDLE_TEST_A);
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
                case WIFI_CHAR_SSID:
                rsp.attr_value.len = strlen((char *)_config->wifiConfig->ssid);
                memcpy(rsp.attr_value.value, _config->wifiConfig->ssid, rsp.attr_value.len);
                ESP_LOGI(TAG, "Read ssid: %s", rsp.attr_value.value);
                break;

                case WIFI_CHAR_PASSWORD:
                rsp.attr_value.len = strlen((char *)_config->wifiConfig->password);
                memcpy(rsp.attr_value.value, _config->wifiConfig->password, rsp.attr_value.len);
                ESP_LOGI(TAG, "Read password: %s", rsp.attr_value.value);
                break;

                case MQTT_CHAR_URL:
                rsp.attr_value.len = strlen((char *)_config->mqttConfig->url);
                memcpy(rsp.attr_value.value, _config->mqttConfig->url, rsp.attr_value.len);
                ESP_LOGI(TAG, "Read url: %s", rsp.attr_value.value);
                break;

                case MQTT_CHAR_PORT:
                rsp.attr_value.len = sizeof(uint16_t);
                memcpy(rsp.attr_value.value, (uint8_t *)&_config->mqttConfig->port, rsp.attr_value.len);
                ESP_LOGI(TAG, "Read port: %d", *((uint16_t *)rsp.attr_value.value));
                break;

                case SENSORS_CHAR_STATE:
                rsp.attr_value.len = sizeof(char);
                rsp.attr_value.value[0] = isSensorRunning();
                ESP_LOGI(TAG, "Read sensors state: %d", rsp.attr_value.value[0]);
                break;

                case SENSORS_CHAR_DELAI:
                rsp.attr_value.len = sizeof(TickType_t);
                delai = getRefreshDelai() * portTICK_PERIOD_MS;
                memcpy(rsp.attr_value.value, (uint8_t *)&delai, rsp.attr_value.len);
                ESP_LOGI(TAG, "Read sensors delai: %d", *((TickType_t *)rsp.attr_value.value));
                break;
            }
        }
        if (rsp.attr_value.len >= ESP_GATT_DEF_BLE_MTU_SIZE){
            if (readTruncated < 0)
                readTruncated = 0;
            rsp.attr_value.len -= readTruncated;
            memcpy(rsp.attr_value.value, rsp.attr_value.value + readTruncated, rsp.attr_value.len);
            readTruncated += ESP_GATT_DEF_BLE_MTU_SIZE - 1;
        } else 
            readTruncated = -1;
        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
                                    ESP_GATT_OK, &rsp);
        break;
    }
    case ESP_GATTS_WRITE_EVT: {
        ESP_LOGI(TAG, "GATT_WRITE_EVT, conn_id %d, trans_id %d, handle %d", param->write.conn_id, param->write.trans_id, param->write.handle);
        for (int i = 0;i < CHAR_NUM;i++)
        {
            if (gl_profile_tab.chars[i].char_handle != param->write.handle || !_config)
                continue;
            switch (gl_profile_tab.chars[i].char_uuid.uuid.uuid16){
                case WIFI_CHAR_SSID:
                ESP_LOGI(TAG, "Changing ssid: %s", param->write.value);
                if (_config->wifiConfig->ssid) {
                    free(_config->wifiConfig->ssid);
                    _config->wifiConfig->ssid = (uint8_t *)malloc(sizeof(char) * BUFF_SIZE);
                }
                _config->wifiConfig->ssid[param->write.len + param->write.offset] = 0;
                memcpy(_config->wifiConfig->ssid + param->write.offset, param->write.value, param->write.len);
                break;

                case WIFI_CHAR_PASSWORD:
                ESP_LOGI(TAG, "Changing password: %s", param->write.value);
                if (param->write.offset == 0 && _config->wifiConfig->password){
                    free(_config->wifiConfig->password);
                    _config->wifiConfig->password = (uint8_t *)malloc(sizeof(char) * BUFF_SIZE);
                }
                _config->wifiConfig->password[param->write.offset + param->write.len] = 0;
                memcpy(_config->wifiConfig->password + param->write.offset, param->write.value, param->write.len);
                break;

                case WIFI_CHAR_ACTION:
                ESP_LOGI(TAG, "Restart wifi");
                if (!wifiIsUsed()) {
                    startWifiClient(_config->wifiConfig);
                } else {
                    restartWifiClient(_config->wifiConfig);
                }
                saveWifiConfig(_config->wifiConfig);
                break;

                case MQTT_CHAR_URL:
                if (param->write.offset == 0 && _config->mqttConfig->url){
                    free(_config->mqttConfig->url);
                    _config->mqttConfig->url = (uint8_t *)malloc(sizeof(char) * BUFF_SIZE);
                }
                _config->mqttConfig->url[param->write.offset + param->write.len] = 0;
                memcpy(_config->mqttConfig->url + param->write.offset, param->write.value, param->write.len);
                ESP_LOGI(TAG, "Changing url: %s", _config->mqttConfig->url);
                break;

                case MQTT_CHAR_PORT:
                ESP_LOGI(TAG, "Changing port: %d", *((uint16_t *)param->write.value));
                _config->mqttConfig->port = *((uint16_t *)param->write.value);
                break;

                case MQTT_CHAR_ACTION:
                ESP_LOGI(TAG, "Restart mqtt");
                if (mqttIsRunning()) {
                    restartMqttClient(_config->mqttConfig);
                } else {
                    startMqttClient(_config->mqttConfig);
                }
                saveMqttConfig(_config->mqttConfig);
                break;

                case SENSORS_CHAR_STATE:
                if (param->write.len != 1)
                    break;
                if (param->write.value[0] == 1)
                    startSensorClient();
                else if (param->write.value[0] == 0)
                    stopSensorClient();                
                break;

                case SENSORS_CHAR_DELAI:
                if (param->write.len != 4)
                    break;
                setRefreshDelai(pdMS_TO_TICKS(*((TickType_t *)param->write.value)));
                break;
            }
            write_event_env(gatts_if, &a_prepare_write_env, param);
        }
        break;
    }
    case ESP_GATTS_EXEC_WRITE_EVT:
        ESP_LOGI(TAG,"ESP_GATTS_EXEC_WRITE_EVT");
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        exec_write_event_env(&a_prepare_write_env, param);
        break;
    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d\n", param->create.status, param->create.service_handle);
        gl_profile_tab.service_handle = param->create.service_handle;
        esp_ble_gatts_start_service(gl_profile_tab.service_handle);

        if (param->create.service_id.id.uuid.uuid.uuid16 == SERVICE_WIFI)
            createWifiService();
        else if (param->create.service_id.id.uuid.uuid.uuid16 == SERVICE_MQTT)
            createMqttService();
        else if (param->create.service_id.id.uuid.uuid.uuid16 == SERVICE_SENSORS)
            createSensorsService();
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
    case ESP_GATTS_START_EVT:
        ESP_LOGI(TAG, "SERVICE_START_EVT, status %d, service_handle %d\n",
                 param->start.status, param->start.service_handle);
        break;
    case ESP_GATTS_STOP_EVT:
        ESP_LOGI(TAG, "SERVICE_STOP_EVT, status %d, service_handle %d\n",
                 param->start.status, param->start.service_handle);
        break;
    case ESP_GATTS_CONNECT_EVT: {
        heap_caps_print_heap_info(MALLOC_CAP_8BIT);
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
        esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT_MITM);
        createAlert("ble_connect", "Someone has been connected from BLE", MINOR, false);
        startLcd();
        break;
    }
    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_DISCONNECT_EVT, disconnect reason 0x%x", param->disconnect.reason);
        esp_ble_gap_start_advertising(&adv_params);
        createAlert("ble_connect", "Someone has been disconnected from BLE", MINOR, true);
        break;
    case ESP_GATTS_CONF_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_CONF_EVT, status %d attr_handle %d", param->conf.status, param->conf.handle);
        if (param->conf.status != ESP_GATT_OK){
            esp_log_buffer_hex(TAG, param->conf.value, param->conf.len);
        }
        break;
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
    /* set the security iocap & auth_req & key size & init key response key parameters to the stack*/
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_BOND;     //bonding with peer device after authentication
    esp_ble_io_cap_t iocap = ESP_IO_CAP_OUT;           //set the IO capability to No output No input
    uint8_t key_size = 16;      //the key size should be 7~16 bytes
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    //set static passkey
    uint8_t auth_option = ESP_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_ENABLE;
    _passkey = (uint32_t)((rand() / (float)RAND_MAX) * UINT32_MAX);
    esp_ble_gap_set_security_param(ESP_BLE_SM_PASSKEY, &_passkey, sizeof(uint32_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_ONLY_ACCEPT_SPECIFIED_SEC_AUTH, &auth_option, sizeof(uint8_t));
    /* If your BLE device act as a Slave, the init_key means you hope which types of key of the master should distribut to you,
    and the response key means which key you can distribut to the Master;
    If your BLE device act as a master, the response key means you hope which types of key of the slave should distribut to you,
    and the init key means which key you can distribut to the slave. */
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));
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