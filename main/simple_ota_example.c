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

/*
char imagenes_var[20][10] ={{"","","","","","","","","",""},
 {"","","","","","","","","",""},{"","","","","","","","","",""},{"","","","","","","","","",""},{"","","","","","","","","",""},
 {"","","","","","","","","",""},{"","","","","","","","","",""},{"","","","","","","","","",""},{"","","","","","","","","",""},
 {"","","","","","","","","",""},{"","","","","","","","","",""},{"","","","","","","","","",""},{"","","","","","","","","",""},
 {"","","","","","","","","",""},{"","","","","","","","","",""},{"","","","","","","","","",""},{"","","","","","","","","",""},
 {"","","","","","","","","",""},{"","","","","","","","","",""},{"","","","","","","","","",""}
 };
*/

// VAR SPI
spi_device_handle_t spi;

// Wifi
char *ip_str = NULL;

#define OTA_URL_SIZE 256 
//Interrupciones
#define ESP_INTR_FLAG_DEFAULT 0
static xQueueHandle gpio_evt_queue = NULL;
int int_terminada = 1;

// HTTP Var
char *output_buffer = NULL;  // Buffer to store response of http request from event handler
//int output_len=0;       // Stores number of bytes read

uint8_t menu_pos=0;
uint8_t max_menu=7;
uint8_t cant_menu=0;


char menu_titulos[8][13]={"             ","             ","             ","             ",
						  "             ","             ","             ","             "};
int pos_imagen = 0;


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
		if (output_buffer == NULL) {
			output_buffer = (char *) malloc(evt->data_len);
			//output_len = evt->data_len;
			if (output_buffer == NULL) {
				ESP_LOGE(INFO_TAG, "buffer Null Failed to allocate memory for output buffer");
				return ESP_FAIL;
			}
		}
		
		memcpy(output_buffer, evt->data, (evt->data_len)+1);
		
		//asprintf(&output_buffer, "%s",evt->data);
		
		ESP_LOGI(INFO_TAG, "HTTP_EVENT_ON_DATA, DATOS=%s", output_buffer);
    
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


void tarea_ciclo(void *pvParameter)
{
	
	uint32_t io_num;
	char *mensaje_str = NULL;
	char cadena[13];

	obtener_versiones();
	
	while (1) {
		
		if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
			
			switch(io_num){

    			case CAMB_MENU:
					ESP_LOGI(INFO_TAG, "Entra a cambio de Menu");
					
					if(menu_pos == cant_menu){
						menu_pos = 0;
					}
					else{
						menu_pos += 1;
					}
					
					ESP_LOGI(INFO_TAG, "Valor Menu %d",menu_pos);
					
					switch(menu_pos){
						case 0:
						borrar_menu((cant_menu*2)-1);
						break;
						case 1:
						camb_menu(1,1);
						break;
						case 2:
						camb_menu(1,3);
						break;
						case 3:
						camb_menu(3,5);
						break;
						case 4:
						camb_menu(5,7);
						break;
						case 5:
						camb_menu(7,9);
						break;
						case 6:
						camb_menu(9,11);
						break;
						case 7:
						camb_menu(11,13);
						break;

					}
					break;
					
				case CAMB_PANT:
					ESP_LOGI(INFO_TAG, "Entra a cambio pantalla");
					obtener_versiones();
					break;
					
				case SELECCION:
				
					strncpy(cadena,menu_titulos[menu_pos-1],13);
				
					ESP_LOGI(INFO_TAG, "Entra a seleccion");
					ESP_LOGI(INFO_TAG, "MENU ---%s",cadena);
					
					asprintf(&mensaje_str, "https://innovacionesco.com:3389/%s\n",cadena);

					ESP_LOGI(INFO_TAG,"IMAGEN APUNTAR %s",mensaje_str);
					//obtener_versiones();
					break;
			}
			
			
		}
		
        vTaskDelay(500 / portTICK_PERIOD_MS);
		int_terminada = 1;
    }
	
}

void graficar_menu(){
	
	int linea_lcd = 1;
	ESP_LOGI(INFO_TAG,"TAMAÑO MENU ==== %d",sizeof(menu_titulos[0]));
	ESP_LOGI(INFO_TAG,"TAMAÑO MENU ==== %d",strlen(menu_titulos[0]));
	ESP_LOGI(INFO_TAG,"PRIMER MENU==== %s",menu_titulos[0]);
	
	for(int i=0;i<max_menu;i++){
	   //printf("FALTA PARA CENTRALIZAR %d \n", (15-strlen(menu_titulos[j]))/2);
		escribir_algo(&menu_titulos[i],13,linea_lcd,2,1,TFTLETRACOLOR,TFTBACKCOLOR);
		linea_lcd += 2;
	}
					
}


