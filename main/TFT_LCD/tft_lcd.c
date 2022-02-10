#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include "esp_log.h"


/*
#include "../matriz8x8.h"
#include "../Macros_pines.h"
*/
#include "../principal.h"


#include "tft_lcd.h"
#include "font8x8_basic.h"

void drawPixel(uint8_t x, uint8_t y, uint16_t color);
void disp_spi_transfer_cmd_data(uint8_t *data, uint32_t len);
void setAddrWindow(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void fillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color);
void display_print(uint8_t c);
void testdrawtext(char *text, uint16_t color);
void enviar_valor_spi(const uint8_t cmd);
void setAddrWindow(uint8_t x, uint8_t y, uint8_t w, uint8_t h);



extern const unsigned char	A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z,Espac,n2;
extern const unsigned char	N0,N1,N2,N3,N4,N5,N6,N7,N8,N9;
extern const unsigned char  MAYndatos,MAYancho,Amay,Mmay,Apmin,Smay,Nmay,Cmay,Mtest;
extern const unsigned char Amin,Bmin,Cmin,Dmin,Emin,Rmin,Tmin,Imin,Zmin,Omin,Smin,Nmin,Mmin,Umin,TILDE;
extern const unsigned char flecha_der,flecha_izq;
extern const unsigned char	CFeliz,CPreo,CTriste,CEnojado;

//extern int cursor_x;


void escribir_algo(char * text,int text_len,int renglon,uint8_t tam_tex,int cursor_x,uint16_t color,uint16_t back_color)
{
	//printf("Letra (%s) obtenida!\n", text);

	uint8_t image[8];
	int _text_len = text_len;
	int ancho = 8;
	int alto = 8;
	if (_text_len > 16) _text_len = 16;
	
	cursor_x =(ancho*2)*cursor_x;
	
	for (uint8_t i = 0; i < _text_len; i++) {
	
		memcpy(image, font8x8_basic_tr[(uint8_t)text[i]], 8);
		Enviar_msn_tft(&image,8,cursor_x,renglon*alto*tam_tex,ancho,color,back_color,tam_tex);
		//Enviar_msn_tft(const unsigned char *p_msn,int ndatos,int posx, int posy,int ancho,uint16_t color,uint16_t back_color)
		cursor_x+=(ancho*2);
		//printf("valor (%d) reading!\n", cursor_x);
		//cursor_x+=5;
	}
	cursor_x=0;
	
}

void dib_menu_inicial(int menu){

	
	escribir_menus(0,45,24,ST7789_WHITE,ST7789_BLACK);
	escribir_menus(0,45,70,ST7789_WHITE,ST7789_BLACK);
	escribir_menus(0,45,116,ST7789_WHITE,ST7789_BLACK);
	escribir_menus(0,45,162,ST7789_WHITE,ST7789_BLACK);
		
}

void Enviar_msn_tft(const unsigned char *p_msn,int ndatos,int posx, int posy,int ancho,uint16_t color,uint16_t back_color,uint8_t tam_tex)
{
	unsigned int cuenta;
	unsigned int cur_x = 0;
	

	uint8_t line = 0;
	//uint8_t tam_tex = 2;
	
	/*
	if(*(p_msn+2)!=0){
		ndatos=*(p_msn+2);
	}
	else{
		ndatos=*(p_msn+0)**(p_msn+1);
		printf("valor NUMERO DATOS ENVIAR(%d) reading!\n", ndatos);
	}*/

	for( cuenta=0;cuenta<ndatos;cuenta++ )
		/* While data remains to be sent */
    	{
			/*
			if(cuenta==0){
				ancho = *(p_msn+cuenta);
				cursor_x += ancho;
				continue;
			}
			if(cuenta==1){
				//alto
				continue;
			}
			if(cuenta==2){
				//ndatos
				continue;
			}
			*/
		    //line = (uint8_t) p_msn+cuenta;
		    line = *(p_msn+cuenta);
			

			/*
		    if(cur_x > ancho-1){
		    	posy += 8;
		    	// posx = posx;
		    	cur_x = 0;
		    }*/
			

			//if(line != 0){

				setAddrWindow(posx+cur_x, posy, 1, 8);

				for(uint8_t j = 0; j < 8; j++) {

					if(line & 1) { 
						// decide pintar un solo pixel se debe agregar pintal un cuadrado para aumentar el tamaño del pixel
						if(tam_tex == 1){
							enviar_valor_spi(color >> 8);
							enviar_valor_spi(color & 0xFF);
						}
						else{
							fillRect(posx+cur_x * tam_tex, posy + j * tam_tex, tam_tex, tam_tex, color);
						}
					}
					else{
						if(tam_tex == 1){
							enviar_valor_spi(back_color >> 8);
							enviar_valor_spi(back_color & 0xFF);
						}
						else{
							fillRect(posx+cur_x * tam_tex, posy + j * tam_tex, tam_tex, tam_tex, back_color);
						}
					}
					
					line =  line>> 1;

				}
			//}

			cur_x += 1;

    	}

	return;
}

void Limpiar_tft_menu(){
	 fillRect(0, 10, 240, 230, TFTBACKCOLOR);
}

void color_tft_rect(uint16_t color){
	 fillRect(20, 10, 200, 230, color);
}




