#include "RemoteControl.h"
#include "Logger.h"
#include <RTClib.h>

RemoteControl remote;

// Horizonte del arranque diferido. Más allá de esto casi seguro es un error de fecha.
#define DSTART_MAX_AHEAD_S (7UL * 24UL * 3600UL)

static const char* const STAGE_NAMES[] = {"idle", "Stage 1", "Stage 2", "Stage 3", "error"};

static uint8_t daysInMonth(uint16_t year, uint8_t month) {
    static const uint8_t DAYS[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && (year % 4 == 0) && (year % 100 != 0 || year % 400 == 0)) return 29;
    return DAYS[month - 1];
}

void RemoteControl::publish(const ControlStatus &s) {
    portENTER_CRITICAL(&mux);
    status = s;
    has_status = true;
    portEXIT_CRITICAL(&mux);
}

bool RemoteControl::take(ControlCommand &cmd) {
    portENTER_CRITICAL(&mux);
    const bool got = has_pending;
    if (got) { cmd = pending; has_pending = false; }
    portEXIT_CRITICAL(&mux);
    return got;
}

void RemoteControl::finish(uint32_t id, bool ok, const char *msg) {
    portENTER_CRITICAL(&mux);
    done_id = id;
    done_ok = ok;
    strlcpy(done_msg, msg, sizeof(done_msg));
    portEXIT_CRITICAL(&mux);
}

uint32_t RemoteControl::request(ControlCommand cmd, char *err, size_t err_len) {
    portENTER_CRITICAL(&mux);
    const bool ready = has_status;
    const bool busy  = has_pending;
    const uint8_t stage = status.stage;
    const bool locked = status.lock;
    portEXIT_CRITICAL(&mux);

    if (!ready) { strlcpy(err, "The controller is still starting. Try again in a few seconds.", err_len); return 0; }
    if (busy)   { strlcpy(err, "Another command is still being processed.", err_len); return 0; }

    // Mismas reglas que el SCADA: START y D-START solo con el cuarto en reposo (el
    // callback MQTT los ignora fuera de IDLE). STOP en cualquier etapa.
    if (cmd.action == CTRL_START || cmd.action == CTRL_DSTART) {
        if (locked) {
            strlcpy(err, "Maintenance lockdown is active. Lift it before starting the process.", err_len);
            return 0;
        }
        if (stage != 0) {
            snprintf(err, err_len, "The process is already running (%s). Stop it first.",
                     STAGE_NAMES[stage < 5 ? stage : 4]);
            return 0;
        }
    } else if (cmd.action == CTRL_STOP) {
        if (stage == 0) { strlcpy(err, "The process is not running.", err_len); return 0; }
    } else if (cmd.action == CTRL_LOCK_ON || cmd.action == CTRL_LOCK_OFF) {
        const bool on = cmd.action == CTRL_LOCK_ON;
        if (on == locked) {
            strlcpy(err, on ? "Maintenance lockdown is already active." : "Maintenance lockdown is not active.", err_len);
            return 0;
        }
    } else {
        strlcpy(err, "Unknown action.", err_len);
        return 0;
    }

    if (cmd.action == CTRL_DSTART) {
        bool trusted = false;
        const uint32_t now = logger.epochAt(millis(), &trusted);
        if (!trusted || now == 0) {
            strlcpy(err, "The controller clock is not set, so a delayed start can't be scheduled. Use Start now instead.", err_len);
            return 0;
        }
        if (cmd.hour > 23 || cmd.minute > 59 || cmd.month < 1 || cmd.month > 12 || cmd.day < 1) {
            strlcpy(err, "Invalid date or time.", err_len);
            return 0;
        }

        // Controller::saveStage2StartTime() arma la fecha con el año en curso y, si ya
        // pasó, la corre al siguiente. Se valida con la misma regla para que lo que se
        // acepta acá sea exactamente lo que después se programa.
        const DateTime n(now);
        uint16_t year = n.year();
        if (cmd.day > daysInMonth(year, cmd.month)) { strlcpy(err, "Invalid date.", err_len); return 0; }
        DateTime target(year, cmd.month, cmd.day, cmd.hour, cmd.minute, 0);
        const bool past = target.unixtime() <= now;
        if (past) {
            year++;
            if (cmd.day > daysInMonth(year, cmd.month)) { strlcpy(err, "Invalid date.", err_len); return 0; }
            target = DateTime(year, cmd.month, cmd.day, cmd.hour, cmd.minute, 0);
        }
        const uint32_t ahead = target.unixtime() - now;
        if (ahead > DSTART_MAX_AHEAD_S) {
            snprintf(err, err_len, past
                     ? "That time has already passed (controller time is %02u:%02u on %02u/%02u)."
                     : "Stage 2 can be scheduled at most 7 days ahead (controller time is %02u:%02u on %02u/%02u).",
                     n.hour(), n.minute(), n.day(), n.month());
            return 0;
        }
    }

    portENTER_CRITICAL(&mux);
    uint32_t id = 0;
    if (!has_pending) {
        id = cmd.id = next_id++;
        pending = cmd;
        has_pending = true;
    }
    portEXIT_CRITICAL(&mux);
    if (!id) strlcpy(err, "Another command is still being processed.", err_len);
    return id;
}

