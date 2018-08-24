import cv2
import numpy as np
from matplotlib import pyplot as plt
import math

w = 1024;
d = 8;

tex = np.zeros((w,w,4), np.uint8)

cv2.rectangle(tex, (0,0), (w, w), (183, 138, 18, 127), -1, 8, 0);
cv2.rectangle(tex, (10*d,0), (w - 10*d, w/2-7*d), (183, 138, 18, 0), -1, 8, 0);
cv2.rectangle(tex, (12*d,0), (w - 12*d, w/2-5*d), (183, 138, 18, 0), -1, 8, 0);
cv2.circle(tex, (12*d, w/2-7*d), 2*d, (183, 138, 18, 0), -1, 8, 0);
cv2.circle(tex, (w - 12*d, w/2-7*d), 2*d, (183, 138, 18, 0), -1, 8, 0);

cv2.rectangle(tex, (10*d,w/2 + 7*d), (w - 10*d, w), (183, 138, 18, 0), -1, 8, 0);
cv2.rectangle(tex, (12*d,w/2 + 5*d), (w - 12*d, w), (183, 138, 18, 0), -1, 8, 0);
cv2.circle(tex, (12*d, w/2 + 7*d), 2*d, (183, 138, 18, 0), -1, 8, 0);
cv2.circle(tex, (w - 12*d, w/2 + 7*d), 2*d, (183, 138, 18, 0), -1, 8, 0);

cv2.rectangle(tex, (0,0), (7*d, w), (131, 141, 149, 255), -1, 8, 0);
cv2.rectangle(tex, (w,0), (w - 7*d, w), (131, 141, 149, 255), -1, 8, 0);
cv2.rectangle(tex, (0,w/2 - 2*d), (w, w/2 + 2*d), (131, 141, 149, 255), -1, 8, 0);

def draw_hex(tex, x0, y0, r, c):
    lt = []
    for i in range(0, 6):
        t = i / 6.0
        x = x0 + math.cos(t * 2 * 3.14) * r
        y = y0 + math.sin(t * 2 * 3.14) * r
        lt.append([int(x),int(y)]);
    cv2.fillConvexPoly(tex, np.asarray(lt), c);

for i in range(3*d, w/2 - 2*d, 3*d + 4):
    dark_grey = (131 - 30, 141- 30, 149- 30, 255);
    draw_hex(tex, 2*d, i, 1.5*d, dark_grey)
    draw_hex(tex, 5*d, i + 1.5*d + 2, 1.5*d, dark_grey)
    draw_hex(tex, w - 2*d, i, 1.5*d, dark_grey)
    draw_hex(tex, w - 5*d, i + 1.5*d + 2, 1.5*d, dark_grey)

for i in range(w/2 + 3*d, w - 3*d, 3*d + 4):
    dark_grey = (131 - 30, 141- 30, 149- 30, 255);
    draw_hex(tex, 2*d, i + 1.5*d + 2, 1.5*d, dark_grey)
    draw_hex(tex, 5*d, i, 1.5*d, dark_grey)
    draw_hex(tex, w - 2*d, i + 1.5*d + 2, 1.5*d, dark_grey)
    draw_hex(tex, w - 5*d, i, 1.5*d, dark_grey)

cv2.imshow('image', tex)
plt.imshow(tex), plt.colorbar(),plt.show()
cv2.waitKey(0)
cv2.destroyAllWindows()

cv2.imwrite("texture.png", tex)
