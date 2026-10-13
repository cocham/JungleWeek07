#ifndef __LIB_KERNEL_LIST_H
#define __LIB_KERNEL_LIST_H

/* 이중 연결 리스트.
 *
 * 이 구현은 동적 메모리 할당을 필요로 하지 않는다. 대신 리스트 원소로
 * 사용할 구조체마다 struct list_elem 멤버를 포함해야 한다. 모든 리스트
 * 함수는 이 struct list_elem을 대상으로 동작한다. list_entry 매크로는
 * struct list_elem을 이를 포함하는 구조체 객체의 포인터로 변환한다.
 *
 * 예를 들어 struct foo의 리스트가 필요하다면 struct foo에 다음과 같이
 * struct list_elem 멤버를 포함해야 한다.
 *
 * struct foo {
 *   struct list_elem elem;
 *   int bar;
 *   ...기타 멤버...
 * };
 *
 * 그러면 struct foo의 리스트를 다음과 같이 선언하고 초기화할 수 있다.
 *
 * struct list foo_list;
 *
 * list_init (&foo_list);
 *
 * 리스트 순회는 struct list_elem을 이를 포함하는 구조체로 변환해야 하는
 * 대표적인 경우이다. foo_list를 사용하는 예는 다음과 같다.
 *
 * struct list_elem *e;
 *
 * for (e = list_begin (&foo_list); e != list_end (&foo_list);
 * e = list_next (e)) {
 *   struct foo *f = list_entry (e, struct foo, elem);
 *   ...f를 사용하는 작업...
 * }
 *
 * 실제 사용 예는 소스 곳곳에서 찾을 수 있다. 예를 들어 threads 디렉터리의
 * malloc.c, palloc.c, thread.c는 모두 리스트를 사용한다.
 *
 * 이 리스트의 인터페이스는 C++ STL의 list<> 템플릿을 참고했다.
 * list<>에 익숙하다면 쉽게 사용할 수 있다. 다만 자료형 검사는 전혀 하지
 * 않으며, 다른 정확성 검사도 제한적이므로 잘못 사용하면 문제가 발생한다.
 *
 * 리스트 용어:
 *
 * - "front"(첫 원소): 리스트의 첫 번째 원소. 빈 리스트에서는 정의되지
 *   않는다. list_front()가 반환한다.
 *
 * - "back"(마지막 원소): 리스트의 마지막 원소. 빈 리스트에서는 정의되지
 *   않는다. list_back()이 반환한다.
 *
 * - "tail"(꼬리 센티널): 마지막 원소 바로 뒤에 있는 경계 원소.
 *   빈 리스트에서도 정의된다. list_end()가 반환하며, 정방향 순회의
 *   종료 지점을 나타내는 센티널로 사용한다.
 *
 * - "beginning"(정방향 시작점): 비어 있지 않은 리스트에서는 첫 원소,
 *   빈 리스트에서는 꼬리 센티널이다. list_begin()이 반환하며,
 *   앞에서 뒤로 순회할 때 시작점으로 사용한다.
 *
 * - "head"(머리 센티널): 첫 원소 바로 앞에 있는 경계 원소.
 *   빈 리스트에서도 정의된다. list_rend()가 반환하며, 역방향 순회의
 *   종료 지점을 나타내는 센티널로 사용한다.
 *
 * - "reverse beginning"(역방향 시작점): 비어 있지 않은 리스트에서는
 *   마지막 원소, 빈 리스트에서는 머리 센티널이다. list_rbegin()이
 *   반환하며, 뒤에서 앞으로 순회할 때 시작점으로 사용한다.
 *
 * - "interior element"(내부 원소): 머리나 꼬리 센티널이 아닌 실제
 *   리스트 원소. 빈 리스트에는 내부 원소가 없다. */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* 리스트 원소. */
struct list_elem
{
   struct list_elem *prev; /* 이전 리스트 원소. */
   struct list_elem *next; /* 다음 리스트 원소. */
};

/* 리스트. */
struct list
{
   struct list_elem head; /* 리스트의 머리 센티널. */
   struct list_elem tail; /* 리스트의 꼬리 센티널. */
};

/* 리스트 원소 포인터 LIST_ELEM을 그 원소를 포함하는 구조체의 포인터로
   변환한다. 바깥 구조체의 이름 STRUCT와 리스트 원소의 멤버 이름 MEMBER를
   지정한다. 사용 예는 이 파일 상단의 긴 주석을 참고한다. */
#define list_entry(LIST_ELEM, STRUCT, MEMBER) \
   ((STRUCT *)((uint8_t *)&(LIST_ELEM)->next - offsetof(STRUCT, MEMBER.next)))

void list_init(struct list *);

/* 리스트 순회. */
struct list_elem *list_begin(struct list *);
struct list_elem *list_next(struct list_elem *);
struct list_elem *list_end(struct list *);

struct list_elem *list_rbegin(struct list *);
struct list_elem *list_prev(struct list_elem *);
struct list_elem *list_rend(struct list *);

struct list_elem *list_head(struct list *);
struct list_elem *list_tail(struct list *);

/* 리스트 삽입. */
void list_insert(struct list_elem *, struct list_elem *);
void list_splice(struct list_elem *before,
                 struct list_elem *first, struct list_elem *last);
void list_push_front(struct list *, struct list_elem *);
void list_push_back(struct list *, struct list_elem *);

/* 리스트 제거. */
struct list_elem *list_remove(struct list_elem *);
struct list_elem *list_pop_front(struct list *);
struct list_elem *list_pop_back(struct list *);

/* 리스트 원소 접근. */
struct list_elem *list_front(struct list *);
struct list_elem *list_back(struct list *);

/* 리스트 속성. */
size_t list_size(struct list *);
bool list_empty(struct list *);

/* 기타 연산. */
void list_reverse(struct list *);

/* 보조 데이터 AUX를 사용하여 두 리스트 원소 A와 B의 값을 비교한다. A가
   B보다 작으면 true, 크거나 같으면 false를 반환한다. */
typedef bool list_less_func(const struct list_elem *a,
                            const struct list_elem *b,
                            void *aux);

/* 정렬된 원소를 가진 리스트의 연산. */
void list_sort(struct list *,
               list_less_func *, void *aux);
void list_insert_ordered(struct list *, struct list_elem *,
                         list_less_func *, void *aux);
void list_unique(struct list *, struct list *duplicates,
                 list_less_func *, void *aux);

/* 최댓값과 최솟값. */
struct list_elem *list_max(struct list *, list_less_func *, void *aux);
struct list_elem *list_min(struct list *, list_less_func *, void *aux);

#endif /* lib/kernel/list.h 끝. */
