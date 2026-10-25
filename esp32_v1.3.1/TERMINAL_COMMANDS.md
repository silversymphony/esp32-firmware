# TÀI LIỆU DANH SÁCH LỆNH CẤU HÌNH TERMINAL (BLE & SERIAL CONSOLE)
> **ESP32 Wi-Fi Repeater Gateway • Hardware BLE Passkey & GitHub Cloud OTA**  
> *Áp dụng đồng thời cho: Cổng nạp USB Serial (115200 baud) & Console Không Dây Bluetooth LE (Nordic UART Service)*

---

## 1. TỔNG QUAN TẬP LỆNH

Tất cả các thông số cấu hình trên Web Portal đều có thể cài đặt trực tiếp thông qua giao diện Terminal dòng lệnh. Mọi thay đổi sẽ được **lưu vĩnh viễn vào bộ nhớ Flash NVS** (`Preferences`) và tự động nạp lại mỗi khi khởi động mạch.

| Nhóm | Lệnh | Cú pháp ví dụ | Chức năng |
| :--- | :--- | :--- | :--- |
| **Trợ giúp & Trạng thái** | `HELP` hoặc `?` | `HELP` | In ra danh sách toàn bộ các lệnh có sẵn |
| | `STATUS` | `STATUS` | Xem toàn bộ thông số mạng, IP, PIN, RAM, Uptime |
| **Wi-Fi Nguồn (STA)** | `SCAN` | `SCAN` | Quét tất cả mạng Wi-Fi 2.4GHz xung quanh |
| | `WIFI` | `WIFI:TenWifiNguon,MatKhau` | Kết nối vào Wi-Fi nguồn (để trống pass nếu mở) |
| | `STA` | `STA:ON` hoặc `STA:OFF` | Bật/tắt Wi-Fi nguồn (TẮT = Chế độ Mạng nội bộ để điện thoại dùng 5G) |
| **Mạng Phát (SoftAP)** | `AP` | `AP:ODE - wireless WiFi,Ode6868@` | Đổi Tên (SSID) và Mật khẩu Wi-Fi phát ra |
| | `APCH` | `APCH:6` hoặc `APCH:AUTO` | Đổi kênh phát (1 đến 13 hoặc 0: Auto) |
| | `AUTOCH` | `AUTOCH` | Quét & tự động chọn kênh tối ưu ít nhiễu nhất |
| | `PROFILES` | `PROFILES` | Xem danh sách mẫu cấu hình mạng phát (Presets) |
| | `PROFILE:APPLY` | `PROFILE:APPLY:p1` | Kích hoạt mẫu cấu hình và khởi động lại AP |
| | `PROFILE:UPDATE` | `PROFILE:UPDATE:p1` | Cập nhật mẫu đã chọn với thông số SoftAP hiện tại |
| | `PROFILE:SAVE` | `PROFILE:SAVE:QuanCafe` | Lưu cấu hình hiện tại thành mẫu mới |
| | `PROFILE:CLONE` | `PROFILE:CLONE` | Sao chép cấu hình Wi-Fi nguồn (STA) thành Preset AP |
| | `PROFILE:DEL` | `PROFILE:DEL:p2` | Xóa một mẫu cấu hình khỏi danh sách |
| | `APMAC` | `APMAC:84:F1:47:4F:27:A0` | Đổi địa chỉ BSSID (MAC) phần cứng mạng phát |
| | `APIP` | `APIP:10.51.31.254,255.255.252.0` | Đổi IP Gateway và Subnet Mask |
| | `DHCP` | `DHCP:10.51.28.10,10.51.28.200` | Cài đặt dải cấp IP ngẫu nhiên Custom DHCP (191 IP) |
| | `BOOST` | `BOOST:ON` hoặc `BOOST:OFF` | Bật/tắt phát sóng cực đại 20 dBm (~100mW) vs 15 dBm |
| **Quản Lý Từ Xa (Cloud)** | `CLOUD` | `CLOUD:ON` hoặc `CLOUD:OFF` | Bật/tắt tính năng kết nối Cloudflare Worker Relay |
| | `CLOUD:CONFIG` | `CLOUD:CONFIG:https://xxx.workers.dev,secret123` | Cài đặt địa chỉ Cloudflare Worker URL và Access Key |
| | `CLOUD:STATUS` | `CLOUD:STATUS` | Kiểm tra trạng thái kết nối Cloud Relay |
| **Bảo Mật Bluetooth** | `SETPIN` | `SETPIN:686868` | Đổi mã PIN phần cứng ghép đôi Bluetooth (đúng 6 số) |
| | `BLERADIO` | `BLERADIO:OFF` | Tắt sóng Bluetooth LE (tàng hình, tiết kiệm điện) |
| **GitHub Cloud OTA** | `OTAURL` | `OTAURL:https://raw.githubusercontent.com/silversymphony/esp32-firmware/main/version.json` | Cài đặt đường dẫn kiểm tra phiên bản trên GitHub |
| | `OTA` | `OTA` | Kiểm tra phiên bản mới từ GitHub qua HTTPS |
| | `OTA:RUN` | `OTA:RUN` | Bắt đầu tải `esp32-firmware.bin` và ghi Flash |
| **Hệ Thống** | `CLEARLOGS` | `CLEARLOGS` hoặc `CLS` | Xóa sạch bộ đệm nhật ký hoạt động |
| | `REBOOT` | `REBOOT` | Khởi động lại vi điều khiển ESP32 |

