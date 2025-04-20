#include "PMW-Wifi.h"

#include <cstring>
#include <esp_check.h>

#define WIFI_CONNECTION_STATUS_TIMEOUT_MS (CONFIG_PMW_WIFI_CONNECTION_TIMEOUT_MS * 1.25)

#ifndef CONFIG_PMW_WIFI_PMF_REQUIRED
#define CONFIG_PMW_WIFI_PMF_REQUIRED false
#endif
#ifndef CONFIG_PMW_WIFI_PMF_CAPABLE
#define CONFIG_PMW_WIFI_PMF_CAPABLE false
#endif
#if CONFIG_PMW_WIFI_WPA3_SAE_PWE_HUNT_AND_PECK
#define PMW_WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
#define EXAMPLE_H2E_IDENTIFIER ""
#elif CONFIG_PMW_WIFI_WPA3_SAE_PWE_HASH_TO_ELEMENT
#define PMW_WIFI_SAE_MODE WPA3_SAE_PWE_HASH_TO_ELEMENT
#define EXAMPLE_H2E_IDENTIFIER CONFIG_PMW_WIFI_PW_ID
#elif CONFIG_PMW_WIFI_WPA3_SAE_PWE_BOTH
#define PMW_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define EXAMPLE_H2E_IDENTIFIER CONFIG_PMW_WIFI_PW_ID
#endif
#if CONFIG_PMW_WIFI_AUTH_OPEN
#define PMW_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_PMW_WIFI_AUTH_WEP
#define PMW_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_PMW_WIFI_AUTH_WPA_PSK
#define PMW_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_PMW_WIFI_AUTH_WPA2_PSK
#define PMW_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_PMW_WIFI_AUTH_WPA_WPA2_PSK
#define PMW_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_PMW_WIFI_AUTH_WPA3_PSK
#define PMW_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_PMW_WIFI_AUTH_WPA2_WPA3_PSK
#define PMW_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_PMW_WIFI_AUTH_WAPI_PSK
#define PMW_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

PMW_Wifi* PMW_Wifi::instance = nullptr;
EspOSInterface PMW_Wifi::osInterface;

