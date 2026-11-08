#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <esp_wifi.h>
#include <esp_netif.h>

extern "C" {
  #if __has_include("dhcpserver/dhcpserver.h")
  #include "dhcpserver/dhcpserver.h"
  #elif __has_include("dhcpserver.h")
  #include "dhcpserver.h"
  #endif
}

#ifndef ESP_NETIF_REQUESTED_IP_ADDRESS
#define ESP_NETIF_REQUESTED_IP_ADDRESS ((esp_netif_dhcp_option_id_t)50)
#endif
#ifndef ESP_NETIF_ROUTER_SOLICITATION_ADDRESS
#define ESP_NETIF_ROUTER_SOLICITATION_ADDRESS ((esp_netif_dhcp_option_id_t)32)
#endif
#ifndef ESP_NETIF_DOMAIN_NAME_SERVER
#define ESP_NETIF_DOMAIN_NAME_SERVER ((esp_netif_dhcp_option_id_t)6)
#endif
#ifndef OFFER_ROUTER
#define OFFER_ROUTER 0x01
#endif
#ifndef OFFER_DNS
#define OFFER_DNS 0x02
#endif

#if __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>
#endif

// Nhúng thư viện NimBLE-Arduino (Siêu nhẹ, tiết kiệm ~350KB Flash so với Bluedroid)
#include <NimBLEDevice.h>
#if __has_include(<NimBLEConnInfo.h>)
#include <NimBLEConnInfo.h>
#define NIMBLE_V2 1
#endif

// Nhúng thư viện định tuyến NAT và DNS của LwIP
#if __has_include("lwip/lwip_napt.h")
#include "lwip/lwip_napt.h"
#elif __has_include("lwip/napt.h")
#include "lwip/napt.h"
#endif
extern "C" {
  void ip_napt_enable(uint32_t addr, int enable);
}

#include <lwip/ip_addr.h>
#include <lwip/ip4_addr.h>
#include <lwip/dns.h>
#include <lwip/tcpip.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <WiFiUdp.h>
#include <Update.h>
#include <ESPmDNS.h>

// Nhúng thư viện nâng cấp Firmware không dây qua GitHub HTTPS (Native ESP-IDF OTA siêu nhẹ)
#include <esp_http_client.h>
#include <esp_crt_bundle.h>
#include <esp_https_ota.h>
#include <time.h>
#include <sys/time.h>

// =========================================================================
//                  QUẢN LÝ THÔNG SỐ CẤU HÌNH (PREFERENCES NVS)
// =========================================================================
Preferences prefs;

// Phiên bản Firmware & Quản trị GitHub OTA
const String FIRMWARE_VERSION = "1.4.0";
String ota_version_url;
bool pending_ota = false;
String pending_ota_bin_url = "";

// Cờ chuyển đổi trạng thái Wi-Fi nguồn bất đồng bộ (tránh cắt đứt socket HTTP phản hồi)
bool pending_sta_mode_change = false;
bool pending_sta_mode_val = false;
unsigned long sta_mode_change_time = 0;

enum OtaCheckState {
  OTA_CHECK_IDLE = 0,
  OTA_CHECK_REQUESTED,
  OTA_CHECK_RUNNING,
  OTA_CHECK_SUCCESS,
  OTA_CHECK_FAILED
};

volatile OtaCheckState ota_check_state = OTA_CHECK_IDLE;
String ota_new_ver = "";
String ota_bin_url = "";
String ota_changelog = "";
String ota_err_msg = "";

bool checkGithubOta(String &newVer, String &binUrl, String &changelog, String &errMsg);
void performGithubOta(const String &binUrl);
bool parse_mac(const String &macStr, uint8_t *macArr);
void configureSoftApDhcp(bool randomizePool = false);
void triggerStaConnect();
void stopStaConnecting();
String serializeLogsJson();
String performWifiScanJson();

// Quản lý mẫu cấu hình mạng phát (SoftAP Presets)
#define MAX_PROFILES 10
struct ApProfile {
  String id;
  String name;
  String ssid;
  String pass;
  uint8_t channel;
  String bssid;
  String ip;
  String subnet;
  String dhcp_start;
  String dhcp_end;
};
ApProfile profiles[MAX_PROFILES];
int profile_count = 0;
uint8_t current_effective_channel = 6;
uint8_t findOptimalChannel(String *detailLog = NULL);
String serializeProfiles();
void loadProfilesFromNvs();
void saveProfilesToNvs();
bool addOrUpdateProfile(const ApProfile &newP);
bool deleteProfile(const String &id);
bool applyProfile(const String &id);

// Điều khiển đèn LED xanh báo trạng thái Wi-Fi nguồn (Onboard GPIO 2)
#define WIFI_STA_LED_PIN 2

// Wi-Fi nguồn (STA)
bool sta_enabled = false; // Mặc định lần đầu: TẮT Wi-Fi nguồn (Mạng nội bộ Offline dùng 5G)
bool sta_connected_state = false; // Chốt trạng thái kết nối STA để chống spam log
String sta_ssid;
String sta_password;
String cached_scan_json = "[]";
unsigned long last_scan_time = 0;

// Bộ máy trạng thái kết nối Wi-Fi nguồn (Non-blocking STA Reconnect State Machine)
enum StaConnState {
  STA_STATE_IDLE,       // Nghỉ ngơi, không quét kênh ngầm, radio 100% dành trọn cho SoftAP
  STA_STATE_CONNECTING, // Đang trong cửa sổ kết nối thử nghiệm ngắn (tối đa 4 giây)
  STA_STATE_CONNECTED   // Đã kết nối thành công vào Wi-Fi nguồn
};
StaConnState sta_conn_state = STA_STATE_IDLE;
unsigned long sta_connect_start_time = 0;
unsigned long sta_next_retry_time = 0;
unsigned long last_ap_client_connected_time = 0;

// Hàm cập nhật trạng thái đèn LED chỉ báo Wi-Fi nguồn (GPIO 2)
void updateWifiLed() {
  if (!sta_enabled) {
    digitalWrite(WIFI_STA_LED_PIN, LOW); // Tắt hẳn khi Wi-Fi nguồn TẮT
  } else {
    if (WiFi.status() == WL_CONNECTED) {
      digitalWrite(WIFI_STA_LED_PIN, HIGH); // Sáng liên tục khi ĐÃ KẾT NỐI
    } else {
      // Nhấp nháy 1Hz (500ms sáng / 500ms tắt) khi đang bật nhưng chưa có kết nối
      digitalWrite(WIFI_STA_LED_PIN, (millis() / 500) % 2 == 0 ? HIGH : LOW);
    }
  }
}

// Wi-Fi phát ra (AP)
String ap_ssid;
String ap_password;
uint8_t ap_channel;
String ap_bssid_str;
String ap_ip_str;
String ap_subnet_str;
String dhcp_start_str;
String dhcp_end_str;

// Chế độ công suất phát cực đại (Max TX Power Boost: mặc định là KHÔNG BẬT - 15 dBm)
bool tx_power_boost = false;

// Quản lý bảo mật Bluetooth LE
bool ble_enabled = true;
uint32_t ble_pin = 123456; // Mã PIN mặc định: 123456

// Web Server & Captive DNS Server
WebServer server(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;

// Trạng thái vận hành
bool napt_enabled = false;
bool dns_captive_running = false;
void startDnsProxy();
void stopDnsProxy();
void handleDnsProxy();
bool should_reboot = false;
unsigned long reboot_time = 0;
unsigned long last_sta_check = 0;
volatile bool dhcp_reconfigure_needed = false;

// =========================================================================
//      BLUETOOTH LOW ENERGY (BLE) - NORDIC UART SERVICE (CONSOLE & CONFIG)
// =========================================================================
#define BLE_DEVICE_NAME        "ESP32-Gateway-BLE"
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

NimBLEServer *pBleServer = NULL;
NimBLECharacteristic *pBleTxChar = NULL;
bool bleConnected = false;
bool oldBleConnected = false;
volatile bool ble_cmd_pending = false;
String pending_ble_cmd = "";

// =========================================================================
//              HỆ THỐNG GHI NHẬT KÝ KHÔNG DÂY (LOG BUFFER)
// =========================================================================
#define LOG_BUFFER_SIZE 30
String logBuffer[LOG_BUFFER_SIZE];
int logHead = 0;
int logCount = 0;

void addLogToBuffer(const String &line) {
  logBuffer[logHead] = line;
  logHead = (logHead + 1) % LOG_BUFFER_SIZE;
  if (logCount < LOG_BUFFER_SIZE) {
    logCount++;
  }
}

// Bắn thông báo log qua BLE Notify trực tiếp, NON-BLOCKING (không dùng delay để tránh nghẽn Wi-Fi & BLE Stack)
void sendBleNotification(const String &msg) {
  if (!bleConnected || pBleTxChar == NULL) return;

  String out = msg + "\r\n";
  if (out.length() > 64) {
    out = out.substring(0, 60) + "..\r\n";
  }
  pBleTxChar->setValue((const uint8_t*)out.c_str(), out.length());
  pBleTxChar->notify();
}

void logMessage(const String &msg) {
  Serial.println(msg);
  addLogToBuffer(msg);
  sendBleNotification(msg);
}

void logPrintf(const char *format, ...) {
  char buf[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);

  String s = String(buf);
  while (s.endsWith("\r") || s.endsWith("\n")) {
    s.remove(s.length() - 1);
  }
  logMessage(s);
}

// Tạm dừng kết nối STA để giải phóng radio 100% cho SoftAP
void stopStaConnecting() {
  WiFi.disconnect(false);
  esp_wifi_disconnect();
  esp_wifi_set_channel(current_effective_channel, WIFI_SECOND_CHAN_NONE);
  sta_conn_state = STA_STATE_IDLE;
  sta_next_retry_time = millis() + 30000; // Nghỉ 30s trước khi thử lại
  logPrintf("[STA] Tam dung quet Wi-Fi nguon. Khoa kenh SoftAP: %d. Thu lai sau 30s.", current_effective_channel);
}

// Bắt đầu cửa sổ kết nối thử nghiệm vào Wi-Fi nguồn (tối đa 4s)
void triggerStaConnect() {
  if (!sta_enabled || sta_ssid.length() == 0) return;
  if (WiFi.status() == WL_CONNECTED) {
    sta_conn_state = STA_STATE_CONNECTED;
    return;
  }
  logPrintf("[STA] Thu ket noi vao Wi-Fi nguon: '%s' (Cua so toi da 4s)...", sta_ssid.c_str());
  WiFi.mode(WIFI_AP_STA);
  WiFi.setAutoReconnect(false);
  WiFi.begin(sta_ssid.c_str(), (sta_password.length() > 0 ? sta_password.c_str() : NULL));
  sta_conn_state = STA_STATE_CONNECTING;
  sta_connect_start_time = millis();
}

// Kiểm tra chuỗi có đúng 6 chữ số hợp lệ hay không
bool isValidPin(const String &s) {
  if (s.length() != 6) return false;
  for (int i = 0; i < 6; i++) {
    if (!isDigit(s[i])) return false;
  }
  return true;
}

// Cập nhật mã PIN bảo mật phần cứng cho NimBLE
void updateBleSecurityPin(uint32_t pin) {
  ble_pin = pin;
  NimBLEDevice::setSecurityPasskey(ble_pin);
}

// Thiết lập toàn bộ tham số bảo mật phần cứng NimBLE
void configureBleSecurity() {
  NimBLEDevice::setSecurityAuth(true, true, true); // Bonding, MITM, Secure Connections
  NimBLEDevice::setSecurityPasskey(ble_pin);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);
}

// Điều chỉnh công suất phát sóng Wi-Fi (Max TX Power Boost)
void applyTxPower(bool boost) {
  // Đơn vị 0.25 dBm: 80 = 20 dBm (Max Boost ~100mW), 60 = 15 dBm (Standard)
  int8_t pwr = boost ? 80 : 60;
  esp_err_t err = esp_wifi_set_max_tx_power(pwr);
  if (err == ESP_OK) {
    logPrintf("[POWER] Cong suat phat Wi-Fi: %s (%s dBm)", 
              boost ? "CUC DAI (Max Boost)" : "TIEU CHUAN", 
              boost ? "20" : "15");
  } else {
    logPrintf("[POWER] Canh bao: Khong the set cong suat (Ma loi: %d)", err);
  }
}

// Bật hoặc tắt sóng Bluetooth LE
void setBleRadio(bool enable) {
  ble_enabled = enable;
  prefs.begin("gateway", false);
  prefs.putBool("ble_en", ble_enabled);
  prefs.end();

  if (ble_enabled) {
    NimBLEDevice::startAdvertising();
    logMessage("[BLE] Da BAT song Bluetooth LE (ESP32-Gateway-BLE)");
  } else {
    if (bleConnected && pBleServer != NULL) {
      pBleServer->disconnect(0);
    }
    NimBLEDevice::getAdvertising()->stop();
    bleConnected = false;
    logMessage("[BLE] Da TAT hoan toan song Bluetooth LE (Khong the do thay)");
  }
}

// Phản hồi kết quả lệnh trực tiếp ra USB Serial Monitor và Bluetooth LE Console
void sendResponse(const String &msg) {
  Serial.println(msg);

  // Đồng bộ toàn bộ phản hồi vào Web Portal Log Buffer (tách theo từng dòng)
  int sIdx = 0;
  int mLen = msg.length();
  while (sIdx < mLen) {
    int nl = msg.indexOf('\n', sIdx);
    String l = (nl >= 0) ? msg.substring(sIdx, nl) : msg.substring(sIdx);
    l.trim();
    if (l.length() > 0) {
      addLogToBuffer(l);
    }
    if (nl >= 0) sIdx = nl + 1;
    else break;
  }

  if (!bleConnected || pBleTxChar == NULL) return;

  String text = msg;
  text.replace("\r\n", "\n");

  int startIdx = 0;
  int textLen = text.length();

  while (startIdx < textLen) {
    int nextNewline = text.indexOf('\n', startIdx);
    String line;
    if (nextNewline >= 0) {
      line = text.substring(startIdx, nextNewline) + "\r\n";
      startIdx = nextNewline + 1;
    } else {
      line = text.substring(startIdx) + "\r\n";
      startIdx = textLen;
    }

    size_t lineLen = line.length();
    for (size_t i = 0; i < lineLen; i += 20) {
      size_t chunkLen = lineLen - i;
      if (chunkLen > 20) chunkLen = 20;

      pBleTxChar->setValue((const uint8_t*)(line.c_str() + i), chunkLen);
      pBleTxChar->notify();
      delay(15);
    }
    delay(5);
  }
}

