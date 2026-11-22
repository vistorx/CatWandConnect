#pragma once

// microlink's build options. Its Kconfig only runs in an ESP-IDF CMake build;
// Arduino-ESP32 ships ESP-IDF precompiled, so the values are set here and
// force-included in every microlink source (library.json). Sized for a
// classic ESP32 without PSRAM, as measured by the remote access prototype
// (#267): a small tailnet of about 20 devices.

#define CONFIG_ML_MAX_PEERS 8
#define CONFIG_ML_NVS_MAX_PEERS 16
#define CONFIG_ML_H2_BUFFER_SIZE_KB 64
#define CONFIG_ML_JSON_BUFFER_SIZE_KB 64
// Home relay before the control server assigns one (18 = Paris).
#define CONFIG_ML_DERP_REGION 18
#define CONFIG_ML_DERP_VERIFY_CERT 1
#define CONFIG_ML_PRIORITY_PEER_IP ""
#define CONFIG_ML_PACKET_FILTER_ENFORCE 1
#define CONFIG_ML_FILTER_MAX_ENTRIES 4096
// Subnet routes, for the camera through remote access (#271). None is
// advertised yet.
#define CONFIG_ML_SUBNET_ROUTER 1
#define CONFIG_ML_ADVERTISE_ROUTES ""
#define CONFIG_ML_UDP_RX_TASK_PRIO 7
#define CONFIG_ML_STOP_TIMEOUT_MS 30000
#define CONFIG_ML_LOG_LEVEL 3
#define CONFIG_ML_LOG_LEVEL_DEFAULT 1
#define CONFIG_ML_WIFI_SSID ""
#define CONFIG_ML_WIFI_PASSWORD ""
#define CONFIG_ML_TAILSCALE_AUTH_KEY ""
#define CONFIG_ML_DEVICE_NAME ""
#define CONFIG_ML_CTRL_HOST "controlplane.tailscale.com"
#define CONFIG_ML_CTRL_NOISE_PUBKEY_HEX ""
// Left out on purpose: ML_OTA_ROLLBACK (the robot's own update flow decides
// whether a firmware is kept), ML_ENABLE_CONFIG_HTTPD (microlink's own web
// page), ML_ENABLE_CELLULAR, ML_ZERO_COPY_WG, ML_ROUTER_IPV6, ML_CTRL_TLS.
