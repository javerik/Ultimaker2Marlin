#include <avr/pgmspace.h>

#include "marlin.h"
#include "cardreader.h"
#include "temperature.h"
#include "UltiLCD2_hi_lib.h"
#include "UltiLCD2_menu_print.h"
#include "UltiLCD2_menu_material.h"

uint8_t lcd_cache[LCD_CACHE_SIZE];
#define LCD_CACHE_NR_OF_FILES lcd_cache[(LCD_CACHE_SIZE - 1)]
#define LCD_CACHE_ID(n) lcd_cache[(n)]
#define LCD_CACHE_FILENAME(n) ((char*)&lcd_cache[2*LCD_CACHE_COUNT + (n) * LONG_FILENAME_LENGTH])
#define LCD_CACHE_TYPE(n) lcd_cache[LCD_CACHE_COUNT + (n)]

void lcd_menu_main();//TODO
void doCooldown();//TODO
static void lcd_menu_print_heatup();
static void lcd_menu_print_printing();
static void lcd_menu_print_classic_warning();
static void lcd_menu_print_classic();
static void lcd_menu_print_abort();
static void lcd_menu_print_ready();

void lcd_clear_cache()
{
    for(uint8_t n=0; n<LCD_CACHE_COUNT; n++)
        LCD_CACHE_ID(n) = 0xFF;
    LCD_CACHE_NR_OF_FILES = 0xFF;
}

void doCancelPrint()
{
    card.sdprinting = false;
    doCooldown();
    enquecommand_P(PSTR("G28"));
}
static void cardUpdir()
{
    card.updir();
}

static char* lcd_sd_menu_filename_callback(uint8_t nr)
{
    //This code uses the card.longFilename as buffer to store the filename, to save memory.
    if (nr == 0)
    {
        if (card.atRoot())
        {
            strcpy_P(card.longFilename, PSTR("<- RETURN"));
        }else{
            strcpy_P(card.longFilename, PSTR("<- BACK"));
        }
    }else{
        card.longFilename[0] = '\0';
        for(uint8_t idx=0; idx<LCD_CACHE_COUNT; idx++)
        {
            if (LCD_CACHE_ID(idx) == nr)
                strcpy(card.longFilename, LCD_CACHE_FILENAME(idx));
        }
        if (card.longFilename[0] == '\0')
        {
            card.getfilename(nr - 1);
            if (!card.longFilename[0])
                strcpy(card.longFilename, card.filename);
            if (!card.filenameIsDir)
            {
                if (strchr(card.longFilename, '.')) strchr(card.longFilename, '.')[0] = '\0';
            }

            uint8_t idx = nr % LCD_CACHE_COUNT;
            LCD_CACHE_ID(idx) = nr;
            strcpy(LCD_CACHE_FILENAME(idx), card.longFilename);
            LCD_CACHE_TYPE(idx) = card.filenameIsDir ? 1 : 0;
        }
    }
    return card.longFilename;
}

void lcd_sd_menu_details_callback(uint8_t nr)
{
    if (nr == 0)
    {
        lcd_lib_draw_gfx(80, 13, backArrowGfx);
        return;
    }
    for(uint8_t idx=0; idx<LCD_CACHE_COUNT; idx++)
    {
        if (LCD_CACHE_ID(idx) == nr)
        {
            if (LCD_CACHE_TYPE(idx) == 1)
            {
                lcd_lib_draw_gfx(80, 20, folderGfx);
            }else{
                lcd_lib_draw_stringP(70, 15, PSTR("No info"));
                lcd_lib_draw_stringP(70, 25, PSTR("Available"));
            }
        }
    }
}

