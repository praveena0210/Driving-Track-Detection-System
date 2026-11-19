#include "esp_camera.h"
#include <WiFi.h>
#include "esp_http_server.h"

// ===========================
// Camera Pins - AI Thinker ESP32-CAM
// ===========================
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

// ===========================
// Wi-Fi Credentials
// ===========================
const char *ssid = "iot2025";
const char *password = "testing2025";

// ===========================
// Motor & Obstacle Pins
// ===========================
const int alertPin = 12;
const int in1 = 2;
const int in2 = 4;
const int in3 = 14;
const int in4 = 15;

const bool OBSTACLE_ACTIVE_LOW = false;

httpd_handle_t stream_httpd = NULL;   // Port 80
httpd_handle_t motor_httpd  = NULL;   // Port 81

// Forward declaration for the HTML page (needs motor IP/port)
String buildIndexHtml();

// ===========================
// Motor helpers
// ===========================
void stopRobot() {
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
}

void printMotorState(const char* tag) {
  Serial.printf("[MOTOR-%s] in1=%d in2=%d in3=%d in4=%d\n",
                tag,
                digitalRead(in1), digitalRead(in2),
                digitalRead(in3), digitalRead(in4));
}

inline bool obstacleDetected() {
  int s = digitalRead(alertPin);
  return OBSTACLE_ACTIVE_LOW ? (s == LOW) : (s == HIGH);
}

// ===========================
// HTML Control Page
// ===========================
static esp_err_t index_handler(httpd_req_t *req){
  Serial.println("[HTTP:80] / requested");
  httpd_resp_set_type(req, "text/html");
  String html = buildIndexHtml();
  return httpd_resp_send(req, html.c_str(), html.length());
}

String buildIndexHtml() {
  String ip = WiFi.localIP().toString();
  String base = "http://" + ip + ":81";   // ← motor commands go to port 81
  String html = "<html lang='en'><head><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>ESP32-CAM Robot Control</title>";
  html += "<style>";
  html += "body{font-family:Arial;text-align:center;margin:0;background:#1a1a1a;color:#fff;}";
  html += "h2{margin:10px;font-size:20px;}";
  html += "img{width:100%;max-width:400px;height:auto;border:2px solid #444;border-radius:8px;}";
  html += ".btn{background:#4CAF50;color:#fff;padding:14px 22px;font-size:16px;margin:5px;border:none;border-radius:5px;cursor:pointer;width:100px;user-select:none;}";
  html += ".btn:active{background:#2E7D32;}";
  html += ".stop{background:#f44336;}";
  html += ".stop:active{background:#B71C1C;}";
  html += "</style></head><body>";
  html += "<h2>ESP32-CAM Robot Live Control</h2>";
  html += "<div><img src=\"/stream\" /></div><br>";
  // Use non-blocking fetch with keepalive
  html += "<div><button class='btn' ontouchstart=\"cmd('forward')\" onmousedown=\"cmd('forward')\">Forward</button></div>";
  html += "<div>";
  html += "<button class='btn' ontouchstart=\"cmd('left')\" onmousedown=\"cmd('left')\">Left</button>";
  html += "<button class='btn stop' ontouchstart=\"cmd('stop')\" onmousedown=\"cmd('stop')\">Stop</button>";
  html += "<button class='btn' ontouchstart=\"cmd('right')\" onmousedown=\"cmd('right')\">Right</button>";
  html += "</div>";
  html += "<div><button class='btn' ontouchstart=\"cmd('backward')\" onmousedown=\"cmd('backward')\">Back</button></div>";
  html += "<script>";
  html += "const BASE='" + base + "';";
  html += "function cmd(a){";
  html += "  fetch(BASE+'/'+a,{mode:'no-cors',cache:'no-store'}).catch(e=>{});";
  html += "  if(navigator.vibrate) navigator.vibrate(30);";
  html += "}";
  html += "</script>";
  html += "</body></html>";
  return html;
}

// ===========================
// Motor handlers (Port 81)
// ===========================
static esp_err_t forward_handler(httpd_req_t *req){
  Serial.printf("[HTTP:81] FORWARD | alertPin=%d obstacle=%d\n",
                digitalRead(alertPin), obstacleDetected());
  if(obstacleDetected()) {
    stopRobot();
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, "Obstacle", HTTPD_RESP_USE_STRLEN);
  } else {
    digitalWrite(in1, HIGH); digitalWrite(in2, LOW);
    digitalWrite(in3, HIGH); digitalWrite(in4, LOW);
    printMotorState("FWD");
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, "Forward", HTTPD_RESP_USE_STRLEN);
  }
  return ESP_OK;
}

