/*  TAB-P
 *
 *  time functions
 *  utctime(s) and possibly localtime(s) are pure functions
 *
 *  non-pure functions
 *
 *  TODO for later: fix sleep
 */
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#include "m_time.h"
#include "cmod.h"
#include "number.h"
#include "node.h"
#include "run.h"
#include "compile.h"
#include "err.h"

 // seconds since epoch, with sub second accuracy
static cell *ctime_seconds(cell *args) {
    number seconds;
    struct timeval tv;
    arg0(args);
    gettimeofday(&tv, (struct timezone *)0);
    seconds.dividend.fval = tv.tv_sec + tv.tv_usec / 1000000.0;
    seconds.divisor = 0;
    return cell_number(&seconds);
}

// get one item
static int getkey(cell *a, const char *nam, int *setp) {
    cell *value = NIL;
    cell *key = cell_symbol(nam);
    integer_t i;
    if (!assoc_get(a, key, &value)) {
        cell_unref(error_rt1("missing entry for", key));
        return 0;
    }
    cell_unref(key);
    if (!get_any_integer(value, &i, NIL)) {
        return 0;
    }
    // integer conversion overflow
    if (i > INT_MAX || i < (INT_MIN+1900)) {
        cell_unref(err_overflow(NIL));
        return 0;
    }
    *setp = i;
    return 1;
}

// pick values from assoc
// do not unref assoc if OK
static int assoc2tm(cell *a, struct tm *tp) {
    memset(tp, 0, sizeof(struct tm));
    tp->tm_isdst = -1; // means figure out DST
    // TODO deal with seconds separately, adding fraction
    if (!cell_is_assoc(a)) {
        cell_unref(error_rt1("not an assoc", a));
        return 0;
    }
    if (!getkey(a, "sec", &(tp->tm_sec))
     || !getkey(a, "min", &(tp->tm_min))
     || !getkey(a, "hour", &(tp->tm_hour))
     || !getkey(a, "mday", &(tp->tm_mday))
     || !getkey(a, "mon", &(tp->tm_mon))
     || !getkey(a, "year", &(tp->tm_year))) {
    }
    {
        cell *isdst = cell_symbol("isdst");
        cell *value = NIL;
        if (assoc_get(a, isdst, &value)) {
            if (value == hash_t) tp->tm_isdst = 1;
            else if (value == hash_f) tp->tm_isdst = 0;
            // TODO complain if not void
            cell_unref(value);
        }
        cell_unref(isdst);
    }
    tp->tm_mon -= 1; // unix uses 0..11
    tp->tm_year -= 1900; // unix again
    return 1;
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
      assoc_set(result, cell_symbol("isdst"), cell_boolean(tp->tm_isdst > 0));
    return result;
}

// get seconds as argument to struct timeval
static int get_seconds(cell *a, struct timeval *tvp) {
    number secs;
    if (!get_number(a, &secs, NIL)) return 0;
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
    return 1;
}

// calendar time UTC
// one optional argument is seconds after, or before, epoch
// fractional seconds are supported
static cell *ctime_utctime(cell *a) {
    struct tm tm;
    struct timeval tv;
    if (!get_seconds(a, &tv)) return err_overflow(NIL);
    gmtime_r(&(tv.tv_sec), &tm);
    return tm2assoc(&tm, tv.tv_usec);
}

// calendar time local
static cell *ctime_localtime(cell *a) {
    struct tm tm;
    struct timeval tv;
    if (!get_seconds(a, &tv)) return err_overflow(NIL);
    localtime_r(&(tv.tv_sec), &tm);
    return tm2assoc(&tm, tv.tv_usec);
}

// convert calendar time local to seconds from epoch
static cell *ctime_mktime(cell *a) {
    struct tm tm;
    time_t t;
    if (!assoc2tm(a, &tm)) {
        return cell_error();
    }
    if ((t = mktime(&tm)) == (time_t) -1) {
        return error_rt1("cannot convert from calendar time", a);
    }
    cell_unref(a);
    // TODO add fractional second
    return cell_integer(t);
}

// measure time required for evaluating all arguments
static cell *ctime_time(cell *args) {
    cell *f;
    if (!list_pop(&args, &f)) {
        return error_rt1("function to time expected", args);
    }
        // TODO no environment
        // TODO re-invocation of run_main() is questionable
#if 0
        // TODO instead, compile this:
        //     push negative time
        //     call cell_func(f, args)
        //     discard value
        //     push positive time
        //     call subtract
               // push negative time
               cell_func(cell_ref(hash_minus), 
               cell_list(cell_cfunN(ctime_seconds), NIL));
               cell_func(cell_ref(hash_minus), cell_list(cell_cfunN(ctime_seconds), NIL));
               call cell_func(f, args)


#else
    // TODO have to make silly program
    cell *prog = NIL;
    cell **pp = &prog;

    *pp = newnode(c_DOQPUSH);
    (*pp)->_.cons.car = args;
    pp = &(*pp)->_.cons.cdr;

    *pp = newnode(c_DOQPUSH);
    (*pp)->_.cons.car = f;
    pp = &(*pp)->_.cons.cdr;

    *pp = newnode(c_DOAPPLY);

    number secs;
    struct timeval tv;
    gettimeofday(&tv, (struct timezone *)0);
    secs.dividend.fval = tv.tv_sec + tv.tv_usec / 1000000.0;
    secs.divisor = 0;

    run_main_force(prog, NIL, NIL);

    gettimeofday(&tv, (struct timezone *)0);
    secs.dividend.fval = (tv.tv_sec + tv.tv_usec / 1000000.0) - secs.dividend.fval;
    return cell_number(&secs);
#endif
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
    // TODO consider having a cache for this
    cell *a = cell_assoc();

    assoc_set(a, cell_symbol("seconds"),   cell_cfunN(ctime_seconds));
    assoc_set(a, cell_symbol("sleep"),     cell_cfun1(ctime_sleep));
    assoc_set(a, cell_symbol("localtime"), cell_cfun1_pure(ctime_localtime)); // TODO purish
    assoc_set(a, cell_symbol("mktime"),    cell_cfun1_pure(ctime_mktime));    // TODO purish
    assoc_set(a, cell_symbol("time"),      cell_cfunN(ctime_time));
    assoc_set(a, cell_symbol("utctime"),   cell_cfun1_pure(ctime_utctime));

    return a;
}

