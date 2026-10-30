#include "CalibrationMath.h"
#include <math.h>

static float segment(const CalPoint &a, const CalPoint &b, uint16_t raw) {
    const float slope = (b.temp - a.temp) / (float)(b.raw - a.raw);
    return a.temp + slope * ((float)raw - (float)a.raw);
}

const char* calValidate(CalPoint pts[CAL_POINTS]) {
    for (int i = 0; i < CAL_POINTS; i++) {
        if (isnan(pts[i].temp) || isinf(pts[i].temp)) return "A reference temperature is missing.";
        if (pts[i].temp < CAL_TEMP_LO || pts[i].temp > CAL_TEMP_HI)
            return "Reference temperatures must be between -50 and 60 C.";
        if (pts[i].raw < CAL_RAW_MIN)
            return "A point reads below 4 mA (819 counts): the probe loop is open or faulty.";
        if (pts[i].raw > CAL_RAW_MAX) return "A point is above the ADC range.";
    }

    // Orden por cuentas (inserción: son 3)
    for (int i = 1; i < CAL_POINTS; i++) {
        CalPoint p = pts[i];
        int j = i - 1;
        while (j >= 0 && pts[j].raw > p.raw) { pts[j + 1] = pts[j]; j--; }
        pts[j + 1] = p;
    }

    for (int i = 1; i < CAL_POINTS; i++) {
        const CalPoint &a = pts[i - 1], &b = pts[i];
        if (b.raw - a.raw < CAL_MIN_RAW_GAP)
            return "Two points read almost the same counts. Use baths at least 5 C apart.";
        if (b.temp <= a.temp)
            return "Counts and temperatures go in opposite directions. Check the reference values.";
        if (b.temp - a.temp < CAL_MIN_TEMP_GAP)
            return "Points must be at least 5 C apart.";
        const float slope = (b.temp - a.temp) / (float)(b.raw - a.raw);
        if (slope < CAL_SLOPE_MIN || slope > CAL_SLOPE_MAX)
            return "The counts don't match the temperature change. Check the reference values and the probe.";
    }
    return nullptr;
}

float calApply(const ChannelCal &cal, uint16_t raw, float factory_slope, float factory_offset) {
    if (!cal.valid) return raw * factory_slope + factory_offset;
    // Tramo 1 para todo lo que esté por debajo del punto central (incluida la
    // extrapolación hacia abajo), tramo 2 para el resto.
    if (raw <= cal.pts[1].raw) return segment(cal.pts[0], cal.pts[1], raw);
    return segment(cal.pts[1], cal.pts[2], raw);
}