static esp_err_t backward_handler(httpd_req_t *req){
  Serial.println("[HTTP:81] BACKWARD");
  digitalWrite(in1, LOW); digitalWrite(in2, HIGH);
  digitalWrite(in3, LOW); digitalWrite(in4, HIGH);
  printMotorState("BWD");
  httpd_resp_set_type(req, "text/plain");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_send(req, "Backward", HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

static esp_err_t left_handler(httpd_req_t *req){
  Serial.println("[HTTP:81] LEFT");
  digitalWrite(in1, LOW); digitalWrite(in2, HIGH);
  digitalWrite(in3, HIGH); digitalWrite(in4, LOW);
  printMotorState("LEFT");
  httpd_resp_set_type(req, "text/plain");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_send(req, "Left", HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

static esp_err_t right_handler(httpd_req_t *req){
  Serial.println("[HTTP:81] RIGHT");
  digitalWrite(in1, HIGH); digitalWrite(in2, LOW);
  digitalWrite(in3, LOW); digitalWrite(in4, HIGH);
  printMotorState("RIGHT");
  httpd_resp_set_type(req, "text/plain");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_send(req, "Right", HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

static esp_err_t stop_handler(httpd_req_t *req){
  Serial.println("[HTTP:81] STOP");
  stopRobot();
  printMotorState("STOP");
  httpd_resp_set_type(req, "text/plain");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_send(req, "Stop", HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

// ===========================
// STREAM HANDLER (Port 80)
// ===========================
static esp_err_t stream_handler(httpd_req_t *req){
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  char part_buf[80];

  Serial.println("[HTTP:80] /stream client connected");

  static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=123456789000000000000987654321";
  static const char* _STREAM_BOUNDARY     = "\r\n--123456789000000000000987654321\r\n";
  static const char* _STREAM_PART         = "Content-Type: image/bmp\r\nContent-Length: %u\r\n\r\n";

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if(res != ESP_OK) return res;

  while (true) {
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      res = ESP_FAIL;
      break;
    }

    uint8_t * bmp_buf = NULL;
    size_t bmp_len = 0;
    bool ok = frame2bmp(fb, &bmp_buf, &bmp_len);
    esp_camera_fb_return(fb);
    fb = NULL;

    if (!ok || bmp_buf == NULL) {
      Serial.println("BMP conversion failed");
      res = ESP_FAIL;
      break;
    }

    size_t hlen = snprintf(part_buf, sizeof(part_buf), _STREAM_PART, bmp_len);
    res = httpd_resp_send_chunk(req, part_buf, hlen);
    if (res == ESP_OK) res = httpd_resp_send_chunk(req, (const char *)bmp_buf, bmp_len);
    if (res == ESP_OK) res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    free(bmp_buf);

    vTaskDelay(1);

    if (res != ESP_OK) break;
  }

  Serial.println("[HTTP:80] /stream client disconnected");
  return res;
}

// ===========================
// Start STREAM server (port 80, Core 0)
// ===========================
void startStreamServer(){
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port       = 80;
  config.ctrl_port         = 32768;
  config.max_uri_handlers  = 4;
  config.max_open_sockets  = 4;
  config.stack_size        = 8192;
  config.core_id           = 0;

  httpd_uri_t index_uri  = { .uri="/",       .method=HTTP_GET, .handler=index_handler,  .user_ctx=NULL };
  httpd_uri_t stream_uri = { .uri="/stream", .method=HTTP_GET, .handler=stream_handler, .user_ctx=NULL };

  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &index_uri);
    httpd_register_uri_handler(stream_httpd, &stream_uri);
    Serial.println("Stream server started on port 80 (Core 0)");
  } else {
    Serial.println("Stream server FAILED to start!");
  }
}

// ===========================
// Start MOTOR server (port 81, Core 1)
// Separate instance → never blocked by stream
// ===========================
void startMotorServer(){
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port       = 81;
  config.ctrl_port         = 32769;   // must be different from stream ctrl_port
  config.max_uri_handlers  = 8;
  config.max_open_sockets  = 4;
  config.stack_size        = 4096;
  config.core_id           = 1;

  httpd_uri_t forward_uri  = { .uri="/forward",  .method=HTTP_GET, .handler=forward_handler,  .user_ctx=NULL };
  httpd_uri_t backward_uri = { .uri="/backward", .method=HTTP_GET, .handler=backward_handler, .user_ctx=NULL };
  httpd_uri_t left_uri     = { .uri="/left",     .method=HTTP_GET, .handler=left_handler,     .user_ctx=NULL };
  httpd_uri_t right_uri    = { .uri="/right",    .method=HTTP_GET, .handler=right_handler,    .user_ctx=NULL };
  httpd_uri_t stop_uri     = { .uri="/stop",     .method=HTTP_GET, .handler=stop_handler,     .user_ctx=NULL };

  if (httpd_start(&motor_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(motor_httpd, &forward_uri);
    httpd_register_uri_handler(motor_httpd, &backward_uri);
    httpd_register_uri_handler(motor_httpd, &left_uri);
    httpd_register_uri_handler(motor_httpd, &right_uri);
    httpd_register_uri_handler(motor_httpd, &stop_uri);
    Serial.println("Motor server started on port 81 (Core 1)");
  } else {
    Serial.println("Motor server FAILED to start!");
  }
}

// ===========================
// Task wrappers
// ===========================
void streamServerTask(void *param) {
  startStreamServer();
  vTaskDelete(NULL);
}

void motorServerTask(void *param) {
  startMotorServer();
  vTaskDelete(NULL);
}

// ===========================
// Robot loop task — Core 1 (obstacle edge detection)
// ===========================
void robotLoopTask(void *param) {
  static unsigned long lastPrint = 0;
  static int lastAlert = -1;

  while (true) {
    int alertState = digitalRead(alertPin);

    if (alertState != lastAlert) {
      Serial.printf("[LOOP] alertPin changed: %d -> %d\n", lastAlert, alertState);
      lastAlert = alertState;

      if (obstacleDetected()) {
        stopRobot();
        Serial.println("[LOOP] Obstacle! Emergency stop");
      }
    }

    if (millis() - lastPrint > 5000) {
      lastPrint = millis();
      Serial.printf("[LOOP] alive | alertPin=%d | IP=%s | heap=%d\n",
                    alertState,
                    WiFi.localIP().toString().c_str(),
                    ESP.getFreeHeap());
    }

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// ===========================
// SETUP
// ===========================
void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  delay(1000);
  Serial.println("\n\n=== ESP32-CAM Robot Booting ===");

  pinMode(alertPin, INPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  stopRobot();

  Serial.printf("Motor pins: in1=%d in2=%d in3=%d in4=%d alert=%d\n",
                in1, in2, in3, in4, alertPin);
  Serial.printf("Initial alertPin state = %d\n", digitalRead(alertPin));

  Serial.println("Testing motor pins...");
  digitalWrite(in1, HIGH); delay(150); digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH); delay(150); digitalWrite(in2, LOW);
  digitalWrite(in3, HIGH); delay(150); digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH); delay(150); digitalWrite(in4, LOW);
  Serial.println("Motor pin test done.");

  // Camera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;

  config.pixel_format = PIXFORMAT_RGB565;
  config.frame_size   = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_count     = 2;
  config.fb_location  = CAMERA_FB_IN_PSRAM;
  config.grab_mode    = CAMERA_GRAB_LATEST;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed 0x%x\n", err);
    return;
  }
  Serial.println("Camera initialized OK");

  // WiFi
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi OK. IP: ");
  Serial.println(WiFi.localIP());

  // Motor server FIRST (needs to be ready before page loads)
  xTaskCreatePinnedToCore(motorServerTask,  "motorServer",  6000, NULL, 2, NULL, 1);
  delay(100);
  // Stream server SECOND
  xTaskCreatePinnedToCore(streamServerTask, "streamServer", 10000, NULL, 2, NULL, 0);
  delay(100);
  // Robot loop
  xTaskCreatePinnedToCore(robotLoopTask,    "robotLoop",    4096, NULL, 1, NULL, 1);

  Serial.print("Open http://");
  Serial.print(WiFi.localIP());
  Serial.println("/ in your browser");
  Serial.println("=== Ready ===");
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}