void lcd_menu_print_select()
{
    if (!IS_SD_INSERTED)
    {
        lcd_lib_encoder_pos = MENU_ITEM_POS(0);
        lcd_info_screen(lcd_menu_main);
        lcd_lib_draw_string_centerP(15, PSTR("No SD CARD!"));
        lcd_lib_draw_string_centerP(25, PSTR("Please insert card"));
        lcd_lib_update_screen();
        card.release();
        return;
    }
    
    if (!card.cardOK)
    {
        lcd_info_screen(lcd_menu_main);
        lcd_lib_draw_string_centerP(16, PSTR("Reading card..."));
        lcd_lib_update_screen();
        lcd_clear_cache();
        card.initsd();
        return;
    }
    
    if (LCD_CACHE_NR_OF_FILES == 0xFF)
        LCD_CACHE_NR_OF_FILES = card.getnrfilenames();
    uint8_t nrOfFiles = LCD_CACHE_NR_OF_FILES + 1;
    if (nrOfFiles == 1)
    {
        if (card.atRoot())
            lcd_info_screen(lcd_menu_main, NULL, PSTR("OK"));
        else
            lcd_info_screen(lcd_menu_print_select, cardUpdir, PSTR("OK"));
        lcd_lib_draw_string_centerP(25, PSTR("No files found!"));
        lcd_lib_update_screen();
        lcd_clear_cache();
        return;
    }
    
    if (lcd_lib_button_pressed)
    {
        uint8_t selIndex = uint16_t(lcd_lib_encoder_pos/ENCODER_TICKS_PER_MENU_ITEM);
        if (selIndex == 0)
        {
            if (card.atRoot())
            {
                lcd_change_to_menu(lcd_menu_main);
            }else{
                lcd_clear_cache();
                lcd_lib_beep();
                card.updir();
            }
        }else{
            card.getfilename(selIndex - 1);
            if (!card.filenameIsDir)
            {
                //Start print
                card.openFile(card.filename, true);
                if (card.isFileOpen())
                {
                    card.longFilename[20] = '\0';
                    if (strchr(card.longFilename, '.')) strchr(card.longFilename, '.')[0] = '\0';
                    
                    char buffer[64];
                    card.fgets(buffer, sizeof(buffer));
                    buffer[sizeof(buffer)-1] = '\0';
                    while (strlen(buffer) > 0 && buffer[strlen(buffer)-1] < ' ') buffer[strlen(buffer)-1] = '\0';
                    if (strcmp_P(buffer, PSTR(";FLAVOR:UltiGCode")) != 0)
                    {
                        card.fgets(buffer, sizeof(buffer));
                        buffer[sizeof(buffer)-1] = '\0';
                        while (strlen(buffer) > 0 && buffer[strlen(buffer)-1] < ' ') buffer[strlen(buffer)-1] = '\0';
                    }
                    card.setIndex(0);
                    if (strcmp_P(buffer, PSTR(";FLAVOR:UltiGCode")) == 0)
                    {
                        //New style GCode flavor without start/end code.
                        // Temperature settings, filament settings, fan settings, start and end-code are machine controlled.
                        target_temperature[0] = material.temperature;
                        target_temperature_bed = material.bed_temperature;
                        volume_to_filament_length = 1.0 / (M_PI * (material.diameter / 2.0) * (material.diameter / 2.0));
                        fanSpeedPercent = material.fan_speed;
                        extrudemultiply = material.flow;
                        
                        fanSpeed = 0;
                        lcd_change_to_menu(lcd_menu_print_heatup);
                    }else{
                        //Classic gcode file
                        
                        //Set the settings to defaults so the classic GCode has full control
                        fanSpeedPercent = 100;
                        volume_to_filament_length = 1.0;
                        extrudemultiply = 100;
                        
                        lcd_change_to_menu(lcd_menu_print_classic_warning, MENU_ITEM_POS(0));
                    }
                }
            }else{
                lcd_lib_beep();
                lcd_clear_cache();
                card.chdir(card.filename);
                SELECT_MENU_ITEM(0);
            }
            return;//Return so we do not continue after changing the directory or selecting a file. The nrOfFiles is invalid at this point.
        }
    }
    lcd_scroll_menu(PSTR("SD CARD"), nrOfFiles, lcd_sd_menu_filename_callback, lcd_sd_menu_details_callback);
}

static void lcd_menu_print_heatup()
{
    lcd_info_screen(lcd_menu_print_abort, NULL, PSTR("ABORT"));
    
    if (current_temperature[0] >= target_temperature[0] - TEMP_WINDOW && current_temperature_bed >= target_temperature_bed - TEMP_WINDOW)
    {
        //TODO: Custom start code.
        enquecommand_P(PSTR("G28"));
        enquecommand_P(PSTR("G92 E-5"));
        enquecommand_P(PSTR("G1 F200 E0"));
        card.startFileprint();
        starttime = millis();
        currentMenu = lcd_menu_print_printing;
    }

    uint8_t progress = 125;
    uint8_t p;
    
    p = (current_temperature[0] - 20) * 125 / (target_temperature[0] - 20 - TEMP_WINDOW);
    if (p < progress)
        progress = p;
    p = (current_temperature_bed - 20) * 125 / (target_temperature_bed - 20 - TEMP_WINDOW);
    if (p < progress)
        progress = p;
    
    if (progress < minProgress)
        progress = minProgress;
    else
        minProgress = progress;
    
    lcd_lib_draw_string_centerP(15, PSTR("Heating up"));
    lcd_lib_draw_string_centerP(25, PSTR("before printing"));

    lcd_progressbar(progress);
    
    lcd_lib_update_screen();
}

