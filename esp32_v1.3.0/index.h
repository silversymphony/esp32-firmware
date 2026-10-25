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
  .header-right { display: flex; align-items: center; gap: 8px; }
  .lang-switch { display: flex; background: var(--card-inner); border: 1px solid var(--border); border-radius: 8px; padding: 2px; gap: 2px; }
  .lang-btn { background: transparent; border: none; color: var(--text-muted); padding: 5px 8px; font-size: 10px; font-weight: 700; border-radius: 6px; cursor: pointer; transition: 0.2s; line-height: 1; user-select: none; }
  .lang-btn:hover { color: #fff; }
  .lang-btn.active { background: var(--accent); color: #fff; box-shadow: 0 1px 3px rgba(0,0,0,0.3); }
  .toast { position: fixed; top: 16px; left: 50%; transform: translateX(-50%); background: #111827; border: 1px solid var(--accent); color: #67e8f9; padding: 8px 16px; border-radius: 9999px; font-size: 11px; font-weight: 700; z-index: 9999; box-shadow: 0 4px 16px rgba(0,0,0,0.6); pointer-events: none; transition: opacity 0.3s ease; }
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
  
  <!-- Toast Notification Box -->
  <div id="toastBox" class="toast hidden"></div>

  <!-- Header with Bilingual Language Switcher -->
  <div class="header">
    <div class="header-left">
      <div class="header-icon">📶</div>
      <div>
        <div class="header-title" id="headerTitle">ESP32 Gateway Manager</div>
        <div class="header-ip" id="headIp">10.51.31.254</div>
      </div>
    </div>
    <div class="header-right">
      <div class="lang-switch">
        <button type="button" class="lang-btn active" id="btnLangVi" onclick="setLanguage('vi')">🇻🇳 VI</button>
        <button type="button" class="lang-btn" id="btnLangEn" onclick="setLanguage('en')">🇬🇧 EN</button>
      </div>
      <div id="connBadge" class="badge badge-danger">ĐANG KIỂM TRA</div>
    </div>
  </div>

  <!-- Captive Portal Banner -->
  <div id="captiveAlert" class="captive-alert hidden">
    <div style="font-size: 20px;">⚠️</div>
    <div>
      <div id="captiveTitle" style="font-size: 12px; font-weight: 700; color: #fbbf24; margin-bottom: 2px;">Chưa kết nối Internet! (Captive Portal)</div>
      <div id="captiveDesc" style="font-size: 11px; color: #fef3c7; line-height: 1.4;">Vui lòng chuyển sang tab <b>Wi-Fi Nguồn</b> để quét và chọn Wi-Fi cấp mạng cho hệ thống.</div>
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
        <span id="cardTitleSta">🌐 Wi-Fi Nguồn (Internet STA)</span>
        <span id="staStatusText" style="font-size: 11px; color: #9ca3af;">Đang tải...</span>
      </div>
      <div class="grid-2">
        <div class="info-box">
          <div class="info-label" id="lblStaSsid">Tên mạng nguồn</div>
          <div class="info-val" id="valStaSsid">---</div>
        </div>
        <div class="info-box">
          <div class="info-label" id="lblStaPass">Mật khẩu nguồn</div>
          <div class="info-val" style="color: #fde047;" id="valStaPass">---</div>
        </div>
        <div class="info-box">
          <div class="info-label" id="lblStaRssi">Cường độ sóng (RSSI)</div>
          <div class="info-val" id="valStaRssi">---</div>
        </div>
        <div class="info-box">
          <div class="info-label" id="lblStaIp">IP ESP32 nhận được</div>
          <div class="info-val" style="color: #67e8f9;" id="valStaIp">---</div>
        </div>
        <div class="info-box">
          <div class="info-label" id="lblStaGw">Gateway nguồn</div>
          <div class="info-val" id="valStaGw">---</div>
        </div>
        <div class="info-box">
          <div class="info-label" id="lblStaSubnet">Subnet Mask nguồn</div>
          <div class="info-val" id="valStaSubnet">---</div>
        </div>
        <div class="info-box" style="grid-column: span 2;">
          <div class="info-label" id="lblStaBssid">BSSID (MAC Router nguồn)</div>
          <div class="info-val" style="color: #fde047;" id="valStaBssid">---</div>
        </div>
      </div>

      <!-- Nút sao chép cấu hình Wi-Fi nguồn thành Preset AP -->
      <button type="button" id="btnCloneStaTab1" class="btn" style="background: linear-gradient(135deg, #0284c7, #2563eb); color: #fff; font-size: 11px; padding: 9px 12px; margin-top: 10px;" onclick="cloneStaToPreset()">
        📥 Sao Chép Wi-Fi Nguồn Sang Preset AP
      </button>
    </div>

    <!-- Card AP -->
    <div class="card">
      <div class="card-title">
        <span id="cardTitleAp">📡 Mạng Phát (SoftAP & Gateway)</span>
        <div style="display: flex; gap: 4px;">
          <span id="valPowerBadge" class="badge badge-blue">15 dBm TIÊU CHUẨN</span>
          <span id="valApChannelBadge" class="badge badge-blue">KÊNH 6</span>
        </div>
      </div>
      <div class="grid-2">
        <div class="info-box">
          <div class="info-label" id="lblApSsid">Tên Wi-Fi phát ra</div>
          <div class="info-val" id="valApSsid">---</div>
        </div>
        <div class="info-box">
          <div class="info-label" id="lblApPass">Mật khẩu phát SoftAP</div>
          <div class="info-val" style="color: #fbbf24;" id="valApPass">---</div>
        </div>
        <div class="info-box">
          <div class="info-label" id="lblApBssid">Địa chỉ BSSID (MAC)</div>
          <div class="info-val" style="color: #fde047;" id="valApBssid">---</div>
        </div>
        <div class="info-box">
          <div class="info-label" id="lblApCh">Kênh phát sóng</div>
          <div class="info-val" id="valApChannel">---</div>
        </div>
        <div class="info-box">
          <div class="info-label" id="lblApIp">IP Gateway</div>
          <div class="info-val" style="color: #67e8f9;" id="valApIp">---</div>
        </div>
        <div class="info-box">
          <div class="info-label" id="lblApSubnet">Subnet Mask</div>
          <div class="info-val" id="valApSubnet">---</div>
        </div>
        <div class="info-box" style="grid-column: span 2;">
          <div class="info-label" id="lblDhcpPool">Cấp IP Ngẫu Nhiên Mỗi Lần Kết Nối (Random DHCP)</div>
          <div class="info-val" style="color: #a7f3d0;" id="valDhcpPool">10.51.28.10  ➔  10.51.28.200 (Ngẫu nhiên)</div>
        </div>
      </div>
    </div>

    <!-- Card BLE Status -->
    <div class="card" style="padding: 12px; display: flex; align-items: center; justify-content: space-between;">
      <div style="display: flex; align-items: center; gap: 10px;">
        <span style="font-size: 20px;">ᛒ</span>
        <div>
          <div id="lblBleTitle" style="font-size: 12px; font-weight: 700; color: #93c5fd;">Bluetooth LE Bảo Mật (PIN)</div>
          <div style="font-size: 10px; color: #9ca3af; font-family: monospace;"><span id="lblBleSub">Tên: ESP32-Gateway-BLE • PIN: </span><span id="dashBlePin" style="color:#fbbf24; font-weight:bold;">686868</span></div>
        </div>
      </div>
      <span id="valBleBadge" class="badge badge-success">ĐÃ BẬT</span>
    </div>

    <!-- Card Sys -->
    <div class="card" style="padding: 12px;">
      <div style="display: flex; justify-content: space-around; text-align: center;">
        <div>
          <div class="info-label" id="lblHeap">RAM Trống</div>
          <div class="info-val" style="color: #34d399;" id="valHeap">---</div>
        </div>
        <div style="width: 1px; background: var(--border);"></div>
        <div>
          <div class="info-label" id="lblUptime">Uptime</div>
          <div class="info-val" id="valUptime">---</div>
        </div>
        <div style="width: 1px; background: var(--border);"></div>
        <div>
          <div class="info-label" id="lblFwVer">Firmware</div>
          <div class="info-val" style="color: #fbbf24;" id="valFwVer">v1.3.0</div>
        </div>
        <div style="width: 1px; background: var(--border);"></div>
        <div>
          <div class="info-label" id="lblNat">Định tuyến NAT</div>
          <div class="info-val" style="color: #38bdf8;" id="valNat">ON</div>
        </div>
      </div>
    </div>
  </div>

  <!-- TAB 2: CÀI ĐẶT WI-FI NGUỒN -->
  <div id="tab2" class="hidden">
    <!-- Card Chế độ Wi-Fi Nguồn / Mạng nội bộ 5G -->
    <div class="card" style="margin-bottom: 12px; display: flex; align-items: center; justify-content: space-between;">
      <div>
        <div style="font-weight: 700; font-size: 13px;" id="lblStaToggleTitle">Chế độ Wi-Fi Nguồn (Internet Repeater)</div>
        <div style="font-size: 11px; color: var(--text-muted); margin-top: 2px;" id="lblStaToggleDesc">Tắt để biến ESP32 thành Mạng nội bộ Offline (Điện thoại dùng 5G)</div>
      </div>
      <label class="switch">
        <input type="checkbox" id="staEnableSwitch" onchange="toggleStaMode(this.checked)">
        <span class="slider"></span>
      </label>
    </div>

    <!-- Alert Mạng Nội Bộ -->
    <div id="staDisabledAlert" class="card hidden" style="background: rgba(37, 99, 235, 0.15); border: 1px solid rgba(37, 99, 235, 0.4); margin-bottom: 12px; padding: 12px;">
      <div style="display: flex; align-items: center; gap: 8px;">
        <span style="font-size: 18px;">📶</span>
        <div>
          <div style="font-size: 12px; font-weight: 700; color: #60a5fa;" id="lblStaOfflineTitle">Chế độ Mạng Nội Bộ Đang Bật (Dùng 5G)</div>
          <div style="font-size: 11px; color: #cbd5e1; margin-top: 2px;" id="lblStaOfflineDesc">Wi-Fi nguồn đã tắt. Điện thoại kết nối vào Wi-Fi này sẽ tự động dùng dữ liệu di động 5G để truy cập Internet mà không bị gián đoạn.</div>
        </div>
      </div>
    </div>

    <div class="card">
      <div class="card-title">
        <span id="cardTitleScan">Quét Mạng Wi-Fi 2.4GHz</span>
        <button class="btn-scan" id="scanBtn" onclick="scanWifi(true)">🔍 Quét mạng</button>
      </div>
      <div class="wifi-list" id="wifiScanResults">
        <div id="scanPlaceholder" style="font-size: 11px; color: var(--text-muted); text-align: center; padding: 12px;">Bấm nút "Quét mạng" để tìm Wi-Fi xung quanh...</div>
      </div>

      <div style="border-top: 1px solid var(--border); padding-top: 12px;">
        <div class="form-group">
          <label id="lblInputStaSsid">Tên Wi-Fi Nguồn (SSID)</label>
          <input type="text" id="staSsidInput" placeholder="Nhập hoặc chọn Wi-Fi bên trên">
        </div>
        <div class="form-group">
          <label id="lblInputStaPass">Mật khẩu Wi-Fi Nguồn</label>
          <div class="pass-wrapper">
            <input type="password" id="staPassInput" placeholder="Nhập mật khẩu (để trống nếu mở)">
            <button class="pass-toggle" type="button" onclick="togglePass('staPassInput', this)">👁️</button>
          </div>
        </div>
        <div class="notice" id="noticeSta">💾 Cấu hình sẽ được lưu vĩnh viễn vào bộ nhớ Flash ESP32 (Preferences NVS).</div>
        <button class="btn btn-primary" id="btnSaveSta" onclick="saveSta()">🚀 Lưu Cấu Hình & Kết Nối</button>
      </div>
    </div>
  </div>

  <!-- TAB 3: CÀI ĐẶT MẠNG PHÁT (AP) & MAX POWER BOOST -->
  <div id="tab3" class="hidden">

    <!-- Card Quản Lý Danh Sách Mẫu Mạng Phát (Presets) -->
    <div class="card" style="background: linear-gradient(135deg, rgba(30, 58, 138, 0.22), rgba(17, 24, 39, 0.95)); border: 1px solid rgba(59, 130, 246, 0.4);">
      <div class="card-title">
        <span style="display: flex; align-items: center; gap: 6px;">
          <span>📋</span>
          <span id="presetTitle" style="color: #93c5fd;">Mẫu Cấu Hình Mạng Phát (Presets)</span>
        </span>
        <span id="presetCountBadge" class="badge badge-blue">0 MẪU</span>
      </div>

      <div class="form-group" style="margin-bottom: 8px;">
        <label id="lblSelectPreset">Chọn nhanh cấu hình đã lưu:</label>
        <div style="display: flex; gap: 8px;">
          <select id="profileSelect" style="font-size: 11px;" onchange="onProfileSelectChange(this.value)">
            <option value="">-- Bấm để chọn cấu hình --</option>
          </select>
          <button type="button" id="btnLoadPreset" class="btn btn-primary" style="width: auto; padding: 0 12px; font-size: 11px; white-space: nowrap;" onclick="loadProfileToForm()">
            📋 Nạp vào Form
          </button>
        </div>
      </div>

      <div class="form-group" style="margin-bottom: 8px;">
        <label id="lblPresetName">Tên mẫu cấu hình:</label>
        <input type="text" id="presetNameInput" placeholder="Ví dụ: Wi-Fi Quán Cà Phê" style="font-size: 11px;">
      </div>

      <div style="display: grid; grid-template-columns: repeat(3, 1fr); gap: 6px; margin-bottom: 8px;">
        <button type="button" id="btnApplyPreset" class="btn btn-success" style="font-size: 11px; padding: 7px 4px;" onclick="applySelectedProfile()">
          🚀 Áp dụng & Reboot
        </button>
        <button type="button" id="btnUpdatePreset" class="btn" style="background: #2563eb; color: #fff; font-size: 11px; padding: 7px 4px;" onclick="updateSelectedProfile()">
          💾 Lưu Cập Nhật
        </button>
        <button type="button" id="btnDeletePreset" class="btn" style="background: #7f1d1d; color: #fecaca; font-size: 11px; padding: 7px 4px;" onclick="deleteSelectedProfile()">
          🗑️ Xóa mẫu
        </button>
      </div>

      <div style="border-top: 1px dashed rgba(255,255,255,0.1); padding-top: 8px; display: flex; flex-direction: column; gap: 6px;">
        <button type="button" id="btnSaveNewPreset" class="btn" style="background: linear-gradient(135deg, #10b981, #059669); color: #fff; font-size: 11px; padding: 8px 12px;" onclick="saveCurrentAsNewProfile()">
          ➕ Lưu form bên dưới thành mẫu mới
        </button>
        <button type="button" id="btnCloneStaPreset" class="btn" style="background: linear-gradient(135deg, #0284c7, #2563eb); color: #fff; font-size: 11px; padding: 8px 12px;" onclick="cloneStaToPreset()">
          📥 Sao chép từ Wi-Fi Nguồn thành Preset mới
        </button>
      </div>
    </div>

    <div class="card">
      <div class="card-title">
        <span id="cardTitleApConfig">Cấu Hình Mạng Phát (SoftAP & Gateway)</span>
      </div>

      <!-- Max TX Power Boost Switch -->
      <div class="switch-box">
        <div>
          <div class="switch-label-title" id="boostTitle">⚡ Phát Sóng Cực Đại (Max TX Power Boost)</div>
          <div class="switch-label-desc" id="boostDesc">Mức công suất tối đa phần cứng <b>20 dBm (~100mW)</b></div>
        </div>
        <label class="switch">
          <input type="checkbox" id="powerBoostToggle" onchange="togglePowerBoost(this)">
          <span class="slider slider-amber"></span>
        </label>
      </div>
      <div id="powerBoostNotice" class="notice" style="color: #60a5fa;">🍃 <b>Mức tiêu chuẩn 15 dBm:</b> Tiết kiệm điện năng, giúp chip ESP32 mát hơn.</div>
      
      <div class="form-group">
        <label id="lblInputApSsid">Tên Wi-Fi Phát Ra (AP SSID)</label>
        <input type="text" id="apSsidInput" placeholder="ODE - wireless WiFi">
      </div>

      <div class="form-group">
        <label id="lblInputApPass">Mật Khẩu Phát (để trống nếu Open)</label>
        <div class="pass-wrapper">
          <input type="password" id="apPassInput" placeholder="Ode6868@">
          <button class="pass-toggle" type="button" onclick="togglePass('apPassInput', this)">👁️</button>
        </div>
      </div>

      <div class="grid-2">
        <div class="form-group">
          <label id="lblApChannel">Kênh Sóng (Channel)</label>
          <select id="apChannelSelect" onchange="onChannelChange(this.value)">
            <option value="0" id="optCh0">⚡ Kênh 0 - Tự Động Quét Chọn Kênh Tối Ưu (Auto)</option>
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
          </select>
        </div>
        <div class="form-group">
          <label id="lblInputApBssid">Địa Chỉ BSSID (MAC)</label>
          <input type="text" id="apBssidInput" placeholder="84:F1:47:4F:27:A0">
        </div>
      </div>

      <div class="grid-2">
        <div class="form-group">
          <label id="lblInputApIp">IP Gateway (Cổng ESP32)</label>
          <input type="text" id="apIpInput" placeholder="10.51.31.254">
        </div>
        <div class="form-group">
          <label id="lblInputApSubnet">Subnet Mask</label>
          <input type="text" id="apSubnetInput" placeholder="255.255.252.0">
        </div>
      </div>

      <div class="info-box" style="margin-bottom: 12px;">
        <label id="lblInputDhcp" style="color: #cbd5e1; margin-bottom: 6px;">Dải Cấp IP Tự Động (DHCP Ngẫu Nhiên - Không Giới Hạn)</label>
        <div class="grid-2">
          <div>
            <span id="lblDhcpStart" style="font-size: 10px; color: var(--text-muted);">Bắt đầu:</span>
            <input type="text" id="dhcpStartInput" placeholder="10.51.28.10">
          </div>
          <div>
            <span id="lblDhcpEnd" style="font-size: 10px; color: var(--text-muted);">Kết thúc:</span>
            <input type="text" id="dhcpEndInput" placeholder="10.51.28.200">
          </div>
        </div>
      </div>

      <div id="noticeDhcp" class="notice" style="color: #fde68a;">🎲 Hệ thống tự động bốc ngẫu nhiên 1 IP trong dải này cho thiết bị mỗi lần kết nối!</div>
      <button class="btn btn-success" id="btnSaveAp" onclick="saveAp()">💾 Lưu Tham Số & Khởi Động Lại AP</button>
    </div>
  </div>

  <!-- TAB 4: WIRELESS CONSOLE LOG & GITHUB OTA -->
  <div id="tab4" class="hidden">
    
    <!-- Quản Lý Từ Xa Qua Internet (Cloud Remote Access) -->
    <div class="card" style="background: linear-gradient(135deg, rgba(14, 165, 233, 0.12), rgba(17, 24, 39, 0.9)); border-color: rgba(56, 189, 248, 0.35);">
      <div class="card-title">
        <span style="display: flex; align-items: center; gap: 6px;">
          <span>☁️</span>
          <span id="cardTitleCloud" style="color: #38bdf8;">Quản Lý Từ Xa Qua Internet (Cloud Remote)</span>
        </span>
        <span id="badgeCloudStatus" class="badge badge-danger">OFFLINE</span>
      </div>

      <!-- Switch Bật/Tắt Cloud Remote -->
      <div class="switch-box switch-box-blue" style="margin-bottom: 12px;">
        <div>
          <div class="switch-label-title" id="cloudToggleTitle" style="color: #38bdf8;">Kích hoạt Quản lý Từ xa</div>
          <div class="switch-label-desc" id="cloudToggleDesc">Kết nối ngầm tới Cloudflare Worker để quản trị từ bất kỳ đâu qua Internet</div>
        </div>
        <label class="switch">
          <input type="checkbox" id="cloudRemoteToggle" onchange="toggleCloudRemote(this.checked)">
          <span class="slider"></span>
        </label>
      </div>

      <div class="form-group">
        <label id="lblCloudUrl">Cloudflare Worker URL</label>
        <input type="text" id="inputCloudUrl" placeholder="https://my-esp32.workers.dev" style="font-size: 11px;">
      </div>

      <div class="form-group">
        <label id="lblCloudKey">Mã Khóa Xác Thực Bí Mật (Secret Access Key)</label>
        <div class="pass-wrapper">
          <input type="password" id="inputCloudKey" placeholder="Nhập khóa bảo mật (VD: 686868)">
          <button class="pass-toggle" type="button" onclick="togglePass('inputCloudKey', this)">👁️</button>
        </div>
      </div>

      <div style="display: flex; gap: 8px; margin-top: 10px;">
        <button class="btn btn-primary" id="btnSaveCloud" onclick="saveCloudConfig()" style="flex: 1;">💾 Lưu Cấu Hình Cloud</button>
        <a id="btnOpenCloudLink" href="#" target="_blank" class="btn hidden" style="flex: 1; background: #0284c7; color: #fff; text-decoration: none; display: flex; align-items: center; justify-content: center; gap: 4px; font-weight: 700;">🌐 Mở Web Từ Xa ↗</a>
      </div>

      <div id="cloudExplanation" class="notice" style="color: #cbd5e1; margin-top: 10px; margin-bottom: 0;">
        💡 <b>Cách hoạt động:</b> Khi ESP32 có Internet, nó sẽ mở một kết nối bảo mật HTTPS ra ngoài tới Cloudflare Worker. Nhờ đó, bạn có thể truy cập Web Portal từ xa qua 4G/5G hoặc bất cứ đâu mà không cần mở cổng modem (Port Forwarding), không sợ nhà mạng chặn CGNAT!
      </div>
    </div>

    <!-- Nâng Cấp Firmware Không Dây (GitHub OTA) -->
    <div class="card" style="background: linear-gradient(135deg, rgba(79, 70, 229, 0.12), rgba(17, 24, 39, 0.9)); border-color: rgba(99, 102, 241, 0.35);">
      <div class="card-title">
        <span style="display: flex; align-items: center; gap: 6px;">
          <span>🚀</span>
          <span id="cardTitleOta" style="color: #a5b4fc;">Nâng Cấp Firmware Không Dây (GitHub OTA)</span>
        </span>
        <span id="badgeFwVerOta" class="badge" style="background: rgba(99, 102, 241, 0.2); color: #c7d2fe; border: 1px solid rgba(99, 102, 241, 0.4);">v1.3.0</span>
      </div>

      <div class="form-group">
        <label id="lblOtaUrl">Đường dẫn GitHub Version (version.json)</label>
        <div style="display: flex; gap: 8px;">
          <input type="text" id="inputOtaUrl" value="https://raw.githubusercontent.com/silversymphony/esp32-firmware/main/version.json" style="font-size: 11px;">
          <button class="btn btn-primary" id="btnSaveOtaUrl" style="width: auto; padding: 0 14px; font-size: 11px;" onclick="saveOtaUrl()">Lưu URL</button>
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
            🎉 <span id="otaFoundTitle">Tìm thấy bản mới:</span> <span id="otaNewVerText" style="color: #fbbf24; font-family: monospace;">---</span>
          </div>
          <span id="otaReadyTag" style="font-size: 10px; color: #9ca3af; font-family: monospace;">Sẵn sàng tải</span>
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
          <span id="btnRunOtaText">Bắt Đầu Tải & Ghi Flash Ngay</span>
        </button>
      </div>

      <div id="otaExplanation" class="notice" style="color: #cbd5e1; margin-top: 10px; margin-bottom: 0;">
        💡 <b>Cơ chế hoạt động:</b> ESP32 kết nối trực tiếp đến GitHub bằng giao thức bảo mật HTTPS, tải file <code>esp32-firmware.bin</code> và nạp tự động vào phân vùng dự phòng (Dual OTA Partition). Sau khi nạp xong, chip tự restart sang bản mới an toàn 100%.
      </div>
    </div>

    <!-- Quản trị & Bảo mật Bluetooth LE -->
    <div class="card">
      <div class="card-title">
        <span id="cardTitleBle">🛡️ Bảo Mật & Quản Trị Bluetooth LE</span>
        <span id="bleStatusBadgeTab4" class="badge badge-success">ĐANG BẬT</span>
      </div>

      <!-- Switch Bật / Tắt sóng Bluetooth -->
      <div class="switch-box switch-box-blue">
        <div>
          <div class="switch-label-title" id="bleRadioTitle" style="color: #93c5fd;">Sóng Bluetooth LE (BLE Radio)</div>
          <div class="switch-label-desc" id="bleRadioDesc">Tắt hoàn toàn Bluetooth để tàng hình và tiết kiệm điện</div>
        </div>
        <label class="switch">
          <input type="checkbox" id="bleRadioToggle" onchange="toggleBleRadio(this)">
          <span class="slider"></span>
        </label>
      </div>

      <!-- Đổi mã PIN Bluetooth -->
      <div class="form-group" style="margin-top: 8px;">
        <label id="lblBlePin">Mã PIN Ghép Đôi Bảo Mật (Đúng 6 Chữ Số)</label>
        <div style="display: flex; gap: 8px;">
          <div class="pass-wrapper" style="flex: 1;">
            <input type="password" id="inputBlePin" maxlength="6" placeholder="VD: 686868" style="font-weight: 700; letter-spacing: 2px; text-align: center; color: #fbbf24;">
            <button class="pass-toggle" type="button" onclick="togglePass('inputBlePin', this)">👁️</button>
          </div>
          <button class="btn btn-primary" id="btnSavePin" style="width: auto; padding: 0 16px; font-size: 11px;" onclick="saveBlePin()">💾 Lưu PIN</button>
        </div>
        <div id="hintBlePin" style="font-size: 10px; color: #9ca3af; margin-top: 4px;">Dùng mã PIN này khi điện thoại yêu cầu ghép đôi Bluetooth.</div>
      </div>

      <div id="bleExplanation" class="notice" style="color: #cbd5e1; line-height: 1.5;">
        🔒 <b>Bảo mật bằng mã PIN phần cứng (BLE Passkey):</b><br>
        • Khi điện thoại kết nối, hệ điều hành sẽ yêu cầu nhập mã PIN 6 số để ghép đôi mã hóa.<br>
        • Sau khi ghép đôi thành công, log sẽ tự động truyền trực tiếp không dây và có thể gửi lệnh ngay mà <b>không cần bước xác thực phụ</b>.<br>
        • <b>Đổi mã PIN nhanh qua BLE:</b> Gõ lệnh <code style="color: #34d399;">SETPIN:<mã_mới></code> trên terminal.
      </div>
    </div>

    <!-- Console Terminal -->
    <div class="card">
      <div class="card-title">
        <span id="cardTitleTerm">📟 Nhật Ký Hoạt Động (Wireless Console Log)</span>
        <button class="btn-scan" id="btnClearLogs" onclick="clearLogs()">Xóa log</button>
      </div>
      <div class="terminal" id="termLogs">
        <div>[DANG TAI NHAT KY HE THONG...]</div>
      </div>
    </div>

  </div>

  <!-- Footer with Firmware Version -->
  <div class="footer">
    <div id="footerDesc">ESP32 NAPT Repeater Gateway • Built with Custom Random DHCP, BLE Security & GitHub OTA</div>
    <div style="margin-top: 5px; color: #9ca3af; font-size: 11px;">
      <span id="footerFwLabel">Firmware:</span> <span id="footerFwVer" style="color: #fbbf24; font-weight: 700;">v1.3.0</span>
    </div>
  </div>
</div>

<script>
var currentTab = 1;
var lastStatusData = null;

// Quản lý bộ nhớ lưu trữ ngôn ngữ an toàn (chống lỗi Safari & Captive Portal)
function getStoredLang() {
  try {
    if (typeof localStorage !== 'undefined' && localStorage) {
      return localStorage.getItem('portal_lang') || 'vi';
    }
  } catch (e) {}
  return 'vi';
}

function setStoredLang(lang) {
  try {
    if (typeof localStorage !== 'undefined' && localStorage) {
      localStorage.setItem('portal_lang', lang);
    }
  } catch (e) {}
}

var currentLang = getStoredLang();

function showToast(msg) {
  var t = document.getElementById('toastBox');
  if (!t) return;
  t.innerText = msg;
  t.classList.remove('hidden');
  t.style.opacity = '1';
  if (window.toastTimer) clearTimeout(window.toastTimer);
  window.toastTimer = setTimeout(function() {
    t.style.opacity = '0';
    setTimeout(function() { t.classList.add('hidden'); }, 300);
  }, 1600);
}

var I18N = {
  vi: {
    headerTitle: "ESP32 Gateway Manager",
    captiveTitle: "Chưa kết nối Internet! (Captive Portal)",
    captiveDesc: "Vui lòng chuyển sang tab <b>Wi-Fi Nguồn</b> để quét và chọn Wi-Fi cấp mạng cho hệ thống.",
    btnTab1: "📊 Trạng thái",
    btnTab2: "🌐 Nguồn",
    btnTab3: "📡 Mạng Phát",
    btnTab4: "📟 Log & OTA",
    cardTitleSta: "🌐 Wi-Fi Nguồn (Internet STA)",
    lblStaSsid: "Tên mạng nguồn",
    lblStaPass: "Mật khẩu nguồn",
    lblStaRssi: "Cường độ sóng (RSSI)",
    lblStaIp: "IP ESP32 nhận được",
    lblStaGw: "Gateway nguồn",
    lblStaSubnet: "Subnet Mask nguồn",
    lblStaBssid: "BSSID (MAC Router nguồn)",
    btnCloneStaTab1: "📥 Sao Chép Wi-Fi Nguồn Sang Preset AP",
    cardTitleAp: "📡 Mạng Phát (SoftAP & Gateway)",
    lblApSsid: "Tên Wi-Fi phát ra",
    lblApPass: "Mật khẩu phát SoftAP",
    lblApBssid: "Địa chỉ BSSID (MAC)",
    lblApCh: "Kênh phát sóng",
    lblApIp: "IP Gateway",
    lblApSubnet: "Subnet Mask",
    lblDhcpPool: "Cấp IP Ngẫu Nhiên Mỗi Lần Kết Nối (Random DHCP)",
    lblBleTitle: "Bluetooth LE Bảo Mật (PIN)",
    lblBleSub: "Tên: ESP32-Gateway-BLE • PIN: ",
    lblHeap: "RAM Trống",
    lblUptime: "Uptime",
    lblFwVer: "Firmware",
    lblNat: "Định tuyến NAT",
    lblStaToggleTitle: "Chế độ Wi-Fi Nguồn (Internet Repeater)",
    lblStaToggleDesc: "Tắt để biến ESP32 thành Mạng nội bộ Offline (Điện thoại dùng 5G)",
    lblStaOfflineTitle: "Chế độ Mạng Nội Bộ Đang Bật (Dùng 5G)",
    lblStaOfflineDesc: "Wi-Fi nguồn đã tắt. Điện thoại kết nối vào Wi-Fi này sẽ tự động dùng dữ liệu di động 5G để truy cập Internet mà không bị gián đoạn.",
    cardTitleScan: "Quét Mạng Wi-Fi 2.4GHz",
    scanBtn: "🔍 Quét mạng",
    scanPlaceholder: "Bấm nút \"Quét mạng\" để tìm Wi-Fi xung quanh...",
    lblInputStaSsid: "Tên Wi-Fi Nguồn (SSID)",
    phStaSsid: "Nhập hoặc chọn Wi-Fi bên trên",
    lblInputStaPass: "Mật khẩu Wi-Fi Nguồn",
    phStaPass: "Nhập mật khẩu (để trống nếu mở)",
    noticeSta: "💾 Cấu hình sẽ được lưu vĩnh viễn vào bộ nhớ Flash ESP32 (Preferences NVS).",
    btnSaveSta: "🚀 Lưu Cấu Hình & Kết Nối",
    presetTitle: "Mẫu Cấu Hình Mạng Phát (Presets)",
    lblSelectPreset: "Chọn nhanh cấu hình đã lưu:",
    lblPresetName: "Tên mẫu cấu hình:",
    btnLoadPreset: "📋 Nạp vào Form",
    btnApplyPreset: "🚀 Áp dụng & Reboot",
    btnUpdatePreset: "💾 Lưu Cập Nhật",
    btnDeletePreset: "🗑️ Xóa mẫu",
    btnSaveNewPreset: "➕ Lưu form bên dưới thành mẫu mới",
    btnCloneStaPreset: "📥 Sao chép từ Wi-Fi Nguồn thành Preset mới",
    cardTitleApConfig: "Cấu Hình Mạng Phát (SoftAP & Gateway)",
    boostTitle: "⚡ Phát Sóng Cực Đại (Max TX Power Boost)",
    boostDesc: "Mức công suất tối đa phần cứng <b>20 dBm (~100mW)</b>",
    lblInputApSsid: "Tên Wi-Fi Phát Ra (AP SSID)",
    lblInputApPass: "Mật Khẩu Phát (để trống nếu Open)",
    lblApChannel: "Kênh Sóng (Channel)",
    optCh0: "⚡ Kênh 0 - Tự Động Quét Chọn Kênh Tối Ưu (Auto)",
    lblInputApBssid: "Địa Chỉ BSSID (MAC)",
    lblInputApIp: "IP Gateway (Cổng ESP32)",
    lblInputApSubnet: "Subnet Mask",
    lblInputDhcp: "Dải Cấp IP Tự Động (DHCP Ngẫu Nhiên - Không Giới Hạn)",
    lblDhcpStart: "Bắt đầu:",
    lblDhcpEnd: "Kết thúc:",
    noticeDhcp: "🎲 Hệ thống tự động bốc ngẫu nhiên 1 IP trong dải này cho thiết bị mỗi lần kết nối!",
    btnSaveAp: "💾 Lưu Tham Số & Khởi Động Lại AP",
    cardTitleCloud: "Quản Lý Từ Xa Qua Internet (Cloud Remote)",
    cloudToggleTitle: "Kích hoạt Quản lý Từ xa",
    cloudToggleDesc: "Kết nối ngầm tới Cloudflare Worker để quản trị từ bất kỳ đâu qua Internet",
    lblCloudUrl: "Cloudflare Worker URL",
    lblCloudKey: "Mã Khóa Xác Thực Bí Mật (Secret Access Key)",
    btnSaveCloud: "💾 Lưu Cấu Hình Cloud",
    cardTitleOta: "Nâng Cấp Firmware Không Dây (GitHub OTA)",
    lblOtaUrl: "Đường dẫn GitHub Version (version.json)",
    btnSaveOtaUrl: "Lưu URL",
    checkOtaText: "Kiểm Tra Bản Cập Nhật Mới",
    otaFoundTitle: "Tìm thấy bản mới:",
    otaReadyTag: "Sẵn sàng tải",
    btnRunOtaText: "Bắt Đầu Tải & Ghi Flash Ngay",
    otaExplanation: "💡 <b>Cơ chế hoạt động:</b> ESP32 kết nối trực tiếp đến GitHub bằng giao thức bảo mật HTTPS, tải file <code>esp32-firmware.bin</code> và nạp tự động vào phân vùng dự phòng (Dual OTA Partition). Sau khi nạp xong, chip tự restart sang bản mới an toàn 100%.",
    cardTitleBle: "🛡️ Bảo Mật & Quản Trị Bluetooth LE",
    bleRadioTitle: "Sóng Bluetooth LE (BLE Radio)",
    bleRadioDesc: "Tắt hoàn toàn Bluetooth để tàng hình và tiết kiệm điện",
    lblBlePin: "Mã PIN Ghép Đôi Bảo Mật (Đúng 6 Chữ Số)",
    btnSavePin: "💾 Lưu PIN",
    hintBlePin: "Dùng mã PIN này khi điện thoại yêu cầu ghép đôi Bluetooth.",
    bleExplanation: "🔒 <b>Bảo mật bằng mã PIN phần cứng (BLE Passkey):</b><br>• Khi điện thoại kết nối, hệ điều hành sẽ yêu cầu nhập mã PIN 6 số để ghép đôi mã hóa.<br>• Sau khi ghép đôi thành công, log sẽ tự động truyền trực tiếp không dây và có thể gửi lệnh ngay mà <b>không cần bước xác thực phụ</b>.<br>• <b>Đổi mã PIN nhanh qua BLE:</b> Gõ lệnh <code style=\"color: #34d399;\">SETPIN:&lt;mã_mới&gt;</code> trên terminal.",
    cardTitleTerm: "📟 Nhật Ký Hoạt Động (Wireless Console Log)",
    btnClearLogs: "Xóa log",
    footerDesc: "ESP32 NAPT Repeater Gateway • Built with Custom Random DHCP, BLE Security & GitHub OTA",
    footerFwLabel: "Firmware:"
  },
  en: {
    headerTitle: "ESP32 Gateway Manager",
    captiveTitle: "No Internet Connection! (Captive Portal)",
    captiveDesc: "Please switch to the <b>Source Wi-Fi</b> tab to scan and connect to an upstream Wi-Fi network.",
    btnTab1: "📊 Status",
    btnTab2: "🌐 Source",
    btnTab3: "📡 SoftAP",
    btnTab4: "📟 Log & OTA",
    cardTitleSta: "🌐 Source Wi-Fi (Internet STA)",
    lblStaSsid: "Source Network SSID",
    lblStaPass: "Source Password",
    lblStaRssi: "Signal Strength (RSSI)",
    lblStaIp: "ESP32 STA IP",
    lblStaGw: "Upstream Gateway",
    lblStaSubnet: "Source Subnet Mask",
    lblStaBssid: "BSSID (Source Router MAC)",
    btnCloneStaTab1: "📥 Clone Source Wi-Fi to SoftAP Preset",
    cardTitleAp: "📡 Broadcast Network (SoftAP & Gateway)",
    lblApSsid: "SoftAP Wi-Fi SSID",
    lblApPass: "SoftAP Password",
    lblApBssid: "BSSID Address (MAC)",
    lblApCh: "Broadcast Channel",
    lblApIp: "Gateway IP",
    lblApSubnet: "Subnet Mask",
    lblDhcpPool: "Random DHCP Allocation On Each Connect",
    lblBleTitle: "Secure Bluetooth LE (PIN)",
    lblBleSub: "Name: ESP32-Gateway-BLE • PIN: ",
    lblHeap: "Free RAM",
    lblUptime: "Uptime",
    lblFwVer: "Firmware",
    lblNat: "NAT Routing",
    lblStaToggleTitle: "Source Wi-Fi Mode (Internet Repeater)",
    lblStaToggleDesc: "Disable to make ESP32 an Offline Local LAN (Phone uses 5G)",
    lblStaOfflineTitle: "Local LAN Mode Active (5G Cellular)",
    lblStaOfflineDesc: "Source Wi-Fi is disabled. Phones connected to this Wi-Fi will keep using 5G mobile data for Internet access.",
    cardTitleScan: "Scan 2.4GHz Wi-Fi Networks",
    scanBtn: "🔍 Scan Wi-Fi",
    scanPlaceholder: "Click \"Scan Wi-Fi\" to find nearby networks...",
    lblInputStaSsid: "Source Wi-Fi Name (SSID)",
    phStaSsid: "Enter or select Wi-Fi above",
    lblInputStaPass: "Source Wi-Fi Password",
    phStaPass: "Enter password (leave empty if open)",
    noticeSta: "💾 Configuration is permanently saved to ESP32 Flash memory (Preferences NVS).",
    btnSaveSta: "🚀 Save Configuration & Connect",
    presetTitle: "SoftAP Configuration Presets",
    lblSelectPreset: "Quick select saved preset:",
    lblPresetName: "Preset Profile Name:",
    btnLoadPreset: "📋 Load to Form",
    btnApplyPreset: "🚀 Apply & Reboot",
    btnUpdatePreset: "💾 Save Update",
    btnDeletePreset: "🗑️ Delete Preset",
    btnSaveNewPreset: "➕ Save current form as new preset",
    btnCloneStaPreset: "📥 Clone from Source Wi-Fi to New Preset",
    cardTitleApConfig: "SoftAP & Gateway Configuration",
    boostTitle: "⚡ Max TX Power Boost",
    boostDesc: "Maximum hardware RF power <b>20 dBm (~100mW)</b>",
    lblInputApSsid: "SoftAP Broadcast Name (SSID)",
    lblInputApPass: "SoftAP Password (leave empty if Open)",
    lblApChannel: "Wi-Fi Channel",
    optCh0: "⚡ Channel 0 - Auto Scan Optimal Channel (Auto)",
    lblInputApBssid: "BSSID Address (MAC)",
    lblInputApIp: "Gateway IP (ESP32)",
    lblInputApSubnet: "Subnet Mask",
    lblInputDhcp: "DHCP IP Pool (Randomized Allocation - Unlimited)",
    lblDhcpStart: "Start:",
    lblDhcpEnd: "End:",
    noticeDhcp: "🎲 Randomly allocates an IP within this range on each client connection!",
    btnSaveAp: "💾 Save Parameters & Restart AP",
    cardTitleCloud: "Remote Internet Management (Cloud Remote)",
    cloudToggleTitle: "Enable Remote Access",
    cloudToggleDesc: "Connects outbound to Cloudflare Worker for worldwide remote control",
    lblCloudUrl: "Cloudflare Worker URL",
    lblCloudKey: "Secret Access Key / Password",
    btnSaveCloud: "💾 Save Cloud Config",
    cardTitleOta: "Over-the-Air Firmware Upgrade (GitHub OTA)",
    lblOtaUrl: "GitHub Version URL (version.json)",
    btnSaveOtaUrl: "Save URL",
    checkOtaText: "Check for Updates",
    otaFoundTitle: "New Version Found:",
    otaReadyTag: "Ready to flash",
    btnRunOtaText: "Download & Flash Firmware Now",
    otaExplanation: "💡 <b>How it works:</b> ESP32 connects directly to GitHub via secure HTTPS, downloads <code>esp32-firmware.bin</code> and flashes into the secondary OTA partition. Once complete, the device safely reboots into the new version.",
    cardTitleBle: "🛡️ Bluetooth LE Security & Control",
    bleRadioTitle: "Bluetooth LE Radio (BLE Radio)",
    bleRadioDesc: "Disable Bluetooth completely for stealth & power saving",
    lblBlePin: "Secure Pairing PIN (Exactly 6 Digits)",
    btnSavePin: "💾 Save PIN",
    hintBlePin: "Use this PIN when phone requests Bluetooth pairing.",
    bleExplanation: "🔒 <b>Hardware BLE Passkey Security:</b><br>• When your phone connects, the OS prompts for the 6-digit PIN to establish encrypted pairing.<br>• Once paired, logs stream wirelessly and commands can be executed immediately <b>without secondary auth</b>.<br>• <b>Fast PIN update via BLE:</b> Send <code style=\"color: #34d399;\">SETPIN:&lt;new_pin&gt;</code> on terminal.",
    cardTitleTerm: "📟 Wireless Console Log",
    btnClearLogs: "Clear log",
    footerDesc: "ESP32 NAPT Repeater Gateway • Built with Custom Random DHCP, BLE Security & GitHub OTA",
    footerFwLabel: "Firmware:"
  }
};

function setLanguage(lang) {
  currentLang = lang;
  setStoredLang(lang);
  document.documentElement.lang = lang;

  var btnVi = document.getElementById('btnLangVi');
  var btnEn = document.getElementById('btnLangEn');
  if (btnVi) btnVi.classList.toggle('active', lang === 'vi');
  if (btnEn) btnEn.classList.toggle('active', lang === 'en');
  
  var t = I18N[lang];
  for (var k in t) {
    var el = document.getElementById(k);
    if (el) {
      if (t[k].indexOf('<') !== -1) {
        el.innerHTML = t[k];
      } else {
        el.innerText = t[k];
      }
    }
  }

  var staSsid = document.getElementById('staSsidInput');
  if (staSsid && t.phStaSsid) staSsid.placeholder = t.phStaSsid;
  var staPass = document.getElementById('staPassInput');
  if (staPass && t.phStaPass) staPass.placeholder = t.phStaPass;

  // Hiển thị toast thông báo phản hồi chuyển ngôn ngữ ngay lập tức
  showToast(lang === 'en' ? 'Language switched to English 🇬🇧' : 'Đã chuyển sang Tiếng Việt 🇻🇳');

  // Cập nhật lại nhãn động theo ngôn ngữ mới
  if (lastStatusData) {
    renderStatus(lastStatusData);
  } else {
    // Khởi tạo trạng thái mặc định tức thì nếu chưa có status từ ESP32
    var badge = document.getElementById('connBadge');
    if (badge) badge.innerText = (lang === 'en' ? 'CHECKING...' : 'ĐANG KIỂM TRA');
    var pBadge = document.getElementById('valPowerBadge');
    if (pBadge && !pBadge.classList.contains('badge-warning')) {
      pBadge.innerText = (lang === 'en' ? '15 dBm STANDARD' : '15 dBm TIÊU CHUẨN');
    }
    var bBadge = document.getElementById('valBleBadge');
    if (bBadge) bBadge.innerText = (lang === 'en' ? 'ENABLED' : 'ĐÃ BẬT');
    var pCount = document.getElementById('presetCountBadge');
    if (pCount) pCount.innerText = savedProfiles.length + (lang === 'en' ? ' PRESETS' : ' MẪU');
  }
}

function showTab(n) {
  currentTab = n;
  for (var i = 1; i <= 4; i++) {
    document.getElementById('tab' + i).classList.add('hidden');
    document.getElementById('btnTab' + i).classList.remove('active');
  }
  document.getElementById('tab' + n).classList.remove('hidden');
  document.getElementById('btnTab' + n).classList.add('active');

  if (n === 2) {
    var list = document.getElementById('wifiScanResults');
    if (!list.innerHTML || list.innerHTML.indexOf('wifi-item') === -1) {
      scanWifi(false);
    }
  }

  if (n === 3) {
    fetchProfiles();
  }

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

function renderStatus(d) {
  var badge = document.getElementById('connBadge');
  var captive = document.getElementById('captiveAlert');
  document.getElementById('headIp').innerText = d.ap_ip || '10.51.31.254';

  var staSw = document.getElementById('staEnableSwitch');
  if (staSw) {
    staSw.checked = (d.sta_enabled !== false);
  }
  var staAlert = document.getElementById('staDisabledAlert');
  if (staAlert) {
    if (d.sta_enabled === false) {
      staAlert.classList.remove('hidden');
    } else {
      staAlert.classList.add('hidden');
    }
  }

  if (d.sta_enabled === false) {
    badge.className = 'badge badge-blue';
    badge.innerText = currentLang === 'en' ? 'LOCAL LAN (5G)' : 'MẠNG NỘI BỘ (5G)';
    captive.classList.add('hidden');
    document.getElementById('staStatusText').innerText = currentLang === 'en' ? 'Local Only (5G active)' : 'Mạng nội bộ (Điện thoại dùng 5G)';
    document.getElementById('staStatusText').style.color = '#60a5fa';
  } else if (d.connected) {
    badge.className = 'badge badge-success';
    badge.innerText = currentLang === 'en' ? 'CONNECTED' : 'ĐÃ KẾT NỐI';
    captive.classList.add('hidden');
    document.getElementById('staStatusText').innerText = currentLang === 'en' ? 'Active' : 'Đang hoạt động';
    document.getElementById('staStatusText').style.color = '#34d399';
    var btnClone1 = document.getElementById('btnCloneStaTab1');
    if (btnClone1) btnClone1.disabled = false;
  } else {
    badge.className = 'badge badge-danger';
    badge.innerText = currentLang === 'en' ? 'DISCONNECTED' : 'MẤT KẾT NỐI';
    captive.classList.remove('hidden');
    document.getElementById('staStatusText').innerText = currentLang === 'en' ? 'Disconnected' : 'Chưa kết nối';
    document.getElementById('staStatusText').style.color = '#f87171';
  }

  document.getElementById('valStaSsid').innerText = d.sta_ssid || (currentLang === 'en' ? '(Not configured)' : '(Chưa cấu hình)');
  document.getElementById('valStaPass').innerText = d.sta_pass ? d.sta_pass : (currentLang === 'en' ? '(No password)' : '(Không có mật khẩu)');
  document.getElementById('valStaRssi').innerText = d.connected ? (d.sta_rssi + ' dBm') : '---';
  document.getElementById('valStaIp').innerText = d.connected ? d.sta_ip : (currentLang === 'en' ? 'No IP' : 'Chưa có IP');
  document.getElementById('valStaGw').innerText = d.connected ? (d.sta_gw || '---') : '---';
  document.getElementById('valStaSubnet').innerText = d.connected ? (d.sta_subnet || '255.255.255.0') : '---';
  document.getElementById('valStaBssid').innerText = d.connected ? (d.sta_bssid || '---') : '---';

  document.getElementById('valApSsid').innerText = d.ap_ssid || 'ODE - wireless WiFi';
  document.getElementById('valApPass').innerText = d.ap_pass ? d.ap_pass : (currentLang === 'en' ? '(Open network / No pass)' : '(Mạng mở / Không pass)');
  document.getElementById('valApBssid').innerText = d.ap_bssid || '84:F1:47:4F:27:A0';
  if (d.ap_channel === 0) {
    document.getElementById('valApChannel').innerText = (currentLang === 'en' ? 'Auto (Channel ' : 'Tự động (Kênh ') + (d.effective_channel || 'Auto') + ')';
    document.getElementById('valApChannelBadge').innerText = 'AUTO (CH ' + (d.effective_channel || '?') + ')';
  } else {
    document.getElementById('valApChannel').innerText = (currentLang === 'en' ? 'Channel ' : 'Kênh ') + (d.ap_channel || 6);
    document.getElementById('valApChannelBadge').innerText = (currentLang === 'en' ? 'CH ' : 'KÊNH ') + (d.ap_channel || 6);
  }
  document.getElementById('valApIp').innerText = d.ap_ip || '10.51.31.254';
  document.getElementById('valApSubnet').innerText = d.ap_subnet || '255.255.252.0';
  
  var dStart = d.dhcp_start || '10.51.28.10';
  var dEnd = d.dhcp_end || '10.51.28.200';
  document.getElementById('valDhcpPool').innerText = dStart + '  ➔  ' + dEnd + (currentLang === 'en' ? ' (Random)' : ' (Ngẫu nhiên)');

  // Cập nhật trạng thái Max TX Power Boost
  var pBadge = document.getElementById('valPowerBadge');
  var pToggle = document.getElementById('powerBoostToggle');
  var pNotice = document.getElementById('powerBoostNotice');
  if (d.tx_boost) {
    pBadge.className = 'badge badge-warning';
    pBadge.innerText = '20 dBm BOOST';
    pToggle.checked = true;
    pNotice.innerHTML = currentLang === 'en' 
      ? '🚀 <b>Active 20 dBm:</b> Enhanced wall penetration, 30-50% extended range.'
      : '🚀 <b>Đang kích hoạt 20 dBm:</b> Tăng cường độ xuyên tường, phát sóng xa hơn 30-50%.';
    pNotice.style.color = '#fde68a';
  } else {
    pBadge.className = 'badge badge-blue';
    pBadge.innerText = currentLang === 'en' ? '15 dBm STANDARD' : '15 dBm TIÊU CHUẨN';
    pToggle.checked = false;
    pNotice.innerHTML = currentLang === 'en'
      ? '🍃 <b>Standard 15 dBm:</b> Energy efficient, lower operating temperature.'
      : '🍃 <b>Mức tiêu chuẩn 15 dBm:</b> Tiết kiệm điện năng, giúp chip ESP32 mát hơn.';
    pNotice.style.color = '#60a5fa';
  }

  // Cập nhật trạng thái Bluetooth & PIN
  var bBadge = document.getElementById('valBleBadge');
  var bBadgeTab4 = document.getElementById('bleStatusBadgeTab4');
  var bToggle = document.getElementById('bleRadioToggle');
  document.getElementById('dashBlePin').innerText = d.ble_pin || '686868';

  if (d.ble_enabled !== false) {
    bBadge.className = 'badge badge-success';
    bBadge.innerText = currentLang === 'en' ? 'ENABLED' : 'ĐÃ BẬT';
    bBadgeTab4.className = 'badge badge-success';
    bBadgeTab4.innerText = currentLang === 'en' ? 'ENABLED' : 'ĐANG BẬT';
    bToggle.checked = true;
  } else {
    bBadge.className = 'badge badge-danger';
    bBadge.innerText = currentLang === 'en' ? 'DISABLED' : 'ĐÃ TẮT';
    bBadgeTab4.className = 'badge badge-danger';
    bBadgeTab4.innerText = currentLang === 'en' ? 'DISABLED' : 'ĐÃ TẮT';
    bToggle.checked = false;
  }

  if (!document.getElementById('inputBlePin').value && d.ble_pin) {
    document.getElementById('inputBlePin').value = d.ble_pin;
  }

  document.getElementById('valHeap').innerText = d.free_heap ? (Math.round(d.free_heap / 1024) + ' KB') : '---';
  var uptimeSec = d.uptime || 0;
  var mins = Math.floor(uptimeSec / 60);
  var hrs = Math.floor(mins / 60);
  mins = mins % 60;
  document.getElementById('valUptime').innerText = (hrs < 10 ? '0' : '') + hrs + 'h ' + (mins < 10 ? '0' : '') + mins + 'm';
  document.getElementById('valNat').innerText = d.napt ? (currentLang === 'en' ? 'ON' : 'BẬT') : (currentLang === 'en' ? 'OFF' : 'TẮT');

  if (d.fw_ver) {
    document.getElementById('valFwVer').innerText = 'v' + d.fw_ver;
    document.getElementById('badgeFwVerOta').innerText = 'v' + d.fw_ver;
    document.getElementById('footerFwVer').innerText = 'v' + d.fw_ver;
  }
  if (d.ota_url && !document.getElementById('inputOtaUrl').value) {
    document.getElementById('inputOtaUrl').value = d.ota_url;
  }

  // Cập nhật trạng thái Cloud Remote
  var cToggle = document.getElementById('cloudRemoteToggle');
  if (cToggle) cToggle.checked = (d.cloud_enabled === true);

  var cBadge = document.getElementById('badgeCloudStatus');
  if (cBadge) {
    if (d.cloud_online) {
      cBadge.className = 'badge badge-success';
      cBadge.innerText = 'ONLINE 🟢';
    } else if (d.cloud_enabled) {
      cBadge.className = 'badge badge-warning';
      cBadge.innerText = currentLang === 'en' ? 'CONNECTING...' : 'ĐANG KẾT NỐI...';
    } else {
      cBadge.className = 'badge badge-danger';
      cBadge.innerText = currentLang === 'en' ? 'DISABLED' : 'ĐÃ TẮT';
    }
  }

  if (d.cloud_url && !document.getElementById('inputCloudUrl').value) {
    document.getElementById('inputCloudUrl').value = d.cloud_url;
  }
  if (d.cloud_key && !document.getElementById('inputCloudKey').value) {
    document.getElementById('inputCloudKey').value = d.cloud_key;
  }

  var cLink = document.getElementById('btnOpenCloudLink');
  if (cLink) {
    if (d.cloud_url && d.cloud_url.length > 8) {
      cLink.href = d.cloud_url;
      cLink.classList.remove('hidden');
    } else {
      cLink.classList.add('hidden');
    }
  }

  if (!document.getElementById('staSsidInput').value) document.getElementById('staSsidInput').value = d.sta_ssid || '';
  if (!document.getElementById('staPassInput').value && d.sta_pass) document.getElementById('staPassInput').value = d.sta_pass;
  if (!document.getElementById('apSsidInput').value) document.getElementById('apSsidInput').value = d.ap_ssid || '';
  if (!document.getElementById('apPassInput').value && d.ap_pass) document.getElementById('apPassInput').value = d.ap_pass;
  if (!document.getElementById('apBssidInput').value) document.getElementById('apBssidInput').value = d.ap_bssid || '';
  if (!document.getElementById('apIpInput').value) document.getElementById('apIpInput').value = d.ap_ip || '';
  if (!document.getElementById('apSubnetInput').value) document.getElementById('apSubnetInput').value = d.ap_subnet || '';
  if (!document.getElementById('dhcpStartInput').value) document.getElementById('dhcpStartInput').value = d.dhcp_start || '10.51.28.10';
  if (!document.getElementById('dhcpEndInput').value) document.getElementById('dhcpEndInput').value = d.dhcp_end || '10.51.28.200';
  if (d.ap_channel !== undefined) document.getElementById('apChannelSelect').value = d.ap_channel;
}

function updateStatus() {
  fetch('/api/status')
    .then(function(res) { return res.json(); })
    .then(function(d) {
      lastStatusData = d;
      renderStatus(d);
    })
    .catch(function(err) {
      console.log('Status fetch skipped / local mode:', err);
    });
}

function saveCloudConfig() {
  var isEn = document.getElementById('cloudRemoteToggle').checked;
  var url = document.getElementById('inputCloudUrl').value.trim();
  var key = document.getElementById('inputCloudKey').value.trim();

  var data = new URLSearchParams();
  data.append('enabled', isEn ? '1' : '0');
  data.append('url', url);
  data.append('key', key);

  fetch('/api/save_cloud', { method: 'POST', body: data })
    .then(function(res) { return res.json(); })
    .then(function(d) {
      if (d.msg) showToast(d.msg);
      updateStatus();
    })
    .catch(function(err) {
      alert(currentLang === 'en' ? 'Error saving Cloud configuration!' : 'Lỗi lưu cấu hình Cloud Remote!');
    });
}

function toggleCloudRemote(isEn) {
  saveCloudConfig();
}

function toggleStaMode(isEn) {
  var data = new URLSearchParams();
  data.append('enabled', isEn ? '1' : '0');

  fetch('/api/set_sta_mode', { method: 'POST', body: data })
    .then(function(res) { return res.json(); })
    .then(function(d) {
      if (d.msg) {
        showToast(d.msg);
      }
      updateStatus();
    })
    .catch(function(err) {
      alert(currentLang === 'en' ? 'Error updating Wi-Fi source mode!' : 'Lỗi cập nhật chế độ Wi-Fi nguồn!');
      var sw = document.getElementById('staEnableSwitch');
      if (sw) sw.checked = !isEn;
    });
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
      alert(currentLang === 'en' ? 'Error updating RF TX power!' : 'Lỗi cập nhật công suất phát!');
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
      alert(currentLang === 'en' ? 'Error toggling Bluetooth radio!' : 'Lỗi cập nhật sóng Bluetooth!');
      cb.checked = !isEn;
    });
}

function saveBlePin() {
  var pin = document.getElementById('inputBlePin').value.trim();
  if (!/^\d{6}$/.test(pin)) {
    alert(currentLang === 'en' ? 'Bluetooth PIN must be exactly 6 digits (0-9)!' : 'Mã PIN Bluetooth phải gồm đúng 6 chữ số (0-9)!');
    return;
  }
  var data = new URLSearchParams();
  data.append('pin', pin);

  fetch('/api/save_ble', { method: 'POST', body: data })
    .then(function(res) { return res.json(); })
    .then(function(d) {
      alert((currentLang === 'en' ? 'Bluetooth PIN updated successfully: ' : 'Đã cập nhật mã PIN Bluetooth thành công: ') + pin);
      updateStatus();
    })
    .catch(function(err) { alert(currentLang === 'en' ? 'Error saving PIN!' : 'Lỗi khi lưu mã PIN!'); });
}

function fetchLogs() {
  fetch('/api/logs')
    .then(function(res) { return res.json(); })
    .then(function(lines) {
      var term = document.getElementById('termLogs');
      if (lines.length === 0) {
        term.innerHTML = '<div style="color:#9ca3af;">' + (currentLang === 'en' ? '(No activity logs yet)' : '(Chưa có nhật ký hoạt động nào)') + '</div>';
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
      document.getElementById('termLogs').innerHTML = '<div style="color:#9ca3af;">' + (currentLang === 'en' ? '(Logs cleared)' : '(Đã xóa toàn bộ nhật ký)') + '</div>';
    });
}

function scanWifi(force) {
  var btn = document.getElementById('scanBtn');
  var list = document.getElementById('wifiScanResults');
  btn.innerText = currentLang === 'en' ? 'Scanning...' : 'Đang tải...';
  btn.disabled = true;

  var url = force ? '/api/scan?force=1' : '/api/scan';
  fetch(url)
    .then(function(res) { return res.json(); })
    .then(function(networks) {
      btn.innerText = currentLang === 'en' ? '🔍 Rescan' : '🔍 Quét lại';
      btn.disabled = false;
      if (!networks || networks.length === 0) {
        list.innerHTML = '<div style="font-size:11px;color:#9ca3af;text-align:center;padding:12px;">' + (currentLang === 'en' ? 'No Wi-Fi networks found. Click "Rescan".' : 'Chưa tìm thấy mạng Wi-Fi nào. Hãy bấm "Quét lại".') + '</div>';
        return;
      }
      var html = '';
      for (var i = 0; i < networks.length; i++) {
        var w = networks[i];
        if (!w.ssid) continue;
        html += '<div class="wifi-item" onclick="selectWifi(\'' + encodeURIComponent(w.ssid) + '\')">';
        html += '  <div>';
        html += '    <div class="wifi-item-title">' + w.ssid + '</div>';
        html += '    <div class="wifi-item-sub">' + (currentLang === 'en' ? 'Channel: ' : 'Kênh: ') + w.ch + ' ' + (w.secure ? '🔒' : '🔓') + '</div>';
        html += '  </div>';
        html += '  <div class="wifi-item-rssi" style="color:' + (w.rssi > -70 ? '#34d399' : '#f59e0b') + '">' + w.rssi + ' dBm</div>';
        html += '</div>';
      }
      list.innerHTML = html;
    })
    .catch(function(err) {
      btn.innerText = currentLang === 'en' ? '🔍 Rescan' : '🔍 Quét lại';
      btn.disabled = false;
      list.innerHTML = '<div style="font-size:11px;color:#f87171;text-align:center;padding:12px;">' + (currentLang === 'en' ? 'Error scanning Wi-Fi networks!' : 'Lỗi khi tải danh sách Wi-Fi!') + '</div>';
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
    alert(currentLang === 'en' ? 'Please enter upstream Wi-Fi SSID!' : 'Vui lòng nhập tên Wi-Fi nguồn!');
    return;
  }
  var data = new URLSearchParams();
  data.append('ssid', ssid);
  data.append('pass', pass);

  fetch('/api/save_sta', { method: 'POST', body: data })
    .then(function(res) { return res.json(); })
    .then(function(d) {
      alert(d.msg || (currentLang === 'en' ? 'Source Wi-Fi saved! ESP32 reconnecting...' : 'Đã lưu Wi-Fi nguồn! ESP32 đang kết nối lại...'));
      showTab(1);
      setTimeout(updateStatus, 3000);
    })
    .catch(function(err) { alert(currentLang === 'en' ? 'Request error!' : 'Lỗi khi gửi yêu cầu!'); });
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
    alert(currentLang === 'en' ? 'Please fill in all required fields!' : 'Vui lòng điền đầy đủ các trường bắt buộc!');
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
      alert(d.msg || (currentLang === 'en' ? 'SoftAP parameters saved! ESP32 restarting...' : 'Đã lưu cấu hình mạng phát! ESP32 đang khởi động lại...'));
    })
    .catch(function(err) { alert(currentLang === 'en' ? 'Error saving SoftAP parameters!' : 'Lỗi khi lưu tham số AP!'); });
}

var cachedBinUrl = '';

function saveOtaUrl() {
  var url = document.getElementById('inputOtaUrl').value.trim();
  if (!url) {
    alert(currentLang === 'en' ? 'Please enter version.json URL!' : 'Vui lòng nhập đường dẫn version.json!');
    return;
  }
  var data = new URLSearchParams();
  data.append('url', url);

  fetch('/api/save_ota_url', { method: 'POST', body: data })
    .then(function(res) { return res.json(); })
    .then(function(d) {
      if (d.success) {
        alert(currentLang === 'en' ? 'GitHub OTA URL saved successfully!' : 'Đã lưu URL GitHub OTA thành công!');
      } else {
        alert((currentLang === 'en' ? 'Error: ' : 'Lỗi: ') + (d.error || 'Failed to save URL'));
      }
    })
    .catch(function(err) { alert(currentLang === 'en' ? 'Connection error saving URL!' : 'Lỗi kết nối khi lưu URL!'); });
}

function checkGithubOta() {
  var btn = document.getElementById('btnCheckOta');
  var txt = document.getElementById('checkOtaText');
  var icon = document.getElementById('checkOtaIcon');
  var resultBox = document.getElementById('otaResultBox');

  btn.disabled = true;
  txt.innerText = currentLang === 'en' ? 'Connecting to GitHub...' : 'Đang kết nối GitHub...';
  icon.innerText = '⏳';

  fetch('/api/check_ota', { method: 'POST' })
    .then(function(res) { return res.json(); })
    .then(function(d) {
      btn.disabled = false;
      txt.innerText = currentLang === 'en' ? 'Check for Updates' : 'Kiểm Tra Bản Cập Nhật Mới';
      icon.innerText = '🔍';

      if (!d.success) {
        alert((currentLang === 'en' ? 'OTA check failed: ' : 'Không thể kiểm tra OTA: ') + (d.error || 'Unknown error'));
        return;
      }

      if (!d.has_update) {
        alert(currentLang === 'en' 
          ? ('Device is running latest version (' + d.current_ver + ')!\nNo update available on GitHub.')
          : ('Thiết bị đang chạy phiên bản mới nhất (' + d.current_ver + ')!\nKhông có bản cập nhật mới trên GitHub.'));
        resultBox.classList.add('hidden');
        return;
      }

      cachedBinUrl = d.bin_url;
      document.getElementById('otaNewVerText').innerText = 'v' + d.remote_ver + (currentLang === 'en' ? ' (Current: v' : ' (Hiện tại: v') + d.current_ver + ')';
      document.getElementById('otaChangelogText').innerText = d.changelog || (currentLang === 'en' ? 'System improvements and bugfixes from GitHub.' : 'Bản nâng cấp tối ưu hóa hệ thống và sửa lỗi từ GitHub.');
      resultBox.classList.remove('hidden');
    })
    .catch(function(err) {
      btn.disabled = false;
      txt.innerText = currentLang === 'en' ? 'Check for Updates' : 'Kiểm Tra Bản Cập Nhật Mới';
      icon.innerText = '🔍';
      alert(currentLang === 'en' ? 'Connection error checking OTA!' : 'Lỗi kết nối khi gửi yêu cầu kiểm tra OTA!');
    });
}

function startGithubOta() {
  if (!cachedBinUrl) {
    alert(currentLang === 'en' ? 'No firmware binary information available!' : 'Chưa có thông tin file esp32-firmware.bin!');
    return;
  }
  var confirmMsg = currentLang === 'en'
    ? 'CONFIRM FIRMWARE UPGRADE:\n\nESP32 will download firmware from GitHub and flash it into memory.\nProcess takes 30 - 60 seconds.\nDo NOT power off the device!\n\nProceed now?'
    : 'XÁC NHẬN NÂNG CẤP FIRMWARE:\n\nESP32 sẽ tải file từ GitHub và ghi đè vào phân vùng Flash.\nQuá trình này mất khoảng 30 - 60 giây.\nKhông được tắt nguồn thiết bị!\n\nBạn có muốn bắt đầu không?';
    
  if (!confirm(confirmMsg)) {
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
        alert((currentLang === 'en' ? 'OTA start failed: ' : 'Lỗi khởi động OTA: ') + (d.error || 'Unable to start'));
        btnRun.classList.remove('hidden');
        progBox.classList.add('hidden');
        return;
      }

      var progress = 0;
      var timer = setInterval(function() {
        progress += 2;
        if (progress > 95) progress = 95;
        pBar.style.width = progress + '%';
        pPct.innerText = progress + '%';
        pTxt.innerText = (currentLang === 'en' ? 'Downloading firmware & flashing (' : 'Đang tải firmware & ghi Flash (') + progress + '%)...';

        if (progress >= 95) {
          clearInterval(timer);
          pTxt.innerText = currentLang === 'en' ? 'ESP32 finishing flash and rebooting...' : 'ESP32 đang hoàn tất ghi và khởi động lại...';
          setTimeout(function() {
            pBar.style.width = '100%';
            pPct.innerText = '100%';
            pTxt.innerText = currentLang === 'en' ? 'Reloading dashboard...' : 'Đang tải lại giao diện...';
            setTimeout(function() {
              location.reload();
            }, 3000);
          }, 8000);
        }
      }, 500);
    })
    .catch(function(err) {
      alert(currentLang === 'en' 
        ? 'Lost connection to ESP32 (Device may be flashing and rebooting). Please wait 15 seconds then refresh page.'
        : 'Mất kết nối với ESP32 (Thiết bị có thể đang nạp Flash và khởi động lại). Vui lòng đợi 15 giây rồi tải lại trang.');
    });
}

