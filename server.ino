
void setup_server()
{
  DBG_OUTPUT_PORT.begin(115200);
  DBG_OUTPUT_PORT.print("\n");
  DBG_OUTPUT_PORT.setDebugOutput(false);
  SPIFFS.begin();
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      DBG_OUTPUT_PORT.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    DBG_OUTPUT_PORT.printf("\n");
  }

  DBG_OUTPUT_PORT.println("Reading configuration");
  load_config();

  //WIFI INIT
  DBG_OUTPUT_PORT.print("Connecting to ");
  DBG_OUTPUT_PORT.println(wifi_ssid);

  WiFi.disconnect();
  delay(500);
  WiFi.mode(WIFI_STA);
  WiFi.hostname(wifi_host);
  WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());

  uint8_t wifi_attempts = 0;
  while ( WiFi.status() != WL_CONNECTED && wifi_attempts < 10 )
  {
    delay(500);
    DBG_OUTPUT_PORT.print(".");
    wifi_attempts ++;
  }
  if ( WiFi.status() != WL_CONNECTED )
  {
    DBG_OUTPUT_PORT.println("");
    DBG_OUTPUT_PORT.println("Unable to connect to WiFi");
    DBG_OUTPUT_PORT.println("Starting AP!");

    WiFi.disconnect();
    WiFi.mode(WIFI_AP);

    IPAddress ip(192, 168, 5, 1);
    IPAddress mask(255, 255, 255, 0);
    WiFi.softAPConfig(ip, ip, mask);
    WiFi.softAP(String(wifi_host + "_esp").c_str());

    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);
  }
  else
  {
    DBG_OUTPUT_PORT.println("");
    DBG_OUTPUT_PORT.print("Connected! IP address: ");
    DBG_OUTPUT_PORT.println(WiFi.localIP());

    MDNS.begin(wifi_host.c_str());
  }
  DBG_OUTPUT_PORT.print("Open http://");
  DBG_OUTPUT_PORT.print(wifi_host);
  DBG_OUTPUT_PORT.println(".local/edit to see the file browser");

  //SERVER INIT
  //list directory
  server.on("/list", HTTP_GET, handleFileList);
  //load editor
  server.on("/edit", HTTP_GET, [](){
    if(!handleFileRead("/edit.html")) server.send(404, "text/plain", "FileNotFound");
  });
  //create file
  server.on("/edit", HTTP_PUT, handleFileCreate);
  //delete file
  server.on("/edit", HTTP_DELETE, handleFileDelete);
  //first callback is called after the request has ended with all parsed arguments
  //second callback handles file uploads at that location
  server.on("/edit", HTTP_POST, [](){ server.send(200, "text/plain", ""); }, handleFileUpload);

  server.on("/config", HTTP_POST, [](){
    int arg_count = server.args();

    if ( server.hasArg("wifi_ssid") )
    {
      wifi_ssid = server.arg("wifi_ssid");
    }
    if ( server.hasArg("wifi_password") )
    {
      wifi_password = server.arg("wifi_password");
    }
    if ( server.hasArg("wifi_host") )
    {
      wifi_host = server.arg("wifi_host");
    }
    String json = "{";
    json += "\"arg_count\":\"" + String(arg_count) + "\"";
    json += "}";
    server.send(200, "text/json", json);

    DBG_OUTPUT_PORT.println("HTTP server started");

    save_config();
  });

  server.on("/config", HTTP_GET, [](){
    String json = "{";
    json += "\"wifi_ssid\":\"" + String(wifi_ssid) + "\"";
    json += ",\"wifi_password\":\"" + String(wifi_password) + "\"";
    json += ",\"wifi_host\":\"" + String(wifi_host) + "\"";
    json += ",\"min_temp\":\"" + String(MIN_TEMPERATURE) + "\"";
    json += ",\"max_temp\":\"" + String(MAX_TEMPERATURE) + "\"";
    json += ",\"target_temp\":\"" + String(keezer_target_temperature) + "\"";
    json += ",\"threshold\":\"" + String(keezer_temperature_threshold) + "\"";
    json += "}";
    server.send(200, "text/json", json);

    json = String();
  });

  server.on("/restart", HTTP_GET, [](){
    ESP.restart();
  });

  server.on("/target", HTTP_GET, [](){

    String json = "{";
    json += "\"target_temp\":\"" + String(keezer_target_temperature) + "\"";
    json += "}";
    server.send(200, "text/json", json);

    json = String();
  });

  server.on("/target", HTTP_POST, [](){
    if ( server.hasArg("target_temp") )
    {
        keezer_target_temperature = server.arg("target_temp").toInt();
    }
    if ( keezer_target_temperature > MAX_TEMPERATURE )
    {
        keezer_target_temperature = MAX_TEMPERATURE;
    }
    if ( keezer_target_temperature < MIN_TEMPERATURE )
    {
        keezer_target_temperature = MIN_TEMPERATURE;
    }

    String json = "{";
    json += "\"target_temp\":\"" + String(keezer_target_temperature) + "\"";
    json += "}";
    server.send(200, "text/json", json);

    json = String();

    DBG_OUTPUT_PORT.print("Target set to: ");
    DBG_OUTPUT_PORT.println(keezer_target_temperature);

    save_config();
  });

  server.on("/state", HTTP_GET, [](){
    String json = "{";
    json += "\"heap\":" + String(ESP.getFreeHeap());
    json += ",\"status\":" + String(keezer_state);
    json += ",\"since\":" + String(keezer_timer_last);
    json += ",\"last_reset\":" + String(last_resetReason);
    json += ",\"target_temp\":\"" + String(keezer_target_temperature) + "\"";
    json += ",\"current_temp\":\"" + String(ds_temp_sensor[DS_KEEZER].tempF) + "\"";
    json += ",\"uptime\":" + String(millis());
    json += "}";
    server.send(200, "text/json", json);
    json = String();
  });

  // serve static files with 24hr max-age control
  #ifndef DEBUG_SERVER
    server.serveStatic("/", SPIFFS, "/", "max-age=86400");
  #else
    server.serveStatic("/normalize.css", SPIFFS, "/normalize.css", "max-age=86400");
    server.serveStatic("/skeleton.css", SPIFFS, "/skeleton.css", "max-age=86400");
    server.serveStatic("/edit.html", SPIFFS, "/edit.html", "max-age=86400");
  #endif

  //called when the url is not defined here
  //use it to load content from SPIFFS
  server.onNotFound([](){
    if( !handleFileRead(server.uri()) )
    {
      server.send(404, "text/plain", "FileNotFound");
    }
  });

  server.begin();
  DBG_OUTPUT_PORT.println("HTTP server started");
}

