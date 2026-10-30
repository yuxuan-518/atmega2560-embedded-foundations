#include "display_view.h"
#include "config.h"
#include "gfx.h"
#include "motor.h"
#include "fault.h"
#include "estop.h"
#include "limit.h"
#include "homing.h"
#include "position.h"
#include "range.h"
#include "nh3.h"
#include "imu.h"
#include "mpu6050.h"
#include "pot.h"
#include "light.h"
#include "counter.h"
#include "loopback.h"
#include "dac.h"
#include "scanner.h"
#include "usart.h"

#include <avr/pgmspace.h>

typedef struct {
    uint32_t now;
    uint8_t page;
    motor_state_t motor;
    int32_t position;
    uint32_t hz;
    uint32_t speed_hz;
    uint32_t max_hz;
    uint8_t dir;
    bool homed;
    bool positioning;
    int32_t target;
    fault_t fault;
    bool estop;
    bool limit_min;
    bool limit_max;
    bool range_on;
    bool range_fresh;
    bool range_valid;
    uint32_t range_count;
    range_result_t range_result;
    uint16_t range_mm;
    range_alarm_t alarm;
    uint32_t alarm_mm;
    nh3_state_t nh3;
    uint32_t ppb;
    uint32_t warm_left;
    imu_state_t imu;
    bool imu_fresh;
    bool imu_has;
    int32_t accel_mg[3];
    bool pot_valid[2];
    uint16_t pot_raw[2];
    light_data_t light;
    uint16_t light_level;
    bool lamp;
    bool counter_on;
    uint32_t count;
    uint32_t rate_x100;
    loop_state_t loop;
    bool dac_has;
    uint16_t dac;
    uint8_t found;
    uint32_t accel;
    int32_t soft_min;
    int32_t soft_max;
    uint32_t display_errors;
    uint32_t imu_errors;
    uint32_t spi_errors;
} view_t;

static view_t view;
static char row[GFX_TEXT_COLUMNS + 1];
static uint8_t row_len = 0;

static const char page_motion[] PROGMEM = "MOTION";
static const char page_sensor[] PROGMEM = "SENSOR";
static const char page_system[] PROGMEM = "SYSTEM";

static const char *const page_names[] PROGMEM = {
    page_motion,
    page_sensor,
    page_system
};

static void row_begin(void)
{
    row_len = 0;
    row[0] = '\0';
}

static void row_char(char c)
{
    if(row_len < GFX_TEXT_COLUMNS){
        row[row_len] = c;
        row_len++;
        row[row_len] = '\0';
    }
}

static void row_P(const char *text)
{
    char c = pgm_read_byte(text);

    while(c != '\0'){
        row_char(c);
        text++;
        c = pgm_read_byte(text);
    }
}

static void row_pad(uint8_t col)
{
    while(row_len < col && row_len < GFX_TEXT_COLUMNS){
        row_char(' ');
    }
}

static const uint32_t row_powers[] PROGMEM = {
    1000000000UL, 100000000UL, 10000000UL, 1000000UL, 100000UL, 10000UL, 1000UL, 100UL, 10UL, 1UL
};

static void row_number(uint32_t value, bool negative, uint8_t width, uint8_t min_digits)
{
    char digits[10];
    uint8_t count = 0;

    for(uint8_t i = 0; i < 10; i++){
        uint32_t power = pgm_read_dword(&row_powers[i]);
        char digit = '0';

        while(value >= power){
            value -= power;
            digit++;
        }

        if(count != 0 || digit != '0' || i == 9 || 10 - i <= min_digits){
            digits[count] = digit;
            count++;
        }
    }

    uint8_t length = (uint8_t)(count + (negative ? 1 : 0));

    while(length < width){
        row_char(' ');
        length++;
    }

    if(negative){
        row_char('-');
    }

    for(uint8_t i = 0; i < count; i++){
        row_char(digits[i]);
    }
}

static void row_uint(uint32_t value, uint8_t width)
{
    row_number(value, false, width, 1);
}

static void row_int(int32_t value, uint8_t width)
{
    if(value < 0){
        row_number((uint32_t)(-(value + 1)) + 1, true, width, 1);
    } else {
        row_number((uint32_t)value, false, width, 1);
    }
}

static void row_capped(uint32_t value)
{
    if(value > 999){
        row_P(PSTR("999+"));
    } else {
        row_uint(value, 1);
    }
}

static void row_dash(uint8_t width)
{
    row_pad((uint8_t)(row_len + width - 1));
    row_char('-');
}

static void row_motor_state(motor_state_t state)
{
    static const char names[][10] PROGMEM = {
        "STOPPED", "ACCEL", "CRUISE", "DECEL", "STOPPING", "REVERSING", "JOG"
    };

    if(state <= MOTOR_JOG){
        row_P(names[state]);
    }
}