// =========================================================================
//            QUẢN LÝ MẪU CẤU HÌNH MẠNG PHÁT (PRESETS) & AUTO CHANNEL
// =========================================================================
var savedProfiles = [];

function onProfileSelectChange(id) {
  if (!id) {
    var nameInp = document.getElementById('presetNameInput');
    if (nameInp) nameInp.value = '';
    return;
  }
  for (var i = 0; i < savedProfiles.length; i++) {
    if (savedProfiles[i].id === id) {
      var nameInp = document.getElementById('presetNameInput');
      if (nameInp) nameInp.value = savedProfiles[i].name || '';
      break;
    }
  }
}

function fetchProfiles(selectIdAfter) {
  fetch('/api/profiles')
    .then(function(res) { return res.json(); })
    .then(function(list) {
      savedProfiles = list || [];
      var sel = document.getElementById('profileSelect');
      var badge = document.getElementById('presetCountBadge');
      if (badge) badge.innerText = savedProfiles.length + (currentLang === 'en' ? ' PRESETS' : ' MẪU');

      var targetId = selectIdAfter || sel.value;
      sel.innerHTML = '<option value="">-- ' + (currentLang === 'en' ? 'Click to select preset (' : 'Bấm để chọn cấu hình mẫu (') + savedProfiles.length + ') --</option>';
      for (var i = 0; i < savedProfiles.length; i++) {
        var p = savedProfiles[i];
        var opt = document.createElement('option');
        opt.value = p.id;
        opt.innerText = (i + 1) + '. ' + p.name + ' [' + p.ssid + ' | ' + (p.channel == 0 ? 'Auto Ch' : ('Ch ' + p.channel)) + ']';
        sel.appendChild(opt);
      }
      if (targetId) {
        sel.value = targetId;
        onProfileSelectChange(targetId);
      }
    })
    .catch(function(err) { console.log(err); });
}

