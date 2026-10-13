#include "list.h"
#include "../debug.h"

/* 이중 연결 리스트에는 두 개의 헤더 원소가 있다. 첫 원소 바로 앞의
   머리 센티널(head)과 마지막 원소 바로 뒤의 꼬리 센티널(tail)이다.
   머리 센티널의 prev와 꼬리 센티널의 next는 널이다. 나머지 두 링크는
   리스트의 내부 원소들을 통해 서로를 향한다.

   빈 리스트의 모습:

   +------+     +------+
   <---| head |<--->| tail |--->
   +------+     +------+

   원소가 두 개인 리스트의 모습:

   +------+     +-------+     +-------+     +------+
   <---| head |<--->|   1   |<--->|   2   |<--->| tail |<--->
   +------+     +-------+     +-------+     +------+

   이 대칭적인 배치는 리스트 처리의 여러 특수한 경우를 없애 준다.
   예를 들어 list_remove()에는 포인터 대입이 두 번만 필요하고 조건문은
   필요하지 않다. 헤더 원소가 없을 때보다 코드가 훨씬 간단하다.

   각 헤더 원소의 포인터 중 하나만 사용하므로 두 헤더를 하나로 합쳐도
   이런 단순함을 유지할 수 있다. 하지만 두 원소를 별도로 두면 일부
   연산에 유용한 검사를 추가할 수 있다. */

static bool is_sorted(struct list_elem *a, struct list_elem *b,
					  list_less_func *less, void *aux) UNUSED;

/* ELEM이 머리 센티널이면 true, 그렇지 않으면 false를 반환한다. */
static inline bool is_head(struct list_elem *elem)
{
	return elem != NULL && elem->prev == NULL && elem->next != NULL;
}

/* ELEM이 내부 원소이면 true, 그렇지 않으면 false를 반환한다. */
static inline bool is_interior(struct list_elem *elem)
{
	return elem != NULL && elem->prev != NULL && elem->next != NULL;
}

/* ELEM이 꼬리 센티널이면 true, 그렇지 않으면 false를 반환한다. */
static inline bool is_tail(struct list_elem *elem)
{
	return elem != NULL && elem->prev != NULL && elem->next == NULL;
}

/* LIST를 빈 리스트로 초기화한다. */
void list_init(struct list *list)
{
	ASSERT(list != NULL);
	list->head.prev = NULL;
	list->head.next = &list->tail;
	list->tail.prev = &list->head;
	list->tail.next = NULL;
}

/* LIST의 순회 시작점을 반환한다. */
struct list_elem *list_begin(struct list *list)
{
	ASSERT(list != NULL);
	return list->head.next;
}

/* 리스트에서 ELEM 다음 원소를 반환한다. ELEM이 마지막 원소이면 꼬리
   센티널을 반환한다. ELEM 자체가 꼬리 센티널이면 결과는 정의되지 않는다. */
struct list_elem *list_next(struct list_elem *elem)
{
	ASSERT(is_head(elem) || is_interior(elem));
	return elem->next;
}

/* LIST의 꼬리 센티널을 반환한다.

   list_end()는 리스트를 앞에서 뒤로 순회할 때 자주 사용한다. 사용 예는
   list.h 상단의 긴 주석을 참고한다. */
struct list_elem *list_end(struct list *list)
{
	ASSERT(list != NULL);
	return &list->tail;
}

/* LIST를 뒤에서 앞으로 순회하기 위한 역방향 순회 시작점을 반환한다. */
struct list_elem *list_rbegin(struct list *list)
{
	ASSERT(list != NULL);
	return list->tail.prev;
}

/* 리스트에서 ELEM 이전 원소를 반환한다. ELEM이 첫 원소이면 머리 센티널을
   반환한다. ELEM 자체가 머리 센티널이면 결과는 정의되지 않는다. */
struct list_elem *list_prev(struct list_elem *elem)
{
	ASSERT(is_interior(elem) || is_tail(elem));
	return elem->prev;
}

/* LIST의 머리 센티널을 반환한다.

   list_rend()는 리스트를 뒤에서 앞으로 순회할 때 자주 사용한다.
   list.h 상단의 예제를 따른 일반적인 사용법은 다음과 같다.

   for (e = list_rbegin (&foo_list); e != list_rend (&foo_list);
   e = list_prev (e))
   {
   struct foo *f = list_entry (e, struct foo, elem);
   ...f를 사용하는 작업...
   }
   */
