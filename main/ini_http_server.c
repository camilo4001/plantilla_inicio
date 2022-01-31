/*
   GPIO control over http server.
   Big thanks to esp-idf examples for wifi connecting and lwip httpserver_netconn example
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/api.h"
#include "principal.h"
#include "dht11.h"

int state = 0;
const static char http_html_hdr[] = "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n";
const static char comando_response[] = "<html><head>   <title>Control</title>   <style>	body {background-color: white;}	h1   {color: #0d651c;}    h3   {color: black;}	p    {color: black;}.default {  table-layout: fixed;  width: 100%;  border-collapse: collapse;  border: 3px solid #49d834;  text-align:center;  }     </style>    </head> <body><h1>&#161;COMANDOS ENVIADOS&#33;</h1> <br> <a href=\"/#\">Clic aqu&iacute; para volver a la pantalla inicial  &nbsp;&nbsp;&#x21a9;</a> <br> </body></html>";
//const static char http_index_hml[] = "<html><head><title>Control</title></head><body><h1>Control</h1><a href=\"h\">On</a><br><a href=\"l\">Off</a></body></html>";


//VARIBLES DE SENSORES
extern int sensor_suelo;
extern struct dht11_reading dht11_datos;
// Luz 1
extern uint8_t  pwm_luz_sal1;
extern uint8_t  pwm_horario_encendido;
//SAL1
extern uint8_t  intervalo_encendido_sal1;
extern uint8_t duracion_encedido_salida1;

//registro salidas
extern uint8_t registro_salidas; // cada bit representa el estado de una salida 1=prendido 0=Apagado  bits altos
extern uint8_t salidas_activas;  // bit bajos
//Fecha
extern char fecha_str[64];
extern uint8_t comando_env[];
extern uint8_t ant_comando_env[];
//#define EXAMPLE_WIFI_SSID "Tomas_2G"
//#define EXAMPLE_WIFI_PASS "tomas4982"
//const int DIODE_PIN = 5;

//static EventGroupHandle_t wifi_event_group;
//const int CONNECTED_BIT = BIT0;

extern char fecha_reg1[64];
extern char fecha_reg2[64];
extern char fecha_reg3[64];


extern char rep_str_sal1a[64];
extern char rep_str_sal1b[64];
extern char rep_str_sal1c[64];

extern char rep_str_sal2a[64];
extern char rep_str_sal2b[64];
extern char rep_str_sal2c[64];

extern char rep_str_sal3a[64];
extern char rep_str_sal3b[64];
extern char rep_str_sal3c[64];

extern char rep_str_sal4a[64];
extern char rep_str_sal4b[64];
extern char rep_str_sal4c[64];

extern uint8_t	hist1temp;
extern uint8_t	hist1tempdec;
extern uint8_t	hist1humed;
extern uint8_t	hist1suelo;

extern uint8_t	hist2temp;
extern uint8_t	hist2tempdec;
extern uint8_t	hist2humed;
extern uint8_t	hist2suelo;

extern uint8_t	hist3temp;
extern uint8_t	hist3tempdec;
extern uint8_t	hist3humed;
extern uint8_t	hist3suelo;

//extern int final_sensor_suelo;
extern uint8_t suelo_medida_base;
extern uint8_t umbral_disparos_suelo;
extern uint8_t numero_disparos_suelo;
extern uint8_t modo_func_suelo;

extern uint8_t estado_comando;

extern uint8_t tiempo_falt_act_sal1;

extern uint8_t horaaini;
extern uint8_t minutoini;
extern uint8_t esperando_ini;

static void
http_server_netconn_serve(struct netconn *conn)
{
  struct netbuf *inbuf;
  char *buf;
  u16_t buflen;
  err_t err;
  char *html_message = NULL;
  int habvsal1 = 0;
  int habvsal2 = 0;
  int habvsal3 = 0;
  int habvsal4 = 0;
  int actvsal1 = 0;
  int actvsal2 = 0;
  int actvsal3 = 0;
  int actvsal4 = 0;
  int umb_suelo = 0;
  int dis_suelo = 0;
  int mod_suelo = 0;
  int dur_sal1 = 0;
  int int_sal1 = 0;
  char enc_tiempo[10] = "Activado";
  char enc_sensor[10] = "Activado";
  char modo_sensor[15] = "Cont Humedad";
  char hora_inicio[15] = "Establecida";
  char comando_actual[15] = "NA       ";
  char comando_estado[10] = "NA     ";
  int sal_hab1 = 1;
  int sal_hab2 = 1;
  int sal_hab3 = 1;
  int sal_hab4 = 1;
  int sal_activa1 = 1;
  int sal_activa2 = 1;
  int sal_activa3 = 1;
  int sal_activa4 = 1;
  int var_n_disparos_suelo = numero_disparos_suelo;
  int horaini = 0;
  int minuini = 0;

  /* Read the data from the port, blocking if nothing yet there.
   We assume the request (the part we care about) is in one netbuf */
  err = netconn_recv(conn, &inbuf);

  if (err == ERR_OK) {
    netbuf_data(inbuf, (void**)&buf, &buflen);

	//printf("***********PETICION OBTEBIDA WEB  ***************** valores %c%c%c%c%c%c%c%c%c%c\n", buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7],buf[8],buf[9]);
	//printf("valor incial petición *************++ %c \n",buf[0]);
	//printf("COMPARANDO GET EN INICIAL = %d  \n",strncmp(buf,"GET",3));
    /* Is this an HTTP GET command? (only check the first 5 chars, since
    there are other formats for GET, and we're keeping it very simple )*/
    if (buflen>=5 &&
        buf[0]=='G' &&
        buf[1]=='E' &&
        buf[2]=='T' &&
        buf[3]==' ' &&
        buf[4]=='/' ) {
          printf("%c\n", buf[5]);

       if(buf[5]=='?' && ant_comando_env[0] == DEFECTO_COMANDO){
		   
		   printf("Entra a encontrar valores \n");
		   	
			if(buf[6]=='h' && buf[7]=='a' && buf[8]=='b'){
				sscanf(buf, FORMATO_PETICION_WEB_HAB,&habvsal1,&habvsal2,&habvsal3,&habvsal4);
				printf("valor encontrados vs1 = %d vs2 = %d vs3 = %d vs4 = %d\n",habvsal1,habvsal2,habvsal3,habvsal4);
				comando_env[0] = INHABILITAR_SALIDAS;
				comando_env[1] = (habvsal1*0x08)|(habvsal2*0x04)|(habvsal3*0x02)|(habvsal4*0x01);
				estado_comando = COMANDO_GUARDADO;
				printf("valor final variable salidas %d",comando_env[1]);
			}
			if(buf[6]=='a' && buf[7]=='c' && buf[8]=='t'){
				sscanf(buf, FORMATO_PETICION_WEB_ACT,&actvsal1,&actvsal2,&actvsal3,&actvsal4);
				printf("valor encontrados vs1 = %d vs2 = %d vs3 = %d vs4 = %d\n",actvsal1,actvsal2,actvsal3,actvsal4);
				comando_env[0] = ACTIVAR_SALIDA;
				comando_env[1] = (actvsal1*0x80)|(actvsal2*0x40)|(actvsal3*0x20)|(actvsal4*0x10);
				estado_comando = COMANDO_GUARDADO;
				printf("valor final variable salidas %d",comando_env[1]);
			}
			if(buf[6]=='u' && buf[7]=='m' && buf[8]=='b'){
				sscanf(buf, FORMATO_PETICION_WEB_UMB,&umb_suelo);
				if(umb_suelo >= 0){
					printf("valor encontrados umbral_suelo = %d\n",umb_suelo);
					comando_env[0] = CAMB_UMBRAL_SUELO;
					comando_env[1] = umb_suelo;
					estado_comando = COMANDO_GUARDADO;
				}
			}
			if(buf[6]=='d' && buf[7]=='i' && buf[8]=='s'){
				sscanf(buf, FORMATO_PETICION_WEB_DIS,&dis_suelo);
				if(dis_suelo >= 0){
					printf("valor encontrados ventana_suelo = %d\n",dis_suelo);
					comando_env[0] = CAMB_DISPARADOR_SUELO;
					comando_env[1] = dis_suelo;
					estado_comando = COMANDO_GUARDADO;
				}
			}
			if(buf[6]=='m' && buf[7]=='o' && buf[8]=='d'){
				sscanf(buf, FORMATO_PETICION_WEB_MOD,&mod_suelo);
				if(mod_suelo >= 0){
					printf("valor encontrados modo = %d\n",mod_suelo);
					comando_env[0] = CAMB_MODO_SUELO;
					comando_env[1] = mod_suelo;
					estado_comando = COMANDO_GUARDADO;
				}
			}
			if(buf[6]=='d' && buf[7]=='u' && buf[8]=='r'){
				// Tiempo asumido en minutos
				sscanf(buf, FORMATO_PETICION_WEB_DUR,&dur_sal1);
				if(dur_sal1 > 0){
					printf("Duracion encendido salida = %d\n",dur_sal1);
					comando_env[0] = CAMB_REG_DUR_SAL1;
					comando_env[1] = dur_sal1;
					estado_comando = COMANDO_GUARDADO;
				}
			}
			if(buf[6]=='i' && buf[7]=='n' && buf[8]=='t'){
				// Tiempo asumido en horas
				sscanf(buf, FORMATO_PETICION_WEB_INT,&int_sal1);
				if(int_sal1 >= 0){
					printf("valor encontrados intervalo encendido salida = %d\n",int_sal1);
					comando_env[0] = CAMB_REG_INTERVALO_SAL1;
					comando_env[1] = int_sal1;
					estado_comando = COMANDO_GUARDADO;
				}
			}			
			if(buf[6]=='h' && buf[7]=='o' && buf[8]=='r'){
				// Tiempo asumido en horas
				sscanf(buf, FORMATO_PETICION_WEB_SINC,&horaini,&minuini);
				if(horaini >= 0 && minuini >= 0){
					horaaini=horaini;
					minutoini=minuini;
					printf("Hora a establecer hora = %d  minuto = %d\n",horaaini,minutoini);
					comando_env[0] = SINC_HORA_ESP;
					comando_env[1] = 44;
					esperando_ini = 1;
					estado_comando = COMANDO_GUARDADO;
					nvs_poner_valres_ini();
				}
				//else{esperando_ini=0;}
			}
			
		
		  aplicar_comandos(comando_env[0],comando_env[1]);
			
		  ESP_LOGI("HTMLINFO", "\n ****** HTML A MOSTRAR \"%s\" ***tamaño %d************\n", comando_response,sizeof(comando_response)-1);

		  netconn_write(conn, http_html_hdr, sizeof(http_html_hdr)-1, NETCONN_NOCOPY);
		  netconn_write(conn, comando_response, sizeof(comando_response)-1, NETCONN_NOCOPY);
			
       
	   }
	   else{
		   
		   if(intervalo_encendido_sal1==0){
			   enc_tiempo[0] = 'A';
			   enc_tiempo[1] = 'p';
			   enc_tiempo[2] = 'a';
			   enc_tiempo[3] = 'g';
			   enc_tiempo[4] = 'a';
			   enc_tiempo[5] = 'd';
			   enc_tiempo[6] = 'o';
			   enc_tiempo[7] = ' ';
			   enc_tiempo[8] = ' ';
			   //enc_tiempo[0] = "Apagado";
		   }
		   if(suelo_medida_base==0){
			   enc_sensor[0] = 'A';
			   enc_sensor[1] = 'p';
			   enc_sensor[2] = 'a';
			   enc_sensor[3] = 'g';
			   enc_sensor[4] = 'a';
			   enc_sensor[5] = 'd';
			   enc_sensor[6] = 'o';
			   enc_sensor[7] = ' ';
			   enc_sensor[8] = ' ';
			   //enc_sensor = "Apagado";
		   }
		   if(modo_func_suelo==1){
			   modo_sensor[0] = 'A';
			   modo_sensor[1] = 'c';
			   modo_sensor[2] = 't';
			   modo_sensor[3] = ' ';
			   modo_sensor[4] = 'S';
			   modo_sensor[5] = 'a';
			   modo_sensor[6] = 'l';
			   modo_sensor[7] = 'i';
			   modo_sensor[8] = 'd';
			   modo_sensor[9] = 'a';
			   modo_sensor[10] = ' ';
			   modo_sensor[11] = ' ';
			   //enc_sensor = "Apagado";
		   }	
		   
		   if(esperando_ini==1){
			   hora_inicio[0] = 'P';
			   hora_inicio[1] = 'e';
			   hora_inicio[2] = 'n';
			   hora_inicio[3] = 'd';
			   hora_inicio[4] = 'i';
			   hora_inicio[5] = 'e';
			   hora_inicio[6] = 'n';
			   hora_inicio[7] = 't';
			   hora_inicio[8] = 'e';
			   hora_inicio[9] = ' ';
			   hora_inicio[10] = ' ';
			   hora_inicio[11] = ' ';
		   }			   
		   if(esperando_ini==2){
			   hora_inicio[0] = 'E';
			   hora_inicio[1] = 'n';
			   hora_inicio[2] = 'c';
			   hora_inicio[3] = 'o';
			   hora_inicio[4] = 'n';
			   hora_inicio[5] = 't';
			   hora_inicio[6] = 'r';
			   hora_inicio[7] = 'a';
			   hora_inicio[8] = 'd';
			   hora_inicio[9] = 'a';
			   hora_inicio[10] = ' ';
			   hora_inicio[11] = ' ';
		   }		   
		   if(esperando_ini==3){
			   hora_inicio[0] = 'E';
			   hora_inicio[1] = 'n';
			   hora_inicio[2] = 'v';
			   hora_inicio[3] = 'i';
			   hora_inicio[4] = 'a';
			   hora_inicio[5] = 'n';
			   hora_inicio[6] = 'd';
			   hora_inicio[7] = 'o';
			   hora_inicio[8] = ' ';
			   hora_inicio[9] = ' ';
			   hora_inicio[10] = ' ';
			   hora_inicio[11] = ' ';
		   }
		   
		   

		  if((salidas_activas&0x08) == 0){
			sal_hab1 = 0;
		  }
		  if((salidas_activas&0x04) == 0){
			sal_hab2 = 0;
		  }
		  if((salidas_activas&0x02) == 0){
			sal_hab3 = 0;
		  }
		  if((salidas_activas&0x01) == 0){
			sal_hab4 = 0;		  
		  }	
		  if((registro_salidas&0x80) == 0){
			sal_activa1 = 0;
		  }
		  if((registro_salidas&0x40) == 0){
			sal_activa2 = 0;
		  }
		  if((registro_salidas&0x20) == 0){
			sal_activa3 = 0;
		  }
		  if((registro_salidas&0x10) == 0){
			sal_activa4 = 0;
		  }

		  if(comando_env[0] == ACTIVAR_SALIDA){
			comando_actual[0] = 'A';
			comando_actual[1] = 'c';
			comando_actual[2] = 't';
			comando_actual[3] = ' ';
			comando_actual[4] = 'S';
			comando_actual[5] = 'a';
			comando_actual[6] = 'l';
			comando_actual[7] = 'i';
			comando_actual[8] = 'd';
			comando_actual[9] = 'a';
		  }
		  if(comando_env[0] == INHABILITAR_SALIDAS){
			comando_actual[0] = 'D';
			comando_actual[1] = 'e';
			comando_actual[2] = 's';
			comando_actual[3] = ' ';
			comando_actual[4] = 'S';
			comando_actual[5] = 'a';
			comando_actual[6] = 'l';
			comando_actual[7] = 'i';
			comando_actual[8] = 'd';
			comando_actual[9] = 'a';
		  }		  
		  if(comando_env[0] == CAMB_REG_DUR_SAL1){
			comando_actual[0] = 'D';
			comando_actual[1] = 'u';
			comando_actual[2] = 'r';
			comando_actual[3] = ' ';
			comando_actual[4] = 'S';
			comando_actual[5] = 'a';
			comando_actual[6] = 'l';
			comando_actual[7] = 'i';
			comando_actual[8] = 'd';
			comando_actual[9] = 'a';
		  }
		  if(comando_env[0] == CAMB_REG_INTERVALO_SAL1){
			comando_actual[0] = 'I';
			comando_actual[1] = 'n';
			comando_actual[2] = 't';
			comando_actual[3] = ' ';
			comando_actual[4] = 'S';
			comando_actual[5] = 'a';
			comando_actual[6] = 'l';
			comando_actual[7] = 'i';
			comando_actual[8] = 'd';
			comando_actual[9] = 'a';
		  }
		  if(comando_env[0] == CAMB_UMBRAL_SUELO){
			comando_actual[0] = 'U';
			comando_actual[1] = 'm';
			comando_actual[2] = 'b';
			comando_actual[3] = ' ';
			comando_actual[4] = 'S';
			comando_actual[5] = 'u';
			comando_actual[6] = 'e';
			comando_actual[7] = 'l';
			comando_actual[8] = 'o';
			comando_actual[9] = ' ';
		  }
		  if(comando_env[0] == CAMB_DISPARADOR_SUELO){
			comando_actual[0] = 'D';
			comando_actual[1] = 'i';
			comando_actual[2] = 's';
			comando_actual[3] = 'p';
			comando_actual[4] = ' ';
			comando_actual[5] = 'S';
			comando_actual[6] = 'u';
			comando_actual[7] = 'e';
			comando_actual[8] = 'l';
			comando_actual[9] = 'o';
		  }		  
		  
		  if(comando_env[0] == SINC_HORA_ESP){
			comando_actual[0] = 'S';
			comando_actual[1] = 'i';
			comando_actual[2] = 'n';
			comando_actual[3] = 'c';
			comando_actual[4] = ' ';
			comando_actual[5] = 'H';
			comando_actual[6] = 'o';
			comando_actual[7] = 'r';
			comando_actual[8] = 'a';
			comando_actual[9] = ' ';
		  }		  
		  if(comando_env[0] == SINC_HORA_INI){
			comando_actual[0] = 'A';
			comando_actual[1] = 'c';
			comando_actual[2] = 't';
			comando_actual[3] = ' ';
			comando_actual[4] = 'S';
			comando_actual[5] = 'i';
			comando_actual[6] = 'n';
			comando_actual[7] = 'c';
			comando_actual[8] = ' ';
			comando_actual[9] = ' ';
		  }
		  

		  if(estado_comando == COMANDO_ENVIADO){
			comando_estado[0] = 'E';
			comando_estado[1] = 'n';
			comando_estado[2] = 'v';
			comando_estado[3] = 'i';
			comando_estado[4] = 'a';
			comando_estado[5] = 'd';
			comando_estado[6] = 'o';
			comando_estado[7] = ' ';

		  }		  
		  if(estado_comando == COMANDO_GUARDADO){
			comando_estado[0] = 'G';
			comando_estado[1] = 'u';
			comando_estado[2] = 'a';
			comando_estado[3] = 'r';
			comando_estado[4] = 'd';
			comando_estado[5] = 'a';
			comando_estado[6] = 'd';
			comando_estado[7] = 'o';

		  }
		  if(estado_comando == COMANDO_APLICADO){
			comando_estado[0] = 'A';
			comando_estado[1] = 'p';
			comando_estado[2] = 'l';
			comando_estado[3] = 'i';
			comando_estado[4] = 'c';
			comando_estado[5] = 'a';
			comando_estado[6] = 'd';
			comando_estado[7] = 'o';

		  }
		  if(estado_comando == COMANDO_FALLIDO){
			comando_estado[0] = 'F';
			comando_estado[1] = 'a';
			comando_estado[2] = 'l';
			comando_estado[3] = 'l';
			comando_estado[4] = 'i';
			comando_estado[5] = 'd';
			comando_estado[6] = 'o';
			comando_estado[7] = ' ';
		  }		  
		  
		  
		  asprintf(&html_message, FORMATO_HTML,dht11_datos.temperature,dht11_datos.temperature_decimal,dht11_datos.humidity,sensor_suelo,suelo_medida_base,umbral_disparos_suelo,modo_sensor,horaaini,minutoini,hora_inicio,duracion_encedido_salida1,intervalo_encendido_sal1,enc_tiempo,enc_sensor,tiempo_falt_act_sal1,var_n_disparos_suelo,comando_actual,comando_env[1],comando_estado,fecha_reg1,hist1temp,hist1tempdec,hist1humed,hist1suelo,fecha_reg2,hist2temp,hist2tempdec,hist2humed,hist2suelo,fecha_reg3,hist3temp,hist3tempdec,hist3humed,hist3suelo,rep_str_sal1a,rep_str_sal1b,rep_str_sal1c,rep_str_sal2a,rep_str_sal2b,rep_str_sal2c,rep_str_sal3a,rep_str_sal3b,rep_str_sal3c,rep_str_sal4a,rep_str_sal4b,rep_str_sal4c,sal_activa1,sal_activa2,sal_activa3,sal_activa4,sal_hab1,sal_hab2,sal_hab3,sal_hab4);

		  //ESP_LOGI("HTMLINFO", "\n ****** HTML A MOSTRAR \"%s\" ***tamaño %d************\n", html_message,strlen(html_message)-1);

		  netconn_write(conn, http_html_hdr, sizeof(http_html_hdr)-1, NETCONN_NOCOPY);
		  netconn_write(conn, html_message, strlen(html_message)-1, NETCONN_NOCOPY);
	  }
    }

  }
  /* Close the connection (server closes in HTTP) */
  netconn_close(conn);

  /* Delete the buffer (netconn_recv gives us ownership,
   so we have to make sure to deallocate the buffer) */
  netbuf_delete(inbuf);
}

static void http_server(void *pvParameters)
{
  struct netconn *conn, *newconn;
  err_t err;
  conn = netconn_new(NETCONN_TCP);
  netconn_bind(conn, NULL, 80);
  netconn_listen(conn);
  do {
     err = netconn_accept(conn, &newconn);
     if (err == ERR_OK) {
       http_server_netconn_serve(newconn);
       netconn_delete(newconn);
     }
   } while(err == ERR_OK);
   netconn_close(conn);
   netconn_delete(conn);
}

void http_server_ini()
{
	printf("********INICIA HTTP SERVER**************\n");
    //nvs_flash_init();
    //sys_init();
    //initialise_wifi();
    xTaskCreate(&http_server, "http_server", 4096, NULL, 5, NULL);
}