function getSelectedProfile() {
  var id = document.getElementById('profileSelect').value;
  if (!id) return null;
  for (var i = 0; i < savedProfiles.length; i++) {
    if (savedProfiles[i].id === id) return savedProfiles[i];
  }
  return null;
}

function loadProfileToForm(silent) {
  var p = getSelectedProfile();
  if (!p) {
    if (!silent) alert(currentLang === 'en' ? 'Please select a profile from the list first!' : 'Vui lòng chọn một mẫu cấu hình từ danh sách trước!');
    return;
  }
  var nameInp = document.getElementById('presetNameInput');
  if (nameInp) nameInp.value = p.name || '';
  document.getElementById('apSsidInput').value = p.ssid || '';
  document.getElementById('apPassInput').value = p.pass || '';
  document.getElementById('apChannelSelect').value = p.channel !== undefined ? p.channel : 6;
  document.getElementById('apBssidInput').value = p.bssid || '';
  document.getElementById('apIpInput').value = p.ip || '';
  document.getElementById('apSubnetInput').value = p.subnet || '';
  document.getElementById('dhcpStartInput').value = p.dhcp_start || '10.51.28.10';
  document.getElementById('dhcpEndInput').value = p.dhcp_end || '10.51.28.200';
  if (!silent) {
    alert((currentLang === 'en' ? 'Loaded parameters from preset "' : 'Đã nạp toàn bộ thông số của mẫu "') + p.name + '"!\n' + 
          (currentLang === 'en' ? 'You can edit below and click "Save Update" to update this preset.' : 'Bạn có thể chỉnh sửa bên dưới rồi bấm "Lưu Cập Nhật" để lưu lại vào mẫu này.'));
  }
}

