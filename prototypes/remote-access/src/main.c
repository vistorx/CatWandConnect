/*
 * Remote access prototype (#267): does Tailscale (microlink) run on a
 * classic ESP32 without PSRAM, next to a web page and a /ws-like stream?
 *
 * Throwaway code, deleted once the results are recorded on #251.
 *
 * - Serial setup on the USB console (115200 baud): Wi-Fi SSID, Wi-Fi
 *   password, Tailscale auth key, device name, optional subnet route.
 *   Saved in NVS on the board; secrets are never echoed or logged.
 * - HTTP on port 80: "/" status page, "/api/status" JSON, "/ws" WebSocket
 *   that streams ~10 small JSON messages per second and echoes what the
 *   page sends (round-trip time through the tunnel).
 * - A status line on the serial log every 10 s.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "driver/uart.h"
#include "esp_event.h"
#include "esp_heap_caps.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "microlink.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "proto";

#define CONSOLE_UART UART_NUM_0
#define NVS_NS "proto"
#define SETUP_WINDOW_MS 3000
#define STATUS_PERIOD_MS 10000
#define WS_PERIOD_MS 100
#define WS_MAX_CLIENTS 4

/* ------------------------------------------------------------------------
 * Settings (NVS namespace "proto")
 * ---------------------------------------------------------------------- */

typedef struct {
    char ssid[33];
    char pass[65];
    char key[128];
    char name[33];
    char route[24];
} settings_t;

static settings_t s_cfg;

static bool settings_load(settings_t *c) {
    memset(c, 0, sizeof(*c));
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) != ESP_OK) return false;
    size_t n;
    n = sizeof(c->ssid);  nvs_get_str(h, "ssid", c->ssid, &n);
    n = sizeof(c->pass);  nvs_get_str(h, "pass", c->pass, &n);
    n = sizeof(c->key);   nvs_get_str(h, "key", c->key, &n);
    n = sizeof(c->name);  nvs_get_str(h, "name", c->name, &n);
    n = sizeof(c->route); nvs_get_str(h, "route", c->route, &n);
    nvs_close(h);
    return c->ssid[0] && c->key[0];
}

static esp_err_t settings_save(const settings_t *c) {
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    nvs_set_str(h, "ssid", c->ssid);
    nvs_set_str(h, "pass", c->pass);
    nvs_set_str(h, "key", c->key);
    nvs_set_str(h, "name", c->name);
    nvs_set_str(h, "route", c->route);
    err = nvs_commit(h);
    nvs_close(h);
    return err;
}

/* Erase our settings and microlink's keys and peer cache. */
static void settings_erase_all(void) {
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_erase_all(h);
        nvs_commit(h);
        nvs_close(h);
    }
    microlink_factory_reset();
}

/* ------------------------------------------------------------------------
 * Serial console input
 * ---------------------------------------------------------------------- */

static void out(const char *s) {
    fputs(s, stdout);
    fflush(stdout);
}

static int read_byte(TickType_t wait) {
    uint8_t b;
    return uart_read_bytes(CONSOLE_UART, &b, 1, wait) == 1 ? b : -1;
}

static void drain_input(void) {
    while (read_byte(pdMS_TO_TICKS(20)) >= 0) {
    }
}

/* Reads one line. masked: echo '*' instead of the characters. */
static void read_line(char *buf, size_t size, bool masked) {
    size_t len = 0;
    buf[0] = 0;
    for (;;) {
        int c = read_byte(portMAX_DELAY);
        if (c < 0) continue;
        if (c == '\r' || c == '\n') {
            if (c == '\r') {
                /* swallow a following \n */
                int next = read_byte(pdMS_TO_TICKS(30));
                (void)next;
            }
            break;
        }
        if (c == 0x08 || c == 0x7f) {
            if (len > 0) {
                len--;
                out("\b \b");
            }
            continue;
        }
        if (c < 0x20 || c > 0x7e) continue; /* printable ASCII only */
        if (len + 1 >= size) continue;
        buf[len++] = (char)c;
        buf[len] = 0;
        char echo[2] = {masked ? '*' : (char)c, 0};
        out(echo);
    }
    out("\r\n");
    /* trim surrounding spaces (pasted keys often carry one) */
    while (len > 0 && isspace((unsigned char)buf[len - 1])) buf[--len] = 0;
    size_t start = 0;
    while (buf[start] && isspace((unsigned char)buf[start])) start++;
    if (start) memmove(buf, buf + start, len - start + 1);
}

