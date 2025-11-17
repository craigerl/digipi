#!/bin/bash

# record 30 seconds of audio, put in foobar.wav

arecord -d 30 -c 2 -f S16_LE -r 44100 -t wav  foobar.wav

#USB card
#arecord -D default:CARD=CODEC  -d 30 -c 2 -f S16_LE -r 44100 -t wav  foobar.wav