// Xử lý toàn bộ lệnh cấu hình hệ thống từ Bluetooth LE và USB Serial Console
void handleBleCommand(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;
  logPrintf("[CONSOLE-RX] Lenh nhan duoc: '%s'", cmd.c_str());

  // Nếu người dùng gõ AUTH theo thói quen
  if (cmd.startsWith("AUTH:") || cmd.startsWith("auth:")) {
    sendResponse("[OK] Thiet bi da duoc bao ve bang ma PIN phan cung (Khong can lenh AUTH)!");
    return;
  }

  // 1. HELP / ?: Trợ giúp toàn bộ danh sách lệnh
  if (cmd.equalsIgnoreCase("HELP") || cmd.equalsIgnoreCase("help") || cmd == "?") {
    String h = "=== DANH SACH LENH TERMINAL (BLE & SERIAL) ===\n";
    h += "-- [1. Wi-Fi Nguon (STA)] --\n";
    h += "• SCAN              -> Quet mang Wi-Fi 2.4GHz xung quanh\n";
    h += "• WIFI:ssid,pass    -> Ket noi vao Wi-Fi nguon (pass de trong neu mo)\n";
    h += "• STA:ON | OFF      -> Bat/Tat Wi-Fi nguon (TAT de dien thoai dung 5G)\n";
    h += "\n-- [2. Mang Phat (SoftAP & DHCP)] --\n";
    h += "• AP:ssid,pass      -> Doi Ten & Mat khau Wi-Fi phat\n";
    h += "• APCH:kenh         -> Doi kenh phat (1 - 11 hoac 0: AUTO)\n";
    h += "• AUTOCH            -> Tu dong quet & chon kenh toi uu it nhieu nhat\n";
    h += "• PROFILES          -> Xem danh sach mau cau hinh mang phat (Presets)\n";
    h += "• PROFILE:APPLY:id  -> Ap dung mau cau hinh (VD: PROFILE:APPLY:p1)\n";
    h += "• PROFILE:UPDATE:id -> Cap nhat Preset voi thong so mang phat hien tai\n";
    h += "• PROFILE:SAVE:ten  -> Luu cau hinh hien tai thanh mau moi\n";
    h += "• PROFILE:CLONE     -> Sao chep Wi-Fi nguon (STA) thanh Preset AP\n";
    h += "• PROFILE:DEL:id    -> Xoa mau cau hinh khoi danh sach\n";
    h += "• APMAC:mac         -> Doi dia chi MAC phan cung (VD: APMAC:84:F1:47:4F:27:A0)\n";
    h += "• APIP:ip,subnet    -> Doi Gateway IP & Subnet (VD: APIP:10.51.31.254,255.255.255.0)\n";
    h += "• DHCP:start,end    -> Doi dai cap IP ngau nhien (VD: DHCP:10.51.31.10,10.51.31.200)\n";
    h += "• BOOST:ON | OFF    -> Bat/tat phat song cuc dai 20 dBm (~100mW)\n";
    h += "\n-- [3. Bao Mat & Bluetooth] --\n";
    h += "• SETPIN:pin_6_so   -> Doi ma PIN ghep doi Bluetooth (VD: SETPIN:123456)\n";
    h += "• BLERADIO:OFF      -> Tat song Bluetooth LE (tang hinh)\n";
    h += "\n-- [4. Cloud OTA GitHub] --\n";
    h += "• OTAURL:url        -> Cai dat duong dan version.json\n";
    h += "• OTA               -> Kiem tra ban cap nhat moi tu GitHub\n";
    h += "• OTA:RUN           -> Tai va ghi Flash ban moi tu GitHub\n";
    h += "\n-- [5. He Thong & Truy Cap Truc Tiep] --\n";
    h += "• STATUS            -> Xem toan bo trang thai he thong & IP\n";
    h += "• CLEARLOGS         -> Xoa bo nho dem nhat ky (log buffer)\n";
    h += "• REBOOT            -> Khoi dong lai ESP32";
    sendResponse(h);
    return;
  }

  // 2. STATUS: Xem toàn bộ thông số hoạt động
  if (cmd.equalsIgnoreCase("STATUS") || cmd.equalsIgnoreCase("status")) {
    char pinStr[10];
    sprintf(pinStr, "%06u", ble_pin);
    String st = "=== TRANG THAI ESP32 GATEWAY ===\n";
    st += "Firmware: v" + FIRMWARE_VERSION + "\n";
    if (!sta_enabled) {
      st += "STA Status: DA TAT (Che do Mang noi bo - Dien thoai dung 5G)\n";
    } else {
      st += "STA SSID: " + (sta_ssid.length() > 0 ? sta_ssid : "(Chua cau hinh)") + "\n";
      st += "STA Pass: " + (sta_password.length() > 0 ? sta_password : "(Khong co mat khau)") + "\n";
      st += "STA Status: " + String(WiFi.status() == WL_CONNECTED ? "DA KET NOI" : "CHUA KET NOI") + "\n";
      if (WiFi.status() == WL_CONNECTED) {
        st += "STA IP: " + WiFi.localIP().toString() + "\n";
        st += "STA Gateway: " + WiFi.gatewayIP().toString() + "\n";
        st += "STA RSSI: " + String(WiFi.RSSI()) + " dBm\n";
      }
    }
    st += "-------------------------------\n";
    st += "AP SSID: " + ap_ssid + "\n";
    st += "AP Pass: " + (ap_password.length() > 0 ? ap_password : "(Khong co mat khau)") + "\n";
    st += "AP Gateway: " + ap_ip_str + "\n";
    st += "AP Subnet: " + ap_subnet_str + "\n";
    st += "AP MAC: " + ap_bssid_str + "\n";
    st += "AP Channel: " + (ap_channel == 0 ? ("Auto (Dang phat: " + String(current_effective_channel) + ")") : String(ap_channel)) + "\n";
    st += "AP Presets: " + String(profile_count) + " mau da luu\n";
    st += "AP Clients: " + String(WiFi.softAPgetStationNum()) + "\n";
    st += "DHCP Pool: " + dhcp_start_str + " -> " + dhcp_end_str + " (Random)\n";
    st += "TX Power: " + String(tx_power_boost ? "20 dBm (Max Boost)" : "15 dBm (Standard)") + "\n";
    st += "BLE PIN: " + String(pinStr) + " | Radio: " + (ble_enabled ? "BAT" : "TAT") + "\n";
    st += "OTA URL: " + ota_version_url + "\n";
    st += "NAT Routing: " + String(napt_enabled ? "ON" : "OFF") + "\n";
    st += "Truy cap SoftAP: http://" + ap_ip_str + "/\n";
    if (WiFi.status() == WL_CONNECTED) {
      st += "Truy cap LAN: http://" + WiFi.localIP().toString() + "/\n";
    }
    st += "Truy cap mDNS: http://esp32.local/\n";
    st += "RAM Free: " + String(ESP.getFreeHeap() / 1024) + " KB\n";
    st += "Flash Chip: " + String(ESP.getFlashChipSize() / (1024 * 1024)) + " MB\n";
    st += "Uptime: " + String(millis() / 1000) + "s";
    sendResponse(st);
    return;
  }

  // 3. SCAN: Quét sóng Wi-Fi xung quanh
  if (cmd.equalsIgnoreCase("SCAN") || cmd.equalsIgnoreCase("scan")) {
    sendResponse("[SCAN] Dang quet cac mang Wi-Fi 2.4GHz xung quanh...");
    performWifiScanJson();
    sendResponse("[SCAN] Quet xong! Da cap nhat danh sach Wi-Fi.");
    return;
  }

  // 4. WIFI:ssid,pass: Cấu hình Wi-Fi nguồn
  if (cmd.startsWith("WIFI:") || cmd.startsWith("wifi:")) {
    String payload = cmd.substring(5);
    payload.trim();
    int comma = payload.indexOf(',');
    String newSsid = (comma >= 0) ? payload.substring(0, comma) : payload;
    String newPass = (comma >= 0) ? payload.substring(comma + 1) : "";
    newSsid.trim(); newPass.trim();

    if (newSsid.equalsIgnoreCase("CLEAR") || newSsid.length() == 0) {
      sta_ssid = "";
      sta_password = "";
      sta_enabled = false;
      prefs.begin("gateway", false);
      prefs.remove("sta_ssid");
      prefs.remove("sta_pass");
      prefs.putBool("sta_en", false);
      prefs.end();
      WiFi.disconnect(true);
      esp_wifi_disconnect();
      WiFi.mode(WIFI_AP);
      esp_wifi_set_channel(current_effective_channel, WIFI_SECOND_CHAN_NONE);
      sta_conn_state = STA_STATE_IDLE;
      disableNapt();
      stopCaptiveDns();
      configureSoftApDhcp(false);
      delay(50);
      esp_wifi_deauth_sta(0);
      sendResponse("[OK] Da xoa trang Wi-Fi nguon! Mac dinh tro ve de trong.");
      return;
    }

    sta_ssid = newSsid;
    sta_password = newPass;
    sta_enabled = true;

    logPrintf("[STA] BLE dat Wi-Fi nguon: '%s'. Dang thu ket noi (chi luu Flash NVS khi ket noi thanh cong)...", sta_ssid.c_str());
    sendResponse("[OK] ESP32 dang thu ket noi vao: " + sta_ssid + ". Khi ket noi thanh cong se tu dong luu vao Flash!");

    triggerStaConnect();
    return;
  }

  // 4b. STA:ON | STA:OFF: Bật hoặc tắt Wi-Fi nguồn (Chế độ mạng nội bộ 5G)
  if (cmd.equalsIgnoreCase("STA:ON") || cmd.equalsIgnoreCase("sta:on")) {
    sta_enabled = true;
    prefs.begin("gateway", false);
    prefs.putBool("sta_en", true);
    prefs.end();
    if (sta_ssid.length() > 0) {
      triggerStaConnect();
      sendResponse("[OK] Da BAT Wi-Fi nguon (Internet Repeater). ESP32 dang ket noi vao: " + sta_ssid);
    } else {
      sta_conn_state = STA_STATE_IDLE;
      sendResponse("[OK] Da BAT Wi-Fi nguon. Chua co SSID duoc luu, hay dung lenh: WIFI:ssid,pass");
    }
    return;
  }

  if (cmd.equalsIgnoreCase("STA:OFF") || cmd.equalsIgnoreCase("sta:off")) {
    sta_enabled = false;
    prefs.begin("gateway", false);
    prefs.putBool("sta_en", false);
    prefs.end();
    WiFi.disconnect(true);
    esp_wifi_disconnect();
    WiFi.mode(WIFI_AP);
    esp_wifi_set_channel(current_effective_channel, WIFI_SECOND_CHAN_NONE);
    sta_conn_state = STA_STATE_IDLE;
    disableNapt();
    stopCaptiveDns();
    configureSoftApDhcp(false);
    delay(50);
    esp_wifi_deauth_sta(0);
    sendResponse("[OK] Da TAT Wi-Fi nguon! ESP32 chuyen sang che do MANG NOI BO OFFLINE. Dien thoai ket noi vao se giu nguyen 5G de truy cap Internet.");
    return;
  }

  // 5. AP:ssid[,pass]: Cấu hình Tên và Mật khẩu mạng phát SoftAP
  if (cmd.startsWith("AP:") || cmd.startsWith("ap:")) {
    String payload = cmd.substring(3);
    payload.trim();
    int comma = payload.indexOf(',');
    String newSsid = (comma >= 0) ? payload.substring(0, comma) : payload;
    String newPass = (comma >= 0) ? payload.substring(comma + 1) : ap_password;
    newSsid.trim(); newPass.trim();

    if (newSsid.length() > 0) {
      ap_ssid = newSsid;
      ap_password = newPass;

      prefs.begin("gateway", false);
      prefs.putString("ap_ssid", ap_ssid);
      prefs.putString("ap_pass", ap_password);
      prefs.end();

      sendResponse("[OK] Da luu SoftAP SSID='" + ap_ssid + "'. ESP32 se tu dong khoi dong lai sau 1.5 giay!");
      should_reboot = true;
      reboot_time = millis() + 1500;
    } else {
      sendResponse("[ERROR] Cu phap sai! Hay dung: AP:TenWifi,MatKhau (hoac AP:TenWifi de giu mat khau cu)");
    }
    return;
  }

  // 6. APCH:kenh: Đổi kênh phát sóng SoftAP (1 - 11 hoặc 0: Auto)
  if (cmd.startsWith("APCH:") || cmd.startsWith("apch:")) {
    String chStr = cmd.substring(5); chStr.trim();
    if (chStr.equalsIgnoreCase("AUTO") || chStr == "0") {
      ap_channel = 0;
      prefs.begin("gateway", false);
      prefs.putUChar("ap_ch", ap_channel);
      prefs.end();
      sendResponse("[OK] Da chuyen Kenh phat sang che do TU DONG (Auto Channel). ESP32 se khoi dong lai sau 1.5 giay!");
      should_reboot = true;
      reboot_time = millis() + 1500;
      return;
    }
    int ch = chStr.toInt();
    if (ch >= 1 && ch <= 11) {
      ap_channel = ch;
      prefs.begin("gateway", false);
      prefs.putUChar("ap_ch", ap_channel);
      prefs.end();
      sendResponse("[OK] Da luu kenh phat SoftAP: " + String(ap_channel) + ". ESP32 se khoi dong lai sau 1.5 giay!");
      should_reboot = true;
      reboot_time = millis() + 1500;
    } else {
      sendResponse("[ERROR] Kenh hop le tu 1 den 11 hoac 0 (AUTO)! (Kenh 12-13 bi chan de tranh dien thoai khong nhan song)");
    }
    return;
  }

  // 6b. AUTOCH / APCH:AUTO: Tự động phân tích và chọn kênh tối ưu
  if (cmd.equalsIgnoreCase("AUTOCH") || cmd.equalsIgnoreCase("autoch")) {
    String detail = "";
    uint8_t bestCh = findOptimalChannel(&detail);
    ap_channel = bestCh;
    prefs.begin("gateway", false);
    prefs.putUChar("ap_ch", ap_channel);
    prefs.end();
    sendResponse("[OK] " + detail + "\n=> Da tu dong chon Kenh: " + String(bestCh) + ". ESP32 se khoi dong lai sau 1.5 giay!");
    should_reboot = true;
    reboot_time = millis() + 1500;
    return;
  }

  // 6c. PROFILES: Xem danh sách các mẫu cấu hình mạng phát (Presets)
  if (cmd.equalsIgnoreCase("PROFILES") || cmd.equalsIgnoreCase("profiles")) {
    String out = "=== DANH SACH MAU CAU HINH MANG PHAT (PRESETS) ===\n";
    if (profile_count == 0) {
      out += "(Danh sach trong)\n";
    } else {
      for (int i = 0; i < profile_count; i++) {
        ApProfile &p = profiles[i];
        out += "[" + String(i + 1) + "] ID: " + p.id + " | '" + p.name + "'\n";
        out += "    SSID: " + p.ssid + " | Pass: " + (p.pass.length() > 0 ? p.pass : "[Open]") + " | Ch: " + (p.channel == 0 ? "Auto" : String(p.channel)) + "\n";
        out += "    IP: " + p.ip + " | Sub: " + p.subnet + " | DHCP: " + p.dhcp_start + " -> " + p.dhcp_end + "\n";
      }
      out += "Lenh: 'PROFILE:APPLY:id', 'PROFILE:SAVE:TenMoi', 'PROFILE:DEL:id'";
    }
    sendResponse(out);
    return;
  }

  // 6d. PROFILE:APPLY:id - Kích hoạt mẫu cấu hình
  if (cmd.startsWith("PROFILE:APPLY:") || cmd.startsWith("profile:apply:")) {
    String id = cmd.substring(14); id.trim();
    int num = id.toInt();
    if (num >= 1 && num <= profile_count) {
      id = profiles[num - 1].id;
    }
    if (applyProfile(id)) {
      sendResponse("[OK] Da ap dung mau cau hinh! ESP32 dang khoi dong lai...");
    } else {
      sendResponse("[ERROR] Khong tim thay ID mau cau hinh: " + id);
    }
    return;
  }

  // 6d2. PROFILE:UPDATE:id (hoac PROFILE:UPDATE:id,ten) - Cap nhat mau cau hinh voi thong so SoftAP hien tai
  if (cmd.startsWith("PROFILE:UPDATE:") || cmd.startsWith("profile:update:")) {
    String payload = cmd.substring(15); payload.trim();
    String id = payload;
    String newName = "";
    int comma = payload.indexOf(',');
    if (comma >= 0) {
      id = payload.substring(0, comma); id.trim();
      newName = payload.substring(comma + 1); newName.trim();
    }
    int num = id.toInt();
    if (num >= 1 && num <= profile_count) {
      id = profiles[num - 1].id;
    }
    bool found = false;
    for (int i = 0; i < profile_count; i++) {
      if (profiles[i].id == id) {
        if (newName.length() > 0) profiles[i].name = newName;
        profiles[i].ssid = ap_ssid;
        profiles[i].pass = ap_password;
        profiles[i].channel = ap_channel;
        profiles[i].bssid = ap_bssid_str;
        profiles[i].ip = ap_ip_str;
        profiles[i].subnet = ap_subnet_str;
        profiles[i].dhcp_start = dhcp_start_str;
        profiles[i].dhcp_end = dhcp_end_str;
        saveProfilesToNvs();
        sendResponse("[OK] Da cap nhat Preset '" + profiles[i].name + "' (ID: " + id + ") voi thong so mang phat hien tai!");
        found = true;
        break;
      }
    }
    if (!found) {
      sendResponse("[ERROR] Khong tim thay ID mau cau hinh: " + id);
    }
    return;
  }

  // 6e. PROFILE:DEL:id - Xóa mẫu cấu hình
  if (cmd.startsWith("PROFILE:DEL:") || cmd.startsWith("profile:del:")) {
    String id = cmd.substring(12); id.trim();
    int num = id.toInt();
    if (num >= 1 && num <= profile_count) {
      id = profiles[num - 1].id;
    }
    if (deleteProfile(id)) {
      sendResponse("[OK] Da xoa mau cau hinh: " + id);
    } else {
      sendResponse("[ERROR] Khong tim thay ID mau cau hinh: " + id);
    }
    return;
  }

  // 6f. PROFILE:SAVE:ten - Lưu cấu hình hiện tại thành mẫu mới
  if (cmd.startsWith("PROFILE:SAVE:") || cmd.startsWith("profile:save:")) {
    String name = cmd.substring(13); name.trim();
    if (name.length() == 0) name = "Preset " + String(profile_count + 1);
    ApProfile p;
    p.id = "p" + String(millis());
    p.name = name;
    p.ssid = ap_ssid;
    p.pass = ap_password;
    p.channel = ap_channel;
    p.bssid = ap_bssid_str;
    p.ip = ap_ip_str;
    p.subnet = ap_subnet_str;
    p.dhcp_start = dhcp_start_str;
    p.dhcp_end = dhcp_end_str;

    if (addOrUpdateProfile(p)) {
      sendResponse("[OK] Da luu cau hinh hien tai thanh Preset: '" + name + "' (ID: " + p.id + ")");
    } else {
      sendResponse("[ERROR] Danh sach mau da day (toi da 10 mau)!");
    }
    return;
  }

  // 6g. PROFILE:CLONE - Sao chép cấu hình Wi-Fi nguồn thành Preset AP
  if (cmd.equalsIgnoreCase("PROFILE:CLONE") || cmd.equalsIgnoreCase("PROFILE:CLONESTA")) {
    bool connected = (WiFi.status() == WL_CONNECTED);
    if (!connected && sta_ssid.length() == 0) {
      sendResponse("[ERROR] Chua ket noi Wi-Fi nguon (STA) de sao chep!");
      return;
    }
    String s_ssid = sta_ssid;
    String s_pass = sta_password;
    String s_bssid = connected ? WiFi.BSSIDstr() : ap_bssid_str;
    if (s_bssid.length() == 0) s_bssid = ap_bssid_str;

    IPAddress gw = connected ? WiFi.gatewayIP() : IPAddress(10, 51, 31, 254);
    String s_gw = gw.toString();
    if (s_gw == "0.0.0.0") s_gw = "10.51.31.254";

    IPAddress sub = connected ? WiFi.subnetMask() : IPAddress(255, 255, 255, 0);
    String s_sub = sub.toString();
    if (s_sub == "0.0.0.0") s_sub = "255.255.255.0";

    int s_ch = connected ? WiFi.channel() : ap_channel;

    IPAddress my_ip = connected ? WiFi.localIP() : IPAddress(10, 51, 31, 100);
    String prefix = String(my_ip[0]) + "." + String(my_ip[1]) + "." + String(my_ip[2]) + ".";
    String s_dhcp_start = prefix + "10";
    String s_dhcp_end = prefix + "200";

    ApProfile p;
    p.id = "p" + String(millis());
    p.name = s_ssid + " (Clone)";
    p.ssid = s_ssid;
    p.pass = s_pass;
    p.channel = s_ch;
    p.bssid = s_bssid;
    p.ip = s_gw;
    p.subnet = s_sub;
    p.dhcp_start = s_dhcp_start;
    p.dhcp_end = s_dhcp_end;

    if (addOrUpdateProfile(p)) {
      sendResponse("[OK] Da sao chep Wi-Fi nguon thanh Preset: '" + p.name + "'!");
      sendResponse("   • SSID: " + p.ssid);
      sendResponse("   • BSSID: " + p.bssid);
      sendResponse("   • Gateway: " + p.ip + " | Subnet: " + p.subnet);
      sendResponse("   • DHCP: " + p.dhcp_start + " -> " + p.dhcp_end);
      sendResponse("   • Channel: " + String(p.channel));
    } else {
      sendResponse("[ERROR] Danh sach mau da day (toi da 10 mau)!");
    }
    return;
  }

  // 7. APMAC:mac: Đổi địa chỉ BSSID (MAC) phần cứng
  if (cmd.startsWith("APMAC:") || cmd.startsWith("apmac:")) {
    String macStr = cmd.substring(6); macStr.trim();
    uint8_t dummy[6];
    if (parse_mac(macStr, dummy)) {
      ap_bssid_str = macStr;
      prefs.begin("gateway", false);
      prefs.putString("ap_bssid", ap_bssid_str);
      prefs.end();
      sendResponse("[OK] Da luu dia chi MAC SoftAP: " + ap_bssid_str + ". Gui 'REBOOT' de ap dung!");
    } else {
      sendResponse("[ERROR] Dinh dang MAC sai! Vi du hop le: APMAC:84:F1:47:4F:27:A0");
    }
    return;
  }

  // 8. APIP:ip,subnet: Đổi Gateway IP và Subnet Mask
  if (cmd.startsWith("APIP:") || cmd.startsWith("apip:")) {
    String payload = cmd.substring(5); payload.trim();
    int comma = payload.indexOf(',');
    String newIp = (comma >= 0) ? payload.substring(0, comma) : payload;
    String newSub = (comma >= 0) ? payload.substring(comma + 1) : ap_subnet_str;
    newIp.trim(); newSub.trim();
    IPAddress testIp, testSub;
    if (testIp.fromString(newIp) && testSub.fromString(newSub)) {
      ap_ip_str = newIp;
      ap_subnet_str = newSub;
      prefs.begin("gateway", false);
      prefs.putString("ap_ip", ap_ip_str);
      prefs.putString("ap_subnet", ap_subnet_str);

      // Tự động đồng bộ dải DHCP sang cùng subnet /24 với Gateway IP mới
      IPAddress curDhcpStart;
      if (!curDhcpStart.fromString(dhcp_start_str) || curDhcpStart[0] != testIp[0] || curDhcpStart[1] != testIp[1] || curDhcpStart[2] != testIp[2]) {
        dhcp_start_str = String(testIp[0]) + "." + String(testIp[1]) + "." + String(testIp[2]) + ".10";
        dhcp_end_str   = String(testIp[0]) + "." + String(testIp[1]) + "." + String(testIp[2]) + ".200";
        prefs.putString("dhcp_start", dhcp_start_str);
        prefs.putString("dhcp_end", dhcp_end_str);
      }
      prefs.end();
      sendResponse("[OK] Da luu Gateway IP: " + ap_ip_str + " | Subnet: " + ap_subnet_str + " | DHCP: " + dhcp_start_str + " -> " + dhcp_end_str + ". Gui 'REBOOT' de ap dung!");
    } else {
      sendResponse("[ERROR] IP hoac Subnet khong hop le! Vi du: APIP:10.51.31.254,255.255.255.0");
    }
    return;
  }

  // 9. DHCP:start,end: Cấu hình dải cấp IP ngẫu nhiên
  if (cmd.startsWith("DHCP:") || cmd.startsWith("dhcp:")) {
    String payload = cmd.substring(5); payload.trim();
    int comma = payload.indexOf(',');
    if (comma >= 0) {
      String startIp = payload.substring(0, comma);
      String endIp = payload.substring(comma + 1);
      startIp.trim(); endIp.trim();
      IPAddress sIp, eIp;
      if (sIp.fromString(startIp) && eIp.fromString(endIp)) {
        dhcp_start_str = startIp;
        dhcp_end_str = endIp;
        prefs.begin("gateway", false);
        prefs.putString("dhcp_start", dhcp_start_str);
        prefs.putString("dhcp_end", dhcp_end_str);
        prefs.end();
        configureSoftApDhcp();
        sendResponse("[OK] Da luu dai cap DHCP: " + dhcp_start_str + " -> " + dhcp_end_str);
        return;
      }
    }
    sendResponse("[ERROR] Cu phap sai! Vi du: DHCP:10.51.31.10,10.51.31.200");
    return;
  }

  // 10. BOOST:ON | BOOST:OFF: Bật tắt phát sóng cực đại
  if (cmd.equalsIgnoreCase("BOOST:ON") || cmd.equalsIgnoreCase("BOOST:1")) {
    tx_power_boost = true;
    prefs.begin("gateway", false);
    prefs.putBool("tx_boost", true);
    prefs.end();
    applyTxPower(true);
    sendResponse("[OK] Da BAT che do phat song cuc dai (20 dBm / ~100mW)!");
    return;
  }
  if (cmd.equalsIgnoreCase("BOOST:OFF") || cmd.equalsIgnoreCase("BOOST:0")) {
    tx_power_boost = false;
    prefs.begin("gateway", false);
    prefs.putBool("tx_boost", false);
    prefs.end();
    applyTxPower(false);
    sendResponse("[OK] Da VE che do phat song tieu chuan (15 dBm)!");
    return;
  }

  // 11. SETPIN:xxxxxx: Đổi mã PIN bảo mật Bluetooth
  if (cmd.startsWith("SETPIN:") || cmd.startsWith("setpin:")) {
    String newPinStr = cmd.substring(7);
    newPinStr.trim();
    if (newPinStr.length() == 6 && isValidPin(newPinStr)) {
      ble_pin = newPinStr.toInt();
      prefs.begin("gateway", false);
      prefs.putUInt("ble_pin", ble_pin);
      prefs.end();
      updateBleSecurityPin(ble_pin);
      logPrintf("[BLE-SEC] Da cap nhat ma PIN moi: %06u", ble_pin);
      sendResponse("[OK] Da doi ma PIN thanh cong sang: " + newPinStr);
    } else {
      sendResponse("[ERROR] Ma PIN khong hop le! Yeu cau dung 6 chu so (0-9).");
    }
    return;
  }

  // 12. BLERADIO:OFF: Tắt sóng Bluetooth LE
  if (cmd.equalsIgnoreCase("BLERADIO:OFF") || cmd.equalsIgnoreCase("BLERADIO:0")) {
    sendResponse("[OK] Dang tat hoan toan song Bluetooth LE theo yeu cau...");
    delay(500);
    setBleRadio(false);
    return;
  }

  // 13. OTAURL:url: Đổi đường dẫn GitHub version.json
  if (cmd.startsWith("OTAURL:") || cmd.startsWith("otaurl:")) {
    String u = cmd.substring(7); u.trim();
    if (u.length() > 0) {
      ota_version_url = u;
      prefs.begin("gateway", false);
      prefs.putString("ota_url", ota_version_url);
      prefs.end();
      sendResponse("[OK] Da luu GitHub OTA URL: " + ota_version_url);
    } else {
      sendResponse("[ERROR] URL khong duoc de trong!");
    }
    return;
  }

  // 14. OTA: Kiểm tra cập nhật từ GitHub
  if (cmd.equalsIgnoreCase("OTA") || cmd.equalsIgnoreCase("ota")) {
    if (WiFi.status() != WL_CONNECTED) {
      sendResponse("[ERROR] ESP32 chua ket noi Wi-Fi nguon (Internet)!");
      return;
    }
    String newVer, binUrl, changelog, errMsg;
    sendResponse("[OTA] Dang kiem tra version tren GitHub: " + ota_version_url + " ...");
    if (checkGithubOta(newVer, binUrl, changelog, errMsg)) {
      if (newVer != FIRMWARE_VERSION) {
        sendResponse("[OTA] Tim thay ban moi: v" + newVer + " (Hien tai: v" + FIRMWARE_VERSION + ")");
        if (changelog.length() > 0) sendResponse("[OTA] Changelog: " + changelog);
        sendResponse("[OTA] Gui lenh 'OTA:RUN' de bat dau tai va nap Flash!");
      } else {
        sendResponse("[OTA] ESP32 dang chay ban moi nhat: v" + FIRMWARE_VERSION);
      }
    } else {
      sendResponse("[ERROR] " + errMsg);
    }
    return;
  }

  // 15. OTA:RUN: Tải và nạp Flash firmware từ GitHub
  if (cmd.equalsIgnoreCase("OTA:RUN") || cmd.equalsIgnoreCase("ota:run")) {
    if (WiFi.status() != WL_CONNECTED) {
      sendResponse("[ERROR] ESP32 chua ket noi Wi-Fi nguon (Internet)!");
      return;
    }
    String newVer, binUrl, changelog, errMsg;
    if (checkGithubOta(newVer, binUrl, changelog, errMsg)) {
      sendResponse("[OTA] Bat dau tai firmware v" + newVer + " tu GitHub va ghi Flash...");
      pending_ota_bin_url = binUrl;
      pending_ota = true;
    } else {
      sendResponse("[ERROR] " + errMsg);
    }
    return;
  }

  // 16. CLEARLOGS / CLS: Xóa nhật ký hoạt động
  if (cmd.equalsIgnoreCase("CLEARLOGS") || cmd.equalsIgnoreCase("clearlogs") || cmd.equalsIgnoreCase("CLS") || cmd.equalsIgnoreCase("cls")) {
    logCount = 0;
    logHead = 0;
    sendResponse("[OK] Da xoa toan bo nhat ky hoat dong!");
    return;
  }

  // 17. REBOOT: Khởi động lại thiết bị
  if (cmd.equalsIgnoreCase("REBOOT") || cmd.equalsIgnoreCase("reboot")) {
    sendResponse("[OK] ESP32 se khoi dong lai sau 1 giay...");
    delay(500);
    ESP.restart();
    return;
  }

  sendResponse("[UNKNOWN] Lenh khong hop le. Gui 'HELP' hoac '?' de xem danh sach lenh.");
}

