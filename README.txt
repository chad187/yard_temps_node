Process to add nodes
1. put esp32 into dfu mode by
    Hold the BOOT/GPIO0 button, tap the RESET/EN button once, and release BOOT/GPIO0
2. login to chirpstack go to applications -> gateway-app -> add device
    fill out name, generate device EUI, join EUI 0000000000000000, device profile tenant/stack monitor, disable frame-counter validation(keeping it off may be possible)
3. copy device EUI and paste into config.h
    ed70c6ee72c16e89 => #define RADIOLIB_LORAWAN_DEV_EUI   0xed70c6ee72c16e89ULL;
4. go to OTAA keys and generate a key and set gen app key to 00000000000000000000000000000000
5. get that key and take it here https://bits.ondrovo.com/hexc.html
   d61b23328f32f2cbeb4aacd70ee48e91 -> 0xd6, 0x1b, 0x23, 0x32, 0x8f, 0x32, 0xf2, 0xcb, 0xeb, 0x4a, 0xac, 0xd7, 0x0e, 0xe4, 0x8e, 0x91
6. put your new hex key in config.h here:
    #define RADIOLIB_LORAWAN_APP_KEY
    and
    #define RADIOLIB_LORAWAN_NWK_KEY
4. upload firmware