static bool route_valid(const char *r) {
    unsigned a, b, c, d, p;
    char extra;
    if (sscanf(r, "%u.%u.%u.%u/%u%c", &a, &b, &c, &d, &p, &extra) != 5) return false;
    return a < 256 && b < 256 && c < 256 && d < 256 && p <= 32;
}

static void print_settings(const settings_t *c) {
    printf("  Wi-Fi network : %s\r\n", c->ssid[0] ? c->ssid : "(none)");
    printf("  Wi-Fi password: %s (%u characters)\r\n", c->pass[0] ? "set" : "empty",
           (unsigned)strlen(c->pass));
    printf("  Auth key      : %s (%u characters)\r\n", c->key[0] ? "set" : "missing",
           (unsigned)strlen(c->key));
    printf("  Device name   : %s\r\n", c->name);
    printf("  Subnet route  : %s\r\n", c->route[0] ? c->route : "(none)");
    fflush(stdout);
}

static void setup_wizard(void) {
    settings_t c;
    settings_load(&c);
    out("\r\n=== Setup ===\r\n");
    out("Wi-Fi network name (SSID): ");
    read_line(c.ssid, sizeof(c.ssid), false);
    out("Wi-Fi password (hidden, Enter for an open network): ");
    read_line(c.pass, sizeof(c.pass), true);
    for (;;) {
        out("Tailscale auth key (hidden, starts with tskey-auth-): ");
        read_line(c.key, sizeof(c.key), true);
        if (strncmp(c.key, "tskey-", 6) == 0) break;
        out("That does not look like an auth key (it starts with tskey-). Try again.\r\n");
    }
    char name[33] = {0};
    printf("Device name on the tailnet [%s]: ", c.name[0] ? c.name : "catwand-proto");
    fflush(stdout);
    read_line(name, sizeof(name), false);
    if (name[0]) {
        strlcpy(c.name, name, sizeof(c.name));
    } else if (!c.name[0]) {
        strlcpy(c.name, "catwand-proto", sizeof(c.name));
    }
    for (;;) {
        out("Subnet route to advertise, e.g. 192.168.1.50/32 (Enter = none): ");
        read_line(c.route, sizeof(c.route), false);
        if (!c.route[0] || route_valid(c.route)) break;
        out("Expected a.b.c.d/nn. Try again.\r\n");
    }
    if (settings_save(&c) == ESP_OK) {
        out("Saved on the board:\r\n");
        print_settings(&c);
    } else {
        out("Could not save the settings.\r\n");
    }
    memset(&c, 0, sizeof(c));
}

static void boot_menu(bool have_cfg) {
    if (!have_cfg) {
        /* Repeat the prompt: the console may be opened after the boot. */
        drain_input();
        do {
            out("\r\nNo settings saved on this board yet. Press Enter to start the setup.\r\n");
        } while (read_byte(pdMS_TO_TICKS(5000)) < 0);
        drain_input();
        setup_wizard();
        return;
    }
    out("\r\nPress any key within 3 s to change the settings...\r\n");
    drain_input();
    if (read_byte(pdMS_TO_TICKS(SETUP_WINDOW_MS)) < 0) return;
    drain_input();
    for (;;) {
        out("\r\n  1  Set up Wi-Fi and the auth key again\r\n"
            "  2  Erase all settings (Wi-Fi, auth key, tailnet identity)\r\n"
            "  3  Continue\r\nChoice: ");
        int ch = read_byte(portMAX_DELAY);
        out("\r\n");
        if (ch == '1') {
            setup_wizard();
            return;
        }
        if (ch == '2') {
            settings_erase_all();
            out("Erased. Restarting...\r\n");
            vTaskDelay(pdMS_TO_TICKS(500));
            esp_restart();
        }
        if (ch == '3') return;
    }
}

/* ------------------------------------------------------------------------
 * State and counters
 * ---------------------------------------------------------------------- */

