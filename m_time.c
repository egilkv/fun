/*  TAB-P
 *
 *  time functions
 *  non-pure functions
 *
 *  TODO for later:
 *      int nanosleep(const struct timespec *req, struct timespec *rem);
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
    seconds.dividend.fval = tv.tv_sec + tv.tv_usec / 1000000.0;
    seconds.divisor = 0;
    return cell_number(&seconds);
}

// calendar time UTC
static cell *tm2assoc(struct tm *tp, integer_t usec) {
    cell *result = cell_assoc();
    assoc_set(result, cell_symbol("sec"),   
              usec == 0 ? cell_integer(tp->tm_sec) : cell_real(tp->tm_sec + usec / 1000000.0)); // 0 .. 60.99
    assoc_set(result, cell_symbol("min"),   cell_integer(tp->tm_min)); // 0 .. 59
    assoc_set(result, cell_symbol("hour"),  cell_integer(tp->tm_hour)); // 0 .. 23
    assoc_set(result, cell_symbol("mday"),  cell_integer(tp->tm_mday)); // 1 .. 31
    assoc_set(result, cell_symbol("mon"),   cell_integer(1 + tp->tm_mon)); // 1 .. 12
    assoc_set(result, cell_symbol("year"),  cell_integer(1900 + tp->tm_year)); // 1900 ..
    assoc_set(result, cell_symbol("wday"),  cell_integer(tp->tm_wday)); // 0 .. 6, 0=sunday
    assoc_set(result, cell_symbol("yday"),  cell_integer(tp->tm_yday)); // 0 .. 365
    if (tp->tm_isdst >= 0)
      assoc_set(result, cell_symbol("isdst"), cell_ref(tp->tm_isdst > 0 ? hash_t:hash_f));
    return result;
}

// calendar time UTC
static int get_seconds(cell *args, struct timeval *tvp) {
    cell *a;
    if (list_pop(&args, &a)) {
        number secs;
        if (!get_number(a, &secs, args)) return 0;
        // TODO what about negative numbers?
	arg0(args);
        if (secs.divisor == 1) { // integer?
            tvp->tv_sec = secs.dividend.ival;
            tvp->tv_usec = 0;
        } else {
            number isecs = secs;
            make_float(&secs);
            if (!make_integer(&isecs)) return 0; // overflow

            tvp->tv_sec = isecs.dividend.ival; // rounded down
            tvp->tv_usec = (secs.dividend.fval - isecs.dividend.ival) * 1000000;
            if (tvp->tv_usec < 0) {
                // happens only with negative seconds
                tvp->tv_usec += 1000000;
                tvp->tv_sec -= 1;
            }
            assert(tvp->tv_usec >= 0 && tvp->tv_usec < 1000000);
        }
    } else {
        gettimeofday(tvp, (struct timezone *)0);
    }
    return 1;
}

// calendar time UTC
// one optional argument is seconds after, or before, epoch
// fractional seconds are supported
static cell *ctime_utctime(cell *args) {
    struct tm tm;
    struct timeval tv;
    if (!get_seconds(args, &tv)) return err_overflow(NIL);
    gmtime_r(&(tv.tv_sec), &tm);
    return tm2assoc(&tm, tv.tv_usec);
}

// calendar time local
static cell *ctime_localtime(cell *args) {
    struct tm tm;
    struct timeval tv;
    if (!get_seconds(args, &tv)) return err_overflow(NIL);
    localtime_r(&(tv.tv_sec), &tm);
    return tm2assoc(&tm, tv.tv_usec);
}

// seconds since epoch, with sub second accuracy
static cell *ctime_sleep(cell *a) {
    number secs;
    struct timespec ts;
    if (!get_number(a, &secs, NIL)) return 0;
    // TODO negative?
    if (secs.divisor == 1) { // integer?
        ts.tv_sec = secs.dividend.ival;
        ts.tv_nsec = 0;
    } else {
        number isecs = secs;
        make_float(&secs);
        if (!make_integer(&isecs)) return err_overflow(NIL); // overflow

        ts.tv_sec = isecs.dividend.ival; // rounded down
        ts.tv_nsec = (secs.dividend.fval - isecs.dividend.ival) * 1000000000;
        if (ts.tv_nsec < 0) {
            // happens only with negative seconds
            ts.tv_nsec += 1000000000;
            ts.tv_sec -= 1;
        }
        assert(ts.tv_nsec >= 0 && ts.tv_nsec < 1000000000);
    }
    if (ts.tv_sec < 0) return cell_void(); // TODO should there be an error message?

    {   // TODO will have to be redone
        struct timespec rem;
        while (nanosleep(&ts, &rem) == -1) {
            // assert(errno == EINTR);
            ts = rem;
        }
    }
    return cell_void();
}

cell *module_time() {
    if (!time_assoc) {
        time_assoc = cell_assoc();

        assoc_set(time_assoc, cell_symbol("seconds"),cell_cfun0(ctime_seconds));
        assoc_set(time_assoc, cell_symbol("sleep"),cell_cfun1(ctime_sleep));
	assoc_set(time_assoc, cell_symbol("localtime"),cell_cfunN(ctime_localtime));
	assoc_set(time_assoc, cell_symbol("utctime"),cell_cfunN(ctime_utctime));
    }
    // TODO static time_assoc owns one, hard to avoid
    return cell_ref(time_assoc);
}

