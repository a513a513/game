#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <time.h>
#include <windows.h>

// [업그레이드 1] 최신 컴파일러 호환을 위한 표준 매크로 보정
#ifndef CLK_TCK
#define CLK_TCK CLOCKS_PER_SEC
#endif

// 게임 설정 구조체 (업그레이드 항목 관리용)
typedef struct {
	int mode;          // [업그레이드 9] 1: 1인용(vs AI), 2: 2인용 대결
	int ai_level;      // [업그레이드 10] AI 난이도 (1:쉬움, 2:보통, 3:어려움)
	int target_wins;   // [업그레이드 13] 승리 목표 승수
	double game_time;  // 제한 시간
} GameConfig;

// 라운드 상태 구조체
typedef struct {
	int hit_cnt[2];    // 총 연타 횟수
	int crit_cnt[2];   // [업그레이드 15] 크리티컬 횟수
	int fever[2];      // [업그레이드 17] 피버 게이지 (0~10)
	int skill_used[2]; // [업그레이드 18] 필살기(S/K키) 사용 여부
	int wind;          // [업그레이드 14] 풍향 (-1:좌풍, 0:무풍, 1:우풍)
	double max_cps[2]; // [업그레이드 29] 최고 초당 타수 기록
} RoundState;

// [업그레이드 21] 콘솔 색상 변경 함수
void set_color(int color) {
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

void gotoxy(int x, int y) {
	COORD Pos = { (SHORT)(x - 1), (SHORT)(y - 1) };
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), Pos);
}

// [업그레이드 4] 화면 깜빡임 방지용 콘솔 커서 숨김 함수
void hide_cursor(void) {
	CONSOLE_CURSOR_INFO cursorInfo = { 1, FALSE };
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), (COORD){0, 0});
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
}

// [업그레이드 6] 콘솔 입력 버퍼 비우기 함수
void flush_buffer(void) {
	while (_kbhit()) _getch();
}

void intro_game(char team_name[][20], GameConfig *cfg);
void display_line(int center_x);
void display_status(int center_x, char t_name[][20]);
void display_time_bar(double pst, double total_time);
void display_team_hud(RoundState *st);
void display_start(char t_name[][20], int s_w[], int s_l[], RoundState *st, GameConfig cfg);
void make_decision(int *center_x, int s_w[], int s_l[], char t_name[][20], RoundState *st, double ps);
void game_control(int *center_x, clock_t *st_time, double *ps, RoundState *st, GameConfig cfg, char t_name[][20]);
void save_log_and_award(char t_name[][20], int s_w[], int s_l[], RoundState *st);

int main(void)
{
	// [업그레이드 5] 콘솔 창 타이틀 설정
	SetConsoleTitle(TEXT("줄다리기 게임"));
	hide_cursor();
	srand((unsigned int)time(NULL));

	// [업그레이드 30] 게임 종료 후 재대결(Restart) 루프
	char retry;
	do {
		int score_win[2] = { 0 }, score_loose[2] = { 0 }, center_x;
		// [업그레이드 2] 버퍼 오버플로우 방지를 위한 팀 이름 배열 [2][20] 확장
		char team_name[2][20];
		double pst;
		clock_t start;
		GameConfig cfg;
		RoundState st = { {0,0}, {0,0}, {0,0}, {0,0}, 0, {0.0, 0.0} };

		system("cls");
		intro_game(team_name, &cfg);

		do {
			st.hit_cnt[0] = st.hit_cnt[1] = 0;
			st.crit_cnt[0] = st.crit_cnt[1] = 0;
			st.fever[0] = st.fever[1] = 0;
			st.skill_used[0] = st.skill_used[1] = 0;
			st.wind = (rand() % 3) - 1; // [업그레이드 14] 랜덤 바람 생성

			display_start(team_name, score_win, score_loose, &st, cfg);

			// [업그레이드 3] 기준점 화살표(▼, x=40)와 줄의 중심 좌표를 40으로 완벽 통일
			center_x = 40;
			start = clock();
			pst = 0;

			do {
				game_control(&center_x, &start, &pst, &st, cfg, team_name);
			} while (pst < cfg.game_time && ((25 < center_x) && (center_x < 55)));

			make_decision(&center_x, score_win, score_loose, team_name, &st, pst);

		} while ((score_win[0] < cfg.target_wins) && (score_win[1] < cfg.target_wins));

		// [업그레이드 29 & 30] 최종 시상식 및 파일 로그 저장
		save_log_and_award(team_name, score_win, score_loose, &st);

		gotoxy(1, 21);
		set_color(11);
		printf("게임을 다시 시작하시겠습니까? (R: 재시작 / 아무 키: 종료) > ");
		set_color(7);
		flush_buffer();
		retry = _getch();
	} while (retry == 'r' || retry == 'R');

	gotoxy(1, 23);
	printf("게임을 완전히 종료합니다.\n");
	return 0;
}