// Callbacks quản lý kết nối NimBLE Server
class BleServerCallbacks : public NimBLEServerCallbacks {
#ifdef NIMBLE_V2
  void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
#else
  void onConnect(NimBLEServer* pServer) override {
#endif
    bleConnected = true;
    Serial.println("[BLE] Dien thoai da ket noi thanh cong!");
    addLogToBuffer("[BLE] Dien thoai da ket noi thanh cong!");
  }

#ifdef NIMBLE_V2
  void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
#else
  void onDisconnect(NimBLEServer* pServer) override {
#endif
    bleConnected = false;
    Serial.println("[BLE] Dien thoai da ngat ket noi Bluetooth LE.");
    addLogToBuffer("[BLE] Dien thoai da ngat ket noi Bluetooth LE.");
    if (ble_enabled) {
      NimBLEDevice::startAdvertising();
    }
  }
};

// Callbacks nhận dữ liệu từ điện thoại gửi qua BLE RX
class BleRxCallbacks : public NimBLECharacteristicCallbacks {
#ifdef NIMBLE_V2
  void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo& connInfo) override {
#else
  void onWrite(NimBLECharacteristic *pCharacteristic) override {
#endif
    std::string val = pCharacteristic->getValue();
    if (val.length() > 0) {
      pending_ble_cmd = String(val.c_str());
      ble_cmd_pending = true;
    }
  }
};

void initBleServer() {
  NimBLEDevice::init(BLE_DEVICE_NAME);

  // Thiết lập bảo mật phần cứng (Passkey 123456 & Bonding)
  configureBleSecurity();

  pBleServer = NimBLEDevice::createServer();
  pBleServer->setCallbacks(new BleServerCallbacks());

  NimBLEService *pService = pBleServer->createService(SERVICE_UUID);

  // Characteristic TX: Bắn thông báo log về điện thoại (Notify) - Yêu cầu mã hóa kết nối
  pBleTxChar = pService->createCharacteristic(
    CHARACTERISTIC_UUID_TX,
    NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::READ_ENC
  );

  // Characteristic RX: Nhận lệnh cấu hình từ điện thoại (Write) - Yêu cầu mã hóa kết nối
  NimBLECharacteristic *pBleRxChar = pService->createCharacteristic(
    CHARACTERISTIC_UUID_RX,
    NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR | NIMBLE_PROPERTY::WRITE_ENC
  );
  pBleRxChar->setCallbacks(new BleRxCallbacks());

  pService->start();

  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();

  // Đóng gói cờ và Tên thiết bị vào gói tin quảng bá chính (Primary Advertisement Data)
  NimBLEAdvertisementData advData;
  advData.setFlags(0x06); // General Discoverable, BR/EDR not supported
  advData.setName(BLE_DEVICE_NAME);
  pAdvertising->setAdvertisementData(advData);

  // Đóng gói Service UUID 128-bit vào gói tin phản hồi quét (Scan Response Data)
  NimBLEAdvertisementData scanData;
  scanData.addServiceUUID(SERVICE_UUID);
  scanData.setName(BLE_DEVICE_NAME);
  pAdvertising->setScanResponseData(scanData);

#ifdef NIMBLE_V2
  pAdvertising->enableScanResponse(true);
#else
  pAdvertising->setScanResponse(true);
#endif

  if (ble_enabled) {
    pAdvertising->start();
    logPrintf("[BLE] Da khoi tao Nordic UART Service: %s (PIN: %06u)", BLE_DEVICE_NAME, ble_pin);
  } else {
    pAdvertising->stop();
    logMessage("[BLE] Bluetooth LE dang o che do TAT (Theo cau hinh).");
  }
}

// =========================================================================
//           CẤU TRÚC GÓI TIN DHCP ĐƯỢC ĐỊNH NGHĨA TRONG dhcp_packet.h
// =========================================================================

// =========================================================================
//           GIAO DIỆN WEB QUẢN TRỊ (PROGMEM - TỐI ƯU 100% OFFLINE)
// =========================================================================
#include "index.h"


// =========================================================================
//                           HÀM HỖ TRỢ KỸ THUẬT & GITHUB OTA
// =========================================================================

// Trích xuất giá trị trường chuỗi từ JSON (nhẹ, tiết kiệm RAM)
String parseJsonValue(const String &json, const String &key) {
  String searchKey = "\"" + key + "\"";
  int keyIdx = json.indexOf(searchKey);
  if (keyIdx < 0) return "";
  int colonIdx = json.indexOf(':', keyIdx + searchKey.length());
  if (colonIdx < 0) return "";
  int quoteStart = json.indexOf('\"', colonIdx + 1);
  if (quoteStart < 0) return "";
  int quoteEnd = json.indexOf('\"', quoteStart + 1);
  if (quoteEnd < 0) return "";
  String val = json.substring(quoteStart + 1, quoteEnd);
  val.replace("\\/", "/");
  return val;
}

// Trích xuất số nguyên từ JSON
int parseJsonInt(const String &json, const String &key, int defaultVal) {
  String searchKey = "\"" + key + "\"";
  int keyIdx = json.indexOf(searchKey);
  if (keyIdx < 0) return defaultVal;
  int colonIdx = json.indexOf(':', keyIdx + searchKey.length());
  if (colonIdx < 0) return defaultVal;
  int i = colonIdx + 1;
  while (i < json.length() && (json[i] == ' ' || json[i] == '\t' || json[i] == '\"')) i++;
  int start = i;
  while (i < json.length() && (isDigit(json[i]) || json[i] == '-')) i++;
  if (start == i) return defaultVal;
  return json.substring(start, i).toInt();
}

// Trích xuất boolean từ JSON
bool parseJsonBool(const String &json, const String &key, bool defaultVal) {
  String searchKey = "\"" + key + "\"";
  int keyIdx = json.indexOf(searchKey);
  if (keyIdx < 0) return defaultVal;
  int colonIdx = json.indexOf(':', keyIdx + searchKey.length());
  if (colonIdx < 0) return defaultVal;
  int commaIdx = json.indexOf(',', colonIdx + 1);
  int braceIdx = json.indexOf('}', colonIdx + 1);
  int endIdx = json.length();
  if (commaIdx >= 0 && commaIdx < endIdx) endIdx = commaIdx;
  if (braceIdx >= 0 && braceIdx < endIdx) endIdx = braceIdx;
  String val = json.substring(colonIdx + 1, endIdx);
  val.trim();
  val.toLowerCase();
  if (val.indexOf("true") >= 0 || val == "1") return true;
  if (val.indexOf("false") >= 0 || val == "0") return false;
  return defaultVal;
}

// Tự động phân tích và chọn kênh phát Wi-Fi tối ưu nhất (ít nhiễu nhất)
uint8_t findOptimalChannel(String *detailLog) {
  logMessage("[AUTO-CH] Dang quet chu dong va danh gia do nhieu kenh 2.4GHz...");
  int n = WiFi.scanNetworks(false, false, false, 40);

  int score[14] = {0};
  int apCount[14] = {0};

  if (n > 0) {
    for (int i = 0; i < n; i++) {
      int ch = WiFi.channel(i);
      int rssi = WiFi.RSSI(i);
      if (ch >= 1 && ch <= 13) {
        apCount[ch]++;
        int weight = max(10, rssi + 105);
        score[ch] += weight * 3;
        if (ch > 1) score[ch - 1] += weight * 2;
        if (ch < 13) score[ch + 1] += weight * 2;
        if (ch > 2) score[ch - 2] += weight;
        if (ch < 12) score[ch + 2] += weight;
      }
    }
    WiFi.scanDelete();
  }

  // Ưu tiên các kênh chuẩn không chồng lấn quốc tế (1, 6, 11)
  const uint8_t stdChannels[] = {1, 6, 11};
  uint8_t bestCh = 6;
  int minScore = score[6]; // Mặc định ưu tiên kênh 6

  for (int i = 0; i < 3; i++) {
    uint8_t ch = stdChannels[i];
    if (score[ch] < minScore) {
      minScore = score[ch];
      bestCh = ch;
    }
  }

  String logStr = "=== KET QUA DANH GIA KENH 2.4GHz ===\n";
  logStr += "Ch 1: " + String(apCount[1]) + " AP (Diem nhieu: " + String(score[1]) + ")\n";
  logStr += "Ch 6: " + String(apCount[6]) + " AP (Diem nhieu: " + String(score[6]) + ")\n";
  logStr += "Ch 11: " + String(apCount[11]) + " AP (Diem nhieu: " + String(score[11]) + ")\n";
  logStr += "=> DA CHON KENH TOI UU: Kenh " + String(bestCh) + " (Kenh 12-13 loai tru de tranh dien thoai khong nhan song)";
  logMessage(logStr);

  if (detailLog != NULL) {
    *detailLog = logStr;
  }
  return bestCh;
}

