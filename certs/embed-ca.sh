#!/usr/bin/env bash
#
# embed-ca.sh — convierte certs/ca.crt en el header C++ src/mqtt_certs.h que el
# firmware incrusta para verificar al broker por TLS.
#
# Corre esto cada vez que (re)generes el CA con gen-certs.sh.

set -euo pipefail
cd "$(dirname "$0")"

CA_FILE="ca.crt"
OUT="../src/mqtt_certs.h"

if [[ ! -f "$CA_FILE" ]]; then
  echo "No existe $CA_FILE. Corre primero ./gen-certs.sh <broker>" >&2
  exit 1
fi

{
  echo "// AUTOGENERADO por certs/embed-ca.sh — NO editar a mano."
  echo "// Certificado raíz (CA) propio para verificar el broker MQTT por TLS."
  echo "// Regenerar: cd certs && ./embed-ca.sh"
  echo "#ifndef MQTT_CERTS_H"
  echo "#define MQTT_CERTS_H"
  echo ""
  echo "// CA público — seguro de versionar. La clave privada (ca.key) NO se incrusta."
  echo "static const char MQTT_CA_CERT[] PROGMEM = R\"EOF("
  cat "$CA_FILE"
  echo ")EOF\";"
  echo ""
  echo "#endif // MQTT_CERTS_H"
} > "$OUT"

echo "==> Escrito $OUT desde $CA_FILE"
