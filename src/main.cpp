/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-websocket-server-arduino/
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/

// Import required libraries
#include <WiFi.h>
#include <AsyncTCP.h>
#include <AsyncJson.h>
#include <Arduino.h>
#include <ESPAsyncWebServer.h>

// Replace with your network credentials
const char *ssid = "ESP32";
const char *password = "12345678";

/* Put IP Address details */
IPAddress local_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

bool ledState = 0;
const int ledPin = 2;

const int LEFT_MOTOR_FWD_PIN = 12;
const int LEFT_MOTOR_REV_PIN = 13;
const int RIGHT_MOTOR_FWD_PIN = 14;
const int RIGHT_MOTOR_REV_PIN = 15;

const int LEFT_MOTOR_FWD_CHANNEL = 0;
const int LEFT_MOTOR_REV_CHANNEL = 1;
const int RIGHT_MOTOR_FWD_CHANNEL = 2;
const int RIGHT_MOTOR_REV_CHANNEL = 3;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>

<head>
    <title>Robot Controller</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="icon" href="data:,">
    <style>
        html {
            font-family: Arial, Helvetica, sans-serif;
            text-align: center;
        }

        h1 {
            font-size: 1.8rem;
            color: white;
        }

        h2 {
            font-size: 1.5rem;
            font-weight: bold;
            color: #143642;
        }

        .topnav {
            overflow: hidden;
            background-color: #143642;
        }

        body {
            margin: 0;
        }

        .content {
            padding: 30px;
            max-width: 600px;
            margin: 0 auto;
        }

        .card {
            background-color: #F8F7F9;
            ;
            box-shadow: 2px 2px 12px 1px rgba(140, 140, 140, .5);
            padding-top: 10px;
            padding-bottom: 20px;
        }

        .button {
            padding: 15px 50px;
            font-size: 24px;
            text-align: center;
            outline: none;
            color: #fff;
            background-color: #0f8b8d;
            border: none;
            border-radius: 5px;
            -webkit-touch-callout: none;
            -webkit-user-select: none;
            -khtml-user-select: none;
            -moz-user-select: none;
            -ms-user-select: none;
            user-select: none;
            -webkit-tap-highlight-color: rgba(0, 0, 0, 0);
        }

        /*.button:hover {background-color: #0f8b8d}*/
        .button:active {
            background-color: #0f8b8d;
            box-shadow: 2 2px #CDCDCD;
            transform: translateY(2px);
        }

        .state {
            font-size: 1.5rem;
            color: #8c8c8c;
            font-weight: bold;
        }
    </style>
    <title>Robot Controller</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="icon" href="data:,">
</head>

<body>
    <div class="topnav">
        <h1>Robot Controller</h1>
    </div>
    <div class="content">
        <div class="card">
            <!-- <h2>Output - GPIO 2</h2>
            <p class="state">state: <span id="state">%STATE%</span></p> -->
            <input type="text" id="speed" name="speed" value="100">
            <p><button id="leftForwardButton" class="button">FWD</button><button id="rightForwardButton"
                    class="button">FWD</button></p>
            <p><button id="leftReverseButton" class="button">REV</button><button id="rightReverseButton"
                    class="button">REV</button></p>
        </div>
    </div>
    <script>
        var ip = window.location.hostname;
        //var ip = "192.168.1.1";
        var gateway = `ws://${ip}/ws`;
        var websocket;
        window.addEventListener('load', onLoad);
        function initWebSocket() {
            console.log('Trying to open a WebSocket connection...');
            websocket = new WebSocket(gateway);
            websocket.onopen = onOpen;
            websocket.onclose = onClose;
            websocket.onmessage = onMessage; // <-- add this line
        }
        function onOpen(event) {
            console.log('Connection opened');
        }
        function onClose(event) {
            console.log('Connection closed');
            setTimeout(initWebSocket, 2000);
        }
        function onMessage(event) {
            var state;
            if (event.data == "1") {
                state = "ON";
            }
            else {
                state = "OFF";
            }
        }
        function onLoad(event) {
            initWebSocket();
            initButton();
        }
        function initButton() {
            document.getElementById('leftForwardButton').addEventListener('pointerdown', () => { handleEvent(1, 1); });
            document.getElementById('leftForwardButton').addEventListener('pointerup', () => { handleEvent(1, 0);});

            document.getElementById('leftReverseButton').addEventListener('pointerdown', () => { handleEvent(1, -1);});
            document.getElementById('leftReverseButton').addEventListener('pointerup', () => { handleEvent(1, 0);});

            document.getElementById('rightForwardButton').addEventListener('pointerdown', () => { handleEvent(2, 1);});
            document.getElementById('rightForwardButton').addEventListener('pointerup',() => { handleEvent(2, 0);});

            document.getElementById('rightReverseButton').addEventListener('pointerdown', () => { handleEvent(2, -1);});
            document.getElementById('rightReverseButton').addEventListener('pointerup', () => { handleEvent(2, 0);});
        }
        function handleEvent(motor, power) {
            var text;
            power = parseInt(document.getElementById('speed').value) * power
            if (motor === 1) {
                text = JSON.stringify({
                    leftMotorPower: power
                });
            } else {
                text = JSON.stringify({
                    rightMotorPower: power
                });
            }
            if (websocket.readyState === WebSocket.OPEN) {
                websocket.send(text);
            }
            console.log(text);
        }
    </script>