void limpiar_var_menu(){
	
	//char *mensaje_str = NULL;
	
	char val = ' ';
	for(int i=0;i<max_menu;i++){	
		for(int j=0;j<13;j++){
			menu_titulos[i][j]=val;
		}
	}
}

void obtener_versiones(){
	
	char *token;
	char delimitador[] = ",";
	uint8_t linea_lcd = 0;
	int i = 0;
	char *mensaje_str = NULL;
	int v_longitud=0;

	
	ESP_LOGI(INFO_TAG, "Starting GET example");
	
	asprintf(&mensaje_str, "https://innovacionesco.com:3389/imagenes/%d",pos_imagen);
	
	esp_http_client_config_t config = {
		.url = mensaje_str,
		.cert_pem = (char *)server_cert_pem_start,
		.event_handler = _http_event_handler,
	};
	
	ESP_LOGI(INFO_TAG, "Url obtenida para get:-----");
	printf("url: %s \n",mensaje_str);
	esp_http_client_handle_t client = esp_http_client_init(&config);

	// GET
	ESP_LOGI(INFO_TAG, "Efectua el GET:-----");
	esp_err_t err = esp_http_client_perform(client);
	if (err == ESP_OK) {
		ESP_LOGI(INFO_TAG, "HTTP GET Status = %d, content_length = %d",
				esp_http_client_get_status_code(client),
				esp_http_client_get_content_length(client));
		
		
		token = strtok(output_buffer, delimitador);
		if(token != NULL){
			linea_lcd = 0;
			limpiar_var_menu();
			if(strncmp(token,TAG_ENTRADA,3) == 0){
				escribir_algo("             ",13,1,2,0,TFTLETRACOLOR,TFTBACKCOLOR);
				while(token != NULL && strncmp(token,TAG_SALIDA,3) != 0){
					ESP_LOGI(INFO_TAG,"TOKEN ==== %d",*(token+1));
					ESP_LOGI(INFO_TAG,"TAMAÑO TOKEN ==== %d",strlen(token));
					// Sólo en la primera pasamos la cadena; en las siguientes pasamos NULL
					if(linea_lcd == 0){
						linea_lcd += 1;
						token = strtok(NULL, delimitador);
						ESP_LOGI(INFO_TAG,"LONGITUD Token: %s\n", token);
						sscanf(token, "%d",&v_longitud);
						ESP_LOGI(INFO_TAG,"VALOR LONGITUD ENTERO: %d\n", v_longitud);
						token = strtok(NULL, delimitador);
						continue;
					}
					ESP_LOGI(INFO_TAG,"Token: %s\n", token);
					
					for(int j=0;j<strlen(token);j++){
						menu_titulos[i][j]=*(token+j);
					}
					ESP_LOGI(INFO_TAG,"MENU GUARDADO ==== %s",menu_titulos[i]);
					//escribir_algo(token,strlen(token),linea_lcd,2,(15-strlen(token))/2,TFTLETRACOLOR,TFTBACKCOLOR);
					token = strtok(NULL, delimitador);
					i++; 
					linea_lcd += 2;
				}
			}

		}
		
		borrar_menu((menu_pos*2)-1);
		menu_pos = 0;

		cant_menu = i;		
		if((v_longitud-i)>0){
			pos_imagen += i;
		}
		else{
			pos_imagen = 0;
		}

		ESP_LOGI(INFO_TAG,"POS_IMAGEN==== %d",pos_imagen);
		graficar_menu();
		output_buffer = NULL;
		
	} else {
		ESP_LOGE(INFO_TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
		output_buffer = NULL;
	}
	//ESP_LOG_BUFFER_HEX(INFO_TAG, local_response_buffer, strlen(local_response_buffer));
	
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

void camb_menu(int renglon_ant,int renglon_act){
	
	
	escribir_algo(" ",1,renglon_ant,2,0,ST7789_YELLOW,TFTBACKCOLOR);
	escribir_algo(" ",1,renglon_ant,2,14,ST7789_YELLOW,TFTBACKCOLOR);
	
	escribir_algo("=",1,renglon_act,2,0,ST7789_ORANGE,TFTBACKCOLOR);
	escribir_algo("=",1,renglon_act,2,14,ST7789_ORANGE,TFTBACKCOLOR);	
}
void borrar_menu(int renglon){
	ESP_LOGI(INFO_TAG,"Borrar renglon %d",renglon);
	escribir_algo(" ",1,renglon,2,0,ST7789_YELLOW,TFTBACKCOLOR);
	escribir_algo(" ",1,renglon,2,14,ST7789_YELLOW,TFTBACKCOLOR);
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
	
	/*
	escribir_algo("Conectando...",13,1,2,0,TFTLETRACOLOR,TFTBACKCOLOR);
	
	escribir_algo("Menu1",5,1,2,5,TFTLETRACOLOR,TFTBACKCOLOR);
	escribir_algo("Menu2",5,3,2,5,TFTLETRACOLOR,TFTBACKCOLOR);
	escribir_algo("Menu3",5,5,2,5,TFTLETRACOLOR,TFTBACKCOLOR);
	escribir_algo("Menu4",5,7,2,5,TFTLETRACOLOR,TFTBACKCOLOR);
	escribir_algo("Menu5",5,9,2,5,TFTLETRACOLOR,TFTBACKCOLOR);
	escribir_algo("Menu6",5,11,2,5,TFTLETRACOLOR,TFTBACKCOLOR);
	escribir_algo("Menu7",5,13,2,5,TFTLETRACOLOR,TFTBACKCOLOR);
	
	*/
	
	//escribir_algo("MenuN",5,14,2,5,TFTLETRACOLOR,TFTBACKCOLOR);

	
	//escribir_algo("*",1,3,2,0,ST7789_YELLOW,TFTBACKCOLOR);
	//escribir_algo("*",1,3,2,14,ST7789_YELLOW,TFTBACKCOLOR);
	
	//vTaskDelay(3000 / portTICK_PERIOD_MS);
	//camb_menu(3,5);
	
	//create a queue to handle gpio event from isr
	char *mensaje_str = NULL;
	int ojo_tam = 4;
	
	escribir_algo("\\",1,3,3,4,TFTLETRACOLOR,TFTBACKCOLOR);
	escribir_algo("/",1,3,3,9,TFTLETRACOLOR,TFTBACKCOLOR);
	escribir_algo("-",1,3,ojo_tam,4,TFTLETRACOLOR,TFTBACKCOLOR);
	escribir_algo("-",1,3,ojo_tam,9,TFTLETRACOLOR,TFTBACKCOLOR);
	escribir_algo("-----",ojo_tam,7,3,5,TFTLETRACOLOR,TFTBACKCOLOR);
	
	vTaskDelay(1000 / portTICK_PERIOD_MS);
	while(1){
		//probar animaciones
		
		// PARPADEO
		for(int x=0;x<=3;x++){

			vTaskDelay(2000 / portTICK_PERIOD_MS);
			
			escribir_algo("-",1,3,ojo_tam,4,TFTLETRACOLOR,TFTBACKCOLOR);
			escribir_algo("-",1,3,ojo_tam,9,TFTLETRACOLOR,TFTBACKCOLOR);
			escribir_algo("-----",4,7,3,5,TFTLETRACOLOR,TFTBACKCOLOR);
			
			vTaskDelay(300 / portTICK_PERIOD_MS);
			
			escribir_algo("O",1,3,ojo_tam,4,TFTLETRACOLOR,TFTBACKCOLOR);
			escribir_algo("O",1,3,ojo_tam,9,TFTLETRACOLOR,TFTBACKCOLOR);
			escribir_algo("-----",4,7,3,5,TFTLETRACOLOR,TFTBACKCOLOR);
		}
		//GUIÑO IZQUIERDO
		vTaskDelay(1000 / portTICK_PERIOD_MS);

		escribir_algo("O",1,3,ojo_tam,4,TFTLETRACOLOR,TFTBACKCOLOR);
		escribir_algo("-",1,3,ojo_tam,9,TFTLETRACOLOR,TFTBACKCOLOR);
		escribir_algo("-----",4,7,3,5,TFTLETRACOLOR,TFTBACKCOLOR);
		
		vTaskDelay(500 / portTICK_PERIOD_MS);
		
		escribir_algo("O",1,3,ojo_tam,4,TFTLETRACOLOR,TFTBACKCOLOR);
		escribir_algo("O",1,3,ojo_tam,9,TFTLETRACOLOR,TFTBACKCOLOR);
		escribir_algo("-----",4,7,3,5,TFTLETRACOLOR,TFTBACKCOLOR);	
		
		//GUIÑO DERECHO
		vTaskDelay(1000 / portTICK_PERIOD_MS);

		escribir_algo("O",1,3,ojo_tam,9,TFTLETRACOLOR,TFTBACKCOLOR);
		escribir_algo("-",1,3,ojo_tam,4,TFTLETRACOLOR,TFTBACKCOLOR);
		escribir_algo("-----",4,7,3,5,TFTLETRACOLOR,TFTBACKCOLOR);
		
		vTaskDelay(500 / portTICK_PERIOD_MS);
		
		escribir_algo("O",1,3,ojo_tam,9,TFTLETRACOLOR,TFTBACKCOLOR);
		escribir_algo("O",1,3,ojo_tam,4,TFTLETRACOLOR,TFTBACKCOLOR);
		escribir_algo("-----",4,7,3,5,TFTLETRACOLOR,TFTBACKCOLOR);	
		vTaskDelay(1000 / portTICK_PERIOD_MS);

		//Sorpersa
		escribir_algo("O",1,3,ojo_tam,9,TFTLETRACOLOR,TFTBACKCOLOR);
		escribir_algo("O",1,3,ojo_tam,4,TFTLETRACOLOR,TFTBACKCOLOR);
		escribir_algo("-----",4,7,3,5,TFTLETRACOLOR,TFTBACKCOLOR);	
		escribir_algo("O",1,3,ojo_tam,9,TFTLETRACOLOR,TFTBACKCOLOR);
		escribir_algo("O",1,3,ojo_tam,4,TFTLETRACOLOR,TFTBACKCOLOR);
		escribir_algo(" () ",4,7,3,5,TFTLETRACOLOR,TFTBACKCOLOR);
		vTaskDelay(2000 / portTICK_PERIOD_MS);
		escribir_algo("O",1,3,ojo_tam,9,TFTLETRACOLOR,TFTBACKCOLOR);
		escribir_algo("O",1,3,ojo_tam,4,TFTLETRACOLOR,TFTBACKCOLOR);
		escribir_algo("----",4,7,3,5,TFTLETRACOLOR,TFTBACKCOLOR);
		
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		escribir_algo("o",1,3,ojo_tam,4,TFTLETRACOLOR,TFTBACKCOLOR);
		escribir_algo("O",1,3,ojo_tam,9,TFTLETRACOLOR,TFTBACKCOLOR);
		escribir_algo("-----",ojo_tam,7,3,5,TFTLETRACOLOR,TFTBACKCOLOR);
		vTaskDelay(2000 / portTICK_PERIOD_MS);
		
		escribir_algo("^",1,3,ojo_tam,4,TFTLETRACOLOR,TFTBACKCOLOR);
		escribir_algo("^",1,3,ojo_tam,9,TFTLETRACOLOR,TFTBACKCOLOR);
		escribir_algo("-----",ojo_tam,7,3,5,TFTLETRACOLOR,TFTBACKCOLOR);
		vTaskDelay(2000 / portTICK_PERIOD_MS);

		/*
		escribir_algo("-",1,1,2,0,TFTLETRACOLOR,TFTBACKCOLOR);
		for(int x=10;x<=99;x++){
			asprintf(&mensaje_str, "%d",x);
			escribir_algo(mensaje_str,2,1,2,0,TFTLETRACOLOR,TFTBACKCOLOR);
			//escribir_algo("-",2,1,2,x+1,TFTLETRACOLOR,TFTBACKCOLOR);
			//vTaskDelay(5 / portTICK_PERIOD_MS);
		}
		*/
		/*
		escribir_algo("      ---      ",15,7,2,0,TFTLETRACOLOR,TFTBACKCOLOR);
		vTaskDelay(2 / portTICK_PERIOD_MS);
		escribir_algo("     -----     ",15,7,2,0,TFTLETRACOLOR,TFTBACKCOLOR);
		vTaskDelay(2 / portTICK_PERIOD_MS);
		escribir_algo("    -------    ",15,7,2,0,TFTLETRACOLOR,TFTBACKCOLOR);
		vTaskDelay(2 / portTICK_PERIOD_MS);
		escribir_algo("     -----     ",15,7,2,0,TFTLETRACOLOR,TFTBACKCOLOR);
		vTaskDelay(2 / portTICK_PERIOD_MS);
		escribir_algo("      ---      ",15,7,2,0,TFTLETRACOLOR,TFTBACKCOLOR);
		*/
	}
	
	gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
	
	//**********************
    //xTaskCreate(&simple_ota_example_task, "ota_example_task", 8192, NULL, 5, NULL);
    xTaskCreate(&tarea_ciclo, "tarea_ciclo", 8192, NULL, 5, NULL);

}