static void lcd_menu_print_printing()
{
    lcd_question_screen(NULL, NULL, PSTR("TUNE"), lcd_menu_print_abort, NULL, PSTR("ABORT"));
    
    uint8_t progress = card.getFilePos() / ((card.getFileSize() + 123) / 124);
    lcd_lib_draw_string_centerP(15, PSTR("Printing"));
    lcd_lib_draw_string_center(25, card.longFilename);
    unsigned long printTime = (millis() - starttime);
    unsigned long totalTime = float(printTime) * float(card.getFileSize()) / float(card.getFilePos());
    unsigned long timeLeft = (totalTime - printTime) / 1000L;
    
    char buffer[8];
    int_to_time_string(timeLeft, buffer);
    lcd_lib_draw_string(5, 5, buffer);

    if (!card.sdprinting && !is_command_queued())
    {
        doCooldown();
        enquecommand_P(PSTR("G92 E5"));
        enquecommand_P(PSTR("G1 F200 E0"));
        enquecommand_P(PSTR("G28"));
        currentMenu = lcd_menu_print_ready;
        SELECT_MENU_ITEM(0);
    }

    lcd_progressbar(progress);
    
    lcd_lib_update_screen();
}

static void doStartPrint()
{
    card.startFileprint();
    starttime = millis();
}
static void lcd_menu_print_classic_warning()
{
    lcd_question_screen(lcd_menu_print_classic, doStartPrint, PSTR("CONTINUE"), lcd_menu_print_select, NULL, PSTR("CANCEL"));
    
    lcd_lib_draw_string_centerP(10, PSTR("This file ignores"));
    lcd_lib_draw_string_centerP(20, PSTR("material settings"));

    lcd_lib_update_screen();
}

static void lcd_menu_print_classic()
{
    lcd_info_screen(lcd_menu_print_abort, NULL, PSTR("ABORT"));
    
    uint8_t progress = card.getFilePos() / ((card.getFileSize() + 123) / 124);
    if (current_temperature[0] < target_temperature[0] - TEMP_WINDOW - 5 || current_temperature_bed < target_temperature_bed - TEMP_WINDOW - 5)
    {
        progress = 125;
        uint8_t p;
        
        p = (current_temperature[0] - 20) * 125 / (target_temperature[0] - 20 - TEMP_WINDOW);
        if (p < progress)
            progress = p;
        p = (current_temperature_bed - 20) * 125 / (target_temperature_bed - 20 - TEMP_WINDOW);
        if (p < progress)
            progress = p;
        
        if (progress < minProgress)
            progress = minProgress;
        else
            minProgress = progress;
        lcd_lib_draw_string_centerP(15, PSTR("Heating"));
    }else{
        lcd_lib_draw_string_centerP(15, PSTR("Printing"));
    }
    lcd_lib_draw_string_center(25, card.longFilename);
    
    if (!card.sdprinting)
    {
        doCooldown();
        currentMenu = lcd_menu_print_ready;
        SELECT_MENU_ITEM(0);
    }

    lcd_progressbar(progress);
    
    lcd_lib_update_screen();
}

static void lcd_menu_print_abort()
{
    lcd_question_screen(lcd_menu_print_ready, doCancelPrint, PSTR("YES"), previousMenu, NULL, PSTR("NO"));
    
    lcd_lib_draw_string_centerP(15, PSTR("Abort the print?"));

    lcd_lib_update_screen();
}

static void lcd_menu_print_ready()
{
    lcd_info_screen(lcd_menu_main, NULL, PSTR("BACK TO MENU"));

    if (current_temperature[0] > 80)
    {
        lcd_lib_draw_string_centerP(15, PSTR("Printer cooling down"));

        int16_t progress = 124 - (current_temperature[0] - 80);
        if (progress < 0) progress = 0;
        if (progress > 124) progress = 124;
        
        if (progress < minProgress)
            progress = minProgress;
        else
            minProgress = progress;
            
        lcd_progressbar(progress);
        char buffer[8];
        int_to_string(current_temperature[0], buffer, PSTR("C"));
        lcd_lib_draw_string_center(25, buffer);
    }else{
        lcd_lib_draw_string_centerP(15, PSTR("Print finished"));
    }
    lcd_lib_update_screen();
}