

const char* SERVER_INDEX_HTML = R"RAW_HTML(
<!DOCTYPE html>
<html lang="en">
  <head>
    <title>ESP32 Web Server</title>
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <link rel="icon" href="data:," />
    <link rel="stylesheet" type="text/css" href="style.css" />
    <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js"></script>
  </head>
  <body>
    <nav>
      <ul>
        <li><a href="/">Home</a></li>
        <li><a href="/logs">Logs</a></li>
        <li><a href="/edit-settings">Config</a></li>
        <li><a href="/webserial">Web Serial</a></li>
      </ul>
    </nav>

    <div class="content">
      <div id="top-bar">
        <div>
          <h1 style="margin: 0">OTA Update</h1>
          <span>Upload a new firmware to update the device</span>
        </div>
        <h3>{{LOCATION}}</h3>
      </div>

      <form
        method="POST"
        action="#"
        enctype="multipart/form-data"
        id="upload_form"
      >
        <h2>Firmware update</h2>
        <input
          type="file"
          name="update"
          id="file"
          onchange="sub(this, 'file-input')"
          style="display: none"
        />
        <label id="file-input" for="file"> Choose file...</label>
        <input type="submit" class="btn" value="Update" />
        <br /><br />
        <div id="prg"></div>
        <div id="prgbar"><div id="bar"></div></div>
        <br />
        <small>* Caution, be sure to upload the correct firmware</small>
        <br />
      </form>
      <form
        name="uploadConfigForm"
        method="POST"
        action="/replace-config"
        enctype="multipart/form-data"
      >
        <h2>Update config file</h2>
        <label id="config-input" for="config"> Choose file...</label>
        <input
          id="config"
          type="file"
          name="upload"
          style="display: none"
          onchange="sub(this, 'config-input')"
        />
        <input type="submit" value="Upload" />

        <small>* Caution, be sure to upload the correct config file</small>
      </form>

      <form name="resetForm">
        <h2>Reboot</h2>
        <input
          type="submit"
          class="btn"
          value="Reset"
        />
        <small
          >* This will reboot the device, be sure no process is running</small
        >
      </form>
      <div id="bottom-bar">
        <!-- time, version and location -->
        <p>Time: 12:00:00</p>
        <p>Version: {{VERSION}}</p>
      </div>
    </div>
  </body>
  <script>
    function check(form) {
      var xhr = new XMLHttpRequest();
      xhr.open("POST", "/reset", true);
      xhr.send();
    }
    function sub(obj, id) {
      var fileName = obj.value.split("\\\\");
      document.getElementById(id).innerHTML =
        "   " + fileName[fileName.length - 1];
    }
    $("#upload_form").submit(function (e) {
      e.preventDefault();
      var form = $("#upload_form")[0];
      var data = new FormData(form);
      $.ajax({
        url: "/update",
        type: "POST",
        data: data,
        contentType: false,
        processData: false,
        xhr: function () {
          var xhr = new window.XMLHttpRequest();
          xhr.upload.addEventListener(
            "progress",
            function (evt) {
              if (evt.lengthComputable) {
                var per = evt.loaded / evt.total;
                $("#prg").html("progress: " + Math.round(per * 100) + "%");
                $("#bar").css("width", Math.round(per * 100) + "%");
              }
            },
            false
          );
          return xhr;
        },
        success: function (d, s) {
          alert("Firmware updated successfully");
        },
        error: function (a, b, c) {
          alert("Error updating the firmware");
          $("#prg").html("");
          $("#bar").css("width", Math.round(0) + "%");
        },
      });
    });

    $("form[name='uploadConfigForm']").submit(function (e) {
      // Aquí iría el manejo específico para el submit de uploadConfigForm si lo necesitas
    });

    $("form[name='resetForm']").submit(function (e) {
      e.preventDefault();
      check(this);
    });
  </script>
</html>
)RAW_HTML";

const  char* EDIT_SETTINGS_HTML= R"RAW_HTML(
  <!DOCTYPE html>
<html lang="en">
  <head>
    <title>ESP32 Web Server</title>
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <link rel="icon" href="data:," />
    <link rel="stylesheet" type="text/css" href="style.css" />
    <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js"></script>
  </head>
  <body>
    <nav>
      <ul>
        <li><a href="/">Home</a></li>
        <li><a href="/logs">Logs</a></li>
        <li><a href="/edit-settings">Config</a></li>
        <li><a href="/webserial">Web Serial</a></li>
      </ul>
    </nav>

    <div class="content">
      <div id="top-bar">
        <div>
          <h1 style="margin: 0">Edit Settings</h1>
          <span>Choose the settings you want to edit</span>
        </div>
        <h3>{{LOCATION}}</h3>
      </div>

      <div class="d-flex justify-center align-center" style="flex: 1">
        <a href="/edit-config?file=config.txt">
          <div class="card" style="width: 290px; height: 350px">
            System Settings
          </div>
        </a>
        <a href="/edit-config?file=defaultParameters.txt">
        <div class="card" style="width: 290px; height: 350px">
          Thawing Settings
        </div>
    </a>
      </div>

      <div id="bottom-bar">
        <p>Time: 12:00:00</p>
        <p>Version: {{VERSION}}</p>
      </div>
    </div>
  </body>
