

// ============================================================================
//  Web UI (served from the ESP32). Vanilla HTML/CSS/JS, no CDNs, no frameworks
//  so it works fully offline on the LAN / AP portal. Shared look-and-feel lives
//  in STYLE_CSS; pages keep the backend contract intact:
//    placeholders {{LOCATION}} {{VERSION}} //{{LOGS}}, form actions /update,
//    /replace-config, /reset, /update-config, and the element ids used by JS.
// ============================================================================

// Reusable bits kept as macros so every page shares the exact same chrome.
#define NAV_BLOCK(home,logs,cfg) \
"    <aside class=\"sidebar\">\n" \
"      <div class=\"brand\"><svg width=\"20\" height=\"20\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\"><path d=\"M12 2v20M3 12h18M5.2 5.2l13.6 13.6M18.8 5.2 5.2 18.8\"/></svg>Thawing Room</div>\n" \
"      <nav>\n" \
"        <a href=\"/\" " home "><svg width=\"17\" height=\"17\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M3 10.5 12 3l9 7.5\"/><path d=\"M5 9.5V20h14V9.5\"/></svg>Home</a>\n" \
"        <a href=\"/logs\" " logs "><svg width=\"17\" height=\"17\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M14 3H7a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h10a2 2 0 0 0 2-2V8z\"/><path d=\"M14 3v5h5M9 13h6M9 17h4\"/></svg>Logs</a>\n" \
"        <a href=\"/edit-settings\" " cfg "><svg width=\"17\" height=\"17\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M4 7h11M4 17h7\"/><circle cx=\"18\" cy=\"7\" r=\"2.5\"/><circle cx=\"14\" cy=\"17\" r=\"2.5\"/></svg>Settings</a>\n" \
"      </nav>\n" \
"      <div class=\"spacer\"></div>\n" \
"      <div class=\"meta\">{{LOCATION}} &middot; v{{VERSION}}</div>\n" \
"    </aside>\n"

