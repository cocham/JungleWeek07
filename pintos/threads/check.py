#!/usr/bin/env python3
import sys
import os
import subprocess
import glob

# ANSI Colors
GREEN = "\033[92m"
RED = "\033[91m"
YELLOW = "\033[93m"
CYAN = "\033[96m"
BOLD = "\033[1m"
RESET = "\033[0m"
DIM = "\033[2m"
CLEAR_LINE = "\r\033[K"

TEST_GROUPS = {
    "Alarm Clock": [
        ("alarm-single", "단일 스레드 수면 및 기상"),
        ("alarm-multiple", "여러 스레드 각자 다른 시간 수면/기상"),
        ("alarm-simultaneous", "여러 스레드 동시 수면/기상"),
        ("alarm-priority", "잠에서 깬 스레드의 우선순위 정렬"),
        ("alarm-zero", "0틱 수면 예외 처리"),
        ("alarm-negative", "음수 틱 수면 예외 처리"),
    ],
    "기본 우선순위 스케줄링": [
        ("priority-change", "실행 중 우선순위 변경 시 양보"),
        ("priority-preempt", "높은 우선순위 스레드 생성 시 즉시 선점"),
        ("priority-fifo", "동일 우선순위 스레드 간 FIFO 순서"),
        ("priority-sema", "세마포어 대기자 중 높은 우선순위부터 깨우기"),
        ("priority-condvar", "조건 변수 대기자 중 높은 우선순위부터 깨우기"),
    ],
    "우선순위 기부 (Priority Donation - 다음 파트)": [
        ("priority-donate-one", "단일 락 우선순위 기부"),
        ("priority-donate-multiple", "여러 락 우선순위 기부 1"),
        ("priority-donate-multiple2", "여러 락 우선순위 기부 2"),
        ("priority-donate-nest", "중첩 락 우선순위 기부"),
        ("priority-donate-sema", "세마포어와 기부 상호작용"),
        ("priority-donate-lower", "기부 상태에서 기본 우선순위 하향"),
        ("priority-donate-chain", "연쇄적 우선순위 기부 (A->B->C)"),
    ],
}

def setup_environment(build_dir):
    # Find pintos utils directory and add to PATH
    cur = build_dir
    while cur and cur != "/":
        utils_candidate = os.path.join(cur, "utils")
        if os.path.exists(utils_candidate):
            os.environ["PATH"] = utils_candidate + ":" + os.environ.get("PATH", "")
            break
        # also check pintos/utils
        pintos_utils = os.path.join(cur, "pintos", "utils")
        if os.path.exists(pintos_utils):
            os.environ["PATH"] = pintos_utils + ":" + os.environ.get("PATH", "")
            break
        parent = os.path.dirname(cur)
        if parent == cur: break
        cur = parent

def find_build_dir():
    current = os.path.abspath(os.getcwd())
    if os.path.basename(current) == "build" and os.path.exists("Makefile"):
        return current
    if os.path.exists(os.path.join(current, "build")):
        return os.path.join(current, "build")
    for cand in [
        os.path.join(current, "pintos", "threads", "build"),
        os.path.join(current, "threads", "build"),
        "/workspaces/pintos_22.04_lab_docker/pintos/threads/build"
    ]:
        if os.path.exists(cand):
            return cand
    return None

def run_test(build_dir, test_name):
    result_file = os.path.join(build_dir, f"tests/threads/{test_name}.result")
    output_file = os.path.join(build_dir, f"tests/threads/{test_name}.output")
    errors_file = os.path.join(build_dir, f"tests/threads/{test_name}.errors")

    # Clean old results so it always runs fresh
    for f in [result_file, output_file, errors_file]:
        if os.path.exists(f):
            try: os.remove(f)
            except: pass

    cmd = f"make tests/threads/{test_name}.result"
    
    subprocess.run(
        cmd,
        shell=True,
        cwd=build_dir,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        env=os.environ
    )
    
    if os.path.exists(result_file):
        with open(result_file, "r", encoding="utf-8", errors="ignore") as f:
            content = f.read().strip()
            if content.lower().startswith("pass"):
                return True, "PASS", ""
            else:
                lines = content.splitlines()
                reason = "출력 불일치 (순서 또는 내용 다름)"
                for line in lines:
                    if "Test output failed" in line:
                        reason = "출력 결과 불일치"
                        break
                    elif "no \"Timer: # ticks\" message" in line:
                        reason = "종료 안 됨 (시간 초과 또는 무한 대기)"
                        break
                    elif "Kernel PANIC" in line:
                        reason = "커널 패닉 발생"
                        break
                    elif "Run produced no output" in line:
                        reason = "실행 실패 (출력 없음)"
                        break
                return False, "FAIL", reason
    return False, "FAIL", "테스트 빌드/실행 실패"

