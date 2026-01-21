import numpy as np
import sounddevice as sd

def analyze_tap():
    fs = 44100
    duration = 0.4

    audio = sd.rec(int(fs * duration), samplerate=fs, channels=1)
    sd.wait()

    signal = audio[:, 0]
    energy = np.mean(np.abs(np.fft.fft(signal)))

    if energy > 2500:
        return "MALAUHOG", signal
    elif energy > 1500:
        return "MALAKATAD", signal
    else:
        return "MALAKANIN", signal