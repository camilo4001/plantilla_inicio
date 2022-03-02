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


// VAR SPI
spi_device_handle_t spi;

#define OTA_URL_SIZE 256 
//Interrupciones
#define ESP_INTR_FLAG_DEFAULT 0
static xQueueHandle gpio_evt_queue = NULL;
int int_terminada = 1;


//Interrupcion interruptores
void IRAM_ATTR gpio_isr_handler(void* arg) {
  // Funcion de llegada despues de interrupcion
  uint32_t gpio_num = (uint32_t) arg;
  
  if(int_terminada==1){
	int_terminada=0;
	//ESP_LOGI(INT_TAG,"ENTRA A INTERRUPCIÓN");
	xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
  }
}


void tarea_ciclo(void *pvParameter)
{
	
	uint32_t io_num;

	//obtener_versiones();
	
	while (1) {
		
		if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
			
			switch(io_num){

    			case CAMB_MENU:
					ESP_LOGI(INFO_TAG, "Entra a cambio de Menu");
					break;
					
				case CAMB_PANT:
					ESP_LOGI(INFO_TAG, "Entra a cambio pantalla");
					break;
					
				case SELECCION:
					ESP_LOGI(INFO_TAG, "Entra a seleccion");
					break;
			}
			
			
		}
		
        vTaskDelay(500 / portTICK_PERIOD_MS);
		int_terminada = 1;
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
	//BOTONES CUBO PRINCIPAL----------------------------------------------------------------------------
	gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);// INSTALA SERVICIO ISR POR DEFECTO.
	
	gpio_set_direction(CAMB_MENU, GPIO_MODE_INPUT); //32
    gpio_set_pull_mode(CAMB_MENU,GPIO_PULLUP_ONLY);
	gpio_isr_handler_add(CAMB_MENU, gpio_isr_handler, (void*) CAMB_MENU);
    gpio_set_intr_type(CAMB_MENU, GPIO_INTR_NEGEDGE);
	
	gpio_set_direction(CAMB_PANT, GPIO_MODE_INPUT); //27
    gpio_set_pull_mode(CAMB_PANT,GPIO_PULLUP_ONLY);
	gpio_isr_handler_add(CAMB_PANT, gpio_isr_handler, (void*) CAMB_PANT);
    gpio_set_intr_type(CAMB_PANT, GPIO_INTR_NEGEDGE);
	
	gpio_set_direction(SELECCION, GPIO_MODE_INPUT); //5
    gpio_set_pull_mode(SELECCION,GPIO_PULLUP_ONLY);
	gpio_isr_handler_add(SELECCION, gpio_isr_handler, (void*) SELECCION);
    gpio_set_intr_type(SELECCION, GPIO_INTR_NEGEDGE);
}


void app_main()
{

    // Initialize NVS.
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
	
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
	
	init_wifi();            // Inicia Wifi
	//http_server_ini();    // Inicia servidor web local
	//ble_inicio(); 		// Inicializar Bluetooth 
	tft_init();				// Inicializa lcd ST7789 pines a usar en principal.h 
	
	//Iniciar primer configuraciones
	fillScreen(TFTBACKCOLOR);     // PINTA LCD DE COLOR DE FONDO
	ESP_LOGI(INFO_TAG,"SE PINTA EN NEGRO");
	
	
	escribir_algo("HOLA A TODOS",12,1,2,1,TFTLETRACOLOR,TFTBACKCOLOR); // Mensaje , largo_del_mensaje, linea LCD, Tamaño, Offset en X, ColorLetra, Color fondo
	escribir_algo("...A PROGRAMAR",14,3,2,0,TFTLETRACOLOR,TFTBACKCOLOR); // Mensaje , largo_del_mensaje, linea LCD, Tamaño, Offset en X, ColorLetra, Color fondo
	
	gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
	
	//**********************
    //xTaskCreate(&simple_ota_example_task, "ota_example_task", 8192, NULL, 5, NULL);
    xTaskCreate(&tarea_ciclo, "tarea_ciclo", 8192, NULL, 5, NULL);

}