void intro_game(char team_name[][20], GameConfig *cfg)
{
	set_color(11);
	printf("====================================================================\n");
	printf("       			★ 줄다리기 게임 ★			   	  	      \n");
	printf("====================================================================\n\n");
	set_color(7);
	printf("[조작법] 1팀: 'A'연타 (필살기 'S')  |  2팀: 'L'연타 (필살기 'K')\n");
	printf("[기능키] 일시정지: 'P'키            |  즉시 종료: 'ESC'키\n");
	printf("※ 홀드 방지 적용: 키를 꾹 누르고 있으면 움직이지 않습니다!\n\n");

	// [업그레이드 9] 1인용(AI) / 2인용 모드 선택
	printf("1. 게임 모드 선택 (1: 1인 플레이 vs AI  /  2: 2인 플레이 대결) > ");
	scanf("%d", &cfg->mode);
	if (cfg->mode != 1) cfg->mode = 2;

	if (cfg->mode == 1) {
		// [업그레이드 10] AI 난이도 선택
		printf("2. AI 난이도 선택 (1: 쉬움  /  2: 보통  /  3: 어려움) > ");
		scanf("%d", &cfg->ai_level);
		if (cfg->ai_level < 1 || cfg->ai_level > 3) cfg->ai_level = 2;
		printf("\n플레이어 팀 이름을 입력하고 Enter > ");
		scanf("%19s", team_name[0]);
		sprintf(team_name[1], "AI_Bot(Lv%d)", cfg->ai_level);
	}
	else {
		for (int i = 0; i < 2; i++) {
			printf("%d번 참가팀의 이름을 입력하고 Enter > ", i + 1);
			scanf("%19s", team_name[i]);
		}
	}

	// [업그레이드 13] 목표 승수 설정
	printf("\n목표 승리 횟수(선승 기준, 1~5)를 입력하고 Enter > ");
	scanf("%d", &cfg->target_wins);
	if (cfg->target_wins < 1 || cfg->target_wins > 5) cfg->target_wins = 2;

	printf("라운드 제한 시간(초 단위, 예: 15)을 입력하고 Enter > ");
	scanf("%lf", &cfg->game_time);
	if (cfg->game_time < 3.0) cfg->game_time = 10.0;

	system("cls");
	hide_cursor();
}

// [업그레이드 3 & 22] 반각/전각 좌표 정중앙 정렬 및 양끝 승리 깃발 표시
void display_line(int center_x)
{
	int i;
	gotoxy(1, 9);
	for (i = 0; i < 78; i++) printf(" ");

	// [업그레이드 22] 양끝 승리 한계선(25, 55) 마커 표시
	gotoxy(25, 9); set_color(9);  printf("◁");
	gotoxy(55, 9); set_color(12); printf("▷");

	// center_x(초기값 40)를 중심으로 좌(-10) ~ 우(+10) 1칸씩 정확히 출력
	for (i = -10; i <= 10; i++)
	{
		gotoxy(center_x + i, 9);
		if (i == 0) {
			set_color(14); printf("■"); // 정중앙 노란색 마커
		}
		else if (i == -6 || i == 6) {
			set_color(12); printf("◆");
		}
		else {
			set_color(7);  printf("□");
		}
	}
	set_color(7);
}

