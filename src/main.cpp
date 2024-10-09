/*
 * Copyright (c) 2024 Isaac Aronson <i@pingas.org>
 * zlib License, see LICENSE file.
 */

#include "bn_core.h"
#include "bn_date.h"
#include "bn_time.h"
#include "bn_keypad.h"
#include "bn_string.h"
#include "bn_bg_palettes.h"
#include "bn_sprite_text_generator.h"

#include "common_info.h"
#include "common_variable_8x16_sprite_font.h"

#define REG_DAT *((volatile uint16_t *)0x080000C4)
#define REG_DIR *((volatile uint16_t *)0x080000C6)
#define REG_CTL *((volatile uint16_t *)0x080000C8)

// We mask 0x60 here and/or add 1 to the LSB to indicate R/W intent
#define MASK_READ(x) (((x)<<1) | 0x61)
#define MASK_WRITE(x) (((x)<<1) | 0x60)

bn::sprite_text_generator text_generator(common::variable_8x16_sprite_font);

#include "hsm.h"

static constexpr bn::string_view week_days[] = {
        "Sunday",
        "Monday",
        "Tuesday",
        "Wednesday",
        "Thursday",
        "Friday",
        "Saturday",
};

using namespace hsm;

class RtcClient {
public:
    explicit RtcClient();

    void Update() {
        sm.ProcessStateTransitions();
        sm.UpdateStates();
    }

private:
    StateMachine sm;
    friend struct ClientStates;
    unsigned short rtcStatus = false;

    /**
     * Will set up the RTC module to read or write from other methods.
     * See the data sheet for the Seiko S-3511 for more details.
     */
    static void commandRTC(int command) {
        // Shift command up to avoid collision with LSB R/W bit
        command <<= 1;
        // Read the 8 bits in MSB->LSB order
        for (int bit = 7; bit >= 0; bit--) {
            // Sample correct bit from given word, avoiding R/W pin
            unsigned short data_bit = (command >> bit) & 0b010;
            // Register value is the third pin high and the second bit set to what to write
            unsigned short reg_value = data_bit | 0b100;

            // Knock on module a few times before commiting write
            for (int i = 0; i < 2; i++)
                REG_DAT = reg_value;

            // Send value plus LSB R/W bit to trigger flush
            REG_DAT = reg_value | 0b001;
        }
    }

    /**
     * Assuming proper signaling to the RTC module prior, will read out 8 bits from the module
     */
    static int readByte() {
        int data = 0; // store data somewhere
        // Read 8 bits
        for (int bit = 0; bit < 8; bit++) {
            // Knock a few times on pin 3
            for (int i = 0; i < 2; i++)
                REG_DAT = 0b100;
            // Raise R/W pin (1) after knocking
            REG_DAT = 0b101;
            // RTC module after seeing R/W pin high will give us a bit on pin 2
            unsigned short reg_value = REG_DAT;
            // Filter pin 2 and shift into place while assembling bit field
            data |= ((reg_value & 0b010) << bit);
        }
        // shift down one as LSB is R/W state
        return data >> 1;
    }

    /**
     * Assuming proper signaling to the RTC module prior, will write out 8 bits to the module
     */
    static void writeByte(int byte) {
        // Shift command up to avoid collision with LSB R/W bit
        byte <<= 1;
        // Write the 8 bits in LSB->MSB order
        for (int bit = 0; bit < 8; bit++) {
            // Sample correct bit from given word, avoiding R/W pin
            unsigned short data_bit = (byte >> bit) & 0b010;
            unsigned short reg_value = data_bit | 0b100;

            // Knock on module a few times before commiting read
            for (int i = 0; i < 2; i++)
                REG_DAT = reg_value;

            // Send value plus LSB R/W bit to trigger flush
            REG_DAT = reg_value | 1;
        }
    }

    /**
     * Handles own signaling, commits full date and time to module
     */
    static void setRTC(int year, int month, int day, int dayOfWeek, int hour, int minute, int second) {
        int i;
        unsigned char dataField[7]{static_cast<unsigned char>(year), static_cast<unsigned char>(month),
                                   static_cast<unsigned char>(day), static_cast<unsigned char>(dayOfWeek),
                                   static_cast<unsigned char>(hour), static_cast<unsigned char>(minute),
                                   static_cast<unsigned char>(second)};
        // Enable control
        REG_CTL = 0b001;
        // Knock data pins 1 and 3
        REG_DAT = 0b001;
        REG_DAT = 0b101;
        // Set dir to all out
        REG_DIR = 0b111;
        // Inform RTC we intend to send a full datetime (write command 2)
        commandRTC(MASK_WRITE(2));
        // Do the writes
        for (i = 0; i < 4; i++) {
            writeByte(toBcd(dataField[i]));
        }
        for (i = 4; i < 7; i++) {
            writeByte(toBcd(dataField[i]));
        }
    }