---

## 2. CHI TIẾT CÁC LỆNH VÀ PHẢN HỒI MẪU

### 2.1. Nhóm Tra Cứu Trạng Thái & Quét Sóng
#### • `HELP` hoặc `?`
- **Mục đích:** Xem hướng dẫn và cú pháp nhanh của tất cả các lệnh.
- **Phản hồi:** Trả về menu lệnh phân theo 5 nhóm chức năng.

#### • `STATUS`
- **Mục đích:** Kiểm tra thông số toàn diện của ESP32.
- **Phản hồi mẫu:**
  ```text
  === TRANG THAI ESP32 GATEWAY ===
  Firmware: v1.1.1
  STA SSID: (Chua cau hinh)
  STA Pass: (Khong co mat khau)
  STA Status: CHUA KET NOI
  STA IP: 192.168.1.11
  STA Gateway: 192.168.1.1
  STA RSSI: -58 dBm
  -------------------------------
  AP SSID: ODE - wireless WiFi
  AP Pass: Ode6868@
  AP Gateway: 10.51.31.254
  AP Subnet: 255.255.252.0
  AP MAC: 84:F1:47:4F:27:A0
  AP Channel: Auto (Dang phat: 6)
  AP Presets: 2 mau da luu
  AP Clients: 2
  DHCP Pool: 10.51.28.10 -> 10.51.28.200 (Random)
  TX Power: 15 dBm (Standard)
  BLE PIN: 686868 | Radio: BAT
  OTA URL: https://raw.githubusercontent.com/silversymphony/esp32-firmware/main/version.json
  NAT Routing: ON
  RAM Free: 182 KB
  Uptime: 15420s
  ```

#### • `SCAN`
- **Mục đích:** Quét danh sách sóng Wi-Fi 2.4GHz trong khu vực lân cận.
- **Phản hồi mẫu:**
  ```text
  [SCAN] Dang quet cac mang Wi-Fi 2.4GHz xung quanh...
  === TIM THAY 5 MANG WI-FI ===
  1. Home_WiFi_5G (-54 dBm) Ch:6 [Lock]
  2. Coffee_Free (-72 dBm) Ch:1 [Open]
  3. Office_5G_Ext (-80 dBm) Ch:11 [Lock]
  ...
  ```

---

### 2.2. Nhóm Cấu Hình Mạng Nguồn (Internet STA)
#### • `WIFI:<ssid>,<password>`
- **Mục đích:** Đổi Wi-Fi nguồn cấp mạng Internet cho ESP32. Mạch sẽ tự động lưu NVS và kết nối ngay.
- **Ví dụ có mật khẩu:** `WIFI:Home_WiFi_5G,Pass12345`
- **Ví dụ không có mật khẩu (Wi-Fi mở):** `WIFI:Coffee_Free`
- **Phản hồi:**
  ```text
  [STA] Da cap nhat Wi-Fi nguon: 'Home_WiFi_5G'. Dang ket noi lai...
  [OK] Da luu Wi-Fi! ESP32 dang ket noi vao: Home_WiFi_5G
  ```

