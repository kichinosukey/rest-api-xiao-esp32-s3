#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <ArduinoJson.h>

// WiFi設定
const char *ssid = "SSID";         // あなたのWiFiネットワーク名に変更
const char *password = "PASSWORD"; // あなたのWiFiパスワードに変更

// DHT11センサー設定
#define DHTPIN 1      // DHT11のデータピンをD1(GPIO1)に接続
#define DHTTYPE DHT11 // DHT 11センサーを使用

DHT dht(DHTPIN, DHTTYPE);
WebServer server(80); // ポート80でWebサーバーを作成

// 最新の読み取り値を保存する変数
float temperature = 0.0;
float humidity = 0.0;
unsigned long lastReadTime = 0;
const unsigned long readInterval = 2000; // 2秒ごとに読み取り

void setup()
{
    // シリアル通信開始
    Serial.begin(115200);
    delay(1000); // シリアルポートが安定するまで待機

    Serial.println("\n\n=== ESP32S3 DHT11 APIサーバー ===");
    Serial.println("初期化を開始します...");

    // DHT11センサーの初期化
    dht.begin();
    Serial.println("DHT11センサーを初期化しました");

    // WiFi接続
    initWiFi();

    // APIエンドポイントの設定
    setupAPI();

    // サーバー開始
    server.begin();
    Serial.println("HTTPサーバーを起動しました");
    Serial.println("準備完了！DHT11センサーからのデータ取得を開始します");
}

void loop()
{
    unsigned long currentMillis = millis();

    // 一定間隔でセンサーから値を読み取る
    if (currentMillis - lastReadTime >= readInterval)
    {
        readSensorData();
        lastReadTime = currentMillis;
    }

    // クライアントリクエストを処理
    server.handleClient();
}

void initWiFi()
{
    Serial.println("\nWiFi接続を開始します...");
    Serial.print("接続先SSID: ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA); // ステーションモードに設定
    WiFi.disconnect();   // 以前の接続をクリア
    delay(100);

    // 利用可能なネットワークのスキャン
    Serial.println("周囲のWiFiネットワークをスキャンしています...");
    int n = WiFi.scanNetworks();
    if (n == 0)
    {
        Serial.println("ネットワークが見つかりませんでした");
    }
    else
    {
        Serial.print(n);
        Serial.println(" 個のネットワークが見つかりました:");
        for (int i = 0; i < n; ++i)
        {
            Serial.print("  ");
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(WiFi.SSID(i));
            Serial.print(" (");
            Serial.print(WiFi.RSSI(i));
            Serial.print(" dBm) ");
            Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "[オープン]" : "[保護]");
            delay(10);
        }
        Serial.println("");
    }

    // WiFiへの接続を開始
    Serial.print("WiFiに接続しています...");

    WiFi.begin(ssid, password);

    // 接続待機（タイムアウト付き）
    int timeout_counter = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
        timeout_counter++;

        // 10秒ごとに改行を入れる
        if (timeout_counter % 20 == 0)
        {
            Serial.println("");
        }

        // 30秒後にタイムアウト
        if (timeout_counter >= 60)
        {
            Serial.println("\n\nWiFi接続がタイムアウトしました！");
            Serial.println("以下を確認してください:");
            Serial.println("1. SSIDとパスワードが正しいか");
            Serial.println("2. WiFiルーターが稼働しているか");
            Serial.println("3. ESP32S3がWiFiの範囲内にあるか");
            Serial.println("4. アンテナが正しく接続されているか");
            Serial.println("\n10秒後に再起動します...");
            delay(10000);
            ESP.restart();
        }
    }

    // 接続成功
    Serial.println("\n\nWiFiに接続しました！");
    Serial.println("================================");
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    Serial.print("IPアドレス: ");
    Serial.println(WiFi.localIP());
    Serial.print("サブネットマスク: ");
    Serial.println(WiFi.subnetMask());
    Serial.print("ゲートウェイIPアドレス: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("信号強度: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    Serial.println("================================");
}

void readSensorData()
{
    // DHT11センサーからの読み取り
    float newT = dht.readTemperature();
    float newH = dht.readHumidity();

    // 値が有効であれば更新
    if (!isnan(newT) && !isnan(newH))
    {
        temperature = newT;
        humidity = newH;

        Serial.println("センサー読み取り成功:");
        Serial.print("  温度: ");
        Serial.print(temperature);
        Serial.println(" °C");
        Serial.print("  湿度: ");
        Serial.print(humidity);
        Serial.println(" %");
    }
    else
    {
        Serial.println("センサーからの読み取りに失敗しました！接続を確認してください");
    }
}

