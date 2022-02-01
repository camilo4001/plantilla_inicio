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

int state = 0;
const static char http_html_hdr[] = "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n";
const static char comando_response[] = "<html><head>   <title>Control</title>   <style>	body {background-color: white;}	h1   {color: #0d651c;}    h3   {color: black;}	p    {color: black;}.default {  table-layout: fixed;  width: 100%;  border-collapse: collapse;  border: 3px solid #49d834;  text-align:center;  }     </style>    </head> <body><h1>&#161;COMANDOS ENVIADOS&#33;</h1> <br> <a href=\"/#\">Clic aqu&iacute; para volver a la pantalla inicial  &nbsp;&nbsp;&#x21a9;</a> <br> </body></html>";
//const static char http_index_hml[] = "<html><head><title>Control</title></head><body><h1>Control</h1><a href=\"h\">On</a><br><a href=\"l\">Off</a></body></html>";



//#define EXAMPLE_WIFI_SSID "Tomas_2G"
//#define EXAMPLE_WIFI_PASS "tomas4982"
//const int DIODE_PIN = 5;

//static EventGroupHandle_t wifi_event_group;
//const int CONNECTED_BIT = BIT0;


static void
http_server_netconn_serve(struct netconn *conn)
{
  struct netbuf *inbuf;
  char *buf;
  u16_t buflen;
  err_t err;
  char *html_message = NULL;

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

       if(buf[5]=='?'){
		   
		   printf("Entra a encontrar valores \n");
		   	/*
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
			*/
		
		  //aplicar_comandos(comando_env[0],comando_env[1]);
			
		  //ESP_LOGI("HTMLINFO", "\n ****** HTML A MOSTRAR \"%s\" ***tamaño %d************\n", comando_response,sizeof(comando_response)-1);

		  netconn_write(conn, http_html_hdr, sizeof(http_html_hdr)-1, NETCONN_NOCOPY);
		  netconn_write(conn, comando_response, sizeof(comando_response)-1, NETCONN_NOCOPY);
			
       
	   }
	   else{
		   
		  //asprintf(&html_message, FORMATO_HTML,dht11_datos.temperature,dht11_datos.temperature_decimal,dht11_datos.humidity,sensor_suelo,suelo_medida_base,umbral_disparos_suelo,modo_sensor,horaaini,minutoini,hora_inicio,duracion_encedido_salida1,intervalo_encendido_sal1,enc_tiempo,enc_sensor,tiempo_falt_act_sal1,var_n_disparos_suelo,comando_actual,comando_env[1],comando_estado,fecha_reg1,hist1temp,hist1tempdec,hist1humed,hist1suelo,fecha_reg2,hist2temp,hist2tempdec,hist2humed,hist2suelo,fecha_reg3,hist3temp,hist3tempdec,hist3humed,hist3suelo,rep_str_sal1a,rep_str_sal1b,rep_str_sal1c,rep_str_sal2a,rep_str_sal2b,rep_str_sal2c,rep_str_sal3a,rep_str_sal3b,rep_str_sal3c,rep_str_sal4a,rep_str_sal4b,rep_str_sal4c,sal_activa1,sal_activa2,sal_activa3,sal_activa4,sal_hab1,sal_hab2,sal_hab3,sal_hab4);

		  //ESP_LOGI("HTMLINFO", "\n ****** HTML A MOSTRAR \"%s\" ***tamaño %d************\n", html_message,strlen(html_message)-1);

		  //netconn_write(conn, http_html_hdr, sizeof(http_html_hdr)-1, NETCONN_NOCOPY);
		  //netconn_write(conn, html_message, strlen(html_message)-1, NETCONN_NOCOPY);
		  netconn_write(conn, http_html_hdr, sizeof(http_html_hdr)-1, NETCONN_NOCOPY);
		  netconn_write(conn, comando_response, sizeof(comando_response)-1, NETCONN_NOCOPY);
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