struct list_elem *list_rend(struct list *list)
{
	ASSERT(list != NULL);
	return &list->head;
}

/* LIST의 머리 센티널을 반환한다.

   list_head()는 다음과 같은 다른 방식의 리스트 순회에도 사용할 수 있다.

   e = list_head (&list);
   while ((e = list_next (e)) != list_end (&list))
   {
   ...
   }
   */
struct list_elem *list_head(struct list *list)
{
	ASSERT(list != NULL);
	return &list->head;
}

/* LIST의 꼬리 센티널을 반환한다. */
struct list_elem *list_tail(struct list *list)
{
	ASSERT(list != NULL);
	return &list->tail;
}

/* BEFORE 바로 앞에 ELEM을 삽입한다. BEFORE는 내부 원소 또는 꼬리
   센티널이어야 한다. 꼬리 센티널 앞에 삽입하는 것은 list_push_back()과
   같다. */
void list_insert(struct list_elem *before, struct list_elem *elem)
{
	ASSERT(is_interior(before) || is_tail(before));
	ASSERT(elem != NULL);

	elem->prev = before->prev;
	elem->next = before;
	before->prev->next = elem;
	before->prev = elem;
}

/* 현재 리스트에서 FIRST부터 LAST 직전까지의 원소를 제거한 뒤 BEFORE 바로
   앞에 삽입한다. BEFORE는 내부 원소 또는 꼬리 센티널이어야 한다. */
void list_splice(struct list_elem *before,
				 struct list_elem *first, struct list_elem *last)
{
	ASSERT(is_interior(before) || is_tail(before));
	if (first == last)
		return;
	last = list_prev(last);

	ASSERT(is_interior(first));
	ASSERT(is_interior(last));

	/* 현재 리스트에서 FIRST부터 LAST까지의 구간을 분리한다. */
	first->prev->next = last->next;
	last->next->prev = first->prev;

	/* FIRST부터 LAST까지의 구간을 새 리스트에 연결한다. */
	first->prev = before->prev;
	last->next = before;
	before->prev->next = first;
	before->prev = last;
}

/* ELEM을 LIST의 맨 앞에 삽입하여 첫 원소가 되도록 한다. */
void list_push_front(struct list *list, struct list_elem *elem)
{
	list_insert(list_begin(list), elem);
}

/* ELEM을 LIST의 맨 뒤에 삽입하여 마지막 원소가 되도록 한다. */
void list_push_back(struct list *list, struct list_elem *elem)
{
	list_insert(list_end(list), elem);
}

/* 리스트에서 ELEM을 제거하고 그다음 원소를 반환한다.
   ELEM이 리스트에 없으면 동작은 정의되지 않는다.

   제거한 ELEM을 리스트에 속한 원소처럼 다루는 것은 안전하지 않다.
   특히 제거한 ELEM에 list_next()나 list_prev()를 사용하면 동작은
   정의되지 않는다. 따라서 다음과 같이 단순하게 순회하며 제거하면
   문제가 발생한다.

   ** 이렇게 사용하면 안 된다 **
   for (e = list_begin (&list); e != list_end (&list); e = list_next (e))
   {
   ...e를 사용하는 작업...
   list_remove (e);
   }
   ** 이렇게 사용하면 안 된다 **

   올바르게 리스트를 순회하며 원소를 제거하는 방법 중 하나는 다음과 같다.

   for (e = list_begin (&list); e != list_end (&list); e = list_remove (e))
   {
   ...e를 사용하는 작업...
   }

   리스트 원소를 free()로 해제해야 한다면 더 주의해야 한다.
   다음 방법은 이 경우에도 사용할 수 있다.

   while (!list_empty (&list))
   {
   struct list_elem *e = list_pop_front (&list);
   ...e를 사용하는 작업...
   }
   */
struct list_elem *list_remove(struct list_elem *elem)
{
	ASSERT(is_interior(elem));
	elem->prev->next = elem->next;
	elem->next->prev = elem->prev;
	return elem->next;
}

/* LIST의 첫 원소를 제거하여 반환한다. 제거 전에 LIST가 비어 있으면 동작은
   정의되지 않는다. */
struct list_elem *list_pop_front(struct list *list)
{
	struct list_elem *front = list_front(list);
	list_remove(front);
	return front;
}

