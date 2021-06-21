/*  TAB-P
 *
 *  critical regions
 *
 *  this is for modern GCC
 *  see also: #ifdef __GNUC__
 *
 *  https://gcc.gnu.org/onlinedocs/gcc-4.1.1/gcc/Atomic-Builtins.html
 *  https://attractivechaos.wordpress.com/2011/10/06/multi-threaded-programming-efficiency-of-locking/
 *
 *  TODO see also
 *      <stdatomic.h>
 *      __sync_add_and_fetch(&(node->ref), 1);
 *      __sync_sub_and_fetch(&(node->ref), 1);
 *
 */

#ifdef __GNUC__
#define LOCK(lockvar)   while (__sync_lock_test_and_set(&(lockvar), 1));

#define UNLOCK(lockvar)  __sync_lock_release(&(lockvar));
#else

#error Needs mutex

#endif