// Hàm chuẩn hóa ký tự đặc biệt cho định dạng JSON
String escapeJson(const String &s) {
  String out = "";
  out.reserve(s.length() + 8);
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '"') {
      out += "\\\"";
    } else if (c == '\\') {
      out += "\\\\";
    } else if (c == '\n') {
      out += "\\n";
    } else if (c == '\r') {
      out += "\\r";
    } else if (c == '\t') {
      out += "\\t";
    } else {
      out += c;
    }
  }
  return out;
}

// Chuyển đổi danh sách profiles thành chuỗi JSON
String serializeProfiles() {
  String json = "[";
  for (int i = 0; i < profile_count; i++) {
    if (i > 0) json += ",";
    ApProfile &p = profiles[i];
    json += "{";
    json += "\"id\":\"" + escapeJson(p.id) + "\",";
    json += "\"name\":\"" + escapeJson(p.name) + "\",";
    json += "\"ssid\":\"" + escapeJson(p.ssid) + "\",";
    json += "\"pass\":\"" + escapeJson(p.pass) + "\",";
    json += "\"channel\":" + String(p.channel) + ",";
    json += "\"bssid\":\"" + escapeJson(p.bssid) + "\",";
    json += "\"ip\":\"" + escapeJson(p.ip) + "\",";
    json += "\"subnet\":\"" + escapeJson(p.subnet) + "\",";
    json += "\"dhcp_start\":\"" + escapeJson(p.dhcp_start) + "\",";
    json += "\"dhcp_end\":\"" + escapeJson(p.dhcp_end) + "\"";
    json += "}";
  }
  json += "]";
  return json;
}

// Phân giải chuỗi JSON thành danh sách profiles
void deserializeProfiles(const String &jsonStr) {
  profile_count = 0;
  int idx = 0;
  while (profile_count < MAX_PROFILES) {
    int start = jsonStr.indexOf('{', idx);
    if (start < 0) break;
    int end = jsonStr.indexOf('}', start);
    if (end < 0) break;
    String obj = jsonStr.substring(start, end + 1);

    ApProfile &p = profiles[profile_count];
    p.id = parseJsonValue(obj, "id");
    if (p.id.length() > 0) {
      p.name = parseJsonValue(obj, "name");
      p.ssid = parseJsonValue(obj, "ssid");
      p.pass = parseJsonValue(obj, "pass");
      p.channel = (uint8_t)parseJsonInt(obj, "channel", 6);
      p.bssid = parseJsonValue(obj, "bssid");
      p.ip = parseJsonValue(obj, "ip");
      p.subnet = parseJsonValue(obj, "subnet");
      p.dhcp_start = parseJsonValue(obj, "dhcp_start");
      p.dhcp_end = parseJsonValue(obj, "dhcp_end");
      profile_count++;
    }
    idx = end + 1;
  }
}

// Lưu danh sách profiles vào bộ nhớ Flash NVS
void saveProfilesToNvs() {
  String s = serializeProfiles();
  prefs.begin("gateway", false);
  prefs.putString("ap_profiles", s);
  prefs.end();
}

// Tải danh sách profiles từ bộ nhớ Flash NVS
void loadProfilesFromNvs() {
  prefs.begin("gateway", false);
  String s = prefs.getString("ap_profiles", "");
  prefs.end();
  if (s.length() < 10) {
    profile_count = 2;
    profiles[0] = {
      "p1",
      "Mặc định (ODE Wireless)",
      "ODE - wireless WiFi",
      "Ode6868@",
      6,
      "84:F1:47:4F:27:A0",
      "10.51.31.254",
      "255.255.255.0",
      "10.51.31.10",
      "10.51.31.200"
    };
    profiles[1] = {
      "p2",
      "Tối ưu Tự Động (Auto Channel)",
      "ODE - wireless WiFi",
      "Ode6868@",
      0, // Auto Channel
      "84:F1:47:4F:27:A0",
      "10.51.31.254",
      "255.255.255.0",
      "10.51.31.10",
      "10.51.31.200"
    };
    saveProfilesToNvs();
  } else {
    deserializeProfiles(s);
    bool changed = false;
    for (int i = 0; i < profile_count; i++) {
      if (profiles[i].ip == "10.51.31.254" && (profiles[i].dhcp_start.startsWith("10.51.28.") || profiles[i].subnet == "255.255.252.0")) {
        profiles[i].dhcp_start = "10.51.31.10";
        profiles[i].dhcp_end = "10.51.31.200";
        profiles[i].subnet = "255.255.255.0";
        changed = true;
      }
    }
    if (changed) {
      saveProfilesToNvs();
    }
  }
}

// Thêm mới hoặc cập nhật một profile
bool addOrUpdateProfile(const ApProfile &newP) {
  for (int i = 0; i < profile_count; i++) {
    if (profiles[i].id == newP.id) {
      profiles[i] = newP;
      saveProfilesToNvs();
      return true;
    }
  }
  if (profile_count < MAX_PROFILES) {
    profiles[profile_count] = newP;
    if (profiles[profile_count].id.length() == 0) {
      profiles[profile_count].id = "p" + String(millis());
    }
    profile_count++;
    saveProfilesToNvs();
    return true;
  }
  return false;
}

// Xóa một profile khỏi danh sách
bool deleteProfile(const String &id) {
  int foundIdx = -1;
  for (int i = 0; i < profile_count; i++) {
    if (profiles[i].id == id) {
      foundIdx = i;
      break;
    }
  }
  if (foundIdx >= 0) {
    for (int i = foundIdx; i < profile_count - 1; i++) {
      profiles[i] = profiles[i + 1];
    }
    profile_count--;
    saveProfilesToNvs();
    return true;
  }
  return false;
}

// Kích hoạt áp dụng một profile
bool applyProfile(const String &id) {
  for (int i = 0; i < profile_count; i++) {
    if (profiles[i].id == id) {
      ApProfile &p = profiles[i];
      ap_ssid = p.ssid;
      ap_password = p.pass;
      ap_channel = p.channel;
      ap_bssid_str = p.bssid;
      ap_ip_str = p.ip;
      ap_subnet_str = p.subnet;
      dhcp_start_str = p.dhcp_start;
      dhcp_end_str = p.dhcp_end;

      prefs.begin("gateway", false);
      prefs.putString("ap_ssid", ap_ssid);
      prefs.putString("ap_pass", ap_password);
      prefs.putUChar("ap_ch", ap_channel);
      prefs.putString("ap_bssid", ap_bssid_str);
      prefs.putString("ap_ip", ap_ip_str);
      prefs.putString("ap_subnet", ap_subnet_str);
      prefs.putString("dhcp_start", dhcp_start_str);
      prefs.putString("dhcp_end", dhcp_end_str);
      prefs.end();

      logPrintf("[PROFILE] Da ap dung Preset: '%s' (SSID: %s, Ch: %d). Dang restart AP...", 
                p.name.c_str(), ap_ssid.c_str(), ap_channel);
      should_reboot = true;
      reboot_time = millis() + 1500;
      return true;
    }
  }
  return false;
}

// Chuyển đổi giữa GitHub Raw URL và jsDelivr CDN Mirror URL
String getJsdelivrMirrorUrl(const String &url) {
  if (url.startsWith("https://raw.githubusercontent.com/")) {
    String sub = url.substring(34);
    int s1 = sub.indexOf('/');
    if (s1 > 0) {
      int s2 = sub.indexOf('/', s1 + 1);
      if (s2 > 0) {
        int s3 = sub.indexOf('/', s2 + 1);
        if (s3 > 0) {
          String owner = sub.substring(0, s1);
          String repo = sub.substring(s1 + 1, s2);
          String branch = sub.substring(s2 + 1, s3);
          String path = sub.substring(s3 + 1);
          return "https://cdn.jsdelivr.net/gh/" + owner + "/" + repo + "@" + branch + "/" + path;
        }
      }
    }
  }
  return url;
}

String getGithubRawUrl(const String &url) {
  if (url.startsWith("https://cdn.jsdelivr.net/gh/")) {
    String sub = url.substring(28);
    int s1 = sub.indexOf('/');
    int at = sub.indexOf('@');
    int s2 = sub.indexOf('/', s1 + 1);
    if (s1 > 0 && at > s1 && s2 > at) {
      String owner = sub.substring(0, s1);
      String repo = sub.substring(s1 + 1, at);
      String branch = sub.substring(at + 1, s2);
      String path = sub.substring(s2 + 1);
      return "https://raw.githubusercontent.com/" + owner + "/" + repo + "/" + branch + "/" + path;
    }
  }
  return url;
}

// Callback thu thập dữ liệu JSON từ esp_http_client
static esp_err_t _ota_http_event_handler(esp_http_client_event_t *evt) {
  if (evt->event_id == HTTP_EVENT_ON_DATA) {
    String *out = (String *)evt->user_data;
    if (out && evt->data && evt->data_len > 0) {
      char tmp[evt->data_len + 1];
      memcpy(tmp, evt->data, evt->data_len);
      tmp[evt->data_len] = '\0';
      *out += tmp;
    }
  }
  return ESP_OK;
}

// Helper thực hiện tải JSON qua HTTPS bằng Native ESP-IDF esp_http_client (Bộ đệm chỉ 1KB thay vì 32KB mbedTLS)
bool fetchOtaJson(const String &targetUrl, String &payloadOut, String &errOut) {
  logPrintf("[OTA] Kiem tra: %s (Free: %u | Max: %u)", targetUrl.c_str(), ESP.getFreeHeap(), ESP.getMaxAllocHeap());

  String responseBody = "";
  esp_http_client_config_t config = {};
  config.url = targetUrl.c_str();
  config.event_handler = _ota_http_event_handler;
  config.user_data = &responseBody;
  config.buffer_size = 1024;
  config.buffer_size_tx = 512;
  config.timeout_ms = 15000;
  config.skip_cert_common_name_check = true;
  config.addr_type = HTTP_ADDR_TYPE_INET;
  config.max_redirection_count = 5;

  esp_http_client_handle_t client = esp_http_client_init(&config);
  if (!client) {
    errOut = "Khong the khoi tao esp_http_client";
    logPrintf("[OTA-LOI] %s", errOut.c_str());
    return false;
  }

  esp_err_t err = esp_http_client_perform(client);
  if (err == ESP_OK) {
    int statusCode = esp_http_client_get_status_code(client);
    if (statusCode == 200) {
      payloadOut = responseBody;
      esp_http_client_cleanup(client);
      return true;
    } else {
      errOut = "May chu tra ve HTTP: " + String(statusCode);
      logPrintf("[OTA-LOI] Ket noi %s that bai: %s", targetUrl.c_str(), errOut.c_str());
      esp_http_client_cleanup(client);
      return false;
    }
  } else {
    errOut = "Loi ket noi: " + String(esp_err_to_name(err));
    logPrintf("[OTA-LOI] Ket noi %s that bai: %s", targetUrl.c_str(), errOut.c_str());
    esp_http_client_cleanup(client);
    return false;
  }
}

// Kiểm tra phiên bản mới từ GitHub / CDN HTTPS (version.json)
bool checkGithubOta(String &newVer, String &binUrl, String &changelog, String &errMsg) {
  if (WiFi.status() != WL_CONNECTED) {
    errMsg = "ESP32 chua ket noi Wi-Fi nguon (Internet)!";
    return false;
  }
  if (ota_version_url.length() == 0) {
    errMsg = "Chua cau hinh duong dan version.json!";
    return false;
  }

  // Khởi tạo mốc thời gian tối thiểu hợp lệ để mbedTLS không từ chối chứng chỉ SSL
  if (time(nullptr) < 1794186000) {
    struct timeval tv = { .tv_sec = 1794186000, .tv_usec = 0 }; // 2026-11-09
    settimeofday(&tv, NULL);
  }
  configTime(7 * 3600, 0, "pool.ntp.org", "time.google.com", "time.cloudflare.com");

  // Thu hoi bo nho scan Wi-Fi truoc khi khoi tao SSL TLS
  cached_scan_json = "[]";

  // Đảm bảo default netif là sta_netif trước khi kết nối HTTPS ra ngoài
  esp_netif_t *sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
  if (sta_netif != NULL) {
    esp_netif_set_default_netif(sta_netif);
  }

  // Luôn cấu hình DNS máy chủ Google (8.8.8.8) và Cloudflare (1.1.1.1) trực tiếp để tránh bị nhà mạng chặn DNS
  ip_addr_t dns1, dns2;
  IP_ADDR4(&dns1, 8, 8, 8, 8);
  IP_ADDR4(&dns2, 1, 1, 1, 1);
  dns_setserver(0, &dns1);
  dns_setserver(1, &dns2);

  String primaryUrl = ota_version_url;
  String mirrorUrl = "";
  if (primaryUrl.indexOf("cdn.jsdelivr.net") >= 0) {
    mirrorUrl = getGithubRawUrl(primaryUrl);
  } else {
    mirrorUrl = getJsdelivrMirrorUrl(primaryUrl);
  }

  String payload = "";
  bool ok = fetchOtaJson(primaryUrl, payload, errMsg);
  if (!ok && mirrorUrl.length() > 0 && mirrorUrl != primaryUrl) {
    logPrintf("[OTA] Thu lai qua Mirror: %s", mirrorUrl.c_str());
    delay(200);
    String mirrorErr = "";
    if (fetchOtaJson(mirrorUrl, payload, mirrorErr)) {
      ok = true;
      errMsg = "";
    } else {
      errMsg += " | Mirror: " + mirrorErr;
    }
  }

  if (!ok) {
    return false;
  }

  newVer = parseJsonValue(payload, "version");
  binUrl = parseJsonValue(payload, "url");
  changelog = parseJsonValue(payload, "changelog");

  if (newVer.length() == 0 || binUrl.length() == 0) {
    errMsg = "File version.json thieu truong 'version' hoac 'url'!";
    return false;
  }

  logPrintf("[OTA] Kiem tra thanh cong! Phien ban may chu: v%s | Hien tai: v%s", newVer.c_str(), FIRMWARE_VERSION.c_str());
  return true;
}

// Helper thực hiện tải firmware và ghi Flash qua Streaming esp_http_client + Update (tối ưu bộ nhớ, chống nghẽn BLE)
bool executeOtaDownload(const String &dlUrl) {
  logPrintf("[OTA] Bat dau tai firmware tu: %s", dlUrl.c_str());
  logPrintf("[OTA-MEM] Free Heap: %u bytes | Max Block: %u bytes", ESP.getFreeHeap(), ESP.getMaxAllocHeap());

  // Thu hồi RAM scan Wi-Fi và tạm dừng DNS captive để giải phóng tối đa bộ nhớ liên tục cho SSL
  cached_scan_json = "[]";
  if (dns_captive_running) {
    dnsServer.stop();
    dns_captive_running = false;
  }
  delay(200);

  // Đảm bảo default netif là sta_netif trước khi kết nối HTTPS ra ngoài
  esp_netif_t *sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
  if (sta_netif != NULL) {
    esp_netif_set_default_netif(sta_netif);
  }

  // Luôn cấu hình DNS máy chủ Google (8.8.8.8) và Cloudflare (1.1.1.1) trực tiếp
  ip_addr_t dns1, dns2;
  IP_ADDR4(&dns1, 8, 8, 8, 8);
  IP_ADDR4(&dns2, 1, 1, 1, 1);
  dns_setserver(0, &dns1);
  dns_setserver(1, &dns2);

  // Khởi tạo mốc thời gian hệ thống tối thiểu
  if (time(nullptr) < 1794186000) {
    struct timeval tv = { .tv_sec = 1794186000, .tv_usec = 0 }; // 2026-11-09
    settimeofday(&tv, NULL);
  }

  String currentUrl = dlUrl;
  esp_http_client_handle_t client = nullptr;
  int status_code = 0;
  int content_length = 0;
  int redirect_count = 0;

  while (redirect_count < 3) {
    esp_http_client_config_t config = {};
    config.url = currentUrl.c_str();
    config.buffer_size = 2048;
    config.buffer_size_tx = 512;
    config.timeout_ms = 25000;
    config.skip_cert_common_name_check = true;
    config.addr_type = HTTP_ADDR_TYPE_INET;

    client = esp_http_client_init(&config);
    if (!client) {
      logMessage("[OTA-LOI] Khong the khoi tao esp_http_client!");
      return false;
    }

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
      logPrintf("[OTA-LOI] Khong the ket noi den may chu (Ma loi: %s)", esp_err_to_name(err));
      esp_http_client_cleanup(client);
      client = nullptr;
      return false;
    }

    content_length = esp_http_client_fetch_headers(client);
    status_code = esp_http_client_get_status_code(client);

    if (status_code == 301 || status_code == 302 || status_code == 307) {
      char *loc = nullptr;
      if (esp_http_client_get_header(client, "Location", &loc) == ESP_OK && loc != nullptr && strlen(loc) > 0) {
        logPrintf("[OTA] Chuyen huong HTTP %d -> %s", status_code, loc);
        currentUrl = String(loc);
        redirect_count++;
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        client = nullptr;
        continue;
      }
    }
    break;
  }

  if (status_code != 200 && status_code != 206) {
    logPrintf("[OTA-LOI] May chu tra ve ma loi HTTP: %d", status_code);
    if (client) {
      esp_http_client_close(client);
      esp_http_client_cleanup(client);
    }
    return false;
  }

  size_t update_size = (content_length > 0) ? (size_t)content_length : UPDATE_SIZE_UNKNOWN;
  if (content_length > 0) {
    logPrintf("[OTA] Kich thuoc Firmware: %d bytes (%.2f MB)", content_length, (float)content_length / (1024.0 * 1024.0));
  } else {
    logMessage("[OTA] Kich thuoc Firmware: Dang chunked stream (khong co Content-Length)");
  }

  if (!Update.begin(update_size)) {
    logPrintf("[OTA-LOI] Khong the khoi dong Update Flash (Ma loi: %d)", Update.getError());
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    return false;
  }

  uint8_t ota_buf[1024];
  int read_bytes = 0;
  size_t total_written = 0;
  int prev_percent = -1;
  unsigned long last_yield = millis();

  while ((read_bytes = esp_http_client_read(client, (char *)ota_buf, sizeof(ota_buf))) > 0) {
    size_t written = Update.write(ota_buf, read_bytes);
    if (written != (size_t)read_bytes) {
      logPrintf("[OTA-LOI] Ghi Flash that bai! (Da ghi: %u / %d)", total_written, content_length);
      Update.abort();
      break;
    }
    total_written += written;

    if (content_length > 0) {
      int percent = (int)((total_written * 100) / content_length);
      if (percent != prev_percent && (percent % 10 == 0 || percent == 100)) {
        prev_percent = percent;
        logPrintf("[OTA] Ghi Flash: %d%% (%u / %d bytes)", percent, total_written, content_length);
      }
    } else {
      if (total_written / (100 * 1024) != (total_written - written) / (100 * 1024)) {
        logPrintf("[OTA] Ghi Flash: %u KB da tai", total_written / 1024);
      }
    }

    // Nhường CPU cho stack BLE và lwIP duy trì kết nối không bị ngắt ('Connection lost')
    if (millis() - last_yield > 20) {
      vTaskDelay(pdMS_TO_TICKS(2));
      last_yield = millis();
    }
  }

  esp_http_client_close(client);
  esp_http_client_cleanup(client);

  if (Update.hasError() || !Update.end(true)) {
    logPrintf("[OTA-LOI] Hoan tat Update that bai (Ma loi: %d)", Update.getError());
    return false;
  }

  if (!Update.isFinished()) {
    logMessage("[OTA-LOI] Update chua hoan tat day du!");
    return false;
  }

  logPrintf("[OTA] 🎉 Nap Flash thanh cong 100%% (%u bytes)! Dang khoi dong lai vao Firmware moi...", total_written);
  delay(1000);
  ESP.restart();
  return true;
}

