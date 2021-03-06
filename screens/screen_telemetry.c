
#include <all.h>



uint8_t telemetry_null (char *name, float x, float y, int8_t button, float data, uint8_t action) {
	return 0;
}

void draw_value_display (ESContext *esContext, float x1, float y1, float fsize, uint16_t len, uint8_t align, char *title, char *format, float value) {
	float x2 = x1 + fsize * (len + 1) * 0.6;
	float y2 = y1 + fsize + 0.04 + (fsize / 3);
	char text[100];
	sprintf(text, format, value);
	draw_box_f(esContext, x1, y1, x2, y2, 10, 10, 10, 200);
	draw_rect_f(esContext, x1, y1, x2, y2, 100, 100, 100, 255);
	draw_rect_f(esContext, x1 + 0.01, y1 + 0.01, x2 - 0.01, y2 - 0.01, 200, 200, 200, 255);
	draw_text_f(esContext, x1 + 0.01, y1 + 0.02, fsize / 3, fsize / 3, FONT_PINK, title);
	if (align == 0) {
		draw_text_f(esContext, x1 + 0.02, y1 + 0.02 + (fsize / 3), fsize, fsize, FONT_GREEN, text);
	} else {
		draw_text_f(esContext, x1 + 0.02 + (fsize * (len - strlen(text)) * 0.6), y1 + 0.02 + (fsize / 3), fsize, fsize, FONT_GREEN, text);
	}
}