#### • `STA:ON` hoặc `STA:OFF`
- **Mục đích:** Bật hoặc tắt Wi-Fi nguồn (Internet STA).
- **`STA:OFF`:** Tắt hoàn toàn kết nối Wi-Fi nguồn, chuyển ESP32 sang **Chế độ Mạng nội bộ Offline (Local LAN)**. Khi đó, DHCP Server của ESP32 sẽ tự động không gửi Default Gateway và DNS cho điện thoại, giúp điện thoại (iOS / Android) kết nối vào SoftAP **vẫn duy trì 100% mạng 5G Cellular để lướt Web/App** mà không bị gián đoạn.
- **`STA:ON`:** Bật lại Wi-Fi nguồn, kết nối lại Internet và bật chia sẻ mạng NAPT.
- **Phản hồi:**
  ```text
  [OK] Da TAT Wi-Fi nguon! ESP32 chuyen sang che do MANG NOI BO OFFLINE. Dien thoai ket noi vao se giu nguyen 5G de truy cap Internet.
  ```

---

### 2.3. Nhóm Cấu Hình Mạng Phát (SoftAP & Gateway)
#### • `AP:<ssid>,<password>`
- **Mục đích:** Đổi tên Wi-Fi phát và mật khẩu phát.
- **Ví dụ:** `AP:ODE - wireless WiFi,Ode6868@`
- **Ghi chú:** Sau khi đổi cấu hình SoftAP, bạn nên gửi lệnh `REBOOT` để khởi động lại mạng phát.

#### • `APCH:<1-13 hoặc AUTO/0>`
- **Mục đích:** Đổi kênh phát sóng Wi-Fi (1 đến 13 hoặc đặt 0 / AUTO để tự động chọn).
- **Ví dụ:** `APCH:6` hoặc `APCH:AUTO`
- **Phản hồi:** `[OK] Da luu kenh phat SoftAP: 6. Gui 'REBOOT' de ap dung!`

#### • `AUTOCH`
- **Mục đích:** Tự động quét và đánh giá độ nhiễu xung quanh 2.4GHz để chọn kênh tối ưu nhất (ít nhiễu nhất).
- **Ví dụ:** `AUTOCH`
- **Phản hồi mẫu:**
  ```text
  [OK] === KET QUA DANH GIA KENH 2.4GHz ===
  Ch 1: 3 AP (Diem nhieu: 285)
  Ch 6: 1 AP (Diem nhieu: 60)
  Ch 11: 0 AP (Diem nhieu: 0)
  Ch 13: 0 AP (Diem nhieu: 0)
  => DA CHON KENH TOI UU: Kênh 11
  => Da tu dong chon Kenh: 11. Gui 'REBOOT' de ap dung!
  ```

#### • `PROFILES`
- **Mục đích:** Xem toàn bộ danh sách các mẫu cấu hình mạng phát (SoftAP Presets) đã lưu trong Flash NVS.
- **Ví dụ:** `PROFILES`
- **Phản hồi mẫu:**
  ```text
  === DANH SACH MAU CAU HINH MANG PHAT (PRESETS) ===
  [1] ID: p1 | 'Mặc định (ODE Wireless)'
      SSID: ODE - wireless WiFi | Pass: Ode6868@ | Ch: 6
      IP: 10.51.31.254 | Sub: 255.255.252.0 | DHCP: 10.51.28.10 -> 10.51.28.200
  [2] ID: p2 | 'Tối ưu Tự Động (Auto Channel)'
      SSID: ODE - wireless WiFi | Pass: Ode6868@ | Ch: Auto
      IP: 10.51.31.254 | Sub: 255.255.252.0 | DHCP: 10.51.28.10 -> 10.51.28.200
  Lenh: 'PROFILE:APPLY:id', 'PROFILE:SAVE:TenMoi', 'PROFILE:CLONE', 'PROFILE:DEL:id'
  ```

#### • `PROFILE:APPLY:<id hoặc số thứ tự>`
- **Mục đích:** Kích hoạt ngay một mẫu cấu hình từ danh sách và khởi động lại cổng phát SoftAP.
- **Ví dụ:** `PROFILE:APPLY:p1` hoặc `PROFILE:APPLY:1`
- **Phản hồi:** `[OK] Da ap dung mau cau hinh! ESP32 dang khoi dong lai...`

