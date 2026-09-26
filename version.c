
#ifdef VERSION_STRING
    #define VER     " "VERSION_STRING
#else
    #define VER     ""
#endif

#ifdef ENABLE_FEAT_F4HWN
    const char Version[]      = AUTHOR_STRING_2 " " VERSION_STRING_2;
    const char Edition[]      = EDITION_STRING;
#else
    const char Version[]      = AUTHOR_STRING VER;
#endif

#ifdef ENABLE_AIS_RX
    // Full requested AIS-branch credit is too long for the small welcome screen.
    const char UART_Version[] = "UV-K5 AIS-RX v0.1 by Dr. Heinz Doofenshmirtz\r\n";
#else
    const char UART_Version[] = "UV-K5 Firmware, " AUTHOR_STRING VER "\r\n";
#endif
