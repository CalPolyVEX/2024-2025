#include "liblvgl/draw/lv_img_buf.h"
#include "liblvgl/lvgl.h" // IWYU pragma: keep
#include "liblvgl/misc/lv_color.h"
#include "liblvgl/widgets/lv_label.h"
#include "lemlib/api.hpp" // IWYU pragma: keep

#define CANVAS_WIDTH 240
#define CANVAS_HEIGHT 240
#define ROBOT_WIDTH 15
#define ROBOT_HEIGHT 15

lv_obj_t *canvas = lv_canvas_create(lv_scr_act());
lv_draw_rect_dsc_t rect_dsc;

lv_obj_t *xLabel = lv_label_create(lv_scr_act());
lv_obj_t *yLabel = lv_label_create(lv_scr_act());
lv_obj_t *thetaLabel = lv_label_create(lv_scr_act());

void drawRobotPosition(int x, int y) {
    int screenX = roundf((-x * 1.7) + 113);
    int screenY = roundf((y * 1.7) + 113);
    lv_canvas_fill_bg(canvas, lv_palette_lighten(LV_PALETTE_GREY, 3), LV_OPA_COVER);
    lv_canvas_draw_rect(canvas, screenX, screenY, ROBOT_WIDTH, ROBOT_HEIGHT, &rect_dsc);
}

void initializeRobotOnScreen() {
    static lv_color_t colorBuffer[LV_CANVAS_BUF_SIZE_TRUE_COLOR(CANVAS_WIDTH, CANVAS_HEIGHT)];
    
    lv_canvas_set_buffer(canvas, colorBuffer, CANVAS_WIDTH, CANVAS_HEIGHT, LV_IMG_CF_TRUE_COLOR);
    lv_canvas_fill_bg(canvas, lv_palette_lighten(LV_PALETTE_GREY, 3), LV_OPA_COVER);

    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.bg_opa = LV_OPA_COVER;
    rect_dsc.bg_color = lv_palette_main(LV_PALETTE_GREEN);
    rect_dsc.border_width = 1;
    rect_dsc.border_opa = LV_OPA_90;
    rect_dsc.border_color = lv_color_black();

    drawRobotPosition(0, 0);
}

void initializeLabels() {
    lv_label_set_text(xLabel, "0");
    lv_label_set_text(yLabel, "0");
    lv_label_set_text(thetaLabel, "0");
    lv_obj_align(xLabel, LV_ALIGN_TOP_MID, 50, 5);
    lv_obj_align(yLabel, LV_ALIGN_TOP_MID, 50, 25);
    lv_obj_align(thetaLabel, LV_ALIGN_TOP_MID, 50, 45);
}

void writeRobotPositionText(lemlib::Pose pose) {
    std::string formattedXPos = fmt::to_string(roundf(pose.x * 100) / 100);
    std::string formattedYPos = fmt::to_string(roundf(pose.y * 100) / 100);
    std::string formattedTheta = fmt::to_string(roundf(pose.theta * 100) / 100);
    char *xPosStr = (char *) formattedXPos.c_str();
    char *yPosStr = (char *) formattedYPos.c_str();
    char *thetaStr = (char *) formattedTheta.c_str();
    lv_label_set_text(xLabel, xPosStr);
    lv_label_set_text(yLabel, yPosStr);
    lv_label_set_text(thetaLabel, thetaStr);
    //lv_label_set_text_fmt(xLabel, "xPosition: %.2f", xPos);
}

void initializeScreen() {
    initializeLabels();
    initializeRobotOnScreen();
}

void updateRobotPositionOnScreen(lemlib::Pose pose) {
    writeRobotPositionText(pose);
    drawRobotPosition(pose.x, pose.y);
}