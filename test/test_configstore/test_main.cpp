// Unit tests NATIVOS (corren en la PC) para la lógica atómica de ConfigStore:
//   write(): .tmp -> validar -> respaldar a .bak -> promover
//   read():  principal -> si corrupto, recupera .bak y lo re-promueve
//
// Usan un FS en RAM (test/mock/SPIFFS.h). Correr con:  pio test -e native
//
#include <unity.h>
#include <SPIFFS.h>          // MockFS SPIFFS (en RAM)
#include "ConfigStore.h"

static const char* PATH = "/config.txt";

// Helpers para nombres derivados
static std::string bak(const char* p){ return std::string(p) + ".bak"; }
static std::string tmp(const char* p){ return std::string(p) + ".tmp"; }

// setUp corre antes de CADA test: FS limpio + mutex creado vía begin().
void setUp(void) {
    SPIFFS.mockClear();
    ConfigStore::begin();   // crea el mutex recursivo (idempotente)
}
void tearDown(void) {}

// --- write() ---------------------------------------------------------------

void test_write_rechaza_json_invalido(void) {
    TEST_ASSERT_FALSE(ConfigStore::write(PATH, "no soy json {{{"));
    TEST_ASSERT_FALSE(SPIFFS.mockHas(PATH));     // no escribió nada
    TEST_ASSERT_FALSE(SPIFFS.mockHas(tmp(PATH).c_str()));  // limpió el .tmp
}

void test_write_crea_archivo_y_sin_residuos(void) {
    TEST_ASSERT_TRUE(ConfigStore::write(PATH, "{\"a\":1}"));
    TEST_ASSERT_TRUE(SPIFFS.mockHas(PATH));
    TEST_ASSERT_EQUAL_STRING("{\"a\":1}", SPIFFS.mockGet(PATH).c_str());
    TEST_ASSERT_FALSE(SPIFFS.mockHas(tmp(PATH).c_str()));  // .tmp se promovió, no quedó
}

void test_write_respalda_el_valor_anterior_en_bak(void) {
    ConfigStore::write(PATH, "{\"v\":1}");        // primera versión
    ConfigStore::write(PATH, "{\"v\":2}");        // segunda: la 1 debe ir a .bak
    TEST_ASSERT_EQUAL_STRING("{\"v\":2}", SPIFFS.mockGet(PATH).c_str());
    TEST_ASSERT_EQUAL_STRING("{\"v\":1}", SPIFFS.mockGet(bak(PATH).c_str()).c_str());
}

// --- read() ----------------------------------------------------------------

void test_read_devuelve_principal_valido(void) {
    SPIFFS.mockSet(PATH, "{\"ok\":true}");
    TEST_ASSERT_EQUAL_STRING("{\"ok\":true}", ConfigStore::read(PATH).c_str());
}

void test_read_inexistente_devuelve_vacio(void) {
    TEST_ASSERT_EQUAL_STRING("", ConfigStore::read("/nope.txt").c_str());
}

void test_read_principal_corrupto_recupera_bak(void) {
    SPIFFS.mockSet(PATH, "}} corrupto {{");          // principal inservible
    SPIFFS.mockSet(bak(PATH).c_str(), "{\"good\":1}"); // respaldo bueno
    TEST_ASSERT_EQUAL_STRING("{\"good\":1}", ConfigStore::read(PATH).c_str());
    // read() re-promueve el .bak al principal
    TEST_ASSERT_EQUAL_STRING("{\"good\":1}", SPIFFS.mockGet(PATH).c_str());
}

void test_read_principal_y_bak_corruptos_devuelve_vacio(void) {
    SPIFFS.mockSet(PATH, "basura");
    SPIFFS.mockSet(bak(PATH).c_str(), "mas basura");
    TEST_ASSERT_EQUAL_STRING("", ConfigStore::read(PATH).c_str());
}

// --- robustez: simular corte de energía dejando un .tmp colgado --------------

void test_tmp_colgado_no_afecta_lectura(void) {
    SPIFFS.mockSet(PATH, "{\"real\":1}");
    SPIFFS.mockSet(tmp(PATH).c_str(), "{\"a-medias\":");  // .tmp de una escritura interrumpida
    TEST_ASSERT_EQUAL_STRING("{\"real\":1}", ConfigStore::read(PATH).c_str());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_write_rechaza_json_invalido);
    RUN_TEST(test_write_crea_archivo_y_sin_residuos);
    RUN_TEST(test_write_respalda_el_valor_anterior_en_bak);
    RUN_TEST(test_read_devuelve_principal_valido);
    RUN_TEST(test_read_inexistente_devuelve_vacio);
    RUN_TEST(test_read_principal_corrupto_recupera_bak);
    RUN_TEST(test_read_principal_y_bak_corruptos_devuelve_vacio);
    RUN_TEST(test_tmp_colgado_no_afecta_lectura);
    return UNITY_END();
}