void draw_value_bar (ESContext *esContext, float x1, float y1, float x2, float y2, uint8_t type, float min, float max, float warning, float critical, float value) {
	char tmp_str[1024];
//	draw_box_f(esContext, x1, y1, x2, y2, 100, 100, 100, 125);

	float x_diff = x2 - x1;
	float y_diff = y2 - y1;
	float v_diff = max - min;

	float y_warning = y2 - (y_diff / v_diff * (warning - min));
	float y_critical = y2 - (y_diff / v_diff * (critical - min));
	float y_max = y2 - (y_diff / v_diff * (max - min));
	float y_value = y2 - (y_diff / v_diff * (value - min));

	draw_box_f(esContext, x1 + x_diff / 6.0 * 3.0, y_max, x1 + x_diff / 6.0 * 4.0, y_warning, 0, 255, 0, 255);
	draw_box_f(esContext, x1 + x_diff / 6.0 * 3.0, y_warning, x1 + x_diff / 6.0 * 4.0, y_critical, 255, 255, 0, 255);
	draw_box_f(esContext, x1 + x_diff / 6.0 * 3.0, y_critical, x1 + x_diff / 6.0 * 4.0, y2, 255, 0, 0, 255);

	draw_line_f(esContext, x1 + x_diff / 6.0 * 3.0, y_warning, x1 + x_diff / 6.0 * 4.0 + x_diff / 10.0, y_warning, 255, 255, 0, 255);
	sprintf(tmp_str, "%0.1f", warning);
	draw_text_f(esContext, x1 + x_diff / 6.0 * 4.0 + x_diff / 10.0, y_warning - 0.02, 0.04, 0.04, FONT_PINK, tmp_str);

	draw_line_f(esContext, x1 + x_diff / 6.0 * 3.0, y_critical, x1 + x_diff / 6.0 * 4.0 + x_diff / 10.0, y_critical, 255, 0, 0, 255);
	sprintf(tmp_str, "%0.1f", critical);
	draw_text_f(esContext, x1 + x_diff / 6.0 * 4.0 + x_diff / 10.0, y_critical - 0.02, 0.04, 0.04, FONT_PINK, tmp_str);


	uint16_t steps = (uint16_t)(y_diff / 0.1);
	uint16_t step = 0;
	for (step = 0; step <= steps; step++) {
		draw_line_f(esContext, x1 + x_diff / 6.0 * 3.0, y2 - (y_diff * (float)step / steps), x1 + x_diff / 6.0 * 3.0 - x_diff / 10.0, y2 - (y_diff * (float)step / steps), 255, 255, 255, 255);
		sprintf(tmp_str, "%0.1f", min + (v_diff * (float)step / steps));
		draw_text_f(esContext, x1, y2 - (y_diff * (float)step / steps) - 0.02, 0.04, 0.04, FONT_WHITE, tmp_str);
	}

	steps *= 5;
	for (step = 0; step <= steps; step++) {
		draw_line_f(esContext, x1 + x_diff / 6.0 * 3.0, y2 - (y_diff * (float)step / steps), x1 + x_diff / 6.0 * 3.0 - x_diff / 20.0, y2 - (y_diff * (float)step / steps), 200, 200, 200, 255);
	}
#ifndef ANDROID

	sprintf(tmp_str, "%0.1f", value);

	if (value < min || value > max) {
		if (value < min) {
			y_value = y2;
		} else {
			y_value = y1;
		}
		draw_triaFilled_f(esContext, x1 + x_diff / 6.0 * 3.4 + 0.005, y_value + 0.005, x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0 + 0.005, y_value - x_diff / 15.0 + 0.005, x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0 + 0.005, y_value + x_diff / 15.0 + 0.005, 255, 0, 0, 100);
		draw_tria_f(esContext, x1 + x_diff / 6.0 * 3.4 + 0.005, y_value + 0.005, x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0 + 0.005, y_value - x_diff / 15.0 + 0.005, x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0 + 0.005, y_value + x_diff / 15.0 + 0.005, 0, 0, 0, 100);
		draw_triaFilled_f(esContext, x1 + x_diff / 6.0 * 3.4, y_value, x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0, y_value - x_diff / 15.0, x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0, y_value + x_diff / 15.0, 255, 0, 0, 255);
		draw_tria_f(esContext, x1 + x_diff / 6.0 * 3.4, y_value, x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0, y_value - x_diff / 15.0, x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0, y_value + x_diff / 15.0, 0, 0, 0, 255);
	} else {
		draw_triaFilled_f(esContext, x1 + x_diff / 6.0 * 3.4 + 0.005, y_value + 0.005, x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0 + 0.005, y_value - x_diff / 15.0 + 0.005, x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0 + 0.005, y_value + x_diff / 15.0 + 0.005, 255, 255, 255, 100);
		draw_tria_f(esContext, x1 + x_diff / 6.0 * 3.4 + 0.005, y_value + 0.005, x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0 + 0.005, y_value - x_diff / 15.0 + 0.005, x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0 + 0.005, y_value + x_diff / 15.0 + 0.005, 0, 0, 0, 100);
		draw_triaFilled_f(esContext, x1 + x_diff / 6.0 * 3.4, y_value, x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0, y_value - x_diff / 15.0, x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0, y_value + x_diff / 15.0, 255, 255, 255, 255);
		draw_tria_f(esContext, x1 + x_diff / 6.0 * 3.4, y_value, x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0, y_value - x_diff / 15.0, x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0, y_value + x_diff / 15.0, 0, 0, 0, 255);
	}
#endif
}

