# Certificados MQTT TLS

CA propio (server-auth) para que el ESP32 verifique al broker Mosquitto.

## Archivos

| Archivo | Versionar | Descripción |
|---|---|---|
| `gen-certs.sh` | ✅ | Genera CA (una vez) + cert de servidor para una IP/hostname |
| `embed-ca.sh` | ✅ | Vuelca `ca.crt` a `src/mqtt_certs.h` (CA embebido en firmware) |
| `ca.crt` | ✅ | CA público (seguro de versionar) |
| `ca.key` | ❌ (gitignored) | **Clave privada del CA — guárdala a salvo** |
| `server.crt` | ❌ (gitignored) | Cert del broker (específico del despliegue) |
| `server.key` | ❌ (gitignored) | Clave privada del broker |

## Flujo

```bash
./gen-certs.sh 192.168.100.69 # usa la IP/hostname real del broker
./embed-ca.sh                 # regenera src/mqtt_certs.h
```

Luego en el broker (Mosquitto):

```
listener 8883
cafile   /ruta/ca.crt
certfile /ruta/server.crt
keyfile  /ruta/server.key
```

Y en `config.txt` del equipo: `"USE_TLS": true`, `"PORT": 8883`.

> El CA se crea una sola vez y se reutiliza. Si necesitas otra IP/hostname, vuelve a
> correr `gen-certs.sh` (regenera solo el cert de servidor); el CA embebido sigue válido.
> Si pierdes `ca.key` no podrás firmar nuevos servidores con el CA ya desplegado.
