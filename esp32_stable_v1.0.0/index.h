#ifndef INDEX_H
#define INDEX_H

#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP32 Gateway Manager</title>
<style>
  :root {
    --bg: #030712;
    --card: #111827;
    --card-inner: #1f2937;
    --border: #374151;
    --text: #f9fafb;
    --text-muted: #9ca3af;
    --accent: #2563eb;
    --accent-hover: #1d4ed8;
    --success: #10b981;
    --warning: #f59e0b;
    --danger: #ef4444;
  }
  * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; }
  body { background: var(--bg); color: var(--text); padding: 12px; min-height: 100vh; display: flex; justify-content: center; }
  .container { width: 100%; max-width: 520px; }
  .header { background: var(--card); border: 1px solid var(--border); border-radius: 16px; padding: 16px; margin-bottom: 12px; display: flex; align-items: center; justify-content: space-between; }
  .header-left { display: flex; align-items: center; gap: 12px; }
  .header-icon { width: 40px; height: 40px; border-radius: 10px; background: linear-gradient(135deg, #06b6d4, #2563eb); display: flex; align-items: center; justify-content: center; font-size: 20px; }
  .header-title { font-size: 15px; font-weight: 700; color: #fff; }
  .header-ip { font-size: 11px; color: var(--text-muted); font-family: monospace; }
  .badge { font-size: 10px; font-weight: 700; padding: 4px 8px; border-radius: 9999px; text-transform: uppercase; letter-spacing: 0.5px; }
  .badge-success { background: rgba(16, 185, 129, 0.2); color: #34d399; border: 1px solid rgba(16, 185, 129, 0.4); }
  .badge-danger { background: rgba(239, 68, 68, 0.2); color: #f87171; border: 1px solid rgba(239, 68, 68, 0.4); }
  .badge-warning { background: rgba(245, 158, 11, 0.2); color: #fbbf24; border: 1px solid rgba(245, 158, 11, 0.4); }
  .badge-blue { background: rgba(37, 99, 235, 0.2); color: #60a5fa; border: 1px solid rgba(37, 99, 235, 0.4); }
  .captive-alert { background: rgba(245, 158, 11, 0.15); border: 1px solid rgba(245, 158, 11, 0.4); border-radius: 14px; padding: 14px; margin-bottom: 12px; display: flex; gap: 10px; }
  .tabs { display: grid; grid-template-columns: 1fr 1fr 1fr 1fr; gap: 4px; background: var(--card); border: 1px solid var(--border); padding: 4px; border-radius: 12px; margin-bottom: 12px; }
  .tab-btn { background: transparent; border: none; color: var(--text-muted); padding: 9px 2px; font-size: 11px; font-weight: 600; border-radius: 8px; cursor: pointer; transition: 0.2s; text-align: center; }
  .tab-btn.active { background: var(--accent); color: #fff; }
  .card { background: var(--card); border: 1px solid var(--border); border-radius: 16px; padding: 16px; margin-bottom: 12px; }
  .card-title { font-size: 13px; font-weight: 700; color: #fff; margin-bottom: 12px; padding-bottom: 8px; border-bottom: 1px solid var(--card-inner); display: flex; justify-content: space-between; align-items: center; }
  .grid-2 { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; }
  .info-box { background: var(--card-inner); padding: 10px; border-radius: 10px; }
  .info-label { font-size: 10px; color: var(--text-muted); text-transform: uppercase; margin-bottom: 2px; }
  .info-val { font-size: 13px; font-weight: 600; color: #fff; font-family: monospace; word-break: break-all; }
  .form-group { margin-bottom: 12px; }
  label { display: block; font-size: 11px; font-weight: 600; color: var(--text-muted); margin-bottom: 4px; }
  input, select { width: 100%; background: var(--card-inner); border: 1px solid var(--border); border-radius: 10px; padding: 10px 12px; color: #fff; font-size: 13px; font-family: monospace; outline: none; }
  input:focus, select:focus { border-color: var(--accent); }
  .pass-wrapper { position: relative; }
  .pass-toggle { position: absolute; right: 10px; top: 10px; background: none; border: none; color: var(--text-muted); cursor: pointer; font-size: 14px; }
  .btn { width: 100%; padding: 12px; font-size: 13px; font-weight: 700; border-radius: 10px; border: none; cursor: pointer; display: flex; align-items: center; justify-content: center; gap: 6px; transition: 0.2s; }
  .btn-primary { background: linear-gradient(135deg, #2563eb, #06b6d4); color: #fff; }
  .btn-primary:hover { opacity: 0.9; }
  .btn-success { background: linear-gradient(135deg, #10b981, #059669); color: #fff; }
  .btn-scan { background: var(--accent); color: #fff; padding: 6px 12px; font-size: 11px; border-radius: 8px; border: none; cursor: pointer; }
  .wifi-list { max-height: 180px; overflow-y: auto; margin-bottom: 12px; display: flex; flex-direction: column; gap: 6px; }
  .wifi-item { background: var(--card-inner); padding: 10px 12px; border-radius: 10px; cursor: pointer; display: flex; align-items: center; justify-content: space-between; border: 1px solid transparent; }
  .wifi-item:hover { border-color: var(--accent); }
  .wifi-item-title { font-size: 12px; font-weight: 600; color: #fff; }
  .wifi-item-sub { font-size: 10px; color: var(--text-muted); font-family: monospace; }
  .wifi-item-rssi { font-size: 11px; font-family: monospace; font-weight: 700; }
  .notice { font-size: 11px; color: var(--text-muted); background: var(--card-inner); padding: 8px 10px; border-radius: 8px; margin-bottom: 12px; line-height: 1.4; }
  .footer { text-align: center; font-size: 10px; color: #6b7280; font-family: monospace; margin-top: 10px; }
  .hidden { display: none !important; }
  
  /* Switch Toggle CSS */
  .switch-box { display: flex; align-items: center; justify-content: space-between; background: linear-gradient(135deg, rgba(245, 158, 11, 0.1), rgba(17, 24, 39, 0.8)); border: 1px solid rgba(245, 158, 11, 0.3); border-radius: 12px; padding: 12px; margin-bottom: 12px; }
  .switch-box-blue { background: linear-gradient(135deg, rgba(37, 99, 235, 0.1), rgba(17, 24, 39, 0.8)); border-color: rgba(37, 99, 235, 0.3); }
  .switch-label-title { font-size: 12px; font-weight: 700; color: #fde68a; margin-bottom: 2px; }
  .switch-label-desc { font-size: 10px; color: #9ca3af; }
  .switch { position: relative; display: inline-block; width: 44px; height: 24px; flex-shrink: 0; }
  .switch input { opacity: 0; width: 0; height: 0; }
  .slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #374151; transition: .3s; border-radius: 24px; }
  .slider:before { position: absolute; content: ""; height: 18px; width: 18px; left: 3px; bottom: 3px; background-color: white; transition: .3s; border-radius: 50%; }
  input:checked + .slider { background-color: #2563eb; }
  input:checked + .slider.slider-amber { background-color: #f59e0b; }
  input:checked + .slider:before { transform: translateX(20px); }

  /* Terminal Log Box */
  .terminal { background: #030712; border: 1px solid var(--border); border-radius: 10px; padding: 10px; height: 220px; overflow-y: auto; font-family: monospace; font-size: 11px; line-height: 1.5; color: #34d399; }
  .terminal div { word-break: break-all; margin-bottom: 2px; }
</style>
</head>
<body>
<div class="container">
  
  <!-- Header -->
  <div class="header">
    <div class="header-left">
      <div class="header-icon">📶</div>
      <div>
        <div class="header-title">ESP32 Gateway Manager</div>
        <div class="header-ip" id="headIp">10.51.31.254</div>
      </div>
    </div>
    <div id="connBadge" class="badge badge-danger">ĐANG KIỂM TRA</div>
  </div>

  <!-- Captive Portal Banner -->
  <div id="captiveAlert" class="captive-alert hidden">
    <div style="font-size: 20px;">⚠️</div>
    <div>
      <div style="font-size: 12px; font-weight: 700; color: #fbbf24; margin-bottom: 2px;">Chưa kết nối Internet! (Captive Portal)</div>
      <div style="font-size: 11px; color: #fef3c7; line-height: 1.4;">Vui lòng chuyển sang tab <b>Wi-Fi Nguồn</b> để quét và chọn Wi-Fi cấp mạng cho hệ thống.</div>
    </div>
  </div>

  <!-- Tabs Navigation (4 Tabs) -->
  <div class="tabs">
    <button class="tab-btn active" id="btnTab1" onclick="showTab(1)">📊 Trạng thái</button>
    <button class="tab-btn" id="btnTab2" onclick="showTab(2)">🌐 Nguồn</button>
    <button class="tab-btn" id="btnTab3" onclick="showTab(3)">📡 Mạng Phát</button>
    <button class="tab-btn" id="btnTab4" onclick="showTab(4)">📟 Log & OTA</button>
  </div>

  <!-- TAB 1: TRẠNG THÁI HỆ THỐNG -->
  <div id="tab1">
    <!-- Card STA -->
    <div class="card">
      <div class="card-title">
        <span>🌐 Wi-Fi Nguồn (Internet STA)</span>
        <span id="staStatusText" style="font-size: 11px; color: #9ca3af;">Đang tải...</span>
      </div>
      <div class="grid-2">
        <div class="info-box">
          <div class="info-label">Tên mạng nguồn</div>
          <div class="info-val" id="valStaSsid">---</div>
        </div>
        <div class="info-box">
          <div class="info-label">Cường độ sóng (RSSI)</div>
          <div class="info-val" id="valStaRssi">---</div>
        </div>
        <div class="info-box">
          <div class="info-label">IP ESP32 nhận được</div>
          <div class="info-val" style="color: #67e8f9;" id="valStaIp">---</div>
        </div>
        <div class="info-box">
          <div class="info-label">Gateway nguồn</div>
          <div class="info-val" id="valStaGw">---</div>
        </div>
      </div>
    </div>

    <!-- Card AP -->
    <div class="card">
      <div class="card-title">
        <span>📡 Mạng Phát (SoftAP & Gateway)</span>
        <div style="display: flex; gap: 4px;">
          <span id="valPowerBadge" class="badge badge-warning">20 dBm BOOST</span>
          <span id="valApChannelBadge" class="badge badge-blue">KÊNH 6</span>
        </div>
      </div>
      <div class="grid-2">
        <div class="info-box">
          <div class="info-label">Tên Wi-Fi phát ra</div>
          <div class="info-val" id="valApSsid">---</div>
        </div>
        <div class="info-box">
          <div class="info-label">Địa chỉ BSSID (MAC)</div>
          <div class="info-val" style="color: #fde047;" id="valApBssid">---</div>
        </div>
        <div class="info-box">
          <div class="info-label">IP Gateway</div>
          <div class="info-val" style="color: #67e8f9;" id="valApIp">---</div>
        </div>
        <div class="info-box">
          <div class="info-label">Subnet Mask</div>
          <div class="info-val" id="valApSubnet">---</div>
        </div>
        <div class="info-box" style="grid-column: span 2;">
          <div class="info-label">Cấp IP Ngẫu Nhiên Mỗi Lần Kết Nối (Random DHCP)</div>
          <div class="info-val" style="color: #a7f3d0;" id="valDhcpPool">---  ➔  ---</div>
        </div>
      </div>
    </div>

    <!-- Card BLE Status -->
    <div class="card" style="padding: 12px; display: flex; align-items: center; justify-content: space-between;">
      <div style="display: flex; align-items: center; gap: 10px;">
        <span style="font-size: 20px;">ᛒ</span>
        <div>
          <div style="font-size: 12px; font-weight: 700; color: #93c5fd;">Bluetooth LE Bảo Mật (PIN)</div>
          <div style="font-size: 10px; color: #9ca3af; font-family: monospace;">Tên: ESP32-Gateway-BLE • PIN: <span id="dashBlePin" style="color:#fbbf24; font-weight:bold;">686868</span></div>
        </div>
      </div>
      <span id="valBleBadge" class="badge badge-success">ĐÃ BẬT</span>
    </div>

    <!-- Card Sys -->
    <div class="card" style="padding: 12px;">
      <div style="display: flex; justify-content: space-around; text-align: center;">
        <div>
          <div class="info-label">RAM Trống</div>
          <div class="info-val" style="color: #34d399;" id="valHeap">---</div>
        </div>
        <div style="width: 1px; background: var(--border);"></div>
        <div>
          <div class="info-label">Uptime</div>
          <div class="info-val" id="valUptime">---</div>
        </div>
        <div style="width: 1px; background: var(--border);"></div>
        <div>
          <div class="info-label">Firmware</div>
          <div class="info-val" style="color: #fbbf24;" id="valFwVer">v1.0.0</div>
        </div>
        <div style="width: 1px; background: var(--border);"></div>
        <div>
          <div class="info-label">Định tuyến NAT</div>
          <div class="info-val" style="color: #38bdf8;" id="valNat">ON</div>
        </div>
      </div>
    </div>
  </div>

  <!-- TAB 2: CÀI ĐẶT WI-FI NGUỒN -->
  <div id="tab2" class="hidden">
    <div class="card">
      <div class="card-title">
        <span>Quét Mạng Wi-Fi 2.4GHz</span>
        <button class="btn-scan" id="scanBtn" onclick="scanWifi()">🔍 Quét mạng</button>
      </div>
      <div class="wifi-list" id="wifiScanResults">
        <div style="font-size: 11px; color: var(--text-muted); text-align: center; padding: 12px;">Bấm nút "Quét mạng" để tìm Wi-Fi xung quanh...</div>
      </div>

      <div style="border-top: 1px solid var(--border); padding-top: 12px;">
        <div class="form-group">
          <label>Tên Wi-Fi Nguồn (SSID)</label>
          <input type="text" id="staSsidInput" placeholder="Nhập hoặc chọn Wi-Fi bên trên">
        </div>
        <div class="form-group">
          <label>Mật khẩu Wi-Fi Nguồn</label>
          <div class="pass-wrapper">
            <input type="password" id="staPassInput" placeholder="Nhập mật khẩu (để trống nếu mở)">
            <button class="pass-toggle" type="button" onclick="togglePass('staPassInput', this)">👁️</button>
          </div>
        </div>
        <div class="notice">💾 Cấu hình sẽ được lưu vĩnh viễn vào bộ nhớ Flash ESP32 (Preferences NVS).</div>
        <button class="btn btn-primary" onclick="saveSta()">🚀 Lưu Cấu Hình & Kết Nối</button>
      </div>
    </div>
  </div>

  <!-- TAB 3: CÀI ĐẶT MẠNG PHÁT (AP) & MAX POWER BOOST -->
  <div id="tab3" class="hidden">
    <div class="card">
      <div class="card-title">
        <span>Cấu Hình Mạng Phát (SoftAP & Gateway)</span>
      </div>

      <!-- Max TX Power Boost Switch -->
      <div class="switch-box">
        <div>
          <div class="switch-label-title">⚡ Phát Sóng Cực Đại (Max TX Power Boost)</div>
          <div class="switch-label-desc">Mức công suất tối đa phần cứng <b>20 dBm (~100mW)</b></div>
        </div>
        <label class="switch">
          <input type="checkbox" id="powerBoostToggle" onchange="togglePowerBoost(this)">
          <span class="slider slider-amber"></span>
        </label>
      </div>
      <div id="powerBoostNotice" class="notice" style="color: #fde68a;">🚀 Đang kích hoạt 20 dBm: Tăng cường độ xuyên tường, phát sóng xa hơn 30-50%.</div>
      
      <div class="form-group">
        <label>Tên Wi-Fi Phát Ra (AP SSID)</label>
        <input type="text" id="apSsidInput">
      </div>

      <div class="form-group">
        <label>Mật Khẩu Phát (để trống nếu Open)</label>
        <div class="pass-wrapper">
          <input type="password" id="apPassInput">
          <button class="pass-toggle" type="button" onclick="togglePass('apPassInput', this)">👁️</button>
        </div>
      </div>

      <div class="grid-2">
        <div class="form-group">
          <label>Kênh Sóng (Channel)</label>
          <select id="apChannelSelect">
            <option value="1">Kênh 1 (2.412 GHz)</option>
            <option value="6">Kênh 6 (2.437 GHz)</option>
            <option value="11">Kênh 11 (2.462 GHz)</option>
            <option value="2">Kênh 2</option>
            <option value="3">Kênh 3</option>
            <option value="4">Kênh 4</option>
            <option value="5">Kênh 5</option>
            <option value="7">Kênh 7</option>
            <option value="8">Kênh 8</option>
            <option value="9">Kênh 9</option>
            <option value="10">Kênh 10</option>
            <option value="12">Kênh 12</option>
            <option value="13">Kênh 13</option>
          </select>
        </div>
        <div class="form-group">
          <label>Địa Chỉ BSSID (MAC)</label>
          <input type="text" id="apBssidInput" placeholder="84:F1:47:4F:27:A0">
        </div>
      </div>

      <div class="grid-2">
        <div class="form-group">
          <label>IP Gateway (Cổng ESP32)</label>
          <input type="text" id="apIpInput" placeholder="10.51.31.254">
        </div>
        <div class="form-group">
          <label>Subnet Mask</label>
          <input type="text" id="apSubnetInput" placeholder="255.255.252.0">
        </div>
      </div>

      <div class="info-box" style="margin-bottom: 12px;">
        <label style="color: #cbd5e1; margin-bottom: 6px;">Dải Cấp IP Tự Động (DHCP Ngẫu Nhiên - Không Giới Hạn)</label>
        <div class="grid-2">
          <div>
            <span style="font-size: 10px; color: var(--text-muted);">Bắt đầu:</span>
            <input type="text" id="dhcpStartInput" placeholder="10.51.28.50">
          </div>
          <div>
            <span style="font-size: 10px; color: var(--text-muted);">Kết thúc:</span>
            <input type="text" id="dhcpEndInput" placeholder="10.51.31.100">
          </div>
        </div>
      </div>

      <div class="notice" style="color: #fde68a;">🎲 Hệ thống tự động bốc ngẫu nhiên 1 IP trong dải này cho thiết bị mỗi lần kết nối!</div>
      <button class="btn btn-success" onclick="saveAp()">💾 Lưu Tham Số & Khởi Động Lại AP</button>
    </div>
  </div>

  <!-- TAB 4: WIRELESS CONSOLE LOG & GITHUB OTA -->
  <div id="tab4" class="hidden">
    
    <!-- Nâng Cấp Firmware Không Dây (GitHub OTA) -->
    <div class="card" style="background: linear-gradient(135deg, rgba(79, 70, 229, 0.12), rgba(17, 24, 39, 0.9)); border-color: rgba(99, 102, 241, 0.35);">
      <div class="card-title">
        <span style="display: flex; align-items: center; gap: 6px;">
          <span>🚀</span>
          <span style="color: #a5b4fc;">Nâng Cấp Firmware Không Dây (GitHub OTA)</span>
        </span>
        <span id="badgeFwVerOta" class="badge" style="background: rgba(99, 102, 241, 0.2); color: #c7d2fe; border: 1px solid rgba(99, 102, 241, 0.4);">v1.0.0</span>
      </div>

      <div class="form-group">
        <label>Đường dẫn GitHub Version (version.json)</label>
        <div style="display: flex; gap: 8px;">
          <input type="text" id="inputOtaUrl" value="https://raw.githubusercontent.com/silversymphony/esp32-firmware/main/version.json" style="font-size: 11px;">
          <button class="btn btn-primary" style="width: auto; padding: 0 14px; font-size: 11px;" onclick="saveOtaUrl()">Lưu URL</button>
        </div>
      </div>

      <button id="btnCheckOta" class="btn" style="background: linear-gradient(135deg, #4f46e5, #2563eb); color: #fff; margin-bottom: 8px;" onclick="checkGithubOta()">
        <span id="checkOtaIcon">🔍</span>
        <span id="checkOtaText">Kiểm Tra Bản Cập Nhật Mới</span>
      </button>

      <!-- Khung hiển thị khi phát hiện bản mới -->
      <div id="otaResultBox" class="hidden" style="background: #030712; border: 1px solid rgba(99, 102, 241, 0.4); border-radius: 12px; padding: 12px; margin-top: 8px;">
        <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px;">
          <div style="font-size: 12px; font-weight: 700; color: #34d399;">
            🎉 Tìm thấy bản mới: <span id="otaNewVerText" style="color: #fbbf24; font-family: monospace;">---</span>
          </div>
          <span style="font-size: 10px; color: #9ca3af; font-family: monospace;">Sẵn sàng tải</span>
        </div>

        <div id="otaChangelogText" style="font-size: 11px; color: #cbd5e1; line-height: 1.5; margin-bottom: 10px; background: var(--card-inner); padding: 8px 10px; border-radius: 8px;">
        </div>

        <!-- Thanh tiến trình nạp Flash -->
        <div id="otaProgressBox" class="hidden" style="margin-bottom: 10px;">
          <div style="width: 100%; height: 8px; background: #1f2937; border-radius: 9999px; overflow: hidden; margin-bottom: 4px;">
            <div id="otaProgressBar" style="width: 0%; height: 100%; background: linear-gradient(90deg, #3b82f6, #10b981); transition: width 0.3s;"></div>
          </div>
          <div style="display: flex; justify-content: space-between; font-size: 10px; font-family: monospace; color: #9ca3af;">
            <span id="otaProgressStatus">Đang tải và ghi Flash...</span>
            <span id="otaProgressPercent">0%</span>
          </div>
        </div>

        <button id="btnRunOta" class="btn btn-success" onclick="startGithubOta()">
          <span>🚀</span>
          <span>Bắt Đầu Tải & Ghi Flash Ngay</span>
        </button>
      </div>

      <div class="notice" style="color: #cbd5e1; margin-top: 10px; margin-bottom: 0;">
        💡 <b>Cơ chế hoạt động:</b> ESP32 kết nối trực tiếp đến GitHub bằng giao thức bảo mật HTTPS, tải file <code>esp32-firmware.bin</code> và nạp tự động vào phân vùng dự phòng (Dual OTA Partition). Sau khi nạp xong, chip tự restart sang bản mới an toàn 100%.
      </div>
    </div>

    <!-- Quản trị & Bảo mật Bluetooth LE -->
    <div class="card">
      <div class="card-title">
        <span>🛡️ Bảo Mật & Quản Trị Bluetooth LE</span>
        <span id="bleStatusBadgeTab4" class="badge badge-success">ĐANG BẬT</span>
      </div>

      <!-- Switch Bật / Tắt sóng Bluetooth -->
      <div class="switch-box switch-box-blue">
        <div>
          <div class="switch-label-title" style="color: #93c5fd;">Sóng Bluetooth LE (BLE Radio)</div>
          <div class="switch-label-desc">Tắt hoàn toàn Bluetooth để tàng hình và tiết kiệm điện</div>
        </div>
        <label class="switch">
          <input type="checkbox" id="bleRadioToggle" onchange="toggleBleRadio(this)">
          <span class="slider"></span>
        </label>
      </div>

      <!-- Đổi mã PIN Bluetooth -->
      <div class="form-group" style="margin-top: 8px;">
        <label>Mã PIN Ghép Đôi Bảo Mật (Đúng 6 Chữ Số)</label>
        <div style="display: flex; gap: 8px;">
          <div class="pass-wrapper" style="flex: 1;">
            <input type="password" id="inputBlePin" maxlength="6" placeholder="VD: 686868" style="font-weight: 700; letter-spacing: 2px; text-align: center; color: #fbbf24;">
            <button class="pass-toggle" type="button" onclick="togglePass('inputBlePin', this)">👁️</button>
          </div>
          <button class="btn btn-primary" style="width: auto; padding: 0 16px; font-size: 11px;" onclick="saveBlePin()">💾 Lưu PIN</button>
        </div>
        <div style="font-size: 10px; color: #9ca3af; margin-top: 4px;">Dùng mã PIN này khi điện thoại yêu cầu ghép đôi Bluetooth.</div>
      </div>

      <div class="notice" style="color: #cbd5e1; line-height: 1.5;">
        🔒 <b>Bảo mật bằng mã PIN phần cứng (BLE Passkey):</b><br>
        • Khi điện thoại kết nối, hệ điều hành sẽ yêu cầu nhập mã PIN 6 số để ghép đôi mã hóa.<br>
        • Sau khi ghép đôi thành công, log sẽ tự động truyền trực tiếp không dây và có thể gửi lệnh ngay mà <b>không cần bước xác thực phụ</b>.<br>
        • <b>Đổi mã PIN nhanh qua BLE:</b> Gõ lệnh <code style="color: #34d399;">SETPIN:<mã_mới></code> trên terminal.
      </div>
    </div>

    <!-- Console Terminal -->
    <div class="card">
      <div class="card-title">
        <span>📟 Nhật Ký Hoạt Động (Wireless Console Log)</span>
        <button class="btn-scan" onclick="clearLogs()">Xóa log</button>
      </div>
      <div class="terminal" id="termLogs">
        <div>[DANG TAI NHAT KY HE THONG...]</div>
      </div>
    </div>

  </div>

  <div class="footer">ESP32 NAPT Repeater Gateway • Built with Custom Random DHCP, BLE Security & GitHub OTA</div>
</div>

<script>
var currentTab = 1;

function showTab(n) {
  currentTab = n;
  for (var i = 1; i <= 4; i++) {
    document.getElementById('tab' + i).classList.add('hidden');
    document.getElementById('btnTab' + i).classList.remove('active');
  }
  document.getElementById('tab' + n).classList.remove('hidden');
  document.getElementById('btnTab' + n).classList.add('active');

  if (n === 4) {
    fetchLogs();
  }
}

function togglePass(id, btn) {
  var el = document.getElementById(id);
  if (el.type === 'password') {
    el.type = 'text';
    btn.innerText = '🔒';
  } else {
    el.type = 'password';
    btn.innerText = '👁️';
  }
}

function updateStatus() {
  fetch('/api/status')
    .then(function(res) { return res.json(); })
    .then(function(d) {
      var badge = document.getElementById('connBadge');
      var captive = document.getElementById('captiveAlert');
      document.getElementById('headIp').innerText = d.ap_ip;

      if (d.connected) {
        badge.className = 'badge badge-success';
        badge.innerText = 'ĐÃ KẾT NỐI';
        captive.classList.add('hidden');
        document.getElementById('staStatusText').innerText = 'Đang hoạt động';
        document.getElementById('staStatusText').style.color = '#34d399';
      } else {
        badge.className = 'badge badge-danger';
        badge.innerText = 'MẤT KẾT NỐI';
        captive.classList.remove('hidden');
        document.getElementById('staStatusText').innerText = 'Chưa kết nối';
        document.getElementById('staStatusText').style.color = '#f87171';
      }

      document.getElementById('valStaSsid').innerText = d.sta_ssid || '(Chưa cấu hình)';
      document.getElementById('valStaRssi').innerText = d.connected ? (d.sta_rssi + ' dBm') : '---';
      document.getElementById('valStaIp').innerText = d.connected ? d.sta_ip : 'Chưa có IP';
      document.getElementById('valStaGw').innerText = d.connected ? d.sta_gw : '---';

      document.getElementById('valApSsid').innerText = d.ap_ssid;
      document.getElementById('valApBssid').innerText = d.ap_bssid;
      document.getElementById('valApIp').innerText = d.ap_ip;
      document.getElementById('valApSubnet').innerText = d.ap_subnet;
      document.getElementById('valApChannelBadge').innerText = 'KÊNH ' + d.ap_channel;
      document.getElementById('valDhcpPool').innerText = d.dhcp_start + '  ➔  ' + d.dhcp_end + ' (Ngẫu nhiên)';

      // Cập nhật trạng thái Max TX Power Boost
      var pBadge = document.getElementById('valPowerBadge');
      var pToggle = document.getElementById('powerBoostToggle');
      var pNotice = document.getElementById('powerBoostNotice');
      if (d.tx_boost) {
        pBadge.className = 'badge badge-warning';
        pBadge.innerText = '20 dBm BOOST';
        pToggle.checked = true;
        pNotice.innerHTML = '🚀 <b>Đang kích hoạt 20 dBm:</b> Tăng cường độ xuyên tường, phát sóng xa hơn 30-50%.';
      } else {
        pBadge.className = 'badge badge-blue';
        pBadge.innerText = '15 dBm TIÊU CHUẨN';
        pToggle.checked = false;
        pNotice.innerHTML = '🍃 <b>Mức tiêu chuẩn 15 dBm:</b> Tiết kiệm điện năng, giúp chip ESP32 mát hơn.';
      }

      // Cập nhật trạng thái Bluetooth & PIN
      var bBadge = document.getElementById('valBleBadge');
      var bBadgeTab4 = document.getElementById('bleStatusBadgeTab4');
      var bToggle = document.getElementById('bleRadioToggle');
      document.getElementById('dashBlePin').innerText = d.ble_pin;

      if (d.ble_enabled) {
        bBadge.className = 'badge badge-success';
        bBadge.innerText = 'ĐÃ BẬT';
        bBadgeTab4.className = 'badge badge-success';
        bBadgeTab4.innerText = 'ĐANG BẬT';
        bToggle.checked = true;
      } else {
        bBadge.className = 'badge badge-danger';
        bBadge.innerText = 'ĐÃ TẮT';
        bBadgeTab4.className = 'badge badge-danger';
        bBadgeTab4.innerText = 'ĐÃ TẮT';
        bToggle.checked = false;
      }

      if (!document.getElementById('inputBlePin').value) {
        document.getElementById('inputBlePin').value = d.ble_pin;
      }

      document.getElementById('valHeap').innerText = Math.round(d.free_heap / 1024) + ' KB';
      var mins = Math.floor(d.uptime / 60);
      var hrs = Math.floor(mins / 60);
      mins = mins % 60;
      document.getElementById('valUptime').innerText = (hrs < 10 ? '0' : '') + hrs + 'h ' + (mins < 10 ? '0' : '') + mins + 'm';
      document.getElementById('valNat').innerText = d.napt ? 'BẬT' : 'TẮT';

      if (d.fw_ver) {
        document.getElementById('valFwVer').innerText = 'v' + d.fw_ver;
        document.getElementById('badgeFwVerOta').innerText = 'v' + d.fw_ver;
      }
      if (d.ota_url && !document.getElementById('inputOtaUrl').value) {
        document.getElementById('inputOtaUrl').value = d.ota_url;
      }

      if (!document.getElementById('staSsidInput').value) document.getElementById('staSsidInput').value = d.sta_ssid;
      if (!document.getElementById('apSsidInput').value) document.getElementById('apSsidInput').value = d.ap_ssid;
      if (!document.getElementById('apBssidInput').value) document.getElementById('apBssidInput').value = d.ap_bssid;
      if (!document.getElementById('apIpInput').value) document.getElementById('apIpInput').value = d.ap_ip;
      if (!document.getElementById('apSubnetInput').value) document.getElementById('apSubnetInput').value = d.ap_subnet;
      if (!document.getElementById('dhcpStartInput').value) document.getElementById('dhcpStartInput').value = d.dhcp_start;
      if (!document.getElementById('dhcpEndInput').value) document.getElementById('dhcpEndInput').value = d.dhcp_end;
      document.getElementById('apChannelSelect').value = d.ap_channel;
    })
    .catch(function(err) { console.log(err); });
}

function togglePowerBoost(cb) {
  var isBoost = cb.checked;
  var data = new URLSearchParams();
  data.append('boost', isBoost ? '1' : '0');

  fetch('/api/power_boost', { method: 'POST', body: data })
    .then(function(res) { return res.json(); })
    .then(function(d) {
      updateStatus();
    })
    .catch(function(err) {
      alert('Lỗi cập nhật công suất phát!');
      cb.checked = !isBoost;
    });
}

function toggleBleRadio(cb) {
  var isEn = cb.checked;
  var data = new URLSearchParams();
  data.append('enabled', isEn ? '1' : '0');

  fetch('/api/save_ble', { method: 'POST', body: data })
    .then(function(res) { return res.json(); })
    .then(function(d) {
      updateStatus();
    })
    .catch(function(err) {
      alert('Lỗi cập nhật sóng Bluetooth!');
      cb.checked = !isEn;
    });
}

function saveBlePin() {
  var pin = document.getElementById('inputBlePin').value.trim();
  if (!/^\d{6}$/.test(pin)) {
    alert('Mã PIN Bluetooth phải gồm đúng 6 chữ số (0-9)!');
    return;
  }
  var data = new URLSearchParams();
  data.append('pin', pin);

  fetch('/api/save_ble', { method: 'POST', body: data })
    .then(function(res) { return res.json(); })
    .then(function(d) {
      alert('Đã cập nhật mã PIN Bluetooth thành công: ' + pin);
      updateStatus();
    })
    .catch(function(err) { alert('Lỗi khi lưu mã PIN!'); });
}

function fetchLogs() {
  fetch('/api/logs')
    .then(function(res) { return res.json(); })
    .then(function(lines) {
      var term = document.getElementById('termLogs');
      if (lines.length === 0) {
        term.innerHTML = '<div style="color:#9ca3af;">(Chưa có nhật ký hoạt động nào)</div>';
        return;
      }
      var html = '';
      for (var i = 0; i < lines.length; i++) {
        var l = lines[i];
        var col = '#34d399';
        if (l.indexOf('[DHCP') >= 0) col = '#a7f3d0';
        else if (l.indexOf('[BLE-SEC') >= 0 || l.indexOf('[SECURITY') >= 0) col = '#fbbf24';
        else if (l.indexOf('[BLE') >= 0) col = '#93c5fd';
        else if (l.indexOf('[POWER') >= 0) col = '#fde047';
        else if (l.indexOf('[NAT') >= 0) col = '#38bdf8';
        else if (l.indexOf('[OTA') >= 0) col = '#c084fc';
        else if (l.indexOf('[CANH BAO') >= 0 || l.indexOf('[LOI') >= 0 || l.indexOf('[ERROR') >= 0) col = '#f87171';
        html += '<div style="color:' + col + '">' + l + '</div>';
      }
      term.innerHTML = html;
      term.scrollTop = term.scrollHeight;
    })
    .catch(function(err) { console.log(err); });
}

function clearLogs() {
  fetch('/api/clear_logs', { method: 'POST' })
    .then(function() {
      document.getElementById('termLogs').innerHTML = '<div style="color:#9ca3af;">(Đã xóa toàn bộ nhật ký)</div>';
    });
}

function scanWifi() {
  var btn = document.getElementById('scanBtn');
  var list = document.getElementById('wifiScanResults');
  btn.innerText = 'Đang quét...';
  btn.disabled = true;

  fetch('/api/scan')
    .then(function(res) { return res.json(); })
    .then(function(networks) {
      btn.innerText = '🔍 Quét lại';
      btn.disabled = false;
      if (networks.length === 0) {
        list.innerHTML = '<div style="font-size:11px;color:#9ca3af;text-align:center;padding:12px;">Không tìm thấy mạng Wi-Fi nào.</div>';
        return;
      }
      var html = '';
      for (var i = 0; i < networks.length; i++) {
        var w = networks[i];
        html += '<div class="wifi-item" onclick="selectWifi(\'' + encodeURIComponent(w.ssid) + '\')">';
        html += '  <div>';
        html += '    <div class="wifi-item-title">' + w.ssid + '</div>';
        html += '    <div class="wifi-item-sub">Kênh: ' + w.ch + ' ' + (w.secure ? '🔒' : '🔓') + '</div>';
        html += '  </div>';
        html += '  <div class="wifi-item-rssi" style="color:' + (w.rssi > -70 ? '#34d399' : '#f59e0b') + '">' + w.rssi + ' dBm</div>';
        html += '</div>';
      }
      list.innerHTML = html;
    })
    .catch(function(err) {
      btn.innerText = '🔍 Quét lại';
      btn.disabled = false;
      list.innerHTML = '<div style="font-size:11px;color:#f87171;text-align:center;padding:12px;">Lỗi khi quét mạng!</div>';
    });
}

function selectWifi(encodedSsid) {
  var ssid = decodeURIComponent(encodedSsid);
  document.getElementById('staSsidInput').value = ssid;
  document.getElementById('staPassInput').value = '';
  document.getElementById('staPassInput').focus();
}

function saveSta() {
  var ssid = document.getElementById('staSsidInput').value;
  var pass = document.getElementById('staPassInput').value;
  if (!ssid) {
    alert('Vui lòng nhập tên Wi-Fi nguồn!');
    return;
  }
  var data = new URLSearchParams();
  data.append('ssid', ssid);
  data.append('pass', pass);

  fetch('/api/save_sta', { method: 'POST', body: data })
    .then(function(res) { return res.json(); })
    .then(function(d) {
      alert(d.msg || 'Đã lưu Wi-Fi nguồn! ESP32 đang kết nối lại...');
      showTab(1);
      setTimeout(updateStatus, 3000);
    })
    .catch(function(err) { alert('Lỗi khi gửi yêu cầu!'); });
}

function saveAp() {
  var ssid = document.getElementById('apSsidInput').value;
  var pass = document.getElementById('apPassInput').value;
  var ch = document.getElementById('apChannelSelect').value;
  var bssid = document.getElementById('apBssidInput').value;
  var ip = document.getElementById('apIpInput').value;
  var subnet = document.getElementById('apSubnetInput').value;
  var start = document.getElementById('dhcpStartInput').value;
  var end = document.getElementById('dhcpEndInput').value;

  if (!ssid || !ip || !subnet) {
    alert('Vui lòng điền đầy đủ các trường bắt buộc!');
    return;
  }

  var data = new URLSearchParams();
  data.append('ssid', ssid);
  data.append('pass', pass);
  data.append('channel', ch);
  data.append('bssid', bssid);
  data.append('ip', ip);
  data.append('subnet', subnet);
  data.append('dhcp_start', start);
  data.append('dhcp_end', end);

  fetch('/api/save_ap', { method: 'POST', body: data })
    .then(function(res) { return res.json(); })
    .then(function(d) {
      alert(d.msg || 'Đã lưu cấu hình mạng phát! ESP32 đang khởi động lại...');
    })
    .catch(function(err) { alert('Lỗi khi lưu tham số AP!'); });
}

var cachedBinUrl = '';

function saveOtaUrl() {
  var url = document.getElementById('inputOtaUrl').value.trim();
  if (!url) {
    alert('Vui lòng nhập đường dẫn version.json!');
    return;
  }
  var data = new URLSearchParams();
  data.append('url', url);

  fetch('/api/save_ota_url', { method: 'POST', body: data })
    .then(function(res) { return res.json(); })
    .then(function(d) {
      if (d.success) {
        alert('Đã lưu URL GitHub OTA thành công!');
      } else {
        alert('Lỗi: ' + (d.error || 'Không thể lưu URL'));
      }
    })
    .catch(function(err) { alert('Lỗi kết nối khi lưu URL!'); });
}

function checkGithubOta() {
  var btn = document.getElementById('btnCheckOta');
  var txt = document.getElementById('checkOtaText');
  var icon = document.getElementById('checkOtaIcon');
  var resultBox = document.getElementById('otaResultBox');

  btn.disabled = true;
  txt.innerText = 'Đang kết nối GitHub...';
  icon.innerText = '⏳';

  fetch('/api/check_ota', { method: 'POST' })
    .then(function(res) { return res.json(); })
    .then(function(d) {
      btn.disabled = false;
      txt.innerText = 'Kiểm Tra Bản Cập Nhật Mới';
      icon.innerText = '🔍';

      if (!d.success) {
        alert('Không thể kiểm tra OTA: ' + (d.error || 'Lỗi không xác định'));
        return;
      }

      if (!d.has_update) {
        alert('Thiết bị đang chạy phiên bản mới nhất (' + d.current_ver + ')!\nKhông có bản cập nhật mới trên GitHub.');
        resultBox.classList.add('hidden');
        return;
      }

      cachedBinUrl = d.bin_url;
      document.getElementById('otaNewVerText').innerText = 'v' + d.remote_ver + ' (Hiện tại: v' + d.current_ver + ')';
      document.getElementById('otaChangelogText').innerText = d.changelog || 'Bản nâng cấp tối ưu hóa hệ thống và sửa lỗi từ GitHub.';
      resultBox.classList.remove('hidden');
    })
    .catch(function(err) {
      btn.disabled = false;
      txt.innerText = 'Kiểm Tra Bản Cập Nhật Mới';
      icon.innerText = '🔍';
      alert('Lỗi kết nối khi gửi yêu cầu kiểm tra OTA!');
    });
}

function startGithubOta() {
  if (!cachedBinUrl) {
    alert('Chưa có thông tin file esp32-firmware.bin!');
    return;
  }
  if (!confirm('XÁC NHẬN NÂNG CẤP FIRMWARE:\n\nESP32 sẽ tải file từ GitHub và ghi đè vào phân vùng Flash.\nQuá trình này mất khoảng 30 - 60 giây.\nKhông được tắt nguồn thiết bị!\n\nBạn có muốn bắt đầu không?')) {
    return;
  }

  var btnRun = document.getElementById('btnRunOta');
  var progBox = document.getElementById('otaProgressBox');
  var pBar = document.getElementById('otaProgressBar');
  var pTxt = document.getElementById('otaProgressStatus');
  var pPct = document.getElementById('otaProgressPercent');

  btnRun.classList.add('hidden');
  progBox.classList.remove('hidden');

  var data = new URLSearchParams();
  data.append('bin_url', cachedBinUrl);

  fetch('/api/run_ota', { method: 'POST', body: data })
    .then(function(res) { return res.json(); })
    .then(function(d) {
      if (!d.success) {
        alert('Lỗi khởi động OTA: ' + (d.error || 'Không thể bắt đầu'));
        btnRun.classList.remove('hidden');
        progBox.classList.add('hidden');
        return;
      }

      // Mô phỏng thanh tiến trình tải và ghi Flash
      var progress = 0;
      var timer = setInterval(function() {
        progress += 2;
        if (progress > 95) progress = 95;
        pBar.style.width = progress + '%';
        pPct.innerText = progress + '%';
        pTxt.innerText = 'Đang tải firmware & ghi Flash (' + progress + '%)...';

        if (progress >= 95) {
          clearInterval(timer);
          pTxt.innerText = 'ESP32 đang hoàn tất ghi và khởi động lại...';
          setTimeout(function() {
            pBar.style.width = '100%';
            pPct.innerText = '100%';
            pTxt.innerText = 'Đang tải lại giao diện...';
            setTimeout(function() {
              location.reload();
            }, 3000);
          }, 8000);
        }
      }, 500);
    })
    .catch(function(err) {
      alert('Mất kết nối với ESP32 (Thiết bị có thể đang nạp Flash và khởi động lại). Vui lòng đợi 15 giây rồi tải lại trang.');
    });
}

// Chạy cập nhật trạng thái ban đầu và định kỳ
updateStatus();
setInterval(updateStatus, 4000);
setInterval(function() {
  if (currentTab === 4) {
    fetchLogs();
  }
}, 2500);
</script>
</body>
</html>
)rawliteral";

#endif // INDEX_H
