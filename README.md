# STM32F411 USB to I2S DAC Audio Bridge

## Features

* USB Full Speed Class 1 device
* Isochronous with endpoint feedback to maintain synchronization
* Bus powered
* Supports 24-bit 44.1kHz, 48kHz and 96kHz audio streams
* I2S master output with I2S Philips standard 24/32 data frame
* Optional MCLK output generation


## Credits
[Dragonman USB Audio project](https://github.com/dragonman225/stm32f469-usbaudio)

## Software Development Environment
* Ubuntu 20.04 AMDx64
* STM32CubeIDE v1.3.0 (makefile project)

## Hardware

* WeAct STM32F411CEU6 development board
* PCM5102A I2S DAC module

<img src="prototype.jpg" />

## Checking USB Audio device on Ubuntu 20.04

* Execute `lsusb` with and without the USB-Audio DAC plugged in, you should see the 
  new USB device
  
<img src="lsusb.png" />
  
* Execute `aplay -L` and look for `PCM5102 DAC`

<img src="aplay_output.png" />

* Run the Sound application without the USB-Audio DAC plugged in and check the
  Speaker/Headphone output options
* Plug in the USB-Audio DAC and check again, you should see at least one new option.
  Select this for playing sound output via the USB-Audio DAC
* Execute `cat /proc/asound/DAC/stream0` when a song is playing

<img src="stream.png" />

## Optimizing Pulseaudio on Ubuntu 20.04 for USB-Audio DAC

* Edit `/etc/pulse/daemon.conf` as root
* Force re-sampling to 96kHz, resize to 24bits
* Use highest quality re-sampling algorithm
* Save file, log out and login again for the changes to take effect

<img src="pulseaudio_config.png" />
