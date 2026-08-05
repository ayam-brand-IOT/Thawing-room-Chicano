#!/usr/bin/env bash
#
# gen-certs.sh — genera el CA propio y el certificado de servidor para el broker
# Mosquitto que el ESP32 (Thawing Room) verificará por TLS (server-auth).
#
# Modelo de confianza:
#   - Creamos UNA autoridad certificadora propia (ca.key / ca.crt).
#   - El ca.crt (público) se incrusta en el firmware (src/mqtt_certs.h) para que
#     el ESP32 valide la identidad del broker. NO cambia aunque cambie la IP.
#   - El server.key / server.crt se instalan en Mosquitto. El server.crt va firmado
#     por nuestro CA y lleva en su SAN la IP/hostname con que el ESP32 se conecta.
#
# IMPORTANTE:
#   - ca.key y server.key son PRIVADAS. NO se commitean (ver certs/.gitignore).
#   - Si pierdes ca.key no puedes firmar nuevos servidores con el CA ya incrustado.
#
# Uso:
#   ./gen-certs.sh <broker-ip-o-hostname>
#   ./gen-certs.sh 192.168.1.50
#   ./gen-certs.sh mqtt.miplanta.local
#
# Vuelve a correrlo con otra dirección y reusa el MISMO CA (solo regenera el
# certificado de servidor), así el firmware embebido sigue siendo válido.

set -euo pipefail

cd "$(dirname "$0")"

BROKER_ADDR="${1:-}"
if [[ -z "$BROKER_ADDR" ]]; then
  echo "Uso: $0 <broker-ip-o-hostname>" >&2
  echo "Ej:  $0 192.168.1.50" >&2
  exit 1
fi

CA_DAYS=3650        # 10 años
SERVER_DAYS=3650    # 10 años (el ESP32 industrial no debe quedarse sin servicio por expiración)
CA_SUBJ="/C=MX/O=ThawingRoom/CN=ThawingRoom-Root-CA"
SERVER_SUBJ="/C=MX/O=ThawingRoom/CN=${BROKER_ADDR}"

# ¿La dirección es una IP? -> SAN doble IP+DNS. Si no, SAN tipo DNS.
#
# Por qué IP **y** DNS para una IP: el mbedTLS de arduino-esp32 2.0.x NO hace match
# de SANs tipo "IP Address" contra el hostname; solo compara SANs tipo DNS. Si el
# cert lleva la IP únicamente como "IP Address", el ESP32 rechaza el handshake con
# -9984 (CN mismatch). Agregando también DNS:<ip> el match funciona en ese core y en
# los más nuevos. (Cores recientes ya matchean IP-SAN, así que esto es retrocompatible.)
if [[ "$BROKER_ADDR" =~ ^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
  SAN="IP:${BROKER_ADDR},DNS:${BROKER_ADDR}"
else
  SAN="DNS:${BROKER_ADDR}"
fi

echo "==> Broker address : ${BROKER_ADDR}"
echo "==> SAN            : ${SAN}"

# --- 1. CA (solo si no existe; reutilizable) ----------------------------------
if [[ -f ca.key && -f ca.crt ]]; then
  echo "==> CA ya existe, se reutiliza (ca.key / ca.crt)"
else
  echo "==> Generando CA propio..."
  openssl genrsa -out ca.key 2048
  openssl req -new -x509 -sha256 -days "$CA_DAYS" \
    -key ca.key -out ca.crt -subj "$CA_SUBJ"
fi

# --- 2. Clave + CSR del servidor ----------------------------------------------
echo "==> Generando clave y CSR del servidor..."
openssl genrsa -out server.key 2048
openssl req -new -sha256 -key server.key -out server.csr -subj "$SERVER_SUBJ"

# --- 3. Firmar el certificado de servidor con el CA (con SAN) ------------------
echo "==> Firmando certificado de servidor con el CA..."
openssl x509 -req -sha256 -days "$SERVER_DAYS" \
  -in server.csr \
  -CA ca.crt -CAkey ca.key -CAcreateserial \
  -out server.crt \
  -extfile <(printf "subjectAltName=%s\nbasicConstraints=CA:FALSE\nkeyUsage=digitalSignature,keyEncipherment\nextendedKeyUsage=serverAuth\n" "$SAN")

rm -f server.csr

echo ""
echo "==> Listo."
echo "    CA (incrustar en firmware) : certs/ca.crt"
echo "    Broker (instalar en Mosquitto):"
echo "       cafile   = certs/ca.crt"
echo "       certfile = certs/server.crt"
echo "       keyfile  = certs/server.key"
echo ""
echo "    Regenera el header del firmware con:"
echo "       ./embed-ca.sh"
