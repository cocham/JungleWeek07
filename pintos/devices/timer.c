#include "devices/timer.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include "threads/interrupt.h"
#include "threads/io.h"
#include "threads/synch.h"
#include "threads/thread.h"

/* See [8254] for hardware details of the 8254 timer chip. */

#if TIMER_FREQ < 19
#error 8254 timer requires TIMER_FREQ >= 19
#endif
#if TIMER_FREQ > 1000
#error TIMER_FREQ <= 1000 recommended
#endif

/* Number of timer ticks since OS booted. */
static int64_t ticks;

/* Number of loops per timer tick.
   Initialized by timer_calibrate(). */
static unsigned loops_per_tick;

static intr_handler_func timer_interrupt;
static bool too_many_loops (unsigned loops);
static void busy_wait (int64_t loops);
static void real_time_sleep (int64_t num, int32_t denom);

static struct list sleep_list;

/* Sets up the 8254 Programmable Interval Timer (PIT) to
   interrupt PIT_FREQ times per second, and registers the
   corresponding interrupt. */
void
timer_init (void) {
	/* 8254 input frequency divided by TIMER_FREQ, rounded to
	   nearest. */
	uint16_t count = (1193180 + TIMER_FREQ / 2) / TIMER_FREQ;

	outb (0x43, 0x34);    /* CW: counter 0, LSB then MSB, mode 2, binary. */
	outb (0x40, count & 0xff);
	outb (0x40, count >> 8);

	intr_register_ext (0x20, timer_interrupt, "8254 Timer");

	list_init(&sleep_list);
}

/* Calibrates loops_per_tick, used to implement brief delays. */
void
timer_calibrate (void) {
	unsigned high_bit, test_bit;

	ASSERT (intr_get_level () == INTR_ON);
	printf ("Calibrating timer...  ");

	/* Approximate loops_per_tick as the largest power-of-two
	   still less than one timer tick. */
	loops_per_tick = 1u << 10;
	while (!too_many_loops (loops_per_tick << 1)) {
		loops_per_tick <<= 1;
		ASSERT (loops_per_tick != 0);
	}

	/* Refine the next 8 bits of loops_per_tick. */
	high_bit = loops_per_tick;
	for (test_bit = high_bit >> 1; test_bit != high_bit >> 10; test_bit >>= 1)
		if (!too_many_loops (high_bit | test_bit))
			loops_per_tick |= test_bit;

	printf ("%'"PRIu64" loops/s.\n", (uint64_t) loops_per_tick * TIMER_FREQ);
}

/* Returns the number of timer ticks since the OS booted. */
int64_t
timer_ticks (void) {
	enum intr_level old_level = intr_disable ();
	int64_t t = ticks;
	intr_set_level (old_level);
	barrier ();
	return t;
}

/* Returns the number of timer ticks elapsed since THEN, which
   should be a value once returned by timer_ticks(). */
int64_t
timer_elapsed (int64_t then) {
	return timer_ticks () - then;
}

/* Suspends execution for approximately TICKS timer ticks. */
void
timer_sleep (int64_t ticks) { // 스레드가 특정 틱 동안 자는 것을 요청하는 함수이다.
	if (ticks > 0) // 0초 이하로 자는 것은 말이 되지 않으니 사전에 방지하는 것이다.
	{
		int64_t start = timer_ticks (); // timer_ticks는 컴퓨터가 켜진 뒤 지금까지 흐른 현재 시각을 뜻한다. (작업관리자에 있는 작동시간과 같은 것이다.)
		struct thread *cur_thread = thread_current(); // 자러 온 스레드의 정보를 가져와 cur_thread에 담음.

		ASSERT (intr_get_level () == INTR_ON); 
		/* 
		자러가도 되는 스레드인지 아닌지를 확인하는 것이다. 
		ASSERT는 안전 장치 같은 개념이다. 
		ASSERT 괄호 안에 있는 조건이 참이면 그대로 통과한다. 
		하지만, 거짓이라면 즉시 실행을 멈추고 컴퓨터를 뻗게 만든다. 
		intr 즉, 인터럽트는 가로채기이다. 
		CPU가 작업을 하고 있으면 가로채서 다른 작업을 시키는 것이다. 
		인터럽트가 INTR_ON이라는 것은 언제든 가로채도 된다는 뜻이다.
		자러가면 안 되는 스레드는 가로채는 것도 안 되기 때문에 'intr_get_level () == INTR_ON'로 구별이 가능하다.
		*/

		// while (timer_elapsed (start) < ticks) // timer_elapsed는 현재 시각 - 기준 시각(start)한 값이다. 타이머를 시작하고 얼머나 지났는지 알아내는 것이다.
		// {
			// thread_yield (); //thread_yield는 일을 안 하고 자러 가는 것이 아닌 줄 맨 뒤로 가서 다시 기다리는 것이기 때문에 CPU가 쉴 수 없는 환경이 만들어 지게 됨.
		// } 

		enum intr_level isIntr = intr_disable(); 
		/* 
		인터럽트를 잠시 INTR_OFF(가로채기 금지) 상태로 바꿔준다. 
		금지 상태로 만드는 이유는 sleep_list에 잔다고 명부를 적었는데 자려고 하는 찰나에 CPU를 가로채 버린다면, 
		CPU는 sleep_list를 보고 자고 있는 애라 착각하며 깨우는 작업을 하기 때문에 크래시가 일어남. 
		*/

		cur_thread -> tick_wakeup = start + ticks; // 일어날 시각(cur_thread -> tick_wakeup)에 현재 시각(start) + 잘 시간(ticks)을 넣는다.
		list_push_back(&sleep_list, &cur_thread -> elem); // elem은 cur_thread에 있는 고리이다. cur_thread를 다 가져오려면 너무 무겁기 때문에 가볍게 하기 위해 elem로 고리만 걸어 깨우는 리스트에 스레드 이름을 올린 것이다.
		
		thread_block(); // 스레드를 재움

		intr_set_level(isIntr); // 스레드가 일어났을 때 가로채기 금지(INTR_OFF)였던 걸 가로채기 가능(INTR_ON) 상태로 바꾸고 다른 작업을 이어 나간다.
	}
}

