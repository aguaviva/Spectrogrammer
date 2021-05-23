# Spectrogrammer
Android app that shows a spectrogram

## Features
 - Low latency audio processing, uses
     - Multithreaded native code
     - OpenSL
 - Spectrum types:
   - FFT, for scientific purposes
     - Up to 8192 frequency bins
     - Log/Linear scales
   - Goertzel bank filter, centered at notes frequencies, for tuning instruments      
 - Configurable FFT overlap
 - Vertical bars smoothing 
 - Pan & Zoom
 - Measuring cursor 
 

## Screenshots

This is me singing from the lowest note I can perform to the highest. I did this just for the screenshot, and it turns out I have the range of a baritono!

<img src="device-2021-05-02-172252.png" width="200"/>

And the final Q/A test, displaying Lena(https://www.youtube.com/watch?v=S64FROErFYA), or half of it as YouTube downsampled the sound to 22Khz.

<img src="lena-2021-05-02-175125.png" width="200"/>
