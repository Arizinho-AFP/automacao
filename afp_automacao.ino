// ============================================================
// AFP Sistemas — Firmware Genérico ESP32
// Sistema de Automação Comercial
// ============================================================
// BIBLIOTECAS NECESSÁRIAS (instale pelo Library Manager):
//   - Adafruit MCP23X17 (by Adafruit)
//   - ArduinoJson (by Benoit Blanchon)
//   - WebSocketsServer (by Links2004)
//   - WiFiManager (by tzapu)
//   - NTPClient (by Fabrice Weinberg)
//   - LiquidCrystal I2C (by Frank de Brabander)
//   - Espalexa (by Christian Ehrnsperger) ← só se USE_ALEXA = true
// ============================================================

// ============================================================
//  ██████╗ ██████╗ ███╗   ██╗███████╗██╗ ██████╗
// ██╔════╝██╔═══██╗████╗  ██║██╔════╝██║██╔════╝
// ██║     ██║   ██║██╔██╗ ██║█████╗  ██║██║  ███╗
// ██║     ██║   ██║██║╚██╗██║██╔══╝  ██║██║   ██║
// ╚██████╗╚██████╔╝██║ ╚████║██║     ██║╚██████╔╝
//  ╚═════╝ ╚═════╝ ╚═╝  ╚═══╝╚═╝     ╚═╝ ╚═════╝
//
// ▶ EDITE APENAS ESTE BLOCO PARA CADA NOVO CLIENTE
// ============================================================

// Nome exibido no LCD e nos logs
#define CLIENT_NAME   "Demo AFP"

// Slug do cliente — deve ser igual ao cadastrado no api.php
#define CLIENT_SLUG   "demo"

// Chave de autenticação da API — deve ser igual à api_key no api.php
#define API_KEY       "afp-demo-k7x9m2p3"

// URL do servidor (sem https:// e sem barra final)
#define API_HOST      "afpsistemas.com.br"

// Caminho da API no servidor
#define API_PATH      "/automacao/api.php"

// PIN de acesso ao painel local (6 dígitos)
#define PIN_LOCAL     "123456"

// IP fixo do ESP32 na rede do cliente
#define IP_LOCAL      "192.168.1.150"
#define IP_GATEWAY    "192.168.1.1"
#define IP_SUBNET     "255.255.255.0"
#define IP_DNS        "8.8.8.8"

// Habilitar integração com Alexa? (true/false)
#define USE_ALEXA     false

// ============================================================
// DISPOSITIVOS DO CLIENTE
// pin < 16  → pino físico do MCP23017
// pin >= 100 → Sonoff remoto (índice no array SONOFF_IPS)
// ============================================================
struct Device {
    const char* id;   // ID usado na API e no painel
    const char* nome; // Nome para Alexa (se USE_ALEXA = true)
    int pin;
};

Device devices[] = {
    // ── MCP23017 (pinos 0-15) ──────────────────────────────
    { "luz_01",  "Luz Um",        0  },
    { "luz_02",  "Luz Dois",      1  },
    { "luz_03",  "Luz Três",      2  },
    { "luz_04",  "Luz Quatro",    3  },
    { "luz_05",  "Luz Cinco",     4  },
    { "luz_06",  "Luz Seis",      5  },
    { "luz_07",  "Luz Sete",      6  },
    { "vent_01", "Vento Um",      7  },
    { "vent_02", "Vento Dois",    8  },
    { "vent_03", "Vento Três",    9  },
    { "vent_04", "Vento Quatro",  10 },
    { "vent_05", "Vento Cinco",   11 },
    { "vent_06", "Vento Seis",    12 },
    { "vent_07", "Vento Sete",    13 },
    { "vent_08", "Vento Oito",    14 },
    { "vent_09", "Vento Nove",    15 },

    // ── Sonoffs (pinos 100+) ───────────────────────────────
    // Índice = pin - 100 → posição no array SONOFF_IPS
    { "refletor_dir", "Refletor Direito",   100 },
    { "refletor_esq", "Refletor Esquerdo",  101 },
    { "logo",         "Logo",               102 },
    { "fachada",      "Fachada",            103 },
    { "ar_01",        "Ar Um",              104 },
    { "ar_02",        "Ar Dois",            105 },
    { "ar_03",        "Ar Três",            106 },
    { "ar_04",        "Ar Quatro",          107 },
    { "ar_05",        "Ar Cinco",           108 },
};