boolean load_config()
{
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    DBG_OUTPUT_PORT.println("Failed to open config file");
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    DBG_OUTPUT_PORT.println("Config file size is too large");
    return false;
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(buf.get(), size);

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());

  if (!json.success()) {
    DBG_OUTPUT_PORT.println("Failed to parse config file");
    return false;
  }

  if (json.containsKey("wifi_ssid"))
  {
    wifi_ssid = json["wifi_ssid"].asString();
  }
  if (json.containsKey("wifi_password"))
  {
    wifi_password = json["wifi_password"].asString();
  }
  if (json.containsKey("wifi_host"))
  {
    wifi_host = json["wifi_host"].asString();
  }

  if (json.containsKey("target_temp"))
  {
    keezer_target_temperature = json["target_temp"];
  }
  if (json.containsKey("threshold"))
  {
    keezer_temperature_threshold = json["threshold"];
  }

  DBG_OUTPUT_PORT.print("Loaded wifi_ssid: ");
  DBG_OUTPUT_PORT.println(wifi_ssid);
  DBG_OUTPUT_PORT.print("Loaded wifi_host: ");
  DBG_OUTPUT_PORT.println(wifi_host);

  return true;
}

bool save_config() {
  DBG_OUTPUT_PORT.println("Saving Configuration");

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["wifi_ssid"] = wifi_ssid;
  json["wifi_password"] = wifi_password;
  json["wifi_host"] = wifi_host;
  json["target_temp"] = keezer_target_temperature;
  json["threshold"] = keezer_temperature_threshold;

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    DBG_OUTPUT_PORT.println("Failed to open config file for writing");
    return false;
  }

  json.printTo(configFile);
  return true;
}
