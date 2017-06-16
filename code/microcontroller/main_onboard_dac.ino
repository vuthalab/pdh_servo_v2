// Code for the laser servo controller
// This version is meant for a modified servo controller, where the external
// AD5541A DAC is ditched in favor of the onboard 12 bit DAC.

// The following changes were made on the board:
// - The AD5541A chip was removed
// - The output of the DAC pin on the photon was heavily low pass filtered,
//   using a 15kOhm, and 1uF||0.1uF RC filter to get a ~20Hz cutoff frequency, and
//   then jumpered to the output test point of the original DAC chip.
// - The external flip switch S2 was originally connected to the DAC pin on the
//   photon. This connection was removed by de-soldering the output pin of S2,
//   and bending it out of the board. Since the digital output pins that were
//   controlling the original DAC were freed up, the output of S2 was connected
//   to D6. Previously, D6 was connected to the LDAC_ latch pin of the external
//   DAC.

// After all these modifications, here's the update pin configuration:
// PIN configuration
// PIN# PIN_name        Connection          Type
// 5    WKUP            DS3                 Digital In
// 7    A5              S3_output_enable    Digital Out
// 8    A4              S2_curr_int         Digital Out
// 9    A3              S1_input            Digital Out
// 10   A2              S4_piezo_enable     Digital Out
// 11   A1              pz_out_buffer       Analog In
// 12   A0              transmission        Analog In
// 13   D0              S5_piezo_int        Digital Out
// 14   D1              S6_piezo_offset     Ditigal Out
// -----------------------------------------------------
// 19   D6              DS2                 Digital In
// 20   D7              DS1                 Digital In

// Uses of toggle switches
// DS1 switches wifi on or off
// DS2 enables smart locking. If transmission through the cavity is detected,
// then switches to the integrators are opened.
// DS3 enables piezo scanning


STARTUP(WiFi.selectAntenna(ANT_EXTERNAL));

// System code is run in a separate thread. This makes the loop() code run
// much faster.
SYSTEM_THREAD(ENABLED);
// Semi-automatic mode ensures that we run setup() before attempting to
// connect to the cloud.
SYSTEM_MODE(SEMI_AUTOMATIC);

int DS3 = WKP;
int DS2 = D6;
int S3_output_enable = A5;
int S2_curr_int = A4;
int S1_input = A3;
int S4_piezo_enable = A2;
int pz_out_buffer = A1;
int transmission = A0;
int S5_piezo_int = D0;
int S6_piezo_offset = D1;
int DS1 = D7;
int SS_SPI = D5;

int dac_center_default = 2048;
int dac_scan_range_default = 150;  // scan range
int dac_scan_step_default = 1;

int dac_center = dac_center_default; // center value around which to scan
int dac_word = dac_center;  // actual dac value written
int dac_scan_range = dac_scan_range_default;  // scan range
int dac_scan_step = dac_scan_step_default;
int dac_scan_value = -dac_scan_range;

int transmission_threshold = 15;  // counts
int transmission_threshold_max = 2000;  // counts
int trans_global = 0;

int time_when_lock_acquired = 0;

// globals
bool ds1_state;
bool ds2_state;
bool ds3_state;

int lock_state;
int previous_lock_state;

bool force_unlock;

// lock state can be in any of the following
#define LOCK_NOT_ATTEMPTING 0x00
#define LOCK_NOT_ACQUIRED   0x01
#define LOCK_ACQUIRED       0x02


int setS1(String state) {
    if(state == "HIGH") {
        digitalWrite(S1_input, HIGH);
        return 1;
    }
    else if (state == "LOW") {
        digitalWrite(S1_input, LOW);
        return 0;
    }

}

int setS3(String state) {
    if(state == "HIGH") {
        digitalWrite(S3_output_enable, HIGH);
        return 1;
    }
    else if (state == "LOW") {
        digitalWrite(S3_output_enable, LOW);
        return 0;
    }

}

int setS4(String state) {
    if(state == "HIGH") {
        digitalWrite(S4_piezo_enable, HIGH);
        return 1;
    }
    else if (state == "LOW") {
        digitalWrite(S4_piezo_enable, LOW);
        return 0;
    }

}

int setS6(String state) {
    if(state == "HIGH") {
        digitalWrite(S6_piezo_offset, HIGH);
        return 1;
    }
    else if (state == "LOW") {
        digitalWrite(S6_piezo_offset, LOW);
        return 0;
    }

}

int setS5(String state) {
    if(state == "HIGH") {
        digitalWrite(S5_piezo_int, HIGH);
        return 1;
    }
    else if (state == "LOW") {
        digitalWrite(S5_piezo_int, LOW);
        return 0;
    }

}