// ── IPs dos Sonoffs ────────────────────────────────────────
// Ordem deve corresponder aos pinos 100, 101, 102...
const char* SONOFF_IPS[] = {
    "192.168.1.201",  // índice 0 → pino 100 (refletor_dir)
    "192.168.1.202",  // índice 1 → pino 101 (refletor_esq)
    "192.168.1.203",  // índice 2 → pino 102 (logo)
    "192.168.1.204",  // índice 3 → pino 103 (fachada)
    "192.168.1.205",  // índice 4 → pino 104 (ar_01)
    "192.168.1.206",  // índice 5 → pino 105 (ar_02)
    "192.168.1.207",  // índice 6 → pino 106 (ar_03)
    "192.168.1.208",  // índice 7 → pino 107 (ar_04)
    "192.168.1.209",  // índice 8 → pino 108 (ar_05)
};

// ============================================================
// FIM DO BLOCO DE CONFIGURAÇÃO
// ============================================================

// ── INCLUDES ───────────────────────────────────────────────
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_MCP23X17.h>
#include <LittleFS.h>
#include <ArduinoOTA.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <WebSocketsServer.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <LiquidCrystal_I2C.h>

#if USE_ALEXA
  #define ESPALEXA_MAXDEVICES 35
  #include <Espalexa.h>
  Espalexa espalexa;
#endif

// ── CONSTANTES ─────────────────────────────────────────────
#define I2C_SDA     21
#define I2C_SCL     22
#define NUM_DEVICES (sizeof(devices) / sizeof(devices[0]))
#define NUM_SONOFFS (sizeof(SONOFF_IPS) / sizeof(SONOFF_IPS[0]))

// ── OBJETOS GLOBAIS ────────────────────────────────────────
WebServer          server(80);
WebSocketsServer   webSocket(81);
Adafruit_MCP23X17  mcp;
LiquidCrystal_I2C  lcd(0x27, 16, 2);
WiFiUDP            ntpUDP;
NTPClient          timeClient(ntpUDP, "pool.ntp.org", -10800, 60000);

// ── ESTADO ─────────────────────────────────────────────────
int   virtualStates[NUM_SONOFFS] = {0};
unsigned long timers[50]         = {0};
char  pin_correto[7];
bool  shouldSaveConfig = false;

// ── PROTÓTIPOS ─────────────────────────────────────────────
void saveStates();
void broadcastUpdate();
void updateCloud(String id, int state);
void uploadScheduleToCloud(String json);
bool handleFileRead(String path);

// ============================================================
// HARDWARE — MCP23017 + Sonoffs
// ============================================================

void setDeviceState(int pin, bool on) {
    if (pin < 16) {
        mcp.digitalWrite(pin, on ? LOW : HIGH);
    } else if (pin >= 100) {
        int idx = pin - 100;
        if (idx < 0 || idx >= (int)NUM_SONOFFS) return;
        if (WiFi.status() == WL_CONNECTED) {
            HTTPClient http;
            String url = "http://" + String(SONOFF_IPS[idx]) + "/comando?estado=" + (on ? "on" : "off");
            http.setTimeout(300);
            http.begin(url);
            http.GET();
            http.end();
        }
        virtualStates[pin - 100] = on ? 1 : 0;
    }
}

int getDeviceState(int pin) {
    if (pin < 16) return (mcp.digitalRead(pin) == LOW) ? 1 : 0;
    if (pin >= 100) {
        int idx = pin - 100;
        if (idx >= 0 && idx < (int)NUM_SONOFFS) return virtualStates[idx];
    }
    return 0;
}

// ============================================================
// GRUPOS — mapeia prefixo para lista de índices
// ============================================================

void toggleGroup(const String& prefix, bool on) {
    for (int i = 0; i < (int)NUM_DEVICES; i++) {
        if (String(devices[i].id).startsWith(prefix)) {
            setDeviceState(devices[i].pin, on);
            timers[i] = 0;
        }
    }
}

// ============================================================
// JSON DE ESTADOS
// ============================================================

String getStatusJson() {
    String json = "{";
    for (int i = 0; i < (int)NUM_DEVICES; i++) {
        json += "\"" + String(devices[i].id) + "\":" + getDeviceState(devices[i].pin);
        if (i < (int)NUM_DEVICES - 1) json += ",";
    }
    json += "}";
    return json;
}

void broadcastUpdate() {
    webSocket.broadcastTXT(getStatusJson());
}

