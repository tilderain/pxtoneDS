# pxtone DS

pxtone sample playback project for Nintendo DS

## Compilation
Run `make NITRO=1`. Rom won't work in the latest version of melonDS due to this issue: https://github.com/Arisotura/melonDS/issues/1116

## Notes
Load times should be ok-ish for small songs (20kb).
Expect loading times of upwards of 30 seconds for large projects. (and a swift crash if it tries to allocate more than 4mb of ram)
You can reduce memory usage and load time by half when using lower sample rate sounds and 8 bpp.
Bottleneck is currently ptv and ptn wave generation, and ogg decoding.
Recommended to convert sounds to PCM and/or compress the ptcop.

pxtone implements a sample-by-sample software mixer, which runs very poorly on the DS by default.
Optimal playback is by using the DS's hardware mixer, in this sense pxtone is really just an enhanced version of Organya.

Reverb, overdrive, and pan time effects are unavailable. There may be a way to implement reverb (need to look into how the NDS SDK does it).

This project uses modified libnds functions, in soundfifo.cpp, as well as a modified arm7 default code.
You will need to include the arm7 code here and use the `getFreeChannel` function from soundFifo.cpp in your project.

## Thanks to
devkitPro

Pixel