const char* SERVER_INDEX_HTML = R"RAW_HTML(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <title>Thawing Room &middot; Device</title>
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <link rel="icon" href="data:," />
    <link rel="stylesheet" type="text/css" href="style.css" />
  </head>
  <body>
)RAW_HTML"
  NAV_BLOCK("class=\"active\" aria-current=\"page\"", "", "")
  R"RAW_HTML(
    <div class="content">
      <main class="page">
        <div class="topbar">
          <div>
            <h1>Device maintenance</h1>
            <p class="sub">Firmware, configuration and power for this controller.</p>
          </div>
          <span class="chip"><span class="dot"></span>{{LOCATION}}</span>
        </div>

        <div class="grid">
          <form id="upload_form" class="card" method="POST" action="/update" enctype="multipart/form-data">
            <div class="card-head">
              <span class="card-icon"><svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"><rect x="6" y="6" width="12" height="12" rx="2"/><path d="M9 2v3M15 2v3M9 19v3M15 19v3M2 9h3M2 15h3M19 9h3M19 15h3"/></svg></span>
              <h2>Firmware update</h2>
            </div>
            <p class="desc">Upload a new <code>.bin</code>. The device reboots when done.</p>
            <div class="file">
              <input type="file" name="update" id="file" accept=".bin" onchange="setFileLabel(this,'file-input')" />
              <label id="file-input" for="file">Choose firmware file&hellip;</label>
            </div>
            <div class="progress"><div id="bar"></div></div>
            <div class="progress-label" id="prg"></div>
            <div class="btn-row">
              <button type="submit" id="fw-btn" class="btn">Update firmware</button>
            </div>
            <small>Make sure it is the correct build for this board.</small>
          </form>

          <form name="uploadConfigForm" class="card" method="POST" action="/replace-config" enctype="multipart/form-data">
            <div class="card-head">
              <span class="card-icon"><svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"><path d="M14 3H7a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h10a2 2 0 0 0 2-2V8z"/><path d="M14 3v5h5"/></svg></span>
              <h2>Configuration file</h2>
            </div>
            <p class="desc">Replace <code>config.txt</code> with an uploaded JSON file.</p>
            <div class="file">
              <input id="config" type="file" name="upload" accept=".txt,.json" onchange="setFileLabel(this,'config-input')" />
              <label id="config-input" for="config">Choose config file&hellip;</label>
            </div>
            <div class="btn-row">
              <button type="submit" class="btn btn--ghost">Upload &amp; replace</button>
            </div>
            <small>An invalid file can stop the device from booting cleanly.</small>
          </form>

          <form id="reset_form" class="card">
            <div class="card-head">
              <span class="card-icon danger"><svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"><path d="M12 3v9"/><path d="M6.4 6.4a8 8 0 1 0 11.2 0"/></svg></span>
              <h2>Restart device</h2>
            </div>
            <p class="desc">Reboot the controller. The thawing process is interrupted.</p>
            <div class="btn-row">
              <button type="submit" class="btn btn--danger">Reboot now</button>
            </div>
            <small>Confirm no critical process is running before rebooting.</small>
          </form>
        </div>
      </main>
      <footer class="footer">
        <span>Thawing Room &middot; {{LOCATION}}</span>
        <span>v{{VERSION}} &middot; <span class="tnum" id="clock">--:--:--</span></span>
      </footer>
    </div>

    <script>
      function setFileLabel(input, labelId){
        var name = (input.value || '').split(/[\\/]/).pop();
        var l = document.getElementById(labelId);
        l.textContent = name ? name : l.getAttribute('data-empty') || 'Choose file…';
        l.classList.toggle('has-file', !!name);
      }
      var fw = document.getElementById('upload_form');
      fw.addEventListener('submit', function(e){
        e.preventDefault();
        var btn = document.getElementById('fw-btn');
        var prg = document.getElementById('prg');
        var bar = document.getElementById('bar');
        if (!document.getElementById('file').value){ prg.textContent = 'Pick a firmware file first.'; return; }
        btn.disabled = true; prg.textContent = 'Uploading… 0%';
        var xhr = new XMLHttpRequest();
        xhr.open('POST', '/update', true);
        xhr.upload.addEventListener('progress', function(evt){
          if (evt.lengthComputable){
            var per = Math.round(evt.loaded / evt.total * 100);
            bar.style.width = per + '%';
            prg.textContent = 'Uploading… ' + per + '%';
          }
        });
        xhr.onload = function(){
          if (xhr.status >= 200 && xhr.status < 300){ bar.style.width='100%'; prg.textContent = 'Done. Device is rebooting…'; }
          else { btn.disabled=false; bar.style.width='0%'; prg.textContent = 'Update failed (HTTP ' + xhr.status + ').'; }
        };
        xhr.onerror = function(){ btn.disabled=false; bar.style.width='0%'; prg.textContent = 'Update failed (connection lost).'; };
        xhr.send(new FormData(fw));
      });
      document.getElementById('reset_form').addEventListener('submit', function(e){
        e.preventDefault();
        if (!confirm('Reboot the device now? Any running process will be interrupted.')) return;
        var x = new XMLHttpRequest(); x.open('POST', '/reset', true); x.send();
      });
      function tick(){ document.getElementById('clock').textContent = new Date().toLocaleTimeString(); }
      setInterval(tick, 1000); tick();
    </script>
  </body>
</html>
)RAW_HTML";

