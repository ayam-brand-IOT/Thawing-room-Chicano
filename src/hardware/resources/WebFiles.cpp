

// ============================================================================
//  Web UI (served from the ESP32). Vanilla HTML/CSS/JS, no CDNs, no frameworks
//  so it works fully offline on the LAN / AP portal. Shared look-and-feel lives
//  in STYLE_CSS; pages keep the backend contract intact:
//    placeholders {{LOCATION}} {{VERSION}} //{{LOGS}}, form actions /update,
//    /replace-config, /reset, /update-config, and the element ids used by JS.
// ============================================================================

// Reusable bits kept as macros so every page shares the exact same chrome.
#define NAV_BLOCK(home,events,logs,cfg) \
"    <aside class=\"sidebar\">\n" \
"      <div class=\"brand\"><svg width=\"20\" height=\"20\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\"><path d=\"M12 2v20M3 12h18M5.2 5.2l13.6 13.6M18.8 5.2 5.2 18.8\"/></svg>Thawing Room</div>\n" \
"      <nav>\n" \
"        <a href=\"/\" " home "><svg width=\"17\" height=\"17\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M3 10.5 12 3l9 7.5\"/><path d=\"M5 9.5V20h14V9.5\"/></svg>Home</a>\n" \
"        <a href=\"/events\" " events "><svg width=\"17\" height=\"17\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"1.8\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><path d=\"M3 12h4l3-8 4 16 3-8h4\"/></svg>Events</a>\n" \
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
  NAV_BLOCK("class=\"active\" aria-current=\"page\"", "", "", "")
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
  NAV_BLOCK("", "", "", "class=\"active\" aria-current=\"page\"")
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
  NAV_BLOCK("", "", "class=\"active\" aria-current=\"page\"", "")
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
  NAV_BLOCK("", "class=\"active\" aria-current=\"page\"", "", "")
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
  NAV_BLOCK("", "", "", "class=\"active\" aria-current=\"page\"")
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