#### • `PROFILE:SAVE:<tên_mẫu>`
- **Mục đích:** Lưu toàn bộ thông số mạng phát hiện tại thành một mẫu cấu hình Preset mới.
- **Ví dụ:** `PROFILE:SAVE:GiaDinh`
- **Phản hồi:** `[OK] Da luu cau hinh hien tai thanh Preset: 'GiaDinh' (ID: p1745423000)`

#### • `PROFILE:CLONE` hoặc `PROFILE:CLONESTA`
- **Mục đích:** Tự động sao chép toàn bộ thông số của Wi-Fi nguồn (Internet STA) hiện tại thành 1 Preset SoftAP mới (gồm SSID, BSSID/MAC Router, Gateway IP, Subnet Mask và dải IP DHCP theo quy tắc IP nguồn).
- **Ví dụ:** `PROFILE:CLONE`
- **Phản hồi:**
  ```text
  [OK] Da sao chep Wi-Fi nguon thanh Preset: 'Coffee_Free (Clone)'!
     • SSID: Coffee_Free
     • BSSID: 84:F1:47:4F:27:A0
     • Gateway: 192.168.1.1 | Subnet: 255.255.255.0
     • DHCP: 192.168.1.10 -> 192.168.1.200
     • Channel: 6
  ```

#### • `PROFILE:DEL:<id hoặc số thứ tự>`
- **Mục đích:** Xóa một mẫu cấu hình ra khỏi danh sách.
- **Ví dụ:** `PROFILE:DEL:p2` hoặc `PROFILE:DEL:2`
- **Phản hồi:** `[OK] Da xoa mau cau hinh: p2`

#### • `APMAC:<xx:xx:xx:xx:xx:xx>`
- **Mục đích:** Đổi địa chỉ BSSID (MAC phần cứng) của mạng phát SoftAP.
- **Ví dụ:** `APMAC:84:F1:47:4F:27:A0`
- **Phản hồi:** `[OK] Da luu dia chi MAC SoftAP: 84:F1:47:4F:27:A0. Gui 'REBOOT' de ap dung!`

#### • `APIP:<gateway_ip>,<subnet_mask>`
- **Mục đích:** Đổi địa chỉ IP cổng Gateway và Subnet Mask của mạng phát.
- **Ví dụ:** `APIP:10.51.31.254,255.255.252.0`
- **Phản hồi:** `[OK] Da luu Gateway IP: 10.51.31.254 | Subnet: 255.255.252.0. Gui 'REBOOT' de ap dung!`

#### • `DHCP:<start_ip>,<end_ip>`
- **Mục đích:** Cấu hình dải cấp phát IP ngẫu nhiên cho Custom DHCP Server trên cổng UDP 67.
- **Ví dụ:** `DHCP:10.51.28.10,10.51.28.200` (dải gồm 191 địa chỉ IP).
- **Phản hồi:** `[OK] Da luu dai cap DHCP: 10.51.28.10 -> 10.51.28.200`

#### • `BOOST:ON` hoặc `BOOST:OFF`
- **Mục đích:** Điều chỉnh công suất phát sóng Wi-Fi phần cứng.
  - `BOOST:ON`: Mức công suất tối đa phần cứng **20 dBm (~100mW)**, xuyên tường tốt hơn 30-50%.
  - `BOOST:OFF`: Mức tiêu chuẩn **15 dBm (~30mW)**, giúp chip mát hơn và tiết kiệm điện.
- **Phản hồi:** `[OK] Da BAT che do phat song cuc dai (20 dBm / ~100mW)!`

---

### 2.4. Nhóm Bảo Mật Bluetooth LE
#### • `SETPIN:<6_chữ_số>`
- **Mục đích:** Thay đổi mã PIN ghép đôi phần cứng Bluetooth LE (Passkey Bonding). Mã PIN phải đúng 6 chữ số (0-9).
- **Ví dụ:** `SETPIN:686868`
- **Phản hồi:** `[OK] Da doi ma PIN thanh cong sang: 686868`
- **Lưu ý:** Ngay khi đổi xong, các thiết bị mới kết nối Bluetooth sẽ phải nhập mã PIN mới này trên màn hình điện thoại thì mới ghép đôi được.