const char* EDIT_SETTINGS_HTML = R"RAW_HTML(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <title>Thawing Room &middot; Settings</title>
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <link rel="icon" href="data:," />
    <link rel="stylesheet" type="text/css" href="style.css" />
  </head>
  <body>
)RAW_HTML"
  NAV_BLOCK("", "", "class=\"active\" aria-current=\"page\"")
  R"RAW_HTML(
    <div class="content">
      <main class="page">
        <div class="topbar">
          <div>
            <h1>Settings</h1>
            <p class="sub">Choose which configuration to edit.</p>
          </div>
          <span class="chip"><span class="dot"></span>{{LOCATION}}</span>
        </div>

        <div class="grid">
          <a class="card linkcard" href="/edit-config?file=config.txt">
            <div class="card-head">
              <span class="card-icon"><svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"><path d="M4 7h11M4 17h7"/><circle cx="18" cy="7" r="2.5"/><circle cx="14" cy="17" r="2.5"/></svg></span>
              <h2>System settings</h2>
            </div>
            <p class="desc">Wi-Fi, MQTT broker, TLS, hostname and time zone. <code>config.txt</code></p>
          </a>
          <a class="card linkcard" href="/edit-config?file=defaultParameters.txt">
            <div class="card-head">
              <span class="card-icon"><svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"><path d="M12 3a9 9 0 1 0 9 9"/><path d="M12 7v5l3 2"/></svg></span>
              <h2>Thawing parameters</h2>
            </div>
            <p class="desc">Stage timings, set-points and PID defaults. <code>defaultParameters.txt</code></p>
          </a>
        </div>
      </main>
      <footer class="footer">
        <span>Thawing Room &middot; {{LOCATION}}</span>
        <span>v{{VERSION}} &middot; <span class="tnum" id="clock">--:--:--</span></span>
      </footer>
    </div>
    <script>
      function tick(){ document.getElementById('clock').textContent = new Date().toLocaleTimeString(); }
      setInterval(tick, 1000); tick();
    </script>
  </body>
</html>
)RAW_HTML";

const char* LOGS_HTML = R"RAW_HTML(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <title>Thawing Room &middot; Logs</title>
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <link rel="icon" href="data:," />
    <link rel="stylesheet" type="text/css" href="style.css" />
  </head>
  <body>
)RAW_HTML"
  NAV_BLOCK("", "class=\"active\" aria-current=\"page\"", "")
  R"RAW_HTML(
    <div class="content">
      <main class="page">
        <div class="topbar">
          <div>
            <h1>Logs</h1>
            <p class="sub">Per-process log files stored on the SD card.</p>
          </div>
          <span class="chip"><b id="count" class="tnum">0</b>&nbsp;files</span>
        </div>
        <div id="log-list" class="log-list"></div>
      </main>
      <footer class="footer">
        <span>Thawing Room &middot; {{LOCATION}}</span>
        <span>v{{VERSION}} &middot; <span class="tnum" id="clock">--:--:--</span></span>
      </footer>
    </div>
    <script>
      const logs = [
        //{{LOGS}}
      ];
      const list = document.getElementById('log-list');
      const fileIcon = '<svg width="17" height="17" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"><path d="M14 3H7a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h10a2 2 0 0 0 2-2V8z"/><path d="M14 3v5h5"/></svg>';
      const dlIcon = '<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"><path d="M12 4v10m0 0 4-4m-4 4-4-4"/><path d="M5 19h14"/></svg>';
      if (logs.length === 0){
        list.innerHTML = '<div class="empty">No log files yet. They appear here once the controller has run a process with an SD card inserted.</div>';
      } else {
        logs.forEach(function(log){
          const row = document.createElement('div');
          row.className = 'log-row';
          row.innerHTML = fileIcon + '<a href="/download_log?file=' + log + '">' + log + '</a>' + dlIcon;
          list.appendChild(row);
        });
      }
      document.getElementById('count').textContent = logs.length;
      function tick(){ document.getElementById('clock').textContent = new Date().toLocaleTimeString(); }
      setInterval(tick, 1000); tick();
    </script>
  </body>
</html>
)RAW_HTML";

