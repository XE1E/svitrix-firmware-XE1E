#pragma once

/**
 * @file DateFormat.h
 * @brief Nombres de mes en español para strftime().
 *
 * El libc de ESP32 (newlib) no trae locale es_MX/es_ES: %b/%B en strftime()
 * siempre salen en inglés ("Aug"), aunque el resto del reloj ya está en
 * español (fases lunares, "DIA"/"DIAS"...). localizeMonthFormat() sustituye
 * esos especificadores por texto literal en español ANTES de llamar a
 * strftime(), que sí resuelve el resto del formato (%d, %m, %y...) sin
 * problema porque el texto sustituido no contiene '%'.
 *
 * Used by: DateApp (src/Apps), MenuManager (previsualización del formato).
 */

#include <Arduino.h>
#include <ctime>

/**
 * Devuelve `fmt` con %B (mes completo) y %b (mes abreviado) sustituidos por
 * su nombre en español para el mes de `lt`. El resultado se pasa a
 * strftime() para expandir el resto de especificadores.
 */
String localizeMonthFormat(const char *fmt, const struct tm *lt);
