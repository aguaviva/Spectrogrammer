# Spectrogrammer

Android app that computes a spectrogram

## Features
- 4096 bins (freq resolution of )
- Linear / Log scales for both axis
- Hold function
    - Save/load/delete/rename for comparing 
- Waterfall
- Configurable 
    - fft decay
    - fft overlap


## Screenshot
<img src="Screenshot.png" alt="Screenshot" width="200"/>

## Building
```
# clone with
git clone git@github.com:aguaviva/spectrogrammer.git --recurse-submodules --shallow-submodules
# build kissfft
make kissfft
# build and run the app (connect your cellphone!)
make push run 
```

## Notes/stuff I need help with:
- If app loses focus will crash/lose data
- getting a nice makefile that can compile for linux and windows
- I can't get to build the app and kissfft in on step
- Created a rudimetary lib for imgui to speed up linking, is there a better way?
- App shouldn't refresh as soon as possible, only when needed
- should prob use Cnlohr audio libs :)

## credits
- cnlohr for [rawdrawandroid](https://github.com/cnlohr/rawdrawandroid)
- mborgerding for [kissfft](https://github.com/mborgerding/kissfft)

[def]: Screenshot.png
