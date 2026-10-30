#include "Calibration.h"
#include "ConfigStore.h"
#include "EventLog.h"
#include "Logger.h"
#include "config.h"
#include <ArduinoJson.h>

SensorCalibration calib;

// "prev" puede ser la rampa de fábrica: se guarda como {"factory":1}. Por eso además de
// prev[] hace falta saber si HAY algo a qué volver.
static bool has_prev[CAL_CHANNELS] = {false, false, false};

static const char* const KEYS[CAL_CHANNELS]  = {"ta", "ts", "tc"};
static const char* const NAMES[CAL_CHANNELS] = {"Ta", "Ts", "Tc"};

const char* SensorCalibration::channelKey(uint8_t ch)  { return ch < CAL_CHANNELS ? KEYS[ch] : "?"; }
const char* SensorCalibration::channelName(uint8_t ch) { return ch < CAL_CHANNELS ? NAMES[ch] : "?"; }

float SensorCalibration::factorySlope(uint8_t ch) {
    return ch == CAL_TA ? TA_TEMP_SLOPE : ch == CAL_TS ? TS_TEMP_SLOPE : TC_TEMP_SLOPE;
}
float SensorCalibration::factoryOffset(uint8_t ch) {
    return ch == CAL_TA ? TA_TEMP_OFFSET : ch == CAL_TS ? TS_TEMP_OFFSET : TC_TEMP_OFFSET;
}

// 0 = no hay, 1 = cargada, -1 = está pero no sirve
static int parseCal(JsonVariantConst v, ChannelCal &out) {
    out = ChannelCal();
    if (v.isNull() || v["factory"].as<bool>()) return 0;
    JsonArrayConst pts = v["pts"];
    if (pts.size() != CAL_POINTS) return -1;
    for (int i = 0; i < CAL_POINTS; i++) {
        out.pts[i].raw  = pts[i][0] | 0;
        out.pts[i].temp = pts[i][1] | NAN;
    }
    if (calValidate(out.pts) != nullptr) { out = ChannelCal(); return -1; }
    out.at = v["at"] | 0;
    out.valid = true;
    return 1;
}

static void putCal(JsonObject obj, const ChannelCal &cal) {
    if (!cal.valid) { obj["factory"] = 1; return; }
    obj["at"] = cal.at;
    JsonArray pts = obj.createNestedArray("pts");
    for (int i = 0; i < CAL_POINTS; i++) {
        JsonArray p = pts.createNestedArray();
        p.add(cal.pts[i].raw);
        p.add(roundf(cal.pts[i].temp * 100.0f) / 100.0f);
    }
}

void SensorCalibration::begin() {
    if (model_mutex == nullptr) model_mutex = xSemaphoreCreateMutex();

    const String content = ConfigStore::read(CAL_FILE);
    if (content.length()) {
        StaticJsonDocument<1536> doc;
        if (deserializeJson(doc, content)) {
            events.emit(EV_ALERT, SRC_SYSTEM, "calibration.txt is unreadable: all probes use the factory ramp");
        } else {
            for (uint8_t ch = 0; ch < CAL_CHANNELS; ch++) {
                JsonVariantConst entry = doc[KEYS[ch]];
                if (parseCal(entry["cur"], cur[ch]) < 0) {
                    events.emit(EV_ALERT, SRC_SYSTEM, "%s calibration is invalid: using the factory ramp", NAMES[ch]);
                }
                has_prev[ch] = !entry["prev"].isNull() && parseCal(entry["prev"], prev[ch]) >= 0;
            }
        }
    }

    for (uint8_t ch = 0; ch < CAL_CHANNELS; ch++) {
        act[ch] = cur[ch];
        if (cur[ch].valid) {
            LOG_I(TAG_ADC, "%s: calibracion de 3 puntos (%u=%.1f, %u=%.1f, %u=%.1f)", NAMES[ch],
                  cur[ch].pts[0].raw, cur[ch].pts[0].temp, cur[ch].pts[1].raw, cur[ch].pts[1].temp,
                  cur[ch].pts[2].raw, cur[ch].pts[2].temp);
        } else {
            LOG_I(TAG_ADC, "%s: rampa de fabrica (%.4f, %.2f)", NAMES[ch], factorySlope(ch), factoryOffset(ch));
        }
    }
}

void SensorCalibration::sample(uint8_t ch, uint16_t raw) {
    if (ch >= CAL_CHANNELS) return;
    portENTER_CRITICAL(&mux);
    ring[ch][ring_next[ch]] = raw;
    ring_next[ch] = (ring_next[ch] + 1) % CAL_WINDOW;
    if (ring_count[ch] < CAL_WINDOW) ring_count[ch]++;
    last_raw[ch] = raw;
    portEXIT_CRITICAL(&mux);
}

