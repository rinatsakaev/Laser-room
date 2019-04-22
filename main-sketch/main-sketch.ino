const int LASER_COUNT = 8;
const int LASER_PINS[LASER_COUNT]  = {6, 7, 8, 9, 10, 11, 12, 13};
const int SENSOR_PINS[LASER_COUNT] = {A0, A1, A2, A3, A4, A5, A6, A7};
const int NO_SENSOR = -1;

int sensor_for_laser[LASER_COUNT];
bool sensor_state_for_laser[LASER_COUNT];
bool isSystemActive = false;
bool testing_mode = false;
int threshold = 100;

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
	  int windowSize = 100;
	  long sum = 0;
	  for (int i = 0; i < windowSize; i++) {
		byte val = analogRead(SENSOR_PINS[sensor]);
		sum += val;
	  }
	  return (sum / windowSize) >= threshold;
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
    bool usedSensors[LASER_COUNT];
    for (int i = 0; i < LASER_COUNT; ++i)
        usedSensors[i] = false;
    int notMatched = LASER_COUNT;
    while (notMatched != 0) {
        for (int i = 0; i < LASER_COUNT; ++i) {
            if (sensor_for_laser[i] != NO_SENSOR)
                continue;
            turnLaserOn(i);
            for (int sensorIndex = 0; sensorIndex < LASER_COUNT; ++sensorIndex) {
                delay(2);
                if (isSensorLighted(sensorIndex) && !usedSensors[sensorIndex]) {
					Serial.println("Laser pin " + String(LASER_PINS[i]) + " -> Sensor pin " + String(SENSOR_PINS[sensorIndex]));
                    sensor_for_laser[i] = sensorIndex;
                    usedSensors[sensorIndex] = true;
                    notMatched--;
                    break;
                }
            }
            turnLaserOff(i);
        }
    }
    for (int i = 0; i < length; ++i)
        turnLaserOn(i);
}

inline void processCommands() {
    if (!Serial.available())
        return;

    String cmd = Serial.readString();
	cmd.trim();
	String cmd_part = "";
	String arg_part = "";
	bool isSplitted = false;
	for (int i = 0; i < cmd.length(); i++){
		if (cmd[i] == ' '){
			isSplitted = true;
			continue;
		}
		if (!isSplitted)
			cmd_part += cmd[i];
		if (isSplitted)
			arg_part += cmd[i];
	}
    if (cmd_part == "stop") {
        clearState();
        Serial.println("System Deactivated");
    }
    else if (cmd_part == "start") {
        activateSystem();
        Serial.println("System Activated");
    }
    else if (cmd_part == "test") {
        testing_mode = !testing_mode;
        if (testing_mode)
            Serial.println("SetUp Mode Activated");
        else
            Serial.println("SetUp Mode Deactivated");
    }
    else if (cmd_part == "map") {
        findMapping();
        Serial.println("Mapping Done!");
    } else if (cmd_part == "on"){
		turnLaserOn(arg_part[0].toInt());
		Serial.println("Laser "+arg_part[0]+" turned on");
	} else if (cmd_part == "off"){
		turnLaserOff(arg_part[0].toInt());
		Serial.println("Laser "+arg_part[0]+" turned off");
	} else if (cmd_part == "thresh") {
		threshold = arg_part.toInt();
		Serial.println("Threshold set to " + String(threshold));
	} else {
		Serial.println("Unknown command" + String(cmd_part) + " arg = "+String(arg_part));
	}
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
    for (int i = 0; i < count; ++i) {
        for (int j = 0; j < LASER_COUNT; ++j)
            turnLaserOn(j);
        delay(delay_time);
        for (int j = 0; j < LASER_COUNT; ++j)
            turnLaserOff(j);
        delay(delay_time);
    }
}

inline void checkGridStatus() {
    if (testing_mode) {
        Serial.print("+");
        for (int i = 0; i < LASER_COUNT; ++i)
            if (isSensorLighted(sensor_for_laser[i]))
                Serial.print(i);
        Serial.print("+   -");
        for (int i = 0; i < LASER_COUNT; ++i)
            if (!isSensorLighted(sensor_for_laser[i]))
                Serial.print(i);
        Serial.println("-");
        delay(2000);
    }
    for (int i = 0; i < LASER_COUNT; ++i) {
        bool sensorIsLighted = isSensorLighted(sensor_for_laser[i]);
        if (sensorIsLighted != sensor_state_for_laser[i])
            sensor_state_for_laser[i] = sensorIsLighted;

        if (isSystemActive && !sensorIsLighted) {
            reportLaserCrossing(i);
            lasers_blink(250, 5);
            clearState();
            Serial.println("System Deactivated");
            return;
        }
    }
}

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