// [업그레이드 23] 실시간 우세 전황 중계창
void display_status(int center_x, char t_name[][20])
{
	gotoxy(20, 5);
	if (center_x < 40) {
		set_color(9);
		printf("[실시간 전황] ◀ %-10s 팀이 %2d칸 차이로 리드 중! (승리까지 %2d칸) ", t_name[0], 40 - center_x, center_x - 25);
	}
	else if (center_x > 40) {
		set_color(12);
		printf("[실시간 전황] ▶ %-10s 팀이 %2d칸 차이로 리드 중! (승리까지 %2d칸) ", t_name[1], center_x - 40, 55 - center_x);
	}
	else {
		set_color(14);
		printf("[실시간 전황] ◆ 정확히 기준점(40) 중앙에서 팽팽하게 맞서고 있습니다! ");
	}
	set_color(7);
}

// [업그레이드 24] 남은 시간 시각적 프로그레스 바 출력
void display_time_bar(double pst, double total_time)
{
	double ratio = (total_time - pst) / total_time;
	if (ratio < 0) ratio = 0;
	int bars = (int)(ratio * 10);

	gotoxy(48, 1);
	printf("남은시간: [");
	set_color(bars <= 3 ? 12 : 10); // 30% 이하일 때 빨간색 경고
	for (int i = 0; i < 10; i++) {
		if (i < bars) printf("■");
		else printf("□");
	}
	set_color(7);
	printf("]");
}

// [업그레이드 27] 각 팀 실시간 타수, 피버 게이지, 필살기 보유 상태 HUD
void display_team_hud(RoundState *st)
{
	for (int i = 0; i < 2; i++) {
		int base_x = (i == 0) ? 5 : 52;
		gotoxy(base_x, 12);
		printf("연타수: %3d타 (크리:%2d)", st->hit_cnt[i], st->crit_cnt[i]);

		gotoxy(base_x, 13);
		printf("피버: [");
		set_color(14);
		for (int f = 0; f < 5; f++) {
			if (f < st->fever[i] / 2) printf("■");
			else printf("□");
		}
		set_color(7);
		printf("] 필살기(%c): %s", (i == 0) ? 'S' : 'K', st->skill_used[i] ? "사용완료" : "준비완료!");
	}
}

void display_start(char t_name[][20], int s_w[], int s_l[], RoundState *st, GameConfig cfg)
{
	system("cls");
	hide_cursor();

	gotoxy(1, 1);
	printf("제한시간: %4.1f초 (%d선승제)", cfg.game_time, cfg.target_wins);
	gotoxy(28, 1);
	printf("경과시간:  0.0초");
	display_time_bar(0, cfg.game_time);

	gotoxy(38, 7); printf("기준점");
	gotoxy(40, 8); set_color(14); printf("▼");
	gotoxy(40, 10); printf("▲"); set_color(7);

	display_line(40);
	display_status(40, t_name);
	display_team_hud(st);

	for (int i = 0; i < 2; i++) {
		int base_x = (i == 0) ? 5 : 52;
		set_color(i == 0 ? 9 : 12);
		gotoxy(base_x, 6); printf("팀명: %-14s", t_name[i]);
		gotoxy(base_x, 7); printf("전적: %d승 %d패", s_w[i], s_l[i]);
	}
	set_color(7);
	gotoxy(5, 11);  printf("[ 'A'키 연타 / 'S' 필살기 ]");
	gotoxy(52, 11); printf("[ 'L'키 연타 / 'K' 필살기 ]");

	// [업그레이드 14 표시] 풍향 안내
	gotoxy(22, 3);
	set_color(11);
	if (st->wind == -1)      printf("[환경 효과] ◀ 왼쪽 강풍 발생! (1팀 순풍 보너스)   ");
	else if (st->wind == 1)  printf("[환경 효과] ▶ 오른쪽 강풍 발생! (2팀 순풍 보너스) ");
	else                     printf("[환경 효과] 현재 바람이 불지 않습니다. (무풍)      ");
	set_color(7);

	flush_buffer();
	gotoxy(1, 15);
	printf("아무 키나 누르면 3초 카운트다운 후 라운드를 시작합니다...");
	_getch();

	// [업그레이드 25 & 26] 3초 카운트다운 및 비프음 연출
	for (int i = 3; i > 0; i--) {
		gotoxy(1, 15);
		printf("경기 시작 %d초 전... 키에서 손을 떼고 대기하세요!            ", i);
		Beep(523, 150);
		Sleep(850);
	}
	gotoxy(1, 15);
	set_color(14);
	printf("★★★ START! 키를 뗐다 눌렀다 빠르게 연타하세요! (P:일시정지) ★★★");
	set_color(7);
	Beep(1046, 250);
}

