#include "progressBar.h"

#include "Arduino_LED_Matrix.h"

namespace ProgressBar {
ArduinoLEDMatrix matrix;

void setProgress(float progress) {
    constexpr size_t width = 12;
    constexpr size_t height = 8;
    constexpr size_t totalPixels = width * height;

    if (progress < 0.0f)
        progress = 0.0f;
    if (progress > 1.0f)
        progress = 1.0f;

    uint32_t frame[3] = {0, 0, 0};
    size_t pixelsToLight = static_cast<size_t>(progress * totalPixels);

    for (size_t i = 0; i < pixelsToLight; i++) {
        size_t arrayIndex = i / 32;
        size_t bitPosition = i % 32;

        frame[arrayIndex] |= (1UL << (31 - bitPosition));
    }

    matrix.loadFrame(frame);
}

void init() { matrix.begin(); }
} // namespace ProgressBar