static microlink_t *s_ml;
static volatile microlink_state_t s_ml_state = ML_STATE_IDLE;
static volatile uint32_t s_ml_reconnects;   /* entries into RECONNECTING */
static volatile uint32_t s_ml_connects;     /* entries into CONNECTED */
static volatile uint32_t s_wifi_drops;      /* Wi-Fi disconnections after a first connection */
static volatile uint32_t s_rebinds;
static volatile bool s_wifi_up;
static volatile bool s_had_ip;
static volatile bool s_need_rebind;
static volatile uint32_t s_connected_since_s;
static volatile uint32_t s_ws_sent;
static volatile uint32_t s_ws_echoes;
static esp_netif_t *s_sta;
static EventGroupHandle_t s_events;
#define EV_GOT_IP BIT0
static int s_ws_fds[WS_MAX_CLIENTS];
static int s_ws_count;
static SemaphoreHandle_t s_ws_lock;

static uint32_t uptime_s(void) { return (uint32_t)(esp_timer_get_time() / 1000000); }
static uint32_t uptime_ms(void) { return (uint32_t)(esp_timer_get_time() / 1000); }

static const char *state_name(microlink_state_t s) {
    static const char *names[] = {"IDLE",       "WIFI_WAIT",    "CONNECTING", "REGISTERING",
                                  "CONNECTED",  "RECONNECTING", "ERROR",      "AUTH_FAILED",
                                  "NEEDS_LOGIN"};
    return (unsigned)s < sizeof(names) / sizeof(names[0]) ? names[s] : "?";
}

static void on_ml_state(microlink_t *ml, microlink_state_t state, void *ctx) {
    (void)ml;
    (void)ctx;
    if (state == ML_STATE_RECONNECTING && s_ml_state != ML_STATE_RECONNECTING) s_ml_reconnects++;
    if (state == ML_STATE_CONNECTED && s_ml_state != ML_STATE_CONNECTED) {
        s_ml_connects++;
        s_connected_since_s = uptime_s();
    }
    s_ml_state = state;
    ESP_LOGI(TAG, "Tailscale state: %s", state_name(state));
    if (state == ML_STATE_AUTH_FAILED) {
        ESP_LOGW(TAG, "Auth key rejected or expired: create a new one and run the setup again");
    }
}

typedef struct {
    uint32_t heap, min_heap, largest, internal;
    uint32_t up;
    int rssi;
    bool wifi;
    char lan_ip[16];
    char ts_ip[16];
    int peers, direct, relay;
} snapshot_t;

static void take_snapshot(snapshot_t *s) {
    memset(s, 0, sizeof(*s));
    s->heap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    s->min_heap = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
    s->largest = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    s->internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    s->up = uptime_s();
    s->wifi = s_wifi_up;
    wifi_ap_record_t ap;
    s->rssi = (s_wifi_up && esp_wifi_sta_get_ap_info(&ap) == ESP_OK) ? ap.rssi : 0;
    strcpy(s->lan_ip, "-");
    esp_netif_ip_info_t ip;
    if (s_sta && esp_netif_get_ip_info(s_sta, &ip) == ESP_OK && ip.ip.addr) {
        snprintf(s->lan_ip, sizeof(s->lan_ip), IPSTR, IP2STR(&ip.ip));
    }
    strcpy(s->ts_ip, "-");
    if (s_ml) {
        uint32_t vip = microlink_get_vpn_ip(s_ml);
        if (vip) microlink_ip_to_str(vip, s->ts_ip);
        s->peers = microlink_get_peer_count(s_ml);
        for (int i = 0; i < s->peers; i++) {
            microlink_peer_info_t p;
            if (microlink_get_peer_info(s_ml, i, &p) == ESP_OK && p.online) {
                if (p.direct_path) s->direct++;
                else s->relay++;
            }
        }
    }
}

/* JSON escape of a short string into dst. */
static void json_str(char *dst, size_t size, const char *src) {
    size_t j = 0;
    for (size_t i = 0; src[i] && j + 7 < size; i++) {
        unsigned char ch = (unsigned char)src[i];
        if (ch == '"' || ch == '\\') {
            dst[j++] = '\\';
            dst[j++] = ch;
        } else if (ch < 0x20) {
            j += snprintf(dst + j, size - j, "\\u%04x", ch);
        } else {
            dst[j++] = ch;
        }
    }
    dst[j] = 0;
}