#### • `BLERADIO:OFF`
- **Mục đích:** Tắt hoàn toàn sóng phát Bluetooth LE để ẩn danh (tàng hình) và giảm tối đa tiêu thụ điện.
- **Phản hồi:** `[OK] Dang tat hoan toan song Bluetooth LE theo yeu cau...`

---

### 2.5. Nhóm Nâng Cấp Firmware Không Dây (GitHub Cloud OTA)
#### • `OTAURL:<url_version_json>`
- **Mục đích:** Đổi đường dẫn file `version.json` trên kho lưu trữ GitHub.
- **Ví dụ:** `OTAURL:https://raw.githubusercontent.com/silversymphony/esp32-firmware/main/version.json`
- **Phản hồi:** `[OK] Da luu GitHub OTA URL: ...`

#### • `OTA`
- **Mục đích:** ESP32 kết nối trực tiếp đến GitHub qua HTTPS để kiểm tra xem có bản cập nhật mới hay không.
- **Phản hồi mẫu:**
  ```text
  [OTA] Dang kiem tra version tren GitHub...
  [OTA] Tim thay ban moi: v1.1.1 (Hien tai: v1.1.0)
  [OTA] Changelog: Toi uu NAPT va bao mat Bluetooth
  [OTA] Gui lenh 'OTA:RUN' de bat dau tai va nap Flash!
  ```

#### • `OTA:RUN`
- **Mục đích:** Kích hoạt quá trình tải `esp32-firmware.bin` từ GitHub và nạp vào phân vùng OTA của bộ nhớ Flash.
- **Phản hồi mẫu:**
  ```text
  [OTA] Bat dau tai firmware v1.1.1 tu GitHub va ghi Flash...
  [OTA] Tien trinh Flash: 20% ... 40% ... 80% ... 100%
  [OTA] Nap Flash thanh cong! ESP32 dang khoi dong lai vao Firmware moi...
  ```

---

### 2.6. Nhóm Quản Trị Hệ Thống
#### • `CLEARLOGS` hoặc `CLS`
- **Mục đích:** Xóa sạch 35 dòng log lưu tạm trong RAM buffer.
- **Phản hồi:** `[OK] Da xoa toan bo nhat ky hoat dong!`

#### • `REBOOT`
- **Mục đích:** Khởi động lại ESP32 sau 1 giây.
- **Phản hồi:** `[OK] ESP32 se khoi dong lai sau 1 giay...`

---

## 3. HƯỚNG DẪN KẾT NỐI VÀ GỬI LỆNH

### Cách 1: Kết nối không dây qua điện thoại (Bluetooth LE)
1. Cài ứng dụng **Serial Bluetooth Terminal** (Android) hoặc **BLE Terminal** / **LightBlue** (iOS).
2. Bật Bluetooth trên điện thoại, quét tìm thiết bị: **`ESP32-Gateway-BLE`**.
3. Bấm kết nối ➔ Hệ điều hành điện thoại sẽ hiện bảng hỏi: **"Nhập mã PIN ghép đôi"**.
4. Nhập mã PIN: **`686868`** (hoặc mã PIN mới nếu bạn đã đổi).
5. Sau khi ghép đôi thành công, các dòng log của ESP32 sẽ tự động truyền liên tục về màn hình.
6. Gõ bất kỳ lệnh nào trong bảng trên (ví dụ `HELP` hoặc `STATUS`) vào ô nhập và bấm **Send**.

### Cách 2: Kết nối có dây qua máy tính (USB Serial)
1. Cắm cáp USB kết nối ESP32 vào máy tính.
2. Mở **Arduino IDE** ➔ chọn cổng COM/tty ➔ mở **Serial Monitor** (hoặc dùng PuTTY, minicom, screen).
3. Cài đặt tốc độ truyền: **`115200 baud`**.
4. Cài đặt kết thúc dòng: **`Newline`** hoặc **`Both NL & CR`**.
5. Gõ lệnh (ví dụ `STATUS`, `SCAN`, `HELP`) và nhấn **Enter**. Mạch sẽ phản hồi ngay lập tức.
