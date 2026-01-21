import cv2
import numpy as np

def detect_coconut(frame):
    hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)

    green_lower = np.array([35, 40, 40])
    green_upper = np.array([85, 255, 255])

    brown_lower = np.array([5, 40, 40])
    brown_upper = np.array([25, 255, 255])

    green_mask = cv2.inRange(hsv, green_lower, green_upper)
    brown_mask = cv2.inRange(hsv, brown_lower, brown_upper)

    if cv2.countNonZero(green_mask) > 8000:
        return "COCONUT"
    elif cv2.countNonZero(brown_mask) > 8000:
        return "NON_COCONUT"
    else:
        return "NONE"
 