// ============================================================
// PERSISTÊNCIA — LittleFS
// ============================================================

void saveStates() {
    JsonDocument doc;
    for (int i = 0; i < (int)NUM_DEVICES; i++)
        doc[devices[i].id] = getDeviceState(devices[i].pin);
    File f = LittleFS.open("/states.json", "w");
    if (f) { serializeJson(doc, f); f.close(); }
}

void loadLastStates() {
    if (!LittleFS.exists("/states.json")) return;
    File f = LittleFS.open("/states.json", "r");
    if (!f) return;
    JsonDocument doc;
    if (!deserializeJson(doc, f)) {
        for (int i = 0; i < (int)NUM_DEVICES; i++) {
            if (doc.containsKey(devices[i].id)) {
                bool on = doc[devices[i].id] == 1;
                setDeviceState(devices[i].pin, on);
                if (devices[i].pin >= 100)
                    virtualStates[devices[i].pin - 100] = on ? 1 : 0;
            }
        }
    }
    f.close();
}

// ============================================================
// NUVEM — Hostgator
// ============================================================

String buildUrl(String rota = "estados") {
    return "https://" + String(API_HOST) + String(API_PATH) +
           "?key=" + String(API_KEY) + "&rota=" + rota;
}

void updateCloud(String id, int state) {
    if (WiFi.status() != WL_CONNECTED) return;
    WiFiClientSecure client; client.setInsecure();
    HTTPClient http;
    if (!http.begin(client, buildUrl("estados"))) return;
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.setTimeout(2000);
    http.POST("id=" + id + "&estado=" + String(state));
    http.end();
}

void uploadScheduleToCloud(String json) {
    if (WiFi.status() != WL_CONNECTED) return;
    WiFiClientSecure client; client.setInsecure();
    HTTPClient http;
    if (!http.begin(client, buildUrl("agenda"))) return;
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(3000);
    http.POST(json);
    http.end();
}

// ── Sincroniza estados com nuvem (a cada 15s) ──────────────
unsigned long lastCloudCheck = 0;
void checkCloud() {
    if (millis() - lastCloudCheck < 15000) return;
    lastCloudCheck = millis();
    if (WiFi.status() != WL_CONNECTED) return;

    WiFiClientSecure client; client.setInsecure();
    HTTPClient http;
    if (!http.begin(client, buildUrl("estados"))) return;
    http.setTimeout(2000);
    int code = http.GET();
    if (code == 200) {
        JsonDocument doc;
        if (!deserializeJson(doc, http.getString())) {
            bool changed = false;
            for (int i = 0; i < (int)NUM_DEVICES; i++) {
                if (doc.containsKey(devices[i].id)) {
                    int cloudState = doc[devices[i].id];
                    if (cloudState != getDeviceState(devices[i].pin)) {
                        setDeviceState(devices[i].pin, cloudState == 1);
                        timers[i] = 0;
                        changed = true;
                    }
                }
            }
            if (changed) { saveStates(); broadcastUpdate(); }
        }
    }
    http.end();
}

// ── Sincroniza agenda com nuvem (a cada 5min) ──────────────
unsigned long lastAgendaSync = 0;
void syncCloudSchedule() {
    if (millis() - lastAgendaSync < 300000) return;
    lastAgendaSync = millis();
    if (WiFi.status() != WL_CONNECTED) return;

    WiFiClientSecure client; client.setInsecure();
    HTTPClient http;
    if (!http.begin(client, buildUrl("agenda"))) return;
    http.setTimeout(3000);
    if (http.GET() == 200) {
        String payload = http.getString();
        if (payload.startsWith("[") && payload.length() > 2) {
            File f = LittleFS.open("/schedules.json", "w");
            if (f) { f.print(payload); f.close(); }
        }
    }
    http.end();
}

// ============================================================
// AGENDAMENTOS
// ============================================================