void draw_value_bar_duo (ESContext *esContext, float x1, float y1, float x2, float y2, uint8_t type, float min, float max, float warning, float critical, float value, float value2) {
	char tmp_str[1024];
//	draw_box_f(esContext, x1, y1, x2, y2, 100, 100, 100, 125);

	float x_diff = x2 - x1;
	float y_diff = y2 - y1;
	float v_diff = max - min;

	float y_warning = y2 - (y_diff / v_diff * (warning - min));
	float y_critical = y2 - (y_diff / v_diff * (critical - min));
	float y_max = y2 - (y_diff / v_diff * (max - min));
	float y_value = y2 - (y_diff / v_diff * (value - min));
	float y_value2 = y2 - (y_diff / v_diff * (value2 - min));

	draw_box_f(esContext, x1 + x_diff / 6.0 * 3.0, y_max, x1 + x_diff / 6.0 * 4.0, y_warning, 0, 255, 0, 255);
	draw_box_f(esContext, x1 + x_diff / 6.0 * 3.0, y_warning, x1 + x_diff / 6.0 * 4.0, y_critical, 255, 255, 0, 255);
	draw_box_f(esContext, x1 + x_diff / 6.0 * 3.0, y_critical, x1 + x_diff / 6.0 * 4.0, y2, 255, 0, 0, 255);

	draw_line_f(esContext, x1 + x_diff / 6.0 * 3.0, y_warning, x1 + x_diff / 6.0 * 4.0 + x_diff / 10.0, y_warning, 255, 255, 0, 255);
	sprintf(tmp_str, "%0.1f", warning);
	draw_text_f(esContext, x1 + x_diff / 6.0 * 4.0 + x_diff / 10.0, y_warning - 0.02, 0.04, 0.04, FONT_PINK, tmp_str);

	draw_line_f(esContext, x1 + x_diff / 6.0 * 3.0, y_critical, x1 + x_diff / 6.0 * 4.0 + x_diff / 10.0, y_critical, 255, 0, 0, 255);
	sprintf(tmp_str, "%0.1f", critical);
	draw_text_f(esContext, x1 + x_diff / 6.0 * 4.0 + x_diff / 10.0, y_critical - 0.02, 0.04, 0.04, FONT_PINK, tmp_str);


	uint16_t steps = (uint16_t)(y_diff / 0.1);
	uint16_t step = 0.0;
	for (step = 0; step <= steps; step++) {
		draw_line_f(esContext, x1 + x_diff / 6.0 * 3.0, y2 - (y_diff * (float)step / steps), x1 + x_diff / 6.0 * 3.0 - x_diff / 10.0, y2 - (y_diff * (float)step / steps), 255, 255, 255, 255);
		sprintf(tmp_str, "%0.1f", min + (v_diff * (float)step / steps));
		draw_text_f(esContext, x1, y2 - (y_diff * (float)step / steps) - 0.02, 0.04, 0.04, FONT_WHITE, tmp_str);
	}

	steps *= 5;
	for (step = 0; step <= steps; step++) {
		draw_line_f(esContext, x1 + x_diff / 6.0 * 3.0, y2 - (y_diff * (float)step / steps), x1 + x_diff / 6.0 * 3.0 - x_diff / 20.0, y2 - (y_diff * (float)step / steps), 200, 200, 200, 255);
	}

	sprintf(tmp_str, "%0.1f/%0.1f", value, value2);
#ifndef ANDROID
	if (value < min || value > max) {
		if (value < min) {
			y_value = y2;
		} else {
			y_value = y1;
		}
		draw_triaFilled_f(esContext, x1 + x_diff / 6.0 * 3.4 + 0.005, y_value + 0.005, x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0 + 0.005, y_value - x_diff / 15.0 + 0.005, x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0 + 0.005, y_value + x_diff / 15.0 + 0.005, 255, 0, 0, 100);
		draw_tria_f(esContext, x1 + x_diff / 6.0 * 3.4 + 0.005, y_value + 0.005, x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0 + 0.005, y_value - x_diff / 15.0 + 0.005, x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0 + 0.005, y_value + x_diff / 15.0 + 0.005, 0, 0, 0, 100);
		draw_triaFilled_f(esContext, x1 + x_diff / 6.0 * 3.4, y_value, x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0, y_value - x_diff / 15.0, x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0, y_value + x_diff / 15.0, 255, 0, 0, 255);
		draw_tria_f(esContext, x1 + x_diff / 6.0 * 3.4, y_value, x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0, y_value - x_diff / 15.0, x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0, y_value + x_diff / 15.0, 0, 0, 0, 255);
	} else {
		draw_triaFilled_f(esContext, x1 + x_diff / 6.0 * 3.4 + 0.005, y_value + 0.005, x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0 + 0.005, y_value - x_diff / 15.0 + 0.005, x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0 + 0.005, y_value + x_diff / 15.0 + 0.005, 200, 200, 200, 100);
		draw_tria_f(esContext, x1 + x_diff / 6.0 * 3.4 + 0.005, y_value + 0.005, x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0 + 0.005, y_value - x_diff / 15.0 + 0.005, x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0 + 0.005, y_value + x_diff / 15.0 + 0.005, 0, 0, 0, 100);
		draw_triaFilled_f(esContext, x1 + x_diff / 6.0 * 3.4, y_value, x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0, y_value - x_diff / 15.0, x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0, y_value + x_diff / 15.0, 200, 200, 200, 255);
		draw_tria_f(esContext, x1 + x_diff / 6.0 * 3.4, y_value, x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0, y_value - x_diff / 15.0, x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0, y_value + x_diff / 15.0, 0, 0, 0, 255);
	}
	if (value2 < min || value2 > max) {
		if (value2 < min) {
			y_value2 = y2;
		} else {
			y_value2 = y1;
		}
		draw_triaFilled_f(esContext, 0.03 + x1 + x_diff / 6.0 * 3.4 + 0.005, y_value2 + 0.005, 0.03 + x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0 + 0.005, y_value2 - x_diff / 15.0 + 0.005, 0.03 + x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0 + 0.005, y_value2 + x_diff / 15.0 + 0.005, 255, 0, 0, 100);
		draw_tria_f(esContext, 0.03 + x1 + x_diff / 6.0 * 3.4 + 0.005, y_value2 + 0.005, 0.03 + x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0 + 0.005, y_value2 - x_diff / 15.0 + 0.005, 0.03 + x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0 + 0.005, y_value2 + x_diff / 15.0 + 0.005, 0, 0, 0, 100);
		draw_triaFilled_f(esContext, 0.03 + x1 + x_diff / 6.0 * 3.4, y_value2, 0.03 + x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0, y_value2 - x_diff / 15.0, 0.03 + x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0, y_value2 + x_diff / 15.0, 255, 0, 0, 255);
		draw_tria_f(esContext, 0.03 + x1 + x_diff / 6.0 * 3.4, y_value2, 0.03 + x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0, y_value2 - x_diff / 15.0, 0.03 + x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0, y_value2 + x_diff / 15.0, 0, 0, 0, 255);
	} else {
		draw_triaFilled_f(esContext, 0.03 + x1 + x_diff / 6.0 * 3.4 + 0.005, y_value2 + 0.005, 0.03 + x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0 + 0.005, y_value2 - x_diff / 15.0 + 0.005, 0.03 + x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0 + 0.005, y_value2 + x_diff / 15.0 + 0.005, 255, 255, 255, 100);
		draw_tria_f(esContext, 0.03 + x1 + x_diff / 6.0 * 3.4 + 0.005, y_value2 + 0.005, 0.03 + x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0 + 0.005, y_value2 - x_diff / 15.0 + 0.005, 0.03 + x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0 + 0.005, y_value2 + x_diff / 15.0 + 0.005, 0, 0, 0, 100);
		draw_triaFilled_f(esContext, 0.03 + x1 + x_diff / 6.0 * 3.4, y_value2, 0.03 + x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0, y_value2 - x_diff / 15.0, 0.03 + x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0, y_value2 + x_diff / 15.0, 255, 255, 255, 255);
		draw_tria_f(esContext, 0.03 + x1 + x_diff / 6.0 * 3.4, y_value2, 0.03 + x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0, y_value2 - x_diff / 15.0, 0.03 + x1 + x_diff / 6.0 * 4.0 + x_diff / 15.0, y_value2 + x_diff / 15.0, 0, 0, 0, 255);
	}
#endif
}