/* LIST의 마지막 원소를 제거하여 반환한다. 제거 전에 LIST가 비어 있으면
   동작은 정의되지 않는다. */
struct list_elem *list_pop_back(struct list *list)
{
	struct list_elem *back = list_back(list);
	list_remove(back);
	return back;
}

/* LIST의 첫 원소를 반환한다. LIST가 비어 있으면 동작은 정의되지 않는다. */
struct list_elem *list_front(struct list *list)
{
	ASSERT(!list_empty(list));
	return list->head.next;
}

/* LIST의 마지막 원소를 반환한다. LIST가 비어 있으면 동작은 정의되지
   않는다. */
struct list_elem *list_back(struct list *list)
{
	ASSERT(!list_empty(list));
	return list->tail.prev;
}

/* LIST의 원소 수를 반환한다. 원소 수에 대해 O(n) 시간이 걸린다. */
size_t list_size(struct list *list)
{
	struct list_elem *e;
	size_t cnt = 0;

	for (e = list_begin(list); e != list_end(list); e = list_next(e))
		cnt++;
	return cnt;
}

/* LIST가 비어 있으면 true, 그렇지 않으면 false를 반환한다. */
bool list_empty(struct list *list)
{
	return list_begin(list) == list_end(list);
}

/* A와 B가 가리키는 struct list_elem * 값을 서로 바꾼다. */
static void swap(struct list_elem **a, struct list_elem **b)
{
	struct list_elem *t = *a;
	*a = *b;
	*b = t;
}

/* LIST의 원소 순서를 뒤집는다. */
void list_reverse(struct list *list)
{
	if (!list_empty(list))
	{
		struct list_elem *e;

		for (e = list_begin(list); e != list_end(list); e = e->prev)
			swap(&e->prev, &e->next);
		swap(&list->head.next, &list->tail.prev);
		swap(&list->head.next->prev, &list->tail.prev->next);
	}
}

/* 보조 데이터 AUX를 사용하는 LESS를 기준으로 A부터 B 직전까지의 리스트
   원소가 정렬되어 있을 때만 true를 반환한다. */
static bool is_sorted(struct list_elem *a, struct list_elem *b,
					  list_less_func *less, void *aux)
{
	if (a != b)
		while ((a = list_next(a)) != b)
			if (less(a, list_prev(a), aux))
				return false;
	return true;
}

/* A에서 시작하여 B를 넘지 않는 비내림차순 연속 구간을 찾는다. 정렬 기준은
   보조 데이터 AUX를 사용하는 LESS이다. 구간 끝의 다음 원소를 반환한다.
   A부터 B 직전까지의 범위는 비어 있지 않아야 한다. */
static struct list_elem *find_end_of_run(struct list_elem *a, struct list_elem *b,
										 list_less_func *less, void *aux)
{
	ASSERT(a != NULL);
	ASSERT(b != NULL);
	ASSERT(less != NULL);
	ASSERT(a != b);

	do
	{
		a = list_next(a);
	} while (a != b && !less(a, list_prev(a), aux));
	return a;
}

/* A0부터 A1B0 직전까지의 구간과 A1B0부터 B1 직전까지의 구간을 병합하여 B1
   직전에서 끝나는 하나의 구간으로 만든다. 두 입력 구간은 비어 있지 않아야
   하며, 보조 데이터 AUX를 사용하는 LESS를 기준으로 비내림차순 정렬되어
   있어야 한다. 결과 구간도 같은 기준으로 정렬된다. */
static void inplace_merge(struct list_elem *a0, struct list_elem *a1b0,
						  struct list_elem *b1,
						  list_less_func *less, void *aux)
{
	ASSERT(a0 != NULL);
	ASSERT(a1b0 != NULL);
	ASSERT(b1 != NULL);
	ASSERT(less != NULL);
	ASSERT(is_sorted(a0, a1b0, less, aux));
	ASSERT(is_sorted(a1b0, b1, less, aux));

	while (a0 != a1b0 && a1b0 != b1)
		if (!less(a1b0, a0, aux))
			a0 = list_next(a0);
		else
		{
			a1b0 = list_next(a1b0);
			list_splice(a0, list_prev(a1b0), a1b0);
		}
}

/* 보조 데이터 AUX를 사용하는 LESS를 기준으로 LIST를 정렬한다. 자연 반복
   병합 정렬을 사용하며, LIST의 원소 수에 대해 시간 복잡도는 O(n lg n),
   공간 복잡도는 O(1)이다. */
