#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <esp_wifi.h>
#include <esp_netif.h>

// Nhúng thư viện Bluetooth Low Energy (BLE) & GAP Security
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <esp_gap_ble_api.h>

// Nhúng thư viện định tuyến NAT và DNS của LwIP
#if __has_include("lwip/lwip_napt.h")
#include "lwip/lwip_napt.h"
#else
#include "lwip/napt.h"
#endif

#include <lwip/ip_addr.h>
#include <lwip/ip4_addr.h>
#include <lwip/dns.h>
#include <lwip/tcpip.h>

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

// Nhúng thư viện nâng cấp Firmware không dây qua GitHub HTTPS (OTA)
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>

// =========================================================================
//                  QUẢN LÝ THÔNG SỐ CẤU HÌNH (PREFERENCES NVS)
// =========================================================================
Preferences prefs;

// Phiên bản Firmware & Quản trị GitHub OTA
const String FIRMWARE_VERSION = "1.0.0";
String ota_version_url;
bool pending_ota = false;
String pending_ota_bin_url = "";
bool checkGithubOta(String &newVer, String &binUrl, String &changelog, String &errMsg);
void performGithubOta(const String &binUrl);
bool parse_mac(const String &macStr, uint8_t *macArr);

// Wi-Fi nguồn (STA)
String sta_ssid;
String sta_password;

// Wi-Fi phát ra (AP)
String ap_ssid;
String ap_password;
uint8_t ap_channel;
String ap_bssid_str;
String ap_ip_str;
String ap_subnet_str;
String dhcp_start_str;
String dhcp_end_str;

// Chế độ công suất phát cực đại (Max TX Power Boost: 20 dBm vs 15 dBm)
bool tx_power_boost = true;

// Quản lý bảo mật Bluetooth LE
bool ble_enabled = true;
uint32_t ble_pin = 686868;

// Web Server & Captive DNS Server
WebServer server(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;

// Trạng thái vận hành
bool napt_enabled = false;
bool dns_captive_running = false;
bool should_reboot = false;
unsigned long reboot_time = 0;
unsigned long last_sta_check = 0;
volatile bool dhcp_reconfigure_needed = false;
void configureSoftApDhcp();

// =========================================================================
//      BLUETOOTH LOW ENERGY (BLE) - NORDIC UART SERVICE (CONSOLE & CONFIG)
// =========================================================================
#define BLE_DEVICE_NAME        "ESP32-Gateway-BLE"
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

BLEServer *pBleServer = NULL;
BLECharacteristic *pBleTxChar = NULL;
bool bleConnected = false;
bool oldBleConnected = false;

// =========================================================================
//              HỆ THỐNG GHI NHẬT KÝ KHÔNG DÂY (LOG BUFFER)
// =========================================================================
#define LOG_BUFFER_SIZE 35
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

// Bắn thông báo log qua BLE Notify trực tiếp cho điện thoại đã ghép đôi
void sendBleNotification(const String &msg) {
  if (bleConnected && pBleTxChar != NULL) {
    String out = msg + "\n";
    if (out.length() <= 64) {
      pBleTxChar->setValue((uint8_t*)out.c_str(), out.length());
      pBleTxChar->notify();
    } else {
      for (size_t i = 0; i < out.length(); i += 32) {
        String chunk = out.substring(i, i + 32);
        pBleTxChar->setValue((uint8_t*)chunk.c_str(), chunk.length());
        pBleTxChar->notify();
        delay(15);
      }
    }
  }
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

// Kiểm tra chuỗi có đúng 6 chữ số hợp lệ hay không
bool isValidPin(const String &s) {
  if (s.length() != 6) return false;
  for (int i = 0; i < 6; i++) {
    if (!isDigit(s[i])) return false;
  }
  return true;
}

// Cập nhật mã PIN bảo mật phần cứng cho BLE GAP
void updateBleSecurityPin(uint32_t pin) {
  ble_pin = pin;
  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_STATIC_PASSKEY, (void*)&ble_pin, sizeof(uint32_t));
}

// Thiết lập toàn bộ tham số bảo mật phần cứng BLE GAP
void configureBleSecurity() {
  // 1. Cài đặt mã PIN cố định 6 số
  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_STATIC_PASSKEY, (void*)&ble_pin, sizeof(uint32_t));

  // 2. Chế độ xác thực: Bắt buộc Secure Connections + Bảo vệ chống nghe lén (MITM) + Lưu ghép đôi (Bonding)
  esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;
  esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));

  // 3. Khả năng I/O: ESP_IO_CAP_OUT (Mạch sở hữu mã PIN, điện thoại phải nhập)
  esp_ble_io_cap_t iocap = ESP_IO_CAP_OUT; 
  esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));

  // 4. Độ dài khóa mã hóa tối đa (16 bytes = 128 bit AES)
  uint8_t key_size = 16;
  esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));

  // 5. Khóa mã hóa trao đổi
  uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
  uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));
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
    BLEDevice::startAdvertising();
    logMessage("[BLE] Da BAT song Bluetooth LE (ESP32-Gateway-BLE)");
  } else {
    if (bleConnected && pBleServer != NULL) {
      pBleServer->disconnect(0);
    }
    BLEDevice::getAdvertising()->stop();
    bleConnected = false;
    logMessage("[BLE] Da TAT hoan toan song Bluetooth LE (Khong the do thay)");
  }
}