void draw_value_bar2 (ESContext *esContext, float x1, float y1, float x2, float y2, uint8_t type, float min, float max, float warning, float critical, float value) {
	float x_diff = x2 - x1;
	float y_diff = y2 - y1;
	float v_diff = max - min;
	float y_value1 = y2 - (y_diff / v_diff * (0 - min));
	float y_value2 = y2 - (y_diff / v_diff * (value - min));
	uint16_t steps = (uint16_t)(y_diff / 0.02);
	uint16_t step = 0.0;
	for (step = 0; step <= steps; step++) {
		draw_line_f(esContext, x1 + x_diff / 2.0 - 0.003, y2 - (y_diff * (float)step / steps), x1 + x_diff / 2.0 + 0.003, y2 - (y_diff * (float)step / steps), 255, 255, 255, 255);
	}
	if (type == 1) {
		draw_box_f(esContext, x1, y_value1, x2, y_value2, 0, 255, 0, 255);
	} else {
		if (value < warning) {
			draw_box_f(esContext, x1, y_value1, x2, y_value2, 0, 255, 0, 255);
		} else if (value < critical) {
			draw_box_f(esContext, x1, y_value1, x2, y_value2, 0, 255, 255, 255);
		} else {
			draw_box_f(esContext, x1, y_value1, x2, y_value2, 255, 0, 0, 255);
		}
	}
}

