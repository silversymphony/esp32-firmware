# 📡 ESP32 Gateway & Wi-Fi Extender Firmware

Dự án firmware quản trị mạng trung gian và chuyển tiếp Wi-Fi cho ESP32. Tích hợp Web Portal nội bộ, BLE Terminal bảo mật, cơ chế định tuyến NAPT LwIP, DHCP Server ngẫu nhiên dải subnet `/22` (`10.51.28.x`), và tính năng tự động cập nhật Firmware không dây qua GitHub HTTPS OTA.

---

## 🚀 Phiên Bản Mới Nhất: `v1.3.6` (Khuyến Nghị)

- **Firmware Binary**: [`esp32_v1.3.6/esp32_v1.3.6.ino.bin`](esp32_v1.3.6/esp32_v1.3.6.ino.bin)
- **Mã nguồn**: [`esp32_v1.3.6/esp32_v1.3.6.ino`](esp32_v1.3.6/esp32_v1.3.6.ino)
- **Điểm nổi bật v1.3.6**:
  - ✅ **Chuyển đổi Bật/Tắt Wi-Fi nguồn an toàn**: Cơ chế hoãn 300ms đảm bảo HTTP 200 gửi xong trước khi đổi kênh sóng, khóa nút gạt chống spam lệnh, thay alert modal bằng toast mượt mà.
  - ✅ **Terminal Log cuộn thông minh**: Chỉ tự động cuộn xuống đáy khi người dùng đang ở cuối terminal; nếu người dùng kéo lên xem nhật ký cũ, vị trí xem sẽ được giữ nguyên 100% không bị nhảy giật; chỉ cập nhật DOM khi có dòng mới.
  - ✅ **Kiểm tra OTA bất đồng bộ qua FreeRTOS Task (Core 0)**: Tách tiến trình TLS/HTTPS sang tác vụ nền độc lập, vòng lặp `loop()` trên Core 1 không bị đóng băng, WebServer phản hồi request polling ngay lập tức, khắc phục triệt để lỗi rớt mạng/văng Wi-Fi AP.
  - ✅ **DNS kép dự phòng**: Giữ DNS router (DHCP) làm chính, bổ sung Google DNS (`8.8.8.8`) và Cloudflare (`1.1.1.1`) làm dự phòng, phân giải domain GitHub ổn định qua mọi nhà mạng.
  - ✅ **Bảo toàn cấu hình Wi-Fi nguồn**: Khắc phục triệt để lỗi xóa nhầm SSID khi khởi động lại.

---

## 📂 Danh Mục Cấu Trúc Các Phiên Bản (Version History)

| Thư mục | Phiên bản | Đặc điểm chính |
| :--- | :---: | :--- |
| [`esp32_v1.3.6/`](esp32_v1.3.6/) | **v1.3.6** | Bản mới nhất: Chuyển đổi STA an toàn, Terminal Log cuộn thông minh, FreeRTOS OTA background task |
| [`esp32_v1.3.5/`](esp32_v1.3.5/) | **v1.3.5** | Tối ưu 5G fallback, async Wi-Fi scan, 120 dòng log, loại bỏ Web Remote |
| [`esp32_v1.3.4/`](esp32_v1.3.4/) | **v1.3.4** | Bật NAPT LwIP trực tiếp, Web Remote đồng bộ 19 API endpoints với Cloudflare |
| [`esp32_v1.3.3/`](esp32_v1.3.3/) | **v1.3.3** | Chuẩn hóa DHCP Option 3 & 6, tắt Modem Sleep (`WIFI_PS_NONE`) chống rớt sóng AP |
| [`esp32_v1.3.2/`](esp32_v1.3.2/) | **v1.3.2** | Sửa Captive DNS, hỗ trợ cấu hình Wrangler (`wrangler.json`, `wrangler.toml`) |
| [`esp32_v1.3.1/`](esp32_v1.3.1/) | **v1.3.1** | Wi-Fi Static Buffers, sửa lỗi tràn bộ nhớ SA Query khi tải OTA |
| [`esp32_v1.3.0/`](esp32_v1.3.0/) | **v1.3.0** | Bản đầu tiên tích hợp Web Remote qua Cloudflare Worker |
| [`esp32_v1.2.0/`](esp32_v1.2.0/) | **v1.2.0** | Nâng cấp giao diện Web Portal nội bộ 4 Tabs |
| [`esp32_stable_v1.0.0/`](esp32_stable_v1.0.0/) | **v1.0.0** | Bản sơ khai ban đầu ổn định |

---

## ⚙️ Thông Số Mặc Định Hệ Thống (v1.3.6)

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
  write_flash -z 0x10000 esp32_v1.3.6/esp32_v1.3.6.ino.bin
```

### Biên dịch và nạp từ mã nguồn qua `arduino-cli`:
```bash
arduino-cli compile --fqbn esp32:esp32:esp32:PartitionScheme=min_spiffs esp32_v1.3.6
arduino-cli upload -p /dev/cu.usbserial-0001 --fqbn esp32:esp32:esp32:PartitionScheme=min_spiffs esp32_v1.3.6
```