</body>

</html>
)rawliteral";

/**
 * Left Motor (perspective of robot) Control
 * 
 * This function takes a motor power from 127 to -128.
 * 
 * Negative powers are backwards, and positive powers are forwards
 * 
 * A power of zero commands coast mode
 */
void setLeftMotorPower(int8_t power)
{
    Serial.printf("Left Power: %i\n", power);
    if(power > 0) { //Forward
        ledcWrite(LEFT_MOTOR_FWD_CHANNEL, abs(power)*2);
        ledcWrite(LEFT_MOTOR_REV_CHANNEL, 0);
    } else if (power < 0) {
        ledcWrite(LEFT_MOTOR_FWD_CHANNEL, 0);
        ledcWrite(LEFT_MOTOR_REV_CHANNEL, abs(power)*2);
    } else {
        ledcWrite(LEFT_MOTOR_FWD_CHANNEL, 0);
        ledcWrite(LEFT_MOTOR_REV_CHANNEL, 0);
    }
}

/**
 * Right Motor (perspective of robot) Control
 * 
 * This function takes a motor power from 127 to -128.
 * 
 * Negative powers are backwards, and positive powers are forwards
 * 
 * A power of zero commands coast mode
 */
void setRightMotorPower(int8_t power)
{
    Serial.printf("Right Power: %i\n", power);
    if(power > 0) { //Forward
        ledcWrite(RIGHT_MOTOR_FWD_CHANNEL, abs(power)*2);
        ledcWrite(RIGHT_MOTOR_REV_CHANNEL, 0);
    } else if (power < 0) {
        ledcWrite(RIGHT_MOTOR_FWD_CHANNEL, 0);
        ledcWrite(RIGHT_MOTOR_REV_CHANNEL, abs(power)*2);
    } else {
        ledcWrite(RIGHT_MOTOR_FWD_CHANNEL, 0);
        ledcWrite(RIGHT_MOTOR_REV_CHANNEL, 0);
    }
}

void notifyClients()
{
    ws.textAll(String(ledState));
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
    {
        data[len] = 0;
        DynamicJsonDocument jsonBuffer(1024);
        DeserializationError error = deserializeJson(jsonBuffer, data);
        if (!error)
        {
            JsonVariant json = jsonBuffer.as<JsonVariant>();
            auto &&data = json.as<JsonObject>();

            if (data["leftMotorPower"].is<int8_t>())
            {
                int8_t power = data["leftMotorPower"].as<int8_t>();
                setLeftMotorPower(power);
            }

            if (data["rightMotorPower"].is<int8_t>())
            {
                int8_t power = data["rightMotorPower"].as<int8_t>();
                setRightMotorPower(power);
            }
        }
        notifyClients();
    }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len)
{
    switch (type)
    {
    case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        break;
    case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
    case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
        break;
    }
}

void initWebSocket()
{
    ws.onEvent(onEvent);
    server.addHandler(&ws);
}

String processor(const String &var)
{
    Serial.println(var);
    if (var == "STATE")
    {
        if (ledState)
        {
            return "ON";
        }
        else
        {
            return "OFF";
        }
    }
}

void setup()
{
    // Serial port for debugging purposes
    Serial.begin(115200);

    ledcSetup(LEFT_MOTOR_FWD_CHANNEL, 10000, 8);
    ledcSetup(LEFT_MOTOR_REV_CHANNEL, 10000, 8);
    ledcSetup(RIGHT_MOTOR_FWD_CHANNEL, 10000, 8);
    ledcSetup(RIGHT_MOTOR_REV_CHANNEL, 10000, 8);

    ledcAttachPin(LEFT_MOTOR_FWD_PIN, LEFT_MOTOR_FWD_CHANNEL);
    ledcAttachPin(LEFT_MOTOR_REV_PIN, LEFT_MOTOR_REV_CHANNEL);
    ledcAttachPin(RIGHT_MOTOR_FWD_PIN, RIGHT_MOTOR_FWD_CHANNEL);
    ledcAttachPin(RIGHT_MOTOR_REV_PIN, RIGHT_MOTOR_REV_CHANNEL);

    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);

    // Connect to Wi-Fi
    WiFi.softAP(ssid, password);
    WiFi.softAPConfig(local_ip, gateway, subnet);
    delay(100);

    // Print ESP Local IP Address
    Serial.println(WiFi.localIP());

    initWebSocket();

    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", index_html, processor);
    });

    // Start server
    server.begin();
}

void loop()
{
    ws.cleanupClients();
    digitalWrite(ledPin, ledState);
}
