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
const static char comando_response[] = "<html><head>   <title>Control</title>   <style>	body {background-color: white;}	h1   {color: #0d651c;}    h3   {color: black;}	p    {color: black;}.default {  table-layout: fixed;  width: 100%;  border-collapse: collapse;  border: 3px solid #49d834;  text-align:center;  }     </style>    </head> <body><h1>&#161;HOLA MUNDO SERVER&#33;</h1> <br> <a href=\"/#\">Clic aqu&iacute; para volver a la pantalla inicial  &nbsp;&nbsp;&#x21a9;</a> <br> </body></html>";
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
		   	
		  netconn_write(conn, http_html_hdr, sizeof(http_html_hdr)-1, NETCONN_NOCOPY);
		  netconn_write(conn, comando_response, sizeof(comando_response)-1, NETCONN_NOCOPY);
			
       
	   }
	   else{
		   
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
    xTaskCreate(&http_server, "http_server", 4096, NULL, 5, NULL);
}