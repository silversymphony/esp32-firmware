# 📡 ESP32 Gateway & Wi-Fi Extender Firmware

Dự án firmware quản trị mạng trung gian và chuyển tiếp Wi-Fi cho ESP32. Tích hợp Web Portal nội bộ, BLE Terminal bảo mật, cơ chế định tuyến NAPT LwIP, DHCP Server ngẫu nhiên dải subnet `/22` (`10.51.28.x`), và tính năng tự động cập nhật Firmware không dây qua GitHub HTTPS OTA.

---

## 🚀 Phiên Bản Mới Nhất: `v1.3.7` (Khuyến Nghị)

- **Firmware Binary**: [`esp32_v1.3.7/esp32_v1.3.7.ino.bin`](esp32_v1.3.7/esp32_v1.3.7.ino.bin)
- **Mã nguồn**: [`esp32_v1.3.7/esp32_v1.3.7.ino`](esp32_v1.3.7/esp32_v1.3.7.ino)
- **Điểm nổi bật v1.3.7**:
  - ✅ **Hiển thị đầy đủ tên Bluetooth LE (`ESP32-Gateway-BLE`)**: Đóng gói tên trực tiếp vào gói tin quảng bá chính (Primary Advertisement Data) cùng cờ `0x06` giúp mọi điện thoại (Android, Vivo, iPhone) nhận diện và hiển thị rõ tên mạch ngay khi quét.
  - ✅ **Đèn LED xanh chỉ báo Wi-Fi nguồn (GPIO 2)**: Tắt hẳn khi Wi-Fi nguồn TẮT (`sta:off`); Sáng liên tục khi ĐÃ KẾT NỐI; Nhấp nháy 1 giây/lần khi ĐANG BẬT nhưng chưa có kết nối (đang dò tìm hoặc mất sóng).
  - ✅ **Mặc định khởi động lần đầu TẮT Wi-Fi nguồn**: Tối ưu cho chế độ Mạng nội bộ Offline dùng 5G; người dùng chỉ bật khi cần kết nối Wi-Fi ngoài.
  - ✅ **Chống spam log mất kết nối**: Chốt cờ trạng thái kết nối; chỉ báo `[WIFI-STA] Mat ket noi` 1 lần duy nhất khi thực sự bị rớt mạng, không spam lặp lại trong chu kỳ quét/thử lại thất bại.
  - ✅ **Khắc phục triệt để lỗi kiểm tra OTA (`connection refused -1`)**: Bảo toàn 100% DNS nhận từ DHCP của router làm DNS chính (`slot 0`), bổ sung Cloudflare `1.1.1.1` làm dự phòng (`slot 1`); kiểm tra xác thực phân giải DNS domain GitHub trước khi kết nối; mở rộng stack task FreeRTOS lên 16 KB.

---

## 📂 Danh Mục Cấu Trúc Các Phiên Bản (Version History)

| Thư mục | Phiên bản | Đặc điểm chính |
| :--- | :---: | :--- |
| [`esp32_v1.3.7/`](esp32_v1.3.7/) | **v1.3.7** | **Bản mới nhất**: Hiển thị tên BLE, LED xanh GPIO 2 báo STA, mặc định tắt STA, chống spam log, sửa triệt để OTA Check |
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

## ⚙️ Thông Số Mặc Định Hệ Thống (v1.3.7)

| Thông số | Giá trị mặc định |
| :--- | :--- |
| **Wi-Fi AP SSID** | `ODE - wireless WiFi` |
| **Mật khẩu AP** | `Ode6868@` |
| **Kênh phát sóng AP** | Kênh `6` (hoặc tự động theo Wi-Fi nguồn) |
| **Địa chỉ Gateway Web Portal** | `http://10.51.31.254` |
| **Dải cấp phát IP (DHCP)** | `10.51.28.10` – `10.51.28.200` (Subnet `255.255.252.0` / `/22`) |
| **Mã PIN bảo mật BLE Terminal** | `070213` |
| **Đường dẫn kiểm tra OTA** | `https://raw.githubusercontent.com/silversymphony/esp32-firmware/main/version.json` |

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
