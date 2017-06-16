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

bool s2_state = LOW;
bool s5_state = LOW;

uint16_t dac_word = 0x0000;

void setup() {
    analogRead(transmission);
    analogRead(pz_out_buffer);

    pinMode(DS1, INPUT);
    pinMode(DS2, INPUT);
    pinMode(DS3, INPUT);

    pinMode(S1_input, OUTPUT);
    pinMode(S2_curr_int, OUTPUT);
    pinMode(S3_output_enable, OUTPUT);

    pinMode(S4_piezo_enable, OUTPUT);
    pinMode(S5_piezo_int, OUTPUT);
    pinMode(S6_piezo_offset, OUTPUT);

    pinMode(LDAC_, OUTPUT);
    digitalWrite(LDAC_, HIGH); // The serial register is not latched with LDAC_ is HIGH
    pinMode(SS_SPI, OUTPUT);
    digitalWrite(SS_SPI, HIGH);  // HIGH disables the DAC

    digitalWrite(S1_input, LOW);
    digitalWrite(S2_curr_int, s2_state);
    digitalWrite(S3_output_enable, LOW);
    digitalWrite(S4_piezo_enable, LOW);
    digitalWrite(S5_piezo_int, s5_state);
    digitalWrite(S6_piezo_offset, LOW);

    digitalWrite(S6_piezo_offset, LOW);



    SPI1.setBitOrder(MSBFIRST);
    SPI1.setClockSpeed(4, MHZ);
    SPI1.begin();

    Serial.begin(9600);
    update_dac(0x8000);

    Particle.connect();
}

void update_dac(uint16_t dac_word) {
    uint8_t msbyte = dac_word >> 8;
    uint8_t lsbyte = dac_word && 0xFF;

    //digitalWrite(LDAC_, LOW);
    digitalWrite(SS_SPI, LOW);
    SPI1.transfer(msbyte);
    SPI1.transfer(lsbyte);
    digitalWrite(SS_SPI, HIGH);
    digitalWrite(LDAC_, LOW);
    digitalWrite(LDAC_, HIGH);
}

void loop() {
    // if(digitalRead(DS3)) {
    if(true) {
        dac_word += 0x0100;
        //update_dac(dac_word);
        update_dac(dac_word);
        // int pz_mon = analogRead(pz_out_buffer);
        // Serial.printf("%d %d\r\n", dac_word, pz_mon);
    }

    bool ds1_state = digitalRead(DS1);
    if(s5_state != ds1_state) {
        s5_state = ds1_state;
        digitalWrite(S5_piezo_int, s5_state);
    }

    bool ds2_state = digitalRead(DS2);
    if(s2_state != ds2_state) {
        s2_state = ds2_state;
        digitalWrite(S2_curr_int, s2_state);
    }

}