void list_sort(struct list *list, list_less_func *less, void *aux)
{
	size_t output_run_cnt; /* 이번 순회에서 출력한 정렬 구간 수. */

	ASSERT(list != NULL);
	ASSERT(less != NULL);

	/* 비내림차순인 인접 구간을 병합하며 리스트를 반복 순회한다. 정렬
	   구간이 하나만 남을 때까지 계속한다. */
	do
	{
		struct list_elem *a0;	/* 첫 번째 정렬 구간의 시작. */
		struct list_elem *a1b0; /* 첫 번째 구간의 끝이자 두 번째 구간의 시작. */
		struct list_elem *b1;	/* 두 번째 정렬 구간의 끝. */

		output_run_cnt = 0;
		for (a0 = list_begin(list); a0 != list_end(list); a0 = b1)
		{
			/* 반복할 때마다 하나의 병합 결과 구간이 만들어진다. */
			output_run_cnt++;

			/* 서로 인접한 두 비내림차순 구간 A0...A1B0와 A1B0...B1을
			   찾는다. */
			a1b0 = find_end_of_run(a0, list_end(list), less, aux);
			if (a1b0 == list_end(list))
				break;
			b1 = find_end_of_run(a1b0, list_end(list), less, aux);

			/* 두 정렬 구간을 병합한다. */
			inplace_merge(a0, a1b0, b1, less, aux);
		}
	} while (output_run_cnt > 1);

	ASSERT(is_sorted(list_begin(list), list_end(list), less, aux));
}

/* 보조 데이터 AUX를 사용하는 LESS를 기준으로 정렬된 LIST의 올바른 위치에
   ELEM을 삽입한다. 평균적으로 LIST의 원소 수에 대해 O(n) 시간이 걸린다. */
void list_insert_ordered(struct list *list, struct list_elem *elem,
						 list_less_func *less, void *aux)
{
	struct list_elem *e;

	ASSERT(list != NULL);
	ASSERT(elem != NULL);
	ASSERT(less != NULL);

	for (e = list_begin(list); e != list_end(list); e = list_next(e))
		if (less(elem, e, aux))
			break;
	return list_insert(e, elem);
}

/* LIST를 순회하면서, 보조 데이터 AUX를 사용하는 LESS를 기준으로 같은 값인
   인접 원소들 중 첫 번째만 남기고 제거한다. DUPLICATES가 널이 아니면
   제거한 원소들을 DUPLICATES의 끝에 추가한다. */
void list_unique(struct list *list, struct list *duplicates,
				 list_less_func *less, void *aux)
{
	struct list_elem *elem, *next;

	ASSERT(list != NULL);
	ASSERT(less != NULL);
	if (list_empty(list))
		return;

	elem = list_begin(list);
	while ((next = list_next(elem)) != list_end(list))
		if (!less(elem, next, aux) && !less(next, elem, aux))
		{
			list_remove(next);
			if (duplicates != NULL)
				list_push_back(duplicates, next);
		}
		else
			elem = next;
}

/* 보조 데이터 AUX를 사용하는 LESS를 기준으로 LIST의 최댓값 원소를
   반환한다. 최댓값 원소가 여러 개이면 리스트에서 먼저 나타나는 것을
   반환한다. 리스트가 비어 있으면 꼬리 센티널을 반환한다. */
struct list_elem *list_max(struct list *list, list_less_func *less, void *aux)
{
	struct list_elem *max = list_begin(list);
	if (max != list_end(list))
	{
		struct list_elem *e;

		for (e = list_next(max); e != list_end(list); e = list_next(e))
			if (less(max, e, aux))
				max = e;
	}
	return max;
}

/* 보조 데이터 AUX를 사용하는 LESS를 기준으로 LIST의 최솟값 원소를
   반환한다. 최솟값 원소가 여러 개이면 리스트에서 먼저 나타나는 것을
   반환한다. 리스트가 비어 있으면 꼬리 센티널을 반환한다. */
struct list_elem *list_min(struct list *list, list_less_func *less, void *aux)
{
	struct list_elem *min = list_begin(list);
	if (min != list_end(list))
	{
		struct list_elem *e;

		for (e = list_next(min); e != list_end(list); e = list_next(e))
			if (less(e, min, aux))
				min = e;
	}
	return min;
}