// Tải file firmware .bin từ GitHub/CDN và nạp vào phân vùng OTA của bộ nhớ Flash
void performGithubOta(const String &binUrl) {
  if (WiFi.status() != WL_CONNECTED) {
    logMessage("[OTA-LOI] ESP32 chua ket noi Wi-Fi nguon (Internet)!");
    return;
  }

  if (time(nullptr) < 1794186000) {
    struct timeval tv = { .tv_sec = 1794186000, .tv_usec = 0 }; // 2026-11-09
    settimeofday(&tv, NULL);
  }

  esp_netif_t *sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
  if (sta_netif != NULL) {
    esp_netif_set_default_netif(sta_netif);
  }

  // Luôn cấu hình DNS máy chủ Google (8.8.8.8) và Cloudflare (1.1.1.1) trực tiếp
  ip_addr_t dns1, dns2;
  IP_ADDR4(&dns1, 8, 8, 8, 8);
  IP_ADDR4(&dns2, 1, 1, 1, 1);
  dns_setserver(0, &dns1);
  dns_setserver(1, &dns2);

  String primaryBinUrl = binUrl;
  String mirrorBinUrl = "";
  if (primaryBinUrl.indexOf("cdn.jsdelivr.net") >= 0) {
    mirrorBinUrl = getGithubRawUrl(primaryBinUrl);
  } else {
    mirrorBinUrl = getJsdelivrMirrorUrl(primaryBinUrl);
  }

  bool ok = executeOtaDownload(primaryBinUrl);
  if (!ok && mirrorBinUrl.length() > 0 && mirrorBinUrl != primaryBinUrl) {
    logPrintf("[OTA] Thu lai tai firmware qua Mirror: %s", mirrorBinUrl.c_str());
    delay(1000); // Cho TCP stack và heap ổn định lại trước khi thử Mirror
    executeOtaDownload(mirrorBinUrl);
  }
}

// Chuyển đổi giữa IPAddress và uint32_t host order
uint32_t ipToU32(const IPAddress &ip) {
  return ((uint32_t)ip[0] << 24) | ((uint32_t)ip[1] << 16) | ((uint32_t)ip[2] << 8) | ip[3];
}

IPAddress u32ToIp(uint32_t val) {
  return IPAddress((val >> 24) & 0xFF, (val >> 16) & 0xFF, (val >> 8) & 0xFF, val & 0xFF);
}

// Phân giải chuỗi MAC "84:F1:47:4F:27:A0" sang mảng 6 byte
bool parse_mac(const String &macStr, uint8_t *macArr) {
  int values[6];
  if (sscanf(macStr.c_str(), "%x:%x:%x:%x:%x:%x", 
             &values[0], &values[1], &values[2], 
             &values[3], &values[4], &values[5]) == 6) {
    for (int i = 0; i < 6; i++) {
      macArr[i] = (uint8_t)values[i];
    }
    return true;
  }
  return false;
}

// Quản lý DNS Relay: NAPT tự động dịch gói tin DNS (UDP 53) trực tiếp ra mạng ngoài không cần proxy
void startDnsProxy() {}
void stopDnsProxy() {}
void handleDnsProxy() {}

// Bật NAPT và chuyển tiếp DNS an toàn với chuẩn ESP-IDF
void enableNapt() {
  if (!napt_enabled) {
    esp_netif_t *ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    esp_netif_t *sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (sta_netif != NULL) {
      esp_netif_set_default_netif(sta_netif);
    }
    if (ap_netif != NULL) {
      esp_netif_napt_enable(ap_netif);
      esp_netif_ip_info_t ip_info;
      if (esp_netif_get_ip_info(ap_netif, &ip_info) == ESP_OK) {
        LOCK_TCPIP_CORE();
        ip_napt_enable(ip_info.ip.addr, 1);
        const ip_addr_t *cur_dns0 = dns_getserver(0);
        if (cur_dns0 == NULL || ip_addr_isany(cur_dns0)) {
          ip_addr_t dnsserver;
          IP_ADDR4(&dnsserver, 8, 8, 8, 8);
          dns_setserver(0, &dnsserver);
        }
        const ip_addr_t *cur_dns1 = dns_getserver(1);
        if (cur_dns1 == NULL || ip_addr_isany(cur_dns1)) {
          ip_addr_t dnsserver2;
          IP_ADDR4(&dnsserver2, 1, 1, 1, 1);
          dns_setserver(1, &dnsserver2);
        }
        UNLOCK_TCPIP_CORE();
      }
    }
    napt_enabled = true;
    logMessage("[NAT] Da kich hoat NAPT dinh tuyen Internet!");
  }
}

// Tắt NAPT khi mất kết nối STA
void disableNapt() {
  if (napt_enabled) {
    esp_netif_t *ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    if (ap_netif != NULL) {
      esp_netif_napt_disable(ap_netif);
      esp_netif_ip_info_t ip_info;
      if (esp_netif_get_ip_info(ap_netif, &ip_info) == ESP_OK) {
        LOCK_TCPIP_CORE();
        ip_napt_enable(ip_info.ip.addr, 0);
        UNLOCK_TCPIP_CORE();
      }
    }
    napt_enabled = false;
    logMessage("[NAT] Da tam dung NAPT do mat ket noi nguon.");
  }
}

// Bật Captive Portal (chặn DNS để điện thoại tự nhảy popup)
void startCaptiveDns() {
  if (!sta_enabled) {
    if (dns_captive_running) {
      dnsServer.stop();
      dns_captive_running = false;
    }
    return;
  }
  if (!dns_captive_running) {
    IPAddress ap_ip;
    ap_ip.fromString(ap_ip_str);
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(DNS_PORT, "*", ap_ip);
    dns_captive_running = true;
    logMessage("[CAPTIVE] Da khoi chay Captive DNS Server (Cho nhap Wi-Fi).");
  }
}

// Tắt Captive Portal khi đã có Internet
void stopCaptiveDns() {
  if (dns_captive_running) {
    dnsServer.stop();
    dns_captive_running = false;
    logMessage("[CAPTIVE] Da tat Captive DNS Server (Cho phep luu thong DNS binh thuong).");
  }
}

// =========================================================================
//            CẤU HÌNH ESP-IDF NATIVE DHCP SERVER CHO SOFTAP
// =========================================================================

static uint32_t active_pool_start = 0;
static uint32_t active_pool_end = 0;

void configureSoftApDhcp(bool randomizePool) {
  esp_netif_t *ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
  if (ap_netif != NULL) {
    IPAddress s_ip, e_ip, gw_ip, sub_ip;
    if (s_ip.fromString(dhcp_start_str) && e_ip.fromString(dhcp_end_str) && 
        gw_ip.fromString(ap_ip_str) && sub_ip.fromString(ap_subnet_str)) {

      // Tự động kiểm tra và đảm bảo Subnet Mask /24 (255.255.255.0) và dải IP cùng subnet với Gateway IP
      if (s_ip[0] != gw_ip[0] || s_ip[1] != gw_ip[1] || s_ip[2] != gw_ip[2] || ap_subnet_str == "255.255.252.0") {
        ap_subnet_str = "255.255.255.0";
        dhcp_start_str = String(gw_ip[0]) + "." + String(gw_ip[1]) + "." + String(gw_ip[2]) + ".10";
        dhcp_end_str   = String(gw_ip[0]) + "." + String(gw_ip[1]) + "." + String(gw_ip[2]) + ".200";
        sub_ip.fromString(ap_subnet_str);
        s_ip.fromString(dhcp_start_str);
        e_ip.fromString(dhcp_end_str);
        prefs.begin("gateway", false);
        prefs.putString("ap_subnet", ap_subnet_str);
        prefs.putString("dhcp_start", dhcp_start_str);
        prefs.putString("dhcp_end", dhcp_end_str);
        prefs.end();
        Serial.println("[DHCP] Tu dong dong bo Subnet Mask thanh 255.255.255.0 (/24) va dai IP " + dhcp_start_str + " -> " + dhcp_end_str);
        addLogToBuffer("[DHCP] Tu dong dong bo Subnet Mask thanh 255.255.255.0 (/24) va dai IP " + dhcp_start_str + " -> " + dhcp_end_str);
      }

      // 1. Tạm dừng DHCPS để cài đặt thông số IP info và DHCP options
      esp_netif_dhcps_stop(ap_netif);

      // 2. Cấu hình IP và Subnet Mask /24 trực tiếp qua ESP-IDF
      bool has_uplink = (sta_enabled && WiFi.status() == WL_CONNECTED);
      esp_netif_ip_info_t ip_info;
      ip_info.ip.addr = (uint32_t)gw_ip;
      ip_info.netmask.addr = (uint32_t)sub_ip;
      ip_info.gw.addr = has_uplink ? (uint32_t)gw_ip : 0; // Khi có Internet STA: Gateway = gw_ip để DHCP cấp Option 3 cho client; Khi tắt STA: Gateway = 0.0.0.0 để điện thoại dùng 5G
      esp_netif_set_ip_info(ap_netif, &ip_info);

      uint32_t s_u32 = ipToU32(s_ip);
      uint32_t e_u32 = ipToU32(e_ip);
      uint32_t gw_u32 = ipToU32(gw_ip);

      if (e_u32 < s_u32) {
        uint32_t tmp = s_u32; s_u32 = e_u32; e_u32 = tmp;
      }

      uint32_t total = e_u32 - s_u32 + 1;
      const uint32_t WINDOW_SIZE = 60; // ESP-IDF DHCPS_MAX_LEASE mac dinh la 100

      // Chi sinh ngau nhien pool moi khi chua co pool hoac khi duoc yeu cau (khi tat ca client da ngat)
      if (active_pool_start == 0 || active_pool_end == 0 || randomizePool) {
        uint32_t cur_start = s_u32;
        uint32_t cur_end = e_u32;

        if (total > WINDOW_SIZE) {
          uint32_t max_offset = total - WINDOW_SIZE;
          uint32_t rand_offset = esp_random() % (max_offset + 1);
          cur_start = s_u32 + rand_offset;
          cur_end = cur_start + WINDOW_SIZE - 1;

          // Tranh trung IP voi Gateway
          if (gw_u32 >= cur_start && gw_u32 <= cur_end) {
            if (cur_start >= s_u32 + WINDOW_SIZE) {
              cur_start -= WINDOW_SIZE;
              cur_end -= WINDOW_SIZE;
            } else if (cur_end + WINDOW_SIZE <= e_u32) {
              cur_start += WINDOW_SIZE;
              cur_end += WINDOW_SIZE;
            }
          }
        }
        active_pool_start = cur_start;
        active_pool_end = cur_end;
        logPrintf("[DHCP] Da chon ngau nhien pool cap IP: %s -> %s (trong tong dai %s -> %s)",
                  u32ToIp(active_pool_start).toString().c_str(), u32ToIp(active_pool_end).toString().c_str(),
                  dhcp_start_str.c_str(), dhcp_end_str.c_str());
      }

      dhcps_lease_t lease;
      memset(&lease, 0, sizeof(lease));
      lease.enable = true;
      lease.start_ip.addr = (uint32_t)u32ToIp(active_pool_start);
      lease.end_ip.addr   = (uint32_t)u32ToIp(active_pool_end);

      esp_err_t opt_err = esp_netif_dhcps_option(
        ap_netif,
        ESP_NETIF_OP_SET,
        ESP_NETIF_REQUESTED_IP_ADDRESS,
        &lease,
        sizeof(dhcps_lease_t)
      );
      if (opt_err != ESP_OK) {
        logPrintf("[DHCP] Set dai IP that bai: %d", opt_err);
      }
    } else {
      esp_netif_dhcps_stop(ap_netif);
    }

    // 1. Cấu hình Option 3 (Router / Default Gateway):
    // Chỉ cấp Gateway khi THỰC SỰ có kết nối Wi-Fi nguồn (has_uplink)
    // Khi chưa có Internet STA hoặc khi tắt STA (chế độ 5G): TẮT Option 3 (OFFER_ROUTER = 0) -> Điện thoại dùng 5G không bị mất mạng!
    bool has_uplink = (sta_enabled && WiFi.status() == WL_CONNECTED);
    dhcps_offer_t dhcps_router_value = has_uplink ? OFFER_ROUTER : 0;
    esp_netif_dhcps_option(
      ap_netif,
      ESP_NETIF_OP_SET,
      ESP_NETIF_ROUTER_SOLICITATION_ADDRESS,
      &dhcps_router_value,
      sizeof(dhcps_router_value)
    );

    // 2. Cấu hình Option 6 (Domain Name Server - DNS):
    // Khi có Internet STA: Cấp DNS Server để client duyệt web trực tiếp qua NAPT
    // Khi chưa có Internet STA: KHÔNG cấp DNS (dhcps_dns_value = 0) để điện thoại dùng DNS 5G
    dhcps_offer_t dhcps_dns_value = has_uplink ? OFFER_DNS : 0;
    esp_netif_dhcps_option(
      ap_netif,
      ESP_NETIF_OP_SET,
      ESP_NETIF_DOMAIN_NAME_SERVER,
      &dhcps_dns_value,
      sizeof(dhcps_dns_value)
    );

    if (dhcps_dns_value == OFFER_DNS) {
      esp_netif_dns_info_t dns_main, dns_backup;
      dns_main.ip.type = ESP_IPADDR_TYPE_V4;
      dns_backup.ip.type = ESP_IPADDR_TYPE_V4;

      IPAddress upstreamDns = WiFi.dnsIP(0);
      if (upstreamDns == IPAddress(0, 0, 0, 0)) {
        upstreamDns = IPAddress(8, 8, 8, 8); // Google Public DNS
      }
      dns_main.ip.u_addr.ip4.addr = (uint32_t)upstreamDns;

      IPAddress backupDns = WiFi.dnsIP(1);
      if (backupDns == IPAddress(0, 0, 0, 0) || backupDns == upstreamDns) {
        backupDns = IPAddress(1, 1, 1, 1); // Cloudflare Public DNS
      }
      dns_backup.ip.u_addr.ip4.addr = (uint32_t)backupDns;

      esp_netif_set_dns_info(ap_netif, ESP_NETIF_DNS_MAIN, &dns_main);
      esp_netif_set_dns_info(ap_netif, ESP_NETIF_DNS_BACKUP, &dns_backup);
    }

    esp_err_t start_err = esp_netif_dhcps_start(ap_netif);
    logPrintf("[DHCP] Khoi chay Native ESP-IDF DHCP Server! (Ket qua: %d)", start_err);

    // Luon dam bao Default Netif la STA khi da ket noi Wi-Fi nguon
    esp_netif_t *sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (sta_netif != NULL && has_uplink) {
      esp_netif_set_default_netif(sta_netif);
    }
  } else {
    logMessage("[DHCP-LOI] Khong tim thay netif WIFI_AP_DEF!");
  }
}

