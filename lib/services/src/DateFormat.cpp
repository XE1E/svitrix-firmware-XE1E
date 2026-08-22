#include "DateFormat.h"

static const char *const kMonthAbbr[12] = {
    "Ene", "Feb", "Mar", "Abr", "May", "Jun",
    "Jul", "Ago", "Sep", "Oct", "Nov", "Dic"};

static const char *const kMonthFull[12] = {
    "Enero", "Febrero", "Marzo", "Abril", "Mayo", "Junio",
    "Julio", "Agosto", "Septiembre", "Octubre", "Noviembre", "Diciembre"};

String localizeMonthFormat(const char *fmt, const struct tm *lt)
{
    String out(fmt);
    int mon = lt->tm_mon % 12;
    if (mon < 0)
        mon += 12;
    out.replace("%B", kMonthFull[mon]);
    out.replace("%b", kMonthAbbr[mon]);
    return out;
}
