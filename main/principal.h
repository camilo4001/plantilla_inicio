//CONSTANTES
#define INFO_TAG "INFO_PROCESS"
#define SPI_TAG "SPIINFO"

//--------------------------------------------------------- USAR ESTOS PINES PARA ESTANDAR EN LCD ST7789
//**********************SPI 3 LCD PRINCIPAL ST7789
#define SPI_BUS  HSPI_HOST
#define PIN_NUM_MISO 25
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18 //18

//********************* TFT ST7789
#define RST_PIN      2//
#define DC_PIN       15//
//---------------------------------------------------------

//*********************WIFI
#define CONFIG_ESP_WIFI_SSID  "Tomas_Prueba"
#define CONFIG_ESP_WIFI_PASSWORD  "contraPrueba"



//********************* Botones  // 3 BOTONES ESTANDAR
#define CAMB_MENU    32
#define SELECCION    27
#define CAMB_PANT    5

//******pantalla tft
// Some ready-made 16-bit ('565') color settings:
#define   ST7789_BLACK   0x0000
#define   ST7789_BLUE    0x001F
#define   ST7789_RED     0xF800
#define   ST7789_GREEN   0x07E0
#define   ST7789_CYAN    0x07FF
#define   ST7789_MAGENTA 0xF81F
#define   ST7789_YELLOW  0xFFE0
#define   ST7789_ORANGE  0xFCA0
//#define   ST7789_ORANGE  0xFD00
#define   ST7789_WHITE   0xFFFF
#define   ST7789_PURPLE  0x945F

#define   TFTWIDHT  240
#define   TFTHIGH  240

#define   TFTBACKCOLOR	 	0x0000
#define   TFTLETRACOLOR	 	0xFFFF
#define   TFTSELECTORCOLOR	0xFCA0

#define   TFT_ALTURA_MENU    46	 
#define   TFT_MARGEN_MENU_SUP    24	  
#define   TFT_MARGEN_MENU_INF    24	 
#define   TFT_MARGEN_MARCO_SUP   18	 

#define   TFTMENU_POR_PAGINA  5	

//DEFINICION DE FUNCIONES
void http_server_ini();
void init_wifi();
void ble_inicio();
//*****
void tft_init();
void tft_init(void);
void fillScreen(uint16_t color);
void testdrawtext(char *text, uint16_t color);
void escribir_algo(char * text,int text_len,int renglon,uint8_t tam_tex,int cursor_x,uint16_t color,uint16_t back_color);
//*****
void enviar_valor_spi(const uint8_t cmd);