const char* wifi_event_to_string(wifi_event_t event)
{
    switch (event)
    {
        case WIFI_EVENT_WIFI_READY: return "WIFI_EVENT_WIFI_READY";
        case WIFI_EVENT_SCAN_DONE: return "WIFI_EVENT_SCAN_DONE";
        case WIFI_EVENT_STA_START: return "WIFI_EVENT_STA_START";
        case WIFI_EVENT_STA_STOP: return "WIFI_EVENT_STA_STOP";
        case WIFI_EVENT_STA_CONNECTED: return "WIFI_EVENT_STA_CONNECTED";
        case WIFI_EVENT_STA_DISCONNECTED: return "WIFI_EVENT_STA_DISCONNECTED";
        case WIFI_EVENT_STA_AUTHMODE_CHANGE: return "WIFI_EVENT_STA_AUTHMODE_CHANGE";
        case WIFI_EVENT_STA_WPS_ER_SUCCESS: return "WIFI_EVENT_STA_WPS_ER_SUCCESS";
        case WIFI_EVENT_STA_WPS_ER_FAILED: return "WIFI_EVENT_STA_WPS_ER_FAILED";
        case WIFI_EVENT_STA_WPS_ER_TIMEOUT: return "WIFI_EVENT_STA_WPS_ER_TIMEOUT";
        case WIFI_EVENT_STA_WPS_ER_PIN: return "WIFI_EVENT_STA_WPS_ER_PIN";
        case WIFI_EVENT_STA_WPS_ER_PBC_OVERLAP: return "WIFI_EVENT_STA_WPS_ER_PBC_OVERLAP";
        case WIFI_EVENT_AP_START: return "WIFI_EVENT_AP_START";
        case WIFI_EVENT_AP_STOP: return "WIFI_EVENT_AP_STOP";
        case WIFI_EVENT_AP_STACONNECTED: return "WIFI_EVENT_AP_STACONNECTED";
        case WIFI_EVENT_AP_STADISCONNECTED: return "WIFI_EVENT_AP_STADISCONNECTED";
        case WIFI_EVENT_AP_PROBEREQRECVED: return "WIFI_EVENT_AP_PROBEREQRECVED";
        case WIFI_EVENT_FTM_REPORT: return "WIFI_EVENT_FTM_REPORT";
        case WIFI_EVENT_STA_BSS_RSSI_LOW: return "WIFI_EVENT_STA_BSS_RSSI_LOW";
        case WIFI_EVENT_ACTION_TX_STATUS: return "WIFI_EVENT_ACTION_TX_STATUS";
        case WIFI_EVENT_ROC_DONE: return "WIFI_EVENT_ROC_DONE";
        case WIFI_EVENT_STA_BEACON_TIMEOUT: return "WIFI_EVENT_STA_BEACON_TIMEOUT";
        case WIFI_EVENT_CONNECTIONLESS_MODULE_WAKE_INTERVAL_START:
            return "WIFI_EVENT_CONNECTIONLESS_MODULE_WAKE_INTERVAL_START";
        case WIFI_EVENT_AP_WPS_RG_SUCCESS: return "WIFI_EVENT_AP_WPS_RG_SUCCESS";
        case WIFI_EVENT_AP_WPS_RG_FAILED: return "WIFI_EVENT_AP_WPS_RG_FAILED";
        case WIFI_EVENT_AP_WPS_RG_TIMEOUT: return "WIFI_EVENT_AP_WPS_RG_TIMEOUT";
        case WIFI_EVENT_AP_WPS_RG_PIN: return "WIFI_EVENT_AP_WPS_RG_PIN";
        case WIFI_EVENT_AP_WPS_RG_PBC_OVERLAP: return "WIFI_EVENT_AP_WPS_RG_PBC_OVERLAP";
        case WIFI_EVENT_ITWT_SETUP: return "WIFI_EVENT_ITWT_SETUP";
        case WIFI_EVENT_ITWT_TEARDOWN: return "WIFI_EVENT_ITWT_TEARDOWN";
        case WIFI_EVENT_ITWT_PROBE: return "WIFI_EVENT_ITWT_PROBE";
        case WIFI_EVENT_ITWT_SUSPEND: return "WIFI_EVENT_ITWT_SUSPEND";
        case WIFI_EVENT_TWT_WAKEUP: return "WIFI_EVENT_TWT_WAKEUP";
        case WIFI_EVENT_BTWT_SETUP: return "WIFI_EVENT_BTWT_SETUP";
        case WIFI_EVENT_BTWT_TEARDOWN: return "WIFI_EVENT_BTWT_TEARDOWN";
        case WIFI_EVENT_NAN_STARTED: return "WIFI_EVENT_NAN_STARTED";
        case WIFI_EVENT_NAN_STOPPED: return "WIFI_EVENT_NAN_STOPPED";
        case WIFI_EVENT_NAN_SVC_MATCH: return "WIFI_EVENT_NAN_SVC_MATCH";
        case WIFI_EVENT_NAN_REPLIED: return "WIFI_EVENT_NAN_REPLIED";
        case WIFI_EVENT_NAN_RECEIVE: return "WIFI_EVENT_NAN_RECEIVE";
        case WIFI_EVENT_NDP_INDICATION: return "WIFI_EVENT_NDP_INDICATION";
        case WIFI_EVENT_NDP_CONFIRM: return "WIFI_EVENT_NDP_CONFIRM";
        case WIFI_EVENT_NDP_TERMINATED: return "WIFI_EVENT_NDP_TERMINATED";
        case WIFI_EVENT_HOME_CHANNEL_CHANGE: return "WIFI_EVENT_HOME_CHANNEL_CHANGE";
        case WIFI_EVENT_STA_NEIGHBOR_REP: return "WIFI_EVENT_STA_NEIGHBOR_REP";
        case WIFI_EVENT_MAX: return "WIFI_EVENT_MAX";
        default: return "Unknown Wi-Fi event";
    }
}

const char* ip_event_to_string(ip_event_t event)
{
    switch (event)
    {
        case IP_EVENT_STA_GOT_IP: return "IP_EVENT_STA_GOT_IP";
        case IP_EVENT_STA_LOST_IP: return "IP_EVENT_STA_LOST_IP";
        case IP_EVENT_AP_STAIPASSIGNED: return "IP_EVENT_AP_STAIPASSIGNED";
        case IP_EVENT_GOT_IP6: return "IP_EVENT_GOT_IP6";
        case IP_EVENT_ETH_GOT_IP: return "IP_EVENT_ETH_GOT_IP";
        case IP_EVENT_ETH_LOST_IP: return "IP_EVENT_ETH_LOST_IP";
        case IP_EVENT_PPP_GOT_IP: return "IP_EVENT_PPP_GOT_IP";
        case IP_EVENT_PPP_LOST_IP: return "IP_EVENT_PPP_LOST_IP";
        case IP_EVENT_TX_RX: return "IP_EVENT_TX_RX";
        default: return "Unknown IP event";
    }
}