/* Full status as JSON. Returns the length written. */
static int status_json(char *buf, size_t size) {
    snapshot_t s;
    take_snapshot(&s);
    char name[72];
    json_str(name, sizeof(name), s_cfg.name);
    int n = snprintf(buf, size,
                     "{\"uptime_s\":%lu,\"heap_free\":%lu,\"heap_min_free\":%lu,"
                     "\"heap_largest_block\":%lu,\"heap_internal_free\":%lu,"
                     "\"wifi\":{\"connected\":%s,\"rssi\":%d,\"ip\":\"%s\",\"drops\":%lu},"
                     "\"tailscale\":{\"state\":\"%s\",\"ip\":\"%s\",\"name\":\"%s\","
                     "\"connects\":%lu,\"reconnects\":%lu,\"rebinds\":%lu,"
                     "\"connected_for_s\":%lu,\"route\":\"%s\",\"peers\":[",
                     (unsigned long)s.up, (unsigned long)s.heap, (unsigned long)s.min_heap,
                     (unsigned long)s.largest, (unsigned long)s.internal,
                     s.wifi ? "true" : "false", s.rssi, s.lan_ip, (unsigned long)s_wifi_drops,
                     state_name(s_ml_state), s.ts_ip, name, (unsigned long)s_ml_connects,
                     (unsigned long)s_ml_reconnects, (unsigned long)s_rebinds,
                     (unsigned long)(s_ml_state == ML_STATE_CONNECTED ? s.up - s_connected_since_s
                                                                      : 0),
                     s_cfg.route);
    for (int i = 0; s_ml && i < s.peers && n < (int)size - 160; i++) {
        microlink_peer_info_t p;
        if (microlink_get_peer_info(s_ml, i, &p) != ESP_OK) continue;
        char ip[16], host[100];
        microlink_ip_to_str(p.vpn_ip, ip);
        json_str(host, sizeof(host), p.hostname);
        n += snprintf(buf + n, size - n,
                      "%s{\"name\":\"%s\",\"ip\":\"%s\",\"online\":%s,\"path\":\"%s\"}",
                      i ? "," : "", host, ip, p.online ? "true" : "false",
                      p.direct_path ? "direct" : "relay");
    }
    n += snprintf(buf + n, size - n, "]},\"ws\":{\"clients\":%d,\"sent\":%lu,\"echoes\":%lu}}",
                  s_ws_count, (unsigned long)s_ws_sent, (unsigned long)s_ws_echoes);
    return n;
}

/* ------------------------------------------------------------------------
 * HTTP server
 * ---------------------------------------------------------------------- */

static httpd_handle_t s_httpd;

static const char PAGE[] =
    "<!doctype html><html><head><meta charset=utf-8>"
    "<meta name=viewport content='width=device-width,initial-scale=1'>"
    "<title>Remote access prototype</title><style>"
    "body{font:15px system-ui,sans-serif;margin:1em;max-width:40em}"
    "td{padding:2px 8px}td:first-child{color:#666}h2{font-size:1.1em;margin-top:1.4em}"
    "button{font-size:1em;padding:.4em .8em}</style></head><body>"
    "<h1>Remote access prototype</h1>"
    "<p>Throwaway test firmware (#267). Refreshes every 2 s.</p>"
    "<table id=t></table>"
    "<h2>Peers</h2><table id=p></table>"
    "<h2>WebSocket load test</h2>"
    "<p><button id=b>Start</button> <span id=w>stopped</span></p>"
    "<script>"
    "const $=i=>document.getElementById(i);"
    "function kb(v){return (v/1024).toFixed(1)+' KB'}"
    "function dur(s){const h=Math.floor(s/3600),m=Math.floor(s%3600/60);return h+' h '+m+' min '+(s%60)+' s'}"
    "async function poll(){try{const r=await fetch('/api/status');const s=await r.json();"
    "const t=s.tailscale;const rows=[['Uptime',dur(s.uptime_s)],['Free heap',kb(s.heap_free)],"
    "['Minimum free heap',kb(s.heap_min_free)],['Largest free block',kb(s.heap_largest_block)],"
    "['Tailscale',t.state],['Tailnet IP',t.ip],['Name',t.name],['Connected for',dur(t.connected_for_s)],"
    "['Reconnects',t.reconnects],['Wi-Fi drops',s.wifi.drops],['Wi-Fi RSSI',s.wifi.rssi+' dBm'],"
    "['LAN IP',s.wifi.ip],['Subnet route',t.route||'none'],['WebSocket messages sent',s.ws.sent]];"
    "$('t').innerHTML=rows.map(r=>'<tr><td>'+r[0]+'</td><td>'+r[1]+'</td></tr>').join('');"
    "$('p').innerHTML=t.peers.map(p=>'<tr><td>'+p.name+'</td><td>'+p.ip+'</td><td>'+(p.online?p.path:'offline')+'</td></tr>').join('')||'<tr><td>none</td></tr>';"
    "}catch(e){$('t').innerHTML='<tr><td>unreachable</td></tr>'}}"
    "poll();setInterval(poll,2000);"
    "let ws=null,n=0,t0=0,rtt=[],timer=null;"
    "$('b').onclick=()=>{if(ws){ws.close();return}"
    "ws=new WebSocket('ws://'+location.host+'/ws');n=0;rtt=[];t0=performance.now();"
    "ws.onopen=()=>{$('b').textContent='Stop';timer=setInterval(()=>ws.send('p:'+performance.now()),1000)};"
    "ws.onmessage=e=>{if(e.data.startsWith('p:')){rtt.push(performance.now()-parseFloat(e.data.slice(2)));if(rtt.length>20)rtt.shift()}else n++;"
    "const secs=(performance.now()-t0)/1000;const avg=rtt.length?(rtt.reduce((a,b)=>a+b)/rtt.length).toFixed(0)+' ms':'-';"
    "$('w').textContent=n+' messages, '+(n/secs).toFixed(1)+'/s, round trip '+avg};"
    "ws.onclose=()=>{clearInterval(timer);ws=null;$('b').textContent='Start';$('w').textContent+=' (closed)'}};"
    "</script></body></html>";