// Phản hồi kết quả lệnh đồng thời ra USB Serial Monitor và Bluetooth LE Console
void sendResponse(const String &msg) {
  Serial.println(msg);
  sendBleNotification(msg);
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
    h += "\n-- [2. Mang Phat (SoftAP & DHCP)] --\n";
    h += "• AP:ssid,pass      -> Doi Ten & Mat khau Wi-Fi phat\n";
    h += "• APCH:kenh         -> Doi kenh phat song (1 - 13)\n";
    h += "• APMAC:mac         -> Doi dia chi MAC phan cung (VD: APMAC:84:F1:47:4F:27:A0)\n";
    h += "• APIP:ip,subnet    -> Doi Gateway IP & Subnet (VD: APIP:10.51.31.254,255.255.252.0)\n";
    h += "• DHCP:start,end    -> Doi dai cap IP ngau nhien (VD: DHCP:10.51.28.50,10.51.31.100)\n";
    h += "• BOOST:ON | OFF    -> Bat/tat phat song cuc dai 20 dBm (~100mW)\n";
    h += "\n-- [3. Bao Mat & Bluetooth] --\n";
    h += "• SETPIN:pin_6_so   -> Doi ma PIN ghep doi Bluetooth (VD: SETPIN:686868)\n";
    h += "• BLERADIO:OFF      -> Tat song Bluetooth LE (tang hinh)\n";
    h += "\n-- [4. Cloud OTA GitHub] --\n";
    h += "• OTAURL:url        -> Cai dat duong dan version.json\n";
    h += "• OTA               -> Kiem tra ban cap nhat moi tu GitHub\n";
    h += "• OTA:RUN           -> Tai va ghi Flash ban moi tu GitHub\n";
    h += "\n-- [5. He Thong] --\n";
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
    st += "STA SSID: " + (sta_ssid.length() > 0 ? sta_ssid : "(Chua cau hinh)") + "\n";
    st += "STA Status: " + String(WiFi.status() == WL_CONNECTED ? "DA KET NOI" : "CHUA KET NOI") + "\n";
    if (WiFi.status() == WL_CONNECTED) {
      st += "STA IP: " + WiFi.localIP().toString() + "\n";
      st += "STA Gateway: " + WiFi.gatewayIP().toString() + "\n";
      st += "STA RSSI: " + String(WiFi.RSSI()) + " dBm\n";
    }
    st += "-------------------------------\n";
    st += "AP SSID: " + ap_ssid + "\n";
    st += "AP Gateway: " + ap_ip_str + "\n";
    st += "AP Subnet: " + ap_subnet_str + "\n";
    st += "AP MAC: " + ap_bssid_str + "\n";
    st += "AP Channel: " + String(ap_channel) + "\n";
    st += "AP Clients: " + String(WiFi.softAPgetStationNum()) + "\n";
    st += "DHCP Pool: " + dhcp_start_str + " -> " + dhcp_end_str + " (Random)\n";
    st += "TX Power: " + String(tx_power_boost ? "20 dBm (Max Boost)" : "15 dBm (Standard)") + "\n";
    st += "BLE PIN: " + String(pinStr) + " | Radio: " + (ble_enabled ? "BAT" : "TAT") + "\n";
    st += "OTA URL: " + ota_version_url + "\n";
    st += "NAT Routing: " + String(napt_enabled ? "ON" : "OFF") + "\n";
    st += "RAM Free: " + String(ESP.getFreeHeap() / 1024) + " KB\n";
    st += "Uptime: " + String(millis() / 1000) + "s";
    sendResponse(st);
    return;
  }

  // 3. SCAN: Quét sóng Wi-Fi xung quanh
  if (cmd.equalsIgnoreCase("SCAN") || cmd.equalsIgnoreCase("scan")) {
    sendResponse("[SCAN] Dang quet cac mang Wi-Fi 2.4GHz xung quanh...");
    int n = WiFi.scanNetworks(false, false, false, 250);
    if (n == 0) {
      sendResponse("[SCAN] Khong tim thay mang Wi-Fi nao xung quanh.");
    } else {
      String out = "=== TIM THAY " + String(n) + " MANG WI-FI ===\n";
      for (int i = 0; i < n; i++) {
        out += String(i + 1) + ". " + WiFi.SSID(i) + " (" + String(WiFi.RSSI(i)) + " dBm) Ch:" + String(WiFi.channel(i)) + (WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? " [Lock]" : " [Open]") + "\n";
      }
      sendResponse(out);
    }
    WiFi.scanDelete();
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

    if (newSsid.length() > 0) {
      sta_ssid = newSsid;
      sta_password = newPass;

      prefs.begin("gateway", false);
      prefs.putString("sta_ssid", sta_ssid);
      prefs.putString("sta_pass", sta_password);
      prefs.end();

      logPrintf("[STA] Da cap nhat Wi-Fi nguon: '%s'. Dang ket noi lai...", sta_ssid.c_str());
      sendResponse("[OK] Da luu Wi-Fi! ESP32 dang ket noi vao: " + sta_ssid);

      WiFi.disconnect();
      delay(100);
      WiFi.begin(sta_ssid.c_str(), (sta_password.length() > 0 ? sta_password.c_str() : NULL));
    } else {
      sendResponse("[ERROR] Cu phap sai! Hay gui: WIFI:TenWifi,MatKhau");
    }
    return;
  }

  // 5. AP:ssid,pass: Cấu hình Tên và Mật khẩu mạng phát SoftAP
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
      sendResponse("[ERROR] Cu phap sai! Hay dung: AP:TenWifi,MatKhau");
    }
    return;
  }

  // 6. APCH:kenh: Đổi kênh phát sóng SoftAP (1 - 11)
  if (cmd.startsWith("APCH:") || cmd.startsWith("apch:")) {
    String chStr = cmd.substring(5); chStr.trim();
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
      sendResponse("[ERROR] Kenh hop le tu 1 den 11! (Kenh 12-13 bi chan de tranh dien thoai khong nhan song)");
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
      prefs.end();
      sendResponse("[OK] Da luu Gateway IP: " + ap_ip_str + " | Subnet: " + ap_subnet_str + ". Gui 'REBOOT' de ap dung!");
    } else {
      sendResponse("[ERROR] IP hoac Subnet khong hop le! Vi du: APIP:10.51.31.254,255.255.252.0");
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
        sendResponse("[OK] Da luu dai cap DHCP: " + dhcp_start_str + " -> " + dhcp_end_str);
        return;
      }
    }
    sendResponse("[ERROR] Cu phap sai! Vi du: DHCP:10.51.28.50,10.51.31.100");
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