void PMW_Wifi::event_handler(void* selfPtr, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    PMW_Wifi* self = static_cast<PMW_Wifi*>(selfPtr);

    if (event_base == WIFI_EVENT)
    {
        OSInterfaceLogDebug(TAG, "event_handler: event_base=%s, event_id=%s", event_base,
                            wifi_event_to_string(static_cast<wifi_event_t>(event_id)));
    }
    else if (event_base == IP_EVENT)
    {
        OSInterfaceLogDebug(TAG, "event_handler: event_base=%s, event_id=%s", event_base,
                            ip_event_to_string(static_cast<ip_event_t>(event_id)));
    }
    else
    {
        OSInterfaceLogDebug(TAG, "event_handler: event_base=%s, event_id=%ld", event_base, event_id);
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (const uint32_t millis = osInterface.osMillis(); millis < self->timeout_timestamp)
        {
            OSInterfaceLogWarning(TAG, "Wifi STA Disconnected, retrying connection (timeout in %lu ms)",
                                  self->timeout_timestamp - millis);
            esp_wifi_connect();
        }
        else
        {
            ESP_LOGW(TAG, "Connecting to the AP failed");
            xEventGroupSetBits(self->s_wifi_event_group, WIFI_FAIL_BIT);
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* event = static_cast<ip_event_got_ip_t*>(event_data);
        ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(self->s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void PMW_Wifi::printConfig()
{
    wifi_config_t wifi_config{};
    ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &wifi_config));

    ESP_LOGD(TAG, "******************");
    ESP_LOGD(TAG, "Wifi sta config: ");
    ESP_LOGD(TAG, "SSID: '%s'", wifi_config.sta.ssid);
    ESP_LOGD(TAG, "Password: '%s'", wifi_config.sta.password);
    ESP_LOGD(TAG, "Scan Method: %d", wifi_config.sta.scan_method); // Add enum to string conversion if available
    ESP_LOGD(TAG, "BSSID Set: %d", wifi_config.sta.bssid_set);
    ESP_LOGD(TAG, "BSSID: %02x:%02x:%02x:%02x:%02x:%02x", wifi_config.sta.bssid[0], wifi_config.sta.bssid[1],
             wifi_config.sta.bssid[2], wifi_config.sta.bssid[3], wifi_config.sta.bssid[4], wifi_config.sta.bssid[5]);
    ESP_LOGD(TAG, "Channel: %d", wifi_config.sta.channel);
    ESP_LOGD(TAG, "Listen Interval: %d", wifi_config.sta.listen_interval);
    ESP_LOGD(TAG, "Sort Method: %d", wifi_config.sta.sort_method); // Add enum to string conversion if available
    ESP_LOGD(TAG, "Threshold Authmode: %d", wifi_config.sta.threshold.authmode);
    // Add enum to string conversion if available
    ESP_LOGD(TAG, "Threshold RSSI: %d", wifi_config.sta.threshold.rssi);
    ESP_LOGD(TAG, "PMF Config Capable: %d", wifi_config.sta.pmf_cfg.capable);
    ESP_LOGD(TAG, "PMF Config Required: %d", wifi_config.sta.pmf_cfg.required);
    ESP_LOGD(TAG, "RM Enabled: %d", wifi_config.sta.rm_enabled);
    ESP_LOGD(TAG, "BTM Enabled: %d", wifi_config.sta.btm_enabled);
    ESP_LOGD(TAG, "MBO Enabled: %d", wifi_config.sta.mbo_enabled);
    ESP_LOGD(TAG, "FT Enabled: %d", wifi_config.sta.ft_enabled);
    ESP_LOGD(TAG, "OWE Enabled: %d", wifi_config.sta.owe_enabled);
    ESP_LOGD(TAG, "Transition Disable: %d", wifi_config.sta.transition_disable);
    ESP_LOGD(TAG, "SAE PWE H2E: %d", wifi_config.sta.sae_pwe_h2e); // Add enum to string conversion if available
    ESP_LOGD(TAG, "SAE PK Mode: %d", wifi_config.sta.sae_pk_mode); // Add enum to string conversion if available
    ESP_LOGD(TAG, "Failure Retry Count: %d", wifi_config.sta.failure_retry_cnt);
    ESP_LOGD(TAG, "HE DCM Set: %d", wifi_config.sta.he_dcm_set);
    ESP_LOGD(TAG, "HE DCM Max Constellation TX: %d", wifi_config.sta.he_dcm_max_constellation_tx);
    ESP_LOGD(TAG, "HE DCM Max Constellation RX: %d", wifi_config.sta.he_dcm_max_constellation_rx);
    ESP_LOGD(TAG, "HE MCS9 Enabled: %d", wifi_config.sta.he_mcs9_enabled);
    ESP_LOGD(TAG, "HE SU Beamformee Disabled: %d", wifi_config.sta.he_su_beamformee_disabled);
    ESP_LOGD(TAG, "HE Trig SU BMForming Feedback Disabled: %d", wifi_config.sta.he_trig_su_bmforming_feedback_disabled);
    ESP_LOGD(TAG, "HE Trig MU BMForming Partial Feedback Disabled: %d",
             wifi_config.sta.he_trig_mu_bmforming_partial_feedback_disabled);
    ESP_LOGD(TAG, "HE Trig CQI Feedback Disabled: %d", wifi_config.sta.he_trig_cqi_feedback_disabled);
    ESP_LOGD(TAG, "SAE H2E Identifier: '%s'", wifi_config.sta.sae_h2e_identifier);
    ESP_LOGD(TAG, "******************");
}

PMW_Wifi* PMW_Wifi::getInstance()
{
    if (instance == nullptr)
    {
        instance = new PMW_Wifi();
    }
    return instance;
}

PMW_Wifi::PMW_Wifi()
{
    timeout_timestamp = 0;
    esp_wifi_started = false;
    esp_wifi_initialized = false;
    netif_wifi = nullptr;
    s_wifi_event_group = nullptr;
    instance_any_id = nullptr;
    instance_got_ip = nullptr;
}

PMW_Wifi::~PMW_Wifi()
{
    if (esp_wifi_initialized)
    {
        OSInterfaceLogWarning(TAG, "Wi-Fi is still initialized, deinitializing");
        stopWifi();
        finalizeDriver();
    }
}

esp_err_t PMW_Wifi::initialize()
{
    const esp_err_t err = initializeDriver();
    if (err == ESP_OK)
    {
        return startWifi();
    }
    return err;
}

esp_err_t PMW_Wifi::finalize()
{
    if (const esp_err_t err = stopWifi(); err != ESP_OK)
    {
        finalizeDriver();
        return err;
    }
    return finalizeDriver();
}

esp_err_t PMW_Wifi::initializeDriver()
{
    if (esp_wifi_initialized)
    {
        OSInterfaceLogWarning(TAG, "Wi-Fi already initialized");
        return ESP_OK;
    }
    OSInterfaceLogInfo(TAG, "Initializing Wi-Fi");

    esp_err_t err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    {
        OSInterfaceLogError(TAG, "Failed to create default event loop");
        return err;
    }

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "", .password = "", .listen_interval = 0,
            /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (password len => 8).
             * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
             * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             */
            .threshold = {.rssi = -127, .authmode = PMW_WIFI_SCAN_AUTH_MODE_THRESHOLD},
            .pmf_cfg = {.capable = CONFIG_PMW_WIFI_PMF_CAPABLE, .required = CONFIG_PMW_WIFI_PMF_REQUIRED}, .sae_pwe_h2e = PMW_WIFI_SAE_MODE,
            .sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,
        },
    };

    netif_wifi = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&cfg), TAG, AT "Failed to initialize Wi-Fi");
    ESP_RETURN_ON_ERROR(esp_wifi_set_storage(WIFI_STORAGE_RAM), TAG, AT "Failed to set Wi-Fi storage to RAM");
    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, AT "Failed to set Wi-Fi mode to STA");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &wifi_config), TAG, AT "Failed to set Wi-Fi config");
    ESP_RETURN_ON_ERROR(esp_netif_set_hostname(netif_wifi, CONFIG_PMW_WIFI_HOSTNAME), TAG, AT "Failed to set hostname to %s",
                        CONFIG_PMW_WIFI_HOSTNAME);

    s_wifi_event_group = xEventGroupCreate();
    ESP_RETURN_ON_ERROR(s_wifi_event_group == nullptr ? ESP_ERR_NO_MEM : ESP_OK, TAG, AT "Failed to create event group");

    ESP_RETURN_ON_ERROR(
        esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, this, &instance_any_id), TAG,
        AT "Failed to register WIFI event handler");
    ESP_RETURN_ON_ERROR(
        esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, this, &instance_got_ip), TAG,
        AT "Failed to register IP event handler");

    esp_wifi_initialized = true;
    return ESP_OK;
}