static esp_err_t page_get(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, PAGE, sizeof(PAGE) - 1);
}

static esp_err_t status_get(httpd_req_t *req) {
    char *buf = malloc(2048);
    if (!buf) return httpd_resp_send_500(req);
    int n = status_json(buf, 2048);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    esp_err_t err = httpd_resp_send(req, buf, n);
    free(buf);
    return err;
}

static void ws_add(int fd) {
    xSemaphoreTake(s_ws_lock, portMAX_DELAY);
    bool found = false;
    for (int i = 0; i < s_ws_count; i++) found |= s_ws_fds[i] == fd;
    if (!found && s_ws_count < WS_MAX_CLIENTS) s_ws_fds[s_ws_count++] = fd;
    xSemaphoreGive(s_ws_lock);
}

static void ws_remove(int fd) {
    xSemaphoreTake(s_ws_lock, portMAX_DELAY);
    for (int i = 0; i < s_ws_count; i++) {
        if (s_ws_fds[i] == fd) {
            s_ws_fds[i] = s_ws_fds[--s_ws_count];
            break;
        }
    }
    xSemaphoreGive(s_ws_lock);
}

static esp_err_t ws_handler(httpd_req_t *req) {
    if (req->method == HTTP_GET) {
        ws_add(httpd_req_to_sockfd(req));
        ESP_LOGI(TAG, "WebSocket client connected");
        return ESP_OK;
    }
    httpd_ws_frame_t frame = {0};
    esp_err_t err = httpd_ws_recv_frame(req, &frame, 0);
    if (err != ESP_OK) return err;
    if (frame.type == HTTPD_WS_TYPE_CLOSE) {
        ws_remove(httpd_req_to_sockfd(req));
        return ESP_OK;
    }
    if (frame.len == 0 || frame.len > 64 || frame.type != HTTPD_WS_TYPE_TEXT) return ESP_OK;
    uint8_t buf[65];
    frame.payload = buf;
    err = httpd_ws_recv_frame(req, &frame, frame.len);
    if (err != ESP_OK) return err;
    s_ws_echoes++;
    /* echo "p:<time>" back for the page's round-trip measurement */
    return httpd_ws_send_frame(req, &frame);
}

static void on_close(httpd_handle_t hd, int fd) {
    (void)hd;
    ws_remove(fd);
    close(fd);
}

/* ~10 small JSON messages per second to every WebSocket client, like the
 * robot's /ws state stream. */