</html>
)RAW_HTML";

const char* LOGS_HTML = R"RAW_HTML(
<!DOCTYPE html>
<html lang="en">
  <head>
    <title>ESP32 Web Server</title>
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <link rel="icon" href="data:," />
    <link rel="stylesheet" type="text/css" href="style.css" />
  </head>
  <body>
    <nav>
      <ul>
        <li><a href="/">Home</a></li>
        <li><a href="/logs">Logs</a></li>
        <li><a href="/edit-settings">Config</a></li>
        <li><a href="/webserial">Web Serial</a></li>
      </ul>
    </nav>

    <div class="content">
      <div id="top-bar">
        <div>
          <h1 style="margin: 0">Logs</h1>
          <span>View the log files of each thawning process</span>
        </div>
        <h3>{{LOCATION}}</h3>
      </div>
      <div class="logs-container">
        <div class="d-flex justify-sb align-center">
          <h3>Total Logs: </h3>
          <b><u id="count"></u></b>
        </div>
        <!-- here logs will be inyected -->
      </div>
      <div id="bottom-bar">
        <!-- time, version and location -->
        <p>Time: 12:00:00</p>
        <p>Version: {{VERSION}}</p>
      </div>
    </div>
    <script>
        const logs = [
            //{{LOGS}}
        ];

        // create logs cells
        logs.forEach((log, index) => {
            const logCell = document.createElement("div");
            logCell.classList.add("log-cell");
            logCell.innerHTML = `<a href="/download_log?file=${log}">${log}</a>`;
            document.querySelector(".logs-container").appendChild(logCell);
        });

        //add total logs
        document.getElementById("count").innerText = logs.length;
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
    <title>ESP32 Web Server</title>
    <link rel="stylesheet" type="text/css" href="style.css" />
  </head>
  <body>
    <nav>
      <ul>
        <li><a href="/">Home</a></li>
        <li><a href="/logs">Logs</a></li>
        <li><a href="/edit-settings">Config</a></li>
        <li><a href="/webserial">Web Serial</a></li>
      </ul>
    </nav>

    <div class="content">
      <div id="top-bar">
        <div>
          <h1 style="margin: 0">Edit configuration file (JSON)</h1>
          <span>Edit values and click "Update" to save changes</span>
        </div>

        <h3>{{LOCATION}}</h3>
      </div>
      <form
        id="configForm"
        style="max-width: 80%; width: 80%; text-align: left"
        action="/update-config"
        method="POST"
      >
        <div class="d-flex justify-sb align-center">
          <h2 id="file-name">Configuration parameters</h2>
          <div style="display: flex">
            <button class="btn" type="button" onclick="downloadConfig()">
              Download JSON
            </button>
            <br />
            <button class="btn" type="button">Toggle</button>
          </div>
        </div>

        <div id="formContent" class="d-flex flex-column"></div>

        <input type="submit" value="Save" class="btn" style="width: 30%" />
        <small
          >*Caution be sure you are editing the correct values, boot can be
          affected</small
        >
        <br />
      </form>
    </div>

    <div id="bottom-bar">
      <p>Time: 12:00:00</p>
      <p>Version: {{VERSION}}</p>
    </div>
    <script>
      // Obtener los parámetros de la URL
      const queryString = window.location.search;
      const urlParams = new URLSearchParams(queryString);

      // Por ejemplo, si tienes ?nombre=Juan en la URL
      const fileName = urlParams.get("file");

      // Mostrar el valor capturado
      document.getElementById("file-name").textContent = fileName;
      document.addEventListener("DOMContentLoaded", function () {
        fetch(fileName)
          .then((response) => {
            if (!response.ok) {
              throw new Error("File not found, check file name");
            }
            return response.json();
          })
          .then((data) => {
            const formContent = document.getElementById("formContent");
            generateFormFields(formContent, data);
          })
          .catch((error) => {
            console.error(
              "Error could not fetch configuration file:",
              error
            );
            const formContent = document.getElementById("formContent");
            formContent.innerHTML = `<p style="color: red;">Error: ${error.message}</p>`;
          });
      });

      function generateFormFields(container, data, parentKey = "") {
        for (const key in data) {
          if (typeof data[key] === "object" && !Array.isArray(data[key])) {
            generateFormFields(container, data[key], `${parentKey}${key}|`);
          } else {
            const label = document.createElement("label");
            label.setAttribute("for", `${parentKey}${key}`);
            label.textContent = `${parentKey}${key}: `;

            const input = document.createElement("input");
            input.setAttribute("type", "text");
            input.setAttribute("name", `${parentKey}${key}`);
            input.setAttribute("value", data[key]);

            container.appendChild(label);
            container.appendChild(input);
            container.appendChild(document.createElement("br"));
          }
        }
      }

      function downloadConfig() {
        // Lógica para descargar el archivo config.txt
        fetch(fileName)
          .then((response) => response.blob())
          .then((blob) => {
            const url = window.URL.createObjectURL(blob);
            const a = document.createElement("a");
            a.style.display = "none";
            a.href = url;
            a.download = fileName;
            document.body.appendChild(a);
            a.click();
            window.URL.revokeObjectURL(url);
          })
          .catch((error) => {
            console.error(
              "Error at downloading config file: ",
              error
            );
          });
      }
    </script>
  </body>