unsigned long lastScheduleCheck = 0;
void checkSchedules() {
    if (millis() - lastScheduleCheck < 30000) return;
    lastScheduleCheck = millis();
    timeClient.update();

    int cDay  = timeClient.getDay();
    int cHour = timeClient.getHours();
    int cMin  = timeClient.getMinutes();

    if (!LittleFS.exists("/schedules.json")) return;
    File f = LittleFS.open("/schedules.json", "r");
    if (!f) return;

    JsonDocument doc;
    if (deserializeJson(doc, f)) { f.close(); return; }
    f.close();

    JsonArray arr = doc.as<JsonArray>();
    bool changed  = false;
    bool resave   = false;
    JsonDocument newDoc;
    JsonArray    newArr = newDoc.to<JsonArray>();

    for (JsonVariant v : arr) {
        int  d       = v["d"];
        int  h       = v["h"];
        int  m       = v["m"];
        bool isOnce  = v["once"] | false;
        bool keep    = true;

        if ((d == -1 || d == cDay) && h == cHour && m == cMin) {
            const char* id  = v["id"];
            int  target     = v["s"];
            for (int i = 0; i < (int)NUM_DEVICES; i++) {
                if (String(devices[i].id) == String(id)) {
                    if (getDeviceState(devices[i].pin) != target) {
                        setDeviceState(devices[i].pin, target == 1);
                        timers[i] = 0;
                        updateCloud(id, target);
                        changed = true;
                    }
                }
            }
            if (isOnce) { keep = false; resave = true; }
        }
        if (keep) newArr.add(v);
    }

    if (changed) { saveStates(); broadcastUpdate(); }

    if (resave) {
        File fw = LittleFS.open("/schedules.json", "w");
        if (fw) { serializeJson(newArr, fw); fw.close(); }
        String out; serializeJson(newArr, out);
        uploadScheduleToCloud(out);
    }
}

// ============================================================
// TIMERS
// ============================================================

void checkTimers() {
    unsigned long now = millis();
    bool changed = false;
    for (int i = 0; i < (int)NUM_DEVICES; i++) {
        if (timers[i] > 0 && now >= timers[i]) {
            setDeviceState(devices[i].pin, false);
            timers[i] = 0;
            updateCloud(devices[i].id, 0);
            changed = true;
        }
    }
    if (changed) { saveStates(); broadcastUpdate(); }
}

// ============================================================
// AUTENTICAÇÃO LOCAL
// ============================================================

bool isAuthenticated() {
    if (!server.hasHeader("Cookie")) return false;
    String cookie = server.header("Cookie");
    return cookie.indexOf("auth=true") != -1;
}

// ============================================================
// HANDLERS HTTP
// ============================================================

void handleLogin() {
    if (server.hasArg("pin") && server.arg("pin") == String(pin_correto)) {
        server.sendHeader("Set-Cookie", "auth=true; Path=/; Max-Age=31536000");
        server.send(200, "text/plain", "OK");
    } else {
        server.send(401, "text/plain", "PIN Incorreto");
    }
}

void handleLogout() {
    server.sendHeader("Set-Cookie", "auth=false; Path=/; Max-Age=-1");
    server.sendHeader("Location", "/index.html");
    server.send(302, "text/plain", "Deslogado");
}

void handleComando() {
    if (!isAuthenticated()) { server.send(401, "text/plain", "Nao Autorizado"); return; }

    String deviceId = server.arg("id");
    String state    = server.arg("estado");
    String timerStr = server.arg("timer");
    bool   on       = (state == "on" || state == "1");

    for (int i = 0; i < (int)NUM_DEVICES; i++) {
        if (String(devices[i].id) == deviceId) {
            setDeviceState(devices[i].pin, on);
            unsigned long dur = timerStr.toInt();
            timers[i] = (on && dur > 0) ? millis() + dur * 60000UL : 0;
            saveStates();
            broadcastUpdate();
            server.sendHeader("Access-Control-Allow-Origin", "*");
            server.send(200, "text/plain", "OK");
            updateCloud(deviceId, on ? 1 : 0);
            return;
        }
    }
    server.send(400, "text/plain", "Dispositivo nao encontrado");
}

void handleComandoGrupo() {
    if (!isAuthenticated()) { server.send(401, "text/plain", "Nao Autorizado"); return; }

    String group   = server.arg("grupo");
    String state   = server.arg("estado");
    String timerStr = server.arg("timer");
    bool   on      = (state == "on" || state == "1");

    // Mapeia grupo para prefixo de ID
    String prefix = "";
    if      (group == "luzes")        prefix = "luz_";
    else if (group == "ventiladores") prefix = "vent_";
    else if (group == "ares")         prefix = "ar_";
    else if (group == "externos")     prefix = "refletor_";
    else if (group == "tomadas")      prefix = "tomada_";
    else { server.send(400, "text/plain", "Grupo invalido"); return; }

    unsigned long dur    = timerStr.toInt();
    unsigned long tMillis = (on && dur > 0) ? millis() + dur * 60000UL : 0;

    for (int i = 0; i < (int)NUM_DEVICES; i++) {
        String devId = String(devices[i].id);
        // Externos: inclui refletor_ e também logo, fachada
        bool match = devId.startsWith(prefix);
        if (group == "externos" && (devId == "logo" || devId == "fachada")) match = true;

        if (match) {
            setDeviceState(devices[i].pin, on);
            timers[i] = tMillis;
        }
    }

    saveStates();
    broadcastUpdate();
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", "OK");
    updateCloud(group, on ? 1 : 0);
}