void SensorCalibration::setContext(bool lockdown, uint8_t stage, const bool available[CAL_CHANNELS]) {
    portENTER_CRITICAL(&mux);
    ctx_lock = lockdown;
    ctx_stage = stage;
    for (uint8_t ch = 0; ch < CAL_CHANNELS; ch++) {
        // Si el canal deja de venir del ADC (Ts por IR, Tc por LoRa), sus muestras no sirven.
        if (ctx_avail[ch] && !available[ch]) ring_count[ch] = 0;
        ctx_avail[ch] = available[ch];
    }
    ctx_ready = true;
    portEXIT_CRITICAL(&mux);
}

void SensorCalibration::service() {
    ChannelCal next[CAL_CHANNELS];
    bool got[CAL_CHANNELS];
    portENTER_CRITICAL(&mux);
    for (uint8_t ch = 0; ch < CAL_CHANNELS; ch++) {
        got[ch] = has_pending[ch];
        if (got[ch]) { next[ch] = pending[ch]; has_pending[ch] = false; }
    }
    portEXIT_CRITICAL(&mux);
    for (uint8_t ch = 0; ch < CAL_CHANNELS; ch++) {
        if (got[ch]) act[ch] = next[ch];
    }
}

void SensorCalibration::post(uint8_t ch, const ChannelCal &cal) {
    portENTER_CRITICAL(&mux);
    pending[ch] = cal;
    has_pending[ch] = true;
    portEXIT_CRITICAL(&mux);
}

bool SensorCalibration::save(char *msg, size_t msg_len) {
    DynamicJsonDocument doc(2048);
    for (uint8_t ch = 0; ch < CAL_CHANNELS; ch++) {
        JsonObject entry = doc.createNestedObject(KEYS[ch]);
        putCal(entry.createNestedObject("cur"), cur[ch]);
        if (has_prev[ch]) putCal(entry.createNestedObject("prev"), prev[ch]);
    }
    String out;
    serializeJson(doc, out);
    if (!ConfigStore::write(CAL_FILE, out)) {
        strlcpy(msg, "Could not write calibration.txt to the controller's memory. Nothing was changed.", msg_len);
        return false;
    }
    return true;
}

bool SensorCalibration::request(CalOp op, uint8_t ch, CalPoint pts[CAL_POINTS], char *msg, size_t msg_len) {
    if (ch >= CAL_CHANNELS) { strlcpy(msg, "Unknown sensor.", msg_len); return false; }

    portENTER_CRITICAL(&mux);
    const bool ready = ctx_ready, lock = ctx_lock, avail = ctx_avail[ch];
    const uint8_t stage = ctx_stage;
    portEXIT_CRITICAL(&mux);

    if (!ready) { strlcpy(msg, "The controller is still starting. Try again in a few seconds.", msg_len); return false; }
    if (!lock)  { strlcpy(msg, "Turn on the maintenance lockdown before changing a calibration.", msg_len); return false; }
    if (stage != 0) { strlcpy(msg, "Stop the process before changing a calibration.", msg_len); return false; }
    if (!avail) {
        snprintf(msg, msg_len, "%s is not read from the analog input right now (IR or LoRa), so it can't be calibrated here.", NAMES[ch]);
        return false;
    }

    if (op == CAL_OP_SAVE) {
        const char *bad = calValidate(pts);
        if (bad) { strlcpy(msg, bad, msg_len); return false; }
    }

    if (xSemaphoreTake(model_mutex, pdMS_TO_TICKS(2000)) != pdTRUE) {
        strlcpy(msg, "The calibration is busy. Try again.", msg_len);
        return false;
    }
    const ChannelCal old_cur = cur[ch], old_prev = prev[ch];
    const bool old_has_prev = has_prev[ch];
    bool ok = false;

    if (op == CAL_OP_SAVE) {
        ChannelCal n;
        for (int i = 0; i < CAL_POINTS; i++) n.pts[i] = pts[i];
        n.valid = true;
        n.at = logger.epochAt(millis());
        prev[ch] = cur[ch];
        has_prev[ch] = true;
        cur[ch] = n;
        ok = true;
    } else if (op == CAL_OP_REVERT) {
        if (!has_prev[ch]) {
            strlcpy(msg, "There is no previous calibration to go back to.", msg_len);
        } else {
            const ChannelCal t = cur[ch];
            cur[ch] = prev[ch];
            prev[ch] = t;
            ok = true;
        }
    } else {
        if (!cur[ch].valid) {
            strlcpy(msg, "This probe already uses the factory ramp.", msg_len);
        } else {
            prev[ch] = cur[ch];
            has_prev[ch] = true;
            cur[ch] = ChannelCal();
            ok = true;
        }
    }

    if (ok && !save(msg, msg_len)) {
        cur[ch] = old_cur; prev[ch] = old_prev; has_prev[ch] = old_has_prev;
        ok = false;
    }
    const ChannelCal applied = cur[ch];
    xSemaphoreGive(model_mutex);
    if (!ok) return false;

    post(ch, applied);

    if (op == CAL_OP_SAVE) {
        events.emit(EV_PARAM, SRC_WEB, "%s calibration saved (counts=C): %u=%.1f, %u=%.1f, %u=%.1f",
                    NAMES[ch], applied.pts[0].raw, applied.pts[0].temp, applied.pts[1].raw,
                    applied.pts[1].temp, applied.pts[2].raw, applied.pts[2].temp);
        snprintf(msg, msg_len, "%s calibration saved and in use.", NAMES[ch]);
    } else if (op == CAL_OP_REVERT) {
        events.emit(EV_PARAM, SRC_WEB, "%s calibration reverted to the previous one%s", NAMES[ch],
                    applied.valid ? "" : " (factory ramp)");
        snprintf(msg, msg_len, "%s is back on its previous calibration.", NAMES[ch]);
    } else {
        events.emit(EV_PARAM, SRC_WEB, "%s calibration reset to the factory ramp", NAMES[ch]);
        snprintf(msg, msg_len, "%s now uses the factory ramp.", NAMES[ch]);
    }
    return true;
}