function applySelectedProfile() {
  var p = getSelectedProfile();
  if (!p) {
    alert(currentLang === 'en' ? 'Please select a profile from the list!' : 'Vui lòng chọn một mẫu cấu hình từ danh sách!');
    return;
  }
  var confirmMsg = currentLang === 'en'
    ? 'Are you sure you want to activate preset "' + p.name + '"?\nESP32 will apply parameters and reboot SoftAP now.'
    : 'Bạn có chắc muốn kích hoạt mẫu "' + p.name + '"?\nESP32 sẽ áp dụng ngay và khởi động lại cổng phát AP.';
  if (!confirm(confirmMsg)) {
    return;
  }
  var data = new URLSearchParams();
  data.append('id', p.id);
  fetch('/api/apply_profile', { method: 'POST', body: data })
    .then(function(res) { return res.json(); })
    .then(function(d) {
      alert(d.msg || (currentLang === 'en' ? 'Preset applied successfully! ESP32 restarting...' : 'Đã áp dụng mẫu thành công! ESP32 đang khởi động lại...'));
      showTab(1);
    })
    .catch(function(err) { alert(currentLang === 'en' ? 'Error applying profile!' : 'Lỗi khi kích hoạt mẫu cấu hình!'); });
}

function updateSelectedProfile() {
  var p = getSelectedProfile();
  if (!p) {
    alert(currentLang === 'en' ? 'Please select a profile to update!' : 'Vui lòng chọn một mẫu cấu hình từ danh sách cần cập nhật!');
    return;
  }
  var nameInp = document.getElementById('presetNameInput');
  var updatedName = (nameInp && nameInp.value.trim().length > 0) ? nameInp.value.trim() : p.name;
  var ssid = document.getElementById('apSsidInput').value.trim();
  if (!ssid) {
    alert(currentLang === 'en' ? 'SSID cannot be empty!' : 'Tên Wi-Fi (SSID) không được để trống!');
    return;
  }

  var data = new URLSearchParams();
  data.append('id', p.id);
  data.append('name', updatedName);
  data.append('ssid', ssid);
  data.append('pass', document.getElementById('apPassInput').value);
  data.append('channel', document.getElementById('apChannelSelect').value);
  data.append('bssid', document.getElementById('apBssidInput').value);
  data.append('ip', document.getElementById('apIpInput').value);
  data.append('subnet', document.getElementById('apSubnetInput').value);
  data.append('dhcp_start', document.getElementById('dhcpStartInput').value || '10.51.28.10');
  data.append('dhcp_end', document.getElementById('dhcpEndInput').value || '10.51.28.200');

  fetch('/api/save_profile', { method: 'POST', body: data })
    .then(function(res) { return res.json(); })
    .then(function(d) {
      if (d.success) {
        alert((currentLang === 'en' ? 'Profile updated successfully: "' : 'Đã cập nhật mẫu cấu hình thành công: "') + updatedName + '"!');
        fetchProfiles(p.id);
      } else {
        alert((currentLang === 'en' ? 'Error: ' : 'Lỗi: ') + (d.error || 'Update failed'));
      }
    })
    .catch(function(err) { alert(currentLang === 'en' ? 'Error updating profile!' : 'Lỗi khi cập nhật mẫu cấu hình!'); });
}

