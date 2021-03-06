#include <regx52.h>

char twinkling = 0;
char dark_cycle = 0;

unsigned long start_water_dry_time = 0;
unsigned long start_water_too_dry_time = 0;
unsigned int max_time = 1;

enum DisplayState {
	DISPLAY_TIMER,
	DISPLAY_SETTING
} display_state = DISPLAY_TIMER;

enum WaterState {
	WATER_NORMAL,
	WATER_DRY,
	WATER_TOO_DRY
} water_state = WATER_NORMAL;

unsigned long counter0 = 0;
unsigned long pressing_start;
char pressing = 0;

void show_empty();
void show_number(int n);

void init() {
	TMOD = 0x11;	 // 16 bit
	IP = 0x1A;	   // interrupt priorities

	EX0 = 1;
	EX1 = 1;

	EA = 1;

	ET0 = 1;
	TR0 = 1;

	ET1 = 1;
	TR1 = 0;
}

unsigned int current_dry_time() {
	return (counter0 - start_water_dry_time) / 20;
}

unsigned int current_too_dry_time() {
	return (counter0 - start_water_too_dry_time) / 20;
}

void start_beep() {
	TR1 = 0;
	// 2ms
	TH1 = 0xF8;
	TL1 = 0x30;
	TR1 = 1;
}

void too_dry() {
	start_water_too_dry_time = counter0;
	water_state = WATER_TOO_DRY;

	P2_4 = 0;
	twinkling++;
	start_beep();
}

void short_pressing() {	
	if (display_state == DISPLAY_TIMER) {
		if (water_state == WATER_TOO_DRY) {
			P2_4 = 1;
			twinkling--;
		}
		// tuoi nuoc
		water_state = WATER_NORMAL;		
	}
	else if (display_state == DISPLAY_SETTING) {
		max_time = (max_time) % 9 + 1;
	}
}

void long_pressing() {
	P2_1 = ~P2_1;
	if (display_state == DISPLAY_TIMER)	{
		display_state = DISPLAY_SETTING;
		twinkling++;
	}
	else if (display_state == DISPLAY_SETTING) {
		display_state = DISPLAY_TIMER;
		twinkling--;
	}
}

void timer0() interrupt 1 {
	// 50ms
	TH0 = 0x3c;
	TL0 = 0xb0;
	counter0++;

	if (counter0 % 10 == 0)
		dark_cycle = ~dark_cycle & 0x1;

	if (water_state == WATER_DRY && current_dry_time() / 60 >= max_time)
		too_dry();
}

void timer1() interrupt 3 {
	if (pressing) {
		TR1 = 0;
		pressing = 0;
	
		// 500ms
		if (counter0 - pressing_start >= 10)
			long_pressing();
		else
			short_pressing();

		if (water_state == WATER_TOO_DRY)
			start_beep();
	}
	else {
		// 2ms beep
		TH1 = 0xF8;
		TL1 = 0x30;
		P1_5 = ~P1_5;
	}
}

void external0() interrupt 0 {
	if (!pressing) {
		pressing = 1;
		pressing_start = counter0;
	}			 	

	// 2ms
	TR1 = 0;
	TH1 = 0xF8;
	TL1 = 0x30;
	TR1 = 1;
}

void external1() interrupt 2 {
	if (water_state != WATER_DRY) {
		water_state = WATER_DRY;
		start_water_dry_time = counter0;
	}
}

void main() {
	P2 = 0xff;
	init();

	while (1) {
		if (display_state == DISPLAY_TIMER) {
			if (!pressing) {
				if (water_state == WATER_NORMAL)
					show_number(0);
				else if (water_state == WATER_DRY)
					show_number(current_dry_time());
				else if (water_state == WATER_TOO_DRY)
					show_number(current_too_dry_time());
			}
			else
				show_empty();
		}
		else if (display_state == DISPLAY_SETTING) {
			if (!pressing)
				show_number(max_time);
			else
				show_empty();
		}
	}
}

void show_empty() {
	P1_0 = 0;
	P0 = 0xff;
	P1_0 = 1;

	P1_1 = 0;
	P0 = 0xff;
	P1_1 = 1;

	P1_2 = 0;
	P0 = 0xff;
	P1_2 = 1;

	P1_3 = 0;
	P0 = 0xff;
	P1_3 = 1;
}

void __show_number(int led, int n, int dot) 
{
	int i;

	switch (led) {
	case 0:
		P1_0 = 0;
		break;
	case 1:
		P1_1 = 0;
		break;
	case 2:
		P1_2 = 0;
		break;
	case 3:
		P1_3 = 0;
		break;
	}

	switch (n) {
	case 0:
		P0 = ~0x3f;
		break;
	case 1:
		P0 = ~0x06;
		break;
	case 2:
		P0 = ~0x5b;
		break;
	case 3:
		P0 = ~0x4f;
		break;
	case 4:
		P0 = ~0x66;
		break;
	case 5:
		P0 = ~0x6d;
		break;
	case 6:
		P0 = ~0x7d;
		break;
	case 7:
		P0 = ~0x07;
		break;
	case 8:
		P0 = ~0x7f;
		break;
	case 9:
		P0 = ~0x6f;
		break;
	}

	if (dot) 
		P0_7 = 0;

	// Sleep
	i = 400;
	while (i--);

	switch (led) {
	case 0:
		P1_0 = 1;
		break;
	case 1:
		P1_1 = 1;
		break;
	case 2:
		P1_2 = 1;
		break;
	case 3:
		P1_3 = 1;
		break;
	}
}

void show_number(int n) {							
	int n3, n2, n1, n0;

	if (twinkling && dark_cycle) {
		show_empty();	
		return;
	}
	
	n3 = n % 10; n /= 10;
	n2 = n % 10; n /= 10;
	n1 = n % 10; n /= 10;
	n0 = n;

	__show_number(0, n0, 0);
	__show_number(1, n1, 0);
	__show_number(2, n2, 0);
	__show_number(3, n3, 0);
}