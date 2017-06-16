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

uint16_t dac_word = 0x0000;

void setup() {
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
    digitalWrite(S1_input, LOW);
    digitalWrite(S2_curr_int, LOW);
    digitalWrite(S3_output_enable, LOW);
    digitalWrite(S4_piezo_enable, LOW);
    digitalWrite(S5_piezo_int, LOW);
    digitalWrite(S6_piezo_offset, LOW);

    // Keep this high to test out the piezo channel
    digitalWrite(S6_piezo_offset, HIGH);

    SPI1.setBitOrder(MSBFIRST);
    SPI1.setClockSpeed(15, MHZ);
    SPI1.begin();

    Serial.begin(9600);
}

void update_dac(uint16_t dac_word) {
    uint8_t msbyte = dac_word >> 8;
    uint8_t lsbyte = dac_word && 0xFF;

    digitalWrite(LDAC_, LOW);
    digitalWrite(D5, HIGH);
    digitalWrite(D5, LOW);
    SPI1.transfer(msbyte);
    SPI1.transfer(lsbyte);
    digitalWrite(D5, HIGH);
}

void loop() {
    update_dac(dac_word);


    if(digitalRead(DS3)) {
        dac_word += 0x0001;
        int pz_mon = analogRead(pz_out_buffer);
        delay(1);
        Serial.printf("%d %d\r\n", dac_word, pz_mon);
    }

    if(digitalRead(DS1)) {
        digitalWrite(S5_piezo_int, HIGH);
    }
    else {
        digitalWrite(S5_piezo_int, LOW);
    }
    if(digitalRead(DS2)) {
        digitalWrite(S2_curr_int, HIGH);
    }
    else {
        digitalWrite(S2_curr_int, LOW);
    }




}