    /**
     * Handles own signaling, sends factory init signal to module
     */
    static void resetChip() {
        // Wake up
        REG_DAT = 0b001;
        // Raise signal
        REG_DAT = 0b101;
        // Open dir
        REG_DIR = 0b111;
        // Send reset command
        commandRTC(MASK_READ(0));
        // Knock read
        REG_DAT = 0b001;
    }

    /**
     * LUT for fast binary to BCD conversion
     * Credit: Gericom
     */
    static int toBcd(unsigned int value) {
        static const unsigned char sToBcd[100] =
                {
                        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
                        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
                        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,
                        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
                        0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
                        0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
                        0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
                        0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
                        0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
                        0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99
                };

        return sToBcd[value];
    }

};

bn::string<64> &getTimeString(bn::string<64> &text, const bn::optional<bn::time> &time) {
    bn::string<4> hour = bn::to_string<4>(time->hour());
    bn::string<4> minute = bn::to_string<4>(time->minute());
    bn::string<4> second = bn::to_string<4>(time->second());

    if (hour.size() == 1) {
        hour = "0" + hour;
    }

    if (minute.size() == 1) {
        minute = "0" + minute;
    }

    if (second.size() == 1) {
        second = "0" + second;
    }

    text += " ";
    text += hour;
    text += ':';
    text += minute;
    text += ':';
    text += second;
    return text;
}

// Zeller's Congruence algorithm
[[nodiscard]] int calculateDayOfWeekIndex(int year, int month, int day) {
    static constexpr int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    int y = year;

    if (month < 3) {
        y--;
    }

    // 100 and 400 div maybe not necessary...
    return (y + y / 4 - y / 100 + y / 400 + t[month - 1] + day) % 7;
}


bn::string<64> &getDateString(bn::string<64> &text, const bn::optional<bn::date> &date) {
    text += week_days[calculateDayOfWeekIndex(date->year(), date->month(), date->month_day())];
    text += "(";
    text += bn::to_string<1>(date->week_day());
    text += ")";
    text += ' ';
    text += bn::to_string<4>(date->year());
    text += '/';
    text += bn::to_string<4>(date->month());
    text += '/';
    text += bn::to_string<4>(date->month_day());
    return text;
}

struct ClientStates {
    struct BaseState : StateWithOwner<RtcClient> {
    };

    struct WelcomeScene : BaseState {
        bn::vector<bn::sprite_ptr, 128> text_sprites;

        void OnEnter() override {
            text_sprites.clear();

            text_generator.generate(0, -4 * 16, "Luigi's Lucky RTC", text_sprites);
            text_generator.generate(0, -1 * 16, "This program supports hot-swap.", text_sprites);
            text_generator.generate(0, -0 * 16, "Insert your desired hardware!", text_sprites);
            text_generator.generate(0, +4 * 16, "START: query RTC module", text_sprites);
        }

        Transition GetTransition() override {
            if (bn::keypad::start_pressed()) {
                return SiblingTransition<StatusScene>();
            }
            return NoTransition();
        }

        void OnExit() override {
            bn::core::update();
        }

        DEFINE_HSM_STATE(WelcomeScene)
    };

    struct StatusScene : BaseState {
        bn::vector<bn::sprite_ptr, 128> text_sprites;

        void OnEnter() override {
            text_sprites.clear();
            text_generator.generate(0, -4 * 16, "Negotiation with RTC module", text_sprites);

            // We want to init the RTC without resetting it automatically, so skip butano and agbabi methods
            REG_CTL = 0b001; // enable control of remote chip
            // Most get this wrong and send dir first, but real games do the below
            REG_DAT = 0b001; // raise first pin
            REG_DAT = 0b101; // raise third pin while first still high
            // We've banged the bus a little to say hello, now we enable direction and flow that data to the chip
            REG_DIR = 0b111; // raise/commit all three pins
            // Now the RTC is theoretically ready so we tell it to give us its status
            RtcClient::commandRTC(MASK_READ(1));
            // Relax direction 2nd pin to allow for reads
            REG_DIR = 0b101;
            // Read value from RTC assuming it's ready
            int checkValue = RtcClient::readByte();

            // Report on result
            bn::string<64> text;
            bn::string<64> additional;
            bn::string<64> additional2;
            bn::string<64> nextSteps;
            if (checkValue == 0xFF) {
                text = "RTC chip returned only noise.";
                additional = "Bad/misconfigured emu?";
                additional2 = "Cart not plugged?";
                nextSteps = "START: proceed to attempt reset";
            } else if (checkValue == 0x82) {
                text = "RTC chip is in factory state.";
                nextSteps = "START: proceed to initialize";
            } else if (checkValue & 0x80) {
                text = "RTC chip battery is (likely) dead.";
                nextSteps = "START: proceed to attempt init";
            } else if (checkValue & 0x40) {
                text = "RTC chip is in 24 hour mode.";
                nextSteps = "START: proceed to read date & time";
            } else {
                text = "RTC chip is in 12 hour mode.";
                nextSteps = "START: proceed to read date & time";
            }
            Owner().rtcStatus = checkValue;
            text_generator.generate(0, -0 * 16, text, text_sprites);
            text_generator.generate(0, +1 * 16, additional, text_sprites);
            text_generator.generate(0, +2 * 16, additional2, text_sprites);
            text_generator.generate(0, +4 * 16, nextSteps, text_sprites);
        }