static void row_range_result(range_result_t result)
{
    static const char names[][10] PROGMEM = {
        "WAIT", "OK", "NO ECHO", "TIMEOUT", "STUCK", "TOO SHORT", "TOO LONG"
    };

    if(result <= RANGE_TOO_LONG){
        row_P(names[result]);
    }
}

void display_view_take(uint32_t now, uint8_t page, uint32_t display_errors)
{
    const config_t *cfg = config_get();
    int16_t raw[3];

    view.now = now;
    view.page = page;
    view.motor = motor_get_state();
    view.position = motor_get_position();
    view.hz = motor_get_current_hz();
    view.speed_hz = cfg->speed_hz;
    view.max_hz = cfg->max_hz;
    view.dir = motor_get_direction();
    view.homed = homing_is_homed();
    view.positioning = position_busy();
    view.target = position_get_target();
    view.fault = fault_get();
    view.estop = estop_input_active();
    view.limit_min = limit_min_active();
    view.limit_max = limit_max_active();
    view.range_on = range_is_running();
    view.range_fresh = range_is_fresh(now);
    view.range_valid = range_has_valid();
    view.range_count = range_get_measurements();
    view.range_result = range_get_result();
    view.range_mm = range_get_filtered();
    view.alarm = range_get_alarm();
    view.alarm_mm = range_get_alarm_mm();
    view.nh3 = nh3_get_state();
    view.ppb = 0;
    nh3_get_ppb(&view.ppb);
    view.warm_left = nh3_get_warmup_left(now);
    view.imu = imu_get_state();
    view.imu_fresh = imu_is_fresh(now);
    view.imu_has = imu_has_data();
    imu_get_accel(raw);

    for(uint8_t i = 0; i < 3; i++){
        view.accel_mg[i] = mpu6050_accel_mg(raw[i], CONFIG_IMU_ACCEL_FS);
    }

    for(uint8_t i = 0; i < 2; i++){
        view.pot_valid[i] = pot_is_valid(i);
        view.pot_raw[i] = pot_get_raw(i);
    }

    view.light = light_get_data();
    view.light_level = light_get_level();
    view.lamp = light_lamp_on();
    view.counter_on = counter_is_running();
    view.count = counter_get_irq();
    view.rate_x100 = counter_get_irq_rate();
    view.loop = loopback_get_state();
    view.dac_has = dac_has_value();
    view.dac = dac_get_value();
    view.found = scanner_found_count();
    view.accel = cfg->accel;
    view.soft_min = cfg->soft_min;
    view.soft_max = cfg->soft_max;
    view.display_errors = display_errors;
    view.imu_errors = imu_get_errors();
    view.spi_errors = spi_get_count(SPI_TIMEOUT) + spi_get_count(SPI_MODE_LOST);
}

static bool view_limit_fault(void)
{
    return view.fault == FAULT_LIMIT_WIRING || view.fault == FAULT_LIMIT_UNEXPECTED || view.fault == FAULT_SOFT_LIMIT;
}

static bool view_range_stale(void)
{
    return view.range_on && view.range_valid && !view.range_fresh;
}

static bool view_imu_active(void)
{
    return view.imu != IMU_STOPPED && view.imu != IMU_FAILED && view.imu != IMU_ID_ERROR;
}

static bool view_imu_stale(void)
{
    return view_imu_active() && view.imu_has && !view.imu_fresh;
}

bool display_view_fault(void)
{
    return view.fault != FAULT_NONE || view.estop || view.limit_min || view.limit_max || view_range_stale() || view.nh3 == NH3_STALE || view_imu_stale() || view.imu == IMU_FAILED || view.imu == IMU_ID_ERROR;
}

