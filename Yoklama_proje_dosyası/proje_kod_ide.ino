#include "esp_camera.h"
#include <WiFi.h>

// ===== WiFi Ayarları =====
const char* ssid = "Zehra";
const char* password = "Zehra123";

// ===== ESP32-CAM Pinleri =====
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

WiFiServer server(80);

// ===================== FONKSİYONLAR =====================

// ===== YUV422 → RGB888 =====
void yuv422_to_rgb888(uint8_t* yuv, uint8_t* rgb, int width, int height) {
    for (int i = 0; i < width * height * 2; i += 4) {
        int y0 = yuv[i], u = yuv[i+1], y1 = yuv[i+2], v = yuv[i+3];
        int c = y0, d = u - 128, e = v - 128;
        rgb[i*3/2 + 2] = constrain(c + 1.402 * e, 0, 255); // R0
        rgb[i*3/2 + 1] = constrain(c - 0.344*d - 0.714*e, 0, 255); // G0
        rgb[i*3/2 + 0] = constrain(c + 1.772*d, 0, 255); // B0
        c = y1;
        rgb[i*3/2 + 5] = constrain(c + 1.402 * e, 0, 255); // R1
        rgb[i*3/2 + 4] = constrain(c - 0.344*d - 0.714*e, 0, 255); // G1
        rgb[i*3/2 + 3] = constrain(c + 1.772*d, 0, 255); // B1
    }
}

// ===== RGB565 → RGB888 =====
void rgb565_to_rgb888(uint8_t* rgb565, uint8_t* rgb888, int width, int height) {
    uint16_t* src = (uint16_t*)rgb565;
    for (int i = 0; i < width*height; i++) {
        uint16_t p = src[i];
        uint8_t r = ((p>>11)&0x1F)<<3;
        uint8_t g = ((p>>5)&0x3F)<<2;
        uint8_t b = (p&0x1F)<<3;
        rgb888[i*3] = b; rgb888[i*3+1] = g; rgb888[i*3+2] = r;
    }
}

// ===== BMP Header =====
void sendBMPHeader(WiFiClient &client, int width, int height) {
    uint8_t header[54] = {0};
    uint32_t fileSize = 54 + width*height*3;
    header[0] = 'B'; header[1] = 'M';
    header[2] = fileSize; header[3] = fileSize>>8; header[4] = fileSize>>16; header[5] = fileSize>>24;
    header[10] = 54;
    header[14] = 40;
    header[18] = width; header[19] = width>>8; header[20] = width>>16; header[21] = width>>24;
    header[22] = height; header[23] = height>>8; header[24] = height>>16; header[25] = height>>24;
    header[26] = 1;
    header[28] = 24;
    uint32_t imgSize = width*height*3;
    header[34] = imgSize; header[35] = imgSize>>8; header[36] = imgSize>>16; header[37] = imgSize>>24;
    client.write(header, 54);
}

// ===== Kamera Başlat =====
bool initCamera() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM; config.pin_d1 = Y3_GPIO_NUM; config.pin_d2 = Y4_GPIO_NUM; config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM; config.pin_d5 = Y7_GPIO_NUM; config.pin_d6 = Y8_GPIO_NUM; config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM; config.pin_pclk = PCLK_GPIO_NUM; config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM; config.pin_sccb_sda = SIOD_GPIO_NUM; config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM; config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 8000000; config.pixel_format = PIXFORMAT_YUV422;
    config.frame_size = FRAMESIZE_QVGA; config.jpeg_quality = 0; config.fb_count = 1;
    
    if (esp_camera_init(&config) != ESP_OK) return false;
    sensor_t *s = esp_camera_sensor_get();
    if (s) { s->set_saturation(s, 2); s->set_gain_ctrl(s, 1); s->set_bpc(s, 1); s->set_wpc(s,1); }
    return true;
}

// ===== WiFi Başlat =====
void initWiFi() {
    WiFi.begin(ssid, password);
    Serial.print("📡 WiFi bağlanıyor");
    while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
    Serial.println("\n✅ WiFi Bağlandı: " + WiFi.localIP().toString());
}

// ===== Web Sayfası =====
void sendHTMLPage(WiFiClient &client) {
    client.println(R"rawliteral(
HTTP/1.1 200 OK
Content-Type: text/html
Connection: close

<!DOCTYPE html>
<html>
<head>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<title>ESP32-CAM OV7670</title>
<style>
body { font-family: Arial; text-align: center; background: #f0f0f0; }
h1 { color: #333; }
img { border: 3px solid #4CAF50; border-radius: 10px; margin: 20px; }
button { padding: 12px 24px; margin: 10px; background: #4CAF50; color: white; border: none; border-radius: 5px; cursor: pointer; }
button:hover { background: #45a049; }
</style>
<script>
function refreshImage() {
  document.getElementById('cameraImage').src = '/image?t=' + new Date().getTime();
}
setInterval(refreshImage, 2000);
</script>
</head>
<body>
<h1>📷 ESP32-CAM OV7670</h1>
<button onclick='refreshImage()'>🔄 Yenile</button>
<br>
<img id='cameraImage' src='/image' width='320' height='240'>
</body>
</html>
)rawliteral");
}

// ========================================================

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("📷 ESP32-CAM Başlatılıyor...");
    if (!initCamera()) { Serial.println("❌ Kamera başlatılamadı!"); return; }
    initWiFi();
    server.begin();
}

void loop() {
    WiFiClient client = server.available();
    if (!client) return;

    String req = client.readStringUntil('\r'); client.flush();
    if (req.indexOf("GET / ") >= 0) sendHTMLPage(client);
    else if (req.indexOf("GET /image") >= 0) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) { client.stop(); return; }
        uint8_t* rgb888 = (uint8_t*)malloc(fb->width*fb->height*3);
        if (!rgb888) { esp_camera_fb_return(fb); client.stop(); return; }
        if (fb->format == PIXFORMAT_YUV422) yuv422_to_rgb888(fb->buf,rgb888,fb->width,fb->height);
        else if (fb->format == PIXFORMAT_RGB565) rgb565_to_rgb888(fb->buf,rgb888,fb->width,fb->height);
        else memset(rgb888,128,fb->width*fb->height*3);
        client.println("HTTP/1.1 200 OK"); client.println("Content-Type: image/bmp"); client.println("Connection: close"); client.println();
        sendBMPHeader(client, fb->width, fb->height);
        client.write(rgb888, fb->width*fb->height*3);
        free(rgb888); esp_camera_fb_return(fb);
    }
    client.stop();
}