/* Suspends execution for approximately MS milliseconds. */
void
timer_msleep (int64_t ms) {
	real_time_sleep (ms, 1000);
}

/* Suspends execution for approximately US microseconds. */
void
timer_usleep (int64_t us) {
	real_time_sleep (us, 1000 * 1000);
}

/* Suspends execution for approximately NS nanoseconds. */
void
timer_nsleep (int64_t ns) {
	real_time_sleep (ns, 1000 * 1000 * 1000);
}

/* Prints timer statistics. */
void
timer_print_stats (void) {
	printf ("Timer: %"PRId64" ticks\n", timer_ticks ());
}

/* Timer interrupt handler. */
static void
timer_interrupt (struct intr_frame *args UNUSED) { // 하드웨어 타이머가 1초에 100번씩 울릴 때 마다 CPU가 자동으로 호출하는 함수이다. 1초에 100번 실행되는 함수인 것이다.
	ticks++; // 1틱씩 움직임.
	thread_tick(); // 일하고 있는 스레드가 CPU를 오래 독점하고 있는지 확인하는 함수.

	struct list_elem *l = list_begin(&sleep_list); // elem 고리에 달았던 깨우는 리스트의 고리를 잡음.

	while (l != list_end (&sleep_list)) // 리스트 끝에 도달할 때 까지 반복함.
	{
		struct thread *t = list_entry (l, struct thread, elem); // 잡은 고리(l)를 잡아 당겨 struct thread *t을 가져옴. (list_elem에는 깨울 시각이 적혀 있지 않아 struct thread를 가져와 깨울 시각을 알아냄.)

		if (t -> tick_wakeup <= ticks) // 깨울 시각을 가지고 깨울 시간이 되었는지 확인함.
		{
			l = list_remove(l); // 깨울 때가 되었으니 sleep_list에서 깨울 스레드의 고리(l)를 뺌.
			thread_unblock(t); // 스레드를 깨움.
		}
		else // 아직 깨울 시간이 안 되었다면?
		{
			l = list_next(l); // 다음 고리를 확인함. 
		}
	}
}

/* Returns true if LOOPS iterations waits for more than one timer
   tick, otherwise false. */
static bool
too_many_loops (unsigned loops) {
	/* Wait for a timer tick. */
	int64_t start = ticks;
	while (ticks == start)
		barrier ();

	/* Run LOOPS loops. */
	start = ticks;
	busy_wait (loops);

	/* If the tick count changed, we iterated too long. */
	barrier ();
	return start != ticks;
}

/* Iterates through a simple loop LOOPS times, for implementing
   brief delays.

   Marked NO_INLINE because code alignment can significantly
   affect timings, so that if this function was inlined
   differently in different places the results would be difficult
   to predict. */
static void NO_INLINE
busy_wait (int64_t loops) {
	while (loops-- > 0)
		barrier ();
}

/* Sleep for approximately NUM/DENOM seconds. */
static void
real_time_sleep (int64_t num, int32_t denom) {
	/* Convert NUM/DENOM seconds into timer ticks, rounding down.

	   (NUM / DENOM) s
	   ---------------------- = NUM * TIMER_FREQ / DENOM ticks.
	   1 s / TIMER_FREQ ticks
	   */
	int64_t ticks = num * TIMER_FREQ / denom;

	ASSERT (intr_get_level () == INTR_ON);
	if (ticks > 0) {
		/* We're waiting for at least one full timer tick.  Use
		   timer_sleep() because it will yield the CPU to other
		   processes. */
		timer_sleep (ticks);
	} else {
		/* Otherwise, use a busy-wait loop for more accurate
		   sub-tick timing.  We scale the numerator and denominator
		   down by 1000 to avoid the possibility of overflow. */
		ASSERT (denom % 1000 == 0);
		busy_wait (loops_per_tick * num / 1000 * TIMER_FREQ / (denom / 1000));
	}
}