void handleGetSchedule() {
    if (!isAuthenticated()) { server.send(401, "text/plain", "Nao Autorizado"); return; }
    if (LittleFS.exists("/schedules.json")) {
        File f = LittleFS.open("/schedules.json", "r");
        server.streamFile(f, "application/json");
        f.close();
    } else {
        server.send(200, "application/json", "[]");
    }
}

void handleSaveSchedule() {
    if (!isAuthenticated()) { server.send(401, "text/plain", "Nao Autorizado"); return; }
    if (server.hasArg("plain")) {
        String body = server.arg("plain");
        File f = LittleFS.open("/schedules.json", "w");
        if (f) {
            f.print(body); f.close();
            uploadScheduleToCloud(body);
            server.send(200, "text/plain", "OK");
        } else {
            server.send(500, "text/plain", "Erro ao salvar");
        }
    }
}

bool handleFileRead(String path) {
    if (path.endsWith("/")) path = "/index.html";
    String type = "text/plain";
    if      (path.endsWith(".html")) type = "text/html";
    else if (path.endsWith(".css"))  type = "text/css";
    else if (path.endsWith(".js"))   type = "application/javascript";
    else if (path.endsWith(".png"))  type = "image/png";
    else if (path.endsWith(".json")) type = "application/json";

    if (LittleFS.exists(path)) {
        File f = LittleFS.open(path, "r");
        bool isDynamic = path.endsWith("states.json") || path.endsWith("schedules.json");
        server.sendHeader("Cache-Control", isDynamic ? "no-cache" : "public, max-age=31536000");
        server.streamFile(f, type);
        f.close();
        return true;
    }
    return false;
}

// ============================================================
// WEBSOCKET
// ============================================================

void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    if (type == WStype_CONNECTED) webSocket.sendTXT(num, getStatusJson());
}

// ============================================================
// ALEXA — callbacks individuais
// ============================================================

#if USE_ALEXA
void makeAlexaCallback(int deviceIndex, uint8_t brightness) {
    bool on = (brightness > 0);
    setDeviceState(devices[deviceIndex].pin, on);
    timers[deviceIndex] = 0;
    saveStates();
    broadcastUpdate();
}

// Callbacks gerados dinamicamente — um por dispositivo
// Limitação do Espalexa: precisa de função separada por device
// Usamos uma abordagem com array de lambdas via wrapper struct
struct AlexaCallback {
    int index;
    static AlexaCallback* instances[35];
    static int count;

    static void registerAll() {
        // Registra até 35 dispositivos no Espalexa
        for (int i = 0; i < (int)NUM_DEVICES && i < 35; i++) {
            instances[i] = new AlexaCallback{i};
        }
    }
};
AlexaCallback* AlexaCallback::instances[35] = {nullptr};
int AlexaCallback::count = 0;
#endif

// ============================================================
// SETUP
// ============================================================

