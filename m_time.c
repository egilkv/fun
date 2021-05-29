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
static cell *tm2assoc(struct tm *tp) {
    cell *result = cell_assoc();
    assoc_set(result, cell_symbol("sec"),   cell_integer(tp->tm_sec)); // 0..60
    assoc_set(result, cell_symbol("min"),   cell_integer(tp->tm_min)); // 0..59
    assoc_set(result, cell_symbol("hour"),  cell_integer(tp->tm_hour)); // 0..23
    assoc_set(result, cell_symbol("mday"),  cell_integer(tp->tm_mday)); // 1..31
    assoc_set(result, cell_symbol("mon"),   cell_integer(1 + tp->tm_mon)); // 1..12
    assoc_set(result, cell_symbol("year"),  cell_integer(1900 + tp->tm_year)); // 1900..
    assoc_set(result, cell_symbol("wday"),  cell_integer(tp->tm_wday)); // 0..6, 0=sunday
    assoc_set(result, cell_symbol("yday"),  cell_integer(tp->tm_yday)); // 0..365
    if (tp->tm_isdst >= 0)
      assoc_set(result, cell_symbol("isdst"), cell_ref(tp->tm_isdst > 0 ? hash_t:hash_f));
    return result;
}

// calendar time UTC
static cell *ctime_utctime() {
    struct tm tm;
    time_t seconds = time((time_t *)0);
    gmtime_r(&seconds, &tm);

    return tm2assoc(&tm);
}

// calendar time local
static cell *ctime_localtime() {
    struct tm tm;
    time_t seconds = time((time_t *)0);
    localtime_r(&seconds, &tm);

    return tm2assoc(&tm);
}

cell *module_time() {
    if (!time_assoc) {
        time_assoc = cell_assoc();

        assoc_set(time_assoc, cell_symbol("seconds"),cell_cfun0(ctime_seconds));
        assoc_set(time_assoc, cell_symbol("localtime"),cell_cfun0(ctime_localtime));
        assoc_set(time_assoc, cell_symbol("utctime"),cell_cfun0(ctime_utctime));
    }
    // TODO static time_assoc owns one, hard to avoid
    return cell_ref(time_assoc);
}

