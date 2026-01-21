# sound_analyzer.py
import numpy as np
import sounddevice as sd

def analyze_tap():
    fs = 44100
    duration = 0.4

    audio = sd.rec(int(fs * duration), samplerate=fs, channels=1)
    sd.wait()

    energy = np.mean(np.abs(np.fft.fft(audio[:, 0])))
    print(f"Audio energy: {energy}")
    if energy > 2:
        return "MALAUHOG"
    elif energy > 4:
        return "MALAKATAD"
    else:
        return "MALAKANIN"