function deleteSelectedProfile() {
  var p = getSelectedProfile();
  if (!p) {
    alert(currentLang === 'en' ? 'Please select a profile to delete!' : 'Vui lòng chọn mẫu cấu hình cần xóa!');
    return;
  }
  var confirmMsg = currentLang === 'en'
    ? 'Are you sure you want to delete profile "' + p.name + '" from presets list?'
    : 'Bạn có chắc chắn muốn xóa mẫu "' + p.name + '" khỏi danh sách?';
  if (!confirm(confirmMsg)) {
    return;
  }
  var data = new URLSearchParams();
  data.append('id', p.id);
  fetch('/api/delete_profile', { method: 'POST', body: data })
    .then(function(res) { return res.json(); })
    .then(function(d) {
      alert(currentLang === 'en' ? 'Profile deleted successfully!' : 'Đã xóa mẫu cấu hình thành công!');
      fetchProfiles();
    })
    .catch(function(err) { alert(currentLang === 'en' ? 'Error deleting profile!' : 'Lỗi khi xóa mẫu!'); });
}

function saveCurrentAsNewProfile() {
  var ssid = document.getElementById('apSsidInput').value.trim();
  if (!ssid) {
    alert(currentLang === 'en' ? 'Please enter SoftAP SSID in form first!' : 'Vui lòng nhập Tên Wi-Fi (SSID) trong form trước!');
    return;
  }
  var nameInp = document.getElementById('presetNameInput');
  var name = (nameInp && nameInp.value.trim().length > 0) ? nameInp.value.trim() : (ssid + ' (Preset ' + (savedProfiles.length + 1) + ')');

  var data = new URLSearchParams();
  data.append('name', name);
  data.append('ssid', ssid);
  data.append('pass', document.getElementById('apPassInput').value);
  data.append('channel', document.getElementById('apChannelSelect').value);
  data.append('bssid', document.getElementById('apBssidInput').value);
  data.append('ip', document.getElementById('apIpInput').value);
  data.append('subnet', document.getElementById('apSubnetInput').value);
  data.append('dhcp_start', document.getElementById('dhcpStartInput').value || '10.51.28.10');
  data.append('dhcp_end', document.getElementById('dhcpEndInput').value || '10.51.28.200');

  fetch('/api/save_profile', { method: 'POST', body: data })
    .then(function(res) { return res.json(); })
    .then(function(d) {
      if (d.success) {
        alert((currentLang === 'en' ? 'New profile saved: "' : 'Đã lưu mẫu cấu hình mới: "') + name + '"!');
        fetchProfiles(d.id);
      } else {
        alert((currentLang === 'en' ? 'Error: ' : 'Lỗi: ') + (d.error || 'Save failed'));
      }
    })
    .catch(function(err) { alert(currentLang === 'en' ? 'Error saving new profile!' : 'Lỗi khi lưu mẫu mới!'); });
}

