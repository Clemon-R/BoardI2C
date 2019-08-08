#ifndef DEMO_CONF_H_
#define DEMO_CONF_H_

#ifdef BLESERVER_H_
	#define	SERVICE_NUM 		3
	#define SERVICE_WIFI   		0x00FF
	#define SERVICE_MQTT   		0x00FE
	#define SERVICE_SENSORS   	0x00FD

	#define	CHAR_NUM						12
	#define WIFI_CHAR_OFFSET				0
	#define	WIFI_CHAR_SSID					0xFF01
	#define	WIFI_CHAR_PASSWORD				0xFF02
	#define	WIFI_CHAR_ACTION				0xFF03
	#define MQTT_CHAR_OFFSET				3
	#define	MQTT_CHAR_URL					0xFE01
	#define	MQTT_CHAR_PORT					0xFE02
	#define	MQTT_CHAR_ACTION				0xFE03
	#define SENSORS_CHAR_OFFSET				6
	#define	SENSORS_CHAR_STATE				0xFD01
	#define	SENSORS_CHAR_DELAI				0xFD02
	#define	SENSORS_CHAR_TEMP				0xFD03
	#define	SENSORS_CHAR_HUMIDITY			0xFD04
	#define	SENSORS_CHAR_PRESSURE			0xFD05
	#define	SENSORS_CHAR_COLOR				0xFD06

	#define GATTS_NUM_HANDLE_TEST_A	14
	#define DEVICE_NAME            "Demonstrator"
#endif

#ifdef LCD_H_
	#define PIN_NUM_MISO (gpio_num_t)25
	#define PIN_NUM_MOSI (gpio_num_t)23
	#define PIN_NUM_CLK  (gpio_num_t)19
	#define PIN_NUM_CS   (gpio_num_t)22

	#define PIN_NUM_DC   (gpio_num_t)21
	#define PIN_NUM_RST  (gpio_num_t)18
	#define PIN_NUM_BCKL (gpio_num_t)5
#endif

#ifdef MQTTCLIENT_H_
	#define MQTT_STORAGE    "mqtt_config"
	#define STORAGE_URL     "mqtt_url"
	#define STORAGE_PORT    "mqtt_port"
#endif

#ifdef SENSORCLIENT_H_
	#define	I2C_MASTER_NUM	(i2c_port_t)0
	#define I2C_MASTER_SDA_IO	(gpio_num_t)4
	#define I2C_MASTER_SCL_IO	(gpio_num_t)0
	#define I2C_MASTER_FREQ_HZ	10000

	#define DEFAULT_VREF    1100
	#define NO_OF_SAMPLES   128
	#define ADC_CHANNEL     ADC_CHANNEL_0
	#define ADC_ATTEN       ADC_ATTEN_DB_0 

	#define BATTERIE_MAX_VOLTAGE 4200 //In mV
	#define DOWNGRADE_VOLTAGE 4500 //In mV
	#define NBR_TENSION_SAMPLE 13
	#define ADC_VOLTAGE 1100 //In mV /*WARNING*\ Do not change
#endif

#ifdef OTA_H_
	#define OTA_BUF_SIZE 256
	#define BIN_URL	"http://testbin.alwaysdata.net/board-%s.bin" //%s needed and replaced by current version X.X
#endif

#define	BTN_LEFT (gpio_num_t)35
#define	BTN_RIGHT (gpio_num_t)27
#define	BTN_TOP (gpio_num_t)34
#define	BTN_BOTTOM (gpio_num_t)39
#define	BTN_CLICK (gpio_num_t)26
#define BTN_MASK    ((1ULL<<BTN_LEFT) | (1ULL<<BTN_RIGHT) | (1ULL<<BTN_TOP) | (1ULL<<BTN_BOTTOM) | (1ULL<<BTN_CLICK))

#define RGB_1_RED 32
#define RGB_1_GREEN 15
#define RGB_2_RED 33
#define RGB_2_GREEN 12
#define RGB_3_RED 2
#define RGB_3_GREEN 13
#define RGB_3_BLUE 14
#define RGB_MASK    ((1ULL<<RGB_1_RED) | (1ULL<<RGB_1_GREEN) | (1ULL<<RGB_2_RED) | (1ULL<<RGB_2_GREEN) | (1ULL<<RGB_3_RED) | (1ULL<<RGB_3_GREEN) | (1ULL<<RGB_3_BLUE))

#define RGB_ON	0
#define RGB_OFF	1

#ifndef	STACK_QUEUE
	#define STACK_QUEUE 10
#endif

#ifndef	BUFF_SIZE
	#define	BUFF_SIZE 512
#endif

#endif