static void printFloat(Print &out, const char *key, float v) {
    if (isnan(v) || isinf(v)) out.printf("\"%s\":null,", key);
    else                      out.printf("\"%s\":%.1f,", key, v);
}

void RemoteControl::writeJSON(Print &out) {
    portENTER_CRITICAL(&mux);
    const ControlStatus s = status;
    const bool ready = has_status;
    const bool busy = has_pending;
    const uint32_t cmd_id = done_id;
    const bool cmd_ok = done_ok;
    char msg[sizeof(done_msg)];
    memcpy(msg, done_msg, sizeof(msg));
    portEXIT_CRITICAL(&mux);

    bool trusted = false;
    const uint32_t now = logger.epochAt(millis(), &trusted);

    out.printf("{\"ready\":%d,\"now\":%lu,\"trusted\":%d,\"uptime\":%lu,", ready, (unsigned long)now,
               trusted, (unsigned long)(millis() / 1000));
    out.printf("\"stage\":%u,\"step\":%u,\"starting\":%d,\"cycle\":%u,\"active\":%d,\"since\":%lu,\"since_boot\":%d,",
               s.stage, s.step, s.starting, s.cycle, s.cycle_active, (unsigned long)s.stage_since, s.stage_since_boot);

    out.print("\"temps\":{");
    printFloat(out, "ta", s.ta);   printFloat(out, "ts", s.ts);   printFloat(out, "tc", s.tc);
    printFloat(out, "ata", s.avg_ta); printFloat(out, "ats", s.avg_ts); printFloat(out, "atc", s.avg_tc);
    printFloat(out, "ts_set", s.ts_set); printFloat(out, "tc_set", s.tc_set);
    printFloat(out, "sp", s.pid_setpoint);
    out.printf("\"fa\":%d,\"fs\":%d,\"fc\":%d,\"ca\":%u,\"cs\":%u,\"cc\":%u},",
               s.ta_fault, s.ts_fault, s.tc_fault, s.ta_counts, s.ts_counts, s.tc_counts);

    out.printf("\"out\":{\"fan\":%d,\"rev\":%d,\"damper\":%d,\"s1\":%d,\"air\":%u,\"air_max\":%u},",
               s.fan_on, s.fan_reverse, s.damper, s.sprinkler, s.air_duty, s.air_max);
    out.printf("\"s2\":{\"set\":%d,\"h\":%u,\"m\":%u,\"d\":%u,\"mo\":%u,\"left\":%ld},",
               s.s2_set, s.s2_hour, s.s2_minute, s.s2_day, s.s2_month, (long)s.s2_remaining_min);
    out.printf("\"lock\":{\"on\":%d,\"since\":%lu,\"src\":%u},", s.lock, (unsigned long)s.lock_since, s.lock_source);
    out.printf("\"link\":{\"wifi\":%d,\"mqtt\":%d,\"sd\":%d,\"clock\":%d,\"src\":%u},",
               s.wifi, s.mqtt, s.sd, s.time_trusted, s.time_source);

    out.printf("\"busy\":%d,\"cmd\":{\"id\":%lu,\"ok\":%d,\"msg\":\"", busy, (unsigned long)cmd_id, cmd_ok);
    for (const char *c = msg; *c; c++) {   // mensajes propios, pero igual escapados
        if (*c == '"' || *c == '\\') out.print('\\');
        out.print(*c);
    }
    out.print("\"}}");
}
