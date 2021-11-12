#include "display.h"
#include "tm_ili9341.h"
#include "play.h"
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"

static __IO uint16_t x, y, z;
static __IO bool changed = false;
void display_notify(uint16_t x_, uint16_t y_, uint16_t z_)
{
	changed = true;
	x = x_;
	y = y_;
	z = z_;
}

static void button_0(void)
{
	play_sample("A.wav");
}

static void button_1(void)
{
	play_sample("B.wav");
}

static void button_2(void)
{
	play_sample("C.wav");
}

static void button_3(void)
{
	play_sample("D.wav");
}

static void display_pressed(uint16_t x, uint16_t y, uint16_t z)
{
	if (x < ILI9341_WIDTH / 2) {
		if (y < ILI9341_HEIGHT / 2) {
			button_0();
		} else {
			button_1();
		}
	} else {
		if (y < ILI9341_HEIGHT / 2) {
			button_2();
		} else {
			button_3();
		}
	}
}

void display_clear(void)
{
	TM_ILI9341_Fill(ILI9341_COLOR_BLACK);
}

void display_draw_cross(void)
{
	TM_ILI9341_DrawLine(0, ILI9341_HEIGHT / 2, ILI9341_WIDTH, ILI9341_HEIGHT / 2,
			    ILI9341_COLOR_WHITE);
	TM_ILI9341_DrawLine(ILI9341_WIDTH / 2, 0, ILI9341_WIDTH / 2 , ILI9341_HEIGHT,
			    ILI9341_COLOR_WHITE);
}

void display_init(void)
{
	display_draw_cross();
}

void display_task(void *pvParameters)
{
	display_init();

	while(1) {
		vTaskDelay(25);
		if (!changed)
			continue;

		changed = false;
		display_pressed(x, y, z);
		TM_ILI9341_DrawRectangle(x - 5, y - 5, x + 5, y + 5, ILI9341_COLOR_WHITE);
//		display_clear();
//		display_draw_cross();
	}
}