void game_control(int *center_x, clock_t *st_time, double *ps, RoundState *st, GameConfig cfg, char t_name[][20])
{
	// [업그레이드 8] 키 홀드(꾹 누르기) 원천 차단용 상태 기억 변수
	static int hold_a = 0, hold_l = 0, hold_s = 0, hold_k = 0;
	static double last_ai_hit = 0.0;
	// [버그 수정] 새 라운드가 시작되어 *ps가 리셋되면 AI 타이머도 함께 초기화
	if (*ps < last_ai_hit) {
		last_ai_hit = 0.0;
	}

	// [업그레이드 12] ESC 키 즉시 종료 핫키
	if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
		system("cls");
		printf("ESC 키가 눌려 게임을 즉시 종료합니다.\n");
		exit(0);
	}

	// [업그레이드 11] 'P' 키 일시정지 및 타이머 보정 기능
	if (GetAsyncKeyState('P') & 0x8000) {
		clock_t pause_start = clock();
		gotoxy(1, 15);
		set_color(13);
		printf("[일시정지 됨] 재개하려면 'R' 키를 누르세요...                         ");
		set_color(7);
		while (!(GetAsyncKeyState('R') & 0x8000)) Sleep(50);
		*st_time += (clock() - pause_start); // 정지해 있던 시간만큼 시작 시간 보정
		gotoxy(1, 15);
		printf("★★★ 경기가 재개되었습니다! 빠르게 연타하세요! ★★★               ");
	}

	int down_a = (GetAsyncKeyState('A') & 0x8000) != 0;
	int down_l = (GetAsyncKeyState('L') & 0x8000) != 0;
	int down_s = (GetAsyncKeyState('S') & 0x8000) != 0;
	int down_k = (GetAsyncKeyState('K') & 0x8000) != 0;
	int moved = 0, power;

	// [1팀 'A'키 연타 처리]
	if (down_a) {
		if (!hold_a) {
			hold_a = 1;
			st->hit_cnt[0]++;
			st->fever[0]++;
			power = 1;

			// [업그레이드 15] 10% 크리티컬 판정
			if (rand() % 10 == 0) { power += 1; st->crit_cnt[0]++; Beep(750, 20); }
			// [업그레이드 16] 패배 위기(48 이상) 시 역전 버프
			if (*center_x >= 48) power += 1;
			// [업그레이드 14] 순풍 보너스
			if (st->wind == -1 && (rand() % 5 == 0)) power += 1;
			// [업그레이드 17] 10연타 피버 게이지 폭발 보너스
			if (st->fever[0] >= 10) {
				power += 2;
				st->fever[0] = 0;
				gotoxy(5, 11); set_color(14); printf("[★1팀 피버 발동! +2칸★]"); set_color(7);
			}
			else {
				gotoxy(5, 11); printf("[ 'A'키 정상 타격! ]     ");
			}
			*center_x -= power;
			moved = 1;
		}
		else {
			gotoxy(5, 11); set_color(12); printf("[홀드 감지! 손을 떼세요] "); set_color(7);
		}
	}
	else {
		hold_a = 0;
	}

	// [업그레이드 18] 1팀 'S'키 필살기 (라운드당 1회 4칸 당기기)
	if (down_s && !hold_s && !st->skill_used[0]) {
		hold_s = 1;
		st->skill_used[0] = 1;
		*center_x -= 4;
		moved = 1;
		Beep(900, 60);
		gotoxy(5, 11); set_color(11); printf("[★1팀 필살기 발동 -4칸!]"); set_color(7);
	}
	else if (!down_s) hold_s = 0;

	// [2팀 조작: 1인 모드일 경우 AI 자동 타격 / 2인 모드일 경우 'L', 'K'키 처리]
	if (cfg.mode == 1) {
		double ai_interval = (cfg.ai_level == 1) ? 0.32 : (cfg.ai_level == 2 ? 0.19 : 0.13);
		if (*ps - last_ai_hit >= ai_interval) {
			last_ai_hit = *ps;
			st->hit_cnt[1]++;
			st->fever[1]++;
			power = (rand() % 10 == 0) ? 2 : 1;
			if (power == 2) st->crit_cnt[1]++;
			if (st->fever[1] >= 10) { power += 2; st->fever[1] = 0; }
			*center_x += power;
			moved = 1;
		}
		// AI 위기 시 필살기 자동 사용
		if (!st->skill_used[1] && *center_x <= 30) {
			st->skill_used[1] = 1;
			*center_x += 4;
			moved = 1;
			Beep(600, 60);
		}
	}
	else {
		if (down_l) {
			if (!hold_l) {
				hold_l = 1;
				st->hit_cnt[1]++;
				st->fever[1]++;
				power = 1;

				if (rand() % 10 == 0) { power += 1; st->crit_cnt[1]++; Beep(850, 20); }
				if (*center_x <= 32) power += 1;
				if (st->wind == 1 && (rand() % 5 == 0)) power += 1;
				if (st->fever[1] >= 10) {
					power += 2;
					st->fever[1] = 0;
					gotoxy(52, 11); set_color(14); printf("[★2팀 피버 발동! +2칸★]"); set_color(7);
				}
				else {
					gotoxy(52, 11); printf("[ 'L'키 정상 타격! ]     ");
				}
				*center_x += power;
				moved = 1;
			}
			else {
				gotoxy(52, 11); set_color(12); printf("[홀드 감지! 손을 떼세요] "); set_color(7);
			}
		}
		else {
			hold_l = 0;
		}

		// [업그레이드 18] 2팀 'K'키 필살기
		if (down_k && !hold_k && !st->skill_used[1]) {
			hold_k = 1;
			st->skill_used[1] = 1;
			*center_x += 4;
			moved = 1;
			Beep(600, 60);
			gotoxy(52, 11); set_color(11); printf("[★2팀 필살기 발동 +4칸!]"); set_color(7);
		}
		else if (!down_k) hold_k = 0;
	}

	// [업그레이드 19] 돌발 번개 이벤트 (약 0.3% 확률로 양 팀 피버 게이지 방전)
	if (rand() % 350 == 0 && (st->fever[0] > 0 || st->fever[1] > 0)) {
		st->fever[0] = st->fever[1] = 0;
		gotoxy(22, 4); set_color(14);
		printf("? [돌발 낙뢰 발생!] 양 팀의 피버 게이지가 방전되었습니다! ?");
		set_color(7);
	}

	if (*center_x < 25) *center_x = 25;
	if (*center_x > 55) *center_x = 55;

	if (moved) {
		display_line(*center_x);
		display_status(*center_x, t_name);
		display_team_hud(st);
	}

	flush_buffer();

	clock_t end = clock();
	*ps = (double)(end - *st_time) / CLK_TCK;
	gotoxy(28, 1);
	printf("경과시간: %4.1f초", *ps);
	display_time_bar(*ps, cfg.game_time);
	Sleep(15);
}

