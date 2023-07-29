# PowMr-MPPT-Solar-Charge-Controller-read-display

This SW is for the Raspberry Pi Pico is reading data from the display communication of the
PowMr MPPT Solar Charge Controller(s), buffering them and sending them on request to the control-PC, 
using a ModBus protocol that is compatible with the ModBus protocol of the DALY BMS. One of the requests 
allows for the Reset of the charge controller. 
