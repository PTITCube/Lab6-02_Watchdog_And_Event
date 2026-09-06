#include "Camera.h"
#include <SD_MMC.h>
#include <esp_camera.h>

#define CAMERA_MODEL_AI_THINKER

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

PTIT_Camera::PTIT_Camera() : gpsSelectPin(12), camSelectPin(14), cameraReady(false), sdReady(false) {
}

bool PTIT_Camera::initializeSd() {
    if (!SD_MMC.begin("/sdcard", true)) {
        Serial.println("[Camera] MicroSD mount failed");
        sdReady = false;
        return false;
    }

    sdReady = true;
    Serial.println("[Camera] MicroSD ready for image storage");
    return true;
}

bool PTIT_Camera::initializeCamera() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 12;
    config.fb_count = 2;

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("[Camera] Camera init failed with error 0x%x\n", err);
        cameraReady = false;
        return false;
    }

    cameraReady = true;
    Serial.println("[Camera] ESP32-CAM initialized");
    return true;
}

bool PTIT_Camera::init(uint8_t gpsSelectPin, uint8_t camSelectPin) {
    this->gpsSelectPin = gpsSelectPin;
    this->camSelectPin = camSelectPin;

    if (gpsSelectPin != 255 && camSelectPin != 255) {
        pinMode(this->gpsSelectPin, OUTPUT);
        pinMode(this->camSelectPin, OUTPUT);
        digitalWrite(this->gpsSelectPin, LOW);
        digitalWrite(this->camSelectPin, HIGH);
    }

    Serial.println("[Camera] Switching UART2 to ESP32-CAM...");

    initializeSd();
    return initializeCamera();
}

void PTIT_Camera::selectGPS() {
    digitalWrite(gpsSelectPin, HIGH);
    digitalWrite(camSelectPin, LOW);
}

void PTIT_Camera::selectCamera() {
    digitalWrite(gpsSelectPin, LOW);
    digitalWrite(camSelectPin, HIGH);
}

bool PTIT_Camera::isReady() const {
    return cameraReady && sdReady;
}

static bool saveFrameToSd(const camera_fb_t* fb, const char* path) {
    if (fb == nullptr) {
        Serial.println("[Camera] Cannot save image: frame unavailable");
        return false;
    }

    String filePath = path;
    int lastSlash = filePath.lastIndexOf('/');
    if (lastSlash > 0) {
        String dirPath = filePath.substring(0, lastSlash);
        if (!SD_MMC.exists(dirPath.c_str())) {
            if (!SD_MMC.mkdir(dirPath.c_str())) {
                Serial.println("[Camera] Failed to create folder: " + dirPath);
                return false;
            }
        }
    }

    File file = SD_MMC.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("[Camera] Failed to open file for writing: " + String(path));
        return false;
    }

    size_t written = file.write(fb->buf, fb->len);
    file.close();

    if (written != fb->len) {
        Serial.println("[Camera] File write incomplete");
        return false;
    }

    Serial.println("[Camera] Image saved: " + String(path));
    return true;
}

bool PTIT_Camera::capture() {
    return captureToFile("/photos/capture.jpg");
}

bool PTIT_Camera::captureToFile(const char* path) {
    if (!cameraReady) {
        Serial.println("[Camera] Camera is not initialized");
        return false;
    }

    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("[Camera] Failed to capture frame");
        return false;
    }

    bool ok = saveFrameToSd(fb, path);
    esp_camera_fb_return(fb);
    return ok;
}
