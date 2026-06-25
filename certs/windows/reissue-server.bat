@echo off
REM ============================================================================
REM  reissue-server.bat  (Windows)
REM
REM  Reemite el certificado de servidor de Mosquitto firmado por el CA existente,
REM  agregando la IP como SAN tipo IP **y** DNS (necesario para arduino-esp32 2.0.x).
REM
REM  REQUISITOS:
REM    - openssl.exe en el PATH (viene con Git for Windows, o instala "OpenSSL Light"
REM      de slproweb.com). Para probar: abre CMD y escribe:  openssl version
REM    - Ejecutar este .bat en la MISMA carpeta donde estan ca.crt y ca.key.
REM    - El archivo server_ext.cnf debe estar en esta carpeta.
REM
REM  Si el broker NO esta en 192.168.100.69, cambia BROKER aqui ABAJO y tambien
REM  la primera linea de server_ext.cnf (deben coincidir).
REM ============================================================================

set BROKER=192.168.100.69

echo === Generando clave del servidor ===
openssl genrsa -out server.key 2048
if errorlevel 1 goto :error

echo === Generando CSR ===
openssl req -new -key server.key -out server.csr -subj "/CN=%BROKER%"
if errorlevel 1 goto :error

echo === Firmando server.crt con el CA (SAN IP+DNS) ===
openssl x509 -req -days 3650 -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out server.crt -extfile server_ext.cnf
if errorlevel 1 goto :error

del server.csr >nul 2>&1

echo.
echo === LISTO ===
echo Se generaron server.crt y server.key.
echo 1) Copialos a la carpeta de certificados de Mosquitto (junto a ca.crt).
echo 2) Reinicia el servicio Mosquitto:
echo        net stop mosquitto ^&^& net start mosquitto
echo    (o reinicia mosquitto.exe como lo tengas configurado)
goto :eof

:error
echo.
echo *** ERROR: fallo un paso de openssl. Revisa que openssl este instalado y que
echo *** ca.crt, ca.key y server_ext.cnf esten en esta carpeta.
exit /b 1