void make_decision(int *center_x, int s_w[], int s_l[], char t_name[][20], RoundState *st, double ps)
{
	int win = 0;
	Sleep(400);
	flush_buffer();

	// [업그레이드 20] 무승부(center_x == 40)일 경우 서든데스(골든골) 연장전 돌입!
	if (*center_x == 40) {
		gotoxy(1, 15);
		set_color(14);
		printf("? [서든데스 돌입!] 정확히 비겼습니다! 먼저 1타('A' 또는 'L')를 치는 팀이 승리합니다! ?");
		Beep(800, 300);
		while (*center_x == 40) {
			if (GetAsyncKeyState('A') & 0x8000) *center_x = 39;
			else if (GetAsyncKeyState('L') & 0x8000) *center_x = 41;
			Sleep(10);
		}
		display_line(*center_x);
	}

	if (*center_x > 40) {
		win = 2; s_w[1]++; s_l[0]++;
	}
	else {
		win = 1; s_w[0]++; s_l[1]++;
	}

	if (ps <= 0.1) ps = 1.0;
	double cps0 = st->hit_cnt[0] / ps;
	double cps1 = st->hit_cnt[1] / ps;
	if (cps0 > st->max_cps[0]) st->max_cps[0] = cps0;
	if (cps1 > st->max_cps[1]) st->max_cps[1] = cps1;

	gotoxy(1, 15);
	set_color(14);
	printf("▶ %s 팀이 라운드를 승리했습니다! (최종 기준점 대비 %d칸 우세)                    ",
		t_name[win - 1], (win == 1) ? (40 - *center_x) : (*center_x - 40));
	Beep(880, 150); Beep(1046, 250);

	// [업그레이드 28] 라운드 정밀 통계 리포트 출력
	set_color(7);
	gotoxy(1, 16);
	printf("[통계] %s: %d타(%.1f타/초, 크리 %d회) | %s: %d타(%.1f타/초, 크리 %d회)    ",
		t_name[0], st->hit_cnt[0], cps0, st->crit_cnt[0],
		t_name[1], st->hit_cnt[1], cps1, st->crit_cnt[1]);

	gotoxy(1, 17);
	printf("다음 라운드로 진행하려면 아무 키나 누르세요...                                  ");
	flush_buffer();
	_getch();
}