void draw_value_barH (ESContext *esContext, float x1, float y1, float x2, float y2, float min, float max, float value) {
	float x_diff = x2 - x1;
	float v_diff = max - min;
	float x_value2 = x1 + (x_diff / v_diff * (value - min));
	if (value > max) {
		draw_box_f(esContext, x1, y1, x2, y2, 255, 0, 0, 255);
		draw_rect_f(esContext, x1, y1, x2, y2, 255, 255, 255, 55);
	} else if (value < min) {
		draw_rect_f(esContext, x1, y1, x2, y2, 255, 0, 0, 255);
	} else {
		draw_box_f(esContext, x1, y1, x_value2, y2, 0, 255, 0, 255);
		draw_rect_f(esContext, x1, y1, x2, y2, 255, 255, 255, 55);
	}
}

void draw_value_barchannels (ESContext *esContext, float x1, float y1, char *title, int16_t min, int16_t max, uint8_t num, int16_t *values) {
	char tmp_str[10];
	float h = 0.3;
	draw_text_f(esContext, x1, y1, 0.06, 0.06, FONT_WHITE, title);
	uint8_t n = 0;
	for (n = 0; n < num; n++) {
		draw_value_bar2(esContext, x1 + 0.02 + n * 0.05, y1 + 0.1, x1 + 0.04 + n * 0.05, y1 + h, 1, (float)min, (float)max, 0.0, (float)max, (float)values[n]);
		sprintf(tmp_str, "%i", n + 1);
		draw_text_f(esContext, x1 + 0.04 + n * 0.05 - 0.036, y1 + h, 0.05, 0.05, FONT_WHITE, tmp_str);
	}
	if (min < 0) {
		draw_line_f(esContext, x1, y1 + h / 2 + 0.05, x1 + 0.4, y1 + h / 2 + 0.05, 200, 200, 200, 255);
	}
}

void draw_value_barmeter (ESContext *esContext, float x1, float y1, float x2, float y2, char *title, char *unit, uint8_t type, float min, float max, float warning, float critical, float value) {
	char tmp_str[1024];
	draw_text_f(esContext, x1, y1, 0.06, 0.06, FONT_WHITE, title);
	draw_value_bar(esContext, x1, y1 + 0.11, x2, y2 - 0.16, 0, min, max, warning, critical, value);
	sprintf(tmp_str, "%0.1f%s", value, unit);
	draw_text_f(esContext, (x1 + (x2 - x1) / 2) - strlen(tmp_str) * 0.06 / 2.0 * 0.6, y2 - 0.13, 0.06, 0.06, FONT_WHITE, tmp_str);
}