// Callbacks quản lý bảo mật phần cứng BLE (Passkey / Bonding)
class BleSecurityCallbacks : public BLESecurityCallbacks {
  uint32_t onPassKeyRequest() {
    Serial.printf("[BLE-SEC] Client yeu cau PassKey -> Cung cap PIN: %06u\n", ble_pin);
    return ble_pin;
  }
  void onPassKeyNotify(uint32_t pass_key) {
    Serial.printf("[BLE-SEC] onPassKeyNotify: %06u\n", pass_key);
  }
  bool onConfirmPIN(uint32_t pass_key) {
    Serial.printf("[BLE-SEC] onConfirmPIN: %06u (Goc: %06u)\n", pass_key, ble_pin);
    return (pass_key == ble_pin);
  }
  bool onSecurityRequest() {
    return true;
  }
  void onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl) {
    if (cmpl.success) {
      logMessage("[BLE-SEC] Ghep doi PassKey phan cung THANH CONG!");
    } else {
      logPrintf("[BLE-SEC] Ghep doi PassKey THAT BAI! Ma loi: %d", cmpl.fail_reason);
    }
  }
};

// Callbacks quản lý kết nối BLE
class BleServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    bleConnected = true;
    logMessage("[BLE] Dien thoai da ket noi thanh cong!");
    sendBleNotification("=== ESP32 GATEWAY BLE CONSOLE SAN SANG ===");
  }

  void onDisconnect(BLEServer* pServer) {
    bleConnected = false;
    logMessage("[BLE] Dien thoai da ngat ket noi Bluetooth LE.");
  }
};

// Callbacks nhận dữ liệu từ điện thoại gửi qua BLE RX
class BleRxCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String rxValue = String(pCharacteristic->getValue().c_str());
    if (rxValue.length() > 0) {
      handleBleCommand(rxValue);
    }
  }
};

