
const int LASER_COUNT = 10;
const int LASER_PINS[LASER_COUNT]  = {13, A0, A1, A2, A3, A4, A5, 12, 11, 11};
const int SENSOR_PINS[LASER_COUNT] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 10};
const int NO_SENSOR = -1;

int sensor_for_laser[LASER_COUNT];
bool sensor_state_for_laser[LASER_COUNT];
bool isSystemActive = false;
bool testing_mode = false;
String laserString;


void debugBlink(int count) {
    for (int i = 0; i < count; ++i) {
        digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
        delay(100);                        // wait for a second
        digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
        delay(100);                        // wait for a second
    }
}

inline void setup_vars() {
    for (int i = 0; i < LASER_COUNT; ++i) {
        sensor_for_laser[i] = NO_SENSOR;
        sensor_state_for_laser[i] = false;
    }
}

inline void setup_serial() {
    Serial.begin(9600);
    while (!Serial) {
        ; // wait for serial port to connect. Needed for native USB port only
    }
}

inline void setup_laser_pins() {
    for (int i = 0; i < LASER_COUNT; ++i) {
        pinMode(LASER_PINS[i], OUTPUT);
    }
}

inline void setup_sensor_pins() {
    for (int i = 0; i < LASER_COUNT; ++i) {
        pinMode(SENSOR_PINS[i], INPUT);
    }
}

/// ----------------------------------------

inline bool isSensorLighted(int sensor) {
    return digitalRead(SENSOR_PINS[sensor]) == HIGH;
}

inline void changeLaserState(int laser, bool state) {
    sensor_state_for_laser[laser] = state;
    digitalWrite(LASER_PINS[laser], (state) ? HIGH : LOW);
}

inline void turnLaserOn(int laser) {
    changeLaserState(laser, true);
}

inline void turnLaserOff(int laser) {
    changeLaserState(laser, false);
}

inline void clearState() {
    isSystemActive = false;
    for (int i = 0; i < LASER_COUNT; ++i) {
        turnLaserOff(i);
    }
    debugBlink(3);
}

inline void activateSystem() {
    isSystemActive = true;
}


/// ----------------------------------------
inline void findMapping() {
    int length = laserString.length();
    bool usedSensors[LASER_COUNT];
    for (int i = 0; i < LASER_COUNT; ++i)
        usedSensors[i] = false;
    int notMatched = length;
    while (notMatched != 0) {
        for (int i = 0; i < length; ++i) {
            int laserIndex = laserString[i] - 'A';
            if (sensor_for_laser[laserIndex] != NO_SENSOR)
                continue;
            turnLaserOn(laserIndex);
            for (int sensorIndex = 0; sensorIndex < LASER_COUNT; ++sensorIndex) {
                delay(2);
                if (isSensorLighted(sensorIndex) && !usedSensors[sensorIndex]) {
                    sensor_for_laser[laserIndex] = sensorIndex;
                    Serial.println("Found " + String(laserString[i]) + " : " + String(sensorIndex));
                    usedSensors[sensorIndex] = true;
                    notMatched--;
                    break;
                }
            }
            turnLaserOff(laserIndex);
        }
    }
    for (int j = 0; j < length; ++j)
        turnLaserOn(laserString[j] - 'A');
}

inline void load_info() {
    delay(100);
    int n = Serial.read() - '0' + Serial.read() - '0';
    for (int i = 0; i < n; ++i) {
        laserString += char(Serial.read());
        delay(10);
    }
    Serial.println("Lasers: " + laserString);
}

inline void processCommands() {
    if (!Serial.available())
        return;

    byte cmd = Serial.read();
    if (cmd == '0') {
        clearState();
        Serial.println("System Deactivated");
    }
    else if (cmd == '1') {
        activateSystem();
        Serial.println("System Activated");
    }
    else if (cmd == '2') {
        testing_mode = !testing_mode;
        if (testing_mode)
            Serial.println("SetUp Mode Activated");
        else
            Serial.println("SetUp Mode Deactivated");
    }
    else if (cmd == '3') {
        load_info();
        findMapping();
        Serial.println("Mapping Done!");
    }
    else if ('a' <= cmd && cmd < ('a' + LASER_COUNT))
        turnLaserOff(cmd - 'a');
    else if ('A' <= cmd && cmd < ('A' + LASER_COUNT))
        turnLaserOn(cmd - 'A');
}


// --- REPORTING ---
inline void reportStateChange(int laser, bool new_state) {
    if (testing_mode)
        Serial.println(String(laser) + (new_state ? '+' : '-'));
}

inline void reportLaserCrossing(char laser) {
    Serial.println("!!! Crossing: " + String(laser) + " !!!");
}
// ------------------

inline void lasers_blink(int delay_time, int count) {
    int length = laserString.length();
    String lowerLaserString = laserString;
    lowerLaserString.toLowerCase();
    for (int i = 0; i < count; ++i) {
        for (int j = 0; j < length; ++j)
            turnLaserOn(laserString[j] - 'A');
        delay(delay_time);
        for (int j = 0; j < length; ++j)
            turnLaserOff(lowerLaserString[j] - 'a');
        delay(delay_time);
    }
}

inline void checkGridStatus() {
    if (testing_mode) {
        Serial.print("+");
        for (int i = 0; i < LASER_COUNT; ++i)
            if (isSensorLighted(i))
                Serial.print(i);
        Serial.print("+   -");
        for (int i = 0; i < LASER_COUNT; ++i)
            if (!isSensorLighted(i))
                Serial.print(i);
        Serial.println("-");
        delay(2000);
    }
    int length = laserString.length();
    for (int j = 0; j < length; ++j) {
        int i = laserString[j] - 'A';

        bool sensorIsLighted = isSensorLighted(sensor_for_laser[i]);
        if (sensorIsLighted != sensor_state_for_laser[i])
            sensor_state_for_laser[i] = sensorIsLighted;

        if (isSystemActive && !sensorIsLighted) {
            reportLaserCrossing(laserString[j]);
            lasers_blink(250, 5);
            clearState();
            Serial.println("System Deactivated");
            return;
        }
    }
}

// MAIN ARDUINO CODE

void setup() {
    init();
    setup_serial();
    setup_laser_pins();
    setup_sensor_pins();
    setup_vars();
    clearState();
}

void loop() {
    processCommands();
    checkGridStatus();
}