        Transition GetTransition() override {
            if (bn::keypad::start_pressed() && (Owner().rtcStatus & 0x80)) {
                return SiblingTransition<ResetScene>();
            } else if (bn::keypad::start_pressed()) {
                return SiblingTransition<WallClockScene>();
            }
            return NoTransition();
        }

        void OnExit() override {
            bn::core::update();
        }

        DEFINE_HSM_STATE(StatusScene)
    };

    struct WallClockScene : BaseState {
        bn::vector<bn::sprite_ptr, 96> text_sprites;
        bn::vector<bn::sprite_ptr, 32> time_sprites;

        void OnEnter() override {
            if (!bn::date::active() || !bn::time::active()) {
                return;
            }
            text_sprites.clear();
            text_generator.generate(0, -4 * 16, "Read Date and Time", text_sprites);
            if (bn::date::active() && bn::time::active()) {
                text_generator.generate(0, 3 * 16, "SELECT: reset (will confirm first)", text_sprites);
                text_generator.generate(0, 4 * 16, "START: edit (saves current time)", text_sprites);
            } else {
                text_generator.generate(0, 4 * 16, "SELECT: proceed to attempt reset", text_sprites);
            }
        }

        void Update() override {
            bn::string<64> text;

            if (bn::optional<bn::date> date = bn::date::current()) {
                text = getDateString(text, date);
            } else {
                return;
            }

            if (bn::optional<bn::time> time = bn::time::current()) {
                text = getTimeString(text, time);
            } else {
                return;
            }

            time_sprites.clear();
            text_generator.generate(0, 0, text, time_sprites);
        }

        Transition GetTransition() override {
            if (!bn::date::active() || !bn::time::active()) {
                return SiblingTransition<StatusScene>();
            } else if (bn::keypad::select_pressed()) {
                return SiblingTransition<ResetScene>();
            } else if (bn::keypad::start_pressed() && bn::date::active() && bn::time::active()) {
                return SiblingTransition<EditScene>();
            }
            return NoTransition();
        }

        void OnExit() override {
            bn::core::update();
        }

        DEFINE_HSM_STATE(WallClockScene)
    };

    struct EditScene : BaseState {
        enum SelectedComponent {
            Year,
            Month,
            Day,
            Hour,
            Minute,
            Second
        };
        SelectedComponent selectedComponent = Year;
        bn::string<32> readLine;
        int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0, dow = 0;
        int dowOffset = 0;
        bn::vector<bn::sprite_ptr, 96> text_sprites;
        bn::vector<bn::sprite_ptr, 32> time_sprites;

        void ReadFromRTC() {
            bn::optional<bn::date> dateOpt = bn::date::current();
            bn::optional<bn::time> timeOpt = bn::time::current();
            if (!dateOpt.has_value() || !timeOpt.has_value()) return;
            bn::date date = dateOpt.value();
            bn::time time = timeOpt.value();
            year = date.year();
            month = date.month();
            day = date.month_day();
            hour = time.hour();
            minute = time.minute();
            second = time.second();
            dow = date.week_day();
            dowOffset = dow - calculateDayOfWeekIndex(year, month, day);
        }

        void OnEnter() override {
            text_sprites.clear();
            time_sprites.clear();
            ReadFromRTC();
            renderLine();
            text_generator.generate(0, -4 * 16, "RTC Edit", text_sprites);
            text_generator.generate(0, 0 * 16, readLine, time_sprites);
            text_generator.generate(0, +3 * 16, "SELECT: return", text_sprites);
            text_generator.generate(0, +4 * 16, "START: save", text_sprites);
        }

