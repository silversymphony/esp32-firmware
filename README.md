# 📡 ESP32 Gateway & Wi-Fi Extender Firmware

Dự án firmware quản trị mạng trung gian và chuyển tiếp Wi-Fi cho ESP32. Tích hợp Web Portal nội bộ, BLE Terminal bảo mật, cơ chế định tuyến NAPT LwIP, DHCP Server ngẫu nhiên dải subnet `/22` (`10.51.28.x`), và tính năng tự động cập nhật Firmware không dây qua GitHub HTTPS OTA.

---

## 🚀 Phiên Bản Mới Nhất: `v1.3.5` (Khuyến Nghị)

- **Firmware Binary**: [`esp32_v1.3.5/esp32_v1.3.5.ino.bin`](esp32_v1.3.5/esp32_v1.3.5.ino.bin)
- **Mã nguồn**: [`esp32_v1.3.5/esp32_v1.3.5.ino`](esp32_v1.3.5/esp32_v1.3.5.ino)
- **Điểm nổi bật v1.3.5**:
  - ✅ **5G Cellular Fallback**: Điện thoại tự động dùng mạng di động 4G/5G khi tắt Wi-Fi nguồn mà vẫn truy cập Web Portal nội bộ bình thường (DHCP không cấp Gateway/DNS ảo).
  - ✅ **Async Wi-Fi Scan**: Rút ngắn dwell time quét mạng xuống 40ms/kênh, không ngắt kết nối STA, chống rớt beacon giúp không bao giờ văng điện thoại khỏi Wi-Fi AP khi bấm quét mạng.
  - ✅ **Mở rộng Log System**: Bộ đệm log 120 dòng thời gian thực, đồng bộ 100% dữ liệu BLE Terminal và Serial về Web Portal.
  - ✅ **GitHub OTA Timeout & Dual DNS**: Tăng timeout socket lên 15s và bổ sung DNS Google (`8.8.8.8`) / Cloudflare (`1.1.1.1`) khắc phục triệt để lỗi kết nối khi tải firmware từ GitHub.
  - ✅ **Tối ưu hóa RAM/CPU**: Loại bỏ hoàn toàn Web Remote đám mây, tập trung tài nguyên cho Gateway nội bộ và BLE bảo mật.

---

## 📂 Danh Mục Cấu Trúc Các Phiên Bản (Version History)

| Thư mục | Phiên bản | Đặc điểm chính |
| :--- | :---: | :--- |
| [`esp32_v1.3.5/`](esp32_v1.3.5/) | **v1.3.5** | Bản mới nhất: Tối ưu 5G fallback, async Wi-Fi scan, 120 dòng log, GitHub OTA robust |
| [`esp32_v1.3.4/`](esp32_v1.3.4/) | **v1.3.4** | Bật NAPT LwIP trực tiếp, Web Remote đồng bộ 19 API endpoints với Cloudflare |
| [`esp32_v1.3.3/`](esp32_v1.3.3/) | **v1.3.3** | Chuẩn hóa DHCP Option 3 & 6, tắt Modem Sleep (`WIFI_PS_NONE`) chống rớt sóng AP |
| [`esp32_v1.3.2/`](esp32_v1.3.2/) | **v1.3.2** | Sửa Captive DNS, hỗ trợ cấu hình Wrangler (`wrangler.json`, `wrangler.toml`) |
| [`esp32_v1.3.1/`](esp32_v1.3.1/) | **v1.3.1** | Wi-Fi Static Buffers, sửa lỗi tràn bộ nhớ SA Query khi tải OTA |
| [`esp32_v1.3.0/`](esp32_v1.3.0/) | **v1.3.0** | Bản đầu tiên tích hợp Web Remote qua Cloudflare Worker |
| [`esp32_v1.2.0/`](esp32_v1.2.0/) | **v1.2.0** | Nâng cấp giao diện Web Portal nội bộ 4 Tabs |
| [`esp32_stable_v1.0.0/`](esp32_stable_v1.0.0/) | **v1.0.0** | Bản sơ khai ban đầu ổn định |

---

## ⚙️ Thông Số Mặc Định Hệ Thống (v1.3.5)

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

### Nạp nhanh file Binary (`.bin`) qua `esptool`:
```bash
esptool.py --chip esp32 --port /dev/cu.usbserial-0001 --baud 921600 \
  write_flash -z 0x10000 esp32_v1.3.5/esp32_v1.3.5.ino.bin
```

### Biên dịch và nạp từ mã nguồn qua `arduino-cli`:
```bash
arduino-cli compile --fqbn esp32:esp32:esp32 esp32_v1.3.5
arduino-cli upload -p /dev/cu.usbserial-0001 --fqbn esp32:esp32:esp32 esp32_v1.3.5
```
