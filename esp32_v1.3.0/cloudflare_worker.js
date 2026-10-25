/**
 * =========================================================================
 *   CLOUDFLARE WORKER: ESP32 GATEWAY REMOTE WEB PORTAL (QUẢN LÝ TỪ XA)
 * =========================================================================
 * HƯỚNG DẪN TRIỂN KHAI 2 PHÚT (HOÀN TOÀN MIỄN PHÍ TRỌN ĐỜI):
 * 1. Đăng ký/Đăng nhập Cloudflare: https://dash.cloudflare.com/
 * 2. Vào menu: "Workers & Pages" -> Chọn "Create Application" -> "Create Worker".
 * 3. Đặt tên (ví dụ: `my-esp32-gateway`) -> Nhấn "Deploy".
 * 4. Nhấn "Edit code" -> Xóa hết code cũ và DÁN TOÀN BỘ file này vào.
 * 5. Nhấn "Deploy" -> Bạn sẽ có đường link HTTPS an toàn:
 *    https://my-esp32-gateway.<your-name>.workers.dev
 * 6. Mở Web Portal nội bộ của ESP32 (10.51.31.254) hoặc gửi lệnh BLE Terminal:
 *    CLOUD:CONFIG:https://my-esp32-gateway.<your-name>.workers.dev,686868
 * =========================================================================
 */

// CẤU HÌNH MÃ KHÓA BẢO MẬT (Phải trùng với mã key trên ESP32)
const ACCESS_KEY = "686868";

// Trạng thái thiết bị lưu trong RAM của Cloudflare Worker (Global Memory)
let deviceStatus = {
  sta_ssid: "---",
  sta_ip: "---",
  sta_rssi: 0,
  sta_enabled: true,
  ap_ssid: "ODE - wireless WiFi",
  ap_pass: "Ode6868@",
  ap_channel: 6,
  effective_channel: 6,
  ap_ip: "10.51.31.254",
  ap_subnet: "255.255.252.0",
  dhcp_start: "10.51.28.10",
  dhcp_end: "10.51.28.200",
  tx_boost: false,
  ble_enabled: true,
  napt: false,
  free_heap: 0,
  uptime: 0,
  fw_ver: "1.3.0"
};
let lastSyncTime = 0;
let pendingCommands = [];
let recentLogs = [];