esp_err_t PMW_Wifi::finalizeDriver()
{
    if (!esp_wifi_initialized)
    {
        OSInterfaceLogWarning(TAG, "Wi-Fi not initialized");
        return ESP_OK;
    }

    OSInterfaceLogInfo(TAG, "Deinitializing Wi-Fi");

    ESP_RETURN_ON_ERROR(stopWifi(), TAG, AT "Failed to stop Wi-Fi");

    ESP_RETURN_ON_ERROR(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip), TAG,
                        AT "Failed to unregister IP event handler");
    ESP_RETURN_ON_ERROR(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id), TAG,
                        AT "Failed to unregister WIFI event handler");

    vEventGroupDelete(s_wifi_event_group);

    ESP_RETURN_ON_ERROR(esp_wifi_deinit(), TAG, AT "Failed to deinitialize Wi-Fi");

    esp_netif_destroy_default_wifi(netif_wifi);

    esp_wifi_initialized = false;

    return ESP_OK;
}

esp_err_t PMW_Wifi::startWifi()
{
    if (!esp_wifi_started)
    {
        ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, AT "Failed to start Wi-Fi");
        esp_wifi_started = true;
        OSInterfaceLogInfo(TAG, "Wi-Fi started");
    }
    return ESP_OK;
}

esp_err_t PMW_Wifi::stopWifi()
{
    if (esp_wifi_started)
    {
        ESP_RETURN_ON_ERROR(esp_wifi_stop(), TAG, AT "Failed to stop Wi-Fi");
        esp_wifi_started = false;
        OSInterfaceLogInfo(TAG, "Wi-Fi stopped");
    }
    return ESP_OK;
}

