#include <avr/pgmspace.h>

#include "marlin.h"
#include "cardreader.h"//This code uses the card.longFilename as buffer to store data, to save memory.
#include "temperature.h"
#include "ConfigurationStore.h"
#include "UltiLCD2_hi_lib.h"
#include "UltiLCD2_menu_material.h"
#include "UltiLCD2_menu_first_run.h"
#include "UltiLCD2_menu_print.h"

void lcd_menu_main();//TODO
void doCooldown();

static void lcd_menu_first_run_init_2();
static void lcd_menu_first_run_init_remove_knobs();
static void lcd_menu_first_run_init_3();

static void lcd_menu_first_run_bed_level_center_adjust();
static void lcd_menu_first_run_bed_level_left_adjust();
static void lcd_menu_first_run_bed_level_right_adjust();
static void lcd_menu_first_run_bed_level_paper();
static void lcd_menu_first_run_bed_level_paper_center();
static void lcd_menu_first_run_bed_level_paper_left();
static void lcd_menu_first_run_bed_level_paper_right();

static void lcd_menu_first_run_material_load();
static void lcd_menu_first_run_material_load_heatup();
static void lcd_menu_first_run_material_load_insert();
static void lcd_menu_first_run_material_load_forward();
static void lcd_menu_first_run_material_load_wait();

static void lcd_menu_first_run_material_select_1();
static void lcd_menu_first_run_material_select_pla_abs();
static void lcd_menu_first_run_material_select_confirm_pla();
static void lcd_menu_first_run_material_select_confirm_abs();
static void lcd_menu_first_run_material_select_2();

static void lcd_menu_first_run_print_1();
static void lcd_menu_first_run_print_card_detect();

void lcd_menu_first_run_init()
{
    SELECT_MENU_ITEM(0);
    lcd_info_screen(lcd_menu_first_run_init_2, NULL, PSTR("CONTINUE"));
    lcd_lib_draw_string_centerP(10, PSTR("Welcome to the first"));
    lcd_lib_draw_string_centerP(20, PSTR("start up of your"));
    lcd_lib_draw_string_centerP(30, PSTR("Ultimaker! Use the"));
    lcd_lib_draw_string_centerP(40, PSTR("wheel to continue"));
    lcd_lib_update_screen();
}

static void homeAndRaiseBed()
{
    enquecommand_P(PSTR("G28 Z0"));
    char buffer[32];
    sprintf_P(buffer, PSTR("G1 F%i Z%i"), int(homing_feedrate[0]), 35);
    enquecommand(buffer);
}

static void lcd_menu_first_run_init_2()
{
    SELECT_MENU_ITEM(0);
    lcd_info_screen(lcd_menu_first_run_init_remove_knobs, homeAndRaiseBed, PSTR("CONTINUE"));
    lcd_lib_draw_string_centerP(10, PSTR("Because this is the"));
    lcd_lib_draw_string_centerP(20, PSTR("first start up I will"));
    lcd_lib_draw_string_centerP(30, PSTR("walk you through"));
    lcd_lib_draw_string_centerP(40, PSTR("a first run wizard."));
    lcd_lib_update_screen();
}

static void lcd_menu_first_run_init_remove_knobs()
{
    LED_GLOW();
    
    SELECT_MENU_ITEM(0);
    lcd_info_screen(lcd_menu_first_run_init_3, NULL, PSTR("CONTINUE"));
    lcd_lib_draw_string_centerP(10, PSTR("Please remove the"));
    lcd_lib_draw_string_centerP(20, PSTR("black knobs"));
    lcd_lib_draw_string_centerP(30, PSTR("under the"));
    lcd_lib_draw_string_centerP(40, PSTR("print platform."));
    lcd_lib_update_screen();
}

static void homeAndParkHeadForCenterAdjustment()
{
    add_homeing[Z_AXIS] = 0;
    enquecommand_P(PSTR("G28"));
    char buffer[32];
    sprintf_P(buffer, PSTR("G1 F%i Z%i X%i Y%i"), int(homing_feedrate[0]), 35, X_MAX_LENGTH/2, Y_MAX_LENGTH - 10);
    enquecommand(buffer);
}