void setup() {
    Serial.begin(115200);
    setCpuFrequencyMhz(240);
    Wire.begin(I2C_SDA, I2C_SCL);

    // LCD
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("AFP Sistemas");
    lcd.setCursor(0,1); lcd.print(CLIENT_NAME);
    delay(2000);

    // LittleFS
    lcd.clear(); lcd.print("Iniciando...");
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS falhou");
    } else {
        // Carrega PIN salvo (WiFiManager pode ter alterado)
        if (LittleFS.exists("/config.json")) {
            File f = LittleFS.open("/config.json", "r");
            if (f) {
                JsonDocument d;
                if (!deserializeJson(d, f)) {
                    const char* savedPin = d["pin"];
                    if (savedPin) strncpy(pin_correto, savedPin, 6);
                }
                f.close();
            }
        } else {
            strncpy(pin_correto, PIN_LOCAL, 6);
            pin_correto[6] = '\0';
        }
    }

    // MCP23017
    if (!mcp.begin_I2C()) {
        lcd.clear(); lcd.print("Erro MCP23017!");
        Serial.println("MCP23017 não encontrado");
        while(1) delay(1000);
    }
    for (int i = 0; i < (int)NUM_DEVICES; i++) {
        if (devices[i].pin < 16) {
            mcp.pinMode(devices[i].pin, OUTPUT);
            mcp.digitalWrite(devices[i].pin, HIGH); // HIGH = relé desligado
        }
        timers[i] = 0;
    }
    loadLastStates();

    // WiFiManager — IP fixo
    WiFiManager wm;
    wm.setSaveConfigCallback([]() { shouldSaveConfig = true; });

    IPAddress ip, gw, sn, dns;
    ip.fromString(IP_LOCAL);
    gw.fromString(IP_GATEWAY);
    sn.fromString(IP_SUBNET);
    dns.fromString(IP_DNS);
    wm.setSTAStaticIPConfig(ip, gw, sn, dns);

    WiFiManagerParameter pinParam("pin", "PIN de Acesso (6 digitos)", pin_correto, 6);
    wm.addParameter(&pinParam);
    wm.setConnectTimeout(30);

    lcd.clear(); lcd.print("Conectando WiFi");
    String apName = "AFP-" + String(CLIENT_NAME);
    apName.replace(" ", "-");
    if (!wm.autoConnect(apName.c_str(), "afp12345")) {
        ESP.restart();
    }

    if (shouldSaveConfig) {
        strncpy(pin_correto, pinParam.getValue(), 6);
        pin_correto[6] = '\0';
        JsonDocument doc; doc["pin"] = pin_correto;
        File f = LittleFS.open("/config.json", "w");
        if (f) { serializeJson(doc, f); f.close(); }
    }

    // LCD — IP
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("Online!");
    lcd.setCursor(0,1); lcd.print(WiFi.localIP());

    // OTA
    ArduinoOTA.setHostname(("AFP-" + String(CLIENT_SLUG)).c_str());
    ArduinoOTA.begin();

    // NTP
    timeClient.begin();

    // Rotas HTTP
    const char* headers[] = {"Cookie"};
    server.collectHeaders(headers, 1);
    server.on("/login",          HTTP_POST, handleLogin);
    server.on("/logout",         HTTP_GET,  handleLogout);
    server.on("/comando",        HTTP_GET,  handleComando);
    server.on("/comando_grupo",  HTTP_GET,  handleComandoGrupo);
    server.on("/agenda",         HTTP_GET,  handleGetSchedule);
    server.on("/agenda",         HTTP_POST, handleSaveSchedule);
    server.on("/status",         HTTP_GET,  []() {
        server.send(200, "application/json", getStatusJson());
    });

    server.onNotFound([]() {
        #if USE_ALEXA
            if (!espalexa.handleAlexaApiCall(server.uri(), server.arg("plain")))
        #endif
        if (!handleFileRead(server.uri()))
            server.send(404, "text/plain", "404");
    });

    // Alexa
    #if USE_ALEXA
        // Grupos
        espalexa.addDevice("Luzes", [](uint8_t b){ toggleGroup("luz_", b>0); saveStates(); broadcastUpdate(); });
        espalexa.addDevice("Ventiladores", [](uint8_t b){ toggleGroup("vent_", b>0); saveStates(); broadcastUpdate(); });
        espalexa.addDevice("Ares", [](uint8_t b){ toggleGroup("ar_", b>0); saveStates(); broadcastUpdate(); });

        // Dispositivos individuais — até 32 restantes
        for (int i = 0; i < (int)NUM_DEVICES && i < 32; i++) {
            int idx = i; // captura por valor
            espalexa.addDevice(devices[i].nome, [idx](uint8_t b){
                setDeviceState(devices[idx].pin, b>0);
                timers[idx] = 0;
                saveStates();
                broadcastUpdate();
            });
        }
        espalexa.begin(&server);
    #else
        server.begin();
    #endif

    // WebSocket
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    Serial.println("AFP Sistemas — " + String(CLIENT_NAME) + " — Online");
}

// ============================================================
// LOOP
// ============================================================

void loop() {
    #if USE_ALEXA
        espalexa.loop();
    #endif

    ArduinoOTA.handle();
    server.handleClient();
    webSocket.loop();
    checkTimers();
    checkCloud();
    checkSchedules();
    syncCloudSchedule();

    yield();
}
