; Generated fixed normal-vector ABI. Do not edit.
.EXTERN _start
.EXTERN _Device_TimerIsr
.SECTION .app_vectors, TEXT
    BR !!_start
    BR !!device_default_isr
    BR !!device_default_isr ; vector 04
    BR !!device_default_isr ; vector 06
    BR !!device_default_isr ; vector 08
    BR !!device_default_isr ; vector 0A
    BR !!device_default_isr ; vector 0C
    BR !!device_default_isr ; vector 0E
    BR !!device_default_isr ; vector 10
    BR !!device_default_isr ; vector 12
    BR !!device_default_isr ; vector 14
    BR !!device_default_isr ; vector 16
    BR !!device_default_isr ; vector 18
    BR !!device_default_isr ; vector 1A
    BR !!device_default_isr ; vector 1C
    BR !!device_default_isr ; vector 1E
    BR !!device_default_isr ; vector 20
    BR !!device_default_isr ; vector 22
    BR !!device_default_isr ; vector 24
    BR !!device_default_isr ; vector 26
    BR !!device_default_isr ; vector 28
    BR !!device_default_isr ; vector 2A
    BR !!device_default_isr ; vector 2C
    BR !!_Device_TimerIsr ; vector 2E
    BR !!device_default_isr ; vector 30
    BR !!device_default_isr ; vector 32
    BR !!device_default_isr ; vector 34
    BR !!device_default_isr ; vector 36
    BR !!device_default_isr ; vector 38
    BR !!device_default_isr ; vector 3A
    BR !!device_default_isr ; vector 3C
    BR !!device_default_isr ; vector 3E
    BR !!device_default_isr ; vector 40
    BR !!device_default_isr ; vector 42
    BR !!device_default_isr ; vector 44
    BR !!device_default_isr ; vector 46
    BR !!device_default_isr ; vector 48
    BR !!device_default_isr ; vector 4A
    BR !!device_default_isr ; vector 4C
    BR !!device_default_isr ; vector 4E
    BR !!device_default_isr ; vector 50
    BR !!device_default_isr ; vector 52
    BR !!device_default_isr ; vector 54
    BR !!device_default_isr ; vector 56
    BR !!device_default_isr ; vector 58
    BR !!device_default_isr ; vector 5A
    BR !!device_default_isr ; vector 5C
    BR !!device_default_isr ; vector 5E
    BR !!device_default_isr ; vector 60
    BR !!device_default_isr ; vector 62
    BR !!device_default_isr ; vector 64
    BR !!device_default_isr ; vector 66
    BR !!device_default_isr ; vector 68
    BR !!device_default_isr ; vector 6A
    BR !!device_default_isr ; vector 6C
    BR !!device_default_isr ; vector 6E
    BR !!device_default_isr ; vector 70
    BR !!device_default_isr ; vector 72
    BR !!device_default_isr ; vector 74
    BR !!device_default_isr ; vector 76
    BR !!device_default_isr ; vector 78
    BR !!device_default_isr ; vector 7A
    BR !!device_default_isr ; vector 7C
    BR !!device_default_brk
.SECTION .text, TEXT
device_default_isr:
    RETI
device_default_brk:
    RETB
