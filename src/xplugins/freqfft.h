/*
Copyright (c) 2021, Greg de Valois
Software License Agreement (MIT License)
See license.txt for the terms of this license.
*/

#if FREQ_FFT

typedef void (*FreqFFT_SetPos_CB)(int pos, float value);

extern bool FreqFFT_Init(int rate, uint16_t count);
extern void FreqFFT_Fini(void);
extern void FreqFFT_Begin(int min, int max);
extern void FreqFFT_Next(FreqFFT_SetPos_CB valueCB);

#endif // FREQ_FFT
