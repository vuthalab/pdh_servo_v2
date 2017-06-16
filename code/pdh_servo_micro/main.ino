// Code for the laser servo controller

// PIN configuration
// PIN# PIN_name        Connection          Type
// 5    WKUP            DS3                 Digital In
// 6    DAC             DS2                 Digital In
// 7    A5              S3_output_enable    Digital Out
// 8    A4              S2_curr_int         Digital Out
// 9    A3              S1_input            Digital Out
// 10   A2              S4_piezo_enable     Digital Out
// 11   A1              pz_out_buffer       Analog In
// 12   A0              transmission        Analog In
// 13   D0              S5_piezo_int        Digital Out
// 14   D1              S6_piezo_offset     Ditigal Out
// -----------------------------------------------------
// 15   D2              MOSI_SPI            Digital Out
// 17   D4              SCK_SPI             Digital Out
// 18   D5              SS_SPI              Digital Out
// 19   D6              LDAC_               Digital Out
// 20   D7              DS1                 Digital In

// Uses of toggle switches
// DS1 switches wifi on or off
// DS2 enables locking
// DS3 enables piezo scanning

// Semi-automatic mode ensures that we run setup() before attempting to
// connect to the cloud.
STARTUP(WiFi.selectAntenna(ANT_EXTERNAL));
SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

int DS3 = WKP;
int DS2 = DAC;
int S3_output_enable = A5;
int S2_curr_int = A4;
int S1_input = A3;
int S4_piezo_enable = A2;
int pz_out_buffer = A1;
int transmission = A0;
int S5_piezo_int = D0;
int S6_piezo_offset = D1;
int DS1 = D7;
int LDAC_ = D6;
int SS_SPI = D5;

int dac_center_default = 32768;
int dac_scan_range_default = 30000;  // scan range
int dac_scan_step_default = 100;

int dac_center = 32768; // center value around which to scan
int dac_word = dac_center;  // actual dac value written
int dac_scan_range = dac_scan_range_default;  // scan range
int dac_scan_step = dac_scan_step_default;
int dac_scan_value = -dac_scan_range;

int transmission_threshold = 30;  // counts
int transmission_threshold_max = 60;  // counts
int trans_global = 0;


// globals
bool ds1_state;
bool ds2_state;
bool ds3_state;
int lock_state;

#define LOCK_NOT_ATTEMPTING 0x00
#define LOCK_NOT_ACQUIRED   0x01
#define LOCK_ACQUIRED       0x02


void setup() {
    // Read the analog pins once to set them to input mode
    analogRead(transmission);
    analogRead(pz_out_buffer);

    // Set the manual toggle switches to input mode
    pinMode(DS1, INPUT);
    pinMode(DS2, INPUT);
    pinMode(DS3, INPUT);

    // set the analog switch lines to digital output
    pinMode(S1_input, OUTPUT);
    pinMode(S2_curr_int, OUTPUT);
    pinMode(S3_output_enable, OUTPUT);

    pinMode(S4_piezo_enable, OUTPUT);
    pinMode(S5_piezo_int, OUTPUT);
    pinMode(S6_piezo_offset, OUTPUT);

    // Set the DAC latch (LDAC_) and chip select (SS_SPI) pins to ditigal output
    pinMode(LDAC_, OUTPUT);
    pinMode(SS_SPI, OUTPUT);
    digitalWrite(LDAC_, HIGH); // The serial register is not latched with LDAC_ is HIGH
    digitalWrite(SS_SPI, HIGH);  // HIGH disables the DAC, it ignores the clock
                                 // and data lines

    // Start with all the analog switches closed
    digitalWrite(S1_input, LOW);
    digitalWrite(S2_curr_int, LOW);
    digitalWrite(S3_output_enable, LOW);
    digitalWrite(S4_piezo_enable, LOW);
    digitalWrite(S5_piezo_int, LOW);
    digitalWrite(S6_piezo_offset, LOW);

    // read positions of switches
    ds1_state = digitalRead(DS1);
    ds2_state = digitalRead(DS2);
    ds3_state = digitalRead(DS3);


    SPI1.setBitOrder(MSBFIRST);
    SPI1.setClockSpeed(2, MHZ);
    SPI1.begin();

    // Set the DAC output to midscale
    update_dac(dac_word);

    // Register the dac_word variable so that it can be accessed from the cloud
    // Particle.variable("dac_word", dac_word);
    Particle.variable("trans_global", trans_global);
    Particle.variable("lock_state", lock_state);

    Particle.connect();

    lock_state = LOCK_NOT_ATTEMPTING;
}

void update_dac(uint16_t dac_word_) {
    uint8_t msbyte = dac_word_ >> 8;
    uint8_t lsbyte = dac_word_ & 0xFF;

    //digitalWrite(LDAC_, LOW);
    digitalWrite(SS_SPI, LOW);
    SPI1.transfer(msbyte);
    SPI1.transfer(lsbyte);
    digitalWrite(SS_SPI, HIGH);
    digitalWrite(LDAC_, LOW);
    digitalWrite(LDAC_, HIGH);
}

void loop() {
    // Check whether DS1 switch has been flipped.
    bool ds1_state_new = digitalRead(DS1);
    if(ds1_state_new != ds1_state) {
        ds1_state = ds1_state_new;
        if(ds1_state) {
            Particle.connect();
        }
        else {
            // Particle.disconnect();
        }
    }

    ds3_state = digitalRead(DS3);
    // scan if switch is on and lock has not been acquired
    if(ds3_state == HIGH && lock_state!=LOCK_ACQUIRED) {
        dac_scan_value += dac_scan_step;
        if(dac_scan_value >= dac_scan_range)
            dac_scan_value = -dac_scan_range;
        dac_word = dac_center + dac_scan_value;
    }
    // if scanning has been switched off, then recenter
    else if(ds3_state == LOW) {
        dac_word = dac_center;
    }
    update_dac(dac_word);


    // attempt locking only if ds2 is high
    bool ds2_state = digitalRead(DS2);
    if(ds2_state == HIGH)
        lock_state = LOCK_NOT_ACQUIRED;
    else
        lock_state = LOCK_NOT_ATTEMPTING;

    // check if transmission exceeds threshold
    trans_global = analogRead(transmission);
    bool lock_condition = (trans_global > transmission_threshold)
                          && (trans_global < transmission_threshold_max);
    if(lock_condition && lock_state!=LOCK_NOT_ATTEMPTING)
        lock_state = LOCK_ACQUIRED;

    if(lock_state==LOCK_ACQUIRED) {
        digitalWrite(S2_curr_int, HIGH);
        digitalWrite(S5_piezo_int, HIGH);
    }
    else {
        digitalWrite(S2_curr_int, LOW);
        digitalWrite(S5_piezo_int, LOW);
    }
}