// Tính năng mới: Sao chép cấu hình Wi-Fi nguồn thành Preset AP
function cloneStaToPreset() {
  var confirmMsg = currentLang === 'en'
    ? 'Do you want to clone the current Source Wi-Fi into a new SoftAP Preset?\n\n' +
      '• SSID & Password from Source Wi-Fi\n' +
      '• Router BSSID (Hardware MAC)\n' +
      '• Gateway IP & Subnet Mask\n' +
      '• DHCP IP Pool configured matching Source IP scheme'
    : 'Bạn có muốn sao chép cấu hình Wi-Fi nguồn hiện tại thành 1 Preset Mạng Phát mới không?\n\n' +
      '• Tên Wi-Fi (SSID) & Mật khẩu từ Wi-Fi nguồn\n' +
      '• Địa chỉ MAC phần cứng (BSSID của Router nguồn)\n' +
      '• IP Gateway & Subnet Mask tương ứng\n' +
      '• Dải cấp IP DHCP tự động tính theo quy tắc IP nguồn (.10 ➔ .200)';
  if (!confirm(confirmMsg)) return;

  fetch('/api/clone_sta_to_profile', { method: 'POST' })
    .then(function(res) { return res.json(); })
    .then(function(d) {
      if (!d.success) {
        alert((currentLang === 'en' ? 'Clone failed: ' : 'Lỗi sao chép: ') + (d.error || 'Unknown error'));
        return;
      }
      var successMsg = currentLang === 'en'
        ? ('🎉 Successfully cloned Source Wi-Fi to SoftAP Preset!\n\n' +
           '• Preset Name: ' + d.name + '\n' +
           '• SSID: ' + d.ssid + '\n' +
           '• BSSID (MAC): ' + d.bssid + '\n' +
           '• Gateway IP: ' + d.ip + '\n' +
           '• Subnet Mask: ' + d.subnet + '\n' +
           '• DHCP Pool: ' + d.dhcp_start + ' ➔ ' + d.dhcp_end)
        : ('🎉 Đã sao chép cấu hình Wi-Fi nguồn thành Preset thành công!\n\n' +
           '• Tên mẫu: ' + d.name + '\n' +
           '• Tên Wi-Fi: ' + d.ssid + '\n' +
           '• Địa chỉ BSSID (MAC): ' + d.bssid + '\n' +
           '• Gateway IP: ' + d.ip + '\n' +
           '• Subnet Mask: ' + d.subnet + '\n' +
           '• Dải DHCP ngẫu nhiên: ' + d.dhcp_start + ' ➔ ' + d.dhcp_end);
      alert(successMsg);

      showTab(3);
      fetchProfiles(d.id);
    })
    .catch(function(err) {
      // Fallback mô phỏng cho môi trường preview offline
      var sSsid = (lastStatusData && lastStatusData.sta_ssid) ? lastStatusData.sta_ssid : 'Source_WiFi';
      var sBssid = (lastStatusData && lastStatusData.sta_bssid) ? lastStatusData.sta_bssid : '84:F1:47:4F:27:A0';
      var sGw = (lastStatusData && lastStatusData.sta_gw) ? lastStatusData.sta_gw : '192.168.1.1';
      var sSub = (lastStatusData && lastStatusData.sta_subnet) ? lastStatusData.sta_subnet : '255.255.255.0';
      var sIp = (lastStatusData && lastStatusData.sta_ip) ? lastStatusData.sta_ip : '192.168.1.100';
      var parts = sIp.split('.');
      var prefix = (parts.length >= 3) ? (parts[0] + '.' + parts[1] + '.' + parts[2] + '.') : '10.51.28.';
      var dStart = prefix + '10';
      var dEnd = prefix + '200';

      document.getElementById('apSsidInput').value = sSsid;
      document.getElementById('apBssidInput').value = sBssid;
      document.getElementById('apIpInput').value = sGw;
      document.getElementById('apSubnetInput').value = sSub;
      document.getElementById('dhcpStartInput').value = dStart;
      document.getElementById('dhcpEndInput').value = dEnd;

      var fallbackMsg = currentLang === 'en'
        ? ('🎉 Cloned Source Wi-Fi parameters directly to SoftAP Form!\n\n' +
           '• SSID: ' + sSsid + '\n' +
           '• BSSID: ' + sBssid + '\n' +
           '• Gateway: ' + sGw + '\n' +
           '• Subnet: ' + sSub + '\n' +
           '• DHCP: ' + dStart + ' ➔ ' + dEnd)
        : ('🎉 Đã sao chép toàn bộ thông số Wi-Fi nguồn vào Form Mạng Phát!\n\n' +
           '• SSID: ' + sSsid + '\n' +
           '• BSSID: ' + sBssid + '\n' +
           '• Gateway: ' + sGw + '\n' +
           '• Subnet: ' + sSub + '\n' +
           '• Dải DHCP: ' + dStart + ' ➔ ' + dEnd);
      alert(fallbackMsg);
      showTab(3);
    });
}