        void renderLine() {
            readLine = "";
            if (selectedComponent == Year)
                readLine += "<";
            readLine += bn::to_string<2>(year);
            if (selectedComponent == Year)
                readLine += ">";
            readLine += "/";
            if (selectedComponent == Month)
                readLine += "<";
            readLine += bn::to_string<2>(month);
            if (selectedComponent == Month)
                readLine += ">";
            readLine += "/";
            if (selectedComponent == Day)
                readLine += "<";
            readLine += bn::to_string<2>(day);
            if (selectedComponent == Day)
                readLine += ">";
            readLine += " ";
            if (selectedComponent == Hour)
                readLine += "<";
            readLine += bn::to_string<2>(hour);
            if (selectedComponent == Hour)
                readLine += ">";
            readLine += ":";
            if (selectedComponent == Minute)
                readLine += "<";
            readLine += bn::to_string<2>(minute);
            if (selectedComponent == Minute)
                readLine += ">";
            readLine += ":";
            if (selectedComponent == Second)
                readLine += "<";
            readLine += bn::to_string<2>(second);
            if (selectedComponent == Second)
                readLine += ">";
        }

        void Update() override {
            bool dirty = false;
            // Select component
            if (bn::keypad::left_pressed()) {
                dirty = true;
                if (selectedComponent != Year)
                    selectedComponent = static_cast<SelectedComponent>(selectedComponent - 1);
            } else if (bn::keypad::right_pressed()) {
                dirty = true;
                if (selectedComponent != Second)
                    selectedComponent = static_cast<SelectedComponent>(selectedComponent + 1);
            }
            // Mutate component
            if (bn::keypad::up_pressed()) {
                dirty = true;
                switch (selectedComponent) {
                    case Year:
                        year++;
                        year %= 99;
                        break;
                    case Month:
                        month++;
                        month %= 99;
                        break;
                    case Day:
                        day++;
                        day %= 99;
                        break;
                    case Hour:
                        hour++;
                        hour %= 99;
                        break;
                    case Minute:
                        minute++;
                        minute %= 99;
                        break;
                    case Second:
                        second++;
                        second %= 99;
                        break;
                    default:
                        break;
                }
            } else if (bn::keypad::down_pressed()) {
                dirty = true;
                switch (selectedComponent) {
                    case Year:
                        year--;
                        if (year < 0) year = 0;
                        break;
                    case Month:
                        month--;
                        if (month < 0) month = 0;
                        break;
                    case Day:
                        day--;
                        if (day < 0) day = 0;
                        break;
                    case Hour:
                        hour--;
                        if (hour < 0) hour = 0;
                        break;
                    case Minute:
                        minute--;
                        if (minute < 0) minute = 0;
                        break;
                    case Second:
                        second--;
                        if (second < 0) second = 0;
                        break;
                    default:
                        break;
                }
            }
            if (dirty) {
                renderLine();
                time_sprites.clear();
                text_generator.generate(0, 0 * 16, readLine, time_sprites);
            }
        }

        Transition GetTransition() override {
            if (bn::keypad::start_pressed()) {
                SaveTime();
                return SiblingTransition<WallClockScene>();
            } else if (bn::keypad::select_pressed()) {
                return SiblingTransition<WallClockScene>();
            }
            return NoTransition();
        }

        void OnExit() override {
            bn::core::update();
        }

        void SaveTime() const {
            RtcClient::setRTC(year, month, day, (calculateDayOfWeekIndex(year, month, day) + dowOffset + 7) % 7, hour,
                              minute, second);
        }

        DEFINE_HSM_STATE(EditScene)
    };

    struct ResetScene : BaseState {
        bn::vector<bn::sprite_ptr, 128> text_sprites;
        bn::string<35> description;

        void OnEnter() override {
            description = Owner().rtcStatus & 0x80 ? "RTC power flag raised: init?" : "RTC ready: confirm reset?";
            text_sprites.clear();
            text_generator.generate(0, -4 * 16, Owner().rtcStatus == 0x82 ? "RTC Initialize" : "RTC Reset",
                                    text_sprites);
            text_generator.generate(0, +0 * 16, description, text_sprites);
            text_generator.generate(0, +3 * 16, Owner().rtcStatus == 0x82 ? "SELECT: send init" : "SELECT: send reset",
                                    text_sprites);
            text_generator.generate(0, +4 * 16, "START: force read RTC", text_sprites);
        }

        Transition GetTransition() override {
            if (bn::keypad::select_pressed()) {
                RtcClient::resetChip();
                return SiblingTransition<StatusScene>();
            } else if (bn::keypad::start_pressed()) {
                return SiblingTransition<WallClockScene>();
            }
            return NoTransition();
        }

        void OnExit() override {
            bn::core::update();
        }

        DEFINE_HSM_STATE(ResetScene)
    };
};

RtcClient::RtcClient() {
    sm.Initialize<ClientStates::WelcomeScene>(this);
}

int main() {
    bn::core::init();

    RtcClient rtcClient;
    bn::bg_palettes::set_transparent_color(bn::color(16, 20, 16));
    text_generator.set_center_alignment();

    while (!(bn::keypad::start_held() && bn::keypad::select_held() && bn::keypad::a_held() && bn::keypad::b_held())) {
        rtcClient.Update();
        bn::core::update();
    }
    return 0;
}
