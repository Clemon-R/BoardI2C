#include "ButtonClient.h"
#include "freertos/queue.h"

#define BTN_CLICKED 1

static const char	*TAG = "\033[1;43mButtonClient\033[0m";

static xQueueHandle gpio_evt_queue = NULL;
static char	_running = false;
static void	(*_callback)(uint32_t io_num, TypeClick type) = NULL;

static void IRAM_ATTR   gpio_isr_handler(void* arg) //Recepteur du trigger
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL); //Reception l'event et sera receptionné dans une task
}

static esp_err_t	initButtonGpio()
{
    gpio_config_t	gpio_conf;
    esp_err_t	ret;

    ESP_LOGI(TAG, "Initiating GPIO Buttons...");
    gpio_conf.intr_type = GPIO_INTR_ANYEDGE;
    gpio_conf.pin_bit_mask = (1ULL<<BTN_1) | (1ULL<<BTN_2) ;
    gpio_conf.mode = GPIO_MODE_INPUT;
    gpio_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    ret = gpio_config(&gpio_conf);
    if (ret != ESP_OK)
        return ret;

    gpio_install_isr_service(0);
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    if (gpio_evt_queue == NULL)
        return ESP_FAIL;
    ret = gpio_isr_handler_add(BTN_1, gpio_isr_handler, (void*) BTN_1);
    if (ret != ESP_OK)
        return ret;
    ret = gpio_isr_handler_add(BTN_2, gpio_isr_handler, (void*) BTN_2);
    return ret;
}

static esp_err_t	deinitButtonGpio()
{
    gpio_config_t	gpio_conf;
    esp_err_t	ret;

    ESP_LOGI(TAG, "Deinitiating GPIO Buttons...");
    gpio_conf.pin_bit_mask = (1ULL<<BTN_1) | (1ULL<<BTN_2) ;
    gpio_conf.mode = GPIO_MODE_DISABLE;
    ret = gpio_config(&gpio_conf);
    if (ret != ESP_OK)
        return ret;
    ret = gpio_isr_handler_remove(BTN_1);
    if (ret != ESP_OK)
        return ret;
    ret = gpio_isr_handler_remove(BTN_2);
    gpio_uninstall_isr_service();
    return ret;
}

static void taskHandler(void* arg) //Tache de traitement des trigger
{
    uint32_t    io_num;
    uint32_t    clicked = 0;
    TickType_t last_click = xTaskGetTickCount();

    ESP_LOGI(TAG, "Initiating the task...");
    ESP_ERROR_CHECK(initButtonGpio());
    while(_running && _callback) {
        uint32_t current;

        if(xQueueReceive(gpio_evt_queue, &io_num, 200 / portTICK_PERIOD_MS) == pdTRUE) {
            TickType_t tick = xTaskGetTickCount();
            current = (tick - last_click)  * portTICK_PERIOD_MS;
            uint32_t state = gpio_get_level((gpio_num_t)io_num);
            if (clicked > 0 && clicked != io_num)
                continue;
            if (state != BTN_CLICKED) {
                clicked = 0;
                continue;
            }
            if (last_click > 0 && current <= 200) {
                (*_callback)(io_num, Double);
            } else {
                (*_callback)(io_num, Simple);
            }
            clicked = io_num;
            last_click = tick;
        } else if (clicked > 0 && (current = (xTaskGetTickCount() - last_click)  * portTICK_PERIOD_MS) >= 1000) {
            if (current > 2000)
                (*_callback)(clicked, VeryLong);
            else
                (*_callback)(clicked, Long);
        }
    }
    deinitButtonGpio();
    vTaskDelete(NULL);
}

esp_err_t	startButtonClient()
{
    ESP_LOGI(TAG, "Starting the %s task...", TAG);
    if (_running)
        return ESP_FAIL;
    _running = true;
    return xTaskCreate(taskHandler, "buttonTask", 2048, NULL, tskIDLE_PRIORITY, NULL);
}

esp_err_t	stopButtonClient()
{
    ESP_LOGI(TAG, "Stopping the %s task...", TAG);
    if (!_running)
        return ESP_FAIL;
    _running = false;
    return ESP_OK;
}

void	setButtonCallback(void	(*callback)(uint32_t io_num, TypeClick type))
{
    _callback = callback;
}