def main():
    build_dir = find_build_dir()
    if not build_dir:
        print(f"{RED}[오류] threads/build 디렉토리를 찾을 수 없습니다.{RESET}")
        sys.exit(1)

    setup_environment(build_dir)

    arg = sys.argv[1].lower() if len(sys.argv) > 1 else "basic"
    
    selected_groups = {}
    if arg in ["all", "full"]:
        selected_groups = TEST_GROUPS
    elif arg in ["donation", "donate"]:
        selected_groups = {"우선순위 기부 (Priority Donation)": TEST_GROUPS["우선순위 기부 (Priority Donation - 다음 파트)"]}
    elif arg in ["alarm"]:
        selected_groups = {"Alarm Clock": TEST_GROUPS["Alarm Clock"]}
    elif arg in ["priority"]:
        selected_groups = {"기본 우선순위 스케줄링": TEST_GROUPS["기본 우선순위 스케줄링"]}
    else:
        matched_single = []
        for g_name, tests in TEST_GROUPS.items():
            for t_name, desc in tests:
                if arg in t_name.lower():
                    matched_single.append((t_name, desc))
        if matched_single and arg != "basic":
            selected_groups = {"단일 테스트": matched_single}
        else:
            selected_groups = {
                "Alarm Clock": TEST_GROUPS["Alarm Clock"],
                "기본 우선순위 스케줄링": TEST_GROUPS["기본 우선순위 스케줄링"]
            }

    print(f"\n{BOLD}{CYAN}╔══════════════════════════════════════════════════════════════════════╗{RESET}")
    print(f"{BOLD}{CYAN}║                    🏆 Pintos Project 1 Test Runner                   ║{RESET}")
    print(f"{BOLD}{CYAN}╚══════════════════════════════════════════════════════════════════════╝{RESET}\n")

    total_count = 0
    pass_count = 0

    for group_name, tests in selected_groups.items():
        print(f"{BOLD}📂 {group_name}{RESET}")
        print(f"{DIM}──────────────────────────────────────────────────────────────────────{RESET}")
        for test_name, desc in tests:
            total_count += 1
            print(f"  • {test_name:<26} : {DIM}⏳ 실행 중...{RESET}", end="", flush=True)
            passed, status, reason = run_test(build_dir, test_name)
            print(CLEAR_LINE, end="")
            
            if passed:
                pass_count += 1
                badge = f"{GREEN}{BOLD}PASS{RESET} ✅"
                print(f"  • {BOLD}{test_name:<26}{RESET} : [{badge}] {DIM}({desc}){RESET}")
            else:
                badge = f"{RED}{BOLD}FAIL{RESET} ❌"
                print(f"  • {BOLD}{test_name:<26}{RESET} : [{badge}] {YELLOW}({desc}){RESET}")
                if reason:
                    print(f"    {RED}↳ 💡 원인: {reason}{RESET}")
        print()

    rate = (pass_count / total_count * 100) if total_count > 0 else 0
    color = GREEN if rate == 100 else YELLOW if rate >= 70 else RED

    print(f"{DIM}══════════════════════════════════════════════════════════════════════{RESET}")
    print(f"{BOLD}📊 종합 결과: {color}{pass_count} / {total_count} 통과 ({rate:.1f}%){RESET}")
    if rate == 100:
        print(f"{GREEN}{BOLD}🎉 축하합니다! 모든 테스트를 완벽하게 통과했습니다!{RESET}")
    else:
        print(f"{CYAN}🎯 다음 목표: 위 FAIL 항목을 해결하면 됩니다!{RESET}")
    print(f"{DIM}══════════════════════════════════════════════════════════════════════{RESET}\n")

if __name__ == "__main__":
    main()
