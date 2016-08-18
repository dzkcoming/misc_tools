#!/bin/bash

gcc demo.c -o app -O0 -g -I/usr/local/curl/include/ -I/usr/local/include/ -L/usr/local/curl/lib -lcurl -lcrypto
