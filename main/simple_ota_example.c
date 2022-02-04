/* OTA example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "protocol_examples_common.h"
#include "string.h"

#include "nvs.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "principal.h"

//static const char *TAG = "simple_ota_example";
extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

// VAR SPI
spi_device_handle_t spi;

// Wifi
char *ip_str = NULL;

#define OTA_URL_SIZE 256 

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
        ESP_LOGI(INFO_TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(INFO_TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGI(INFO_TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGI(INFO_TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGI(INFO_TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
		ESP_LOGI(INFO_TAG, "HTTP_EVENT_ON_DATA, MSG=%c", evt->data);
		printf(evt);
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGI(INFO_TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(INFO_TAG, "HTTP_EVENT_DISCONNECTED");
        break;
	//case HTTP_EVENT_REDIRECT:
	//	ESP_LOGI(INFO_TAG, "HTTP_EVENT_REDIRECT");
    //    break;
    }
    return ESP_OK;
}


void simple_get_example_task(void *pvParameter)
{
	while (1) {
		ESP_LOGI(INFO_TAG, "Starting GET example");

		esp_http_client_config_t config = {
			.url = "https://innovacionesco.com:3389/imagenes",
			.cert_pem = (char *)server_cert_pem_start,
			.event_handler = _http_event_handler,
		};
		
		ESP_LOGI(INFO_TAG, "Url obtenida para get:-----");
		
		esp_http_client_handle_t client = esp_http_client_init(&config);

		// GET
		esp_err_t err = esp_http_client_perform(client);
		if (err == ESP_OK) {
			ESP_LOGI(INFO_TAG, "HTTP GET Status = %d, content_length = %d",
					esp_http_client_get_status_code(client),
					esp_http_client_get_content_length(client));
		} else {
			ESP_LOGE(INFO_TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
		}
		//ESP_LOG_BUFFER_HEX(INFO_TAG, local_response_buffer, strlen(local_response_buffer));
		
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
	
}

void simple_ota_example_task(void *pvParameter)
{
    ESP_LOGI(INFO_TAG, "Starting OTA example");

    esp_http_client_config_t config = {
        .url = "https://innovacionesco.com:3389/ssd1306",
        .cert_pem = (char *)server_cert_pem_start,
        .event_handler = _http_event_handler,
    };
	
	ESP_LOGI(INFO_TAG, "Url obtenida :-----");
	
//#ifdef CONFIG_EXAMPLE_SKIP_COMMON_NAME_CHECK
    //config.skip_cert_common_name_check = true;
//#endif

	//ESP_LOGI(TAG, "Confiuracion obtenida :-----");
	//printf(config);

    esp_err_t ret = esp_https_ota(&config);
	printf(config.url);
	
    if (ret == ESP_OK) {
        esp_restart();
    } else {
        ESP_LOGE(INFO_TAG, "Firmware upgrade failed");
    }
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void enviar_valor_spi(const uint8_t cmd)
{
	spi_transaction_t t;
	spi_transaction_t v;
	//int resp = 0;

    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length=8;                     //Command is 8 bits
    t.tx_buffer=&cmd;               //Envia el comando o la direccion
    //t.rx_buffer = miso;

    //if (spi_device_transmit(spi, &t) != ESP_OK)
	if (spi_device_polling_transmit(spi, &t) != ESP_OK)
    {
    	ESP_LOGI(SPI_TAG,"ERROR ENVIANDO MENSAJE");
    }


    //ESP_LOGI(SPI_TAG,"SE COMPLETA ENVIO");

}


void ini_puertos(){

	//PINES MANEJO LCD TFT-----------------------------------------------------------------------------
	gpio_pad_select_gpio(RST_PIN); //2
	gpio_pad_select_gpio(DC_PIN);  //15
	//gpio_pad_select_gpio(SSLCD);   //0

	gpio_set_direction(RST_PIN, GPIO_MODE_OUTPUT); //2
	gpio_set_direction(DC_PIN, GPIO_MODE_OUTPUT); //15
    gpio_set_level(DC_PIN, 1);
    gpio_set_level(RST_PIN, 1);
	//------------------------------------------------------------------------------------------------
	
}


void app_main()
{

    // Initialize NVS.
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // 1.OTA app partition table has a smaller NVS partition size than the non-OTA
        // partition table. This size mismatch may cause NVS initialization to fail.
        // 2.NVS partition contains data in new format and cannot be recognized by this version of code.
        // If this happens, we erase NVS partition and initialize NVS again.
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());

	//Codigo Nuevo**********
	
	//Inicia spi	//SPI3
    spi_bus_config_t spi_bus_cfg = {
        //.miso_io_num=PIN_NUM_MISO,
        .mosi_io_num=PIN_NUM_MOSI,
        .sclk_io_num=PIN_NUM_CLK,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1,
		.max_transfer_sz=16*240*2+8

    };

    spi_device_interface_config_t dev_cfg = {
            .clock_speed_hz = 8000000,           
            .mode = 2,                       // SPI mode 0
			.queue_size = 7,
	};

    if(spi_bus_initialize(SPI_BUS, &spi_bus_cfg, 1) != ESP_OK)
    {
    	ESP_LOGI(SPI_TAG,"ERROR INICIALIZAR BUS SPI");
    }

    if (spi_bus_add_device(SPI_BUS, &dev_cfg, &spi) != ESP_OK)
    {
    	ESP_LOGI(SPI_TAG,"ERROR EN ADICIONAR DISPOSITIVO");
    }
	ESP_LOGI(SPI_TAG,"SPI INICIADO");
	

	
	ini_puertos();
	gpio_set_level(DC_PIN, 1);
    gpio_set_level(RST_PIN, 1);
	
	//Inicializa modulos
	
	init_wifi();
	//http_server_ini();
	//ble_inicio();
	tft_init();
	
	//Iniciar primer configuraciones
	fillScreen(TFTBACKCOLOR);
	ESP_LOGI(INFO_TAG,"SE PINTA EN NEGRO");
	
	//**********************
    //xTaskCreate(&simple_ota_example_task, "ota_example_task", 8192, NULL, 5, NULL);
    xTaskCreate(&simple_get_example_task, "get_example_task", 8192, NULL, 5, NULL);
}
