
//FUNCIONES
void dib_menu_inicial(int menu);
void Limpiar_tft_menu();
void Enviar_msn_tft(const unsigned char *p_msn,int ndatos,int posx, int posy,int ancho,uint16_t color,uint16_t back_color,uint8_t tam_tex);
void escribir_menus(int menu,int posx,int posy,uint16_t color,uint16_t back_color );
void marco_menu(int posx, int posy,int alto,int ancho,uint16_t color);
void color_tft_rect(uint16_t color);