function onChannelChange(val) {
  if (val === '0') {
    autoSelectChannel();
  }
}

function autoSelectChannel() {
  var opt0 = document.getElementById('optCh0');
  var originalText = opt0 ? opt0.innerText : '';
  if (opt0) {
    opt0.innerText = currentLang === 'en' ? '⏳ Scanning 2.4GHz channels...' : '⏳ Đang quét kiểm tra kênh 2.4GHz...';
  }

  fetch('/api/auto_channel', { method: 'POST' })
    .then(function(res) { return res.json(); })
    .then(function(d) {
      if (opt0) {
        opt0.innerText = currentLang === 'en' 
          ? '⚡ Channel 0 - Auto Optimal (Detected: Ch ' + (d.channel || '?') + ')'
          : '⚡ Kênh 0 - Tự Động Quét Chọn Kênh Tối Ưu (Phát hiện: Kênh ' + (d.channel || '?') + ')';
      }
      if (d.success) {
        var msg = currentLang === 'en'
          ? ('⚡ Optimal Channel Scan Result:\nChannel ' + d.channel + ' has the lowest interference!\n\n(Channel 0 is active - ESP32 will auto-broadcast on the best channel upon save & restart)')
          : ('⚡ Kết quả quét kênh tối ưu:\nKênh ' + d.channel + ' ít nhiễu và thông thoáng nhất!\n\n(Kênh 0 đang được chọn - ESP32 sẽ tự động phát trên kênh tối ưu này khi lưu & khởi động lại)');
        alert(msg);
      } else {
        alert(currentLang === 'en' ? 'Unable to scan auto channel!' : 'Không thể quét kênh tự động!');
      }
    })
    .catch(function(err) {
      if (opt0) opt0.innerText = originalText;
      console.log(err);
    });
}

// Khởi chạy ngôn ngữ và nạp trạng thái ban đầu
setLanguage(currentLang);
fetchProfiles();
updateStatus();
setInterval(updateStatus, 4000);
setInterval(function() {
  if (currentTab === 4) {
    fetchLogs();
  }
}, 2500);

// Nạp sẵn danh sách Wi-Fi từ bộ đệm của ESP32 khi vừa mở trang
setTimeout(function() {
  scanWifi(false);
}, 600);
</script>
</body>
</html>
)rawliteral";

#endif // INDEX_H