static void printCal(Print &out, const ChannelCal &cal) {
    if (!cal.valid) { out.print("null"); return; }
    out.printf("{\"at\":%lu,\"pts\":[[%u,%.2f],[%u,%.2f],[%u,%.2f]]}", (unsigned long)cal.at,
               cal.pts[0].raw, cal.pts[0].temp, cal.pts[1].raw, cal.pts[1].temp, cal.pts[2].raw, cal.pts[2].temp);
}

bool SensorCalibration::writeJSON(Print &out) {
    uint16_t samples[CAL_CHANNELS][CAL_WINDOW];
    uint8_t  count[CAL_CHANNELS];
    uint16_t last[CAL_CHANNELS];
    bool avail[CAL_CHANNELS];
    portENTER_CRITICAL(&mux);
    memcpy(samples, ring, sizeof(samples));
    memcpy(count, ring_count, sizeof(count));
    memcpy(last, last_raw, sizeof(last));
    memcpy(avail, ctx_avail, sizeof(avail));
    const bool ready = ctx_ready, lock = ctx_lock;
    const uint8_t stage = ctx_stage;
    portEXIT_CRITICAL(&mux);

    if (xSemaphoreTake(model_mutex, pdMS_TO_TICKS(500)) != pdTRUE) return false;
    ChannelCal c[CAL_CHANNELS], p[CAL_CHANNELS];
    bool hp[CAL_CHANNELS];
    for (uint8_t ch = 0; ch < CAL_CHANNELS; ch++) { c[ch] = cur[ch]; p[ch] = prev[ch]; hp[ch] = has_prev[ch]; }
    xSemaphoreGive(model_mutex);

    out.printf("{\"ready\":%d,\"lock\":%d,\"stage\":%u,\"now\":%lu,\"window_ms\":%u,\"ch\":[",
               ready, lock, stage, (unsigned long)logger.epochAt(millis()), CAL_WINDOW * CAL_SAMPLE_MS);
    for (uint8_t ch = 0; ch < CAL_CHANNELS; ch++) {
        uint32_t sum = 0;
        uint16_t lo = 0xFFFF, hi = 0;
        for (uint8_t i = 0; i < count[ch]; i++) {
            const uint16_t v = samples[ch][i];
            sum += v;
            if (v < lo) lo = v;
            if (v > hi) hi = v;
        }
        if (ch) out.print(',');
        out.printf("{\"k\":\"%s\",\"name\":\"%s\",\"avail\":%d,\"f\":[%.5f,%.3f],\"cur\":",
                   KEYS[ch], NAMES[ch], avail[ch], factorySlope(ch), factoryOffset(ch));
        printCal(out, c[ch]);
        out.printf(",\"prev\":%d,\"prev_factory\":%d,", hp[ch], hp[ch] && !p[ch].valid);
        if (count[ch]) {
            out.printf("\"live\":{\"n\":%u,\"mean\":%.1f,\"min\":%u,\"max\":%u,\"last\":%u}}",
                       count[ch], (float)sum / count[ch], lo, hi, last[ch]);
        } else {
            out.print("\"live\":{\"n\":0}}");
        }
    }
    out.print("]}");
    return true;
}
