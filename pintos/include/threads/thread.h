#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include "threads/interrupt.h"
#ifdef VM
#include "vm/vm.h"
#endif

/* 스레드 생명 주기의 상태. */
enum thread_status
{
	THREAD_RUNNING, /* 실행 중인 스레드. */
	THREAD_READY,   /* 실행 중이지 않지만 실행 준비가 된 상태. */
	THREAD_BLOCKED, /* 이벤트 발생을 기다리는 상태. */
	THREAD_DYING    /* 곧 제거될 상태. */
};

/* 스레드 식별자 자료형. 원하는 자료형으로 재정의할 수 있다. */
typedef int tid_t;
#define TID_ERROR ((tid_t) - 1) /* tid_t의 오류 값. */

/* 스레드 우선순위. */
#define PRI_MIN 0      /* 가장 낮은 우선순위. */
#define PRI_DEFAULT 31 /* 기본 우선순위. */
#define PRI_MAX 63     /* 가장 높은 우선순위. */

/* 커널 스레드 또는 사용자 프로세스.
 *
 * 각 스레드 구조체는 전용 4 kB 페이지에 저장된다. 구조체 자체는
 * 페이지의 가장 낮은 주소(오프셋 0)에 위치한다. 페이지의 나머지
 * 공간은 스레드의 커널 스택으로 예약되며, 스택은 페이지의 가장
 * 높은 주소(오프셋 4 kB)에서 아래쪽으로 자란다. 배치는 다음과 같다.
 *
 *      4 kB +---------------------------------+
 *           |            커널 스택            |
 *           |                |                |
 *           |                |                |
 *           |                V                |
 *           |         아래 방향으로 증가      |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           +---------------------------------+
 *           |              magic              |
 *           |            intr_frame           |
 *           |                :                |
 *           |                :                |
 *           |               name              |
 *           |              status             |
 *      0 kB +---------------------------------+
 *
 * 이 배치에서 주의할 점은 두 가지이다.
 *
 *    1. struct thread가 너무 커지면 커널 스택 공간이 부족해진다.
 *       기본 struct thread는 크기가 작으며, 1 kB보다 충분히 작은
 *       크기를 유지하는 것이 좋다.
 *
 *    2. 커널 스택도 너무 커져서는 안 된다. 스택 오버플로가 발생하면
 *       스레드 상태가 손상된다. 따라서 커널 함수는 큰 구조체나 배열을
 *       정적이지 않은 지역 변수로 할당하지 않아야 한다. 대신 malloc()
 *       또는 palloc_get_page()를 사용하여 동적으로 할당한다.
 *
 * 두 문제의 첫 증상은 대개 thread_current()의 단언 실패로 나타난다.
 * 이 함수는 실행 중인 스레드의 struct thread에서 magic 멤버가
 * THREAD_MAGIC인지 검사한다. 스택 오버플로는 보통 이 값을 변경하여
 * 단언 실패를 일으킨다. */
/* elem 멤버는 실행 큐(thread.c)의 원소 또는 세마포어 대기 목록(synch.c)의
 * 원소로 사용된다. 두 용도가 상호 배타적이므로 하나의 멤버를 공유할 수
 * 있다. 준비 상태의 스레드만 실행 큐에 들어가며, 차단 상태의 스레드만
 * 세마포어 대기 목록에 들어간다. */
struct thread
{
	/* thread.c에서 관리한다. */
	tid_t tid;                 /* 스레드 식별자. */
	enum thread_status status; /* 스레드 상태. */
	char name[16];             /* 디버깅용 이름. */
	int priority;              /* 우선순위. */
	int64_t wakeup_tick;       /* 깨어날 절대 틱. */

	/* thread.c와 synch.c에서 공유한다. */
	struct list_elem elem; /* 리스트 원소. */

#ifdef USERPROG
	/* userprog/process.c에서 관리한다. */
	uint64_t *pml4; /* 4단계 페이지 맵. */
#endif
#ifdef VM
	/* 스레드가 소유한 전체 가상 메모리의 테이블. */
	struct supplemental_page_table spt;
#endif

	/* thread.c에서 관리한다. */
	struct intr_frame tf; /* 문맥 전환 정보. */
	unsigned magic;       /* 스택 오버플로를 감지한다. */
};

void thread_sleep(int64_t wakeupt_tick);
void thread_awake(int64_t current_tick);

/* false이면 기본값인 라운드 로빈 스케줄러를 사용한다. true이면 다단계
   피드백 큐 스케줄러를 사용한다. 커널 명령행 옵션 "-o mlfqs"로 제어한다. */
extern bool thread_mlfqs;

void thread_init(void);
void thread_start(void);

void thread_tick(void);
void thread_print_stats(void);

typedef void thread_func(void *aux);
tid_t thread_create(const char *name, int priority, thread_func *, void *);

void thread_block(void);
void thread_unblock(struct thread *);

struct thread *thread_current(void);
tid_t thread_tid(void);
const char *thread_name(void);

void thread_exit(void) NO_RETURN;
void thread_yield(void);

int thread_get_priority(void);
void thread_set_priority(int);

int thread_get_nice(void);
void thread_set_nice(int);
int thread_get_recent_cpu(void);
int thread_get_load_avg(void);

void do_iret(struct intr_frame *tf);

#endif /* threads/thread.h 끝. */