static void view_fault_row(void)
{
    static const char names[][17] PROGMEM = {
        "", "ESTOP", "LIMIT WIRING", "HOME NOT FOUND", "SWITCH STUCK", "HOME WRONG LIMIT", "MOTION STOPPED", "CONFIG", "LIMIT HIT", "SOFT LIMIT"
    };
    uint8_t stale = 0;

    row_begin();

    if(view.fault == FAULT_ESTOP || view.estop){
        row_P(PSTR("! ESTOP"));
    } else if(view_limit_fault()){
        row_P(PSTR("! "));
        row_P(names[view.fault]);
    } else if(view.limit_min || view.limit_max){
        row_P(view.limit_min ? PSTR("! LIMIT MIN") : PSTR("! LIMIT MAX"));
    } else if(view.fault != FAULT_NONE && view.fault <= FAULT_SOFT_LIMIT){
        row_P(PSTR("! "));
        row_P(names[view.fault]);
    } else {
        stale = (uint8_t)((view_range_stale() ? 1 : 0) + (view.nh3 == NH3_STALE ? 1 : 0) + (view_imu_stale() ? 1 : 0));

        if(stale == 1){
            row_P(PSTR("! SENSOR STALE "));
        } else if(stale > 1){
            row_P(PSTR("! STALE"));
        }

        if(stale != 0){
            if(view_range_stale()){
                row_P(stale == 1 ? PSTR("DIST") : PSTR(" DIST"));
            }
            if(view.nh3 == NH3_STALE){
                row_P(stale == 1 ? PSTR("NH3") : PSTR(" NH3"));
            }
            if(view_imu_stale()){
                row_P(stale == 1 ? PSTR("IMU") : PSTR(" IMU"));
            }
        } else if(view.imu == IMU_FAILED || view.imu == IMU_ID_ERROR){
            row_P(PSTR("! IMU FAILED"));
        } else {
            row_P(PSTR("OK"));
        }
    }

    gfx_row(7, row, display_view_fault() ? GFX_INVERT : 0);
}

static void view_header(void)
{
    row_begin();

    if(view.page < CONFIG_DISPLAY_PAGES){
        row_P((const char *)pgm_read_word(&page_names[view.page]));
    }

    row_pad(8);
    row_char('P');
    row_uint(view.page + 1UL, 1);
    row_char('/');
    row_uint(CONFIG_DISPLAY_PAGES, 1);
    row_pad(14);
    row_uint(view.now / 1000, 6);
    row_char('s');
    gfx_row(0, row, GFX_INVERT);
}

static void view_motion(uint8_t line)
{
    row_begin();

    if(line == 1){
        row_P(PSTR("ST "));
        row_motor_state(view.motor);
        row_pad(13);
        row_P(view.dir == MOTOR_DIR_MAX ? PSTR("DIR MAX") : PSTR("DIR MIN"));
    } else if(line == 2){
        row_P(PSTR("POS "));
        row_int(view.position, 11);
        row_P(PSTR(" st"));
    } else if(line == 3){
        row_P(PSTR("HZ "));
        row_uint(view.hz, 6);
        row_P(PSTR(" SET "));
        row_uint(view.speed_hz, 5);
    } else if(line == 4){
        row_P(PSTR("TGT "));

        if(view.positioning){
            row_int(view.target, 10);
        } else {
            row_dash(10);
        }
        row_P(view.homed ? PSTR(" HOME Y") : PSTR(" HOME N"));
    } else if(line == 5){
        row_P(PSTR("LIMIT MIN "));
        row_char(view.limit_min ? '1' : '0');
        row_P(PSTR(" MAX "));
        row_char(view.limit_max ? '1' : '0');
    } else {
        uint32_t fill = 0;

        if(view.max_hz != 0){
            fill = view.hz * 126UL / view.max_hz;
        }
        gfx_bar(6, 0, SSD1306_WIDTH, (uint8_t)(fill > 126 ? 126 : fill), GFX_UNDERLINE);
        return;
    }

    gfx_row(line, row, 0);
}

static void view_nh3(void)
{
    row_P(PSTR("NH3  "));

    if(view.nh3 == NH3_VALID){
        row_uint(view.ppb / 1000, 3);
        row_char('.');
        row_number(view.ppb % 1000, false, 3, 3);
        row_P(PSTR(" ppm"));
    } else if(view.nh3 == NH3_WARMUP){
        row_P(PSTR("WARMUP"));

        if(view.warm_left != 0){
            row_char(' ');
            row_uint((view.warm_left + 999) / 1000, 1);
            row_P(PSTR(" s"));
        }
    } else if(view.nh3 == NH3_STALE){
        row_P(PSTR("STALE"));
    } else if(view.nh3 == NH3_NO_DATA){
        row_P(PSTR("NO DATA"));
    } else {
        row_P(PSTR("OFF"));
    }
}

static void view_imu_state(void)
{
    row_P(PSTR("IMU  "));

    if(view.imu == IMU_STOPPED){
        row_P(PSTR("OFF"));
    } else if(view.imu == IMU_FAILED){
        row_P(PSTR("FAILED"));
    } else if(view.imu == IMU_ID_ERROR){
        row_P(PSTR("ID ERROR"));
    } else if(view.imu == IMU_BACKOFF){
        row_P(view.imu_has && !view.imu_fresh ? PSTR("STALE RETRY") : PSTR("RETRY"));
    } else if(view.imu_fresh){
        row_P(PSTR("OK"));
        row_pad(13);
        row_P(PSTR("X Y Z mg"));
    } else if(view.imu_has){
        row_P(PSTR("STALE"));
    } else {
        row_P(PSTR("INIT"));
    }
}

