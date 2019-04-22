#!/usr/bin/python3
# -*- coding: utf-8 -*-

import sys
import re
import threading
from time import sleep
import serial  # pyserial
from PyQt5.QtCore import QTimer, Qt
from PyQt5.QtWidgets import QMainWindow, QApplication, QDesktopWidget, QLCDNumber


CROSSING_RE = re.compile(r'Crossing:')

SERIAL_PORT = 'COM3'  # 'COM*' - windows. '/dev/ttyUSB0' - linux
START_CMD = "start"
END_CMD = "end"
MAP_CMD = "map"

class TimerWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.seconds = 0
        self.minutes = 5
        self.stopped = True
        self.setup_lasers()
        self.init_ui()

    def setup_debug_mode(self):  # just send commands and configure lasers
        print("Starting debug")
        stop_event = threading.Event()
        listener = threading.Thread(target=self.serial_listener, args=(stop_event, ))
        listener.start()
        while True:
            cmd = input()
            if cmd == "help":
                print("map [sensors]- запустить маппинг; thresh [val] - поставить порог;\n"
                      "start; stop - начать/завершить игру; test - дебаг режим; help - помощь")
                continue
            if not cmd:
                stop_event.set()
                break
            self.device.write(cmd.encode())
        listener.join()
        print("Debug stopped")

    def serial_listener(self, stop_event):
        print('Listener started')
        while not stop_event.wait(0.5):
            if self.device.in_waiting:
                print(self.read_serial())
        print("Listener stopped")

    def setup_lasers(self):
        self.device = serial.Serial(SERIAL_PORT)  # new serial connection
        sleep(2)  # waiting to load
        self.setup_debug_mode()
        self.setup_ports()
        self.device.write(bytes(START_CMD))  # Activate System
        print(self.read_serial())
        sleep(2)
        self.setup_timer()  # timer to check crossing every 100ms

    def read_serial(self):
        return self.device.readline().decode().strip()

    def setup_ports(self):
        self.device.write("{0}".format(MAP_CMD).encode())  # send lazers
        for i in range(len(LASERS) + 2):  # read all info
            print(self.read_serial())

    def setup_timer(self):
        self.laser_timer = QTimer()
        self.laser_timer.setInterval(100)
        self.laser_timer.timeout.connect(self.check_crossing)
        self.laser_timer.start()

    def check_crossing(self):
        if self.device.in_waiting:  # if buffer not empty
            data = self.read_serial()
            if CROSSING_RE.findall(data):
                print(data)
                self.stop()
                self.stopped = True

    def init_ui(self):
        self.setup_lcd_timer()
        self.setWindowTitle('Laser maze')
        self.set_background()
        self.showFullScreen()

    def set_background(self):
        self.setAutoFillBackground(True)
        p = self.palette()
        p.setColor(self.backgroundRole(), Qt.black)
        self.setPalette(p)

    def display(self):
        self.LCD.display("%02d:%02d" % (self.minutes, self.seconds))
        if not self.stopped:
            if self.seconds == 0 and self.minutes == 0:
                self.stop()
            else:
                if self.seconds != 0:
                    self.seconds -= 1
                else:
                    self.seconds = 59
                    if self.minutes != 0:
                        self.minutes -= 1

    def reset(self):
        self.stop()
        self.stopped = True
        self.display()

    def start(self):
        self.timer.start()
        self.laser_timer.start()
        self.device.write(LASERS)  # allows continuation
        self.device.write(bytes(START_CMD))  # activate system

    def stop(self):
        self.timer.stop()
        self.laser_timer.stop()

    def setup_lcd_timer(self):
        screen = QDesktopWidget().screenGeometry()
        self.LCD = QLCDNumber(self)
        self.LCD.setFixedWidth(screen.width())
        self.LCD.setFixedHeight(screen.height())
        self.LCD.setDigitCount(5)
        self.LCD.setStyleSheet("color: rgb(255, 0, 0);")
        self.LCD.display("0" + str(self.minutes) + ":00")
        self.timer = QTimer()
        self.timer.setInterval(1000)
        self.timer.timeout.connect(self.display)

    def keyPressEvent(self, event):
        if event.key() == Qt.Key_Escape:
            self.close()

        if event.key() == Qt.Key_Up:
            if self.minutes < 59:
                self.minutes += 1
                self.seconds = 0
                self.reset()

        if event.key() == Qt.Key_Down:
            if self.minutes > 0:
                self.minutes -= 1
                self.seconds = 0
                self.reset()

        if event.key() == Qt.Key_Right:
            if self.seconds < 59:
                self.seconds += 1
                self.reset()

        if event.key() == Qt.Key_Left:
            if self.seconds > 0:
                self.seconds -= 1
                self.reset()

        if event.key() == Qt.Key_Space:
            if self.stopped:
                self.start()
                self.stopped = False
            else:
                self.reset()

        if event.key() == Qt.Key_Backspace:
            self.minutes = 5
            self.reset()


if __name__ == '__main__':
    app = QApplication(sys.argv)
    ex = TimerWindow()
    sys.exit(app.exec_())