export default {
  async fetch(request, env, ctx) {
    const url = new URL(request.url);

    // 1. ESP32 GỌI ĐỒNG BỘ: POST /api/sync
    if (url.pathname === "/api/sync" && request.method === "POST") {
      const authHeader = request.headers.get("X-ESP32-Key");
      if (authHeader !== ACCESS_KEY) {
        return new Response(JSON.stringify({ error: "Unauthorized" }), { status: 401 });
      }

      try {
        const body = await request.json();
        deviceStatus = { ...deviceStatus, ...body };
        lastSyncTime = Date.now();

        // Lấy danh sách lệnh cần gửi xuống ESP32
        const cmdsToSend = [...pendingCommands];
        pendingCommands = []; // Xóa các lệnh đã gửi

        return new Response(JSON.stringify({ success: true, commands: cmdsToSend }), {
          headers: { "Content-Type": "application/json" }
        });
      } catch (e) {
        return new Response(JSON.stringify({ error: e.message }), { status: 400 });
      }
    }

    // 2. API TRẢ VỀ TRẠNG THÁI CHO TRÌNH DUYỆT TỪ XA: GET /api/status
    if (url.pathname === "/api/status") {
      const isOnline = (Date.now() - lastSyncTime < 9000); // Coi là online nếu đồng bộ trong 9s
      const data = {
        online: isOnline,
        last_sync_ago: Math.round((Date.now() - lastSyncTime) / 1000),
        ...deviceStatus
      };
      return new Response(JSON.stringify(data), {
        headers: { "Content-Type": "application/json", "Access-Control-Allow-Origin": "*" }
      });
    }

    // 3. API GỬI LỆNH TỪ XA XUỐNG ESP32: POST /api/command
    if (url.pathname === "/api/command" && request.method === "POST") {
      const authHeader = request.headers.get("X-Access-Key") || url.searchParams.get("key");
      if (authHeader !== ACCESS_KEY) {
        return new Response(JSON.stringify({ error: "Unauthorized" }), { status: 401 });
      }
      try {
        const body = await request.json();
        if (body.cmd && body.cmd.trim().length > 0) {
          pendingCommands.push(body.cmd.trim());
          return new Response(JSON.stringify({ 
            success: true, 
            msg: `Đã xếp hàng lệnh: '${body.cmd}'. ESP32 sẽ thực thi sau ~1-2 giây!` 
          }), { headers: { "Content-Type": "application/json" } });
        }
        return new Response(JSON.stringify({ error: "Thiếu lệnh (cmd)" }), { status: 400 });
      } catch (e) {
        return new Response(JSON.stringify({ error: e.message }), { status: 400 });
      }
    }

    // 4. KIỂM TRA ĐĂNG NHẬP CHO GIAO DIỆN WEB
    const cookies = request.headers.get("Cookie") || "";
    const hasAuthCookie = cookies.includes(`esp32_auth=${ACCESS_KEY}`);
    const keyParam = url.searchParams.get("key");

    if (keyParam === ACCESS_KEY || hasAuthCookie) {
      // Đã đăng nhập hợp lệ: Trả về trang Web Portal Quản lý Từ xa
      return new Response(renderRemotePortalHtml(ACCESS_KEY), {
        headers: {
          "Content-Type": "text/html; charset=UTF-8",
          "Set-Cookie": `esp32_auth=${ACCESS_KEY}; Path=/; HttpOnly; Max-Age=2592000; SameSite=Strict`
        }
      });
    }

    // 5. NẾU CHƯA ĐĂNG NHẬP: Trả về trang Login
    if (request.method === "POST" && url.pathname === "/login") {
      const formData = await request.formData();
      const enteredKey = formData.get("access_key");
      if (enteredKey === ACCESS_KEY) {
        return new Response("", {
          status: 302,
          headers: {
            "Location": "/",
            "Set-Cookie": `esp32_auth=${ACCESS_KEY}; Path=/; HttpOnly; Max-Age=2592000; SameSite=Strict`
          }
        });
      } else {
        return new Response(renderLoginHtml("Sai Access Key! Vui lòng thử lại."), {
          headers: { "Content-Type": "text/html; charset=UTF-8" }
        });
      }
    }

    return new Response(renderLoginHtml(), {
      headers: { "Content-Type": "text/html; charset=UTF-8" }
    });
  }
};

// GIAO DIỆN ĐĂNG NHẬP BẢO MẬT
function renderLoginHtml(errMsg = "") {
  return `<!DOCTYPE html>
<html lang="vi">
<head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Đăng nhập ESP32 Cloud Remote</title>
<style>
  body { background: #030712; color: #f9fafb; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; display: flex; align-items: center; justify-content: center; min-height: 100vh; margin: 0; padding: 16px; box-sizing: border-box; }
  .box { background: #111827; border: 1px solid #374151; border-radius: 16px; padding: 24px; max-width: 360px; width: 100%; box-shadow: 0 10px 25px rgba(0,0,0,0.5); text-align: center; }
  .icon { font-size: 36px; margin-bottom: 12px; }
  h2 { margin: 0 0 6px 0; font-size: 18px; color: #fff; }
  p { font-size: 12px; color: #9ca3af; margin: 0 0 16px 0; }
  input { width: 100%; padding: 12px; background: #1f2937; border: 1px solid #4b5563; border-radius: 8px; color: #fff; font-size: 14px; text-align: center; letter-spacing: 2px; margin-bottom: 12px; box-sizing: border-box; }
  button { width: 100%; padding: 12px; background: linear-gradient(135deg, #0284c7, #2563eb); border: none; border-radius: 8px; color: #fff; font-weight: 700; font-size: 14px; cursor: pointer; }
  .err { color: #f87171; font-size: 11px; margin-bottom: 12px; }
</style>
</head>
<body>
<div class="box">
  <div class="icon">☁️</div>
  <h2>ESP32 Cloud Remote</h2>
  <p>Nhập mã khóa xác thực bảo mật (Access Key) để mở trang quản trị từ xa</p>
  ${errMsg ? `<div class="err">${errMsg}</div>` : ""}
  <form method="POST" action="/login">
    <input type="password" name="access_key" placeholder="Mã Access Key" required autofocus>
    <button type="submit">Xác Thực & Truy Cập 🚀</button>
  </form>
</div>
</body>
</html>`;
}

