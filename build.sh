#!/bin/bash

pushd RobotUSB
g++ -std=c++17 linux-usb.cpp RobotUSB.cpp -o RobotUSB -lpthread
popd