void setupAPI()
{
    // JSONデータを返すAPIエンドポイント
    server.on("/api/sensor", HTTP_GET, []()
              {
    DynamicJsonDocument jsonDoc(200);
    jsonDoc["temperature"] = temperature;
    jsonDoc["humidity"] = humidity;
    jsonDoc["timestamp"] = millis();
    
    String jsonResponse;
    serializeJson(jsonDoc, jsonResponse);
    
    server.send(200, "application/json", jsonResponse);
    Serial.println("APIリクエスト処理: /api/sensor"); });

    // シンプルなHTMLページを提供するルート
    server.on("/", HTTP_GET, []()
              {
    String html = "<html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<title>ESP32S3 DHT11 センサー</title>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; margin: 20px; }";
    html += ".container { max-width: 800px; margin: 0 auto; }";
    html += ".card { background-color: #f9f9f9; border-radius: 10px; padding: 20px; margin-bottom: 20px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); }";
    html += "h1 { color: #333; }";
    html += ".data { font-size: 24px; margin: 10px 0; }";
    html += ".temp { color: #e74c3c; }";
    html += ".humid { color: #3498db; }";
    html += ".button { background-color: #4CAF50; border: none; color: white; padding: 10px 20px; text-align: center; text-decoration: none; display: inline-block; font-size: 16px; margin: 4px 2px; cursor: pointer; border-radius: 5px; }";
    html += "</style>";
    html += "</head><body>";
    html += "<div class='container'>";
    html += "<h1>ESP32S3 DHT11センサーAPI</h1>";
    html += "<div class='card'>";
    html += "<div class='data'>温度: <span id='temp' class='temp'>--</span> °C</div>";
    html += "<div class='data'>湿度: <span id='humid' class='humid'>--</span> %</div>";
    html += "<div class='data'>更新時間: <span id='time'>--</span></div>";
    html += "<button class='button' onclick='fetchData()'>更新</button>";
    html += "</div>";
    html += "<div class='card'>";
    html += "<h2>API情報</h2>";
    html += "<p>センサーデータにアクセスするには、以下のエンドポイントを使用してください:</p>";
    html += "<code>/api/sensor</code> - JSONフォーマットでセンサーデータを返します";
    html += "</div>";
    html += "</div>";
    html += "<script>";
    html += "function fetchData() {";
    html += "  fetch('/api/sensor')";
    html += "    .then(response => response.json())";
    html += "    .then(data => {";
    html += "      document.getElementById('temp').textContent = data.temperature.toFixed(1);";
    html += "      document.getElementById('humid').textContent = data.humidity.toFixed(1);";
    html += "      const date = new Date();";
    html += "      document.getElementById('time').textContent = date.toLocaleTimeString();";
    html += "    })";
    html += "    .catch(error => console.error('Error:', error));";
    html += "}";
    html += "fetchData();";
    html += "setInterval(fetchData, 5000);";
    html += "</script>";
    html += "</body></html>";
    
    server.send(200, "text/html", html);
    Serial.println("ウェブページリクエスト処理: /"); });

    // システム情報を返すAPIエンドポイント
    server.on("/api/system", HTTP_GET, []()
              {
    DynamicJsonDocument jsonDoc(400);
    jsonDoc["uptime"] = millis() / 1000;
    jsonDoc["heap_free"] = ESP.getFreeHeap();
    jsonDoc["wifi_rssi"] = WiFi.RSSI();
    jsonDoc["ip"] = WiFi.localIP().toString();
    jsonDoc["mac"] = WiFi.macAddress();
    
    String jsonResponse;
    serializeJson(jsonDoc, jsonResponse);
    
    server.send(200, "application/json", jsonResponse);
    Serial.println("APIリクエスト処理: /api/system"); });

    // 404ページ
    server.onNotFound([]()
                      {
    server.send(404, "text/plain", "ページが見つかりません");
    Serial.println("404エラー: リクエストされたページが見つかりません"); });

    Serial.println("APIエンドポイントを設定しました:");
    Serial.println("  /api/sensor - 温度・湿度データ (JSON)");
    Serial.println("  /api/system - システム情報 (JSON)");
    Serial.println("  /          - ウェブインターフェース (HTML)");
}