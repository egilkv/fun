/*  TAB-P
 *
 *  time functions
 *  non-pure functions
 */

#include <time.h>
#include <sys/time.h>
#include <assert.h>

#include "m_time.h"
#include "cmod.h"
#include "number.h"

static cell *time_assoc = NIL;

// seconds since epoch, with sub second accuracy
static cell *ctime_seconds() {
    number seconds;
    struct timeval tv;
    gettimeofday(&tv, (struct timezone *)0);
    seconds.dividend.fval = tv.tv_sec + tv.tv_usec * 0.000001;
    seconds.divisor = 0;
    return cell_number(&seconds);
}

// calendar time UTC
static cell *ctime_utctime() {
    cell *result;
    struct tm tm;
    time_t seconds = time((time_t *)0);
    gmtime_r(&seconds, &tm);

    result = cell_assoc();
    assoc_set(result, cell_symbol("sec"),   cell_integer(tm.tm_sec)); // 0..60
    assoc_set(result, cell_symbol("min"),   cell_integer(tm.tm_min)); // 0..59
    assoc_set(result, cell_symbol("hour"),  cell_integer(tm.tm_hour)); // 0..23
    assoc_set(result, cell_symbol("mday"),  cell_integer(tm.tm_mday)); // 1..31
    assoc_set(result, cell_symbol("mon"),   cell_integer(1 + tm.tm_mon)); // 1..12
    assoc_set(result, cell_symbol("year"),  cell_integer(1900 + tm.tm_year)); // 1900..
    assoc_set(result, cell_symbol("wday"),  cell_integer(tm.tm_wday)); // 0..6, 0=sunday
    assoc_set(result, cell_symbol("yday"),  cell_integer(tm.tm_yday)); // 0..365
//  if (tm.tm_isdst >= 0)
//    assoc_set(result, cell_symbol("isdst"), cell_ref(tm.tm_isdst > 0 ? hash_t:hash_f));
    return result;
}

cell *module_time() {
    if (!time_assoc) {
        time_assoc = cell_assoc();

        assoc_set(time_assoc, cell_symbol("seconds"),cell_cfun0(ctime_seconds));
        assoc_set(time_assoc, cell_symbol("utctime"),cell_cfun0(ctime_utctime));
    }
    // TODO static time_assoc owns one, hard to avoid
    return cell_ref(time_assoc);
}