void draw_value_barmeter_duo (ESContext *esContext, float x1, float y1, float x2, float y2, char *title, char *unit, uint8_t type, float min, float max, float warning, float critical, float value, float value2) {
	char tmp_str[1024];
	draw_text_f(esContext, x1, y1, 0.06, 0.06, FONT_WHITE, title);
	draw_value_bar_duo(esContext, x1, y1 + 0.11, x2, y2 - 0.16, 0, min, max, warning, critical, value, value2);
	sprintf(tmp_str, "%0.1f/%0.1f%s", value, value2, unit);
	draw_text_f(esContext, (x1 + (x2 - x1) / 2) - strlen(tmp_str) * 0.06 / 2.0 * 0.6, y2 - 0.13, 0.06, 0.06, FONT_WHITE, tmp_str);
}


void screen_telemetry (ESContext *esContext) {
#ifndef ANDROID
	glDisable( GL_DEPTH_TEST );
#ifdef SDLGL
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
#else
	ESMatrix modelview;
	esMatrixLoadIdentity(&modelview);
#endif
#endif

	draw_title(esContext, "Telemetry");

	draw_value_barmeter(esContext, -1.2, -0.85, -0.9, -0.15, "SPEED", "km/h", 0, 0.0, 200.0, 0.0, 0.0, ModelData[ModelActive].speed);
	draw_value_barmeter(esContext, -0.7, -0.85, -0.4, -0.15, "ALT", "m", 0, -10.0, 990.0, 0.0, 0.0, ModelData[ModelActive].p_alt);
	if (ModelData[ModelActive].heartbeat == 0 && ModelData[ModelActive].voltage < 5.0) {
		draw_value_barmeter(esContext, -0.2, -0.85, 0.1, -0.15, "VOLTS", "V", 0, 2.0, 4.2, 3.6, 3.1, ModelData[ModelActive].voltage);
	} else {
		draw_value_barmeter(esContext, -0.2, -0.85, 0.1, -0.15, "VOLTS", "V", 0, 8.0, 14.0, 11.0, 10.0, ModelData[ModelActive].voltage);
	}
	draw_value_barmeter(esContext, 0.3, -0.85, 0.6, -0.15, "AMPS", "A", 0, 0.0, 100.0, 75.0, 50.0, ModelData[ModelActive].ampere);
	if (ModelData[ModelActive].voltage_rx == 0) {
		draw_value_barmeter_duo(esContext, 0.8, -0.85, 1.1, -0.15, "VOLTS(FC)", "V", 0, 3.0, 6.0, 4.8, 4.2, ModelData[ModelActive].fc_voltage1, ModelData[ModelActive].fc_voltage2);
	} else {
		draw_value_barmeter(esContext, 0.8, -0.85, 1.1, -0.15, "VOLTS(RX)", "V", 0, 8.0, 14.0, 11.0, 10.0, ModelData[ModelActive].voltage_rx);
	}

	draw_value_barmeter_duo(esContext, -1.2, -0.2, -0.9, 0.5, "RSSI(Tele)", "", 0, 0.0, 100.0, 40.0, 20.0, ModelData[ModelActive].rssi_tx, ModelData[ModelActive].rssi_rx);
	draw_value_barmeter_duo(esContext, -0.7, -0.2, -0.4, 0.5, "RSSI(RC)", "", 0, 0.0, 100.0, 40.0, 20.0, ModelData[ModelActive].rssi_rc_tx, ModelData[ModelActive].rssi_rc_rx);
	if (ModelData[ModelActive].chancount > 8) {
		draw_value_barchannels(esContext, -0.2, -0.2, "Channels", -100, 100, 8, ModelData[ModelActive].radio);
		if (ModelData[ModelActive].chancount > 16) {
			ModelData[ModelActive].chancount = 16;
		}
		draw_value_barchannels(esContext, -0.2, 0.075, "", -100, 100, ModelData[ModelActive].chancount - 8, ModelData[ModelActive].radio + 8);
	} else {
		draw_value_barchannels(esContext, -0.2, -0.2, "Channels", -100, 100, 8, ModelData[ModelActive].radio);
	}
	draw_value_barmeter(esContext, 0.3, -0.2, 0.6, 0.5, "SATS", "", 0, 0.0, 18.0, 6.0, 4.0, ModelData[ModelActive].numSat);
	draw_value_barmeter(esContext, 0.8, -0.2, 1.1, 0.5, "HEART", "", 0, 0.0, 100.0, 40.0, 20.0, ModelData[ModelActive].heartbeat);

	uint8_t n = 0;
	extern char *escStates[];
	extern char *escErrors[];
	char tmp_str[128];
	for (n = 0; n < ModelData[ModelActive].num_motors && n < 8; n++) {
		float xn = -1.35;
		sprintf(tmp_str, "%irpm", ModelData[ModelActive].escstat[n].rpm);
		draw_text_f(esContext, xn + (0.05 * (10 - strlen(tmp_str)) * 0.6), 0.5 + (n * 0.05) + 0.01, 0.05, 0.05, FONT_WHITE, tmp_str);
		draw_value_barH(esContext, xn + 0.33, 0.5 + (n * 0.05) + 0.012, xn + 0.65, 0.5 + (n * 0.05) + 0.049, 0, 10000, (int16_t)ModelData[ModelActive].escstat[n].rpm);
		xn += 0.55;
		sprintf(tmp_str, "%0.1fV", ModelData[ModelActive].escstat[n].vin);
		draw_text_f(esContext, xn + (0.05 * (10 - strlen(tmp_str)) * 0.6), 0.5 + (n * 0.05) + 0.01, 0.05, 0.05, FONT_WHITE, tmp_str);
		draw_value_barH(esContext, xn + 0.33, 0.5 + (n * 0.05) + 0.012, xn + 0.65, 0.5 + (n * 0.05) + 0.049, 0, 240, (int16_t)ModelData[ModelActive].escstat[n].vin * 10.0);
		xn += 0.55;
		sprintf(tmp_str, "%0.1fA", ModelData[ModelActive].escstat[n].amps);
		draw_text_f(esContext, xn + (0.05 * (10 - strlen(tmp_str)) * 0.6), 0.5 + (n * 0.05) + 0.01, 0.05, 0.05, FONT_WHITE, tmp_str);
		draw_value_barH(esContext, xn + 0.33, 0.5 + (n * 0.05) + 0.012, xn + 0.65, 0.5 + (n * 0.05) + 0.049, 0, 500, (int16_t)ModelData[ModelActive].escstat[n].amps * 10.0);
		xn += 0.7;
		sprintf(tmp_str, "%s", escStates[ModelData[ModelActive].escstat[n].stat]);
		draw_text_f(esContext, xn, 0.5 + (n * 0.05) + 0.01, 0.05, 0.05, FONT_WHITE, tmp_str);
		xn += 0.3;
		sprintf(tmp_str, "%s", escErrors[ModelData[ModelActive].escstat[n].err]);
		draw_text_f(esContext, xn, 0.5 + (n * 0.05) + 0.01, 0.05, 0.05, FONT_WHITE, tmp_str);
		xn += 0.2;
		if (ModelData[ModelActive].escstat[n].age > 65000 || ModelData[ModelActive].escstat[n].age == 0) {
			draw_text_f(esContext, xn + (0.05 * (10 - strlen(tmp_str)) * 0.6), 0.5 + (n * 0.05) + 0.01, 0.05, 0.05, FONT_PINK, "FAILED");
		} else {
			sprintf(tmp_str, "%i", ModelData[ModelActive].escstat[n].age);
			draw_text_f(esContext, xn + (0.05 * (10 - strlen(tmp_str)) * 0.6), 0.5 + (n * 0.05) + 0.01, 0.05, 0.05, FONT_WHITE, tmp_str);
		}
		xn += 0.3;
	}

#ifndef ANDROID
#ifdef SDLGL
	glPopMatrix();
#endif
	glEnable( GL_DEPTH_TEST );
#endif
}