// GIAO DIỆN WEB PORTAL QUẢN LÝ TỪ XA TRỌN VẸN
function renderRemotePortalHtml(accessKey) {
  return `<!DOCTYPE html>
<html lang="vi">
<head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP32 Gateway • Cloud Remote</title>
<style>
  :root { --bg: #030712; --card: #111827; --card-inner: #1f2937; --border: #374151; --text: #f9fafb; --text-muted: #9ca3af; --accent: #0284c7; --success: #10b981; --warning: #f59e0b; --danger: #ef4444; }
  * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
  body { background: var(--bg); color: var(--text); padding: 12px; min-height: 100vh; display: flex; justify-content: center; }
  .container { width: 100%; max-width: 520px; }
  .header { background: var(--card); border: 1px solid var(--border); border-radius: 16px; padding: 14px 16px; margin-bottom: 12px; display: flex; align-items: center; justify-content: space-between; }
  .header-left { display: flex; align-items: center; gap: 10px; }
  .header-icon { width: 36px; height: 36px; border-radius: 10px; background: linear-gradient(135deg, #0284c7, #2563eb); display: flex; align-items: center; justify-content: center; font-size: 18px; }
  .badge { font-size: 10px; font-weight: 700; padding: 4px 8px; border-radius: 9999px; text-transform: uppercase; }
  .badge-success { background: rgba(16, 185, 129, 0.2); color: #34d399; border: 1px solid rgba(16, 185, 129, 0.4); }
  .badge-danger { background: rgba(239, 68, 68, 0.2); color: #f87171; border: 1px solid rgba(239, 68, 68, 0.4); }
  .badge-warning { background: rgba(245, 158, 11, 0.2); color: #fbbf24; border: 1px solid rgba(245, 158, 11, 0.4); }
  .badge-blue { background: rgba(2, 132, 199, 0.2); color: #38bdf8; border: 1px solid rgba(2, 132, 199, 0.4); }
  .tabs { display: grid; grid-template-columns: 1fr 1fr 1fr 1fr; gap: 4px; background: var(--card); border: 1px solid var(--border); padding: 4px; border-radius: 12px; margin-bottom: 12px; }
  .tab-btn { background: transparent; border: none; color: var(--text-muted); padding: 8px 2px; font-size: 11px; font-weight: 600; border-radius: 8px; cursor: pointer; text-align: center; }
  .tab-btn.active { background: var(--accent); color: #fff; }
  .card { background: var(--card); border: 1px solid var(--border); border-radius: 16px; padding: 14px; margin-bottom: 12px; }
  .card-title { font-size: 12px; font-weight: 700; color: #94a3b8; margin-bottom: 12px; display: flex; justify-content: space-between; align-items: center; }
  .info-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; }
  .info-box { background: var(--card-inner); border: 1px solid var(--border); border-radius: 10px; padding: 8px 10px; }
  .info-label { font-size: 10px; color: var(--text-muted); margin-bottom: 2px; }
  .info-val { font-size: 12px; font-weight: 700; font-family: monospace; word-break: break-all; }
  .btn { width: 100%; padding: 10px; border-radius: 8px; border: none; font-size: 12px; font-weight: 700; cursor: pointer; transition: 0.2s; margin-top: 8px; }
  .btn-primary { background: linear-gradient(135deg, #0284c7, #2563eb); color: #fff; }
  .btn-success { background: linear-gradient(135deg, #059669, #10b981); color: #fff; }
  .btn-danger { background: linear-gradient(135deg, #dc2626, #ef4444); color: #fff; }
  .form-group { margin-bottom: 10px; }
  .form-group label { display: block; font-size: 11px; color: var(--text-muted); margin-bottom: 4px; }
  .form-group input, .form-group select { width: 100%; padding: 8px 10px; background: var(--card-inner); border: 1px solid var(--border); border-radius: 8px; color: #fff; font-size: 12px; }
  .toast { position: fixed; top: 16px; left: 50%; transform: translateX(-50%); background: #111827; border: 1px solid #38bdf8; color: #38bdf8; padding: 8px 16px; border-radius: 9999px; font-size: 11px; font-weight: 700; z-index: 9999; box-shadow: 0 4px 16px rgba(0,0,0,0.6); }
  .hidden { display: none !important; }
</style>
</head>
<body>
<div id="toastBox" class="toast hidden"></div>
<div class="container">
  
  <!-- Header -->
  <div class="header">
    <div class="header-left">
      <div class="header-icon">☁️</div>
      <div>
        <div style="font-size: 14px; font-weight: 700;">ESP32 Cloud Remote</div>
        <div style="font-size: 10px; color: var(--text-muted); font-family: monospace;">Edge CDN • Cloudflare Worker</div>
      </div>
    </div>
    <span id="cloudBadge" class="badge badge-warning">ĐANG KẾT NỐI...</span>
  </div>

  <!-- Tabs Navigation -->
  <div class="tabs">
    <button id="btnTab1" class="tab-btn active" onclick="showTab(1)">📊 Trạng Thái</button>
    <button id="btnTab2" class="tab-btn" onclick="showTab(2)">🌐 Wi-Fi Nguồn</button>
    <button id="btnTab3" class="tab-btn" onclick="showTab(3)">📡 Mạng Phát</button>
    <button id="btnTab4" class="tab-btn" onclick="showTab(4)">📟 Điều Khiển</button>
  </div>

  <!-- TAB 1: TRẠNG THÁI TỔNG QUAN -->
  <div id="tab1">
    <div class="card">
      <div class="card-title">
        <span>🌐 KẾT NỐI WI-FI NGUỒN (STA)</span>
        <span id="badgeStaStatus" class="badge badge-success">ONLINE</span>
      </div>
      <div class="info-grid">
        <div class="info-box"><div class="info-label">Tên Wi-Fi nguồn</div><div class="info-val" id="valStaSsid">---</div></div>
        <div class="info-box"><div class="info-label">Sóng RSSI</div><div class="info-val" style="color:#a7f3d0;" id="valStaRssi">---</div></div>
        <div class="info-box" style="grid-column: span 2;"><div class="info-label">IP ESP32 nhận từ Router nguồn</div><div class="info-val" style="color:#67e8f9;" id="valStaIp">---</div></div>
      </div>
    </div>

    <div class="card">
      <div class="card-title">
        <span>📡 MẠNG PHÁT CỤC BỘ (SOFTAP)</span>
        <span id="badgeApChannel" class="badge badge-blue">KÊNH 6</span>
      </div>
      <div class="info-grid">
        <div class="info-box"><div class="info-label">Tên Wi-Fi phát</div><div class="info-val" style="color:#38bdf8;" id="valApSsid">---</div></div>
        <div class="info-box"><div class="info-label">Mật khẩu phát</div><div class="info-val" style="color:#fde047;" id="valApPass">---</div></div>
        <div class="info-box"><div class="info-label">IP Gateway</div><div class="info-val" id="valApIp">10.51.31.254</div></div>
        <div class="info-box"><div class="info-label">Định tuyến NAPT</div><div class="info-val" style="color:#34d399;" id="valNapt">ON</div></div>
        <div class="info-box" style="grid-column: span 2;"><div class="info-label">Dải IP DHCP cấp ngẫu nhiên</div><div class="info-val" style="color:#c7d2fe;" id="valDhcpPool">---</div></div>
      </div>
    </div>

    <div class="card" style="padding: 10px;">
      <div style="display: flex; justify-content: space-around; text-align: center;">
        <div><div class="info-label">RAM Trống</div><div class="info-val" style="color:#34d399;" id="valHeap">---</div></div>
        <div style="width: 1px; background: var(--border);"></div>
        <div><div class="info-label">Uptime</div><div class="info-val" id="valUptime">---</div></div>
        <div style="width: 1px; background: var(--border);"></div>
        <div><div class="info-label">Firmware</div><div class="info-val" style="color:#fbbf24;" id="valFwVer">v1.3.0</div></div>
      </div>
    </div>
  </div>

  <!-- TAB 2: CẤU HÌNH WI-FI NGUỒN TỪ XA -->
  <div id="tab2" class="hidden">
    <div class="card">
      <div class="card-title">Đổi Wi-Fi Nguồn Cấp Mạng Cho ESP32</div>
      <div class="form-group">
        <label>Tên Wi-Fi Nguồn Mới (SSID)</label>
        <input type="text" id="inputNewStaSsid" placeholder="Nhập tên Wi-Fi nguồn">
      </div>
      <div class="form-group">
        <label>Mật Khẩu Wi-Fi Nguồn</label>
        <input type="password" id="inputNewStaPass" placeholder="Nhập mật khẩu (để trống nếu Open)">
      </div>
      <button class="btn btn-primary" onclick="remoteSetSta()">🚀 Gửi Lệnh Kết Nối Wi-Fi Mới</button>
    </div>

    <div class="card">
      <div class="card-title">Chế Độ Wi-Fi Nguồn</div>
      <div style="font-size: 11px; color: var(--text-muted); margin-bottom: 10px;">Bật hoặc tắt kết nối Wi-Fi nguồn từ xa:</div>
      <div style="display: flex; gap: 8px;">
        <button class="btn btn-success" style="margin-top:0;" onclick="sendCommand('STA:ON')">Bật Wi-Fi Nguồn</button>
        <button class="btn btn-danger" style="margin-top:0;" onclick="sendCommand('STA:OFF')">Tắt (Mạng nội bộ 5G)</button>
      </div>
    </div>
  </div>

  <!-- TAB 3: CẤU HÌNH MẠNG PHÁT SOFTAP TỪ XA -->
  <div id="tab3" class="hidden">
    <div class="card">
      <div class="card-title">Đổi Tên & Mật Khẩu Wi-Fi Phát</div>
      <div class="form-group">
        <label>Tên Wi-Fi Phát (AP SSID)</label>
        <input type="text" id="inputNewApSsid">
      </div>
      <div class="form-group">
        <label>Mật Khẩu Phát</label>
        <input type="text" id="inputNewApPass">
      </div>
      <button class="btn btn-primary" onclick="remoteSetAp()">💾 Áp Dụng & Tự Động Khởi Động Lại</button>
    </div>

    <div class="card">
      <div class="card-title">Đổi Kênh Phát Sóng (1 - 11 hoặc Auto)</div>
      <div class="form-group">
        <select id="selectNewChannel">
          <option value="0">⚡ Kênh 0 - Tự Động Quét Kênh Tối Ưu</option>
          <option value="1">Kênh 1 (2.412 GHz)</option>
          <option value="6">Kênh 6 (2.437 GHz)</option>
          <option value="11">Kênh 11 (2.462 GHz)</option>
          <option value="2">Kênh 2</option><option value="3">Kênh 3</option><option value="4">Kênh 4</option>
          <option value="5">Kênh 5</option><option value="7">Kênh 7</option><option value="8">Kênh 8</option>
          <option value="9">Kênh 9</option><option value="10">Kênh 10</option>
        </select>
      </div>
      <button class="btn btn-primary" onclick="remoteSetChannel()">📡 Đổi Kênh Phát</button>
    </div>

    <div class="card">
      <div class="card-title">Công Suất Phát Sóng Cực Đại (Boost)</div>
      <div style="display: flex; gap: 8px;">
        <button class="btn btn-success" style="margin-top:0;" onclick="sendCommand('BOOST:ON')">⚡ Bật Boost 20 dBm</button>
        <button class="btn btn-primary" style="margin-top:0; background:#4b5563;" onclick="sendCommand('BOOST:OFF')">🍃 Tiêu Chuẩn 15 dBm</button>
      </div>
    </div>
  </div>

  <!-- TAB 4: CONSOLE TERMINAL & KHỞI ĐỘNG LẠI -->
  <div id="tab4" class="hidden">
    <div class="card">
      <div class="card-title">Terminal Gửi Lệnh Trực Tiếp Tới ESP32</div>
      <div class="form-group">
        <input type="text" id="customCmdInput" placeholder="Ví dụ: HELP, STATUS, AUTOCH, BOOST:ON, REBOOT">
      </div>
      <button class="btn btn-primary" onclick="sendCustomCommand()">📤 Gửi Lệnh Ngay</button>
    </div>

    <div class="card">
      <div class="card-title">Khởi Động Lại Thiết Bị (Reboot)</div>
      <div style="font-size: 11px; color: var(--text-muted); margin-bottom: 8px;">Gửi lệnh khởi động lại ESP32 từ xa qua Internet:</div>
      <button class="btn btn-danger" onclick="if(confirm('Khởi động lại ESP32 ngay bây giờ?')) sendCommand('REBOOT')">⚠️ Khởi Động Lại ESP32 (Reboot)</button>
    </div>
  </div>

</div>

<script>
var accessKey = "${accessKey}";

function showToast(msg) {
  var t = document.getElementById('toastBox');
  t.innerText = msg;
  t.classList.remove('hidden');
  setTimeout(function() { t.classList.add('hidden'); }, 3000);
}

function showTab(n) {
  for (var i = 1; i <= 4; i++) {
    document.getElementById('tab' + i).classList.add('hidden');
    document.getElementById('btnTab' + i).classList.remove('active');
  }
  document.getElementById('tab' + n).classList.remove('hidden');
  document.getElementById('btnTab' + n).classList.add('active');
}

function sendCommand(cmdStr) {
  fetch('/api/command?key=' + accessKey, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ cmd: cmdStr })
  })
  .then(function(res) { return res.json(); })
  .then(function(d) {
    showToast(d.msg || 'Đã gửi lệnh!');
    setTimeout(refreshStatus, 2500);
  })
  .catch(function(err) {
    alert('Lỗi gửi lệnh: ' + err);
  });
}

function remoteSetSta() {
  var s = document.getElementById('inputNewStaSsid').value.trim();
  var p = document.getElementById('inputNewStaPass').value.trim();
  if (!s) return alert('Vui lòng nhập tên Wi-Fi nguồn!');
  sendCommand('WIFI:' + s + (p ? (',' + p) : ''));
}

function remoteSetAp() {
  var s = document.getElementById('inputNewApSsid').value.trim();
  var p = document.getElementById('inputNewApPass').value.trim();
  if (!s) return alert('Vui lòng nhập tên Wi-Fi phát!');
  sendCommand('AP:' + s + (p ? (',' + p) : ''));
}

function remoteSetChannel() {
  var ch = document.getElementById('selectNewChannel').value;
  sendCommand(ch === '0' ? 'AUTOCH' : ('APCH:' + ch));
}

function sendCustomCommand() {
  var cmd = document.getElementById('customCmdInput').value.trim();
  if (!cmd) return;
  sendCommand(cmd);
  document.getElementById('customCmdInput').value = '';
}

function refreshStatus() {
  fetch('/api/status')
    .then(function(res) { return res.json(); })
    .then(function(d) {
      var badge = document.getElementById('cloudBadge');
      if (d.online) {
        badge.className = 'badge badge-success';
        badge.innerText = 'ONLINE 🟢';
      } else {
        badge.className = 'badge badge-danger';
        badge.innerText = 'OFFLINE (' + (d.last_sync_ago || '?') + 's)';
      }

      var staB = document.getElementById('badgeStaStatus');
      if (d.sta_enabled === false) {
        staB.className = 'badge badge-blue';
        staB.innerText = 'MẠNG NỘI BỘ (5G)';
      } else if (d.sta_ip && d.sta_ip !== '---' && d.sta_ip !== '0.0.0.0') {
        staB.className = 'badge badge-success';
        staB.innerText = 'ĐÃ KẾT NỐI';
      } else {
        staB.className = 'badge badge-danger';
        staB.innerText = 'CHƯA KẾT NỐI';
      }

      document.getElementById('valStaSsid').innerText = d.sta_ssid || '---';
      document.getElementById('valStaRssi').innerText = d.sta_rssi ? (d.sta_rssi + ' dBm') : '---';
      document.getElementById('valStaIp').innerText = d.sta_ip || '---';

      document.getElementById('valApSsid').innerText = d.ap_ssid || '---';
      document.getElementById('valApPass').innerText = d.ap_pass || '(Mở / Không pass)';
      document.getElementById('badgeApChannel').innerText = 'KÊNH ' + (d.effective_channel || d.ap_channel || 6);
      document.getElementById('valApIp').innerText = d.ap_ip || '10.51.31.254';
      document.getElementById('valNapt').innerText = d.napt ? 'ON' : 'OFF';
      document.getElementById('valDhcpPool').innerText = (d.dhcp_start || '10.51.28.10') + '  ➔  ' + (d.dhcp_end || '10.51.28.200');

      document.getElementById('valHeap').innerText = d.free_heap ? (Math.round(d.free_heap / 1024) + ' KB') : '---';
      var u = d.uptime || 0;
      var hrs = Math.floor(u / 3600);
      var mins = Math.floor((u % 3600) / 60);
      document.getElementById('valUptime').innerText = hrs + 'h ' + mins + 'm';
      document.getElementById('valFwVer').innerText = 'v' + (d.fw_ver || '1.3.0');

      if (!document.getElementById('inputNewApSsid').value && d.ap_ssid) {
        document.getElementById('inputNewApSsid').value = d.ap_ssid;
      }
      if (!document.getElementById('inputNewApPass').value && d.ap_pass) {
        document.getElementById('inputNewApPass').value = d.ap_pass;
      }
      if (d.ap_channel !== undefined) {
        document.getElementById('selectNewChannel').value = d.ap_channel;
      }
    })
    .catch(function(e) {
      console.log('Refresh status error:', e);
    });
}

// Tự động làm mới mỗi 3.5 giây
refreshStatus();
setInterval(refreshStatus, 3500);
</script>
</body>
</html>`;
}
