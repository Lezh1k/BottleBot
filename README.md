** Bluetooth

1. Install bluez and bluez-tools. Start and enable bluetooth.service via systemctl.
2. From cli run `sudo bluetoothctl`.
3. Run next commands : 
* power on
* scan on
* pair 98:D3:31:F4:11:FF
* input pin :  1234
* quit
4. Instal rfcomm tool (bluez-rfcomm for arch linux).
5. Run `rfcomm bind rfcomm0 98:D3:31:F4:11:FF
6. Use /dev/rfcomm0 as serial port