void initBleServer() {
  BLEDevice::init(BLE_DEVICE_NAME);

  // Thiết lập bảo mật phần cứng GAP trước khi tạo Server
  configureBleSecurity();
  BLEDevice::setSecurityCallbacks(new BleSecurityCallbacks());

  pBleServer = BLEDevice::createServer();
  pBleServer->setCallbacks(new BleServerCallbacks());

  BLEService *pService = pBleServer->createService(SERVICE_UUID);

  // Characteristic TX: Bắn thông báo log về điện thoại (Notify) - Bắt buộc mã hóa
  pBleTxChar = pService->createCharacteristic(
    CHARACTERISTIC_UUID_TX,
    BLECharacteristic::PROPERTY_NOTIFY
  );
  pBleTxChar->addDescriptor(new BLE2902());
  pBleTxChar->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);

  // Characteristic RX: Nhận lệnh cấu hình từ điện thoại (Write) - Bắt buộc mã hóa
  BLECharacteristic *pBleRxChar = pService->createCharacteristic(
    CHARACTERISTIC_UUID_RX,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
  );
  pBleRxChar->setCallbacks(new BleRxCallbacks());
  pBleRxChar->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);

  if (ble_enabled) {
    BLEDevice::startAdvertising();
    logPrintf("[BLE] Da khoi tao Nordic UART Service: %s (PIN: %06u)", BLE_DEVICE_NAME, ble_pin);
  } else {
    BLEDevice::getAdvertising()->stop();
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

// Kiểm tra phiên bản mới từ GitHub HTTPS (version.json)
bool checkGithubOta(String &newVer, String &binUrl, String &changelog, String &errMsg) {
  if (WiFi.status() != WL_CONNECTED) {
    errMsg = "ESP32 chua ket noi Wi-Fi nguon (Internet)!";
    return false;
  }
  if (ota_version_url.length() == 0) {
    errMsg = "Chua cau hinh duong dan GitHub version.json!";
    return false;
  }

  logPrintf("[OTA] Dang kiem tra version tu: %s", ota_version_url.c_str());

  WiFiClientSecure client;
  client.setInsecure(); // Kết nối GitHub HTTPS không cần nạp CA root certificate

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(8000);

  if (!http.begin(client, ota_version_url)) {
    errMsg = "Khong the khoi tao ket noi HTTPS den GitHub!";
    return false;
  }

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    errMsg = "Loi ket noi GitHub (Ma HTTP: " + String(httpCode) + ")";
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  newVer = parseJsonValue(payload, "version");
  binUrl = parseJsonValue(payload, "url");
  changelog = parseJsonValue(payload, "changelog");

  if (newVer.length() == 0 || binUrl.length() == 0) {
    errMsg = "File version.json thieu truong 'version' hoac 'url'!";
    return false;
  }

  logPrintf("[OTA] GitHub version: %s | Hien tai: %s", newVer.c_str(), FIRMWARE_VERSION.c_str());
  return true;
}

// Tải file esp32-firmware.bin từ GitHub và nạp vào phân vùng OTA của bộ nhớ Flash
void performGithubOta(const String &binUrl) {
  logPrintf("[OTA] Bat dau tai firmware va ghi Flash tu: %s", binUrl.c_str());

  WiFiClientSecure client;
  client.setInsecure();

  httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  httpUpdate.rebootOnUpdate(true);

  httpUpdate.onProgress([](int cur, int total) {
    static int lastPercent = -1;
    if (total > 0) {
      int percent = (cur * 100) / total;
      if (percent != lastPercent && (percent % 20 == 0 || percent == 100)) {
        lastPercent = percent;
        logPrintf("[OTA] Tien trinh Flash: %d%% (%d / %d bytes)", percent, cur, total);
      }
    }
  });

  t_httpUpdate_return ret = httpUpdate.update(client, binUrl);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      logPrintf("[OTA-LOI] Cap nhat that bai: (%d) %s", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      logMessage("[OTA] Khong co ban cap nhat!");
      break;
    case HTTP_UPDATE_OK:
      logMessage("[OTA] Nap Flash thanh cong! ESP32 dang khoi dong lai vao Firmware moi...");
      break;
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

// Bật NAPT và chuyển tiếp DNS an toàn với khóa luồng TCPIP
void enableNapt() {
  if (!napt_enabled) {
    IPAddress ap_ip;
    ap_ip.fromString(ap_ip_str);

    ip_addr_t dnsserver;
    IP_ADDR4(&dnsserver, 8, 8, 8, 8);

    LOCK_TCPIP_CORE();
    ip_napt_enable((uint32_t)ap_ip, 1);
    dns_setserver(0, &dnsserver);
    UNLOCK_TCPIP_CORE();

    napt_enabled = true;
    logMessage("[NAT] Da kich hoat NAPT dinh tuyen Internet!");
  }
}

// Tắt NAPT khi mất kết nối STA
void disableNapt() {
  if (napt_enabled) {
    IPAddress ap_ip;
    ap_ip.fromString(ap_ip_str);

    LOCK_TCPIP_CORE();
    ip_napt_enable((uint32_t)ap_ip, 0);
    UNLOCK_TCPIP_CORE();

    napt_enabled = false;
    logMessage("[NAT] Da tam dung NAPT do mat ket noi nguon.");
  }
}

// Bật Captive Portal (chặn DNS để điện thoại tự nhảy popup)
void startCaptiveDns() {
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
//            NATIVE ESP-IDF DHCP SERVER (CẤP IP NGẪU NHIÊN 100% ỔN ĐỊNH)
// =========================================================================

void configureSoftApDhcp() {
  esp_netif_t *ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
  if (ap_netif != NULL) {
    esp_netif_dhcps_stop(ap_netif);

    dhcps_lease_t lease;
    memset(&lease, 0, sizeof(lease));
    lease.enable = true;

    IPAddress s_ip, e_ip, gw_ip, sub_ip;
    if (s_ip.fromString(dhcp_start_str) && e_ip.fromString(dhcp_end_str) && 
        gw_ip.fromString(ap_ip_str) && sub_ip.fromString(ap_subnet_str)) {

      // Tự động kiểm tra và đồng bộ Subnet Mask thành 255.255.252.0 (/22) để Gateway 10.51.31.254 cùng subnet với dải 10.51.28.x
      if (((uint32_t)s_ip & (uint32_t)sub_ip) != ((uint32_t)gw_ip & (uint32_t)sub_ip)) {
        ap_subnet_str = "255.255.252.0";
        sub_ip.fromString(ap_subnet_str);
        prefs.begin("gateway", false);
        prefs.putString("ap_subnet", ap_subnet_str);
        prefs.end();
        WiFi.softAPConfig(gw_ip, gw_ip, sub_ip);
        logMessage("[DHCP] Tu dong sua Subnet Mask thanh 255.255.252.0 de tranh loi loop cap IP!");
      }

      uint32_t s_u32 = ipToU32(s_ip);
      uint32_t e_u32 = ipToU32(e_ip);
      uint32_t gw_u32 = ipToU32(gw_ip);

      if (e_u32 < s_u32) {
        uint32_t tmp = s_u32; s_u32 = e_u32; e_u32 = tmp;
      }

      uint32_t total = e_u32 - s_u32 + 1;
      const uint32_t WINDOW_SIZE = 60; // ESP-IDF DHCPS_MAX_LEASE mặc định là 100

      uint32_t cur_start = s_u32;
      uint32_t cur_end = e_u32;

      if (total > WINDOW_SIZE) {
        uint32_t max_offset = total - WINDOW_SIZE;
        uint32_t rand_offset = esp_random() % (max_offset + 1);
        cur_start = s_u32 + rand_offset;
        cur_end = cur_start + WINDOW_SIZE - 1;

        // Tránh trùng IP với Gateway
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

      IPAddress active_start = u32ToIp(cur_start);
      IPAddress active_end = u32ToIp(cur_end);

      lease.start_ip.addr = (uint32_t)active_start;
      lease.end_ip.addr   = (uint32_t)active_end;

      esp_err_t opt_err = esp_netif_dhcps_option(
        ap_netif,
        ESP_NETIF_OP_SET,
        ESP_NETIF_REQUESTED_IP_ADDRESS,
        &lease,
        sizeof(dhcps_lease_t)
      );
      if (opt_err != ESP_OK) {
        logPrintf("[DHCP] Set dai IP (%s -> %s) that bai: %d", 
                  active_start.toString().c_str(), active_end.toString().c_str(), opt_err);
      } else {
        logPrintf("[DHCP] Da chon ngau nhien pool cap IP: %s -> %s (trong tong dai %s -> %s)",
                  active_start.toString().c_str(), active_end.toString().c_str(),
                  dhcp_start_str.c_str(), dhcp_end_str.c_str());
      }
    }

    // Cấu hình cấp phát DNS Option cho Client kết nối vào SoftAP
    dhcps_offer_t dhcps_dns_value = OFFER_DNS;
    esp_netif_dhcps_option(
      ap_netif,
      ESP_NETIF_OP_SET,
      ESP_NETIF_DOMAIN_NAME_SERVER,
      &dhcps_dns_value,
      sizeof(dhcps_dns_value)
    );

    esp_netif_dns_info_t dns_main, dns_backup;
    dns_main.ip.type = IPADDR_TYPE_V4;
    dns_backup.ip.type = IPADDR_TYPE_V4;

    if (WiFi.status() == WL_CONNECTED) {
      IPAddress staDns = WiFi.dnsIP(0);
      if (staDns == IPAddress(0, 0, 0, 0)) {
        staDns = IPAddress(8, 8, 8, 8);
      }
      dns_main.ip.u_addr.ip4.addr = (uint32_t)staDns;
      dns_backup.ip.u_addr.ip4.addr = (staDns == IPAddress(8, 8, 8, 8)) ? (uint32_t)IPAddress(1, 1, 1, 1) : (uint32_t)IPAddress(8, 8, 8, 8);
    } else {
      IPAddress ap_ip;
      ap_ip.fromString(ap_ip_str);
      dns_main.ip.u_addr.ip4.addr = (uint32_t)ap_ip;
      dns_backup.ip.u_addr.ip4.addr = (uint32_t)IPAddress(8, 8, 8, 8);
    }
    esp_netif_set_dns_info(ap_netif, ESP_NETIF_DNS_MAIN, &dns_main);
    esp_netif_set_dns_info(ap_netif, ESP_NETIF_DNS_BACKUP, &dns_backup);

    esp_err_t start_err = esp_netif_dhcps_start(ap_netif);
    logPrintf("[DHCP] Khoi chay Native ESP-IDF DHCP Server! (Ket qua: %d)", start_err);
  } else {
    logMessage("[DHCP-LOI] Khong tim thay netif WIFI_AP_DEF!");
  }
}

// Lắng nghe sự kiện ngắt kết nối Wi-Fi để đổi pool IP ngẫu nhiên lần tới
void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  if (event == ARDUINO_EVENT_WIFI_AP_STADISCONNECTED) {
    uint8_t *mac = info.wifi_ap_stadisconnected.mac;
    logPrintf("[WIFI-AP] Thiet bi ngat ket noi: %02X:%02X:%02X:%02X:%02X:%02X",
              mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    dhcp_reconfigure_needed = true;
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
  if (WiFi.status() != WL_CONNECTED) {
    String host = server.hostHeader();
    if (host != ap_ip_str) {
      server.sendHeader("Location", String("http://") + ap_ip_str + "/", true);
      server.send(302, "text/plain", "");
      return;
    }
  }
  handleRoot();
}

// API: Trả về trạng thái hệ thống dạng JSON
void handleApiStatus() {
  IPAddress sta_ip = WiFi.localIP();
  IPAddress sta_gw = WiFi.gatewayIP();
  int rssi = WiFi.RSSI();
  bool connected = (WiFi.status() == WL_CONNECTED);

  char pinBuf[10];
  sprintf(pinBuf, "%06u", ble_pin);

  String json = "{";
  json += "\"connected\":" + String(connected ? "true" : "false") + ",";
  json += "\"sta_ssid\":\"" + sta_ssid + "\",";
  json += "\"sta_ip\":\"" + (connected ? sta_ip.toString() : "") + "\",";
  json += "\"sta_gw\":\"" + (connected ? sta_gw.toString() : "") + "\",";
  json += "\"sta_rssi\":" + String(rssi) + ",";
  json += "\"ap_ssid\":\"" + ap_ssid + "\",";
  json += "\"ap_bssid\":\"" + ap_bssid_str + "\",";
  json += "\"ap_ip\":\"" + ap_ip_str + "\",";
  json += "\"ap_subnet\":\"" + ap_subnet_str + "\",";
  json += "\"ap_channel\":" + String(ap_channel) + ",";
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
  json += "\"uptime\":" + String(millis() / 1000);
  json += "}";

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
    server.send(200, "application/json", json);
  } else {
    server.send(400, "application/json", "{\"error\":\"Thieu tham so boost\"}");
  }
}

// API: Cài đặt Bluetooth LE (Bật/tắt sóng và đổi mã PIN)
void handleApiSaveBle() {
  if (server.hasArg("pin")) {
    String p = server.arg("pin");
    p.trim();
    if (p.length() == 6 && isValidPin(p)) {
      ble_pin = p.toInt();
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
  }

  char pinStr[10];
  sprintf(pinStr, "%06u", ble_pin);
  String json = "{";
  json += "\"success\":true,";
  json += "\"ble_enabled\":" + String(ble_enabled ? "true" : "false") + ",";
  json += "\"ble_pin\":\"" + String(pinStr) + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

// API: Lấy danh sách các dòng log gần nhất
void handleApiLogs() {
  String json = "[";
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
  server.send(200, "application/json", json);
}

// API: Xóa bộ đệm log
void handleApiClearLogs() {
  logCount = 0;
  logHead = 0;
  server.send(200, "application/json", "{\"success\":true}");
}

// API: Quét các mạng Wi-Fi 2.4GHz xung quanh
void handleApiScan() {
  logMessage("[SCAN] Dang thuc hien quet song Wi-Fi 2.4GHz...");
  int n = WiFi.scanNetworks(false, false, false, 250);
  String json = "[";
  for (int i = 0; i < n; i++) {
    if (i > 0) json += ",";
    json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
    json += "\"ch\":" + String(WiFi.channel(i)) + ",";
    json += "\"secure\":" + String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "true" : "false") + "}";
  }
  json += "]";
  WiFi.scanDelete();
  server.send(200, "application/json", json);
}

// API: Lưu cấu hình Wi-Fi nguồn (STA) và bắt đầu kết nối
void handleApiSaveSta() {
  if (server.hasArg("ssid")) {
    sta_ssid = server.arg("ssid");
    sta_password = server.arg("pass");

    prefs.begin("gateway", false);
    prefs.putString("sta_ssid", sta_ssid);
    prefs.putString("sta_pass", sta_password);
    prefs.end();

    logPrintf("[STA] Da luu Wi-Fi nguon moi: %s. Dang ket noi lai...", sta_ssid.c_str());

    WiFi.disconnect();
    delay(100);
    WiFi.begin(sta_ssid.c_str(), (sta_password.length() > 0 ? sta_password.c_str() : NULL));

    server.send(200, "application/json", "{\"success\":true,\"msg\":\"Da luu Wi-Fi nguon! ESP32 dang ket noi lai...\"}");
  } else {
    server.send(400, "application/json", "{\"error\":\"Thieu SSID\"}");
  }
}

// API: Lưu các tham số mạng phát (SoftAP) và khởi động lại
void handleApiSaveAp() {
  if (server.hasArg("ssid") && server.hasArg("ip") && server.hasArg("subnet")) {
    ap_ssid = server.arg("ssid");
    ap_password = server.arg("pass");
    if (server.hasArg("channel")) ap_channel = server.arg("channel").toInt();
    if (server.hasArg("bssid")) ap_bssid_str = server.arg("bssid");
    ap_ip_str = server.arg("ip");
    ap_subnet_str = server.arg("subnet");
    if (server.hasArg("dhcp_start")) dhcp_start_str = server.arg("dhcp_start");
    if (server.hasArg("dhcp_end")) dhcp_end_str = server.arg("dhcp_end");

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

// API: Kiểm tra bản cập nhật mới từ GitHub qua HTTPS
void handleApiCheckOta() {
  String newVer = "";
  String binUrl = "";
  String changelog = "";
  String errMsg = "";

  bool ok = checkGithubOta(newVer, binUrl, changelog, errMsg);
  if (!ok) {
    String json = "{\"success\":false,\"error\":\"" + errMsg + "\"}";
    server.send(200, "application/json", json);
    return;
  }

  bool hasUpdate = (newVer != FIRMWARE_VERSION);
  String json = "{";
  json += "\"success\":true,";
  json += "\"current_ver\":\"" + FIRMWARE_VERSION + "\",";
  json += "\"remote_ver\":\"" + newVer + "\",";
  json += "\"has_update\":" + String(hasUpdate ? "true" : "false") + ",";
  json += "\"bin_url\":\"" + binUrl + "\",";
  json += "\"changelog\":\"" + changelog + "\"";
  json += "}";

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
      server.send(200, "application/json", json);
      return;
    }
  }
  server.send(400, "application/json", "{\"error\":\"URL khong hop le!\"}");
}

// =========================================================================
//                               SETUP & LOOP
// =========================================================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- [KHOI TAO ESP32 REPEATER & CUSTOM RANDOM DHCP & BLE SECURE] ---");

  // Đăng ký bắt sự kiện Wi-Fi ngắt kết nối
  WiFi.onEvent(onWiFiEvent);

  // Bước 1: Tải thông số từ bộ nhớ Flash NVS
  prefs.begin("gateway", false);
  sta_ssid       = prefs.getString("sta_ssid", "BerProduction");
  sta_password   = prefs.getString("sta_pass", "a9131289");
  ap_ssid        = prefs.getString("ap_ssid", "ODE - wireless WiFi");
  ap_password    = prefs.getString("ap_pass", "Ode6868@");
  ap_channel     = prefs.getUChar("ap_ch", 6);
  if (ap_channel < 1 || ap_channel > 11) {
    ap_channel = 6;
    prefs.begin("gateway", false);
    prefs.putUChar("ap_ch", 6);
    prefs.end();
  }
  ap_bssid_str   = prefs.getString("ap_bssid", "84:F1:47:4F:27:A0");
  ap_ip_str      = prefs.getString("ap_ip", "10.51.31.254");
  ap_subnet_str  = prefs.getString("ap_subnet", "255.255.252.0");
  dhcp_start_str = prefs.getString("dhcp_start", "10.51.28.50");
  dhcp_end_str   = prefs.getString("dhcp_end", "10.51.31.100"); // 819 IP ngẫu nhiên!
  tx_power_boost = prefs.getBool("tx_boost", true);             // Mặc định bật 20 dBm Max Boost
  ble_enabled     = prefs.getBool("ble_en", true);               // Bật sóng Bluetooth LE
  ble_pin         = prefs.getUInt("ble_pin", 686868);            // Mã PIN ghép đôi 6 số
  ota_version_url = prefs.getString("ota_url", "https://raw.githubusercontent.com/silversymphony/esp32-firmware/main/version.json");
  prefs.end();

  // Bước 2: Khởi động Bluetooth LE Console & Provisioning Server (Bảo mật mã PIN phần cứng)
  initBleServer();

  // Bước 3: Kích hoạt chế độ kép Wi-Fi (STA + AP)
  WiFi.mode(WIFI_AP_STA);

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

  // Bước 5: Cấu hình Gateway và Subnet Mask cho mạng phát SoftAP
  IPAddress ap_ip, gateway, subnet;
  ap_ip.fromString(ap_ip_str);
  gateway.fromString(ap_ip_str);
  subnet.fromString(ap_subnet_str);

  WiFi.softAPConfig(ap_ip, gateway, subnet);
  WiFi.softAP(ap_ssid.c_str(), (ap_password.length() > 0 ? ap_password.c_str() : NULL), ap_channel, 0, 4);

  // Bước 6: Áp dụng công suất phát sóng Wi-Fi (Max TX Power Boost)
  applyTxPower(tx_power_boost);

  // Bước 7: Cấu hình dải IP cho Native ESP-IDF DHCP Server
  configureSoftApDhcp();

  logPrintf("[AP] Phat song thanh cong: SSID: '%s' | MAC: %s | Gateway: %s", 
            ap_ssid.c_str(), WiFi.softAPmacAddress().c_str(), WiFi.softAPIP().toString().c_str());

  // Bước 8: Đăng ký các Endpoint cho Web Server
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleApiStatus);
  server.on("/api/power_boost", HTTP_POST, handleApiPowerBoost);
  server.on("/api/save_ble", HTTP_POST, handleApiSaveBle);
  server.on("/api/logs", HTTP_GET, handleApiLogs);
  server.on("/api/clear_logs", HTTP_POST, handleApiClearLogs);
  server.on("/api/scan", HTTP_GET, handleApiScan);
  server.on("/api/save_sta", HTTP_POST, handleApiSaveSta);
  server.on("/api/save_ap", HTTP_POST, handleApiSaveAp);
  server.on("/api/check_ota", HTTP_POST, handleApiCheckOta);
  server.on("/api/run_ota", HTTP_POST, handleApiRunOta);
  server.on("/api/save_ota_url", HTTP_POST, handleApiSaveOtaUrl);

  // Endpoint bẫy Captive Portal (iOS, Android, Windows)
  server.on("/hotspot-detect.html", HTTP_GET, handleCaptiveRedirect); // Apple
  server.on("/generate_204", HTTP_GET, handleCaptiveRedirect);       // Android
  server.on("/gen_204", HTTP_GET, handleCaptiveRedirect);            // Android
  server.on("/connecttest.txt", HTTP_GET, handleCaptiveRedirect);    // Windows
  server.on("/ncsi.txt", HTTP_GET, handleCaptiveRedirect);           // Windows
  server.onNotFound(handleCaptiveRedirect);

  server.begin();
  logMessage("[WEB] Web Server cong 80 da san sang!");

  // Bước 9: Kết nối vào Wi-Fi nguồn (STA)
  if (sta_ssid.length() > 0) {
    logPrintf("[STA] Dang ket noi vao mang nguon: %s...", sta_ssid.c_str());
    WiFi.begin(sta_ssid.c_str(), (sta_password.length() > 0 ? sta_password.c_str() : NULL));

    unsigned long start_time = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start_time < 10000) {
      delay(400);
      Serial.print(".");
    }
    Serial.println();
  }

  // Bước 10: Kiểm tra trạng thái Internet và thiết lập NAT hoặc Captive Portal
  if (WiFi.status() == WL_CONNECTED) {
    logPrintf("[STA] Da ket noi Internet thanh cong! IP nhan: %s", WiFi.localIP().toString().c_str());
    enableNapt();
    stopCaptiveDns();
  } else {
    logMessage("[CANH BAO] Chua ket noi duoc Wi-Fi nguon. Kich hoat Captive Portal!");
    startCaptiveDns();
  }

  logMessage("===> HE THONG GATEWAY + BLE SECURE + RANDOM DHCP DA SAN SANG! <===");
}

void loop() {
  // 1. Phục vụ các yêu cầu Web Server
  server.handleClient();

  // 2. Cập nhật ngẫu nhiên dải IP DHCP khi có client ngắt kết nối
  if (dhcp_reconfigure_needed) {
    dhcp_reconfigure_needed = false;
    configureSoftApDhcp();
  }

  // 3. Phục vụ Captive DNS Server nếu đang chạy
  if (dns_captive_running) {
    dnsServer.processNextRequest();
  }

  // 4. Giám sát kết nối BLE và tự động quảng bá lại khi điện thoại ngắt kết nối
  if (ble_enabled) {
    if (!bleConnected && oldBleConnected) {
      delay(200);
      BLEDevice::startAdvertising();
      logMessage("[BLE] Phat lai quang ba BLE tim thiet bi...");
      oldBleConnected = bleConnected;
    }
    if (bleConnected && !oldBleConnected) {
      oldBleConnected = bleConnected;
    }
  }

  // 5. Giám sát trạng thái kết nối STA định kỳ mỗi 3 giây
  if (millis() - last_sta_check > 3000) {
    last_sta_check = millis();

    if (WiFi.status() == WL_CONNECTED) {
      if (!napt_enabled) {
        logMessage("[LOOP] STA da ket noi lai! Bat NAPT va tat Captive Portal.");
        enableNapt();
        stopCaptiveDns();
      }
    } else {
      if (napt_enabled || !dns_captive_running) {
        logMessage("[LOOP] STA mat ket noi! Tat NAPT va bat Captive Portal.");
        disableNapt();
        startCaptiveDns();
      }
    }
  }

  // 6. Thực hiện OTA Firmware nếu có lệnh từ Web hoặc BLE
  if (pending_ota) {
    pending_ota = false;
    logMessage("[OTA] Tam dung cac tien trinh mang de uu tien ghi Flash...");
    delay(500); // Đảm bảo gói tin phản hồi HTTP 200 đã bay xong
    performGithubOta(pending_ota_bin_url);
  }

  // 7. Nhận và xử lý lệnh từ cổng USB Serial Monitor (Tương thích 100% với Bluetooth LE)
  if (Serial.available()) {
    String serialCmd = Serial.readStringUntil('\n');
    handleBleCommand(serialCmd);
  }

  // 8. Khởi động lại sau khi lưu cấu hình AP
  if (should_reboot && millis() >= reboot_time) {
    logMessage("[REBOOT] Dang khoi dong lai ESP32...");
    delay(100);
    ESP.restart();
  }
}