// Lắng nghe sự kiện Wi-Fi SoftAP và Wi-Fi Nguồn STA
void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  if (event == ARDUINO_EVENT_WIFI_AP_STADISCONNECTED) {
    uint8_t *mac = info.wifi_ap_stadisconnected.mac;
    char buf[100];
    snprintf(buf, sizeof(buf), "[WIFI-AP] Thiet bi ngat ket noi: %02X:%02X:%02X:%02X:%02X:%02X",
              mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.println(buf);
    addLogToBuffer(String(buf));

    // Chi yeu cau tai ngau nhien pool IP khi tat ca thiet bi deu da ngat ket noi
    wifi_sta_list_t sta_list;
    if (esp_wifi_ap_get_sta_list(&sta_list) == ESP_OK && sta_list.num == 0) {
      dhcp_reconfigure_needed = true;
    }
  } else if (event == ARDUINO_EVENT_WIFI_AP_STACONNECTED) {
    uint8_t *mac = info.wifi_ap_staconnected.mac;
    char buf[120];
    snprintf(buf, sizeof(buf), "[WIFI-AP] Thiet bi moi ket noi: %02X:%02X:%02X:%02X:%02X:%02X. Dang cap IP...",
              mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.println(buf);
    addLogToBuffer(String(buf));

    last_ap_client_connected_time = millis(); // Ghi nhan thoi diem de hoan STA reconnect
  } else if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
    char buf[120];
    snprintf(buf, sizeof(buf), "[WIFI-STA] Da nhan IP: %s | Gateway: %s",
             WiFi.localIP().toString().c_str(), WiFi.gatewayIP().toString().c_str());
    Serial.println(buf);
    addLogToBuffer(String(buf));

    sta_connected_state = true; // Chốt trạng thái đã kết nối thành công
    sta_conn_state = STA_STATE_CONNECTED;

    // Khi có kết nối lần đầu thành công, LÚC NÀY MỚI LƯU vào Flash NVS
    prefs.begin("gateway", false);
    prefs.putString("sta_ssid", sta_ssid);
    prefs.putString("sta_pass", sta_password);
    prefs.putBool("sta_en", true);
    prefs.end();
    logPrintf("[STA] Ket noi thanh cong! Da luu Wi-Fi '%s' vao Flash NVS.", sta_ssid.c_str());

    // Đồng bộ thời gian hệ thống SNTP ngay khi có mạng để mbedTLS xác thực chứng chỉ TLS/SSL
    configTime(7 * 3600, 0, "pool.ntp.org", "time.google.com", "time.cloudflare.com");

    // Bổ sung DNS Cloudflare (1.1.1.1) làm dự phòng, TUYỆT ĐỐI KHÔNG ghi đè DNS chính của router
    esp_netif_t *sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (sta_netif != NULL) {
      esp_netif_dns_info_t dns_cf;
      dns_cf.ip.type = ESP_IPADDR_TYPE_V4;
      dns_cf.ip.u_addr.ip4.addr = (uint32_t)IPAddress(1, 1, 1, 1);
      esp_netif_set_dns_info(sta_netif, ESP_NETIF_DNS_BACKUP, &dns_cf);
      esp_netif_set_default_netif(sta_netif);
    }

    enableNapt();
    stopCaptiveDns();
    configureSoftApDhcp(false);
  } else if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
    if (sta_connected_state) {
      sta_connected_state = false;
      Serial.println("[WIFI-STA] Mat ket noi voi Wi-Fi nguon!");
      addLogToBuffer("[WIFI-STA] Mat ket noi voi Wi-Fi nguon!");
    }
    disableNapt();
    if (sta_enabled) {
      startCaptiveDns();
    } else {
      stopCaptiveDns();
    }
    // Khoa lai kenh SoftAP de bao ve song Wi-Fi AP khong bi loan kenh
    esp_wifi_set_channel(current_effective_channel, WIFI_SECOND_CHAN_NONE);
  }
}

// =========================================================================
//                         CÁC XỬ LÝ HTTP WEB SERVER
// =========================================================================

void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

// Bắt và chuyển hướng các request kiểm tra captive của iOS / Android / Windows
void handleCaptiveRedirect() {
  String uri = server.uri();
  String host = server.hostHeader();

  // 1. Khi CÓ kết nối Internet Wi-Fi nguồn (hoặc khi ở chế độ 5G !sta_enabled):
  // Phản hồi Success / 204 ngay lập tức cho các probe kiểm tra của điện thoại để điện thoại nhận biết đã có Internet
  if ((sta_enabled && WiFi.status() == WL_CONNECTED) || !sta_enabled) {
    if (uri.indexOf("hotspot-detect") >= 0 || uri.indexOf("success.html") >= 0 || uri.indexOf("canonical") >= 0 ||
        host.indexOf("apple") >= 0 || host.indexOf("airport") >= 0 || host.indexOf("ibook") >= 0 || 
        host.indexOf("itools") >= 0 || host.indexOf("thinkdifferent") >= 0) {
      server.send(200, "text/html", "<HTML><HEAD><TITLE>Success</TITLE></HEAD><BODY>Success</BODY></HTML>");
      return;
    }
    if (uri.indexOf("generate_204") >= 0 || uri.indexOf("gen_204") >= 0 ||
        host.indexOf("gstatic") >= 0 || host.indexOf("google") >= 0) {
      server.send(204, "text/plain", "");
      return;
    }
    if (uri.indexOf("connecttest") >= 0 || host.indexOf("msftconnecttest") >= 0) {
      server.send(200, "text/plain", "Microsoft Connect Test");
      return;
    }
    if (uri.indexOf("ncsi") >= 0 || host.indexOf("msftncsi") >= 0) {
      server.send(200, "text/plain", "Microsoft NCSI");
      return;
    }
    if (uri.indexOf("success.txt") >= 0) {
      server.send(200, "text/plain", "success\n");
      return;
    }
    if (host.length() > 0 && host != ap_ip_str && host != WiFi.localIP().toString() && host.indexOf("esp32.local") == -1) {
      server.send(404, "text/plain", "Not Found");
      return;
    }
    handleRoot();
    return;
  }

  // 2. Khi đang bật Wi-Fi nguồn nhưng CHƯA có mạng (chờ cấu hình): chuyển hướng về Web Portal để mở Captive Portal
  if (host.length() > 0 && host != ap_ip_str && host != WiFi.localIP().toString() && host.indexOf("esp32.local") == -1) {
    server.sendHeader("Location", String("http://") + ap_ip_str + "/", true);
    server.send(302, "text/plain", "");
    return;
  }
  handleRoot();
}

// API: Trả về trạng thái hệ thống dạng JSON
void handleApiStatus() {
  IPAddress sta_ip = WiFi.localIP();
  IPAddress sta_gw = WiFi.gatewayIP();
  int rssi = WiFi.RSSI();
  bool connected = (sta_enabled && WiFi.status() == WL_CONNECTED);

  char pinBuf[10];
  sprintf(pinBuf, "%06u", ble_pin);

  String safe_sta_pass = sta_password;
  safe_sta_pass.replace("\\", "\\\\");
  safe_sta_pass.replace("\"", "\\\"");

  String safe_ap_pass = ap_password;
  safe_ap_pass.replace("\\", "\\\\");
  safe_ap_pass.replace("\"", "\\\"");

  String json = "{";
  json += "\"sta_enabled\":" + String(sta_enabled ? "true" : "false") + ",";
  json += "\"sta_connecting\":" + String((sta_enabled && sta_conn_state == STA_STATE_CONNECTING) ? "true" : "false") + ",";
  json += "\"connected\":" + String(connected ? "true" : "false") + ",";
  json += "\"sta_ssid\":\"" + (sta_enabled ? sta_ssid : "") + "\",";
  json += "\"sta_pass\":\"" + (sta_enabled ? safe_sta_pass : "") + "\",";
  json += "\"sta_ip\":\"" + (connected ? sta_ip.toString() : "") + "\",";
  json += "\"sta_gw\":\"" + (connected ? sta_gw.toString() : "") + "\",";
  json += "\"sta_bssid\":\"" + (connected ? WiFi.BSSIDstr() : "") + "\",";
  json += "\"sta_subnet\":\"" + (connected ? WiFi.subnetMask().toString() : "") + "\",";
  json += "\"sta_channel\":" + String(connected ? WiFi.channel() : 0) + ",";
  json += "\"sta_rssi\":" + String(connected ? rssi : 0) + ",";
  json += "\"ap_ssid\":\"" + ap_ssid + "\",";
  json += "\"ap_pass\":\"" + safe_ap_pass + "\",";
  json += "\"ap_bssid\":\"" + ap_bssid_str + "\",";
  json += "\"ap_ip\":\"" + ap_ip_str + "\",";
  json += "\"ap_subnet\":\"" + ap_subnet_str + "\",";
  json += "\"ap_channel\":" + String(ap_channel) + ",";
  json += "\"effective_channel\":" + String(current_effective_channel) + ",";
  json += "\"dhcp_start\":\"" + dhcp_start_str + "\",";
  json += "\"dhcp_end\":\"" + dhcp_end_str + "\",";
  json += "\"tx_boost\":" + String(tx_power_boost ? "true" : "false") + ",";
  json += "\"ble_enabled\":" + String(ble_enabled ? "true" : "false") + ",";
  json += "\"ble_pin\":\"" + String(pinBuf) + "\",";
  json += "\"ble_connected\":" + String(bleConnected ? "true" : "false") + ",";
  json += "\"napt\":" + String(napt_enabled ? "true" : "false") + ",";
  json += "\"fw_ver\":\"" + FIRMWARE_VERSION + "\",";
  json += "\"ota_url\":\"" + ota_version_url + "\",";
  json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"max_alloc_heap\":" + String(ESP.getMaxAllocHeap()) + ",";
  json += "\"uptime\":" + String(millis() / 1000);
  json += "}";

  server.sendHeader("Connection", "close");
  server.send(200, "application/json", json);
}

// API: Bật / tắt chế độ phát sóng cực đại (Max TX Power Boost)
void handleApiPowerBoost() {
  if (server.hasArg("boost")) {
    String val = server.arg("boost");
    tx_power_boost = (val == "1" || val.equalsIgnoreCase("true"));

    prefs.begin("gateway", false);
    prefs.putBool("tx_boost", tx_power_boost);
    prefs.end();

    applyTxPower(tx_power_boost);

    String json = "{\"success\":true,\"tx_boost\":" + String(tx_power_boost ? "true" : "false") + "}";
    server.sendHeader("Connection", "close");
  server.send(200, "application/json", json);
  } else {
    server.send(400, "application/json", "{\"error\":\"Thieu tham so boost\"}");
  }
}

// API: Cài đặt Bluetooth LE (Bật/tắt sóng và đổi mã PIN)
void handleApiSaveBle() {
  String pinArg = "";
  if (server.hasArg("pin")) {
    pinArg = server.arg("pin");
  } else if (server.hasArg("plain")) {
    String plain = server.arg("plain");
    int pIdx = plain.indexOf("pin=");
    if (pIdx >= 0) {
      int amp = plain.indexOf('&', pIdx);
      pinArg = (amp >= 0) ? plain.substring(pIdx + 4, amp) : plain.substring(pIdx + 4);
    } else {
      pinArg = parseJsonValue(plain, "pin");
    }
  }
  pinArg.trim();

  if (pinArg.length() > 0) {
    if (pinArg.length() == 6 && isValidPin(pinArg)) {
      ble_pin = pinArg.toInt();
      prefs.begin("gateway", false);
      prefs.putUInt("ble_pin", ble_pin);
      prefs.end();
      updateBleSecurityPin(ble_pin);
      logPrintf("[BLE-SEC] Da doi ma PIN Bluetooth tu Web Portal thanh: %06u", ble_pin);
    } else {
      server.send(400, "application/json", "{\"error\":\"Ma PIN phai gom dung 6 chu so (0-9)\"}");
      return;
    }
  }

  if (server.hasArg("enabled")) {
    String en = server.arg("enabled");
    bool newEn = (en == "1" || en.equalsIgnoreCase("true"));
    setBleRadio(newEn);
  } else if (server.hasArg("plain")) {
    String plain = server.arg("plain");
    int eIdx = plain.indexOf("enabled=");
    if (eIdx >= 0) {
      int amp = plain.indexOf('&', eIdx);
      String val = (amp >= 0) ? plain.substring(eIdx + 8, amp) : plain.substring(eIdx + 8);
      setBleRadio(val == "1" || val.equalsIgnoreCase("true"));
    }
  }

  char pinStr[10];
  sprintf(pinStr, "%06u", ble_pin);
  String json = "{";
  json += "\"success\":true,";
  json += "\"ble_enabled\":" + String(ble_enabled ? "true" : "false") + ",";
  json += "\"ble_pin\":\"" + String(pinStr) + "\"";
  json += "}";
  server.sendHeader("Connection", "close");
  server.send(200, "application/json", json);
}

// Tuần tự hóa bộ đệm nhật ký hệ thống ra chuỗi JSON
String serializeLogsJson() {
  String json = "[";
  json.reserve(logCount * 60 + 10);
  int start = (logCount < LOG_BUFFER_SIZE) ? 0 : logHead;
  for (int i = 0; i < logCount; i++) {
    int idx = (start + i) % LOG_BUFFER_SIZE;
    if (i > 0) json += ",";
    String line = logBuffer[idx];
    line.replace("\\", "\\\\");
    line.replace("\"", "\\\"");
    line.replace("\r", "");
    line.replace("\n", "");
    json += "\"" + line + "\"";
  }
  json += "]";
  return json;
}

// API: Lấy danh sách các dòng log gần nhất
void handleApiLogs() {
  server.sendHeader("Connection", "close");
  server.send(200, "application/json", serializeLogsJson());
}

// API: Xóa bộ đệm log
void handleApiClearLogs() {
  logCount = 0;
  logHead = 0;
  server.send(200, "application/json", "{\"success\":true}");
}

// Quét nhanh Wi-Fi 2.4GHz và cập nhật bộ đệm JSON (dành cho BLE Console và Serial)
String performWifiScanJson() {
  if (WiFi.getMode() == WIFI_AP) {
    WiFi.mode(WIFI_AP_STA);
    delay(40);
  }
  if (WiFi.status() != WL_CONNECTED && WiFi.status() != WL_IDLE_STATUS) {
    WiFi.disconnect(false);
  }
  logMessage("[SCAN] Dang thuc hien quet Wi-Fi 2.4GHz...");
  int n = WiFi.scanNetworks(false, false, false, 40);
  if (n >= 0) {
    String json = "[";
    for (int i = 0; i < n; i++) {
      if (i > 0) json += ",";
      String ssid = WiFi.SSID(i);
      ssid.replace("\\", "\\\\");
      ssid.replace("\"", "\\\"");
      json += "{\"ssid\":\"" + ssid + "\",";
      json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
      json += "\"ch\":" + String(WiFi.channel(i)) + ",";
      json += "\"secure\":" + String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "true" : "false") + "}";
    }
    json += "]";
    WiFi.scanDelete();
    cached_scan_json = json;
    last_scan_time = millis();
    logPrintf("[SCAN] Quet xong! Tim thay %d mang Wi-Fi.", n);
    return json;
  }
  return cached_scan_json.length() > 2 ? cached_scan_json : "[]";
}

// API: Quét các mạng Wi-Fi 2.4GHz xung quanh bất đồng bộ (Non-blocking Async Scan cho Web Portal)
void handleApiScan() {
  bool force = server.hasArg("force") || server.hasArg("refresh");
  if (!force && cached_scan_json.length() > 5 && (millis() - last_scan_time < 20000)) {
    server.send(200, "application/json", cached_scan_json);
    return;
  }

  int16_t scanStatus = WiFi.scanComplete();
  if (scanStatus == WIFI_SCAN_RUNNING) {
    server.send(200, "application/json", "{\"status\":\"scanning\"}");
    return;
  }

  if (scanStatus >= 0) {
    String json = "[";
    for (int i = 0; i < scanStatus; i++) {
      if (i > 0) json += ",";
      String ssid = WiFi.SSID(i);
      ssid.replace("\\", "\\\\");
      ssid.replace("\"", "\\\"");
      json += "{\"ssid\":\"" + ssid + "\",";
      json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
      json += "\"ch\":" + String(WiFi.channel(i)) + ",";
      json += "\"secure\":" + String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "true" : "false") + "}";
    }
    json += "]";
    WiFi.scanDelete();
    cached_scan_json = json;
    last_scan_time = millis();
    logPrintf("[SCAN] Quet xong! Tim thay %d mang Wi-Fi.", scanStatus);
    server.sendHeader("Connection", "close");
  server.send(200, "application/json", json);
    return;
  }

  // Khởi động tiến trình quét mới bất đồng bộ
  if (WiFi.getMode() == WIFI_AP) {
    WiFi.mode(WIFI_AP_STA);
    delay(30);
  }
  if (WiFi.status() != WL_CONNECTED && WiFi.status() != WL_IDLE_STATUS) {
    WiFi.disconnect(false);
  }
  logMessage("[SCAN] Bat dau quet song Wi-Fi bat dong bo (Async Scan)...");
  int16_t startResult = WiFi.scanNetworks(true, false, false, 40);
  if (startResult == WIFI_SCAN_RUNNING || startResult >= 0) {
    server.send(200, "application/json", "{\"status\":\"scanning\"}");
  } else {
    server.send(200, "application/json", cached_scan_json.length() > 2 ? cached_scan_json : "[]");
  }
}

// API: Xóa hoàn toàn cấu hình Wi-Fi nguồn khỏi Flash NVS (Đưa về mặc định để trống)
void handleApiForgetSta() {
  sta_ssid = "";
  sta_password = "";
  sta_enabled = false;

  prefs.begin("gateway", false);
  prefs.remove("sta_ssid");
  prefs.remove("sta_pass");
  prefs.putBool("sta_en", false);
  prefs.end();

  WiFi.disconnect(true);
  esp_wifi_disconnect();
  WiFi.mode(WIFI_AP);
  esp_wifi_set_channel(current_effective_channel, WIFI_SECOND_CHAN_NONE);
  sta_conn_state = STA_STATE_IDLE;
  disableNapt();
  stopCaptiveDns();
  configureSoftApDhcp(false);
  delay(50);
  esp_wifi_deauth_sta(0);

  logMessage("[STA] Da xoa trang Wi-Fi nguon khoi Flash NVS (Mac dinh de trong).");
  server.send(200, "application/json", "{\"success\":true,\"msg\":\"Da xoa Wi-Fi nguon! Bo nho Flash tro ve trang thai de trong.\"}");
}