const char* EDIT_CONFIG_HTML = R"RAW_HTML(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>Thawing Room &middot; Edit config</title>
    <link rel="icon" href="data:," />
    <link rel="stylesheet" type="text/css" href="style.css" />
  </head>
  <body>
)RAW_HTML"
  NAV_BLOCK("", "", "class=\"active\" aria-current=\"page\"")
  R"RAW_HTML(
    <div class="content">
      <main class="page">
        <div class="topbar">
          <div>
            <h1 id="file-name">Edit configuration</h1>
            <p class="sub">Change a value and save. Keys map directly to the JSON file.</p>
          </div>
          <div class="btn-row">
            <button class="btn btn--ghost" type="button" onclick="downloadConfig()">Download JSON</button>
          </div>
        </div>

        <form id="configForm" class="card" action="/update-config" method="POST">
          <div id="formContent"></div>
          <hr class="divider" />
          <div class="btn-row">
            <button type="submit" class="btn">Save changes</button>
          </div>
          <small>Double-check values &mdash; a bad config can affect boot and the running process.</small>
        </form>
      </main>
      <footer class="footer">
        <span>Thawing Room &middot; {{LOCATION}}</span>
        <span>v{{VERSION}} &middot; <span class="tnum" id="clock">--:--:--</span></span>
      </footer>
    </div>
    <script>
      const urlParams = new URLSearchParams(window.location.search);
      const fileName = urlParams.get("file");
      document.getElementById("file-name").textContent = fileName ? ("Edit " + fileName) : "Edit configuration";

      document.addEventListener("DOMContentLoaded", function () {
        fetch(fileName)
          .then((r) => { if (!r.ok) throw new Error("File not found, check the file name"); return r.json(); })
          .then((data) => generateFormFields(document.getElementById("formContent"), data))
          .catch((err) => {
            document.getElementById("formContent").innerHTML =
              '<div class="empty">Could not load configuration: ' + err.message + '</div>';
          });
      });

      function generateFormFields(container, data, parentKey = "") {
        for (const key in data) {
          if (typeof data[key] === "object" && !Array.isArray(data[key])) {
            generateFormFields(container, data[key], parentKey + key + "|");
          } else {
            const id = parentKey + key;
            const field = document.createElement("div");
            field.className = "field";
            const label = document.createElement("label");
            label.setAttribute("for", id);
            label.textContent = id;
            const input = document.createElement("input");
            input.type = "text"; input.name = id; input.id = id; input.value = data[key];
            field.appendChild(label); field.appendChild(input);
            container.appendChild(field);
          }
        }
      }

      function downloadConfig() {
        fetch(fileName)
          .then((r) => r.blob())
          .then((blob) => {
            const url = window.URL.createObjectURL(blob);
            const a = document.createElement("a");
            a.style.display = "none"; a.href = url; a.download = fileName;
            document.body.appendChild(a); a.click(); window.URL.revokeObjectURL(url); a.remove();
          })
          .catch((err) => console.error("Error downloading config:", err));
      }

      function tick(){ document.getElementById('clock').textContent = new Date().toLocaleTimeString(); }
      setInterval(tick, 1000); tick();
    </script>
  </body>
</html>
)RAW_HTML";


const char* STYLE_CSS = R"RAW_CSS(
:root{
  --bg:#eef2f6;
  --surface:#ffffff;
  --surface-2:#f5f8fb;
  --border:#d6dee7;
  --border-strong:#b9c6d4;
  --ink:#16202b;
  --ink-muted:#566779;
  --accent:#0b6bcb;
  --accent-hover:#0a5cae;
  --accent-soft:rgba(11,107,203,.12);
  --danger:#c0392b;
  --danger-hover:#a5311f;
  --danger-soft:rgba(192,57,43,.12);
  --success:#1f8a4c;
  --nav-bg:#16202b;
  --nav-ink:#aab8c6;
  --radius:10px;
  --shadow-sm:0 1px 2px rgba(16,32,43,.06),0 1px 3px rgba(16,32,43,.05);
  --shadow-md:0 8px 22px rgba(16,32,43,.12);
  --nav-w:226px;
  --ease:cubic-bezier(.23,1,.32,1);
}
*{box-sizing:border-box;}
html,body{margin:0;padding:0;}
body{
  display:flex;min-height:100vh;background:var(--bg);color:var(--ink);
  font-family:system-ui,-apple-system,"Segoe UI",Roboto,Ubuntu,Cantarell,"Helvetica Neue",sans-serif;
  font-size:15px;line-height:1.5;-webkit-font-smoothing:antialiased;
}
h1{margin:0;font-size:1.375rem;font-weight:650;line-height:1.25;}
h2{margin:0;font-size:1rem;font-weight:600;}
a{color:var(--accent);text-decoration:none;}
code{font-family:ui-monospace,SFMono-Regular,Menlo,Consolas,monospace;font-size:.82em;background:var(--surface-2);
  border:1px solid var(--border);border-radius:5px;padding:1px 5px;color:var(--ink-muted);}