static void ws_stream_task(void *arg) {
    (void)arg;
    uint32_t seq = 0;
    char msg[160];
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(WS_PERIOD_MS));
        int fds[WS_MAX_CLIENTS];
        xSemaphoreTake(s_ws_lock, portMAX_DELAY);
        int count = s_ws_count;
        memcpy(fds, s_ws_fds, sizeof(fds));
        xSemaphoreGive(s_ws_lock);
        if (!count) continue;
        seq++;
        int n = snprintf(msg, sizeof(msg),
                         "{\"type\":\"state\",\"seq\":%lu,\"t\":%lu,\"h\":%u,\"v\":%u,"
                         "\"activity\":\"manual\",\"heap\":%lu}",
                         (unsigned long)seq, (unsigned long)uptime_ms(),
                         (unsigned)(seq * 7 % 180), (unsigned)(seq * 3 % 180),
                         (unsigned long)heap_caps_get_free_size(MALLOC_CAP_8BIT));
        httpd_ws_frame_t frame = {
            .type = HTTPD_WS_TYPE_TEXT, .payload = (uint8_t *)msg, .len = (size_t)n};
        for (int i = 0; i < count; i++) {
            if (httpd_ws_get_fd_info(s_httpd, fds[i]) != HTTPD_WS_CLIENT_WEBSOCKET) {
                ws_remove(fds[i]);
                continue;
            }
            if (httpd_ws_send_data(s_httpd, fds[i], &frame) == ESP_OK) s_ws_sent++;
            else ws_remove(fds[i]);
        }
    }
}

static void http_start(void) {
    s_ws_lock = xSemaphoreCreateMutex();
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.server_port = 80;
    cfg.max_open_sockets = 6;
    cfg.lru_purge_enable = true;
    cfg.close_fn = on_close;
    cfg.stack_size = 6144;
    if (httpd_start(&s_httpd, &cfg) != ESP_OK) {
        ESP_LOGE(TAG, "HTTP server failed to start");
        return;
    }
    static const httpd_uri_t page = {.uri = "/", .method = HTTP_GET, .handler = page_get};
    static const httpd_uri_t status = {
        .uri = "/api/status", .method = HTTP_GET, .handler = status_get};
    static const httpd_uri_t ws = {
        .uri = "/ws", .method = HTTP_GET, .handler = ws_handler, .is_websocket = true};
    httpd_register_uri_handler(s_httpd, &page);
    httpd_register_uri_handler(s_httpd, &status);
    httpd_register_uri_handler(s_httpd, &ws);
    xTaskCreate(ws_stream_task, "ws_stream", 3072, NULL, 4, NULL);
    ESP_LOGI(TAG, "HTTP server on port 80 (/, /api/status, /ws)");
}

/* ------------------------------------------------------------------------
 * Wi-Fi
 * ---------------------------------------------------------------------- */

static void wifi_event(void *arg, esp_event_base_t base, int32_t id, void *data) {
    (void)arg;
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t *d = data;
        if (s_wifi_up) {
            s_wifi_drops++;
            ESP_LOGW(TAG, "Wi-Fi lost (reason %d), reconnecting", d->reason);
        }
        s_wifi_up = false;
        xEventGroupClearBits(s_events, EV_GOT_IP);
        /* the main loop retries; no busy loop here */
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *e = data;
        ESP_LOGI(TAG, "Wi-Fi connected, LAN IP " IPSTR, IP2STR(&e->ip_info.ip));
        if (s_had_ip) s_need_rebind = true;
        s_had_ip = true;
        s_wifi_up = true;
        xEventGroupSetBits(s_events, EV_GOT_IP);
    }
}

static void wifi_start(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    s_sta = esp_netif_create_default_wifi_sta();
    esp_netif_set_hostname(s_sta, s_cfg.name);
    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event, NULL));
    wifi_config_t wc = {0};
    strlcpy((char *)wc.sta.ssid, s_cfg.ssid, sizeof(wc.sta.ssid));
    strlcpy((char *)wc.sta.password, s_cfg.pass, sizeof(wc.sta.password));
    wc.sta.threshold.authmode = s_cfg.pass[0] ? WIFI_AUTH_WPA_PSK : WIFI_AUTH_OPEN;
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wc));
    ESP_ERROR_CHECK(esp_wifi_start());
    /* Same as the robot firmware (#193): no modem sleep, lower latency */
    esp_wifi_set_ps(WIFI_PS_NONE);
    memset(&wc, 0, sizeof(wc));
    ESP_LOGI(TAG, "Connecting to Wi-Fi \"%s\"...", s_cfg.ssid);
}

/* ------------------------------------------------------------------------
 * Console commands while running
 * ---------------------------------------------------------------------- */

