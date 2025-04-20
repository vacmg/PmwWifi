#ifndef PMW_WIFI_H
#define PMW_WIFI_H

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include "esp_wifi.h"
#include <list>
#include "EspOSInterface.h"

class PMW_Wifi
{
public:
    static void printConfig();
    static PMW_Wifi* getInstance();

    esp_err_t initialize();
    esp_err_t finalize();

    esp_err_t scan(std::list<wifi_ap_record_t>& ap_record_list);
    esp_err_t connect(const char ssid[sizeof(wifi_config_t::sta.ssid)], const char password[sizeof(wifi_config_t::sta.password)]);

    ~PMW_Wifi();

    constexpr static const char* TAG = "PMW-WIFI";
private:
    PMW_Wifi();

    esp_err_t initializeDriver();
    esp_err_t finalizeDriver();
    esp_err_t startWifi();
    esp_err_t stopWifi();

    static void event_handler(void* selfPtr, esp_event_base_t event_base,
                                int32_t event_id, void* event_data);

    static PMW_Wifi* instance;
    static EspOSInterface osInterface;

    EventGroupHandle_t s_wifi_event_group;
    esp_netif_t* netif_wifi;
    bool esp_wifi_started;
    bool esp_wifi_initialized;

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    uint32_t timeout_timestamp;
};

#endif // PMW_WIFI_H