static void view_sensor(uint8_t line)
{
    row_begin();

    if(line == 1){
        row_P(PSTR("DIST "));

        if(!view.range_on){
            row_P(PSTR("OFF"));
        } else if(view.range_fresh){
            row_uint(view.range_mm, 5);
            row_P(PSTR(" mm"));
        } else if(view.range_valid){
            row_P(PSTR("STALE "));
            row_range_result(view.range_result);
        } else if(view.range_count == 0){
            row_P(PSTR("WAIT"));
        } else {
            row_range_result(view.range_result);
        }
    } else if(line == 2){
        view_nh3();
    } else if(line == 3){
        view_imu_state();
    } else if(line == 4){
        for(uint8_t i = 0; i < 3; i++){
            if(view_imu_active() && view.imu_fresh){
                row_int(view.accel_mg[i], 7);
            } else {
                row_dash(7);
            }
        }
    } else if(line == 5){
        row_P(PSTR("AD"));

        for(uint8_t i = 0; i < 2; i++){
            if(view.pot_valid[i]){
                row_uint(view.pot_raw[i], 5);
            } else {
                row_dash(5);
            }
        }

        row_P(PSTR(" L"));

        if(view.light == LIGHT_OK){
            row_uint(view.light_level, 4);
            row_P(view.lamp ? PSTR(" ON") : PSTR(" --"));
        } else {
            row_dash(4);
            row_P(PSTR(" --"));
        }
    } else {
        row_P(PSTR("ALARM "));

        if(view.alarm == RANGE_ALARM_OFF){
            row_P(PSTR("OFF"));
        } else {
            row_uint(view.alarm_mm, 4);
            row_P(PSTR(" mm "));

            if(view.alarm == RANGE_ALARM_ACTIVE){
                row_P(PSTR("NEAR"));
            } else if(view.alarm == RANGE_ALARM_CLEAR){
                row_P(PSTR("CLEAR"));
            } else {
                row_P(PSTR("NO DATA"));
            }
        }
        gfx_row(6, row, GFX_UNDERLINE);
        return;
    }

    gfx_row(line, row, 0);
}

static void view_system(uint8_t line)
{
    static const char loops[][5] PROGMEM = {
        "IDLE", "RUN", "PASS", "FAIL", "ERR", "STOP"
    };
    static const char links[][7] PROGMEM = {
        "OFF", "NODATA", "WARMUP", "VALID", "STALE"
    };

    row_begin();

    if(line == 1){
        row_P(view.counter_on ? PSTR("CNT ") : PSTR("CNT OFF "));
        row_uint(view.count, view.counter_on ? 8 : 1);

        if(view.counter_on){
            row_uint(view.rate_x100 / 100, 4);
            row_char('.');
            row_number(view.rate_x100 % 100, false, 2, 2);
            row_P(PSTR("/s"));
        }
    } else if(line == 2){
        row_P(PSTR("ZE03 "));

        if(view.nh3 <= NH3_STALE){
            row_P(links[view.nh3]);
        }
        row_pad(13);
        row_P(PSTR("SPI "));

        if(view.loop <= LOOP_STOPPED){
            row_P(loops[view.loop]);
        }
    } else if(line == 3){
        row_P(PSTR("DAC "));

        if(view.dac_has){
            row_uint(view.dac, 4);
        } else {
            row_dash(4);
        }
        row_P(PSTR("  I2C DEV "));
        row_uint(view.found, 3);
    } else if(line == 4){
        row_P(PSTR("SPD "));
        row_uint(view.speed_hz, 5);
        row_P(PSTR(" ACC "));
        row_uint(view.accel, 5);
    } else if(line == 5){
        row_P(PSTR("SOFT"));
        row_int(view.soft_min, 7);
        row_P(PSTR(".."));
        row_int(view.soft_max, 1);
    } else {
        row_P(PSTR("ERR D"));
        row_capped(view.display_errors);
        row_P(PSTR(" I"));
        row_capped(view.imu_errors);
        row_P(PSTR(" S"));
        row_capped(view.spi_errors);
        gfx_row(6, row, GFX_UNDERLINE);
        return;
    }

    gfx_row(line, row, 0);
}

void display_view_render(uint8_t line)
{
    if(line == 0){
        view_header();
    } else if(line == 7){
        view_fault_row();
    } else if(view.page == 0){
        view_motion(line);
    } else if(view.page == 1){
        view_sensor(line);
    } else {
        view_system(line);
    }
}

void display_view_send_page_name(uint8_t page)
{
    if(page < CONFIG_DISPLAY_PAGES){
        usart0_send_string_P((const char *)pgm_read_word(&page_names[page]));
    }
}

const char *display_view_row_text(void)
{
    return row;
}
