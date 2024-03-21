# MP3 player demo for Alif Gen2 AppKit

A GUI and audio playback demo for Alif products.

The GUI is a slightly modified version of LVGL music player demo, modified files were copied to source/GUI. See libs (git submodules) for the dependencies.

## Hardware setup
This demo requires a working Gen2 Appkit with attached display. For audio playback, attach headphones or speakers to the HEADPHONES socket on the board.

## Building
The project uses CMAKE for build configuration and GCC or ARMCLang for compilation and linking. The project has several submodules which have to be initiated before building.

Example:
1. git submodule update --init
1. mkdir build_dir
1. cd build_dir
1. cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain-gnu.cmake -DCMAKE_BUILD_TYPE=Release ..
1. make -j

The resulting binary and elf file are located in the bin directory under the build_dir.

## Changing core
By default, the project is built to target M55_HP core. HE core can be selected by adding -DENSEMBLE_CORE=M55_HE to the cmake command.

## Changing the audio sample
The project expects an MP3 file in stereo format and 48 KHz sample rate. The file is then placed directly into the source code as a buffer and stored to MRAM along with the application.

1. store the audio sample as audio_sample.mp3
1. translate the sample to C header format: `xxd -i audio_sample.mp3 > audio_sample.h`
1. modify the audio_sample.h: 
    - < unsigned char audio_sample_mp3[]
    - \> unsigned char const audio_sample_mp3[]
1. modify the audio_sample.c
    - change artist, title, genre and track length
1. rebuild the project

## Running the demo
Use either debugger or SEtools to store the image to MRAM. The binary should be placed to start of MRAM ("mramAddress": "0x80000000") and set to be executed on the selected core.