small{color:var(--ink-muted);font-size:.8125rem;line-height:1.45;}
.tnum{font-variant-numeric:tabular-nums;}

/* Sidebar (second neutral layer) */
.sidebar{
  position:sticky;top:0;align-self:flex-start;
  width:var(--nav-w);min-width:var(--nav-w);height:100vh;
  background:var(--nav-bg);color:var(--nav-ink);
  display:flex;flex-direction:column;padding:18px 14px;gap:4px;
}
.brand{display:flex;align-items:center;gap:10px;padding:6px 10px 16px;color:#fff;font-weight:650;font-size:.95rem;}
.brand svg{flex:none;color:#5fb0ff;}
.sidebar nav{display:flex;flex-direction:column;gap:2px;}
.sidebar nav a{
  display:flex;align-items:center;gap:11px;padding:10px 12px;border-radius:8px;
  color:var(--nav-ink);font-size:.9rem;font-weight:500;
  transition:background .15s,color .15s;
}
.sidebar nav a svg{flex:none;opacity:.8;}
.sidebar nav a:hover{background:rgba(255,255,255,.07);color:#fff;}
.sidebar nav a:focus-visible{outline:2px solid #5fb0ff;outline-offset:1px;}
.sidebar nav a.active{background:rgba(11,107,203,.22);color:#fff;}
.sidebar nav a.active svg{opacity:1;color:#5fb0ff;}
.sidebar .spacer{flex:1;}
.sidebar .meta{padding:10px;border-top:1px solid rgba(255,255,255,.08);color:#7f8ea0;font-size:.74rem;}

/* Content shell */
.content{flex:1;min-width:0;display:flex;flex-direction:column;}
.page{flex:1;width:100%;max-width:920px;margin:0 auto;padding:26px 28px 40px;}
.topbar{display:flex;align-items:flex-start;justify-content:space-between;gap:16px;margin-bottom:24px;flex-wrap:wrap;}
.topbar > div{min-width:0;}
.topbar .sub{color:var(--ink-muted);font-size:.9rem;margin:4px 0 0;}
.chip{display:inline-flex;align-items:center;gap:7px;background:var(--surface);border:1px solid var(--border);
  border-radius:999px;padding:5px 12px;font-size:.8rem;color:var(--ink-muted);box-shadow:var(--shadow-sm);white-space:nowrap;}
.chip b{color:var(--ink);}
.chip .dot{width:7px;height:7px;border-radius:50%;background:var(--success);box-shadow:0 0 0 3px rgba(31,138,76,.16);}

/* Cards */
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(270px,1fr));gap:18px;}
.card{background:var(--surface);border:1px solid var(--border);border-radius:var(--radius);
  box-shadow:var(--shadow-sm);padding:22px;display:flex;flex-direction:column;gap:13px;margin:0;}
.card-head{display:flex;align-items:center;gap:12px;}
.card-icon{display:grid;place-items:center;width:38px;height:38px;border-radius:9px;
  background:var(--accent-soft);color:var(--accent);flex:none;}
.card-icon.danger{background:var(--danger-soft);color:var(--danger);}
.card .desc{color:var(--ink-muted);font-size:.875rem;margin:-2px 0 0;}
.card .desc code{font-size:.78em;}

/* Link cards */
a.linkcard{color:inherit;cursor:pointer;
  transition:box-shadow .2s var(--ease),transform .2s var(--ease),border-color .15s;}
a.linkcard:hover{box-shadow:var(--shadow-md);transform:translateY(-2px);border-color:var(--border-strong);}
a.linkcard:focus-visible{outline:2px solid var(--accent);outline-offset:2px;}

/* Buttons */
.btn{display:inline-flex;align-items:center;justify-content:center;gap:8px;height:40px;padding:0 16px;
  border-radius:8px;border:1px solid transparent;font-size:.9rem;font-weight:600;font-family:inherit;
  cursor:pointer;background:var(--accent);color:#fff;
  transition:background .15s,border-color .15s,transform .12s var(--ease),box-shadow .15s;}
.btn:hover{background:var(--accent-hover);}
.btn:active{transform:scale(.97);}
.btn:focus-visible{outline:2px solid var(--accent);outline-offset:2px;}
.btn:disabled{opacity:.5;cursor:not-allowed;transform:none;}
.btn--ghost{background:var(--surface);border-color:var(--border);color:var(--ink);}
.btn--ghost:hover{background:var(--surface-2);border-color:var(--border-strong);}
.btn--danger{background:var(--danger);}
.btn--danger:hover{background:var(--danger-hover);}
.btn--danger:focus-visible{outline-color:var(--danger);}
.btn-row{display:flex;flex-wrap:wrap;gap:10px;align-items:center;}

/* File picker */
.file input[type=file]{display:none;}
.file label{display:flex;align-items:center;gap:10px;padding:11px 14px;border:1.5px dashed var(--border);
  border-radius:8px;color:var(--ink-muted);background:var(--surface-2);cursor:pointer;font-size:.875rem;
  word-break:break-all;transition:border-color .15s,color .15s,background .15s;}
.file label:hover{border-color:var(--accent);color:var(--accent);}
.file label.has-file{border-style:solid;border-color:var(--accent);color:var(--ink);background:#fff;}

/* Progress */
.progress{height:8px;background:var(--surface-2);border:1px solid var(--border);border-radius:999px;overflow:hidden;}
.progress > div{height:100%;width:0;background:var(--accent);border-radius:999px;transition:width .2s var(--ease);}
.progress-label{font-size:.8rem;color:var(--ink-muted);min-height:1em;}

/* Form fields (edit-config) */
.field{display:flex;flex-direction:column;gap:5px;margin-bottom:14px;}
.field label{font-size:.78rem;font-weight:600;color:var(--ink-muted);word-break:break-all;}
.field input{height:40px;padding:0 13px;border:1px solid var(--border);border-radius:8px;background:#fff;
  color:var(--ink);font-size:.9rem;font-family:inherit;width:100%;
  transition:border-color .15s,box-shadow .15s;}
.field input:hover{border-color:var(--border-strong);}
.field input:focus{outline:none;border-color:var(--accent);box-shadow:0 0 0 3px var(--accent-soft);}
.divider{height:1px;background:var(--border);border:0;margin:6px 0 16px;}

/* Logs */
.log-list{display:flex;flex-direction:column;gap:8px;}
.log-row{display:flex;align-items:center;gap:12px;padding:13px 15px;background:var(--surface);
  border:1px solid var(--border);border-radius:9px;box-shadow:var(--shadow-sm);
  transition:border-color .15s,background .15s;}
.log-row:hover{border-color:var(--border-strong);background:var(--surface-2);}
.log-row a{color:var(--ink);font-weight:500;font-size:.9rem;flex:1;word-break:break-all;}
.log-row > svg:first-child{color:var(--ink-muted);flex:none;}
.log-row a + svg{color:var(--accent);flex:none;}
.empty{padding:34px 28px;text-align:center;color:var(--ink-muted);font-size:.9rem;
  border:1.5px dashed var(--border);border-radius:var(--radius);background:var(--surface-2);}

/* Footer */
.footer{position:sticky;bottom:0;display:flex;align-items:center;justify-content:space-between;gap:16px;
  padding:9px 28px;background:var(--surface);border-top:1px solid var(--border);
  color:var(--ink-muted);font-size:.78rem;}
.footer .tnum{color:var(--ink);}

/* Responsive: sidebar becomes a top bar on narrow screens */
@media (max-width:760px){
  body{flex-direction:column;}
  .sidebar{flex-direction:row;align-items:center;width:100%;min-width:0;height:auto;
    padding:10px 12px;gap:6px;overflow-x:auto;}
  .brand{padding:0 8px 0 4px;white-space:nowrap;}
  .sidebar nav{flex-direction:row;}
  .sidebar .spacer,.sidebar .meta{display:none;}
  .page{padding:20px 16px 32px;}
  .footer{padding:9px 16px;}
  h1{font-size:1.2rem;}
}

@media (prefers-reduced-motion:reduce){
  *{transition:none !important;}
  .btn:active{transform:none;}
  a.linkcard:hover{transform:none;}
}
)RAW_CSS";