static void lcd_menu_first_run_init_3()
{
    SELECT_MENU_ITEM(0);
    lcd_info_screen(lcd_menu_first_run_bed_level_center_adjust, homeAndParkHeadForCenterAdjustment, PSTR("CONTINUE"));
    lcd_lib_draw_string_centerP(10, PSTR("After transportation"));
    lcd_lib_draw_string_centerP(20, PSTR("we need to do some"));
    lcd_lib_draw_string_centerP(30, PSTR("adjustments, we are"));
    lcd_lib_draw_string_centerP(40, PSTR("going to do that now."));
    lcd_lib_update_screen();
}

static void storeHomingZ_parkHeadForLeftAdjustment()
{
    add_homeing[Z_AXIS] -= current_position[Z_AXIS];
    Config_StoreSettings();
    enquecommand_P("G92 Z0");

    char buffer[32];
    sprintf_P(buffer, PSTR("G1 F%i Z5"), int(homing_feedrate[Z_AXIS]));
    enquecommand(buffer);
    sprintf_P(buffer, PSTR("G1 F%i X%i Y%i"), int(homing_feedrate[X_AXIS]), 5, 10);
    enquecommand(buffer);
    sprintf_P(buffer, PSTR("G1 F%i Z0"), int(homing_feedrate[Z_AXIS]));
    enquecommand(buffer);
}

static void lcd_menu_first_run_bed_level_center_adjust()
{
    LED_GLOW();
    
    if (lcd_lib_encoder_pos == ENCODER_NO_SELECTION)
        lcd_lib_encoder_pos = 0;
    
    if (lcd_lib_encoder_pos != 0)
    {
        current_position[Z_AXIS] += float(lcd_lib_encoder_pos) * 0.05;
        lcd_lib_encoder_pos = 0;
        plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], 60, 0);
    }

    lcd_info_screen(lcd_menu_first_run_bed_level_left_adjust, storeHomingZ_parkHeadForLeftAdjustment, PSTR("COMPLETED"));
    lcd_lib_draw_string_centerP(10, PSTR("Rotate the knob till"));
    lcd_lib_draw_string_centerP(20, PSTR("the nozzle is half a"));
    lcd_lib_draw_string_centerP(30, PSTR("millimeter away"));
    lcd_lib_draw_string_centerP(40, PSTR("from the printbed."));
    lcd_lib_update_screen();
}

static void parkHeadForRightAdjustment()
{
    char buffer[32];
    sprintf_P(buffer, PSTR("G1 F%i Z5"), int(homing_feedrate[Z_AXIS]));
    enquecommand(buffer);
    sprintf_P(buffer, PSTR("G1 F%i X%i Y%i"), int(homing_feedrate[X_AXIS]), X_MAX_POS - 5, 10);
    enquecommand(buffer);
    sprintf_P(buffer, PSTR("G1 F%i Z0"), int(homing_feedrate[Z_AXIS]));
    enquecommand(buffer);
}

static void lcd_menu_first_run_bed_level_left_adjust()
{
    LED_GLOW();
    SELECT_MENU_ITEM(0);
        
    lcd_info_screen(lcd_menu_first_run_bed_level_right_adjust, parkHeadForRightAdjustment, PSTR("COMPLETED"));
    lcd_lib_draw_string_centerP(10, PSTR("Turn left platform"));
    lcd_lib_draw_string_centerP(20, PSTR("screw till the nozzle"));
    lcd_lib_draw_string_centerP(30, PSTR("is a millimeter away"));
    lcd_lib_draw_string_centerP(40, PSTR("from the printbed."));
    
    lcd_lib_update_screen();
}

static void lcd_menu_first_run_bed_level_right_adjust()
{
    LED_GLOW();
    SELECT_MENU_ITEM(0);
    lcd_info_screen(lcd_menu_first_run_bed_level_paper, NULL, PSTR("COMPLETED"));
    lcd_lib_draw_string_centerP(10, PSTR("Turn right platform"));
    lcd_lib_draw_string_centerP(20, PSTR("screw till the nozzle"));
    lcd_lib_draw_string_centerP(30, PSTR("is a millimeter away"));
    lcd_lib_draw_string_centerP(40, PSTR("from the printbed."));
    
    lcd_lib_update_screen();
}