esp_err_t PMW_Wifi::scan(std::list<wifi_ap_record_t>& ap_record_list)
{
    OSInterfaceLogInfo(TAG, "Scanning Wi-Fi networks");
    wifi_scan_config_t scan_config = {.ssid = nullptr, .bssid = nullptr, .channel = 0, .show_hidden = true};

    ESP_RETURN_ON_ERROR(esp_wifi_scan_start(&scan_config, true), TAG, AT "Failed to perform scan");

    esp_err_t err;
    do
    {
        wifi_ap_record_t ap_info;
        err = esp_wifi_scan_get_ap_record(&ap_info);
        ap_record_list.push_back(ap_info);
    } while (err == ESP_OK);

    if (err != ESP_FAIL)
    {
        OSInterfaceLogError(TAG, "Failed to get AP records");
        return err;
    }

    for (auto ap_info: ap_record_list)
    {
        OSInterfaceLogDebug(TAG, "SSID \t\t%s", reinterpret_cast<const char*>(ap_info.ssid));
        OSInterfaceLogDebug(TAG, "RSSI \t\t%d", ap_info.rssi);
        OSInterfaceLogDebug(TAG, "Channel \t\t%d", ap_info.primary);
        OSInterfaceLogDebug(TAG, "Authmode \t\t%d", ap_info.authmode);
        OSInterfaceLogDebug(TAG, "************************");
    }

    OSInterfaceLogInfo(TAG, "Total APs scanned = %u\n", ap_record_list.size());

    return ESP_OK;
}

esp_err_t PMW_Wifi::connect(const char ssid[32], const char password[64])
{
    wifi_config_t wifi_config{};
    ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &wifi_config));

    strncpy(reinterpret_cast<char*>(wifi_config.sta.ssid), ssid, sizeof(wifi_config.sta.ssid));
    strncpy(reinterpret_cast<char*>(wifi_config.sta.password), password, sizeof(wifi_config.sta.password));

    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &wifi_config), TAG, AT "Failed to set Wi-Fi SSID & PASSWORD");
    printConfig();

    ESP_LOGI(TAG, "Connecting to AP SSID:%s", ssid);

    this->timeout_timestamp = osInterface.osMillis() + CONFIG_PMW_WIFI_CONNECTION_TIMEOUT_MS;
    esp_wifi_disconnect();
    esp_wifi_connect();

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdTRUE, pdFALSE,
                                           pdMS_TO_TICKS(WIFI_CONNECTION_STATUS_TIMEOUT_MS));

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "Connected to AP SSID:%s", ssid);
        return ESP_OK;
    }
    if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGW(TAG, "Failed to connect to SSID:%s", ssid);
        return ESP_FAIL;
    }
    ESP_LOGE(TAG, "Timeout waiting for Wi-Fi connection status");
    return ESP_ERR_TIMEOUT;
}
