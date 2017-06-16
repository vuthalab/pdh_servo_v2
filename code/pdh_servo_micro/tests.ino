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
int dac_scan_range_default = 100;  // scan range
int dac_scan_step_default = 1;

int dac_center = dac_center_default; // center value around which to scan
int dac_word = dac_center;  // actual dac value written
int dac_scan_range = dac_scan_range_default;  // scan range
int dac_scan_step = dac_scan_step_default;
int dac_scan_value = -dac_scan_range;

int transmission_threshold = 35;  // counts
int transmission_threshold_max = 100;  // counts
int trans_global = 0;


// globals
bool ds1_state;
bool ds2_state;
bool ds3_state;

int lock_state;
// lock state can be in any of the following
#define LOCK_NOT_ATTEMPTING 0x00
#define LOCK_NOT_ACQUIRED   0x01
#define LOCK_ACQUIRED       0x02


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

    if(ds1_state==HIGH) {
        WiFi.on();
        Particle.connect();
    }
    else {
        WiFi.off();
        Particle.disconnect();

    }
    lock_state = LOCK_NOT_ATTEMPTING;
}

void loop() {
    bool ds1_state_new = digitalRead(DS1);
    // Check whether DS1 switch has been flipped.
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

    // test 1: switch off input by sending high here
    // digitalWrite(S1_input, HIGH);

    // test 2: switch off input by sending high here
    // digitalWrite(S3_output_enable, HIGH);

    // test3: current integrator
    // digitalWrite(S2_curr_int, HIGH);

    // test4: piezo input
    // digitalWrite(S4_piezo_enable, HIGH);

    // test5: piezo integrator
    // digitalWrite(S5_piezo_int, HIGH);


}