static void parkHeadForCenterAdjustment()
{
    char buffer[32];
    sprintf_P(buffer, PSTR("G1 F%i Z5"), int(homing_feedrate[Z_AXIS]));
    enquecommand(buffer);
    sprintf_P(buffer, PSTR("G1 F%i X%i Y%i"), int(homing_feedrate[X_AXIS]), X_MAX_LENGTH / 2, Y_MAX_LENGTH - 10);
    enquecommand(buffer);
    sprintf_P(buffer, PSTR("G1 F%i Z0"), int(homing_feedrate[Z_AXIS]));
    enquecommand(buffer);
}

static void lcd_menu_first_run_bed_level_paper()
{
    SELECT_MENU_ITEM(0);
    lcd_info_screen(lcd_menu_first_run_bed_level_paper_center, parkHeadForCenterAdjustment, PSTR("CONTINUE"));
    lcd_lib_draw_string_centerP(10, PSTR("Repeat this step, but"));
    lcd_lib_draw_string_centerP(20, PSTR("now use a paper"));
    lcd_lib_draw_string_centerP(30, PSTR("sheet to finetune"));
    lcd_lib_draw_string_centerP(40, PSTR("the bed level"));
    lcd_lib_update_screen();
}

static void lcd_menu_first_run_bed_level_paper_center()
{
    LED_GLOW();
    
    if (lcd_lib_encoder_pos == ENCODER_NO_SELECTION)
        lcd_lib_encoder_pos = 0;
    
    if (lcd_lib_encoder_pos != 0)
    {
        current_position[Z_AXIS] += float(lcd_lib_encoder_pos) * 0.05;
        lcd_lib_encoder_pos = 0;
        plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], 60, 0);
    }

    lcd_info_screen(lcd_menu_first_run_bed_level_paper_left, storeHomingZ_parkHeadForLeftAdjustment, PSTR("COMPLETED"));
    lcd_lib_draw_string_centerP(10, PSTR("Slide a paper between"));
    lcd_lib_draw_string_centerP(20, PSTR("the bed & nozzle till"));
    lcd_lib_draw_string_centerP(30, PSTR("paper get a little"));
    lcd_lib_draw_string_centerP(40, PSTR("bit resistance."));
    lcd_lib_update_screen();
}

static void lcd_menu_first_run_bed_level_paper_left()
{
    LED_GLOW();

    SELECT_MENU_ITEM(0);
    lcd_info_screen(lcd_menu_first_run_bed_level_paper_right, parkHeadForRightAdjustment, PSTR("COMPLETED"));
    lcd_lib_draw_string_centerP(20, PSTR("repeat this for"));
    lcd_lib_draw_string_centerP(30, PSTR("the left corner..."));
    lcd_lib_update_screen();
}

static void homeBed()
{
    enquecommand_P(PSTR("G28 Z0"));
}

static void lcd_menu_first_run_bed_level_paper_right()
{
    LED_GLOW();

    SELECT_MENU_ITEM(0);
    lcd_info_screen(lcd_menu_first_run_material_load, homeBed, PSTR("COMPLETED"));
    lcd_lib_draw_string_centerP(20, PSTR("repeat this for"));
    lcd_lib_draw_string_centerP(30, PSTR("the right corner..."));
    lcd_lib_update_screen();
}

static void parkHeadForHeating()
{
    enquecommand_P(PSTR("G1 F12000 X110 Y0"));
}

static void lcd_menu_first_run_material_load()
{
    SELECT_MENU_ITEM(0);
    lcd_info_screen(lcd_menu_first_run_material_load_heatup, parkHeadForHeating, PSTR("COMPLETED"));
    lcd_lib_draw_string_centerP(10, PSTR("Now that we"));
    lcd_lib_draw_string_centerP(20, PSTR("leveled the bed"));
    lcd_lib_draw_string_centerP(30, PSTR("the next step is"));
    lcd_lib_draw_string_centerP(40, PSTR("to insert material."));
    lcd_lib_update_screen();
}