// [업그레이드 29 & 30] 최종 MVP 등급 판정 및 외부 텍스트 파일 자동 기록
void save_log_and_award(char t_name[][20], int s_w[], int s_l[], RoundState *st)
{
	int final_win = (s_w[0] > s_w[1]) ? 0 : 1;
	double best_cps = st->max_cps[final_win];
	char grade = (best_cps >= 8.0) ? 'S' : (best_cps >= 6.0 ? 'A' : (best_cps >= 4.0 ? 'B' : 'C'));

	gotoxy(1, 18);
	set_color(14);
	printf("====================================================================\n");
	printf("?? 최종 우승: %s 팀 (%d승 %d패) | 최고 연타 속도: %.1f타/초 [등급: %c] ??\n",
		t_name[final_win], s_w[final_win], s_l[final_win], best_cps, grade);
	printf("====================================================================\n");
	set_color(7);
	Beep(523, 150); Beep(659, 150); Beep(784, 150); Beep(1046, 400);

	// [업그레이드 30] 경기 전적 파일 입출력 저장
	FILE *fp = fopen("tug_of_war_log.txt", "a");
	if (fp != NULL) {
		fprintf(fp, "[경기 기록] 우승: %s (%d승 %d패) vs 패배: %s | 최고 CPS: %.1f (등급 %c)\n",
			t_name[final_win], s_w[final_win], s_l[final_win], t_name[1 - final_win], best_cps, grade);
		fclose(fp);
		gotoxy(1, 20);
		printf("※ 경기 결과가 'tug_of_war_log.txt' 파일에 자동 저장되었습니다.");
	}
}