// API: Tiếp nhận cấu hình Wi-Fi nguồn (STA) và bắt đầu kết nối thử (chỉ lưu Flash khi kết nối thành công)
void handleApiSaveSta() {
  if (server.hasArg("ssid")) {
    sta_ssid = server.arg("ssid");
    sta_password = server.arg("pass");
    sta_ssid.trim();
    sta_password.trim();

    if (sta_ssid.length() == 0) {
      handleApiForgetSta();
      return;
    }

    sta_enabled = true; // Bật cờ kết nối trong RAM

    logPrintf("[STA] Nhan Wi-Fi: '%s'. Dang thu ket noi (chi luu vao Flash NVS khi ket noi thanh cong)...", sta_ssid.c_str());

    triggerStaConnect();

    server.send(200, "application/json", "{\"success\":true,\"msg\":\"ESP32 dang thu ket noi vao Wi-Fi: " + sta_ssid + "! Thong tin se duoc luu khi ket noi thanh cong.\"}");
  } else {
    server.send(400, "application/json", "{\"error\":\"Thieu SSID\"}");
  }
}

// API: Bật / tắt chế độ Wi-Fi nguồn (Internet Repeater vs Mạng nội bộ Offline dùng 5G)
void handleApiSetStaMode() {
  if (server.hasArg("enabled")) {
    String val = server.arg("enabled");
    bool target_en = (val == "1" || val.equalsIgnoreCase("true"));

    prefs.begin("gateway", false);
    prefs.putBool("sta_en", target_en);
    prefs.end();

    pending_sta_mode_change = true;
    pending_sta_mode_val = target_en;
    sta_mode_change_time = millis() + 300; // Hoãn 300ms để WebServer gửi xong HTTP 200 JSON tới trình duyệt

    if (!target_en) {
      server.send(200, "application/json", "{\"success\":true,\"sta_enabled\":false,\"msg\":\"Da chuyen sang che do MANG NOI BO! Dien thoai ket noi se tu dong dung 5G de truy cap Internet.\"}");
    } else {
      server.send(200, "application/json", "{\"success\":true,\"sta_enabled\":true,\"msg\":\"Da BAT Wi-Fi nguon! ESP32 dang ket noi lai Internet...\"}");
    }
  } else {
    server.send(400, "application/json", "{\"error\":\"Thieu tham so enabled\"}");
  }
}

// API: Khởi động lại thiết bị ESP32 (Reboot System)
void handleApiReboot() {
  server.send(200, "application/json", "{\"success\":true,\"msg\":\"Dang khoi dong lai ESP32...\"}");
  logMessage("[SYS] Yeu cau khoi dong lai qua Web Portal!");
  should_reboot = true;
  reboot_time = millis() + 500;
}

// API: Lưu các tham số mạng phát (SoftAP) và khởi động lại
void handleApiSaveAp() {
  if (server.hasArg("ssid") && server.hasArg("ip") && server.hasArg("subnet")) {
    ap_ssid = server.arg("ssid");
    ap_password = server.arg("pass");
    if (server.hasArg("channel")) {
      int ch = server.arg("channel").toInt();
      if (ch >= 0 && ch <= 11) ap_channel = ch;
    }
    if (server.hasArg("bssid")) ap_bssid_str = server.arg("bssid");
    ap_ip_str = server.arg("ip");
    ap_subnet_str = server.arg("subnet");
    if (server.hasArg("dhcp_start")) dhcp_start_str = server.arg("dhcp_start");
    if (server.hasArg("dhcp_end")) dhcp_end_str = server.arg("dhcp_end");

    IPAddress testGw, testStart;
    if (testGw.fromString(ap_ip_str)) {
      if (!testStart.fromString(dhcp_start_str) || testStart[0] != testGw[0] || testStart[1] != testGw[1] || testStart[2] != testGw[2]) {
        dhcp_start_str = String(testGw[0]) + "." + String(testGw[1]) + "." + String(testGw[2]) + ".10";
        dhcp_end_str   = String(testGw[0]) + "." + String(testGw[1]) + "." + String(testGw[2]) + ".200";
      }
    }

    prefs.begin("gateway", false);
    prefs.putString("ap_ssid", ap_ssid);
    prefs.putString("ap_pass", ap_password);
    prefs.putUChar("ap_ch", ap_channel);
    prefs.putString("ap_bssid", ap_bssid_str);
    prefs.putString("ap_ip", ap_ip_str);
    prefs.putString("ap_subnet", ap_subnet_str);
    prefs.putString("dhcp_start", dhcp_start_str);
    prefs.putString("dhcp_end", dhcp_end_str);
    prefs.end();

    logMessage("[AP] Da luu toan bo tham so mang phat vao NVS Flash!");

    server.send(200, "application/json", "{\"success\":true,\"msg\":\"Da luu cau hinh mang phat! ESP32 se khoi dong lai sau 2 giay...\"}");

    should_reboot = true;
    reboot_time = millis() + 1500;
  } else {
    server.send(400, "application/json", "{\"error\":\"Thieu tham so bat buoc\"}");
  }
}

// API: Kích hoạt kiểm tra bản cập nhật mới từ GitHub (bất đồng bộ qua loopTask)
void handleApiCheckOta() {
  server.sendHeader("Connection", "close");
  if (WiFi.status() != WL_CONNECTED) {
    server.send(200, "application/json", "{\"success\":false,\"error\":\"ESP32 chua ket noi Wi-Fi nguon (Internet)!\"}");
    return;
  }
  if (ota_version_url.length() == 0) {
    server.send(200, "application/json", "{\"success\":false,\"error\":\"Chua cau hinh duong dan version.json!\"}");
    return;
  }
  if (ota_check_state == OTA_CHECK_RUNNING || ota_check_state == OTA_CHECK_REQUESTED) {
    server.send(200, "application/json", "{\"success\":true,\"state\":\"checking\"}");
    return;
  }
  ota_check_state = OTA_CHECK_REQUESTED;
  server.send(200, "application/json", "{\"success\":true,\"state\":\"checking\"}");
}

// API: Thăm dò kết quả kiểm tra OTA từ background task
void handleApiCheckOtaStatus() {
  server.sendHeader("Connection", "close");
  String json = "{";
  if (ota_check_state == OTA_CHECK_SUCCESS) {
    bool hasUpdate = (ota_new_ver != FIRMWARE_VERSION);
    json += "\"done\":true,";
    json += "\"success\":true,";
    json += "\"current_ver\":\"" + FIRMWARE_VERSION + "\",";
    json += "\"remote_ver\":\"" + ota_new_ver + "\",";
    json += "\"has_update\":" + String(hasUpdate ? "true" : "false") + ",";
    json += "\"bin_url\":\"" + ota_bin_url + "\",";
    json += "\"changelog\":\"" + ota_changelog + "\"";
  } else if (ota_check_state == OTA_CHECK_FAILED) {
    json += "\"done\":true,";
    json += "\"success\":false,";
    json += "\"error\":\"" + ota_err_msg + "\"";
  } else {
    json += "\"done\":false,";
    json += "\"success\":true";
  }
  json += "}";
  server.sendHeader("Connection", "close");
  server.send(200, "application/json", json);
}

// API: Kích hoạt quá trình tải và ghi Flash từ GitHub OTA
void handleApiRunOta() {
  if (WiFi.status() != WL_CONNECTED) {
    server.send(400, "application/json", "{\"error\":\"ESP32 chua ket noi Wi-Fi nguon!\"}");
    return;
  }

  String binUrl = "";
  if (server.hasArg("bin_url")) {
    binUrl = server.arg("bin_url");
    binUrl.trim();
  }

  if (binUrl.length() == 0) {
    server.send(400, "application/json", "{\"error\":\"Thieu tham so bin_url!\"}");
    return;
  }

  pending_ota_bin_url = binUrl;
  pending_ota = true;

  logPrintf("[OTA] Chap nhan lenh nang cap tu Web Portal. URL: %s", binUrl.c_str());
  server.send(200, "application/json", "{\"success\":true,\"msg\":\"ESP32 dang bat dau qua trinh tai firmware va ghi Flash...\"}");
}

// API: Lưu cấu hình URL version.json
void handleApiSaveOtaUrl() {
  if (server.hasArg("url")) {
    String u = server.arg("url");
    u.trim();
    if (u.length() > 0) {
      ota_version_url = u;
      prefs.begin("gateway", false);
      prefs.putString("ota_url", ota_version_url);
      prefs.end();
      logPrintf("[OTA] Da luu GitHub Version URL: %s", ota_version_url.c_str());

      String json = "{\"success\":true,\"ota_url\":\"" + ota_version_url + "\"}";
      server.sendHeader("Connection", "close");
  server.send(200, "application/json", json);
      return;
    }
  }
  server.send(400, "application/json", "{\"error\":\"URL khong hop le!\"}");
}

// API: Lấy danh sách toàn bộ Profiles
void handleApiGetProfiles() {
  server.send(200, "application/json", serializeProfiles());
}

// API: Lưu / Thêm mới / Cập nhật Profile
void handleApiSaveProfile() {
  if (server.hasArg("name") && server.hasArg("ssid")) {
    ApProfile p;
    p.id = (server.hasArg("id") && server.arg("id").length() > 0) ? server.arg("id") : ("p" + String(millis()));
    p.name = server.arg("name");
    p.name.trim();
    p.ssid = server.arg("ssid");
    p.ssid.trim();
    p.pass = server.hasArg("pass") ? server.arg("pass") : "";
    p.channel = server.hasArg("channel") ? server.arg("channel").toInt() : 6;
    p.bssid = (server.hasArg("bssid") && server.arg("bssid").length() > 0) ? server.arg("bssid") : ap_bssid_str;
    p.ip = (server.hasArg("ip") && server.arg("ip").length() > 0) ? server.arg("ip") : ap_ip_str;
    p.subnet = (server.hasArg("subnet") && server.arg("subnet").length() > 0) ? server.arg("subnet") : ap_subnet_str;
    p.dhcp_start = (server.hasArg("dhcp_start") && server.arg("dhcp_start").length() > 0) ? server.arg("dhcp_start") : dhcp_start_str;
    p.dhcp_end = (server.hasArg("dhcp_end") && server.arg("dhcp_end").length() > 0) ? server.arg("dhcp_end") : dhcp_end_str;

    if (p.name.length() == 0) p.name = p.ssid;

    if (addOrUpdateProfile(p)) {
      String json = "{\"success\":true,\"id\":\"" + escapeJson(p.id) + "\",\"msg\":\"Da luu mau cau hinh thanh cong!\"}";
      server.sendHeader("Connection", "close");
  server.send(200, "application/json", json);
      logPrintf("[PROFILE] Da luu/cap nhat Preset: '%s' (ID: %s)", p.name.c_str(), p.id.c_str());
    } else {
      server.send(400, "application/json", "{\"error\":\"Danh sach toi da 10 mau cau hinh!\"}");
    }
  } else {
    server.send(400, "application/json", "{\"error\":\"Thieu ten mau hoac SSID!\"}");
  }
}

// API: Xóa Profile
void handleApiDeleteProfile() {
  if (server.hasArg("id")) {
    String id = server.arg("id");
    if (deleteProfile(id)) {
      server.send(200, "application/json", "{\"success\":true,\"msg\":\"Da xoa mau cau hinh khoi danh sach!\"}");
    } else {
      server.send(404, "application/json", "{\"error\":\"Khong tim thay mau cau hinh de xoa!\"}");
    }
  } else {
    server.send(400, "application/json", "{\"error\":\"Thieu ID mau cau hinh!\"}");
  }
}

// API: Áp dụng Profile và khởi động lại AP
void handleApiApplyProfile() {
  if (server.hasArg("id")) {
    String id = server.arg("id");
    if (applyProfile(id)) {
      server.send(200, "application/json", "{\"success\":true,\"msg\":\"Da kich hoat mau cau hinh! ESP32 se khoi dong lai sau 2 giay...\"}");
    } else {
      server.send(404, "application/json", "{\"error\":\"Khong tim thay mau cau hinh de ap dung!\"}");
    }
  } else {
    server.send(400, "application/json", "{\"error\":\"Thieu ID mau cau hinh!\"}");
  }
}

// API: Tự động phân tích và chọn kênh tối ưu
void handleApiAutoChannel() {
  String detail = "";
  uint8_t bestCh = findOptimalChannel(&detail);
  ap_channel = bestCh;
  prefs.begin("gateway", false);
  prefs.putUChar("ap_ch", ap_channel);
  prefs.end();

  String json = "{";
  json += "\"success\":true,";
  json += "\"channel\":" + String(bestCh) + ",";
  json += "\"msg\":\"Da chon Kenh " + String(bestCh) + " toi uu nhat!\"";
  json += "}";
  server.sendHeader("Connection", "close");
  server.send(200, "application/json", json);
}

// API: Sao chép cấu hình Wi-Fi nguồn thành Preset AP
void handleApiCloneStaToProfile() {
  bool connected = (WiFi.status() == WL_CONNECTED);
  if (!connected && sta_ssid.length() == 0) {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"Chua ket noi Wi-Fi nguon!\"}");
    return;
  }

  String s_ssid = sta_ssid;
  String s_pass = sta_password;
  String s_bssid = connected ? WiFi.BSSIDstr() : ap_bssid_str;
  if (s_bssid.length() == 0) s_bssid = ap_bssid_str;

  IPAddress gw = connected ? WiFi.gatewayIP() : IPAddress(10, 51, 31, 254);
  String s_gw = gw.toString();
  if (s_gw == "0.0.0.0") s_gw = "10.51.31.254";

  IPAddress sub = connected ? WiFi.subnetMask() : IPAddress(255, 255, 255, 0);
  String s_sub = sub.toString();
  if (s_sub == "0.0.0.0") s_sub = "255.255.255.0";

  int s_ch = connected ? WiFi.channel() : ap_channel;

  // Dải IP DHCP theo quy tắc giống IP được cấp khi kết nối với Wi-Fi nguồn
  IPAddress my_ip = connected ? WiFi.localIP() : IPAddress(10, 51, 31, 100);
  String prefix = String(my_ip[0]) + "." + String(my_ip[1]) + "." + String(my_ip[2]) + ".";
  String s_dhcp_start = prefix + "10";
  String s_dhcp_end = prefix + "200";

  String p_id = "p_" + String(millis());
  String p_name = s_ssid + " (Clone)";

  ApProfile p;
  p.id = p_id;
  p.name = p_name;
  p.ssid = s_ssid;
  p.pass = s_pass;
  p.channel = s_ch;
  p.bssid = s_bssid;
  p.ip = s_gw;
  p.subnet = s_sub;
  p.dhcp_start = s_dhcp_start;
  p.dhcp_end = s_dhcp_end;

  if (addOrUpdateProfile(p)) {
    String json = "{\"success\":true,\"msg\":\"Da sao chep Wi-Fi nguon thanh Preset thanh cong!\",";
    json += "\"id\":\"" + p.id + "\",";
    json += "\"name\":\"" + p.name + "\",";
    json += "\"ssid\":\"" + p.ssid + "\",";
    json += "\"pass\":\"" + p.pass + "\",";
    json += "\"channel\":" + String(p.channel) + ",";
    json += "\"bssid\":\"" + p.bssid + "\",";
    json += "\"ip\":\"" + p.ip + "\",";
    json += "\"subnet\":\"" + p.subnet + "\",";
    json += "\"dhcp_start\":\"" + p.dhcp_start + "\",";
    json += "\"dhcp_end\":\"" + p.dhcp_end + "\"}";
    server.sendHeader("Connection", "close");
  server.send(200, "application/json", json);
    logMessage(String("[PRESET] Da sao chep Wi-Fi nguon '") + s_ssid + "' thanh Preset AP.");
  } else {
    server.send(500, "application/json", "{\"success\":false,\"error\":\"Danh sach Preset da day (toi da 10 mau)!\"}");
  }
}