void setup() {
    // Read the analog pins once to set them to input mode
    analogRead(transmission);
    analogRead(pz_out_buffer);

    // Set the onboard 12 bit DAC to output
    pinMode(DAC1, OUTPUT);
    analogWrite(DAC1, 2048);  // set DAC to mid-scale

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


    // register setS1 function to the cloud
    Particle.function("setS1", setS1);
    Particle.function("setS3", setS3);
    Particle.function("setS4", setS4);
    Particle.function("setS5", setS5);
    Particle.function("setS6", setS6);
    Particle.variable("dac_word", &dac_word, INT);

    if(ds1_state==HIGH) {
        WiFi.on();
        Particle.connect();
    }
    else {
        WiFi.off();
        Particle.disconnect();

    }
    lock_state = LOCK_NOT_ATTEMPTING;
    previous_lock_state = LOCK_NOT_ATTEMPTING;
    force_unlock = false;
}

void loop() {
    previous_lock_state = lock_state;
    // Check whether DS1 switch has been flipped.
    bool ds1_state_new = digitalRead(DS1);
    if(ds1_state_new != ds1_state) {
        ds1_state = ds1_state_new;
        if(ds1_state) {
            WiFi.on();
            Particle.connect();
        }
        else {
            Particle.disconnect();
            WiFi.off();
        }
    }

    ds3_state = digitalRead(DS3);
    // scan if switch is on and lock has not been acquired
    if(ds3_state == HIGH && lock_state!=LOCK_ACQUIRED) {
        dac_scan_value += dac_scan_step;
        if(dac_scan_value >= dac_scan_range) {
            // this is the end of one scan cycle
            if(lock_state==LOCK_NOT_ACQUIRED) {
                // we are still searching for the transmission and did not
                // find it in this range, hence widen the scan range
                dac_scan_range *=2;
                if(dac_scan_value > 1000) {
                    // do not exceed half range
                    dac_scan_range = 1000;
                }
            }
            dac_scan_value = -dac_scan_range;
        }
        else
        dac_word = dac_center + dac_scan_value;
        delayMicroseconds(500);
    }
    // if scanning has been switched off, then recenter
    else if(ds3_state == LOW) {
        dac_word = dac_center;
        dac_scan_range = dac_scan_range_default;
    }

    // update DAC only when scanning. Since writing to DAC introduces
    // glitches in the output voltage, we avoid it when the laser is locked
    if(lock_state != LOCK_ACQUIRED)
        analogWrite(DAC1, dac_word);

    // check if transmission exceeds threshold
    // set state to LOCK_ACQUIRED if it does and if we want to lock
    trans_global = analogRead(transmission);
    bool lock_condition = (trans_global > transmission_threshold)
                          && (trans_global < transmission_threshold_max);
    if(lock_condition == false && lock_state==LOCK_ACQUIRED) {
        // we just lost lock in this round
        // start with a small scan range
        dac_scan_range = 8;
    }
    // attempt locking only if ds2 is high
    // start with assumption that lock has not been acquired
    bool ds2_state = digitalRead(DS2);
    if(ds2_state == HIGH) {
        lock_state = LOCK_NOT_ACQUIRED;
    }
    else {
        lock_state = LOCK_NOT_ATTEMPTING;
        // reset the scan range if we don't want to attempt locking
        dac_scan_range = dac_scan_range_default;
    }

    if(lock_condition && lock_state!=LOCK_NOT_ATTEMPTING) {

        if(force_unlock) {
            lock_state = LOCK_NOT_ACQUIRED;
            analogWrite(DAC1, dac_word+10);
        }
        else {
            lock_state = LOCK_ACQUIRED;

            // // if measuring cavity ring down, start counting time
            // if (previous_lock_state != LOCK_ACQUIRED) {
            //     // we just acquired lock, start counting time
            //     time_when_lock_acquired = Time.now();
            // }
            // if(Time.now() - time_when_lock_acquired > 1) {
            //     // we have a lock for 2 seconds
            //     // force unlock
            //     force_unlock = true;
            // }

        }



    }

    // if(Time.now() - time_when_lock_acquired > 2) {
    //     force_unlock = false;
    // }

    if(lock_state==LOCK_ACQUIRED) {
        digitalWrite(S2_curr_int, HIGH);
        digitalWrite(S5_piezo_int, HIGH);
        dac_scan_range = dac_scan_range_default;
    }
    else {
        digitalWrite(S2_curr_int, LOW);
        digitalWrite(S5_piezo_int, LOW);
    }
}
