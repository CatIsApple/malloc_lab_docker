# Report 03 · Implicit first-fit 구현과 테스트

측정일: 2026-10-07 · 상태: 측정 완료

[Notion Report](https://app.notion.com/p/3f277d1c02cd81979b0ec56463b33101)

**기본 Trace 11개와 short Trace 2개 모두 통과. Perf index 74/100.**

## 1. 현재 구현

초기 저장소 커밋 `85ffba3` 대비 현재 작업 내용을 기록한다.

- 헤더·푸터와 블록 이동 매크로를 추가했다.
- `mm_init`에서 정렬 패딩·프롤로그·에필로그를 구성하고 초기 힙을 확장한다.
- `extend_heap`은 정렬된 가용 블록을 추가하고 인접 가용 블록과 병합한다.
- `find_fit`은 implicit free list를 순서대로 탐색하는 first-fit 방식이다.
- `place`는 남는 공간이 최소 블록 크기 16바이트 이상이면 분할한다.
- `mm_malloc`은 요청 크기를 정렬하고, 탐색·배치 또는 힙 확장을 수행한다.
- `mm_free`는 블록을 가용 상태로 바꾸고 `coalesce`를 호출한다.
- `memlib.c`에는 힙 초기 할당 실패 시 포인터를 출력하는 기존 진단 코드가 포함됐다.
- `mm_realloc` 본문은 초기 구현이 유지됐다. 이번 회차에서 별도로 구현한 항목은 아니다.

## 2. 실행

Docker DevContainer `aadecfcd865b`, Linux aarch64, GCC 15.2.0, `-Wall -O2 -g` 환경에서 실행했다.
실행 디렉터리는 `/workspaces/malloc_lab_docker/malloc-lab`이다.

```sh
make -B
./mdriver -V -g
./mdriver -V -f short1-bal.rep
./mdriver -V -f short2-bal.rep
```

네 명령 모두 종료 코드 0. 빌드 경고는 수정하지 않았으며 [빌드 로그](attempt-03/build.log)에 보존했다.
측정 대상 소스의 SHA-256은 [실행 기록](attempt-03/run.json)에 포함했다.

## 3. 결과

| 기본 Trace 통과 | 실패 | 전체 util | 전체 Kops | util 점수 | thru 점수 | Perf index |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 11/11 | 0 | 74% | 449 | 44 | 30 | 74/100 |

| Trace | valid | util | ops | secs | Kops |
| --- | --- | ---: | ---: | ---: | ---: |
| amptjp | yes | 99% | 5,694 | 0.002988 | 1,906 |
| cccp | yes | 99% | 5,848 | 0.002842 | 2,058 |
| cp-decl | yes | 99% | 6,648 | 0.004309 | 1,543 |
| expr | yes | 100% | 5,380 | 0.003188 | 1,688 |
| coalescing | yes | 66% | 14,400 | 0.000044 | 328,018 |
| random | yes | 92% | 4,800 | 0.002741 | 1,751 |
| random2 | yes | 92% | 4,800 | 0.002507 | 1,915 |
| binary | yes | 55% | 12,000 | 0.071901 | 167 |
| binary2 | yes | 51% | 24,000 | 0.128827 | 186 |
| realloc | yes | 27% | 14,401 | 0.029885 | 482 |
| realloc2 | yes | 34% | 14,401 | 0.001027 | 14,026 |

기본 Trace 합계는 112,372 operations, 0.250257초다. 표의 수치는 드라이버 출력의 반올림된 값이다.

```text
Perf index = 44 (util) + 30 (thru) = 74/100
correct:11
perfidx:74
```

| 추가 테스트 | valid | ops | 결과 |
| --- | --- | ---: | --- |
| short1-bal.rep | yes | 12 | 통과 |
| short2-bal.rep | yes | 12 | 통과 |

short 테스트는 별도 실행했으며 74점 산정에 포함되지 않는다.

## 4. 관찰

- 1회차의 기본 Trace 결과는 6개 통과·5개 실패였고, 이번 회차는 11개 모두 통과했다.
- 2회차는 Trace 시작 전 종료되어 미측정이었다. 이번 회차는 전체 실행을 완료했지만 2회차와 성능 수치를 비교할 수는 없다.
- 2회차 기록과 이번 회차의 `mm.c` SHA-256은 같다. 실행 성공의 원인을 새로운 allocator 변경으로 단정하지 않는다.
- 활용도는 `expr` 100%, `amptjp`·`cccp`·`cp-decl` 99%였다. `realloc`·`realloc2`는 각각 27%·34%였다.
- `binary`·`binary2` 처리량은 각각 167·186 Kops였다.
- 이번 결과는 제공된 Trace 실행에 대한 검증이다. 기존 `mm_realloc`의 크기 읽기 방식은 새 헤더·푸터 구조에 맞춘 수정이 없어 구현 이슈 #24는 완료로 처리하지 않는다.
- 성능 값은 이번 1회 실행의 측정치이며 실행 환경에 따라 변할 수 있다.

## 5. 관련 이슈

| 구분 | 이슈 | 확인 내용 |
| --- | --- | --- |
| 구현 | #14, #15, #16, #17, #18, #19, #23 | 매크로, 초기화, 확장, 해제, 병합, 할당, 배치 구현 |
| first fit | #20 | 현재 구현에서 확인, 이미 닫힌 이슈 |
| short 테스트 | #25, #26 | 두 Trace 모두 valid yes |
| 기본 테스트 | #27–#37 | 기본 Trace 11개 모두 valid yes |
| 성능 측정 | #38, #39 | util 44점, thru 30점 측정; 별도 목표 점수는 이슈에 명시되지 않음 |
| 미완료·선택 사항 | #21, #22, #24 | next fit·explicit/seglist 미적용, realloc 별도 구현 미완료 |

## 실행 원문

- [빌드](attempt-03/build.log)
- [기본 Trace 11개](attempt-03/default-traces.log)
- [short1](attempt-03/short1.log)
- [short2](attempt-03/short2.log)
- [환경·소스 해시·구조화된 결과](attempt-03/run.json)