static void lcd_menu_first_run_material_load_heatup()
{
    setTargetHotend(230, 0);
    int16_t temp = degHotend(0) - 20;
    int16_t target = degTargetHotend(0) - 10 - 20;
    if (temp < 0) temp = 0;
    if (temp > target)
    {
        volume_to_filament_length = 1.0;//Set the extrusion to 1mm per given value, so we can move the filament a set distance.
        
        currentMenu = lcd_menu_first_run_material_load_insert;
        temp = target;
    }

    uint8_t progress = uint8_t(temp * 125 / target);
    if (progress < minProgress)
        progress = minProgress;
    else
        minProgress = progress;
    
    lcd_basic_screen();
    lcd_lib_draw_string_centerP(10, PSTR("Please wait"));
    lcd_lib_draw_string_centerP(20, PSTR("Printhead heating for"));
    lcd_lib_draw_string_centerP(30, PSTR("material loading"));

    lcd_progressbar(progress);
    
    lcd_lib_update_screen();
}

static void runMaterialForward()
{
    //Override the max feedrate and acceleration values to get a better insert speed and speedup/slowdown
    float old_max_feedrate_e = max_feedrate[E_AXIS];
    float old_retract_acceleration = retract_acceleration;
    max_feedrate[E_AXIS] = FILAMENT_INSERT_FAST_SPEED;
    retract_acceleration = FILAMENT_LONG_MOVE_ACCELERATION;
    
    current_position[E_AXIS] = 0;
    plan_set_e_position(current_position[E_AXIS]);
    for(uint8_t n=0;n<6;n++)
    {
        current_position[E_AXIS] += FILAMENT_FORWARD_LENGTH / 6;
        plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], FILAMENT_INSERT_FAST_SPEED, 0);
    }
    
    //Put back origonal values.
    max_feedrate[E_AXIS] = old_max_feedrate_e;
    retract_acceleration = old_retract_acceleration;
}

static void lcd_menu_first_run_material_load_insert()
{
    LED_GLOW();
    
    if (movesplanned() < 2)
    {
        current_position[E_AXIS] += 0.5;
        plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], FILAMENT_INSERT_SPEED, 0);
    }
    
    SELECT_MENU_ITEM(0);
    lcd_info_screen(lcd_menu_first_run_material_load_forward, runMaterialForward, PSTR("READY"));
    lcd_lib_draw_string_centerP(10, PSTR("Insert new material"));
    lcd_lib_draw_string_centerP(20, PSTR("from the backside of"));
    lcd_lib_draw_string_centerP(30, PSTR("your machine,"));
    lcd_lib_draw_string_centerP(40, PSTR("above the arrow."));
    lcd_lib_update_screen();
}

static void lcd_menu_first_run_material_load_forward()
{
    lcd_basic_screen();
    lcd_lib_draw_string_centerP(20, PSTR("Loading material..."));
    
    if (!blocks_queued())
    {
        lcd_lib_beep();
        led_glow_dir = led_glow = 0;
        //TODO: Set E motor power lower so the motor skips instead of digging into the material
        currentMenu = lcd_menu_first_run_material_load_wait;
        SELECT_MENU_ITEM(0);
    }

    long pos = st_get_position(E_AXIS);
    long targetPos = lround(FILAMENT_FORWARD_LENGTH*axis_steps_per_unit[E_AXIS]);
    uint8_t progress = (pos * 125 / targetPos);
    lcd_progressbar(progress);
    
    lcd_lib_update_screen();
}

static void lcd_menu_first_run_material_load_wait()
{
    LED_GLOW();
    
    lcd_info_screen(lcd_menu_first_run_material_select_1, doCooldown, PSTR("READY"));
    lcd_lib_draw_string_centerP(10, PSTR("Push wheel when"));
    lcd_lib_draw_string_centerP(20, PSTR("material exits"));
    lcd_lib_draw_string_centerP(30, PSTR("from nozzle..."));

    if (movesplanned() < 2)
    {
        current_position[E_AXIS] += 0.5;
        plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], FILAMENT_INSERT_EXTRUDE_SPEED, 0);
    }
    
    lcd_lib_update_screen();
}

static void lcd_menu_first_run_material_select_1()
{
    SELECT_MENU_ITEM(0);
    lcd_info_screen(lcd_menu_first_run_material_select_pla_abs, doCooldown, PSTR("READY"));
    lcd_lib_draw_string_centerP(10, PSTR("Check your filament"));
    lcd_lib_draw_string_centerP(20, PSTR("and tell your UM2"));
    lcd_lib_draw_string_centerP(30, PSTR("what kind of material"));
    lcd_lib_draw_string_centerP(40, PSTR("it is going to use."));
    lcd_lib_update_screen();
}

