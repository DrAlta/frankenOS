MSRSetting user_MsrSetting = {
#ifdef	MSR_LED_SETTING
'0',
'1',
#endif
'0',
'0',
"",
"",
'0',
13,
'1',
#ifdef	MSR_RS232_SETTING
MsrBR9600,
MsrPNone,
MsrDataBits8,
MsrXOnOff,
MsrStopBits1,
DC3,			// X-On
DC1,			// X-Off
#endif
'0',
'0',
{{49,81,37,124,63},{48,26,59,31,63},{48,26,59,31,63}},
{{49,110,91},{49,31,92},{49,94,93},{48,11,58},{48,7,60},{48,14,62}},
{{0},{0},{0},{0},{0},{0}},
{{0,255}},
{{0}}
};
