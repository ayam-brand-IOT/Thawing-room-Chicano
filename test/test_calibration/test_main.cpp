// Unit tests NATIVOS de la calibración de 3 puntos (CalibrationMath).
// Correr con:  pio test -e native -f test_calibration
#include <unity.h>
#include <math.h>
#include "CalibrationMath.h"

void setUp(void) {}
void tearDown(void) {}

static ChannelCal makeCal(CalPoint a, CalPoint b, CalPoint c) {
    ChannelCal cal;
    cal.pts[0] = a; cal.pts[1] = b; cal.pts[2] = c;
    TEST_ASSERT_NULL(calValidate(cal.pts));
    cal.valid = true;
    return cal;
}

// --- calApply ---------------------------------------------------------------

void test_sin_calibracion_usa_la_rampa_de_fabrica(void) {
    ChannelCal cal;   // valid = false
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2000 * 0.0263f - 64.5f, calApply(cal, 2000, 0.0263f, -64.5f));
}

void test_pasa_exacto_por_los_tres_puntos(void) {
    ChannelCal cal = makeCal({1800, -18.0f}, {2450, 0.0f}, {3260, 22.0f});
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -18.0f, calApply(cal, 1800, 0, 0));
    TEST_ASSERT_FLOAT_WITHIN(0.001f,   0.0f, calApply(cal, 2450, 0, 0));
    TEST_ASSERT_FLOAT_WITHIN(0.001f,  22.0f, calApply(cal, 3260, 0, 0));
}

void test_interpola_recto_dentro_de_cada_tramo(void) {
    ChannelCal cal = makeCal({1800, -18.0f}, {2450, 0.0f}, {3260, 22.0f});
    TEST_ASSERT_FLOAT_WITHIN(0.001f,  -9.0f, calApply(cal, 2125, 0, 0));   // mitad del tramo 1
    TEST_ASSERT_FLOAT_WITHIN(0.001f,  11.0f, calApply(cal, 2855, 0, 0));   // mitad del tramo 2
}

void test_extrapola_con_el_tramo_de_la_orilla(void) {
    ChannelCal cal = makeCal({1800, -18.0f}, {2450, 0.0f}, {3260, 22.0f});
    const float s1 = 18.0f / 650.0f, s2 = 22.0f / 810.0f;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -18.0f - s1 * 500, calApply(cal, 1300, 0, 0));
    TEST_ASSERT_FLOAT_WITHIN(0.01f,  22.0f + s2 * 500, calApply(cal, 3760, 0, 0));
}

void test_es_monotona(void) {
    ChannelCal cal = makeCal({1800, -18.0f}, {2450, 0.0f}, {3260, 22.0f});
    float prev = calApply(cal, CAL_RAW_MIN, 0, 0);
    for (uint16_t raw = CAL_RAW_MIN + 1; raw <= CAL_RAW_MAX; raw++) {
        const float t = calApply(cal, raw, 0, 0);
        TEST_ASSERT_TRUE(t > prev);
        prev = t;
    }
}

// --- calValidate ------------------------------------------------------------

void test_ordena_los_puntos_por_cuentas(void) {
    CalPoint pts[CAL_POINTS] = {{3260, 22.0f}, {1800, -18.0f}, {2450, 0.0f}};
    TEST_ASSERT_NULL(calValidate(pts));
    TEST_ASSERT_EQUAL_UINT16(1800, pts[0].raw);
    TEST_ASSERT_EQUAL_UINT16(2450, pts[1].raw);
    TEST_ASSERT_EQUAL_UINT16(3260, pts[2].raw);
}

void test_rechaza_lazo_abierto(void) {
    CalPoint pts[CAL_POINTS] = {{0, -18.0f}, {2450, 0.0f}, {3260, 22.0f}};
    TEST_ASSERT_NOT_NULL(calValidate(pts));
}

void test_rechaza_puntos_muy_cercanos(void) {
    CalPoint pts[CAL_POINTS] = {{2400, -1.0f}, {2450, 0.0f}, {3260, 22.0f}};
    TEST_ASSERT_NOT_NULL(calValidate(pts));   // 1 °C de separación
}

void test_rechaza_temperaturas_al_reves(void) {
    CalPoint pts[CAL_POINTS] = {{1800, 22.0f}, {2450, 0.0f}, {3260, -18.0f}};
    TEST_ASSERT_NOT_NULL(calValidate(pts));
}

void test_rechaza_pendiente_imposible(void) {
    // Un cero de menos en la referencia: 2.2 en vez de 22 -> tramo casi plano
    CalPoint pts[CAL_POINTS] = {{1000, -18.0f}, {2450, -12.0f}, {4000, 2.2f}};
    TEST_ASSERT_NOT_NULL(calValidate(pts));
}

void test_rechaza_referencia_faltante(void) {
    CalPoint pts[CAL_POINTS] = {{1800, -18.0f}, {2450, NAN}, {3260, 22.0f}};
    TEST_ASSERT_NOT_NULL(calValidate(pts));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_sin_calibracion_usa_la_rampa_de_fabrica);
    RUN_TEST(test_pasa_exacto_por_los_tres_puntos);
    RUN_TEST(test_interpola_recto_dentro_de_cada_tramo);
    RUN_TEST(test_extrapola_con_el_tramo_de_la_orilla);
    RUN_TEST(test_es_monotona);
    RUN_TEST(test_ordena_los_puntos_por_cuentas);
    RUN_TEST(test_rechaza_lazo_abierto);
    RUN_TEST(test_rechaza_puntos_muy_cercanos);
    RUN_TEST(test_rechaza_temperaturas_al_reves);
    RUN_TEST(test_rechaza_pendiente_imposible);
    RUN_TEST(test_rechaza_referencia_faltante);
    return UNITY_END();
}
