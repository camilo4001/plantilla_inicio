/*
 * ble_inicial.c
 *
 *  Created on: 7 feb. 2019
 *      Author: Jake
 */

/* Copyright (c) 2017 pcbreflux. All Rights Reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>. *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
//#include "bta_api.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
//#include "esp_bt_main.h"

#include "sdkconfig.h"

#include "ble_uart_server.h"

#define GATTS_TAG "MAIN"

void ble_inicio() {
    esp_err_t ret;
	
	ESP_LOGI(GATTS_TAG,"INICIA CONFIG BLE");

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s initialize controller failed\n", __func__);
        return;
    }
	
	ESP_LOGI(GATTS_TAG,"ACTIVA CONTROLADOR BLE");

    ret = esp_bt_controller_enable(ESP_BT_MODE_BTDM);
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s enable controller failed\n", __func__);
        return;
    }
	
	ESP_LOGI(GATTS_TAG,"INICIA BLUETOOTH");
	
    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s init bluetooth failed\n", __func__);
        return;
    }
	
	ESP_LOGI(GATTS_TAG,"HABILIT BLUETOOTH");
    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s enable bluetooth failed\n", __func__);
        return;
    }
	
	ESP_LOGI(GATTS_TAG,"INICIA TAREAS DE EVENTOS DE BLUETOOTH");

    esp_ble_gatts_register_callback(gatts_event_handler);
    esp_ble_gap_register_callback(gap_event_handler);
    esp_ble_gatts_app_register(BLE_PROFILE_APP_ID);

    return;
}


void deshabilitar_ble(){
	//Funciones a tener encuenta al dormir para deshabilitar bluethooth
	esp_bluedroid_disable();
	esp_bt_controller_disable();
}

void habilitar_ble(){
	
	esp_err_t ret;
	
	ESP_LOGE(GATTS_TAG, "Enable Controller");
	ret = esp_bt_controller_enable(ESP_BT_MODE_BTDM);
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s enable controller failed\n", __func__);
        return;
    }
	
	ESP_LOGI(GATTS_TAG,"Enable Bluedorid");
    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s enable bluetooth failed\n", __func__);
        return;
    }
	
}
