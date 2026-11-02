# 📡 ESP32 Gateway & Wi-Fi Extender Firmware

Dự án firmware quản trị mạng trung gian và chuyển tiếp Wi-Fi cho ESP32. Tích hợp Web Portal nội bộ, BLE Terminal bảo mật, cơ chế định tuyến NAPT LwIP, DHCP Server ngẫu nhiên dải subnet `/22` (`10.51.28.x`), và tính năng tự động cập nhật Firmware không dây qua GitHub HTTPS OTA.

---

## 🚀 Phiên Bản Mới Nhất: `v1.3.8` (Khuyến Nghị)

- **Firmware Binary**: [`esp32_v1.3.8/esp32_v1.3.8.ino.bin`](esp32_v1.3.8/esp32_v1.3.8.ino.bin)
- **Mã nguồn**: [`esp32_v1.3.8/esp32_v1.3.8.ino`](esp32_v1.3.8/esp32_v1.3.8.ino)
- **Điểm nổi bật v1.3.8**:
  - ✅ **Sửa dứt điểm lỗi kiểm tra OTA (`connection refused -1`)**: Cố định gateway SoftAP `gw = 0.0.0.0` để LwIP không gửi quẩn gói tin TCP ra chính ESP32; bảo toàn DNS router nhận từ DHCP trong `enableNapt()`; gán `sta_netif` làm default netif khi mở socket HTTPS.
  - ✅ **Tích hợp Cloudflare CDN Mirror (`cdn.jsdelivr.net`)**: Kết nối tải siêu tốc qua Cloudflare CDN Edge, không bị chặn SNI tại Việt Nam, cơ chế tự động chuyển đổi qua lại giữa GitHub raw và CDN mirror.
  - ✅ **Đổi mã PIN Bluetooth LE mặc định thành `123456`**: Dễ nhớ, bảo mật theo chuẩn ghép đôi phần cứng 6 số.
  - ✅ **Mặc định khởi động lần đầu**: TẮT Wi-Fi nguồn, không lưu sẵn cấu hình Wi-Fi nguồn (người dùng chủ động cấu hình qua Web Portal hoặc BLE).
  - ✅ **Bảo toàn 100% cấu hình mạng**: SoftAP SSID `ODE - wireless WiFi`, mật khẩu `Ode6868@`, IP `10.51.31.254`, Subnet `/22`, dải DHCP ngẫu nhiên `10.51.28.10` - `10.51.28.200`.

---

## 📂 Danh Mục Cấu Trúc Các Phiên Bản (Version History)

| Thư mục | Phiên bản | Đặc điểm chính |
| :--- | :---: | :--- |
| [`esp32_v1.3.8/`](esp32_v1.3.8/) | **v1.3.8** | **Bản mới nhất**: Sửa dứt điểm OTA connection refused (-1), CDN Mirror Cloudflare, PIN BLE 123456, tắt Wi-Fi nguồn mặc định |
| [`esp32_v1.3.7/`](esp32_v1.3.7/) | **v1.3.7** | Hiển thị tên BLE, LED xanh GPIO 2 báo STA, mặc định tắt STA, chống spam log |
| [`esp32_v1.3.6/`](esp32_v1.3.6/) | **v1.3.6** | Chuyển đổi STA an toàn, Terminal Log cuộn thông minh, FreeRTOS OTA background task Core 0 |
| [`esp32_v1.3.5/`](esp32_v1.3.5/) | **v1.3.5** | Tối ưu 5G fallback, async Wi-Fi scan, 120 dòng log, loại bỏ Web Remote |
| [`esp32_v1.3.4/`](esp32_v1.3.4/) | **v1.3.4** | Bật NAPT LwIP trực tiếp, Web Remote đồng bộ 19 API endpoints với Cloudflare |
| [`esp32_v1.3.3/`](esp32_v1.3.3/) | **v1.3.3** | Chuẩn hóa DHCP Option 3 & 6, tắt Modem Sleep (`WIFI_PS_NONE`) chống rớt sóng AP |
| [`esp32_v1.3.2/`](esp32_v1.3.2/) | **v1.3.2** | Sửa Captive DNS, hỗ trợ cấu hình Wrangler (`wrangler.json`, `wrangler.toml`) |
| [`esp32_v1.3.1/`](esp32_v1.3.1/) | **v1.3.1** | Wi-Fi Static Buffers, sửa lỗi tràn bộ nhớ SA Query khi tải OTA |
| [`esp32_v1.3.0/`](esp32_v1.3.0/) | **v1.3.0** | Bản đầu tiên tích hợp Web Remote qua Cloudflare Worker |
| [`esp32_v1.2.0/`](esp32_v1.2.0/) | **v1.2.0** | Nâng cấp giao diện Web Portal nội bộ 4 Tabs |
| [`esp32_stable_v1.0.0/`](esp32_stable_v1.0.0/) | **v1.0.0** | Bản sơ khai ban đầu ổn định |

---

## ⚙️ Thông Số Mặc Định Hệ Thống (v1.3.8)

| Thông số | Giá trị mặc định |
| :--- | :--- |
| **Wi-Fi AP SSID** | `ODE - wireless WiFi` |
| **Mật khẩu AP** | `Ode6868@` |
| **Kênh phát sóng AP** | Kênh `6` (hoặc tự động theo Wi-Fi nguồn) |
| **Địa chỉ Gateway Web Portal** | `http://10.51.31.254` |
| **Dải cấp phát IP (DHCP)** | `10.51.28.10` – `10.51.28.200` (Subnet `255.255.252.0` / `/22`) |
| **Mã PIN bảo mật BLE Terminal** | `123456` |
| **Đường dẫn kiểm tra OTA** | `https://cdn.jsdelivr.net/gh/silversymphony/esp32-firmware@main/version.json` |

---

## 🛠️ Hướng Dẫn Nạp Firmware

### Biên dịch và nạp trực tiếp qua Arduino CLI:
```bash
# 1. Biên dịch firmware
"/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli" compile \
  -b esp32:esp32:esp32:PartitionScheme=min_spiffs \
  esp32_v1.3.7/esp32_v1.3.7.ino \
  --output-dir esp32_v1.3.7/build

# 2. Nạp vào cổng USB Serial (macOS)
"/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli" upload \
  -p /dev/cu.usbserial-0001 \
  -b esp32:esp32:esp32:PartitionScheme=min_spiffs \
  --input-dir esp32_v1.3.7/build
```