// =========================================================================
//                               SETUP & LOOP
// =========================================================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- [KHOI TAO ESP32 REPEATER & CUSTOM RANDOM DHCP & BLE SECURE] ---");
  Serial.printf("[HARDWARE] Flash Chip Size: %d MB\n", ESP.getFlashChipSize() / (1024 * 1024));

  // Vo hieu hoa tinh nang tiet kiem dien Modem Sleep tren Wi-Fi de dam bao SoftAP phat song lien tuc,
  // khong bi giat lag, rot goi tin hoac vang ket noi thiet bi.
  WiFi.enableIPv6(false); // Tat IPv6 de tranh tra ve ban ghi AAAA gay loi connect khong co route
  WiFi.setSleep(false);
  esp_wifi_set_ps(WIFI_PS_NONE);

  // Đăng ký bắt sự kiện Wi-Fi ngắt kết nối
  WiFi.onEvent(onWiFiEvent);

  // Bước 1: Tải thông số từ bộ nhớ Flash NVS
  prefs.begin("gateway", false);
  String installed_ver = prefs.getString("fw_ver", "");

  if (installed_ver != FIRMWARE_VERSION) {
    logPrintf("[UPGRADE] Khoi tao phien ban: v%s (Ban cu: %s).", FIRMWARE_VERSION.c_str(), installed_ver.length() > 0 ? installed_ver.c_str() : "None");
    prefs.putString("fw_ver", FIRMWARE_VERSION);
    // Khi nap firmware moi lan dau: mac dinh luon de trong Wi-Fi nguon, tat Wi-Fi nguon
    prefs.remove("sta_ssid");
    prefs.remove("sta_pass");
    prefs.putBool("sta_en", false);

    // Chuyen doi ma PIN cu 70213 sang ma PIN mac dinh moi 123456 mot lan duy nhat khi nang cap
    uint32_t current_pin = prefs.getUInt("ble_pin", 123456);
    if (current_pin == 70213) {
      prefs.putUInt("ble_pin", 123456);
      logMessage("[UPGRADE] Da cap nhat ma PIN BLE mac dinh moi: 123456");
    }

    logMessage("[INIT] Mac dinh Wi-Fi nguon: DE TRONG (Tat Wi-Fi nguon). Chi luu khi ket noi lan dau thanh cong!");
  }

  sta_enabled    = prefs.getBool("sta_en", false); // Mặc định lần đầu là FALSE (Tắt Wi-Fi nguồn)
  sta_ssid       = prefs.getString("sta_ssid", ""); // Mặc định luôn để trống
  sta_password   = prefs.getString("sta_pass", ""); // Mặc định luôn để trống
  ap_ssid        = prefs.getString("ap_ssid", "ODE - wireless WiFi");
  ap_password    = prefs.getString("ap_pass", "Ode6868@");
  ap_channel     = prefs.getUChar("ap_ch", 6);
  if (ap_channel < 1 || ap_channel > 11) {
    if (ap_channel != 0) { // 0: Auto Channel
      ap_channel = 6;
      prefs.putUChar("ap_ch", 6);
    }
  }
  ap_bssid_str   = prefs.getString("ap_bssid", "84:F1:47:4F:27:A0");
  ap_ip_str      = prefs.getString("ap_ip", "10.51.31.254");
  ap_subnet_str  = prefs.getString("ap_subnet", "255.255.255.0");
  dhcp_start_str = prefs.getString("dhcp_start", "10.51.31.10");
  dhcp_end_str   = prefs.getString("dhcp_end", "10.51.31.200"); // Dải IP cấp ngẫu nhiên 10.51.31.x cùng subnet Gateway
  tx_power_boost = prefs.getBool("tx_boost", false);            // Mặc định KHÔNG BẬT (15 dBm Standard)
  ble_enabled     = prefs.getBool("ble_en", true);               // Bật sóng Bluetooth LE
  ble_pin         = prefs.getUInt("ble_pin", 123456);             // Mã PIN ghép đôi 6 số (mặc định: 123456)
  ota_version_url = prefs.getString("ota_url", "https://cdn.jsdelivr.net/gh/silversymphony/esp32-firmware@main/version.json");

  // Tự động đồng bộ Subnet Mask /24 (255.255.255.0) và dải IP cùng subnet với Gateway IP
  IPAddress check_gw, check_s;
  if (check_gw.fromString(ap_ip_str) && check_s.fromString(dhcp_start_str)) {
    if (check_gw[0] != check_s[0] || check_gw[1] != check_s[1] || check_gw[2] != check_s[2] || ap_subnet_str == "255.255.252.0") {
      ap_subnet_str = "255.255.255.0";
      dhcp_start_str = String(check_gw[0]) + "." + String(check_gw[1]) + "." + String(check_gw[2]) + ".10";
      dhcp_end_str   = String(check_gw[0]) + "." + String(check_gw[1]) + "." + String(check_gw[2]) + ".200";
      prefs.putString("ap_subnet", ap_subnet_str);
      prefs.putString("dhcp_start", dhcp_start_str);
      prefs.putString("dhcp_end", dhcp_end_str);
    }
  }
  prefs.end();

  // Nạp danh sách mẫu cấu hình mạng phát (SoftAP Presets)
  loadProfilesFromNvs();

  // Bước 1b: Cấu hình chân đèn LED báo trạng thái Wi-Fi nguồn (GPIO 2)
  pinMode(WIFI_STA_LED_PIN, OUTPUT);
  updateWifiLed();

  // Bước 2: Khởi động Bluetooth LE Console & Provisioning Server (Bảo mật mã PIN phần cứng)
  initBleServer();

  // Bước 3: Kích hoạt chế độ Wi-Fi (Không quét kênh ngầm khi khởi động để tránh nhảy kênh làm lag Wi-Fi AP)
  if (sta_enabled) {
    WiFi.mode(WIFI_AP_STA);
  } else {
    WiFi.mode(WIFI_AP);
    logMessage("[MODE] Khoi dong o che do MANG NOI BO (Offline Local LAN - Dien thoai dung 5G).");
  }
  WiFi.setAutoReconnect(false);

  // Bước 4: Nạp địa chỉ MAC (BSSID) phần cứng cho cổng phát AP
  uint8_t custom_bssid[6];
  if (parse_mac(ap_bssid_str, custom_bssid)) {
    esp_err_t err_mac = esp_wifi_set_mac(WIFI_IF_AP, custom_bssid);
    if (err_mac == ESP_OK) {
      logPrintf("[OK] Da nap BSSID: %s", ap_bssid_str.c_str());
    } else {
      logMessage("[LOI] Khong the nap BSSID!");
    }
  }

  // Bước 5: Cấu hình Gateway và Subnet Mask /22 cho mạng phát SoftAP qua Native ESP-IDF
  esp_netif_t *ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
  if (ap_netif != NULL) {
    IPAddress ap_ip, subnet;
    ap_ip.fromString(ap_ip_str);
    subnet.fromString(ap_subnet_str);
    esp_netif_ip_info_t ip_info;
    ip_info.ip.addr = (uint32_t)ap_ip;
    ip_info.netmask.addr = (uint32_t)subnet;
    ip_info.gw.addr = (sta_enabled && WiFi.status() == WL_CONNECTED) ? (uint32_t)ap_ip : 0; // Khi có Wi-Fi nguồn: gán gw để DHCPS cấp Option 3; Khi tắt STA: gw=0 để điện thoại dùng 5G
    esp_netif_dhcps_stop(ap_netif);
    esp_netif_set_ip_info(ap_netif, &ip_info);
  }

  current_effective_channel = ap_channel;
  if (current_effective_channel == 0) {
    logMessage("[AP] Kenh phat dat o che do AUTO -> Dang quet chon kenh toi uu...");
    current_effective_channel = findOptimalChannel();
    logPrintf("[AP] Da chon Kenh %d toi uu cho SoftAP!", current_effective_channel);
  }

  WiFi.softAP(ap_ssid.c_str(), (ap_password.length() > 0 ? ap_password.c_str() : NULL), current_effective_channel, 0, 4);

  // Bước 6: Áp dụng công suất phát sóng Wi-Fi (Max TX Power Boost)
  applyTxPower(tx_power_boost);

  // Bước 7: Cấu hình dải IP cho Native ESP-IDF DHCP Server
  configureSoftApDhcp(true);

  logPrintf("[AP] Phat song thanh cong: SSID: '%s' | MAC: %s | Gateway: %s | Kenh: %d%s", 
            ap_ssid.c_str(), WiFi.softAPmacAddress().c_str(), WiFi.softAPIP().toString().c_str(),
            current_effective_channel, (ap_channel == 0 ? " (Auto)" : ""));

  // Bước 8: Đăng ký các Endpoint cho Web Server
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleApiStatus);
  server.on("/api/power_boost", HTTP_POST, handleApiPowerBoost);
  server.on("/api/save_ble", HTTP_POST, handleApiSaveBle);
  server.on("/api/logs", HTTP_GET, handleApiLogs);
  server.on("/api/clear_logs", HTTP_POST, handleApiClearLogs);
  server.on("/api/scan", HTTP_GET, handleApiScan);
  server.on("/api/save_sta", HTTP_POST, handleApiSaveSta);
  server.on("/api/forget_sta", HTTP_POST, handleApiForgetSta);
  server.on("/api/save_ap", HTTP_POST, handleApiSaveAp);
  server.on("/api/profiles", HTTP_GET, handleApiGetProfiles);
  server.on("/api/save_profile", HTTP_POST, handleApiSaveProfile);
  server.on("/api/delete_profile", HTTP_POST, handleApiDeleteProfile);
  server.on("/api/apply_profile", HTTP_POST, handleApiApplyProfile);
  server.on("/api/clone_sta_to_profile", HTTP_POST, handleApiCloneStaToProfile);
  server.on("/api/auto_channel", HTTP_POST, handleApiAutoChannel);
  server.on("/api/check_ota", HTTP_POST, handleApiCheckOta);
  server.on("/api/check_ota_status", HTTP_GET, handleApiCheckOtaStatus);
  server.on("/api/run_ota", HTTP_POST, handleApiRunOta);
  server.on("/api/save_ota_url", HTTP_POST, handleApiSaveOtaUrl);
  server.on("/api/set_sta_mode", HTTP_POST, handleApiSetStaMode);
  server.on("/api/reboot", HTTP_POST, handleApiReboot);

  // Endpoint bẫy Captive Portal (iOS, Android, Windows)
  server.on("/hotspot-detect.html", HTTP_GET, handleCaptiveRedirect); // Apple
  server.on("/generate_204", HTTP_GET, handleCaptiveRedirect);       // Android
  server.on("/gen_204", HTTP_GET, handleCaptiveRedirect);            // Android
  server.on("/connecttest.txt", HTTP_GET, handleCaptiveRedirect);    // Windows
  server.on("/ncsi.txt", HTTP_GET, handleCaptiveRedirect);           // Windows
  server.onNotFound(handleCaptiveRedirect);

  server.begin();
  logMessage("[WEB] Web Server cong 80 da san sang!");

  // Bước 8b: Khởi tạo mDNS (http://esp32.local/)
  if (MDNS.begin("esp32")) {
    MDNS.addService("http", "tcp", 80);
    logMessage("[MDNS] Ten mien cuc bo: http://esp32.local/ da san sang!");
  }

  // Bước 9: Kết nối vào Wi-Fi nguồn (STA) nếu được bật
  if (sta_enabled && sta_ssid.length() > 0) {
    logPrintf("[STA] Dang ket noi vao mang nguon: %s (Toi da 4s)...", sta_ssid.c_str());
    WiFi.setAutoReconnect(false);
    WiFi.begin(sta_ssid.c_str(), (sta_password.length() > 0 ? sta_password.c_str() : NULL));
    sta_conn_state = STA_STATE_CONNECTING;
    sta_connect_start_time = millis();

    unsigned long start_time = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start_time < 4000)) {
      delay(200);
      Serial.print(".");
    }
    Serial.println();

    // Bước 10: Kiểm tra trạng thái Internet và thiết lập NAT hoặc Captive Portal
    if (WiFi.status() == WL_CONNECTED) {
      sta_conn_state = STA_STATE_CONNECTED;
      logPrintf("[STA] Da ket noi Internet thanh cong! IP nhan: %s", WiFi.localIP().toString().c_str());
      configTime(7 * 3600, 0, "pool.ntp.org", "time.google.com", "time.cloudflare.com");
      enableNapt();
      stopCaptiveDns();
      configureSoftApDhcp(false);
    } else {
      logPrintf("[STA] Khong the ket noi Wi-Fi nguon luc khoi dong. Dung quet, khoa kenh SoftAP (%d) va nghi 30s.", current_effective_channel);
      stopStaConnecting();
      startCaptiveDns();
    }
  } else {
    sta_conn_state = STA_STATE_IDLE;
    if (sta_enabled) {
      logMessage("[STA] Thiet bi chua cau hinh Wi-Fi nguon. Vui long vao Web Portal (10.51.31.254) hoac BLE Terminal de chon Wi-Fi.");
      startCaptiveDns();
    } else {
      logMessage("[MODE] Wi-Fi nguon dang TAT -> MANG NOI BO OFFLINE. Dien thoai dung 5G.");
      disableNapt();
      stopCaptiveDns();
      configureSoftApDhcp(false);
    }
  }

  logMessage("===> HE THONG GATEWAY + BLE SECURE + RANDOM DHCP DA SAN SANG! <===");
}

void loop() {
  // 1. Cập nhật trạng thái đèn LED báo Wi-Fi nguồn (GPIO 2)
  updateWifiLed();

  // 1b. Phục vụ các yêu cầu Web Server
  server.handleClient();

  // 2. Tái cấu hình ngẫu nhiên pool DHCP khi tat ca thiet bi da ngat ket noi
  if (dhcp_reconfigure_needed) {
    dhcp_reconfigure_needed = false;
    wifi_sta_list_t sta_list;
    if (esp_wifi_ap_get_sta_list(&sta_list) == ESP_OK && sta_list.num == 0) {
      logMessage("[DHCP] Tat ca thiet bi da ngat ket noi -> Chon ngau nhien pool IP moi...");
      configureSoftApDhcp(true);
    }
  }

  // 3. Phục vụ Captive DNS Server nếu chưa có Internet
  if (dns_captive_running) {
    dnsServer.processNextRequest();
  }

  // 4. Xử lý lệnh nhận được từ Bluetooth LE trong Task chính (tránh nghẽn NimBLE Host Task)
  if (ble_cmd_pending) {
    ble_cmd_pending = false;
    handleBleCommand(pending_ble_cmd);
    pending_ble_cmd = "";
  }

  // 5. Đồng bộ trạng thái kết nối BLE
  if (ble_enabled) {
    if (!bleConnected && oldBleConnected) {
      oldBleConnected = bleConnected;
    }
    if (bleConnected && !oldBleConnected) {
      oldBleConnected = bleConnected;
    }
  }

  // 6. Quản lý trạng thái kết nối Wi-Fi nguồn (Non-blocking Reconnect State Machine)
  if (sta_enabled && sta_ssid.length() > 0) {
    if (WiFi.status() == WL_CONNECTED) {
      if (sta_conn_state != STA_STATE_CONNECTED) {
        sta_conn_state = STA_STATE_CONNECTED;
      }
      if (!napt_enabled) {
        logMessage("[LOOP] STA da ket noi! Bat NAPT va tat Captive Portal.");
        enableNapt();
        stopCaptiveDns();
        configureSoftApDhcp(false);
      }
    } else {
      if (napt_enabled) {
        logMessage("[LOOP] STA mat ket noi! Tat NAPT va bat Captive Portal.");
        disableNapt();
        startCaptiveDns();
      }

      if (sta_conn_state == STA_STATE_CONNECTED) {
        // Vừa bị rớt mạng -> Ngắt quét, khóa lại kênh SoftAP, chờ 15s trước khi thử lại
        stopStaConnecting();
        sta_next_retry_time = millis() + 15000;
      } else if (sta_conn_state == STA_STATE_CONNECTING) {
        // Đang trong cửa sổ kết nối thử nghiệm (cho tối đa 4 giây)
        if (millis() - sta_connect_start_time > 4000) {
          logMessage("[STA] Het thoi gian cho (4s) khong thay router. Dung quet de dam bao on dinh SoftAP.");
          stopStaConnecting();
        }
      } else if (sta_conn_state == STA_STATE_IDLE) {
        // Đang ở trạng thái nghỉ: Radio 100% dành cho SoftAP
        if (millis() >= sta_next_retry_time) {
          // Chỉ thử lại nếu không bận các tác vụ quan trọng:
          // 1. Không có client nào vừa mới kết nối vào SoftAP trong 10 giây qua
          // 2. Không có tiến trình quét Wi-Fi ngầm nào đang chạy
          // 3. Không có tiến trình kiểm tra / nạp OTA nào đang chạy
          bool client_busy = (millis() - last_ap_client_connected_time < 10000);
          bool scan_busy = (WiFi.scanComplete() == WIFI_SCAN_RUNNING);
          bool ota_busy = (ota_check_state != OTA_CHECK_IDLE || pending_ota);

          if (!client_busy && !scan_busy && !ota_busy) {
            triggerStaConnect();
          } else {
            sta_next_retry_time = millis() + 5000; // Hoãn thêm 5s nếu đang bận
          }
        }
      }
    }
  } else {
    // Wi-Fi nguồn tắt hoặc chưa lưu SSID
    if (sta_conn_state != STA_STATE_IDLE) {
      sta_conn_state = STA_STATE_IDLE;
    }
    if (napt_enabled) disableNapt();
    if (dns_captive_running) {
      dnsServer.stop();
      dns_captive_running = false;
    }
  }

  // 6b. Xử lý chuyển đổi chế độ Wi-Fi nguồn sau khi WebServer đã gửi xong phản hồi HTTP
  if (pending_sta_mode_change && millis() >= sta_mode_change_time) {
    pending_sta_mode_change = false;
    sta_enabled = pending_sta_mode_val;

    if (!sta_enabled) {
      WiFi.disconnect(true);
      esp_wifi_disconnect();
      WiFi.mode(WIFI_AP);
      esp_wifi_set_channel(current_effective_channel, WIFI_SECOND_CHAN_NONE);
      sta_conn_state = STA_STATE_IDLE;
      disableNapt();
      stopCaptiveDns();
      configureSoftApDhcp(false);
      delay(50);
      esp_wifi_deauth_sta(0);
      logMessage("[MODE] Da TAT Wi-Fi nguon -> Che do MANG NOI BO (Dien thoai dung 5G).");
    } else {
      triggerStaConnect();
      configureSoftApDhcp(false);
      logMessage("[MODE] Da BAT Wi-Fi nguon -> Dang ket noi Internet...");
    }
  }

  // 6c. Xử lý kiểm tra OTA bất đồng bộ trong loopTask (tái sử dụng stack, không tốn thêm 8KB heap)
  if (ota_check_state == OTA_CHECK_REQUESTED) {
    ota_check_state = OTA_CHECK_RUNNING;
    delay(300); // Chờ WebServer socket gửi nốt HTTP 200 và đóng hoàn toàn kết nối
    logPrintf("[OTA-LOOP] Truoc khi check | Free: %u | Max: %u", ESP.getFreeHeap(), ESP.getMaxAllocHeap());
    bool ok = checkGithubOta(ota_new_ver, ota_bin_url, ota_changelog, ota_err_msg);
    ota_check_state = ok ? OTA_CHECK_SUCCESS : OTA_CHECK_FAILED;
  }

  // 7. Thực hiện OTA Firmware nếu có lệnh từ Web hoặc BLE
  if (pending_ota) {
    pending_ota = false;
    logMessage("[OTA] Tam dung cac tien trinh mang de uu tien ghi Flash...");
    delay(500); // Đảm bảo gói tin phản hồi HTTP 200 đã bay xong
    performGithubOta(pending_ota_bin_url);
  }

  // 8. Nhận và xử lý lệnh từ cổng USB Serial Monitor (Tương thích 100% với Bluetooth LE)
  if (Serial.available()) {
    String serialCmd = Serial.readStringUntil('\n');
    handleBleCommand(serialCmd);
  }

  // 9. Khởi động lại sau khi lưu cấu hình AP hoặc có lệnh từ Web/BLE
  if (should_reboot && millis() >= reboot_time) {
    logMessage("[REBOOT] Dang khoi dong lai ESP32...");
    delay(100);
    ESP.restart();
  }
}