static void lcd_menu_first_run_material_select_pla_abs()
{
    LED_GLOW();
    lcd_tripple_menu(PSTR("PLA"), PSTR("ABS"), NULL);
    lcd_lib_update_screen();
    
    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED(0))
        {
            lcd_material_reset_defaults();
            lcd_material_set_material(0);
            lcd_change_to_menu(lcd_menu_first_run_material_select_confirm_pla);
        }
        else if (IS_SELECTED(1))
        {
            lcd_material_reset_defaults();
            lcd_material_set_material(1);
            lcd_change_to_menu(lcd_menu_first_run_material_select_confirm_abs);
        }
    }
}

static void lcd_menu_first_run_material_select_confirm_pla()
{
    LED_GLOW();
    lcd_question_screen(lcd_menu_first_run_material_select_2, NULL, PSTR("YES"), lcd_menu_first_run_material_select_pla_abs, NULL, PSTR("NO"));
    lcd_lib_draw_string_centerP(10, PSTR("You have chosen"));
    lcd_lib_draw_string_centerP(20, PSTR("PLA as material"));
    lcd_lib_draw_string_centerP(30, PSTR("is this is right?"));
    lcd_lib_update_screen();
}

static void lcd_menu_first_run_material_select_confirm_abs()
{
    LED_GLOW();
    lcd_question_screen(lcd_menu_first_run_material_select_2, NULL, PSTR("YES"), lcd_menu_first_run_material_select_pla_abs, NULL, PSTR("NO"));
    lcd_lib_draw_string_centerP(10, PSTR("You have chosen"));
    lcd_lib_draw_string_centerP(20, PSTR("ABS as material"));
    lcd_lib_draw_string_centerP(30, PSTR("is this is right?"));
    lcd_lib_update_screen();
}

static void lcd_menu_first_run_material_select_2()
{
    SELECT_MENU_ITEM(0);
    lcd_info_screen(lcd_menu_first_run_print_1, NULL, PSTR("CONTINUE"));
    lcd_lib_draw_string_centerP(10, PSTR("Now your ultimaker"));
    lcd_lib_draw_string_centerP(20, PSTR("knows what kind"));
    lcd_lib_draw_string_centerP(30, PSTR("of material it has"));
    lcd_lib_draw_string_centerP(40, PSTR("and how to use this"));
    lcd_lib_update_screen();
}

static void lcd_menu_first_run_print_1()
{
    SELECT_MENU_ITEM(0);
    lcd_info_screen(lcd_menu_first_run_print_card_detect, NULL, PSTR("ARE YOU READY?"));
    lcd_lib_draw_string_centerP(10, PSTR("We have come to"));
    lcd_lib_draw_string_centerP(20, PSTR("the best step"));
    lcd_lib_draw_string_centerP(30, PSTR("and that is making"));
    lcd_lib_draw_string_centerP(40, PSTR("you first print"));
    lcd_lib_update_screen();
}

static void lcd_menu_first_run_print_card_detect()
{
    if (!IS_SD_INSERTED)
    {
        lcd_info_screen(lcd_menu_main);
        lcd_lib_draw_string_centerP(10, PSTR("please insert sd card"));
        lcd_lib_draw_string_centerP(20, PSTR("that came with"));
        lcd_lib_draw_string_centerP(30, PSTR("your ultimaker..."));
        lcd_lib_update_screen();
        card.release();
        return;
    }
    
    if (!card.cardOK)
    {
        lcd_info_screen(lcd_menu_main);
        lcd_lib_draw_string_centerP(16, PSTR("Reading card..."));
        lcd_lib_update_screen();
        card.initsd();
        return;
    }
    
    SELECT_MENU_ITEM(0);
    lcd_info_screen(lcd_menu_print_select, NULL, PSTR("LET'S PRINT"));
    lcd_lib_draw_string_centerP(10, PSTR("Select a printfile"));
    lcd_lib_draw_string_centerP(20, PSTR("on the SD card"));
    lcd_lib_draw_string_centerP(30, PSTR("and hit the knob"));
    lcd_lib_draw_string_centerP(40, PSTR("to print it!"));
    lcd_lib_update_screen();
}