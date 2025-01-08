#include "liblvgl/draw/lv_img_buf.h"
#include "liblvgl/lvgl.h" // IWYU pragma: keep
#include "liblvgl/misc/lv_color.h"
#include "liblvgl/widgets/lv_label.h"
#include "lemlib/api.hpp" // IWYU pragma: keep

#define CANVAS_WIDTH 240
#define CANVAS_HEIGHT 240
#define ROBOT_WIDTH 15
#define ROBOT_HEIGHT 15
#define LABEL_COUNT 12
#define MAGNITUDE 15

lv_obj_t* canvas = lv_canvas_create(lv_scr_act());
lv_draw_rect_dsc_t position;
lv_draw_rect_dsc_t pointer;

lv_obj_t* labels[LABEL_COUNT];
const char* label_texts[LABEL_COUNT] = {"", "", "", "", "", "", "", "", "", "", "", ""};

void draw_robot_position(float x, float y, float h) {
    int screenX = roundf((-x * 1.7) + 113);
    int screenY = roundf((y * 1.7) + 113);

    h -= M_PI_2;
    h *= -1; // vex coords to cartesian coords

    int pointerX = (screenX + 5) + roundf(MAGNITUDE * cos(h));
    int pointerY = (screenY + 5) - roundf(MAGNITUDE * sin(h));

    lv_canvas_fill_bg(canvas, lv_palette_lighten(LV_PALETTE_GREY, 3), LV_OPA_COVER);
    lv_canvas_draw_rect(canvas, screenX, screenY, ROBOT_WIDTH, ROBOT_HEIGHT, &position);

    lv_canvas_draw_rect(canvas, pointerX, pointerY, ROBOT_WIDTH / 3, ROBOT_HEIGHT / 3, &pointer);
}

void initialize_robot_on_screen() {
    static lv_color_t colorBuffer[LV_CANVAS_BUF_SIZE_TRUE_COLOR(CANVAS_WIDTH, CANVAS_HEIGHT)];

    lv_canvas_set_buffer(canvas, colorBuffer, CANVAS_WIDTH, CANVAS_HEIGHT, LV_IMG_CF_TRUE_COLOR);
    lv_canvas_fill_bg(canvas, lv_palette_lighten(LV_PALETTE_GREY, 3), LV_OPA_COVER);

    lv_draw_rect_dsc_init(&position);
    position.bg_opa = LV_OPA_COVER;
    position.bg_color = lv_palette_main(LV_PALETTE_GREEN);
    position.border_width = 1;
    position.border_opa = LV_OPA_90;
    position.border_color = lv_color_black();

    lv_draw_rect_dsc_init(&pointer);
    pointer.bg_opa = LV_OPA_COVER;
    pointer.bg_color = lv_palette_main(LV_PALETTE_LIGHT_BLUE);
    pointer.border_width = 1;
    pointer.border_opa = LV_OPA_90;
    pointer.border_color = lv_color_black();

    draw_robot_position(0, 0, 0);
}

void initialize_labels() {
    for (int i = 0; i < LABEL_COUNT; i++) {
        labels[i] = lv_label_create(lv_scr_act());
        lv_label_set_text(labels[i], label_texts[i]);
        lv_obj_align(labels[i], LV_ALIGN_OUT_LEFT_TOP, CANVAS_WIDTH + 2, 5 + (i * 20)); // Align each label vertically
    }
}

void print_text_at(int index, const char* text) {
    if (index >= 0 && index < LABEL_COUNT) {
        label_texts[index] = (char*)text;
        lv_label_set_text(labels[index], text);
    }
}

void write_robot_position_text(lemlib::Pose pose) {
    std::string formattedXPos = fmt::to_string(roundf(pose.x * 100) / 100);
    std::string formattedYPos = fmt::to_string(roundf(pose.y * 100) / 100);
    std::string formattedTheta = fmt::to_string(roundf(pose.theta * 100) / 100);

    print_text_at(0, formattedXPos.c_str());
    print_text_at(1, formattedYPos.c_str());
    print_text_at(2, formattedTheta.c_str());
}

void initialize_screen() {
    initialize_labels();
    initialize_robot_on_screen();
}

void update_robot_position_on_screen(lemlib::Pose pose) {
    write_robot_position_text(pose);
    draw_robot_position(pose.x, pose.y, pose.theta);
}