static void log_status(void) {
    snapshot_t s;
    take_snapshot(&s);
    ESP_LOGI(TAG,
             "STATUS up=%lus heap=%lu min=%lu block=%lu ts=%s ip=%s peers=%d direct=%d relay=%d "
             "reconnects=%lu wifi_drops=%lu rebinds=%lu rssi=%d ws_sent=%lu",
             (unsigned long)s.up, (unsigned long)s.heap, (unsigned long)s.min_heap,
             (unsigned long)s.largest, state_name(s_ml_state), s.ts_ip, s.peers, s.direct,
             s.relay, (unsigned long)s_ml_reconnects, (unsigned long)s_wifi_drops,
             (unsigned long)s_rebinds, s.rssi, (unsigned long)s_ws_sent);
}

static void console_task(void *arg) {
    (void)arg;
    for (;;) {
        int c = read_byte(portMAX_DELAY);
        if (c == 's') {
            log_status();
        } else if (c == 'r') {
            out("Restarting...\r\n");
            vTaskDelay(pdMS_TO_TICKS(200));
            esp_restart();
        } else if (c == 'e') {
            out("Erase all settings and the tailnet identity? Type Y to confirm: ");
            int y = read_byte(pdMS_TO_TICKS(10000));
            out("\r\n");
            if (y == 'Y') {
                if (s_ml) microlink_stop(s_ml);
                settings_erase_all();
                out("Erased. Restarting...\r\n");
                vTaskDelay(pdMS_TO_TICKS(300));
                esp_restart();
            }
            out("Cancelled.\r\n");
        } else if (c == 'h' || c == '?') {
            out("Commands: s = status now, r = restart (setup at boot), e = erase all settings\r\n");
        }
    }
}

/* ------------------------------------------------------------------------
 * Main
 * ---------------------------------------------------------------------- */

void app_main(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_ERROR_CHECK(uart_driver_install(CONSOLE_UART, 256, 0, 0, NULL, 0));

    out("\r\n\r\nCatWandConnect remote access prototype (#267), Tailscale via microlink\r\n");
    printf("Free heap at boot: %lu bytes, largest block %lu\r\n",
           (unsigned long)heap_caps_get_free_size(MALLOC_CAP_8BIT),
           (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    fflush(stdout);

    boot_menu(settings_load(&s_cfg));
    if (!settings_load(&s_cfg)) {
        out("Settings incomplete, restarting setup.\r\n");
        vTaskDelay(pdMS_TO_TICKS(500));
        esp_restart();
    }
    if (!s_cfg.name[0]) strlcpy(s_cfg.name, "catwand-proto", sizeof(s_cfg.name));
    print_settings(&s_cfg);
    out("Commands while running: s = status, r = restart, e = erase, h = help\r\n");

    s_events = xEventGroupCreate();
    wifi_start();
    http_start();
    xTaskCreate(console_task, "console", 3072, NULL, 3, NULL);

    uint32_t last_retry = 0;
    while (!(xEventGroupWaitBits(s_events, EV_GOT_IP, pdFALSE, pdTRUE, pdMS_TO_TICKS(5000)) &
             EV_GOT_IP)) {
        ESP_LOGW(TAG, "Still waiting for Wi-Fi \"%s\"", s_cfg.ssid);
        esp_wifi_connect();
    }

    microlink_config_t mc = {
        .auth_key = s_cfg.key,
        .device_name = s_cfg.name,
        .max_peers = CONFIG_ML_MAX_PEERS,
        .advertise_routes = s_cfg.route[0] ? s_cfg.route : NULL,
    };
    s_ml = microlink_init(&mc);
    if (!s_ml) {
        ESP_LOGE(TAG, "microlink_init failed (out of memory?)");
    } else {
        microlink_set_state_callback(s_ml, on_ml_state, NULL);
        err = microlink_start(s_ml);
        if (err != ESP_OK) ESP_LOGE(TAG, "microlink_start failed: %d", err);
    }
    log_status();

    uint32_t last_status = uptime_ms();
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(500));
        uint32_t now = uptime_ms();
        if (!s_wifi_up && now - last_retry > 5000) {
            last_retry = now;
            esp_wifi_connect();
        }
        if (s_need_rebind && s_wifi_up && s_ml) {
            s_need_rebind = false;
            s_rebinds++;
            ESP_LOGI(TAG, "Wi-Fi back: rebinding Tailscale sockets");
            microlink_rebind(s_ml);
        }
        if (now - last_status >= STATUS_PERIOD_MS) {
            last_status = now;
            log_status();
        }
    }
}
