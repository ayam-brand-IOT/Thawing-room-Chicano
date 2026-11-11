

// ============================================================================
//  Web UI (served from the ESP32). Vanilla HTML/CSS/JS, no CDNs, no frameworks
//  so it works fully offline on the LAN / AP portal. Shared look-and-feel lives
//  in STYLE_CSS; pages keep the backend contract intact:
//    placeholders {{LOCATION}} {{VERSION}} //{{LOGS}}, form actions /update,
//    /replace-config, /reset, /update-config, and the element ids used by JS.
// ============================================================================

// Reusable bits kept as macros so every page shares the exact same chrome.
#define NAV_BLOCK(control,home,events,logs,cfg,help) \
"    <aside class=\"sidebar\">\n" \
"      <div class=\"brand\"><svg width=\"20\" height=\"20\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\"><path d=\"M12 2v20M3 12h18M5.2 5.2l13.6 13.6M18.8 5.2 5.2 18.8\"/></svg>Thawing Room</div>\n" \
"      <nav>\n" \
"        <a href=\"/control\" " control "><svg width=\"17\" height=\"17\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><circle cx=\"12\" cy=\"12\" r=\"9\"/><path d=\"m10 8.5 5 3.5-5 3.5z\"/></svg>Control</a>\n" \
"        <a href=\"/\" " home "><svg width=\"17\" height=\"17\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M3 10.5 12 3l9 7.5\"/><path d=\"M5 9.5V20h14V9.5\"/></svg>Device</a>\n" \
"        <a href=\"/events\" " events "><svg width=\"17\" height=\"17\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M3 12h4l3-8 4 16 3-8h4\"/></svg>Events</a>\n" \
"        <a href=\"/logs\" " logs "><svg width=\"17\" height=\"17\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M14 3H7a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h10a2 2 0 0 0 2-2V8z\"/><path d=\"M14 3v5h5M9 13h6M9 17h4\"/></svg>Logs</a>\n" \
"        <a href=\"/edit-settings\" " cfg "><svg width=\"17\" height=\"17\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M4 7h11M4 17h7\"/><circle cx=\"18\" cy=\"7\" r=\"2.5\"/><circle cx=\"14\" cy=\"17\" r=\"2.5\"/></svg>Settings</a>\n" \
"        <a href=\"/help\" " help "><svg width=\"17\" height=\"17\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><circle cx=\"12\" cy=\"12\" r=\"9\"/><path d=\"M9.5 9.2a2.6 2.6 0 0 1 5 .9c0 1.7-2.5 2.2-2.5 3.9\"/><path d=\"M12 17h.01\"/></svg>Help</a>\n" \
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
  NAV_BLOCK("", "class=\"active\" aria-current=\"page\"", "", "", "", "")
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
            <div class="progress-label" id="prg" role="status" aria-live="polite"></div>
            <div class="btn-row">
              <button type="submit" id="fw-btn" class="btn">Update firmware</button>
            </div>
            <small>Make sure it is the correct build for this board.</small>
          </form>

          <form name="uploadConfigForm" id="config_form" class="card" method="POST" action="/replace-config" enctype="multipart/form-data">
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
              <button type="submit" class="btn btn--ghost" id="cfg-btn">Upload &amp; replace</button>
            </div>
            <div class="dv-msg" id="cfg-msg" role="status" aria-live="polite" hidden></div>
            <small>The file is checked before it replaces anything: invalid JSON is rejected and the current config is kept.</small>
          </form>

          <form id="reset_form" class="card">
            <div class="card-head">
              <span class="card-icon danger"><svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"><path d="M12 3v9"/><path d="M6.4 6.4a8 8 0 1 0 11.2 0"/></svg></span>
              <h2>Restart device</h2>
            </div>
            <p class="desc">Reboot the controller. The thawing process is interrupted.</p>
            <div class="btn-row">
              <button type="submit" class="btn btn--danger" id="reset-btn">Reboot now</button>
            </div>
            <div class="dv-msg" id="reset-msg" role="status" aria-live="polite" hidden></div>
            <small>A running process resumes where it was after the restart.</small>
          </form>
        </div>
      </main>
      <footer class="footer">
        <span>Thawing Room &middot; {{LOCATION}}</span>
        <span>v{{VERSION}} &middot; <span class="tnum" id="clock">--:--:--</span></span>
      </footer>
    </div>

    <dialog class="ct-dialog" id="dlg" aria-labelledby="dlg-title" aria-describedby="dlg-body">
      <form method="dialog" id="dlg-form">
        <h2 id="dlg-title"></h2>
        <p class="ct-dialog-body" id="dlg-body"></p>
        <div class="ct-dialog-actions">
          <button type="button" class="btn btn--ghost" id="dlg-cancel">Cancel</button>
          <button type="submit" class="btn" id="dlg-ok">Confirm</button>
        </div>
      </form>
    </dialog>

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
          if (xhr.status >= 200 && xhr.status < 300 && xhr.responseText.trim() !== 'FAIL'){
            bar.style.width='100%'; prg.textContent = 'Installed. Restarting on the new firmware…';
            waitForController().then(function(back){
              btn.disabled = false;
              prg.textContent = back ? 'Back online on the new firmware.' : 'The controller has not come back yet. Give it a minute, then reload this page.';
            });
          } else {
            btn.disabled=false; bar.style.width='0%';
            prg.textContent = xhr.responseText.trim() === 'FAIL'
              ? 'The firmware was rejected. The controller restarts on its current firmware.'
              : 'Update failed (HTTP ' + xhr.status + ').';
          }
        };
        xhr.onerror = function(){ btn.disabled=false; bar.style.width='0%'; prg.textContent = 'Update failed (connection lost).'; };
        xhr.send(new FormData(fw));
      });
      // ---- helpers: mensaje en la tarjeta, confirmación y espera tras reiniciar ----
      function say(id, kind, html){
        var el = document.getElementById(id);
        el.className = 'dv-msg is-' + kind; el.innerHTML = html; el.hidden = false;
      }
      var dlg = document.getElementById('dlg'), dlgAction = null;
      function ask(title, body, okText, danger, action){
        document.getElementById('dlg-title').textContent = title;
        document.getElementById('dlg-body').textContent = body;
        var ok = document.getElementById('dlg-ok');
        ok.textContent = okText; ok.className = 'btn' + (danger ? ' btn--danger' : '');
        dlgAction = action;
        dlg.showModal();
        document.getElementById('dlg-cancel').focus();
      }
      document.getElementById('dlg-cancel').addEventListener('click', function(){ dlg.close(); });
      document.getElementById('dlg-form').addEventListener('submit', function(e){ e.preventDefault(); dlg.close(); if (dlgAction) dlgAction(); });

      function waitForController(){
        var t0 = Date.now();
        return new Promise(function(resolve){
          setTimeout(function check(){
            fetch('/api/status', { cache: 'no-store', credentials: 'same-origin' })
              .then(function(r){ if (r.ok || r.status === 401) resolve(true); else throw 0; })   // 401 = ya contesta
              .catch(function(){ if (Date.now() - t0 > 90000) resolve(false); else setTimeout(check, 2000); });
          }, 6000);
        });
      }
      function restart(msgId){
        say(msgId, 'wait', '<span class="ec-spinner" aria-hidden="true"></span>Restarting&hellip; waiting for the controller to come back.');
        fetch('/reset', { method: 'POST', credentials: 'same-origin' }).catch(function(){});
        waitForController().then(function(back){
          say(msgId, back ? 'ok' : 'error', back ? 'Back online.' : 'The controller has not come back yet. Give it a minute, then reload this page.');
        });
      }

      document.getElementById('reset_form').addEventListener('submit', function(e){
        e.preventDefault();
        ask('Restart the controller?', 'It is back in about 30 seconds. A running process resumes where it was.', 'Restart now', true,
            function(){ restart('reset-msg'); });
      });

      // Subir config sin salir de la página (antes navegaba a una hoja que siempre decía
      // "updated successfully", aunque el archivo se hubiera rechazado).
      document.getElementById('config_form').addEventListener('submit', function(e){
        e.preventDefault();
        var form = this, btn = document.getElementById('cfg-btn');
        if (!document.getElementById('config').value) { say('cfg-msg', 'error', 'Choose a config file first.'); return; }
        btn.disabled = true;
        say('cfg-msg', 'wait', '<span class="ec-spinner" aria-hidden="true"></span>Uploading&hellip;');
        fetch('/replace-config', { method: 'POST', credentials: 'same-origin', body: new FormData(form) })
          .then(function(r){ return r.json().catch(function(){ return { ok: false, msg: 'Unexpected answer (HTTP ' + r.status + ').' }; }); })
          .then(function(res){
            if (!res.ok) return say('cfg-msg', 'error', res.msg);
            say('cfg-msg', 'ok', res.msg + ' <button type="button" class="ev-link" id="cfg-restart">Restart now</button>');
            document.getElementById('cfg-restart').addEventListener('click', function(){
              ask('Restart the controller?', 'It loads the new config.txt. If Wi-Fi or the IP address changed, it may come back at a different address.', 'Restart now', false,
                  function(){ restart('cfg-msg'); });
            });
          })
          .catch(function(){ say('cfg-msg', 'error', 'No answer from the controller. Reload the page to check whether the file was replaced.'); })
          .then(function(){ btn.disabled = false; });
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
  NAV_BLOCK("", "", "", "", "class=\"active\" aria-current=\"page\"", "")
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
          <a class="card linkcard" href="/calibration">
            <div class="card-head">
              <span class="card-icon"><svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"><path d="M14 4v10.5a4 4 0 1 1-4 0V4a2 2 0 1 1 4 0z"/><path d="M12 17.5v-6"/></svg></span>
              <h2>Sensor calibration</h2>
            </div>
            <p class="desc">Three-point calibration of Ta, Ts and Tc against a reference thermometer. <code>calibration.txt</code></p>
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
  NAV_BLOCK("", "", "", "class=\"active\" aria-current=\"page\"", "", "")
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
      const sdState = '{{SD_STATE}}';
      if (sdState === 'nosd'){
        list.innerHTML = '<div class="empty"><b>No SD card in the controller.</b><br>Insert a FAT32 card and restart to keep full logs. The last 1000 events are still available in <a href="/events">Events</a>.</div>';
      } else if (sdState === 'error'){
        list.innerHTML = '<div class="empty"><b>The logs folder on the SD card could not be opened.</b><br>The card may be damaged or badly formatted. Recent events are still available in <a href="/events">Events</a>.</div>';
      } else if (logs.length === 0){
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

const char* EVENTS_HTML = R"RAW_HTML(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <title>Thawing Room &middot; Events</title>
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <link rel="icon" href="data:," />
    <link rel="stylesheet" type="text/css" href="style.css" />
  </head>
  <body>
)RAW_HTML"
  NAV_BLOCK("", "", "class=\"active\" aria-current=\"page\"", "", "", "")
  R"RAW_HTML(
    <div class="content">
      <main class="page">
        <div class="topbar">
          <div>
            <h1>Events</h1>
            <p class="sub">Who started, moved and stopped the process, and when.</p>
          </div>
          <span class="chip" id="live-chip"><span class="dot is-idle" id="live-dot"></span><span id="live-text">Connecting&hellip;</span></span>
        </div>

        <div class="ev-banner" id="banner" role="status" hidden>
          <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><path d="M12 9v4M12 17h.01"/><path d="M10.3 3.9 1.8 18a2 2 0 0 0 1.7 3h17a2 2 0 0 0 1.7-3L13.7 3.9a2 2 0 0 0-3.4 0z"/></svg>
          <span id="banner-text">Can&rsquo;t reach the controller. Retrying every 5 seconds.</span>
        </div>

        <section class="ev-section" aria-labelledby="cycle-title">
          <div class="ev-section-head">
            <h2 id="cycle-title">Process cycle</h2>
            <label class="ev-inline-label">
              <span class="sr-only">Choose a cycle</span>
              <select id="cycle-select" class="ev-select" disabled><option>Loading&hellip;</option></select>
            </label>
          </div>
          <div class="card ev-cycle" id="cycle-card" aria-live="polite">
            <div class="ev-skel" style="width:40%"></div>
            <div class="ev-skel ev-skel--bar"></div>
            <div class="ev-skel" style="width:70%"></div>
            <div class="ev-skel" style="width:55%"></div>
          </div>
        </section>

        <section class="ev-section" aria-labelledby="list-title">
          <div class="ev-section-head">
            <h2 id="list-title">All events</h2>
            <span class="ev-updated tnum" id="updated"></span>
          </div>

          <div class="ev-toolbar">
            <div class="ev-filters" id="type-filters" role="group" aria-label="Filter by type"></div>
            <div class="ev-toolbar-row">
              <label class="ev-inline-label">
                <span class="sr-only">Origin</span>
                <select id="origin-select" class="ev-select">
                  <option value="-1">All origins</option>
                  <option value="0">Manual (panel)</option>
                  <option value="1">Remote (SCADA)</option>
                  <option value="2">Web</option>
                  <option value="3">Auto</option>
                  <option value="4">System</option>
                </select>
              </label>
              <label class="ev-inline-label">
                <span class="sr-only">Cycle</span>
                <select id="list-cycle-select" class="ev-select"><option value="all">All cycles</option></select>
              </label>
              <label class="ev-search">
                <span class="sr-only">Search messages</span>
                <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" aria-hidden="true"><circle cx="11" cy="11" r="7"/><path d="m20 20-3.5-3.5"/></svg>
                <input type="search" id="search" placeholder="Search messages" autocomplete="off" />
              </label>
              <button type="button" class="btn btn--ghost ev-btn-sm" id="export-btn">
                <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><path d="M12 4v10m0 0 4-4m-4 4-4-4"/><path d="M5 19h14"/></svg>
                Export CSV
              </button>
            </div>
          </div>

          <div class="ev-list" id="event-list">
            <div class="ev-row ev-row--skel"><div class="ev-skel"></div><div class="ev-skel"></div><div class="ev-skel"></div></div>
            <div class="ev-row ev-row--skel"><div class="ev-skel"></div><div class="ev-skel"></div><div class="ev-skel"></div></div>
            <div class="ev-row ev-row--skel"><div class="ev-skel"></div><div class="ev-skel"></div><div class="ev-skel"></div></div>
            <div class="ev-row ev-row--skel"><div class="ev-skel"></div><div class="ev-skel"></div><div class="ev-skel"></div></div>
          </div>
          <div class="btn-row ev-more" id="more-row" hidden>
            <button type="button" class="btn btn--ghost" id="more-btn">Show older events</button>
          </div>
          <p class="ev-note">The controller keeps the last 1000 events. With an SD card inserted, the full history is also written to <code>events.csv</code>, available under <a href="/logs">Logs</a>.</p>
        </section>
      </main>
      <footer class="footer">
        <span>Thawing Room &middot; {{LOCATION}}</span>
        <span>v{{VERSION}} &middot; <span class="tnum" id="clock">--:--:--</span></span>
      </footer>
    </div>

    <script>
      var TYPES = [
        { label: 'Process',   icon: '<path d="M5 12h14M13 6l6 6-6 6"/>' },
        { label: 'Parameter', icon: '<path d="M4 7h11M4 17h7"/><circle cx="18" cy="7" r="2.5"/><circle cx="14" cy="17" r="2.5"/>' },
        { label: 'Alert',     icon: '<path d="M12 9v4M12 17h.01"/><path d="M10.3 3.9 1.8 18a2 2 0 0 0 1.7 3h17a2 2 0 0 0 1.7-3L13.7 3.9a2 2 0 0 0-3.4 0z"/>' },
        { label: 'Error',     icon: '<circle cx="12" cy="12" r="9"/><path d="m15 9-6 6M9 9l6 6"/>' },
        { label: 'System',    icon: '<rect x="6" y="6" width="12" height="12" rx="2"/><path d="M9 2v3M15 2v3M9 19v3M15 19v3M2 9h3M2 15h3M19 9h3M19 15h3"/>' }
      ];
      var ORIGINS = [
        { label: 'Manual', hint: 'Button on the control panel', human: true,
          icon: '<circle cx="12" cy="12" r="8"/><circle cx="12" cy="12" r="3"/>' },
        { label: 'Remote', hint: 'SCADA (Ecava) over MQTT', human: true,
          icon: '<path d="M5 12.5a7 7 0 0 1 14 0M8.5 12.5a3.5 3.5 0 0 1 7 0"/><path d="M12 13v8"/>' },
        { label: 'Web', hint: 'This web interface', human: true,
          icon: '<circle cx="12" cy="12" r="9"/><path d="M3 12h18M12 3a14 14 0 0 1 0 18M12 3a14 14 0 0 0 0 18"/>' },
        { label: 'Auto', hint: 'Decided by the controller (schedule or temperature)', human: false,
          icon: '<path d="M20 12a8 8 0 0 1-14.3 4.9M4 12a8 8 0 0 1 14.3-4.9"/><path d="M18 3v4.5h-4.5M6 21v-4.5h4.5"/>' },
        { label: 'System', hint: 'Controller start-up or self-check', human: false,
          icon: '<path d="M12 3v9"/><path d="M6.4 6.4a8 8 0 1 0 11.2 0"/>' }
      ];
      var MON = ['Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec'];
      var DOW = ['Sun','Mon','Tue','Wed','Thu','Fri','Sat'];
      var PAGE = 250, POLL_MS = 5000, STEP = 300;
      var F_TRUSTED = 1, F_START = 2, F_END = 4, F_FINISHED = 8;

      var st = {
        events: [], head: 0, live: null, devNow: 0, devNowAt: 0,
        type: -1, origin: -1, cycle: 'all', q: '', shown: STEP,
        selCycle: null, fresh: {}, failing: false
      };

      function $(id){ return document.getElementById(id); }
      function esc(s){ return String(s).replace(/[&<>"']/g, function(c){ return {'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]; }); }
      function svg(path, size){ return '<svg width="'+(size||14)+'" height="'+(size||14)+'" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">'+path+'</svg>'; }
      function pad(n){ return (n < 10 ? '0' : '') + n; }
      // Las horas del equipo son LOCALES guardadas como epoch: se leen con getUTC* para
      // que el navegador no les aplique su propia zona horaria.
      function dt(e){ return new Date(e * 1000); }
      function hm(e){ var t = dt(e); return pad(t.getUTCHours()) + ':' + pad(t.getUTCMinutes()); }
      function hms(e){ var t = dt(e); return hm(e) + ':' + pad(t.getUTCSeconds()); }
      function dayKey(e){ var t = dt(e); return t.getUTCFullYear() * 400 + t.getUTCMonth() * 32 + t.getUTCDate(); }
      function shortDate(e){ var t = dt(e); return MON[t.getUTCMonth()] + ' ' + t.getUTCDate(); }
      function dayLabel(e){
        var t = dt(e), now = deviceNow();
        if (now && dayKey(now) === dayKey(e)) return 'Today, ' + shortDate(e);
        if (now && dayKey(now - 86400) === dayKey(e)) return 'Yesterday, ' + shortDate(e);
        return DOW[t.getUTCDay()] + ', ' + shortDate(e) + ' ' + t.getUTCFullYear();
      }
      function dur(sec){
        if (sec < 60) return '<1m';
        var h = Math.floor(sec / 3600), m = Math.floor(sec % 3600 / 60);
        return h ? h + 'h ' + pad(m) + 'm' : m + 'm';
      }
      function deviceNow(){ return st.devNow ? st.devNow + Math.round((Date.now() - st.devNowAt) / 1000) : 0; }
      function msgHTML(m){ return esc(m).replace(/ -&gt; /g, ' &rarr; ').replace(/ &gt;= /g, ' &ge; '); }
      function stageName(g){ return g >= 1 && g <= 3 ? 'Stage ' + g : g === 4 ? 'Error' : 'Idle'; }

      function typeBadge(k){
        var t = TYPES[k] || TYPES[4];
        return '<span class="ev-type ev-type--' + k + '">' + svg(t.icon, 13) + t.label + '</span>';
      }
      function originPill(o){
        var x = ORIGINS[o] || ORIGINS[4];
        return '<span class="ev-origin' + (x.human ? ' ev-origin--human' : '') + '" title="' + esc(x.hint) + '">' + svg(x.icon, 13) + x.label + '</span>';
      }
      function timeCell(ev){
        if (!ev.e) return '<span class="ev-time ev-time--warn" title="The clock was not set yet. Seconds since the controller started.">boot +' + ev.u + 's</span>';
        var warn = !(ev.f & F_TRUSTED);
        return '<time class="ev-time' + (warn ? ' ev-time--warn' : '') + '"' +
               (warn ? ' title="The controller clock was not synchronized: this time may be off."' : '') + '>' + hms(ev.e) + (warn ? '*' : '') + '</time>';
      }

      // ---------------- data ----------------
      function getJSON(url){
        return fetch(url, { cache: 'no-store', credentials: 'same-origin' }).then(function(r){
          if (!r.ok) throw new Error('HTTP ' + r.status);
          return r.json();
        });
      }
      function takeHeader(r){
        st.head = r.head;
        st.live = { stage: r.stage, cycle: r.cycle, active: r.active };
        if (r.now) { st.devNow = r.now; st.devNowAt = Date.now(); }
      }
      function loadAll(){
        var all = [];
        function page(before){
          return getJSON('/api/events?limit=' + PAGE + (before ? '&before=' + before : '')).then(function(r){
            if (!before) takeHeader(r);
            all = all.concat(r.events);
            var last = r.events[r.events.length - 1];
            if (r.events.length === PAGE && last && last.s > 1) return page(last.s);
            return all;
          });
        }
        return page(0).then(function(list){ st.events = list; st.fresh = {}; });
      }
      function poll(){
        if (document.hidden) return;
        getJSON('/api/events?limit=' + PAGE + '&after=' + st.head).then(function(r){
          // Secuencia más chica que la conocida: el anillo se reinició. Recargar todo.
          if (r.head < st.head || r.events.length === PAGE) return loadAll().then(function(){ ok(true); });
          var before = JSON.stringify(st.live);
          takeHeader(r);
          if (r.events.length) {
            r.events.forEach(function(ev){ st.fresh[ev.s] = true; });
            st.events = r.events.concat(st.events);
            setTimeout(function(){ st.fresh = {}; }, 2000);
          }
          // Redibujar solo si algo cambió: rehacer los <select> cada 5 s cerraría el que
          // el usuario tenga abierto.
          ok(st.failing || r.events.length > 0 || JSON.stringify(st.live) !== before);
        }).catch(fail);
      }
      function ok(changed){
        st.failing = false;
        $('banner').hidden = true;
        $('updated').textContent = 'Updated ' + new Date().toLocaleTimeString();
        if (changed) render();
      }
      function fail(){
        st.failing = true;
        $('banner').hidden = false;
        $('live-dot').className = 'dot is-idle';
        $('live-text').textContent = 'Offline';
      }

      // ---------------- cycles ----------------
      function buildCycles(){
        var map = {}, order = [];
        for (var i = st.events.length - 1; i >= 0; i--) {   // más viejo primero
          var ev = st.events[i];
          if (!ev.c) continue;
          if (!map[ev.c]) { map[ev.c] = { id: ev.c, events: [] }; order.push(ev.c); }
          map[ev.c].events.push(ev);
        }
        return order.map(function(id){
          var c = map[id];
          c.steps = c.events.filter(function(e){ return e.k === 0; });
          c.first = c.steps[0] || c.events[0];
          c.end = null;
          c.steps.forEach(function(e){ if (e.f & F_END) c.end = e; });
          c.partial = !(c.first.f & F_START);
          c.issues = c.events.filter(function(e){ return e.k === 2 || e.k === 3; }).length;
          if (c.end) c.status = (c.end.f & F_FINISHED) ? 'finished' : 'stopped';
          else c.status = (st.live && st.live.active && st.live.cycle === c.id) ? 'running' : 'interrupted';
          var lastE = 0;
          c.events.forEach(function(e){ if (e.e) lastE = e.e; });
          c.t0 = c.first.e;
          c.t1 = c.end ? c.end.e : c.status === 'running' ? (deviceNow() || lastE) : lastE;
          return c;
        }).reverse();
      }
      function segments(c){
        var segs = [];
        for (var i = 0; i < c.steps.length; i++) {
          var s = c.steps[i], next = c.steps[i + 1];
          if (!s.e || s.g < 1 || s.g > 3) continue;
          var t1 = next && next.e ? next.e : c.t1;
          if (t1 <= s.e) continue;
          var last = segs[segs.length - 1];
          if (last && last.g === s.g) last.t1 = t1;
          else segs.push({ g: s.g, t0: s.e, t1: t1 });
        }
        return segs;
      }
      var STATUS = {
        running:     'Running',
        finished:    'Finished',
        stopped:     'Stopped early',
        interrupted: 'No end recorded'
      };
      function cycleOption(c){
        return 'Cycle ' + c.id + (c.t0 ? ' · ' + shortDate(c.t0) + ', ' + hm(c.t0) : '') + ' · ' + STATUS[c.status];
      }

      function renderCycle(cycles){
        var sel = $('cycle-select'), listSel = $('list-cycle-select'), card = $('cycle-card');
        if (!cycles.length) {
          sel.innerHTML = '<option>No cycles yet</option>'; sel.disabled = true;
          listSel.innerHTML = '<option value="all">All cycles</option>';
          card.innerHTML = '<div class="empty">No process cycles recorded yet. A cycle starts when someone presses <b>Start</b> or <b>Delayed start</b>, on the panel or from SCADA.</div>';
          return;
        }
        if (st.selCycle === null || !cycles.some(function(c){ return c.id === st.selCycle; })) st.selCycle = cycles[0].id;
        sel.disabled = false;
        sel.innerHTML = cycles.map(function(c){ return '<option value="' + c.id + '"' + (c.id === st.selCycle ? ' selected' : '') + '>' + esc(cycleOption(c)) + '</option>'; }).join('');
        listSel.innerHTML = '<option value="all">All cycles</option>' + cycles.map(function(c){
          return '<option value="' + c.id + '"' + (String(c.id) === String(st.cycle) ? ' selected' : '') + '>Cycle ' + c.id + '</option>';
        }).join('');

        var c = cycles.filter(function(x){ return x.id === st.selCycle; })[0];
        var segs = segments(c), total = c.t0 && c.t1 ? c.t1 - c.t0 : 0;
        var html = '<div class="ev-cycle-head">' +
          '<span class="ev-status ev-status--' + c.status + '"><span class="ev-status-dot"></span>' + STATUS[c.status] + '</span>' +
          (total ? '<span class="ev-cycle-total tnum">' + dur(total) + (c.status === 'running' ? ' so far' : ' total') + '</span>' : '') +
          (c.issues ? '<button type="button" class="ev-link" data-issues="' + c.id + '">' + c.issues + (c.issues === 1 ? ' alert or error' : ' alerts or errors') + ' in this cycle</button>' : '') +
          '</div>';

        if (segs.length) {
          var span = segs[segs.length - 1].t1 - segs[0].t0 || 1;
          html += '<div class="ev-bar" role="img" aria-label="' + esc(segs.map(function(s){ return stageName(s.g) + ' ' + dur(s.t1 - s.t0); }).join(', ')) + '">' +
            segs.map(function(s){
              var frac = (s.t1 - s.t0) / span;
              return '<div class="ev-bar-seg ev-bar-seg--' + s.g + '" style="flex-grow:' + Math.max(frac, 0.004).toFixed(4) + '" title="' + stageName(s.g) + ': ' + hm(s.t0) + ' – ' + hm(s.t1) + ' (' + dur(s.t1 - s.t0) + ')">' +
                (frac > 0.14 ? '<span>' + stageName(s.g) + '</span>' : '') + '</div>';
            }).join('') + '</div>' +
            '<div class="ev-bar-scale tnum"><span>' + hm(segs[0].t0) + '</span><span>' + (c.status === 'running' ? 'now' : hm(segs[segs.length - 1].t1)) + '</span></div>' +
            '<div class="ev-legend">' + [1, 2, 3].map(function(g){
              var secs = 0; segs.forEach(function(s){ if (s.g === g) secs += s.t1 - s.t0; });
              return '<span class="ev-legend-item"><i class="ev-swatch ev-bar-seg--' + g + '"></i>Stage ' + g + ' <b class="tnum">' + (secs ? dur(secs) : '—') + '</b></span>';
            }).join('') + '</div>';
        }

        if (c.partial) html += '<p class="ev-note">The start of this cycle is older than the last 1000 events and is no longer stored on the controller.</p>';

        var startDay = c.t0 ? dayKey(c.t0) : 0;
        html += '<ol class="ev-steps">' + c.steps.map(function(s){
          var when = s.e ? (dayKey(s.e) !== startDay ? shortDate(s.e) + ' ' : '') + hm(s.e) : 'boot +' + s.u + 's';
          return '<li class="ev-step">' +
            '<span class="ev-step-time tnum">' + when + '</span>' +
            '<span class="ev-step-text">' + msgHTML(s.m) + '</span>' +
            originPill(s.o) +
            '<span class="ev-step-elapsed tnum">' + (s.e && c.t0 && s !== c.first ? '+' + dur(s.e - c.t0) : '') + '</span>' +
            '</li>';
        }).join('') + '</ol>';

        if (c.status === 'interrupted') {
          html += '<p class="ev-note">No stop or finish was recorded for this cycle. The controller may have lost power, or the record was dropped.</p>';
        }
        card.innerHTML = html;
      }

      // ---------------- list ----------------
      function matches(ev, ignoreType){
        if (!ignoreType && st.type >= 0 && ev.k !== st.type) return false;
        if (st.origin >= 0 && ev.o !== st.origin) return false;
        if (st.cycle !== 'all' && String(ev.c) !== String(st.cycle)) return false;
        if (st.q && ev.m.toLowerCase().indexOf(st.q) < 0) return false;
        return true;
      }
      function renderFilters(){
        var counts = [0, 0, 0, 0, 0], all = 0;
        st.events.forEach(function(ev){ if (matches(ev, true)) { all++; counts[ev.k]++; } });
        var btn = function(k, label, n, icon){
          return '<button type="button" class="ev-filter' + (k >= 0 ? ' ev-filter--' + k : '') + '" data-type="' + k + '" aria-pressed="' + (st.type === k) + '">' +
            (icon ? svg(icon, 13) : '') + label + '<span class="n">' + n + '</span></button>';
        };
        $('type-filters').innerHTML = btn(-1, 'All', all) + TYPES.map(function(t, k){ return btn(k, t.label, counts[k], t.icon); }).join('');
      }
      function renderList(){
        var list = $('event-list');
        var rows = st.events.filter(function(ev){ return matches(ev); });
        if (!st.events.length) {
          list.innerHTML = '<div class="empty">No events yet. Starting a process, changing a set-point or restarting the controller will show up here.</div>';
          $('more-row').hidden = true;
          return;
        }
        if (!rows.length) {
          list.innerHTML = '<div class="empty">No events match these filters. <button type="button" class="ev-link" id="clear-filters">Clear filters</button></div>';
          $('more-row').hidden = true;
          return;
        }
        var html = '', lastDay = null, shown = rows.slice(0, st.shown);
        shown.forEach(function(ev){
          var day = ev.e ? dayKey(ev.e) : 'none';
          if (day !== lastDay) {
            html += '<div class="ev-day">' + (ev.e ? dayLabel(ev.e) : 'Before the clock was set') + '</div>';
            lastDay = day;
          }
          html += '<div class="ev-row' + (st.fresh[ev.s] ? ' is-new' : '') + '">' +
            timeCell(ev) + typeBadge(ev.k) +
            '<span class="ev-msg">' + msgHTML(ev.m) + (ev.c ? '<span class="ev-meta">Cycle ' + ev.c + '</span>' : '') + '</span>' +
            originPill(ev.o) + '</div>';
        });
        list.innerHTML = html;
        $('more-row').hidden = rows.length <= st.shown;
      }
      function renderLive(){
        if (st.failing || !st.live) return;
        var l = st.live, running = l.active && l.stage >= 1 && l.stage <= 3;
        $('live-dot').className = 'dot' + (running ? '' : ' is-idle');
        $('live-text').innerHTML = running ? '<b>' + stageName(l.stage) + '</b>&nbsp;&middot; Cycle ' + l.cycle : 'Idle';
      }
      function render(){
        renderLive();
        renderCycle(buildCycles());
        renderFilters();
        renderList();
      }

      // ---------------- export ----------------
      function exportCSV(){
        var rows = st.events.filter(function(ev){ return matches(ev); });
        var q = function(s){ return '"' + String(s).replace(/"/g, '""') + '"'; };
        var csv = ['seq,date,time,time_trusted,type,origin,cycle,stage,message'].concat(rows.map(function(ev){
          var t = ev.e ? dt(ev.e) : null;
          return [ev.s, t ? t.getUTCFullYear() + '-' + pad(t.getUTCMonth() + 1) + '-' + pad(t.getUTCDate()) : '',
                  t ? hms(ev.e) : 'boot+' + ev.u + 's', (ev.f & F_TRUSTED) ? 1 : 0,
                  TYPES[ev.k].label, ORIGINS[ev.o].label, ev.c || '', ev.g, q(ev.m)].join(',');
        })).join('\r\n');
        var a = document.createElement('a');
        a.href = URL.createObjectURL(new Blob([csv], { type: 'text/csv' }));
        a.download = 'events-{{LOCATION}}.csv';
        document.body.appendChild(a); a.click(); a.remove();
        setTimeout(function(){ URL.revokeObjectURL(a.href); }, 1000);
      }

      // ---------------- wiring ----------------
      $('type-filters').addEventListener('click', function(e){
        var b = e.target.closest('[data-type]'); if (!b) return;
        st.type = +b.getAttribute('data-type'); st.shown = STEP; renderFilters(); renderList();
      });
      $('origin-select').addEventListener('change', function(){ st.origin = +this.value; st.shown = STEP; renderFilters(); renderList(); });
      $('list-cycle-select').addEventListener('change', function(){ st.cycle = this.value; st.shown = STEP; renderFilters(); renderList(); });
      var qTimer;
      $('search').addEventListener('input', function(){
        var v = this.value;
        clearTimeout(qTimer);
        qTimer = setTimeout(function(){ st.q = v.trim().toLowerCase(); st.shown = STEP; renderFilters(); renderList(); }, 150);
      });
      $('cycle-select').addEventListener('change', function(){ st.selCycle = +this.value; renderCycle(buildCycles()); });
      $('more-btn').addEventListener('click', function(){ st.shown += STEP; renderList(); });
      $('export-btn').addEventListener('click', exportCSV);
      document.addEventListener('click', function(e){
        var issues = e.target.closest('[data-issues]');
        if (issues) {
          st.cycle = issues.getAttribute('data-issues'); st.type = -1; st.origin = -1;
          $('origin-select').value = '-1';
          render();
          $('list-title').scrollIntoView({ behavior: matchMedia('(prefers-reduced-motion: reduce)').matches ? 'auto' : 'smooth', block: 'start' });
        }
        if (e.target.id === 'clear-filters') {
          st.type = -1; st.origin = -1; st.cycle = 'all'; st.q = '';
          $('origin-select').value = '-1'; $('search').value = '';
          render();
        }
      });
      document.addEventListener('visibilitychange', function(){ if (!document.hidden) poll(); });

      function start(){
        loadAll().then(function(){ ok(true); setInterval(poll, POLL_MS); })
          .catch(function(){ fail(); setTimeout(start, POLL_MS); });
      }
      start();

      function tick(){ $('clock').textContent = new Date().toLocaleTimeString(); }
      setInterval(tick, 1000); tick();
    </script>
  </body>
</html>
)RAW_HTML";

const char* CONTROL_HTML = R"RAW_HTML(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <title>Thawing Room &middot; Control</title>
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <link rel="icon" href="data:," />
    <link rel="stylesheet" type="text/css" href="style.css" />
  </head>
  <body>
)RAW_HTML"
  NAV_BLOCK("class=\"active\" aria-current=\"page\"", "", "", "", "", "")
  R"RAW_HTML(
    <div class="content">
      <main class="page">
        <div class="topbar">
          <div>
            <h1>Control</h1>
            <p class="sub">Start, stop and watch the room in real time.</p>
          </div>
          <span class="chip" id="live-chip"><span class="dot is-idle" id="live-dot"></span><span id="live-text">Connecting&hellip;</span></span>
        </div>

        <div class="ev-banner" id="banner" role="status" hidden>
          <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><path d="M12 9v4M12 17h.01"/><path d="M10.3 3.9 1.8 18a2 2 0 0 0 1.7 3h17a2 2 0 0 0 1.7-3L13.7 3.9a2 2 0 0 0-3.4 0z"/></svg>
          <span>Can&rsquo;t reach the controller. The values below may be out of date. Retrying&hellip;</span>
        </div>
        <div class="ct-notices" id="notices"></div>

        <section class="ct-lock" id="lock-banner" aria-labelledby="lock-title" hidden>
          <span class="ct-lock-icon"><svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.9" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><path d="M14.7 6.3a1 1 0 0 0 0 1.4l1.6 1.6a1 1 0 0 0 1.4 0l3.8-3.8a6 6 0 0 1-7.9 7.9l-6.9 6.9a2.1 2.1 0 0 1-3-3l6.9-6.9a6 6 0 0 1 7.9-7.9z"/></svg></span>
          <div class="ct-lock-text">
            <h2 id="lock-title">Maintenance lockdown</h2>
            <p>Starting is blocked from the panel buttons, the SCADA and this page. The three stage lights on the panel are blinking.</p>
            <p class="ct-lock-meta" id="lock-meta"></p>
          </div>
          <button type="button" class="btn btn--ghost" id="btn-unlock">Lift lockdown</button>
        </section>

        <section class="card ct-hero" aria-labelledby="stage-name">
          <div>
            <h2 class="ct-stage" id="stage-name"><span class="ev-skel" style="display:block;width:220px;height:26px"></span></h2>
            <p class="ct-stage-detail" id="stage-detail">&nbsp;</p>
          </div>
          <ol class="ct-track" id="track" aria-label="Process stages">
            <li data-s="1"><span class="ct-track-bar"></span><b>Stage 1</b><span>Delayed start</span></li>
            <li data-s="2"><span class="ct-track-bar"></span><b>Stage 2</b><span>Active thaw</span></li>
            <li data-s="3"><span class="ct-track-bar"></span><b>Stage 3</b><span>Hold</span></li>
          </ol>
          <div class="ct-next" id="next"></div>
          <div class="ct-actions">
            <button type="button" class="btn" id="btn-start" disabled>
              <svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor" aria-hidden="true"><path d="M8 5.5v13a1 1 0 0 0 1.5.9l10.4-6.5a1 1 0 0 0 0-1.8L9.5 4.6A1 1 0 0 0 8 5.5z"/></svg>
              Start now
            </button>
            <button type="button" class="btn btn--ghost" id="btn-dstart" disabled>
              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><circle cx="12" cy="12" r="9"/><path d="M12 7v5l3 2"/></svg>
              Delayed start
            </button>
            <button type="button" class="btn btn--danger" id="btn-stop" disabled>
              <svg width="15" height="15" viewBox="0 0 24 24" fill="currentColor" aria-hidden="true"><rect x="5" y="5" width="14" height="14" rx="2"/></svg>
              <span id="stop-label">Stop</span>
            </button>
          </div>
          <p class="ct-hint" id="action-hint"></p>
        </section>

        <div class="ct-grid">
          <section class="card ct-panel" aria-labelledby="temps-title">
            <div class="ct-panel-head"><h2 id="temps-title">Temperatures</h2><span class="ct-muted">&deg;C</span></div>
            <div id="temps"><div class="ev-skel" style="height:44px"></div><div class="ev-skel" style="height:44px;margin-top:14px"></div><div class="ev-skel" style="height:44px;margin-top:14px"></div></div>
          </section>
          <section class="card ct-panel" aria-labelledby="eq-title">
            <div class="ct-panel-head"><h2 id="eq-title">Equipment</h2></div>
            <ul class="ct-eq" id="equipment"><li class="ev-skel" style="height:40px"></li><li class="ev-skel" style="height:40px;margin-top:14px"></li><li class="ev-skel" style="height:40px;margin-top:14px"></li></ul>
          </section>
        </div>

        <section class="card ct-panel ct-sys" aria-labelledby="sys-title">
          <h2 id="sys-title" class="sr-only">Controller</h2>
          <dl class="ct-kv" id="system"></dl>
          <div class="ct-maint">
            <div>
              <b>Maintenance lockdown</b>
              <span id="maint-text">Blocks every start while someone works on the room.</span>
            </div>
            <button type="button" class="btn btn--ghost ev-btn-sm" id="btn-lock" disabled>
              <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><rect x="5" y="11" width="14" height="10" rx="2"/><path d="M8 11V7a4 4 0 0 1 8 0v4"/></svg>
              <span id="lock-label">Enable lockdown</span>
            </button>
          </div>
        </section>

        <section class="ev-section" aria-labelledby="recent-title">
          <div class="ev-section-head">
            <h2 id="recent-title">Recent activity</h2>
            <a class="ev-link" href="/events">All events &rarr;</a>
          </div>
          <div class="ev-list" id="recent">
            <div class="ev-row ev-row--skel"><div class="ev-skel"></div><div class="ev-skel"></div><div class="ev-skel"></div></div>
            <div class="ev-row ev-row--skel"><div class="ev-skel"></div><div class="ev-skel"></div><div class="ev-skel"></div></div>
            <div class="ev-row ev-row--skel"><div class="ev-skel"></div><div class="ev-skel"></div><div class="ev-skel"></div></div>
          </div>
        </section>
      </main>
      <footer class="footer">
        <span>Thawing Room &middot; {{LOCATION}}</span>
        <span>v{{VERSION}} &middot; <span title="Time on the controller, which can differ from this device">Room time</span> <span class="tnum" id="clock">--:--:--</span></span>
      </footer>
    </div>

    <dialog class="ct-dialog" id="dlg" aria-labelledby="dlg-title" aria-describedby="dlg-body">
      <form method="dialog" id="dlg-form" novalidate>
        <h2 id="dlg-title"></h2>
        <p class="ct-dialog-body" id="dlg-body"></p>
        <div class="ct-dfields" id="dlg-fields" hidden>
          <div class="field"><label for="ds-date">Stage 2 date</label><input type="date" id="ds-date" /></div>
          <div class="field"><label for="ds-time">Stage 2 time</label><input type="time" id="ds-time" /></div>
          <p class="ct-preview tnum" id="ds-preview" aria-live="polite"></p>
        </div>
        <p class="ct-dialog-error" id="dlg-error" role="alert" hidden></p>
        <div class="ct-dialog-actions">
          <button type="button" class="btn btn--ghost" id="dlg-cancel">Cancel</button>
          <button type="submit" class="btn" id="dlg-ok">Confirm</button>
        </div>
      </form>
    </dialog>
    <div class="ct-toast" id="toast" role="status" aria-live="polite" hidden></div>

    <script>
      var STAGES = [
        { name: 'Idle',    sub: 'Ready to start' },
        { name: 'Stage 1', sub: 'Delayed start' },
        { name: 'Stage 2', sub: 'Active thaw' },
        { name: 'Stage 3', sub: 'Hold' },
        { name: 'Error',   sub: 'Stopping after a fault' }
      ];
      var STEP_NAMES = ['', 'forward run', 'pause', 'reverse run', 'pause'];
      var CLOCK_SRC = ['Not set', 'DS3231 clock', 'Internal clock', 'Estimated since last sync'];
      var TYPES = [
        { label: 'Process',   icon: '<path d="M5 12h14M13 6l6 6-6 6"/>' },
        { label: 'Parameter', icon: '<path d="M4 7h11M4 17h7"/><circle cx="18" cy="7" r="2.5"/><circle cx="14" cy="17" r="2.5"/>' },
        { label: 'Alert',     icon: '<path d="M12 9v4M12 17h.01"/><path d="M10.3 3.9 1.8 18a2 2 0 0 0 1.7 3h17a2 2 0 0 0 1.7-3L13.7 3.9a2 2 0 0 0-3.4 0z"/>' },
        { label: 'Error',     icon: '<circle cx="12" cy="12" r="9"/><path d="m15 9-6 6M9 9l6 6"/>' },
        { label: 'System',    icon: '<rect x="6" y="6" width="12" height="12" rx="2"/><path d="M9 2v3M15 2v3M9 19v3M15 19v3M2 9h3M2 15h3M19 9h3M19 15h3"/>' }
      ];
      var ORIGINS = [
        { label: 'Manual', hint: 'Button on the control panel', human: true },
        { label: 'Remote', hint: 'SCADA (Ecava) over MQTT', human: true },
        { label: 'Web',    hint: 'This web interface', human: true },
        { label: 'Auto',   hint: 'Decided by the controller', human: false },
        { label: 'System', hint: 'Controller start-up or self-check', human: false }
      ];
      var ICONS = {
        fan:    '<circle cx="12" cy="12" r="2"/><path d="M12 10c-.5-3.5.5-7 3.5-7 2.5 0 2.5 3.5-1.5 7M14 12c3.5-.5 7 .5 7 3.5 0 2.5-3.5 2.5-7-1.5M12 14c.5 3.5-.5 7-3.5 7-2.5 0-2.5-3.5 1.5-7M10 12c-3.5.5-7-.5-7-3.5 0-2.5 3.5-2.5 7 1.5"/>',
        air:    '<path d="M3 8h10a3 3 0 1 0-3-3M3 12h15a3 3 0 1 1-3 3M3 16h7"/>',
        damper: '<rect x="3" y="4" width="18" height="16" rx="2"/><path d="M7 9h10M7 15h10"/>',
        water:  '<path d="M12 3s6 6.5 6 11a6 6 0 0 1-12 0c0-4.5 6-11 6-11z"/>',
        warn:   '<path d="M12 9v4M12 17h.01"/><path d="M10.3 3.9 1.8 18a2 2 0 0 0 1.7 3h17a2 2 0 0 0 1.7-3L13.7 3.9a2 2 0 0 0-3.4 0z"/>',
        check:  '<path d="m5 12 5 5 9-10"/>',
        clock:  '<circle cx="12" cy="12" r="9"/><path d="M12 7v5l3 2"/>',
        cross:  '<path d="m6 6 12 12M18 6 6 18"/>'
      };
      var MON = ['Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec'];
      var DOW = ['Sun','Mon','Tue','Wed','Thu','Fri','Sat'];
      var POLL_MS = 2000, RECENT_MS = 10000, CONFIRM_MS = 8000, MAX_AHEAD_S = 7 * 86400;

      var st = { s: null, devNow: 0, devNowAt: 0, failing: false, action: null, sending: false, recentKey: '' };

      function $(id){ return document.getElementById(id); }
      function esc(s){ return String(s).replace(/[&<>"']/g, function(c){ return {'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]; }); }
      function svg(path, size, cls){ return '<svg' + (cls ? ' class="' + cls + '"' : '') + ' width="'+(size||16)+'" height="'+(size||16)+'" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.9" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">'+path+'</svg>'; }
      function pad(n){ return (n < 10 ? '0' : '') + n; }
      // Las horas del equipo son LOCALES guardadas como epoch: se leen con getUTC* para
      // que el navegador no les aplique su propia zona horaria (planta y oficina difieren).
      function dt(e){ return new Date(e * 1000); }
      function hm(e){ var t = dt(e); return pad(t.getUTCHours()) + ':' + pad(t.getUTCMinutes()); }
      function sameDay(a, b){ var x = dt(a), y = dt(b); return x.getUTCFullYear() === y.getUTCFullYear() && x.getUTCMonth() === y.getUTCMonth() && x.getUTCDate() === y.getUTCDate(); }
      function when(e){
        var now = deviceNow(), t = dt(e);
        if (!now || sameDay(e, now)) return hm(e);
        if (sameDay(e, now - 86400)) return 'yesterday ' + hm(e);
        if (sameDay(e, now + 86400)) return 'tomorrow ' + hm(e);
        return DOW[t.getUTCDay()] + ' ' + MON[t.getUTCMonth()] + ' ' + t.getUTCDate() + ', ' + hm(e);
      }
      function dur(sec){
        if (sec < 60) return 'under a minute';
        var d = Math.floor(sec / 86400), h = Math.floor(sec % 86400 / 3600), m = Math.floor(sec % 3600 / 60);
        if (d) return d + 'd ' + h + 'h';
        return h ? h + 'h ' + pad(m) + 'm' : m + 'm';
      }
      function deviceNow(){ return st.devNow ? st.devNow + Math.round((Date.now() - st.devNowAt) / 1000) : 0; }
      function num(v){ return v === null || v === undefined ? '&ndash;' : v.toFixed(1); }
      function getJSON(url){
        return fetch(url, { cache: 'no-store', credentials: 'same-origin' }).then(function(r){
          if (!r.ok) throw new Error('HTTP ' + r.status);
          return r.json();
        });
      }

      // ---------------- status ----------------
      function take(s){
        st.s = s;
        if (s.now) { st.devNow = s.now; st.devNowAt = Date.now(); }
      }
      function poll(){
        if (document.hidden && st.s) return;
        getJSON('/api/status').then(function(s){
          take(s);
          st.failing = false;
          $('banner').hidden = true;
          render();
        }).catch(function(){
          st.failing = true;
          $('banner').hidden = false;
          renderHeader();
          renderActions();
        });
      }

      function render(){
        if (!st.s || !st.s.ready) return;
        renderHeader(); renderLock(); renderHero(); renderActions(); renderTemps(); renderEquipment(); renderSystem(); renderNotices();
      }

      function renderHeader(){
        var s = st.s;
        if (st.failing || !s) {
          $('live-dot').className = 'dot is-idle';
          $('live-text').textContent = st.failing ? 'Offline' : 'Connecting…';
          return;
        }
        var running = s.stage >= 1 && s.stage <= 3;
        if (s.lock.on) {
          $('live-dot').className = 'dot is-lock';
          $('live-text').innerHTML = '<b>Lockdown</b>' + (running ? '&nbsp;&middot; ' + STAGES[s.stage].name : '');
          return;
        }
        $('live-dot').className = 'dot' + (running ? '' : ' is-idle');
        $('live-text').innerHTML = running ? '<b>' + STAGES[s.stage].name + '</b>&nbsp;&middot; Cycle ' + s.cycle : esc(STAGES[s.stage] ? STAGES[s.stage].name : 'Unknown');
      }

      function renderLock(){
        var l = st.s.lock, who = ['the panel', 'the SCADA', 'the web interface', 'the controller', 'the controller'][l.src] || 'the controller';
        $('lock-banner').hidden = !l.on;
        if (l.on) {
          $('lock-meta').innerHTML = 'Enabled by <b>' + who + '</b>' +
            (l.since && deviceNow() ? ' at ' + when(l.since) + ' &middot; ' + dur(deviceNow() - l.since) + ' ago' : '') +
            (st.s.stage >= 1 && st.s.stage <= 3 ? '. <b>' + STAGES[st.s.stage].name + ' is still running</b>: stop it if the work requires it.' : '.');
        }
        $('maint-text').innerHTML = l.on ? '<b class="ct-maint-on">Active.</b> Every start is blocked.' : 'Blocks every start while someone works on the room.';
        $('lock-label').textContent = l.on ? 'Lift lockdown' : 'Enable lockdown';
      }

      function renderHero(){
        var s = st.s, g = s.stage, S = STAGES[g] || STAGES[4], now = deviceNow();
        $('stage-name').innerHTML = esc(S.name) + ' <span>' + esc(S.sub) + '</span>';

        var detail = '';
        if (g === 0) detail = s.lock.on ? 'Nothing is running. Starting is blocked during maintenance.' : 'Nothing is running. Start now, or schedule Stage 2 for later.';
        else if (s.since && now) {
          detail = (s.since_boot ? 'Resumed after a controller restart at ' : 'In this stage since ') + when(s.since) + ' &middot; ' + dur(now - s.since);
        }
        $('stage-detail').innerHTML = detail || '&nbsp;';

        var items = $('track').children;
        for (var i = 0; i < items.length; i++) {
          var n = +items[i].getAttribute('data-s');
          items[i].className = n === g ? 'is-current' : (g >= 1 && g <= 3 && n < g ? 'is-past' : '');
          if (n === g) items[i].setAttribute('aria-current', 'step'); else items[i].removeAttribute('aria-current');
        }

        var next = '';
        if (g === 1) {
          if (s.s2.set) {
            var at = pad(s.s2.h) + ':' + pad(s.s2.m) + ', ' + MON[(s.s2.mo || 1) - 1] + ' ' + s.s2.d;
            next = s.s2.left > 0
              ? svg(ICONS.clock, 16) + '<span>Stage 2 starts at <b class="tnum">' + at + '</b> &middot; in ' + dur(s.s2.left * 60) + '</span>'
              : '<span>Stage 2 is due now (' + at + ').</span>';
          } else {
            next = '<span>Calculating when Stage 2 starts&hellip;</span>';
          }
        } else if (g === 2) {
          var t = s.temps;
          next = '<span>Moves to Stage 3 when both are reached:</span>' + cond('Ts', t.ts, t.ts_set) + cond('Tc', t.tc, t.tc_set);
        } else if (g === 3) {
          next = '<span>Holds until Stop is pressed. Stopping now records the cycle as <b>finished</b>.</span>';
        } else if (g === 4) {
          next = '<span>The controller hit an error and is stopping the process.</span>';
        }
        $('next').innerHTML = next;
      }
      function cond(name, v, target){
        var met = v !== null && target !== null && v >= target;
        return '<span class="ct-cond' + (met ? ' is-met' : '') + '">' + svg(met ? ICONS.check : ICONS.cross, 13) +
               name + ' <span class="tnum">' + num(v) + '</span>' + (met ? ' &ge; ' : ' / ') + '<span class="tnum">' + num(target) + '</span></span>';
      }

      function renderActions(){
        var s = st.s, g = s ? s.stage : -1;
        var off = st.failing || !s || st.sending || (s && s.busy);
        var locked = !!(s && s.lock.on);
        $('btn-start').disabled = off || g !== 0 || locked;
        $('btn-dstart').disabled = off || g !== 0 || locked;
        $('btn-lock').disabled = off;
        $('btn-unlock').disabled = off;
        $('btn-stop').disabled = off || g === 0 || g === -1;
        $('stop-label').textContent = g === 3 ? 'Finish process' : 'Stop';
        var hint = '';
        if (st.failing) hint = 'Controls are disabled while the controller is unreachable.';
        else if (s && s.busy) hint = 'A command is being processed…';
        else if (locked) hint = 'Starting is blocked by the maintenance lockdown.';
        else if (g >= 1) hint = 'Start and delayed start are available once the process is stopped.';
        $('action-hint').textContent = hint;
      }

      var PROBES = [
        { k: 'ta', a: 'ata', f: 'fa', c: 'ca', name: 'Ta', label: 'Room air' },
        { k: 'ts', a: 'ats', f: 'fs', c: 'cs', name: 'Ts', label: 'Product surface', set: 'ts_set' },
        { k: 'tc', a: 'atc', f: 'fc', c: 'cc', name: 'Tc', label: 'Product core', set: 'tc_set' }
      ];
      function renderTemps(){
        var t = st.s.temps, g = st.s.stage, html = '';
        PROBES.forEach(function(p){
          var fault = !!t[p.f], meta = [];
          meta.push('avg <span class="tnum">' + num(t[p.a]) + '</span>');
          if (p.set) meta.push('Stage 3 at &ge; <span class="tnum">' + num(t[p.set]) + '</span>');
          if (p.k === 'ta' && g === 2) meta.push('PID setpoint <span class="tnum">' + num(t.sp) + '</span>');
          if (t[p.c] !== 65535) meta.push('<span title="Raw ADC counts. A healthy 4-20 mA loop reads 819 to 4095.">ADC <span class="tnum">' + t[p.c] + '</span></span>');
          html += '<div class="ct-temp' + (fault ? ' is-fault' : '') + '">' +
            '<div class="ct-temp-name"><b>' + p.name + '</b><span>' + p.label + '</span></div>' +
            '<div class="ct-temp-val tnum">' + num(t[p.k]) + '<small>&deg;C</small></div>' +
            '<div class="ct-temp-meta">' + (fault ? '<span class="ct-badge">' + svg(ICONS.warn, 13) + 'Sensor fault, showing last good value</span>' : '') + meta.join('<span aria-hidden="true">&middot;</span>') + '</div>' +
          '</div>';
        });
        $('temps').innerHTML = html;
      }

      // on: true, false o 'warn' (encendido, pero no por el controlador)
      function eqItem(icon, title, detail, state, on, extra){
        return '<li class="ct-eq-item' + (on ? ' is-on' : '') + '">' +
          '<span class="ct-eq-icon">' + icon + '</span>' +
          '<div class="ct-eq-text"><b>' + title + '</b><span>' + detail + '</span>' + (extra || '') + '</div>' +
          '<span class="ct-state' + (on === 'warn' ? ' is-warn' : on ? ' is-on' : '') + '">' + state + '</span></li>';
      }
      function renderEquipment(){
        var s = st.s, o = s.out, g = s.stage, html = '';
        var fanDetail = 'Idle';
        if (g >= 1 && g <= 3) fanDetail = s.starting ? 'Starting after the 5 s safety delay' : (s.step ? 'Step ' + s.step + ' of 4, ' + STEP_NAMES[s.step] : 'Starting');
        if (o.ov) {
          // El selector del tablero manda: el relé del controlador no dice si los fans giran.
          html += eqItem(svg(ICONS.fan, 18, 'ct-spin'), 'Fan F1', 'Switched on by hand at the panel', 'Override', 'warn');
        } else {
          html += eqItem(svg(ICONS.fan, 18, o.fan ? 'ct-spin' + (o.rev ? ' ct-spin--rev' : '') : ''), 'Fan F1', fanDetail,
                         o.fan ? (o.rev ? 'Reverse' : 'Forward') : 'Off', o.fan);
        }

        var max = o.air_max || 63, pct = Math.round(Math.min(o.air, max) / max * 100);
        var airDetail = o.air > max
          ? '<span title="The PID asks for ' + o.air + ' but the output tops out at ' + max + '.">PID output saturated</span>'
          : 'PWM duty <span class="tnum">' + o.air + '</span> of ' + max;
        html += eqItem(svg(ICONS.air, 18), 'Air infeed', airDetail, '<span class="tnum">' + pct + '%</span>', o.air > 0,
                       '<span class="ct-meter" aria-hidden="true"><i style="transform:scaleX(' + (pct / 100) + ')"></i></span>');
        html += eqItem(svg(ICONS.damper, 18), 'Air damper', 'Output D2', o.damper ? 'Open' : 'Closed', o.damper);
        html += eqItem(svg(ICONS.water, 18), 'Sprinkler S1', 'Flushes the carts', o.s1 ? 'On' : 'Off', o.s1);
        $('equipment').innerHTML = html;
      }

      function kv(label, value, cls){
        return '<div><dt>' + label + '</dt><dd>' + (cls ? '<i class="' + cls + '" aria-hidden="true"></i>' : '') + value + '</dd></div>';
      }
      function renderSystem(){
        var s = st.s, l = s.link, html = '';
        html += kv('SCADA link', l.mqtt ? 'Connected' : l.wifi ? 'Broker unreachable' : 'No Wi-Fi', l.mqtt ? 'is-ok' : 'is-off');
        html += kv('Clock', CLOCK_SRC[l.src] || 'Unknown', l.clock ? (l.src === 1 ? 'is-ok' : 'is-warn') : 'is-off');
        html += kv('SD card', l.sd ? 'Inserted' : 'Not found', l.sd ? 'is-ok' : 'is-warn');
        html += kv('Cycle', s.active ? '#' + s.cycle + ' running' : (s.cycle ? 'Last was #' + s.cycle : 'None yet'));
        html += kv('Uptime', '<span class="tnum">' + dur(s.uptime) + '</span>');
        $('system').innerHTML = html;
      }

      function renderNotices(){
        var s = st.s, list = [];
        if (!s.trusted) list.push(['danger', 'The controller clock is not set. Delayed start is unavailable and times shown here may be wrong.']);
        PROBES.forEach(function(p){
          if (s.temps[p.f]) list.push(['danger', 'Sensor ' + p.name + ' (' + p.label.toLowerCase() + ') is out of range. The last good value is being used; check the probe and its cable.']);
        });
        if (s.out.ov) list.push(['warn', 'The fans were switched on by hand: the <b>fan override</b> switch on the panel is ON. The controller isn&rsquo;t driving them until it&rsquo;s switched back.']);
        if (s.link.wifi && !s.link.mqtt) list.push(['warn', 'The SCADA can&rsquo;t see this room right now: the MQTT broker is unreachable.']);
        $('notices').innerHTML = list.map(function(n){
          return '<div class="ct-notice' + (n[0] === 'danger' ? ' ct-notice--danger' : '') + '" role="status">' + svg(ICONS.warn, 17) + '<span>' + n[1] + '</span></div>';
        }).join('');
      }

      // ---------------- recent events ----------------
      function loadRecent(force){
        if (document.hidden && !force) return;
        getJSON('/api/events?limit=6').then(function(r){
          var key = r.head + ':' + r.events.length;
          if (key === st.recentKey && !force) return;
          st.recentKey = key;
          if (!r.events.length) { $('recent').innerHTML = '<div class="empty">No events recorded yet.</div>'; return; }
          $('recent').innerHTML = r.events.map(function(ev){
            var T = TYPES[ev.k] || TYPES[4], O = ORIGINS[ev.o] || ORIGINS[4];
            return '<div class="ev-row">' +
              '<time class="ev-time">' + (ev.e ? esc(when(ev.e)) : 'boot +' + ev.u + 's') + '</time>' +
              '<span class="ev-type ev-type--' + ev.k + '">' + svg(T.icon, 13) + T.label + '</span>' +
              '<span class="ev-msg">' + esc(ev.m).replace(/ -&gt; /g, ' &rarr; ').replace(/ &gt;= /g, ' &ge; ') + '</span>' +
              '<span class="ev-origin' + (O.human ? ' ev-origin--human' : '') + '" title="' + esc(O.hint) + '">' + O.label + '</span>' +
            '</div>';
          }).join('');
        }).catch(function(){});
      }

      // ---------------- commands ----------------
      var dlg = $('dlg');
      function setText(id, html){ $(id).innerHTML = html; }
      function dateParts(e){ var t = dt(e); return { y: t.getUTCFullYear(), mo: t.getUTCMonth() + 1, d: t.getUTCDate(), h: t.getUTCHours(), m: t.getUTCMinutes() }; }
      function isoDate(p){ return p.y + '-' + pad(p.mo) + '-' + pad(p.d); }

      function openAction(a){
        var s = st.s;
        if (!s || st.sending) return;
        st.action = a;
        $('dlg-error').hidden = true;
        $('dlg-fields').hidden = a !== 'dstart';
        var ok = $('dlg-ok'), g = s.stage;
        ok.disabled = false;
        ok.className = 'btn' + (a === 'stop' && g !== 3 ? ' btn--danger' : '');

        if (a === 'start') {
          setText('dlg-title', 'Start the process now?');
          setText('dlg-body', 'The room goes straight to <b>Stage 2</b> (active thaw). Fans, sprinkler and air infeed start after a 5 second safety delay.');
          ok.textContent = 'Start now';
        } else if (a === 'dstart') {
          setText('dlg-title', 'Schedule a delayed start');
          setText('dlg-body', 'Stage 1 starts now and the room switches to Stage 2 at the time below. Use the <b>room&rsquo;s time</b> (now ' + hm(deviceNow()) + '), not this device&rsquo;s.');
          ok.textContent = 'Start Stage 1';
          prefillSchedule();
        } else if (a === 'lock') {
          setText('dlg-title', 'Enable maintenance lockdown?');
          setText('dlg-body', 'No one can start the process, not from the panel buttons, the SCADA or this page, until the lockdown is lifted. The three stage lights will blink. It stays on after a restart.' +
            (g >= 1 && g <= 3 ? ' <b>' + STAGES[g].name + ' keeps running</b>: stop it separately if the work requires it.' : ''));
          ok.textContent = 'Enable lockdown';
        } else if (a === 'unlock') {
          setText('dlg-title', 'Lift maintenance lockdown?');
          setText('dlg-body', 'Make sure the work on the room is finished. Starting will be allowed again from the panel, the SCADA and this page.');
          ok.textContent = 'Lift lockdown';
        } else {
          if (g === 3) {
            setText('dlg-title', 'Finish the process?');
            setText('dlg-body', 'Fans, sprinkler and air infeed turn off and the cycle is recorded as <b>finished</b>.');
            ok.textContent = 'Finish process';
          } else {
            setText('dlg-title', 'Stop the process?');
            setText('dlg-body', 'The room is in <b>' + esc(STAGES[g] ? STAGES[g].name : 'an unknown stage') + '</b>. Stopping turns off fans, sprinkler and air infeed and records the cycle as stopped early. It can&rsquo;t be resumed: starting again opens a new cycle.');
            ok.textContent = 'Stop process';
          }
        }
        dlg.showModal();
        // En STOP el foco va a Cancelar: un Enter de más no debe parar el cuarto.
        if (a === 'stop' || a === 'unlock') $('dlg-cancel').focus(); else if (a === 'dstart') $('ds-time').focus(); else ok.focus();
      }

      function prefillSchedule(){
        var now = deviceNow(), s2 = st.s.s2, target = 0;
        // Reusar la última programación si sigue en el futuro (la suele dejar el SCADA).
        if (s2.mo >= 1 && s2.d >= 1) {
          var y = dateParts(now).y, e = Date.UTC(y, s2.mo - 1, s2.d, s2.h, s2.m) / 1000;
          if (e <= now) e = Date.UTC(y + 1, s2.mo - 1, s2.d, s2.h, s2.m) / 1000;
          if (e - now > 300 && e - now <= MAX_AHEAD_S) target = e;
        }
        if (!target) target = Math.ceil((now + 2 * 3600) / 900) * 900;   // +2 h, al cuarto de hora
        var p = dateParts(target);
        $('ds-date').value = isoDate(p);
        $('ds-time').value = pad(p.h) + ':' + pad(p.m);
        $('ds-date').min = isoDate(dateParts(now));
        $('ds-date').max = isoDate(dateParts(now + MAX_AHEAD_S));
        updatePreview();
      }
      function readSchedule(){
        var d = /^(\d{4})-(\d{2})-(\d{2})$/.exec($('ds-date').value), t = /^(\d{2}):(\d{2})/.exec($('ds-time').value);
        if (!d || !t) return null;
        var p = { y: +d[1], mo: +d[2], d: +d[3], h: +t[1], m: +t[2] };
        p.epoch = Date.UTC(p.y, p.mo - 1, p.d, p.h, p.m) / 1000;
        return p;
      }
      function updatePreview(){
        var p = readSchedule(), now = deviceNow(), el = $('ds-preview'), bad = '';
        if (!p) bad = 'Pick a date and a time.';
        else if (p.epoch <= now + 60) bad = 'That time has already passed on the room&rsquo;s clock.';
        else if (p.epoch - now > MAX_AHEAD_S) bad = 'Stage 2 can be scheduled at most 7 days ahead.';
        el.className = 'ct-preview tnum' + (bad ? ' is-bad' : '');
        el.innerHTML = bad || 'Stage 2 starts ' + (sameDay(p.epoch, now) ? 'today at ' : '') + when(p.epoch) + ' &middot; in ' + dur(p.epoch - now);
        $('dlg-ok').disabled = !!bad || st.sending;
        return !bad;
      }
      $('ds-date').addEventListener('input', updatePreview);
      $('ds-time').addEventListener('input', updatePreview);

      function showError(msg){
        $('dlg-error').innerHTML = esc(msg);
        $('dlg-error').hidden = false;
        sending(false);
      }
      function sending(on){
        st.sending = on;
        $('dlg-ok').disabled = on;
        $('dlg-cancel').disabled = on;
        if (on) { $('dlg-ok').setAttribute('data-label', $('dlg-ok').textContent); $('dlg-ok').innerHTML = '<span class="ct-spinner" aria-hidden="true"></span>Sending&hellip;'; }
        else if ($('dlg-ok').getAttribute('data-label')) $('dlg-ok').textContent = $('dlg-ok').getAttribute('data-label');
        renderActions();
      }

      $('dlg-form').addEventListener('submit', function(e){
        e.preventDefault();
        if (st.sending) return;
        var body = 'action=' + st.action;
        if (st.action === 'dstart') {
          if (!updatePreview()) return;
          var p = readSchedule();
          body += '&hour=' + p.h + '&minute=' + p.m + '&day=' + p.d + '&month=' + p.mo;
        }
        sending(true);
        fetch('/api/control', {
          method: 'POST', credentials: 'same-origin', cache: 'no-store',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded', 'X-Control': '1' },
          body: body
        }).then(function(r){
          return r.json().catch(function(){ return {}; }).then(function(j){ return { status: r.status, j: j }; });
        }).then(function(res){
          if (res.status === 202 && res.j.id) return waitFor(res.j.id);
          showError(res.j.msg || 'The controller rejected the command (HTTP ' + res.status + ').');
        }).catch(function(){
          showError('No answer from the controller. Check the stage before trying again.');
        });
      });

      // El handler solo encola: la confirmación real llega cuando el loop del equipo lo
      // ejecuta y /api/status trae ese id de vuelta con el resultado.
      function waitFor(id){
        var t0 = Date.now();
        function check(){
          getJSON('/api/status').then(function(s){
            take(s);
            if (s.cmd && s.cmd.id >= id) return done(s.cmd.ok ? 'ok' : 'error', s.cmd.msg);
            if (Date.now() - t0 > CONFIRM_MS) return done('warn', 'The controller has not confirmed the command yet. Check the stage before trying again.');
            setTimeout(check, 350);
          }).catch(function(){
            if (Date.now() - t0 > CONFIRM_MS) return done('warn', 'Lost contact with the controller. Check the stage before trying again.');
            setTimeout(check, 700);
          });
        }
        check();
      }
      function done(kind, msg){
        sending(false);
        dlg.close();
        toast(kind, msg);
        render();
        loadRecent(true);
      }

      var toastTimer = null;
      function toast(kind, msg){
        var el = $('toast');
        el.className = 'ct-toast' + (kind === 'ok' ? '' : ' ct-toast--' + kind);
        el.innerHTML = svg(kind === 'ok' ? ICONS.check : ICONS.warn, 17) + '<span>' + esc(msg) + '</span>';
        el.hidden = false;
        clearTimeout(toastTimer);
        toastTimer = setTimeout(function(){ el.hidden = true; }, kind === 'ok' ? 5000 : 9000);
      }

      $('btn-start').addEventListener('click', function(){ openAction('start'); });
      $('btn-dstart').addEventListener('click', function(){ openAction('dstart'); });
      $('btn-stop').addEventListener('click', function(){ openAction('stop'); });
      $('btn-lock').addEventListener('click', function(){ openAction(st.s && st.s.lock.on ? 'unlock' : 'lock'); });
      $('btn-unlock').addEventListener('click', function(){ openAction('unlock'); });
      $('dlg-cancel').addEventListener('click', function(){ if (!st.sending) dlg.close(); });
      dlg.addEventListener('cancel', function(e){ if (st.sending) e.preventDefault(); });
      $('toast').addEventListener('click', function(){ this.hidden = true; });

      document.addEventListener('visibilitychange', function(){ if (!document.hidden) { poll(); loadRecent(); } });
      function tick(){
        var now = deviceNow();
        if (now) { var t = dt(now); $('clock').textContent = hm(now) + ':' + pad(t.getUTCSeconds()); }
      }
      setInterval(tick, 1000);
      poll(); loadRecent(true);
      setInterval(poll, POLL_MS);
      setInterval(loadRecent, RECENT_MS);
      setInterval(function(){ if (st.s && !st.failing) renderHero(); }, 30000);
    </script>
  </body>
</html>
)RAW_HTML";

const char* CALIBRATION_HTML = R"RAW_HTML(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <title>Thawing Room &middot; Calibration</title>
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <link rel="icon" href="data:," />
    <link rel="stylesheet" type="text/css" href="style.css" />
  </head>
  <body>
)RAW_HTML"
  NAV_BLOCK("", "", "", "", "class=\"active\" aria-current=\"page\"", "")
  R"RAW_HTML(
    <div class="content">
      <main class="page">
        <div class="topbar">
          <div>
            <h1>Sensor calibration</h1>
            <p class="sub">Three-point calibration of each probe against a reference thermometer.</p>
          </div>
          <span class="chip"><span class="dot is-idle" id="live-dot"></span><span id="live-text">Connecting&hellip;</span></span>
        </div>

        <div class="ev-banner" id="banner" role="status" hidden>
          <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><path d="M12 9v4M12 17h.01"/><path d="M10.3 3.9 1.8 18a2 2 0 0 0 1.7 3h17a2 2 0 0 0 1.7-3L13.7 3.9a2 2 0 0 0-3.4 0z"/></svg>
          <span>Can&rsquo;t reach the controller. Retrying&hellip;</span>
        </div>
        <div class="ct-notice cb-gate" id="gate" hidden>
          <svg width="17" height="17" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.9" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><rect x="5" y="11" width="14" height="10" rx="2"/><path d="M8 11V7a4 4 0 0 1 8 0v4"/></svg>
          <span id="gate-text"></span>
          <a class="btn btn--ghost ev-btn-sm" href="/control">Open Control</a>
        </div>

        <section class="cb-resume card" id="resume" hidden>
          <span id="resume-text"></span>
          <div class="btn-row">
            <button type="button" class="btn ev-btn-sm" id="resume-yes">Resume</button>
            <button type="button" class="btn btn--ghost ev-btn-sm" id="resume-no">Discard</button>
          </div>
        </section>

        <section id="overview" aria-labelledby="ov-title">
          <h2 id="ov-title" class="sr-only">Probes</h2>
          <div class="card cb-probes" id="probes">
            <div class="ev-skel" style="height:52px"></div><div class="ev-skel" style="height:52px"></div><div class="ev-skel" style="height:52px"></div>
          </div>
          <p class="ev-note">Calibration covers the whole chain: probe, transmitter, cable, isolation module and ADC. Use a reference thermometer you trust, and baths at least 5&nbsp;&deg;C apart. Good choices are ice water (0&nbsp;&deg;C, the most important one for the product), a cold bath around &minus;18&nbsp;&deg;C and water at room temperature.</p>
        </section>

        <section class="card cb-wizard" id="wizard" aria-labelledby="wz-title" hidden>
          <div class="cb-wz-head">
            <div>
              <h2 id="wz-title"></h2>
              <p class="ct-muted" id="wz-sub"></p>
            </div>
            <button type="button" class="btn btn--ghost ev-btn-sm" id="wz-cancel">Cancel</button>
          </div>
          <ol class="cb-steps" id="wz-steps" aria-label="Calibration steps">
            <li><b>1</b><span>Point 1</span></li>
            <li><b>2</b><span>Point 2</span></li>
            <li><b>3</b><span>Point 3</span></li>
            <li><b>4</b><span>Review</span></li>
          </ol>

          <div id="wz-point">
            <p class="cb-instr" id="wz-instr"></p>
            <div class="cb-live">
              <div class="cb-live-main">
                <span class="cb-counts tnum" id="lv-counts">&ndash;</span>
                <span class="ct-muted">ADC counts (20 s average)</span>
              </div>
              <div class="cb-live-side">
                <span class="tnum" id="lv-temp">&ndash;</span>
                <span class="ct-muted">with the current calibration</span>
              </div>
              <div class="cb-stab" id="lv-stab">
                <span class="cb-stab-bar" aria-hidden="true"><i id="lv-stab-fill"></i></span>
                <span id="lv-stab-text" aria-live="polite">Waiting for readings&hellip;</span>
              </div>
            </div>
            <div class="cb-ref">
              <div class="field">
                <label for="ref-temp">Reference thermometer reads</label>
                <div class="cb-ref-input"><input type="number" id="ref-temp" step="0.1" min="-50" max="60" inputmode="decimal" autocomplete="off" /><span>&deg;C</span></div>
              </div>
              <button type="button" class="btn" id="btn-capture" disabled>Capture point</button>
            </div>
            <button type="button" class="ev-link" id="btn-capture-anyway" hidden>Capture now without waiting</button>
            <p class="ct-dialog-error" id="pt-error" role="alert" hidden></p>
          </div>

          <div id="wz-review" hidden>
            <div class="cb-chart" id="chart" role="img" aria-label="Old and new calibration curves"></div>
            <div class="cb-legend">
              <span><i class="cb-key cb-key--new"></i>New calibration</span>
              <span><i class="cb-key cb-key--old"></i>Current</span>
              <span><i class="cb-key cb-key--band"></i>Product target (&minus;3 to +2&nbsp;&deg;C)</span>
            </div>
            <table class="cb-table">
              <thead><tr><th>Point</th><th>Counts</th><th>Reference</th><th>Reads today</th><th>Change</th></tr></thead>
              <tbody id="rv-rows"></tbody>
            </table>
            <p class="cb-summary" id="rv-summary"></p>
            <p class="ct-dialog-error" id="rv-error" role="alert" hidden></p>
            <div class="ct-actions">
              <button type="button" class="btn btn--ghost" id="rv-back">Back</button>
              <button type="button" class="btn" id="rv-save">Save calibration</button>
            </div>
          </div>

          <ul class="cb-captured" id="wz-captured" aria-label="Captured points"></ul>
        </section>
      </main>
      <footer class="footer">
        <span>Thawing Room &middot; {{LOCATION}}</span>
        <span>v{{VERSION}} &middot; <span class="tnum" id="clock">--:--:--</span></span>
      </footer>
    </div>

    <dialog class="ct-dialog" id="dlg" aria-labelledby="dlg-title" aria-describedby="dlg-body">
      <form method="dialog" id="dlg-form">
        <h2 id="dlg-title"></h2>
        <p class="ct-dialog-body" id="dlg-body"></p>
        <p class="ct-dialog-error" id="dlg-error" role="alert" hidden></p>
        <div class="ct-dialog-actions">
          <button type="button" class="btn btn--ghost" id="dlg-cancel">Cancel</button>
          <button type="submit" class="btn" id="dlg-ok">Confirm</button>
        </div>
      </form>
    </dialog>
    <div class="ct-toast" id="toast" role="status" aria-live="polite" hidden></div>

    <script>
      var LABELS = { ta: 'Room air', ts: 'Product surface', tc: 'Product core' };
      // Mismas reglas que CalibrationMath.cpp: el equipo vuelve a validar al guardar.
      var RAW_MIN = 819, RAW_MAX = 4095, MIN_GAP_C = 5, MIN_GAP_RAW = 40, SLOPE_MIN = 0.008, SLOPE_MAX = 0.12;
      var STABLE_SPAN_C = 0.2, STABLE_MIN_N = 60, ANYWAY_MIN_N = 8;
      var MON = ['Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec'];
      var SAVE_KEY = 'thawing.calibration.wizard';

      var st = { d: null, failing: false, wz: null, busy: false, timer: null };

      function $(id){ return document.getElementById(id); }
      function esc(s){ return String(s).replace(/[&<>"']/g, function(c){ return {'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]; }); }
      function svgIcon(path, size){ return '<svg width="'+(size||16)+'" height="'+(size||16)+'" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.9" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">'+path+'</svg>'; }
      function pad(n){ return (n < 10 ? '0' : '') + n; }
      function fmt(v, d){ return v === null || v === undefined || isNaN(v) ? '&ndash;' : (v < 0 ? '&minus;' : '') + Math.abs(v).toFixed(d === undefined ? 1 : d); }
      function signed(v){ v = Math.round(v * 10) / 10; return (v > 0 ? '+' : v < 0 ? '&minus;' : '&plusmn;') + Math.abs(v).toFixed(1); }
      function dateOf(e){ var t = new Date(e * 1000); return MON[t.getUTCMonth()] + ' ' + t.getUTCDate() + ', ' + t.getUTCFullYear(); }
      function store(v){ try { if (v) localStorage.setItem(SAVE_KEY, JSON.stringify(v)); else localStorage.removeItem(SAVE_KEY); } catch (e) {} }
      function restore(){ try { return JSON.parse(localStorage.getItem(SAVE_KEY) || 'null'); } catch (e) { return null; } }
      function chan(k){ if (!st.d) return null; for (var i = 0; i < st.d.ch.length; i++) if (st.d.ch[i].k === k) return st.d.ch[i]; return null; }

      // ---- misma matemática que el equipo ----
      function apply(c, raw){
        if (!c.cur) return raw * c.f[0] + c.f[1];
        var p = c.cur.pts, a = raw <= p[1][0] ? p[0] : p[1], b = raw <= p[1][0] ? p[1] : p[2];
        return a[1] + (b[1] - a[1]) / (b[0] - a[0]) * (raw - a[0]);
      }
      function applyPts(p, raw){
        var a = raw <= p[1][0] ? p[0] : p[1], b = raw <= p[1][0] ? p[1] : p[2];
        return a[1] + (b[1] - a[1]) / (b[0] - a[0]) * (raw - a[0]);
      }
      function localSlope(c, raw){ return Math.abs(apply(c, raw + 50) - apply(c, raw - 50)) / 100; }
      function validate(points){
        for (var i = 0; i < points.length; i++) {
          var p = points[i];
          if (p[1] < -50 || p[1] > 60) return 'Reference temperatures must be between -50 and 60 °C.';
          if (p[0] < RAW_MIN) return 'A point reads below 4 mA (819 counts): the probe loop is open or faulty.';
        }
        var s = points.slice().sort(function(a, b){ return a[0] - b[0]; });
        for (var j = 1; j < s.length; j++) {
          var dr = s[j][0] - s[j-1][0], dt = s[j][1] - s[j-1][1];
          if (dr < MIN_GAP_RAW) return 'Two points read almost the same counts. Use baths at least 5 °C apart.';
          if (dt <= 0) return 'Counts and temperatures go in opposite directions. Check the reference values.';
          if (dt < MIN_GAP_C) return 'Points must be at least 5 °C apart.';
          var sl = dt / dr;
          if (sl < SLOPE_MIN || sl > SLOPE_MAX) return 'The counts don’t match the temperature change. Check the reference values and the probe.';
        }
        return null;
      }

      function getJSON(url){
        return fetch(url, { cache: 'no-store', credentials: 'same-origin' }).then(function(r){
          if (!r.ok) throw new Error('HTTP ' + r.status);
          return r.json();
        });
      }
      function post(body){
        return fetch('/api/calibration', {
          method: 'POST', credentials: 'same-origin', cache: 'no-store',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded', 'X-Control': '1' }, body: body
        }).then(function(r){ return r.json().catch(function(){ return { ok: false, msg: 'Unexpected answer (HTTP ' + r.status + ').' }; }); });
      }

      function canEdit(){ return st.d && st.d.ready && st.d.lock && st.d.stage === 0 && !st.failing; }

      function poll(){
        getJSON('/api/calibration').then(function(d){
          st.d = d; st.failing = false; $('banner').hidden = true; render();
        }).catch(function(){
          st.failing = true; $('banner').hidden = false; render();
        });
      }
      function schedule(){
        clearInterval(st.timer);
        st.timer = setInterval(function(){ if (!document.hidden) poll(); }, st.wz ? 1000 : 3000);
      }

      // ---------------- render ----------------
      function render(){
        var d = st.d;
        if (!d) return;
        $('live-dot').className = 'dot' + (st.failing ? ' is-idle' : d.lock ? ' is-lock' : ' is-idle');
        $('live-text').innerHTML = st.failing ? 'Offline' : d.lock ? '<b>Lockdown</b> &middot; ready to calibrate' : 'Read only';

        var gate = '';
        if (!d.lock) gate = 'Turn on the <b>maintenance lockdown</b> to calibrate. Readings below are live, but nothing can be captured or saved until then.';
        else if (d.stage !== 0) gate = 'The process is running. Stop it before calibrating: the probes will leave the product.';
        $('gate').hidden = !gate || st.failing;
        $('gate-text').innerHTML = gate;

        renderProbes();
        if (st.wz) renderWizard();
      }

      function renderProbes(){
        var html = '';
        st.d.ch.forEach(function(c){
          var live = c.live && c.live.n ? apply(c, c.live.mean) : null;
          var status = c.cur ? '3-point calibration' + (c.cur.at ? ' &middot; ' + dateOf(c.cur.at) : '') : 'Factory ramp';
          var actions = '';
          if (!c.avail) {
            actions = '<span class="ct-muted">Read by IR or LoRa, not from this input</span>';
          } else {
            actions = '<button type="button" class="btn ev-btn-sm" data-cal="' + c.k + '"' + (canEdit() && !st.wz ? '' : ' disabled') + '>Calibrate</button>';
            if (c.prev) actions += '<button type="button" class="btn btn--ghost ev-btn-sm" data-revert="' + c.k + '"' + (canEdit() ? '' : ' disabled') + '>Undo last</button>';
            if (c.cur) actions += '<button type="button" class="btn btn--ghost ev-btn-sm" data-reset="' + c.k + '"' + (canEdit() ? '' : ' disabled') + '>Factory ramp</button>';
          }
          html += '<div class="cb-probe' + (st.wz && st.wz.ch === c.k ? ' is-active' : '') + '">' +
            '<div class="cb-probe-name"><b>' + c.name + '</b><span>' + LABELS[c.k] + '</span></div>' +
            '<div class="cb-probe-status"><span class="cb-badge' + (c.cur ? ' is-cal' : '') + '">' + status + '</span></div>' +
            '<div class="cb-probe-read tnum"><span class="cb-v">' + (live === null ? '&ndash;' : fmt(live) + '<small>&deg;C</small>') + '</span>' +
              '<span class="cb-c">' + (c.live && c.live.n ? Math.round(c.live.mean) + ' counts' : 'no reading') + '</span></div>' +
            '<div class="cb-probe-actions">' + actions + '</div></div>';
        });
        $('probes').innerHTML = html;
      }

      $('probes').addEventListener('click', function(e){
        var b = e.target.closest('button');
        if (!b || b.disabled) return;
        if (b.getAttribute('data-cal')) startWizard(b.getAttribute('data-cal'), []);
        else if (b.getAttribute('data-revert')) confirmOp('revert', b.getAttribute('data-revert'));
        else if (b.getAttribute('data-reset')) confirmOp('reset', b.getAttribute('data-reset'));
      });

      // ---------------- wizard ----------------
      var INSTR = [
        'Put the probe and the reference thermometer side by side in the first bath, at the same depth. Ice water (0 °C) is a good first point.',
        'Move both to a second bath at least 5 °C away from the first one, for example a cold bath around −18 °C.',
        'Last bath, at least 5 °C away from the other two, for example water at room temperature (20–25 °C).'
      ];
      function startWizard(k, pts){
        st.wz = { ch: k, pts: pts || [], review: false };
        store(st.wz);
        $('resume').hidden = true;
        $('wizard').hidden = false;
        $('ref-temp').value = '';
        schedule(); render();
        $('wizard').scrollIntoView({ behavior: 'smooth', block: 'start' });
        if (!st.wz.review) setTimeout(function(){ $('ref-temp').focus({ preventScroll: true }); }, 300);
      }
      function endWizard(){
        st.wz = null; store(null);
        $('wizard').hidden = true;
        schedule(); render();
      }

      function stability(c){
        var l = c.live || { n: 0 };
        if (!l.n) return { ok: false, anyway: false, pct: 0, text: 'Waiting for readings…' };
        var spanC = (l.max - l.min) * localSlope(c, l.mean);
        var secs = Math.round(l.n * (st.d.window_ms / 80) / 1000), full = Math.round(st.d.window_ms / 1000);
        if (l.n < STABLE_MIN_N) return { ok: false, anyway: l.n >= ANYWAY_MIN_N, pct: l.n / STABLE_MIN_N * 100, text: 'Collecting readings… ' + secs + ' s of ' + Math.round(STABLE_MIN_N * st.d.window_ms / 80 / 1000) + ' s' };
        if (spanC > STABLE_SPAN_C) return { ok: false, anyway: true, pct: Math.max(5, Math.min(95, STABLE_SPAN_C / spanC * 100)), text: 'Settling: the reading moved ' + spanC.toFixed(2) + ' °C in the last ' + full + ' s' };
        return { ok: true, anyway: false, pct: 100, text: 'Stable: within ±' + (spanC / 2).toFixed(2) + ' °C over the last ' + full + ' s' };
      }

      function renderWizard(){
        var w = st.wz, c = chan(w.ch);
        if (!c) return;
        $('wz-title').textContent = 'Calibrating ' + c.name;
        $('wz-sub').textContent = LABELS[c.k] + (c.cur ? ' · currently on a 3-point calibration' : ' · currently on the factory ramp');
        var step = w.review ? 3 : w.pts.length;
        var lis = $('wz-steps').children;
        for (var i = 0; i < lis.length; i++) {
          lis[i].className = i < step ? 'is-done' : i === step ? 'is-current' : '';
          if (i === step) lis[i].setAttribute('aria-current', 'step'); else lis[i].removeAttribute('aria-current');
        }
        $('wz-point').hidden = w.review;
        $('wz-review').hidden = !w.review;

        if (!w.review) {
          var l = c.live || { n: 0 }, s = stability(c);
          $('wz-instr').textContent = INSTR[w.pts.length];
          $('lv-counts').textContent = l.n ? l.mean.toFixed(0) : '–';
          $('lv-temp').innerHTML = l.n ? fmt(apply(c, l.mean)) + ' °C' : '–';
          $('lv-stab').className = 'cb-stab' + (s.ok ? ' is-ok' : '');
          $('lv-stab-fill').style.transform = 'scaleX(' + (s.pct / 100) + ')';
          $('lv-stab-text').textContent = s.text;
          var ref = $('ref-temp').value.trim() !== '' && !isNaN(parseFloat($('ref-temp').value));
          $('btn-capture').textContent = 'Capture point ' + (w.pts.length + 1);
          $('btn-capture').disabled = !(canEdit() && s.ok && ref);
          $('btn-capture-anyway').hidden = !(canEdit() && !s.ok && s.anyway && ref);
        } else {
          renderReview(c);
        }

        $('wz-captured').innerHTML = w.pts.map(function(p, i){
          return '<li><b>Point ' + (i + 1) + '</b><span class="tnum">' + p[0] + ' counts = ' + fmt(p[1]) + ' °C</span>' +
                 '<button type="button" class="ev-link" data-redo="' + i + '">Redo</button></li>';
        }).join('');
      }

      function capture(){
        var c = chan(st.wz.ch), l = c.live, t = parseFloat($('ref-temp').value);
        $('pt-error').hidden = true;
        if (!l || !l.n || isNaN(t)) return;
        var candidate = st.wz.pts.concat([[Math.round(l.mean), Math.round(t * 100) / 100]]);
        // Errores evidentes al momento, no hasta el final: punto repetido, lazo abierto.
        var bad = candidate[candidate.length - 1][0] < RAW_MIN ? 'This reads below 4 mA (819 counts): the probe loop is open or faulty.' : null;
        for (var i = 0; !bad && i < st.wz.pts.length; i++) {
          if (Math.abs(st.wz.pts[i][1] - t) < MIN_GAP_C) bad = 'This bath is less than 5 °C away from point ' + (i + 1) + '. Use a different bath.';
        }
        if (bad) { $('pt-error').textContent = bad; $('pt-error').hidden = false; return; }
        st.wz.pts = candidate;
        if (st.wz.pts.length === 3) st.wz.review = true;
        store(st.wz);
        $('ref-temp').value = '';
        renderWizard();
        if (!st.wz.review) $('ref-temp').focus();
      }
      $('btn-capture').addEventListener('click', capture);
      $('btn-capture-anyway').addEventListener('click', capture);
      $('ref-temp').addEventListener('input', function(){ $('pt-error').hidden = true; renderWizard(); });
      $('ref-temp').addEventListener('keydown', function(e){ if (e.key === 'Enter' && !$('btn-capture').disabled) capture(); });
      $('wz-captured').addEventListener('click', function(e){
        var b = e.target.closest('[data-redo]');
        if (!b) return;
        st.wz.pts.splice(+b.getAttribute('data-redo'), 1);
        st.wz.review = false;
        store(st.wz); renderWizard();
      });
      $('wz-cancel').addEventListener('click', function(){
        if (!st.wz.pts.length) return endWizard();
        ask('Discard this calibration?', 'The points you captured for ' + esc((chan(st.wz.ch) || { name: st.wz.ch }).name) + ' will be lost. The current calibration stays as it is.', 'Discard', true, function(){ endWizard(); return Promise.resolve({ ok: true }); });
      });
      $('rv-back').addEventListener('click', function(){ st.wz.pts.pop(); st.wz.review = false; store(st.wz); renderWizard(); });

      function renderReview(c){
        var pts = st.wz.pts.slice().sort(function(a, b){ return a[0] - b[0]; });
        var bad = validate(pts);
        $('rv-error').hidden = !bad;
        $('rv-error').textContent = bad || '';
        $('rv-save').disabled = !!bad || !canEdit() || st.busy;

        // En orden de captura, igual que los chips de abajo: "Point 2" es el mismo en los dos.
        $('rv-rows').innerHTML = st.wz.pts.map(function(p, i){
          var old = apply(c, p[0]), diff = p[1] - old;
          return '<tr><td>' + (i + 1) + '</td><td class="tnum">' + p[0] + '</td><td class="tnum">' + fmt(p[1]) + ' °C</td>' +
                 '<td class="tnum">' + fmt(old) + ' °C</td><td class="tnum' + (Math.abs(diff) >= 1 ? ' cb-big' : '') + '">' + signed(diff) + ' °C</td></tr>';
        }).join('');

        if (!bad) {
          var at0 = 0, raw0 = 0, best = 1e9;
          for (var r = RAW_MIN; r <= RAW_MAX; r += 4) { var t = applyPts(pts, r); if (Math.abs(t) < best) { best = Math.abs(t); raw0 = r; } }
          at0 = apply(c, raw0);
          $('rv-summary').innerHTML = 'Where the new calibration reads <b>0.0 °C</b>, the probe reads <b>' + fmt(at0) + ' °C</b> today: a change of <b>' + signed(-at0) + ' °C</b> in the product range.';
        } else {
          $('rv-summary').innerHTML = '';
        }
        drawChart(c, bad ? null : pts);
      }

      function drawChart(c, pts){
        var W = 640, H = 260, L = 44, R = 12, T = 12, B = 30;
        var x0 = RAW_MIN, x1 = RAW_MAX, y0 = -50, y1 = 50;
        function X(r){ return L + (r - x0) / (x1 - x0) * (W - L - R); }
        function Y(t){ return T + (y1 - Math.max(y0, Math.min(y1, t))) / (y1 - y0) * (H - T - B); }
        function path(fn){ var d = ''; for (var r = x0; r <= x1; r += 32) d += (d ? 'L' : 'M') + X(r).toFixed(1) + ' ' + Y(fn(r)).toFixed(1); return d; }
        var g = '';
        for (var t = -40; t <= 40; t += 20) g += '<line x1="' + L + '" x2="' + (W - R) + '" y1="' + Y(t) + '" y2="' + Y(t) + '" class="cb-grid"/><text x="' + (L - 8) + '" y="' + (Y(t) + 4) + '" class="cb-ax" text-anchor="end">' + t + '</text>';
        [1000, 2000, 3000, 4000].forEach(function(r){ g += '<text x="' + X(r) + '" y="' + (H - 10) + '" class="cb-ax" text-anchor="middle">' + r + '</text>'; });
        g += '<rect x="' + L + '" width="' + (W - L - R) + '" y="' + Y(2) + '" height="' + (Y(-3) - Y(2)) + '" class="cb-band"/>';
        g += '<path d="' + path(function(r){ return apply(c, r); }) + '" class="cb-old"/>';
        if (pts) {
          g += '<path d="' + path(function(r){ return applyPts(pts, r); }) + '" class="cb-new"/>';
          pts.forEach(function(p){ g += '<circle cx="' + X(p[0]) + '" cy="' + Y(p[1]) + '" r="4.5" class="cb-pt"/>'; });
        }
        $('chart').innerHTML = '<svg viewBox="0 0 ' + W + ' ' + H + '" preserveAspectRatio="none" aria-hidden="true">' + g + '</svg><span class="cb-ax-x">ADC counts</span><span class="cb-ax-y">°C</span>';
      }

      $('rv-save').addEventListener('click', function(){
        var c = chan(st.wz.ch), pts = st.wz.pts.slice().sort(function(a, b){ return a[0] - b[0]; });
        var body = 'action=save&ch=' + c.k;
        pts.forEach(function(p, i){ body += '&r' + (i + 1) + '=' + p[0] + '&t' + (i + 1) + '=' + p[1]; });
        ask('Save the new ' + c.name + ' calibration?', 'It takes effect right away and is used for the process, the SCADA and the event log. The current calibration is kept so you can undo it.', 'Save calibration', false, function(){
          return post(body).then(function(r){ if (r.ok) endWizard(); return r; });
        });
      });

      // ---------------- confirmations ----------------
      var dlg = $('dlg'), dlgAction = null;
      function ask(title, body, okText, danger, action){
        $('dlg-title').textContent = title;
        $('dlg-body').innerHTML = body;
        $('dlg-error').hidden = true;
        $('dlg-ok').textContent = okText;
        $('dlg-ok').className = 'btn' + (danger ? ' btn--danger' : '');
        $('dlg-ok').disabled = false;
        dlgAction = action;
        dlg.showModal();
        $('dlg-cancel').focus();
      }
      function confirmOp(op, k){
        var c = chan(k);
        if (op === 'revert') {
          ask('Undo the last ' + c.name + ' calibration?', c.prev_factory ? 'The probe goes back to the factory ramp.' : 'The probe goes back to its previous 3-point calibration.', 'Undo', false,
              function(){ return post('action=revert&ch=' + k); });
        } else {
          ask('Use the factory ramp for ' + c.name + '?', 'The 3-point calibration is set aside (you can undo this). The factory ramp can be several degrees off at both ends of the range.', 'Use factory ramp', true,
              function(){ return post('action=reset&ch=' + k); });
        }
      }
      $('dlg-form').addEventListener('submit', function(e){
        e.preventDefault();
        if (st.busy || !dlgAction) return;
        st.busy = true; $('dlg-ok').disabled = true; $('dlg-cancel').disabled = true;
        dlgAction().then(function(r){
          st.busy = false; $('dlg-cancel').disabled = false;
          if (r.ok) { dlg.close(); if (r.msg) toast('ok', r.msg); poll(); }
          else { $('dlg-error').textContent = r.msg || 'The controller rejected the change.'; $('dlg-error').hidden = false; $('dlg-ok').disabled = false; }
        }).catch(function(){
          st.busy = false; $('dlg-cancel').disabled = false; $('dlg-ok').disabled = false;
          $('dlg-error').textContent = 'No answer from the controller. Check the calibration before trying again.'; $('dlg-error').hidden = false;
        });
      });
      $('dlg-cancel').addEventListener('click', function(){ if (!st.busy) dlg.close(); });
      dlg.addEventListener('cancel', function(e){ if (st.busy) e.preventDefault(); });

      var toastTimer = null;
      function toast(kind, msg){
        var el = $('toast');
        el.className = 'ct-toast' + (kind === 'ok' ? '' : ' ct-toast--' + kind);
        el.innerHTML = svgIcon(kind === 'ok' ? '<path d="m5 12 5 5 9-10"/>' : '<path d="M12 9v4M12 17h.01"/><path d="M10.3 3.9 1.8 18a2 2 0 0 0 1.7 3h17a2 2 0 0 0 1.7-3L13.7 3.9a2 2 0 0 0-3.4 0z"/>', 17) + '<span>' + esc(msg) + '</span>';
        el.hidden = false;
        clearTimeout(toastTimer);
        toastTimer = setTimeout(function(){ el.hidden = true; }, 6000);
      }
      $('toast').addEventListener('click', function(){ this.hidden = true; });

      // Una calibración toma media hora: si se recarga la página no se pierde lo capturado.
      var saved = restore();
      if (saved && saved.ch && saved.pts && saved.pts.length) {
        $('resume').hidden = false;
        $('resume-text').innerHTML = 'You have an unfinished <b>' + esc(saved.ch.charAt(0).toUpperCase() + saved.ch.slice(1)) + '</b> calibration with ' + saved.pts.length + ' of 3 points captured.';
        $('resume-yes').addEventListener('click', function(){ startWizard(saved.ch, saved.pts); if (saved.review) { st.wz.review = true; renderWizard(); } });
        $('resume-no').addEventListener('click', function(){ store(null); $('resume').hidden = true; });
      }

      function tick(){ $('clock').textContent = new Date().toLocaleTimeString(); }
      setInterval(tick, 1000); tick();
      poll(); schedule();
    </script>
  </body>
</html>
)RAW_HTML";

const char* HELP_HTML = R"RAW_HTML(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <title>Thawing Room &middot; Help</title>
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <link rel="icon" href="data:," />
    <link rel="stylesheet" type="text/css" href="style.css" />
  </head>
  <body>
)RAW_HTML"
  NAV_BLOCK("", "", "", "", "", "class=\"active\" aria-current=\"page\"")
  R"RAW_HTML(
    <div class="content">
      <main class="page">
        <div class="topbar">
          <div>
            <h1>Help</h1>
            <p class="sub">Quick guide for running the room. Works without internet.</p>
          </div>
          <a class="btn btn--ghost ev-btn-sm" href="https://ayam-brand-iot.github.io/thawingroom/" target="_blank" rel="noopener">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.9" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><path d="M4 5.5A1.5 1.5 0 0 1 5.5 4H11v16H5.5A1.5 1.5 0 0 1 4 18.5z"/><path d="M20 5.5A1.5 1.5 0 0 0 18.5 4H13v16h5.5a1.5 1.5 0 0 0 1.5-1.5z"/></svg>
            Full manual
          </a>
        </div>

        <nav class="hp-jump" aria-label="On this page">
          <a href="#buttons">Buttons</a><a href="#lights">Lights</a><a href="#stages">Stages</a><a href="#stop">Stop or finish</a><a href="#problems">Problems</a><a href="#pages">This website</a>
        </nav>

        <section class="hp-sec" id="buttons" aria-labelledby="h-buttons">
          <h2 id="h-buttons">Which button?</h2>
          <div class="hp-choose">
            <div class="hp-choice hp-go">
              <span class="hp-q"><b>Fish is in.</b> Thaw it now.</span>
              <span class="hp-press"><svg viewBox="0 0 24 24" aria-hidden="true"><path fill="currentColor" d="M8 5.5v13a1 1 0 0 0 1.5.9l10.4-6.5a1 1 0 0 0 0-1.8L9.5 4.6A1 1 0 0 0 8 5.5z"/></svg>START</span>
              <span>Goes straight to <b>Stage 2</b>.</span>
            </div>
            <div class="hp-choice hp-later">
              <span class="hp-q"><b>Fish is in.</b> Thaw it later.</span>
              <span class="hp-press"><svg viewBox="0 0 24 24" aria-hidden="true"><g fill="none" stroke="currentColor" stroke-width="2.4" stroke-linecap="round"><circle cx="12" cy="12" r="9"/><path d="M12 7v5l3 2"/></g></svg>D-START</span>
              <span>Waits in <b>Stage 1</b>, then starts Stage 2 at the set time. Set the time in the SCADA or on <a href="/control">Control</a> first.</span>
            </div>
            <div class="hp-choice hp-halt">
              <span class="hp-q"><b>Fish is leaving</b>, or something is wrong.</span>
              <span class="hp-press"><svg viewBox="0 0 24 24" aria-hidden="true"><rect x="5" y="5" width="14" height="14" rx="2.5" fill="currentColor"/></svg>STOP</span>
              <span>Everything turns off. STOP always works.</span>
            </div>
          </div>
          <p class="hp-note">The SCADA and the <a href="/control">Control</a> page have the same three commands. After a power cut the room comes back <b>in the same stage</b> by itself.</p>
        </section>

        <section class="hp-sec" id="lights" aria-labelledby="h-lights">
          <h2 id="h-lights">The stage lights</h2>
          <div class="card hp-lights">
            <div class="hp-light"><span class="hp-lamps"><i></i><i></i><i></i></span><b>All off</b><span>Idle. Ready to start.</span></div>
            <div class="hp-light"><span class="hp-lamps"><i class="on"></i><i></i><i></i></span><b>Light 1</b><span>Stage 1. Waiting for the set time.</span></div>
            <div class="hp-light"><span class="hp-lamps"><i></i><i class="on"></i><i></i></span><b>Light 2</b><span>Stage 2. Thawing.</span></div>
            <div class="hp-light"><span class="hp-lamps"><i></i><i></i><i class="on"></i></span><b>Light 3</b><span>Stage 3. Fish is ready: press STOP when it leaves.</span></div>
            <div class="hp-light hp-light--lock"><span class="hp-lamps"><i class="blink"></i><i class="blink"></i><i class="blink"></i></span><b>All blinking</b><span>Maintenance lockdown. Nothing can start. Ask the supervisor.</span></div>
          </div>
        </section>

        <section class="hp-sec" id="stages" aria-labelledby="h-stages">
          <h2 id="h-stages">The 3 stages</h2>
          <div class="hp-stages">
            <div class="card hp-stage hp-s1"><span class="hp-badge">1</span><b>Wait</b><span>Fans and sprinkler cycle. Air off.</span><span class="hp-ends">Goes to Stage 2 <b>at the set time</b>.</span></div>
            <div class="card hp-stage hp-s2"><span class="hp-badge">2</span><b>Thaw</b><span>Fans, air in and sprinkler.</span><span class="hp-ends">Goes to Stage 3 when <b>both Ts and Tc</b> reach their target.</span></div>
            <div class="card hp-stage hp-s3"><span class="hp-badge">3</span><b>Hold</b><span>Fans and sprinkler. Air off.</span><span class="hp-ends"><b>Never ends by itself.</b> Press STOP when the fish leaves.</span></div>
          </div>
          <div class="hp-probes">
            <span><b>Ta</b> room air</span><span><b>Ts</b> under a fish plate</span><span><b>Tc</b> pushed into a fish</span>
          </div>
          <p class="hp-note hp-note--warn">Keep the Tc probe inside a fish during the whole cycle. Without it Stage 2 can end too early or never end.</p>
        </section>

        <section class="hp-sec" id="stop" aria-labelledby="h-stop">
          <h2 id="h-stop">Stop or finish</h2>
          <div class="hp-outcomes">
            <div class="hp-outcome hp-fin"><svg viewBox="0 0 24 24" aria-hidden="true"><g fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="9.5"/><path d="m7.5 12.3 3 3 6-6.6"/></g></svg><div><b>STOP in Stage 3 = Finished</b><span>The normal end. Saved as finished.</span></div></div>
            <div class="hp-outcome hp-early"><svg viewBox="0 0 24 24" aria-hidden="true"><g fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round"><circle cx="12" cy="12" r="9.5"/><path d="m9 9 6 6M15 9l-6 6"/></g></svg><div><b>STOP in Stage 1 or 2 = Stopped early</b><span>Can't continue. A new START begins a new cycle.</span></div></div>
          </div>
        </section>

        <section class="hp-sec" id="problems" aria-labelledby="h-problems">
          <h2 id="h-problems">Problems</h2>
          <div class="card hp-problems">
            <div class="hp-prob"><span class="hp-lamps hp-lamps--sm"><i class="blink"></i><i class="blink"></i><i class="blink"></i></span><div><b>All lights blinking, START does nothing</b><span>Maintenance lockdown. Only the supervisor can lift it.</span></div></div>
            <div class="hp-prob"><span class="hp-ico hp-ico--red"><svg viewBox="0 0 24 24" aria-hidden="true"><g fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M14 4v10.5a4 4 0 1 1-4 0V4a2 2 0 1 1 4 0z"/><path d="M12 17.5v-6"/></g></svg></span><div><b>A temperature is red</b><span>That probe reads wrong; the room keeps the last good value. Check the cable and that Tc is in the fish. Tell the supervisor.</span></div></div>
            <div class="hp-prob"><span class="hp-ico"><svg viewBox="0 0 24 24" aria-hidden="true"><path fill="currentColor" d="M8 5.5v13a1 1 0 0 0 1.5.9l10.4-6.5a1 1 0 0 0 0-1.8L9.5 4.6A1 1 0 0 0 8 5.5z"/></svg></span><div><b>START does nothing</b><span>A stage light is on: the room is already running. Press STOP first only if you really need a new cycle.</span></div></div>
            <div class="hp-prob"><span class="hp-ico"><svg viewBox="0 0 24 24" aria-hidden="true"><g fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round"><circle cx="12" cy="12" r="9"/><path d="M12 7v5l3 2"/></g></svg></span><div><b>Stage 2 takes too long</b><span>On <a href="/control">Control</a>, check the ✕ / ✓ boxes. A value that never moves can be a bad probe: call the supervisor.</span></div></div>
            <div class="hp-prob"><span class="hp-ico"><svg viewBox="0 0 24 24" aria-hidden="true"><g fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><path d="M12 3v9"/><path d="M6.4 6.4a8 8 0 1 0 11.2 0"/></g></svg></span><div><b>After a power cut</b><span>The room comes back in the same stage. Don't press START again.</span></div></div>
            <div class="hp-prob"><span class="hp-ico"><svg viewBox="0 0 24 24" aria-hidden="true"><g fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><path d="M2.5 9a14 14 0 0 1 19 0M5.5 12.5a9.5 9.5 0 0 1 13 0M8.6 16a5 5 0 0 1 6.8 0"/><circle cx="12" cy="19.3" r="1" fill="currentColor"/></g></svg></span><div><b>A page says “Offline”</b><span>The room keeps working. Check your phone is on the plant Wi-Fi; the page retries by itself.</span></div></div>
          </div>
        </section>

        <section class="hp-sec" id="pages" aria-labelledby="h-pages">
          <h2 id="h-pages">This website</h2>
          <div class="hp-pages">
            <a class="card linkcard" href="/control"><b>Control</b><span>Stage, temperatures, equipment. Start and stop.</span></a>
            <a class="card linkcard" href="/events"><b>Events</b><span>Who started, moved and stopped the process, and when.</span></a>
            <a class="card linkcard" href="/logs"><b>Logs</b><span>Detailed files on the SD card, to download.</span></a>
            <a class="card linkcard" href="/edit-settings"><b>Settings</b><span>Supervisor: parameters, network, sensor calibration.</span></a>
            <a class="card linkcard" href="/"><b>Device</b><span>Supervisor: firmware update and restart.</span></a>
          </div>
          <p class="hp-note">The <a href="https://ayam-brand-iot.github.io/thawingroom/" target="_blank" rel="noopener">full manual</a> has pictures of every screen. It needs internet, so it may not open on the plant Wi-Fi.</p>
        </section>
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
  NAV_BLOCK("", "", "", "", "class=\"active\" aria-current=\"page\"", "")
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

        <section class="ec-result" id="result" role="status" aria-live="polite" hidden></section>

        <form id="configForm" class="card" action="/update-config" method="POST" novalidate>
          <div id="formContent"></div>
          <hr class="divider" />
          <div class="btn-row">
            <button type="submit" class="btn" id="save-btn" disabled>Save changes</button>
            <button type="button" class="btn btn--ghost" id="discard-btn" hidden>Discard changes</button>
            <span class="ec-dirty" id="dirty-text" aria-live="polite"></span>
          </div>
          <small>Double-check values &mdash; a bad config can affect boot and the running process. Saved values take effect after a restart.</small>
        </form>
      </main>
      <footer class="footer">
        <span>Thawing Room &middot; {{LOCATION}}</span>
        <span>v{{VERSION}} &middot; <span class="tnum" id="clock">--:--:--</span></span>
      </footer>
    </div>

    <dialog class="ct-dialog" id="dlg" aria-labelledby="dlg-title" aria-describedby="dlg-body">
      <form method="dialog" id="dlg-form">
        <h2 id="dlg-title">Restart the controller?</h2>
        <p class="ct-dialog-body" id="dlg-body">It restarts in a few seconds and loads the saved settings. A running process resumes where it was.</p>
        <div class="ct-dialog-actions">
          <button type="button" class="btn btn--ghost" id="dlg-cancel">Cancel</button>
          <button type="submit" class="btn" id="dlg-ok">Restart now</button>
        </div>
      </form>
    </dialog>
    <script>
      const urlParams = new URLSearchParams(window.location.search);
      const fileName = urlParams.get("file");
      document.getElementById("file-name").textContent = fileName ? ("Edit " + fileName) : "Edit configuration";

      const form = document.getElementById("configForm");
      const saveBtn = document.getElementById("save-btn");
      const discardBtn = document.getElementById("discard-btn");
      const result = document.getElementById("result");
      let saving = false;

      function esc(s) { return String(s).replace(/[&<>"']/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[c])); }
      function inputs() { return Array.from(form.querySelectorAll("#formContent input")); }
      function dirtyInputs() { return inputs().filter((el) => el.value !== el.dataset.saved); }

      function refreshDirty() {
        const dirty = dirtyInputs();
        inputs().forEach((el) => el.closest(".field").classList.toggle("is-dirty", el.value !== el.dataset.saved));
        saveBtn.disabled = saving || dirty.length === 0;
        discardBtn.hidden = dirty.length === 0 || saving;
        document.getElementById("dirty-text").textContent = dirty.length
          ? dirty.length + (dirty.length === 1 ? " unsaved change" : " unsaved changes") : "";
      }

      function load() {
        return fetch(fileName, { cache: "no-store", credentials: "same-origin" })
          .then((r) => { if (!r.ok) throw new Error("File not found, check the file name"); return r.json(); })
          .then((data) => {
            const box = document.getElementById("formContent");
            box.innerHTML = "";
            generateFormFields(box, data);
            refreshDirty();
          })
          .catch((err) => {
            document.getElementById("formContent").innerHTML =
              '<div class="empty">Could not load configuration: ' + esc(err.message) + '</div>';
          });
      }
      document.addEventListener("DOMContentLoaded", load);
      form.addEventListener("input", refreshDirty);
      discardBtn.addEventListener("click", () => { inputs().forEach((el) => (el.value = el.dataset.saved)); refreshDirty(); });

      // Guardar sin salir de la página: antes el formulario navegaba a una hoja en blanco
      // que solo decía "Configuration updated successfully".
      form.addEventListener("submit", (e) => {
        e.preventDefault();
        if (saving || !dirtyInputs().length) return;
        saving = true;
        saveBtn.textContent = "Saving…";
        refreshDirty();
        fetch("/update-config", {
          method: "POST", credentials: "same-origin", cache: "no-store",
          headers: { "Content-Type": "application/x-www-form-urlencoded" },
          body: new URLSearchParams(new FormData(form)).toString()
        })
          .then((r) => r.json().catch(() => ({ ok: false, msg: "Unexpected answer from the controller (HTTP " + r.status + ")." })))
          .then((res) => {
            if (res.ok) inputs().forEach((el) => (el.dataset.saved = el.value));
            showResult(res);
          })
          .catch(() => showResult({ ok: false, msg: "No answer from the controller. Your changes may not be saved; reload the page to check." }))
          .finally(() => { saving = false; saveBtn.textContent = "Save changes"; refreshDirty(); });
      });

      function showResult(res) {
        result.className = "ec-result " + (res.ok ? "is-ok" : "is-error");
        let html = '<div class="ec-result-head"><b>' + esc(res.ok ? res.msg : "Not saved") + "</b>";
        if (!res.ok) html += "<span>" + esc(res.msg) + "</span>";
        else if (res.restart) html += "<span>The controller reads this file when it starts, so the new values apply after a restart.</span>";
        html += "</div>";
        if (res.changes && res.changes.length) {
          html += '<ul class="ec-changes">' + res.changes.map((c) => "<li>" + esc(c).replace(/ -&gt; /g, " &rarr; ") + "</li>").join("") + "</ul>";
        }
        if (res.ok && res.restart) {
          html += '<div class="btn-row"><button type="button" class="btn ev-btn-sm" id="restart-btn">Restart now</button>' +
                  '<button type="button" class="btn btn--ghost ev-btn-sm" id="later-btn">Later</button></div>';
        }
        result.innerHTML = html;
        result.hidden = false;
        result.scrollIntoView({ behavior: "smooth", block: "nearest" });
        const rb = document.getElementById("restart-btn"), lb = document.getElementById("later-btn");
        if (rb) rb.addEventListener("click", () => { dlg.showModal(); document.getElementById("dlg-cancel").focus(); });
        if (lb) lb.addEventListener("click", () => { result.hidden = true; });
      }

      // ---- reinicio y espera a que el equipo vuelva ----
      const dlg = document.getElementById("dlg");
      document.getElementById("dlg-cancel").addEventListener("click", () => dlg.close());
      document.getElementById("dlg-form").addEventListener("submit", (e) => {
        e.preventDefault();
        dlg.close();
        result.className = "ec-result is-wait";
        result.innerHTML = '<div class="ec-result-head"><b><span class="ec-spinner" aria-hidden="true"></span>Restarting…</b><span>Waiting for the controller to come back. This usually takes 20 to 40 seconds.</span></div>';
        fetch("/reset", { method: "POST", credentials: "same-origin" }).catch(() => {});
        waitForController().then((back) => {
          result.className = "ec-result " + (back ? "is-ok" : "is-error");
          result.innerHTML = back
            ? '<div class="ec-result-head"><b>Back online</b><span>The controller restarted with the saved settings.</span></div>'
            : '<div class="ec-result-head"><b>The controller has not come back yet</b><span>If you changed Wi-Fi or the IP address, it may now be at a different address.</span></div>';
          if (back) load();
        });
      });
      function waitForController() {
        const t0 = Date.now();
        return new Promise((resolve) => {
          // Esperar a que se caiga antes de dar por bueno el primer "ok".
          setTimeout(function check() {
            fetch("/api/status", { cache: "no-store", credentials: "same-origin" })
              .then((r) => { if (r.ok || r.status === 401) resolve(true); else throw 0; })   // 401 = ya contesta, pide auth de nuevo
              .catch(() => { if (Date.now() - t0 > 90000) resolve(false); else setTimeout(check, 2000); });
          }, 6000);
        });
      }

      window.addEventListener("beforeunload", (e) => {
        if (dirtyInputs().length) { e.preventDefault(); e.returnValue = ""; }
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
            input.dataset.saved = input.value;
            input.autocomplete = "off"; input.spellcheck = false;
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
[hidden]{display:none !important;}   /* .btn y otros fijan display: sin esto, hidden no oculta */
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

/* Events */
.sr-only{position:absolute;width:1px;height:1px;padding:0;margin:-1px;overflow:hidden;clip:rect(0 0 0 0);white-space:nowrap;border:0;}
.chip .dot.is-idle{background:#8d9cab;box-shadow:0 0 0 3px rgba(141,156,171,.2);}
.ev-section{margin-bottom:30px;}
.ev-section-head{display:flex;align-items:center;justify-content:space-between;gap:12px;flex-wrap:wrap;margin-bottom:12px;}
.ev-updated{color:var(--ink-muted);font-size:.78rem;}
.ev-note{color:var(--ink-muted);font-size:.8rem;margin:12px 0 0;max-width:70ch;}
.ev-inline-label{display:inline-flex;}
.ev-select{height:36px;max-width:100%;padding:0 34px 0 12px;border:1px solid var(--border);border-radius:8px;
  background:#fff url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='12' height='12' viewBox='0 0 24 24' fill='none' stroke='%23566779' stroke-width='2.4' stroke-linecap='round' stroke-linejoin='round'%3E%3Cpath d='m6 9 6 6 6-6'/%3E%3C/svg%3E") no-repeat right 11px center;
  -webkit-appearance:none;appearance:none;color:var(--ink);font:inherit;font-size:.86rem;cursor:pointer;
  transition:border-color .15s,box-shadow .15s;}
.ev-select:hover{border-color:var(--border-strong);}
.ev-select:focus-visible{outline:none;border-color:var(--accent);box-shadow:0 0 0 3px var(--accent-soft);}
.ev-select:disabled{opacity:.6;cursor:default;}
.ev-link{border:0;background:none;padding:0;font:inherit;font-size:.84rem;font-weight:600;color:var(--accent);cursor:pointer;}
.ev-link:hover{text-decoration:underline;}
.ev-link:focus-visible{outline:2px solid var(--accent);outline-offset:2px;border-radius:3px;}

.ev-banner{display:flex;align-items:center;gap:10px;padding:10px 14px;margin-bottom:18px;border:1px solid rgba(192,57,43,.32);
  border-radius:9px;background:var(--danger-soft);color:#8f2a1d;font-size:.86rem;font-weight:500;}
.ev-banner[hidden]{display:none;}

/* Cycle card */
.ev-cycle{gap:16px;}
.ev-cycle-head{display:flex;align-items:center;gap:14px;flex-wrap:wrap;}
.ev-cycle-total{font-size:.9rem;font-weight:600;}
.ev-status{display:inline-flex;align-items:center;gap:7px;height:26px;padding:0 11px;border-radius:999px;font-size:.78rem;font-weight:650;}
.ev-status-dot{width:7px;height:7px;border-radius:50%;background:currentColor;}
.ev-status--running{background:var(--accent-soft);color:#0a5cae;}
.ev-status--running .ev-status-dot{animation:evPulse 1.8s var(--ease) infinite;}
.ev-status--finished{background:rgba(31,138,76,.13);color:#17683a;}
.ev-status--stopped{background:var(--surface-2);color:#3f4f60;border:1px solid var(--border);}
.ev-status--interrupted{background:rgba(214,148,0,.17);color:#7a5000;}
@keyframes evPulse{0%{box-shadow:0 0 0 0 rgba(11,107,203,.45);}70%{box-shadow:0 0 0 6px rgba(11,107,203,0);}100%{box-shadow:0 0 0 0 rgba(11,107,203,0);}}

.ev-bar{display:flex;gap:3px;height:32px;}
.ev-bar-seg{display:flex;align-items:center;min-width:6px;padding:0 10px;border-radius:6px;overflow:hidden;
  font-size:.76rem;font-weight:650;white-space:nowrap;}
.ev-bar-seg span{overflow:hidden;text-overflow:ellipsis;}
.ev-bar-seg--1{background:#b4d2f1;color:#12304e;}
.ev-bar-seg--2{background:var(--accent);color:#fff;}
.ev-bar-seg--3{background:var(--success);color:#fff;}
.ev-bar-scale{display:flex;justify-content:space-between;margin-top:-8px;color:var(--ink-muted);font-size:.75rem;}
.ev-legend{display:flex;flex-wrap:wrap;gap:6px 20px;font-size:.82rem;color:var(--ink-muted);}
.ev-legend-item{display:inline-flex;align-items:center;gap:7px;}
.ev-legend-item b{color:var(--ink);font-weight:600;}
.ev-swatch{display:inline-block;width:10px;height:10px;border-radius:3px;padding:0;min-width:0;}

.ev-steps{list-style:none;margin:0;padding:0;border-top:1px solid var(--border);}
.ev-step{display:grid;grid-template-columns:86px minmax(0,1fr) auto 64px;align-items:center;gap:12px;padding:10px 0;border-bottom:1px solid var(--border);}
.ev-step:last-child{border-bottom:0;padding-bottom:0;}
.ev-step-time{font-size:.84rem;font-weight:600;}
.ev-step-text{font-size:.9rem;overflow-wrap:anywhere;}
.ev-step-elapsed{text-align:right;color:var(--ink-muted);font-size:.8rem;}

/* Filters */
.ev-toolbar{display:flex;flex-direction:column;gap:10px;margin-bottom:14px;}
.ev-toolbar-row{display:flex;flex-wrap:wrap;gap:8px;align-items:center;}
.ev-filters{display:flex;flex-wrap:wrap;gap:6px;}
.ev-filter{display:inline-flex;align-items:center;gap:7px;height:34px;padding:0 12px;border:1px solid var(--border);
  border-radius:999px;background:var(--surface);color:var(--ink);font:inherit;font-size:.84rem;font-weight:500;cursor:pointer;
  transition:background .15s,border-color .15s,color .15s,transform .12s var(--ease);}
.ev-filter:hover{border-color:var(--border-strong);background:var(--surface-2);}
.ev-filter:active{transform:scale(.97);}
.ev-filter:focus-visible{outline:2px solid var(--accent);outline-offset:2px;}
.ev-filter .n{color:var(--ink-muted);font-size:.78rem;font-variant-numeric:tabular-nums;}
.ev-filter--0 svg{color:#0a5cae;} .ev-filter--1 svg{color:#5b3fa6;} .ev-filter--2 svg{color:#9a6700;}
.ev-filter--3 svg{color:#a5311f;} .ev-filter--4 svg{color:#4a5a6b;}
.ev-filter[aria-pressed="true"]{background:var(--ink);border-color:var(--ink);color:#fff;}
.ev-filter[aria-pressed="true"] svg{color:#fff;}
.ev-filter[aria-pressed="true"] .n{color:rgba(255,255,255,.72);}
.ev-search{position:relative;display:inline-flex;align-items:center;flex:1 1 200px;min-width:0;}
.ev-search svg{position:absolute;left:11px;color:var(--ink-muted);pointer-events:none;}
.ev-search input{width:100%;height:36px;padding:0 12px 0 34px;border:1px solid var(--border);border-radius:8px;background:#fff;
  color:var(--ink);font:inherit;font-size:.86rem;transition:border-color .15s,box-shadow .15s;}
.ev-search input::placeholder{color:#66768a;}
.ev-search input:hover{border-color:var(--border-strong);}
.ev-search input:focus{outline:none;border-color:var(--accent);box-shadow:0 0 0 3px var(--accent-soft);}
.ev-btn-sm{height:36px;padding:0 13px;font-size:.84rem;}

/* Event list */
.ev-list{background:var(--surface);border:1px solid var(--border);border-radius:var(--radius);box-shadow:var(--shadow-sm);overflow:hidden;}
.ev-list > .empty{border:0;border-radius:0;}
.ev-day{padding:8px 16px;background:var(--surface-2);border-bottom:1px solid var(--border);
  color:#3f4f60;font-size:.78rem;font-weight:650;}
.ev-row{display:grid;grid-template-columns:76px 112px minmax(0,1fr) auto;align-items:center;gap:14px;
  padding:10px 16px;border-bottom:1px solid var(--border);}
.ev-row:last-child{border-bottom:0;}
.ev-time{font-family:ui-monospace,SFMono-Regular,Menlo,Consolas,monospace;font-size:.8rem;color:var(--ink);white-space:nowrap;}
.ev-time--warn{color:#7a5000;cursor:help;}
.ev-type{display:inline-flex;align-items:center;gap:6px;width:max-content;height:24px;padding:0 9px 0 7px;border-radius:6px;
  font-size:.76rem;font-weight:650;}
.ev-type--0{background:var(--accent-soft);color:#0a5cae;}
.ev-type--1{background:rgba(107,79,187,.12);color:#5b3fa6;}
.ev-type--2{background:rgba(214,148,0,.17);color:#7a5000;}
.ev-type--3{background:var(--danger-soft);color:#a5311f;}
.ev-type--4{background:#e6ebf0;color:#44525f;}
.ev-msg{font-size:.9rem;overflow-wrap:anywhere;}
.ev-meta{margin-left:8px;color:var(--ink-muted);font-size:.76rem;white-space:nowrap;}
.ev-origin{display:inline-flex;align-items:center;gap:5px;height:24px;padding:0 9px;border:1px solid var(--border);
  border-radius:999px;color:var(--ink-muted);font-size:.76rem;font-weight:500;white-space:nowrap;cursor:default;}
.ev-origin--human{color:var(--ink);border-color:var(--border-strong);font-weight:600;}
.ev-more{justify-content:center;margin-top:14px;}
.ev-more[hidden]{display:none;}
.ev-row.is-new{animation:evIn .22s var(--ease) both,evGlow 1.8s ease-out;}
@keyframes evIn{from{opacity:0;transform:translateY(-4px);}}
@keyframes evGlow{from{background:rgba(11,107,203,.10);}}

/* Loading skeleton */
.ev-skel{height:12px;border-radius:6px;background:linear-gradient(90deg,#e7edf3 0%,#f3f6f9 50%,#e7edf3 100%);
  background-size:200% 100%;animation:evShimmer 1.3s linear infinite;}
.ev-skel--bar{height:32px;}
.ev-row--skel{grid-template-columns:76px 112px 1fr;}
@keyframes evShimmer{from{background-position:100% 0;}to{background-position:-100% 0;}}

@media (pointer:coarse){
  .ev-filter,.ev-select,.ev-search input,.ev-btn-sm{height:44px;}
}
@media (max-width:760px){
  .ev-row{grid-template-columns:auto auto 1fr;grid-template-areas:"time type origin" "msg msg msg";row-gap:7px;}
  .ev-row .ev-time{grid-area:time;} .ev-row .ev-type{grid-area:type;}
  .ev-row .ev-origin{grid-area:origin;justify-self:end;} .ev-row .ev-msg{grid-area:msg;}
  .ev-row--skel{grid-template-columns:1fr 1fr;grid-template-areas:none;}
  .ev-step{grid-template-columns:62px minmax(0,1fr);grid-template-areas:"time text" ". meta";row-gap:6px;}
  .ev-step-time{grid-area:time;} .ev-step-text{grid-area:text;}
  .ev-step .ev-origin{grid-area:meta;justify-self:start;}
  .ev-step-elapsed{display:none;}
  .ev-section-head .ev-inline-label,.ev-section-head .ev-select{width:100%;}
}
@media (prefers-reduced-motion:reduce){
  .ev-row.is-new{animation:evGlow 1.8s ease-out;}
  .ev-skel,.ev-status--running .ev-status-dot{animation:none;}
}

/* Control */
.ct-notices{display:flex;flex-direction:column;gap:8px;margin-bottom:18px;}
.ct-notices:empty{display:none;}
.ct-notice{display:flex;align-items:flex-start;gap:10px;padding:10px 14px;border:1px solid rgba(214,148,0,.38);
  border-radius:9px;background:rgba(214,148,0,.12);color:#6b4600;font-size:.86rem;font-weight:500;}
.ct-notice svg{flex:none;margin-top:1px;}
.ct-notice--danger{border-color:rgba(192,57,43,.32);background:var(--danger-soft);color:#8f2a1d;}

.ct-hero{gap:18px;margin-bottom:18px;}
.ct-stage{font-size:1.5rem;font-weight:700;line-height:1.2;letter-spacing:-.01em;text-wrap:balance;}
.ct-stage > span{margin-left:6px;color:var(--ink-muted);font-size:1.05rem;font-weight:500;letter-spacing:0;}
.ct-stage-detail{margin:5px 0 0;color:var(--ink-muted);font-size:.9rem;}
.ct-track{list-style:none;margin:0;padding:0;display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:6px;}
.ct-track li{display:flex;flex-direction:column;min-width:0;color:var(--ink-muted);font-size:.78rem;}
.ct-track li b{color:var(--ink-muted);font-size:.82rem;font-weight:600;}
.ct-track-bar{height:8px;margin-bottom:7px;border:1px solid var(--border);border-radius:999px;background:var(--surface-2);
  transition:background .3s var(--ease),border-color .3s var(--ease);}
.ct-track li.is-past .ct-track-bar{background:#cfdcea;border-color:#c3d3e4;}
.ct-track li.is-current b{color:var(--ink);}
.ct-track li.is-current[data-s="1"] .ct-track-bar{background:#b4d2f1;border-color:#9cc3ec;}
.ct-track li.is-current[data-s="2"] .ct-track-bar{background:var(--accent);border-color:var(--accent);}
.ct-track li.is-current[data-s="3"] .ct-track-bar{background:var(--success);border-color:var(--success);}
.ct-next{display:flex;flex-wrap:wrap;align-items:center;gap:8px 10px;padding:11px 14px;border:1px solid var(--border);
  border-radius:9px;background:var(--surface-2);font-size:.9rem;}
.ct-next:empty{display:none;}
.ct-next > svg{flex:none;color:var(--accent);}
.ct-cond{display:inline-flex;align-items:center;gap:6px;height:28px;padding:0 11px;border:1px solid var(--border);
  border-radius:999px;background:var(--surface);color:#3f4f60;font-size:.82rem;font-weight:600;}
.ct-cond.is-met{background:rgba(31,138,76,.13);border-color:rgba(31,138,76,.32);color:#17683a;}
.ct-actions{display:flex;flex-wrap:wrap;gap:10px;}
.ct-actions .btn{height:44px;padding:0 18px;}
.ct-actions .btn--danger{margin-left:auto;}
.ct-hint{margin:-6px 0 0;color:var(--ink-muted);font-size:.8rem;}
.ct-hint:empty{display:none;}

.ct-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(300px,1fr));gap:18px;margin-bottom:18px;}
.ct-panel{gap:2px;padding:18px 20px;}
.ct-panel-head{display:flex;align-items:baseline;justify-content:space-between;gap:12px;margin-bottom:6px;}
.ct-muted{color:var(--ink-muted);font-size:.8rem;}

.ct-temp{display:grid;grid-template-columns:minmax(0,1fr) auto;grid-template-areas:"name val" "meta meta";
  gap:3px 12px;padding:12px 0;border-top:1px solid var(--border);}
.ct-temp:first-child{border-top:0;padding-top:4px;}
.ct-temp-name{grid-area:name;display:flex;flex-direction:column;}
.ct-temp-name b{font-size:.95rem;font-weight:650;}
.ct-temp-name span{color:var(--ink-muted);font-size:.8rem;}
.ct-temp-val{grid-area:val;align-self:center;font-size:1.75rem;font-weight:650;line-height:1.1;letter-spacing:-.01em;text-align:right;}
.ct-temp-val small{margin-left:2px;color:var(--ink-muted);font-size:.85rem;font-weight:500;letter-spacing:0;}
.ct-temp-meta{grid-area:meta;display:flex;flex-wrap:wrap;align-items:center;gap:4px 8px;color:var(--ink-muted);font-size:.78rem;}
.ct-temp.is-fault .ct-temp-val{color:#a5311f;}
.ct-badge{display:inline-flex;align-items:center;gap:5px;padding:2px 8px;border-radius:6px;background:var(--danger-soft);
  color:#a5311f;font-weight:600;}

.ct-eq{list-style:none;margin:0;padding:0;}
.ct-eq-item{display:grid;grid-template-columns:38px minmax(0,1fr) auto;align-items:center;gap:12px;padding:11px 0;
  border-top:1px solid var(--border);}
.ct-eq-item:first-child{border-top:0;padding-top:4px;}
.ct-eq-icon{display:grid;place-items:center;width:38px;height:38px;border-radius:9px;background:var(--surface-2);
  color:var(--ink-muted);transition:background .2s,color .2s;}
.ct-eq-item.is-on .ct-eq-icon{background:var(--accent-soft);color:var(--accent);}
.ct-eq-text{display:flex;flex-direction:column;min-width:0;}
.ct-eq-text b{font-size:.92rem;font-weight:600;}
.ct-eq-text > span{color:var(--ink-muted);font-size:.8rem;}
.ct-state{display:inline-flex;align-items:center;gap:6px;height:26px;padding:0 10px;border:1px solid var(--border);
  border-radius:999px;background:var(--surface-2);color:#3f4f60;font-size:.78rem;font-weight:650;white-space:nowrap;}
.ct-state::before{content:"";width:7px;height:7px;border-radius:50%;background:#8d9cab;}
.ct-state.is-on{background:rgba(31,138,76,.13);border-color:rgba(31,138,76,.32);color:#17683a;}
.ct-state.is-on::before{background:var(--success);}
.ct-state.is-warn{background:rgba(214,148,0,.14);border-color:rgba(214,148,0,.4);color:#6b4600;}
.ct-state.is-warn::before{background:#d69400;}
.ct-meter{display:block;max-width:200px;height:6px;margin-top:6px;border:1px solid var(--border);border-radius:999px;
  background:var(--surface-2);overflow:hidden;}
.ct-meter > i{display:block;height:100%;background:var(--accent);transform-origin:left center;
  transition:transform .4s var(--ease);}
.ct-spin{animation:ctSpin 1.1s linear infinite;}
.ct-spin--rev{animation-direction:reverse;}
@keyframes ctSpin{to{transform:rotate(360deg);}}

.ct-sys{margin-bottom:30px;}
.ct-kv{display:grid;grid-template-columns:repeat(auto-fit,minmax(140px,1fr));gap:12px 20px;margin:0;}
.ct-kv > div{display:flex;flex-direction:column;gap:2px;min-width:0;}
.ct-kv dt{color:var(--ink-muted);font-size:.78rem;}
.ct-kv dd{display:flex;align-items:center;gap:7px;margin:0;font-size:.9rem;font-weight:600;}
.ct-kv dd i{flex:none;width:7px;height:7px;border-radius:50%;background:#8d9cab;}
.ct-kv dd i.is-ok{background:var(--success);}
.ct-kv dd i.is-warn{background:#d69400;}
.ct-kv dd i.is-off{background:var(--danger);}

.chip .dot.is-lock{background:#d69400;box-shadow:0 0 0 3px rgba(214,148,0,.22);animation:ctBlink 1s steps(1,end) infinite;}
.ct-lock{display:flex;align-items:flex-start;gap:14px;margin-bottom:18px;padding:18px 20px;border:1.5px solid #d69400;
  border-radius:var(--radius);color:#4d3300;box-shadow:var(--shadow-sm);
  background:repeating-linear-gradient(135deg,#fff6df 0 14px,#fdefc8 14px 28px);}
.ct-lock[hidden]{display:none;}
.ct-lock-icon{display:grid;place-items:center;flex:none;width:42px;height:42px;border-radius:10px;background:#b97f00;color:#fff;}
.ct-lock-text{flex:1;min-width:0;}
.ct-lock-text h2{font-size:1.1rem;font-weight:700;color:#3d2800;}
.ct-lock-text p{margin:3px 0 0;font-size:.88rem;}
.ct-lock-meta{color:#6b4600;}
.ct-lock .btn{flex:none;align-self:center;height:44px;border-color:#c48700;}
.ct-maint{display:flex;align-items:center;justify-content:space-between;gap:12px;flex-wrap:wrap;margin-top:14px;padding-top:14px;
  border-top:1px solid var(--border);}
.ct-maint > div{display:flex;flex-direction:column;min-width:0;}
.ct-maint b{font-size:.9rem;font-weight:600;}
.ct-maint span{color:var(--ink-muted);font-size:.8rem;}
.ct-maint .ct-maint-on{color:#7a5000;font-size:inherit;}
@keyframes ctBlink{50%{opacity:.25;}}
.ct-dialog{width:min(440px,calc(100vw - 32px));padding:0;border:1px solid var(--border);border-radius:12px;
  background:var(--surface);color:var(--ink);box-shadow:0 24px 60px rgba(16,32,43,.28);}
.ct-dialog[open]{animation:ctIn .2s var(--ease);}
.ct-dialog::backdrop{background:rgba(16,32,43,.45);}
.ct-dialog form{display:flex;flex-direction:column;gap:12px;padding:22px;}
.ct-dialog h2{font-size:1.1rem;font-weight:650;}
.ct-dialog-body{margin:0;color:var(--ink-muted);font-size:.9rem;}
.ct-dialog-body b{color:var(--ink);}
.ct-dfields{display:grid;grid-template-columns:1fr 1fr;gap:0 12px;}
.ct-dfields[hidden]{display:none;}
.ct-dfields .field{margin-bottom:6px;}
.ct-preview{grid-column:1/-1;margin:0;font-size:.86rem;font-weight:600;}
.ct-preview.is-bad{color:#a5311f;}
.ct-dialog-error{margin:0;padding:9px 12px;border-radius:8px;background:var(--danger-soft);color:#8f2a1d;font-size:.86rem;font-weight:500;}
.ct-dialog-error[hidden]{display:none;}
.ct-dialog-actions{display:flex;justify-content:flex-end;gap:10px;margin-top:6px;}
.ct-spinner{width:14px;height:14px;border:2px solid rgba(255,255,255,.45);border-top-color:#fff;border-radius:50%;
  animation:ctSpin .7s linear infinite;}
@keyframes ctIn{from{opacity:0;transform:translateY(6px) scale(.98);}}

.ct-toast{position:fixed;left:50%;bottom:56px;z-index:30;display:flex;align-items:center;gap:10px;
  max-width:calc(100vw - 32px);padding:11px 16px;border-radius:10px;background:var(--ink);color:#fff;
  font-size:.88rem;font-weight:500;box-shadow:var(--shadow-md);transform:translateX(-50%);cursor:pointer;
  animation:ctToast .22s var(--ease);}
.ct-toast[hidden]{display:none;}
.ct-toast svg{flex:none;}
.ct-toast--error{background:#8f2a1d;}
.ct-toast--warn{background:#6b4600;}
@keyframes ctToast{from{opacity:0;transform:translate(-50%,8px);}}

@media (pointer:coarse){
  .ct-dialog .btn{height:44px;}
}
@media (max-width:760px){
  .ct-stage{font-size:1.3rem;}
  .ct-stage > span{display:block;margin:2px 0 0;}
  .ct-actions .btn{flex:1 1 auto;}
  .ct-actions .btn--danger{flex-basis:100%;margin-left:0;}
  .ct-panel{padding:16px;}
  .ct-dfields{grid-template-columns:1fr;}
  .ct-lock{flex-wrap:wrap;}
  .ct-lock .btn{flex:1 1 100%;}
}
@media (prefers-reduced-motion:reduce){
  .ct-spin,.ct-dialog[open],.ct-toast,.ct-spinner,.chip .dot.is-lock{animation:none;}
}

/* Calibration */
.cb-gate{align-items:center;margin-bottom:18px;}
.cb-gate[hidden]{display:none;}
.cb-gate span{flex:1;min-width:0;}
.cb-gate .btn{flex:none;}
.cb-resume{flex-direction:row;align-items:center;justify-content:space-between;flex-wrap:wrap;gap:12px;margin-bottom:18px;padding:14px 18px;
  border-color:var(--accent);background:var(--accent-soft);font-size:.9rem;}
.cb-resume[hidden]{display:none;}
.cb-probes{gap:0;padding:6px 20px;}
.cb-probe{display:grid;grid-template-columns:minmax(110px,1fr) minmax(150px,1.2fr) 110px auto;align-items:center;gap:14px;padding:14px 0;
  border-top:1px solid var(--border);}
.cb-probe:first-child{border-top:0;}
.cb-probe.is-active .cb-probe-name b{color:var(--accent);}
.cb-probe-name{display:flex;flex-direction:column;}
.cb-probe-name b{font-size:.98rem;font-weight:650;}
.cb-probe-name span{color:var(--ink-muted);font-size:.8rem;}
.cb-badge{display:inline-flex;align-items:center;height:24px;padding:0 9px;border:1px solid var(--border);border-radius:6px;
  background:var(--surface-2);color:#3f4f60;font-size:.76rem;font-weight:600;white-space:nowrap;}
.cb-badge.is-cal{background:rgba(31,138,76,.12);border-color:rgba(31,138,76,.3);color:#17683a;}
.cb-probe-read{display:flex;flex-direction:column;align-items:flex-end;font-size:1.25rem;font-weight:650;line-height:1.15;}
.cb-probe-read small{margin-left:2px;color:var(--ink-muted);font-size:.75rem;font-weight:500;}
.cb-probe-read .cb-c{color:var(--ink-muted);font-size:.75rem;font-weight:500;}
.cb-probe-actions{display:flex;flex-wrap:wrap;justify-content:flex-end;gap:6px;}

.cb-wizard{gap:16px;margin-top:18px;}
.cb-wizard[hidden]{display:none;}
.cb-wz-head{display:flex;align-items:flex-start;justify-content:space-between;gap:12px;}
.cb-wz-head h2{font-size:1.15rem;font-weight:650;}
.cb-wz-head p{margin:2px 0 0;}
.cb-steps{list-style:none;margin:0;padding:0;display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:6px;}
.cb-steps li{display:flex;align-items:center;gap:8px;padding:8px 10px;border:1px solid var(--border);border-radius:8px;
  background:var(--surface-2);color:var(--ink-muted);font-size:.82rem;font-weight:500;min-width:0;}
.cb-steps li b{display:grid;place-items:center;flex:none;width:22px;height:22px;border-radius:50%;background:var(--surface);
  border:1px solid var(--border-strong);font-size:.75rem;color:var(--ink-muted);}
.cb-steps li span{overflow:hidden;text-overflow:ellipsis;white-space:nowrap;}
.cb-steps li.is-current{border-color:var(--accent);background:var(--accent-soft);color:var(--ink);font-weight:600;}
.cb-steps li.is-current b{background:var(--accent);border-color:var(--accent);color:#fff;}
.cb-steps li.is-done b{background:var(--success);border-color:var(--success);color:#fff;}
.cb-instr{margin:0;font-size:.95rem;max-width:68ch;}
.cb-live{display:grid;grid-template-columns:1fr auto;gap:10px 18px;margin-top:12px;padding:16px 18px;border:1px solid var(--border);
  border-radius:10px;background:var(--surface-2);}
.cb-live-main,.cb-live-side{display:flex;flex-direction:column;}
.cb-live-side{align-items:flex-end;text-align:right;}
.cb-live-side .tnum{font-size:1.15rem;font-weight:650;}
.cb-counts{font-size:2.25rem;font-weight:700;line-height:1.05;letter-spacing:-.02em;}
.cb-stab{grid-column:1/-1;display:flex;align-items:center;gap:10px;font-size:.84rem;color:#6b4600;font-weight:500;}
.cb-stab.is-ok{color:#17683a;}
.cb-stab-bar{flex:none;width:120px;height:6px;border-radius:999px;background:var(--surface);border:1px solid var(--border);overflow:hidden;}
.cb-stab-bar i{display:block;height:100%;background:#d69400;transform-origin:left center;transform:scaleX(0);transition:transform .4s var(--ease),background .3s;}
.cb-stab.is-ok .cb-stab-bar i{background:var(--success);}
.cb-ref{display:flex;align-items:flex-end;gap:12px;flex-wrap:wrap;margin-top:14px;}
.cb-ref .field{margin:0;flex:0 1 240px;}
.cb-ref .btn{height:44px;}
.cb-ref-input{position:relative;display:flex;align-items:center;}
.cb-ref-input input{height:44px;padding-right:38px;font-size:1.05rem;font-variant-numeric:tabular-nums;}
.cb-ref-input span{position:absolute;right:13px;color:var(--ink-muted);font-size:.9rem;pointer-events:none;}
#btn-capture-anyway{margin-top:8px;}
#btn-capture-anyway[hidden]{display:none;}
#pt-error,#rv-error{margin-top:10px;}

.cb-captured{list-style:none;margin:0;padding:0;display:flex;flex-wrap:wrap;gap:8px;}
.cb-captured:empty{display:none;}
.cb-captured li{display:inline-flex;align-items:center;gap:10px;height:34px;padding:0 12px;border:1px solid rgba(31,138,76,.3);
  border-radius:999px;background:rgba(31,138,76,.1);font-size:.84rem;}
.cb-captured li b{color:#17683a;font-weight:650;}

.cb-chart{position:relative;height:260px;padding:0 0 0 0;border:1px solid var(--border);border-radius:10px;background:var(--surface);}
.cb-chart svg{display:block;width:100%;height:100%;}
.cb-grid{stroke:var(--border);stroke-width:1;}
.cb-ax{fill:var(--ink-muted);font-size:11px;}
.cb-band{fill:rgba(31,138,76,.12);}
.cb-old{fill:none;stroke:#8d9cab;stroke-width:2;stroke-dasharray:5 5;}
.cb-new{fill:none;stroke:var(--accent);stroke-width:2.5;}
.cb-pt{fill:var(--accent);stroke:#fff;stroke-width:2;}
.cb-ax-x,.cb-ax-y{position:absolute;color:var(--ink-muted);font-size:.72rem;}
.cb-ax-x{right:12px;bottom:-20px;}
.cb-ax-y{left:8px;top:4px;}
.cb-legend{display:flex;flex-wrap:wrap;gap:6px 18px;margin-top:22px;color:var(--ink-muted);font-size:.8rem;}
.cb-legend span{display:inline-flex;align-items:center;gap:7px;}
.cb-key{display:inline-block;width:18px;height:0;border-top:2.5px solid var(--accent);}
.cb-key--old{border-top:2px dashed #8d9cab;}
.cb-key--band{height:10px;border:0;background:rgba(31,138,76,.2);border-radius:2px;}
.cb-table{width:100%;border-collapse:collapse;margin-top:14px;font-size:.88rem;}
.cb-table th{text-align:left;padding:8px 10px;border-bottom:1px solid var(--border);color:var(--ink-muted);font-size:.76rem;font-weight:600;}
.cb-table td{padding:9px 10px;border-bottom:1px solid var(--border);}
.cb-table th:nth-child(n+2),.cb-table td:nth-child(n+2){text-align:right;}
.cb-big{color:#7a5000;font-weight:650;}
.cb-summary{margin:12px 0 0;font-size:.9rem;max-width:70ch;}
.cb-summary:empty{display:none;}

@media (max-width:760px){
  .cb-probe{grid-template-columns:1fr auto;grid-template-areas:"name read" "status status" "actions actions";gap:8px 12px;}
  .cb-probe-name{grid-area:name;} .cb-probe-read{grid-area:read;}
  .cb-probe-status{grid-area:status;} .cb-probe-actions{grid-area:actions;justify-content:flex-start;}
  .cb-steps li span{display:none;}
  .cb-steps li{justify-content:center;}
  .cb-live{grid-template-columns:1fr;}
  .cb-live-side{align-items:flex-start;text-align:left;}
  .cb-ref .field{flex:1 1 100%;}
  .cb-ref .btn{flex:1 1 100%;}
  .cb-chart{height:200px;}
  .cb-table{font-size:.8rem;}
  .cb-table th,.cb-table td{padding:7px 5px;}
}

/* Edit config */
.ec-result{display:flex;flex-direction:column;gap:10px;margin-bottom:18px;padding:14px 18px;border:1px solid var(--border);
  border-radius:var(--radius);background:var(--surface);box-shadow:var(--shadow-sm);font-size:.9rem;}
.ec-result[hidden]{display:none;}
.ec-result.is-ok{border-color:rgba(31,138,76,.35);background:rgba(31,138,76,.08);}
.ec-result.is-error{border-color:rgba(192,57,43,.32);background:var(--danger-soft);color:#8f2a1d;}
.ec-result.is-wait{border-color:rgba(11,107,203,.3);background:var(--accent-soft);}
.ec-result-head{display:flex;flex-direction:column;gap:2px;}
.ec-result-head b{display:inline-flex;align-items:center;gap:8px;font-weight:650;}
.ec-result-head span{color:var(--ink-muted);}
.ec-result.is-error .ec-result-head span{color:#8f2a1d;}
.ec-changes{margin:0;padding:0 0 0 18px;font-family:ui-monospace,SFMono-Regular,Menlo,Consolas,monospace;font-size:.8rem;}
.ec-changes li{margin:2px 0;overflow-wrap:anywhere;}
.ec-spinner{width:14px;height:14px;border:2px solid rgba(11,107,203,.3);border-top-color:var(--accent);border-radius:50%;
  animation:ctSpin .7s linear infinite;}
.field.is-dirty label{color:var(--accent);}
.field.is-dirty label::after{content:" \2022  edited";font-weight:600;}
.field.is-dirty input{border-color:var(--accent);background:#f6faff;}
.ec-dirty{color:var(--ink-muted);font-size:.84rem;}
@media (prefers-reduced-motion:reduce){ .ec-spinner{animation:none;} }
/* Device page */
.dv-msg{display:flex;align-items:center;flex-wrap:wrap;gap:8px;padding:9px 12px;border-radius:8px;font-size:.86rem;font-weight:500;}
.dv-msg[hidden]{display:none;}
.dv-msg.is-ok{background:rgba(31,138,76,.1);color:#17683a;}
.dv-msg.is-error{background:var(--danger-soft);color:#8f2a1d;}
.dv-msg.is-wait{background:var(--accent-soft);color:#0a5cae;}
/* Help */
html{scroll-behavior:smooth;}
.hp-jump{display:flex;flex-wrap:wrap;gap:6px;margin:-6px 0 26px;}
.hp-jump a{display:inline-flex;align-items:center;height:32px;padding:0 12px;border:1px solid var(--border);border-radius:999px;
  background:var(--surface);color:var(--ink);font-size:.84rem;font-weight:500;}
.hp-jump a:hover{border-color:var(--accent);color:var(--accent);}
.hp-sec{display:flex;flex-direction:column;gap:14px;margin-bottom:34px;scroll-margin-top:16px;}
.hp-sec h2{font-size:1.15rem;font-weight:650;}
.hp-note{margin:0;max-width:70ch;color:var(--ink-muted);font-size:.9rem;}
.hp-note--warn{padding:10px 14px;border-radius:9px;background:var(--danger-soft);color:#8f2a1d;font-weight:500;}
.hp-choose{display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));gap:12px;}
.hp-choice{display:flex;flex-direction:column;gap:8px;padding:16px 18px;background:var(--surface);border:1px solid var(--border);
  border-top:5px solid var(--c);border-radius:var(--radius);box-shadow:var(--shadow-sm);font-size:.9rem;}
.hp-go{--c:var(--success);} .hp-later{--c:var(--accent);} .hp-halt{--c:var(--danger);}
.hp-q{color:var(--ink-muted);} .hp-q b{color:var(--ink);}
.hp-press{display:flex;align-items:center;gap:8px;color:var(--c);font-size:1.45rem;font-weight:750;letter-spacing:.02em;}
.hp-press svg{width:28px;height:28px;flex:none;}
.hp-lights{gap:0;padding:6px 20px;}
.hp-light{display:grid;grid-template-columns:92px 120px 1fr;align-items:center;gap:14px;padding:11px 0;border-top:1px solid var(--border);font-size:.9rem;}
.hp-light:first-child{border-top:0;}
.hp-light span:last-child{color:var(--ink-muted);}
.hp-light--lock b{color:#7a5000;}
.hp-lamps{display:inline-flex;gap:6px;padding:6px 9px;background:#2a3842;border-radius:999px;justify-self:start;}
.hp-lamps i{width:16px;height:16px;border-radius:50%;background:#4a5964;border:2px solid #11191f;}
.hp-lamps i.on{background:#ffcf4a;box-shadow:0 0 8px #ffcf4a;}
.hp-lamps i.blink{animation:hpBlink 1s steps(1,end) infinite;}
.hp-lamps--sm i{width:12px;height:12px;}
@keyframes hpBlink{0%{background:#ffcf4a;box-shadow:0 0 8px #ffcf4a;}50%{background:#4a5964;box-shadow:none;}}
.hp-stages{display:grid;grid-template-columns:repeat(auto-fit,minmax(210px,1fr));gap:12px;}
.hp-stage{gap:6px;padding:16px 18px;font-size:.88rem;}
.hp-stage > span{color:var(--ink-muted);}
.hp-stage b{font-size:1.1rem;}
.hp-badge{display:grid;place-items:center;width:36px;height:36px;border-radius:9px;background:var(--c);color:#fff !important;font-weight:750;font-size:1.1rem;}
.hp-s1{--c:#6d9fd0;} .hp-s2{--c:var(--accent);} .hp-s3{--c:var(--success);}
.hp-stage .hp-ends{margin-top:4px;padding:9px 11px;border:1.5px dashed var(--border);border-radius:8px;color:var(--ink);}
.hp-probes{display:flex;flex-wrap:wrap;gap:8px;}
.hp-probes span{padding:5px 11px;border-radius:999px;background:var(--surface);border:1px solid var(--border);font-size:.84rem;color:var(--ink-muted);}
.hp-probes b{color:var(--ink);}
.hp-outcomes{display:grid;grid-template-columns:repeat(auto-fit,minmax(260px,1fr));gap:12px;}
.hp-outcome{display:flex;align-items:flex-start;gap:12px;padding:14px 16px;border-radius:var(--radius);font-size:.9rem;}
.hp-outcome svg{width:30px;height:30px;flex:none;}
.hp-outcome b{display:block;font-size:1rem;}
.hp-fin{background:rgba(31,138,76,.1);} .hp-fin svg,.hp-fin b{color:#17683a;}
.hp-early{background:var(--danger-soft);} .hp-early svg,.hp-early b{color:#a5311f;}
.hp-outcome span{color:var(--ink-muted);}
.hp-problems{gap:0;padding:6px 20px;}
.hp-prob{display:grid;grid-template-columns:72px 1fr;align-items:center;gap:14px;padding:12px 0;border-top:1px solid var(--border);font-size:.9rem;}
.hp-prob:first-child{border-top:0;}
.hp-prob b{display:block;}
.hp-prob span{color:var(--ink-muted);}
.hp-prob .hp-lamps{justify-self:start;}
.hp-ico{display:grid;place-items:center;width:40px;height:40px;border-radius:10px;background:var(--surface-2);color:var(--ink-muted);}
.hp-ico svg{width:22px;height:22px;}
.hp-ico--red{background:var(--danger-soft);color:var(--danger);}
.hp-pages{display:grid;grid-template-columns:repeat(auto-fit,minmax(140px,1fr));gap:10px;}
.hp-pages .card{gap:4px;padding:14px 16px;font-size:.84rem;}
.hp-pages .card b{font-size:.95rem;}
.hp-pages .card span{color:var(--ink-muted);}
@media (max-width:760px){
  .hp-light{grid-template-columns:auto 1fr;grid-template-areas:"lamps name" "text text";gap:6px 12px;}
  .hp-light .hp-lamps{grid-area:lamps;} .hp-light b{grid-area:name;} .hp-light span:last-child{grid-area:text;}
  .hp-prob{grid-template-columns:auto 1fr;}
}
@media (prefers-reduced-motion:reduce){ .hp-lamps i.blink{animation:none;background:#ffcf4a;} html{scroll-behavior:auto;} }
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