</html>
)RAW_HTML";


const char* STYLE_CSS = R"RAW_CSS(
:root {
    --primary-color: #3498db;
    --white-color: #f1f1f1;
    
    --bottom-bar-height: 30px;
    --nav-bar-width: 200px;
}


#file-input, #config-input, input {
    width:90%;
    height:44px;
    border-radius:4px;
    margin:10px auto;
    font-size:15px;
}
label{
    // overflow-x: scroll;
}
input {
    background: var(--white-color);border:0;
    padding:0 15px;
}
body{
    display: flex;
    flex:1;
    background:var(--primary-color);
    font-family:sans-serif;
    font-size:14px;
    color:#777;
    margin: 0 !important;
}
/* left side nav */
nav{
    display: flex;
    background-color: var(--white-color);
    width: var(--nav-bar-width);
    height:100vh      ;
    top: 0;
    left: 0;
    /* z-index: -100; */
    /* padding-top: 20px; */
}

nav ul{
    list-style: none;
    padding: 0;
    margin-top: 100px;
    width: 100%;
    border-top: 1px solid #ddd;
}
nav ul li{
    padding: 10px 20px;
    border-bottom: 1px solid #ddd;
    cursor: pointer;
}
nav ul li:hover{
    background-color: #f9f9f9;
}
.content{
    display: flex;
    flex:1;
    flex-direction: column;
    height: calc(100vh - var(--bottom-bar-height));
    overflow-y: auto;
    /* padding:20px; */
}
.logs-container{
    overflow-y: auto;
    padding:10px;
}
.log-cell{
    display: flex;
    align-items:  center;
    background-color: #ddd;
    height: 60px;
    margin-bottom: 10px;
    border-radius: 3px;
    padding-left: 15px;
    padding-right: 15px;
    border: 1px solid #ddd;
}
#file-input, #config-input{
    padding-left:5px;
    border:1px solid #ddd;
    line-height:44px;
    text-align:left;
    display:block;
    cursor:pointer;
}
#bar, #prgbar{
    background-color: var(--white-color);
    border-radius:10px;
}
#bar{
    background-color:var(--primary-color);
    width:0%;
    height:10px;
}
form{
    display: flex;
    flex-direction: column;
    background: white;
    width:288px;
    max-width: 288px;
    margin:75px auto;
    padding:30px;
    border-radius:5px;
    text-align:center;
}
.btn{
    background:var(--primary-color);
    color:var(--white-color);
    cursor:pointer;
}
#bottom-bar{
    position: fixed;
    display: flex;
    align-items: center;
    /* flex-direction: row-reverse; */
    gap: 10px;
    bottom: 0;
    left: 0;
    width: 100%;
    padding: 0px 15px;
    background: var(--white-color);
    height: var(--bottom-bar-height);
}

#top-bar{
    display: flex;
    position: sticky;
    align-items: center;
    justify-content: space-between;
    gap: 10px;
    padding: 0px 15px;
    background: var(--white-color);
    height: 65px;

}

.card{
    background:var(--white-color);
    border-radius:5px;
    padding:10px;
    margin:10px;
    cursor: pointer;
}

.card:hover{
    box-shadow: 0 0 10px rgba(0,0,0,0.1);
    transition: .7 all quadratic-bezier(0.25, 0.46, 0.45, 0.94);
    transform: scale(1.01);
}

.d-flex{
    display: flex;
}

.flex-column{
    flex-direction: column;
}

.justify-center{
    justify-content: center;
}

.align-center{
    align-items: center;
}

.justify-sb{
    justify-content: space-between;
}
)RAW_CSS";

