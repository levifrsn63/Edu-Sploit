#include <M5StickCPlus2.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <Preferences.h>
#include <SPIFFS.h>

// Configuration
String ssid = "";
String password = "";
String geminiKey = "";
String deepseekKey = "";
String chatgptKey = "";
String maxTokens = "300";
String knownNetworks = "";

// AP Configuration
String ap_ssid = "Edu-Sploit";
String ap_password = "12345678";
WebServer server(80);

// Menu states
enum MenuState { MAIN_MENU, FILES_MENU, CHAT_MENU, SETTINGS_MENU, WEBUI_MODE, KEYBOARD_INPUT, FILE_VIEW, FILE_CONTENT, MODEL_SELECT };
MenuState currentState = MAIN_MENU;

// Menu navigation
int mainMenuIndex = 0;
int filesMenuIndex = 0;
int chatMenuIndex = 0;
int settingsMenuIndex = 0;
int modelSelectIndex = 0;
int fileListIndex = 0;
int scrollOffset = 0;
int maxScroll = 0;

const char* mainMenuItems[] = {"Files", "Chat", "Settings"};
const char* filesMenuItems[] = {"WebUI", "View Files", "Back"};
const char* chatMenuItems[] = {"New Chat", "Continue", "Back"};

// File list
String fileNames[20];
int fileCount = 0;

// Keyboard
const char keyboard[][10] = {
  {'Q','W','E','R','T','Y','U','I','O','P'},
  {'A','S','D','F','G','H','J','K','L','.'},
  {'Z','X','C','V','B','N','M','!','?',' '},
  {'1','2','3','4','5','6','7','8','9','0'}
};
int keyRow = 0;
int keyCol = 0;
String inputText = "";
bool isInputting = false;

// Chat variables
String lastResponse = "";
String lastTopic = "";
String currentModel = "gemini";
int currentMode = 0;

const char* modeNames[] = {"Direct Q&A", "Blog Post", "Email Writer", "Social Media", "Code Helper", "Summary"};
const char* modePrompts[] = {
  "",
  "Write a detailed and engaging blog post about: ",
  "Write a professional email about: ",
  "Write an engaging social media post about: ",
  "Write clean, well-commented code to: ",
  "Provide a clear and concise summary of: "
};

Preferences preferences;

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  
  M5.Display.setRotation(3);  // 180 degrees
  M5.Display.setTextSize(1);
  M5.Display.fillScreen(BLACK);
  
  Serial.begin(115200);
  
  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
  }
  
  // Load saved settings
  preferences.begin("ai-config", false);
  geminiKey = preferences.getString("geminiKey", geminiKey);
  deepseekKey = preferences.getString("deepseekKey", "");
  chatgptKey = preferences.getString("chatgptKey", "");
  currentModel = preferences.getString("model", "gemini");
  ssid = preferences.getString("ssid", ssid);
  password = preferences.getString("password", password);
  knownNetworks = preferences.getString("knownNetworks", "");
  ap_ssid = preferences.getString("ap_ssid", "Edu-Sploit");
  preferences.end();
  
  // Add current WiFi to known networks if not empty and not already there
  if (ssid.length() > 0 && password.length() > 0) {
    String networkEntry = ssid + ":" + password;
    if (knownNetworks.length() == 0) {
      knownNetworks = networkEntry;
    } else if (knownNetworks.indexOf(networkEntry) == -1) {
      knownNetworks += "\n" + networkEntry;
      preferences.begin("ai-config", false);
      preferences.putString("knownNetworks", knownNetworks);
      preferences.end();
    }
  }
  
  // Try to connect to known networks on boot
  tryAutoConnectWiFi();
  
  drawMainMenu();
}

void loop() {
  M5.update();
  
  // Global C (PWR) long press handler - returns to menu from anywhere
  static unsigned long globalCPress = 0;
  static bool globalCPressed = false;
  
  if (M5.BtnPWR.wasPressed() || M5.BtnC.wasPressed()) {
    globalCPress = millis();
    globalCPressed = true;
  }
  
  if (globalCPressed && (M5.BtnPWR.isPressed() || M5.BtnC.isPressed()) && (millis() - globalCPress > 700)) {
    // Long press C (PWR) = Return to Main Menu from anywhere
    currentState = MAIN_MENU;
    scrollOffset = 0;
    inputText = "";
    drawMainMenu();
    globalCPressed = false;
    globalCPress = 0;
    return;
  } else if (globalCPressed && (!M5.BtnPWR.isPressed() && !M5.BtnC.isPressed()) && (millis() - globalCPress < 700)) {
    // Short press - let individual handlers deal with it
    globalCPressed = false;
    globalCPress = 0;
  }
  
  switch(currentState) {
    case MAIN_MENU:
      handleMainMenu();
      break;
    case FILES_MENU:
      handleFilesMenu();
      break;
    case CHAT_MENU:
      handleChatMenu();
      break;
    case SETTINGS_MENU:
      handleSettingsMenu();
      break;
    case MODEL_SELECT:
      handleModelSelect();
      break;
    case WEBUI_MODE:
      server.handleClient();
      if (M5.BtnB.wasPressed()) {
        stopWebUI();
      }
      break;
    case KEYBOARD_INPUT:
      handleKeyboard();
      break;
    case FILE_VIEW:
      handleFileView();
      break;
    case FILE_CONTENT:
      handleFileContent();
      break;
  }
  
  delay(10);
}

void drawMainMenu() {
  M5.Display.fillScreen(BLACK);
  M5.Display.setCursor(0, 0);
  M5.Display.setTextColor(CYAN);
  M5.Display.setTextSize(2);
  M5.Display.println("MAIN MENU");
  M5.Display.setTextSize(1);
  M5.Display.println("");
  
  for (int i = 0; i < 3; i++) {
    if (i == mainMenuIndex) {
      M5.Display.setTextColor(YELLOW);
      M5.Display.print("> ");
    } else {
      M5.Display.setTextColor(WHITE);
      M5.Display.print("  ");
    }
    M5.Display.println(mainMenuItems[i]);
  }
  
  M5.Display.println("");
  M5.Display.setTextColor(GREEN);
  M5.Display.println("A:Select B:Down");
}

void handleMainMenu() {
  if (M5.BtnA.wasPressed()) {
    switch(mainMenuIndex) {
      case 0:
        currentState = FILES_MENU;
        filesMenuIndex = 0;
        drawFilesMenu();
        break;
      case 1:
        currentState = CHAT_MENU;
        chatMenuIndex = 0;
        drawChatMenu();
        break;
      case 2:
        currentState = SETTINGS_MENU;
        settingsMenuIndex = 0;
        drawSettingsMenu();
        break;
    }
  }
  
  if (M5.BtnB.wasPressed()) {
    mainMenuIndex = (mainMenuIndex + 1) % 3;
    drawMainMenu();
  }
}

void drawFilesMenu() {
  M5.Display.fillScreen(BLACK);
  M5.Display.setCursor(0, 0);
  M5.Display.setTextColor(CYAN);
  M5.Display.setTextSize(2);
  M5.Display.println("FILES");
  M5.Display.setTextSize(1);
  M5.Display.println("");
  
  for (int i = 0; i < 3; i++) {
    if (i == filesMenuIndex) {
      M5.Display.setTextColor(YELLOW);
      M5.Display.print("> ");
    } else {
      M5.Display.setTextColor(WHITE);
      M5.Display.print("  ");
    }
    M5.Display.println(filesMenuItems[i]);
  }
  
  M5.Display.println("");
  M5.Display.setTextColor(GREEN);
  M5.Display.println("A:Select B:Down");
}

void handleFilesMenu() {
  if (M5.BtnA.wasPressed()) {
    switch(filesMenuIndex) {
      case 0:
        startWebUI();
        break;
      case 1:
        loadFileList();
        currentState = FILE_VIEW;
        fileListIndex = 0;
        scrollOffset = 0;
        drawFileList();
        break;
      case 2:
        currentState = MAIN_MENU;
        drawMainMenu();
        break;
    }
  }
  
  if (M5.BtnB.wasPressed()) {
    filesMenuIndex = (filesMenuIndex + 1) % 3;
    drawFilesMenu();
  }
}

void loadFileList() {
  fileCount = 0;
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  
  while (file && fileCount < 20) {
    if (!file.isDirectory()) {
      fileNames[fileCount] = String(file.name());
      fileCount++;
    }
    file = root.openNextFile();
  }
  
  if (fileCount == 0) {
    fileNames[0] = "No files found";
    fileCount = 1;
  }
}

void drawFileList() {
  M5.Display.fillScreen(BLACK);
  M5.Display.setCursor(0, 0);
  M5.Display.setTextColor(CYAN);
  M5.Display.setTextSize(1);
  M5.Display.println("UPLOADED FILES");
  M5.Display.println("");
  
  int displayCount = 0;
  int startIdx = scrollOffset;
  
  for (int i = startIdx; i < fileCount && displayCount < 6; i++) {
    if (i == fileListIndex) {
      M5.Display.setTextColor(YELLOW);
      M5.Display.print("> ");
    } else {
      M5.Display.setTextColor(WHITE);
      M5.Display.print("  ");
    }
    
    String displayName = fileNames[i];
    if (displayName.length() > 20) {
      displayName = displayName.substring(0, 17) + "...";
    }
    M5.Display.println(displayName);
    displayCount++;
  }
  
  M5.Display.println("");
  M5.Display.setTextColor(GREEN);
  M5.Display.println("A:Open B:Up C:Down");
  M5.Display.println("PWR:Back");
}

void handleFileView() {
  if (M5.BtnA.wasPressed()) {
    if (fileNames[fileListIndex] != "No files found") {
      openFile(fileNames[fileListIndex]);
    }
  }
  
  if (M5.BtnB.wasPressed()) {
    if (fileListIndex > 0) {
      fileListIndex--;
      if (fileListIndex < scrollOffset) {
        scrollOffset = fileListIndex;
      }
      drawFileList();
    }
  }
  
  if (M5.BtnC.wasPressed()) {
    if (fileListIndex < fileCount - 1) {
      fileListIndex++;
      if (fileListIndex >= scrollOffset + 6) {
        scrollOffset++;
      }
      drawFileList();
    }
  }
  
  if (M5.BtnPWR.wasPressed()) {
    currentState = FILES_MENU;
    drawFilesMenu();
  }
}

void openFile(String fileName) {
  File file = SPIFFS.open(fileName, "r");
  if (!file) {
    showMessage("Error opening file", 2000);
    return;
  }
  
  String content = file.readString();
  file.close();
  
  currentState = FILE_CONTENT;
  scrollOffset = 0;
  showFileContent(fileName, content);
}

void showFileContent(String fileName, String content) {
  M5.Display.fillScreen(BLACK);
  M5.Display.setCursor(0, 0);
  M5.Display.setTextColor(CYAN);
  M5.Display.setTextSize(1);
  
  String shortName = fileName;
  if (shortName.startsWith("/")) {
    shortName = shortName.substring(1);
  }
  if (shortName.length() > 20) {
    shortName = shortName.substring(0, 17) + "...";
  }
  M5.Display.println(shortName);
  M5.Display.println("");
  
  M5.Display.setTextColor(WHITE);
  
  int charsPerLine = 26;
  int maxLines = 7;
  int totalLines = 0;
  
  for (int i = 0; i < content.length(); i++) {
    if (i % charsPerLine == 0 && i > 0) totalLines++;
  }
  totalLines++;
  maxScroll = max(0, totalLines - maxLines);
  
  int skipLines = scrollOffset;
  int currentLine = 0;
  int linesPrinted = 0;
  
  for (int i = 0; i < content.length() && linesPrinted < maxLines; i++) {
    if (i > 0 && i % charsPerLine == 0) {
      currentLine++;
      if (currentLine > skipLines) {
        M5.Display.println();
        linesPrinted++;
      }
    }
    
    if (currentLine >= skipLines && linesPrinted < maxLines) {
      M5.Display.print(content[i]);
    }
  }
  
  M5.Display.setCursor(0, 120);
  M5.Display.setTextColor(GREEN);
  M5.Display.println("B:Up C:Down PWR:Back");
}

void handleFileContent() {
  if (M5.BtnB.wasPressed()) {
    if (scrollOffset > 0) {
      scrollOffset--;
      String fileName = fileNames[fileListIndex];
      File file = SPIFFS.open(fileName, "r");
      String content = file.readString();
      file.close();
      showFileContent(fileName, content);
    }
  }
  
  if (M5.BtnC.wasPressed()) {
    if (scrollOffset < maxScroll) {
      scrollOffset++;
      String fileName = fileNames[fileListIndex];
      File file = SPIFFS.open(fileName, "r");
      String content = file.readString();
      file.close();
      showFileContent(fileName, content);
    }
  }
  
  if (M5.BtnPWR.wasPressed()) {
    currentState = FILE_VIEW;
    scrollOffset = 0;
    drawFileList();
  }
}

void drawChatMenu() {
  M5.Display.fillScreen(BLACK);
  M5.Display.setCursor(0, 0);
  M5.Display.setTextColor(CYAN);
  M5.Display.setTextSize(2);
  M5.Display.println("CHAT");
  M5.Display.setTextSize(1);
  M5.Display.println("");
  
  for (int i = 0; i < 3; i++) {
    if (i == chatMenuIndex) {
      M5.Display.setTextColor(YELLOW);
      M5.Display.print("> ");
    } else {
      M5.Display.setTextColor(WHITE);
      M5.Display.print("  ");
    }
    M5.Display.println(chatMenuItems[i]);
  }
  
  M5.Display.println("");
  M5.Display.setTextColor(GREEN);
  M5.Display.println("A:Select B:Down");
}

void handleChatMenu() {
  if (M5.BtnA.wasPressed()) {
    switch(chatMenuIndex) {
      case 0:
        startKeyboardInput();
        break;
      case 1:
        if (lastResponse.length() > 0) {
          improveLastResponse();
        } else {
          showMessage("No chat history", 2000);
          drawChatMenu();
        }
        break;
      case 2:
        currentState = MAIN_MENU;
        drawMainMenu();
        break;
    }
  }
  
  if (M5.BtnB.wasPressed()) {
    chatMenuIndex = (chatMenuIndex + 1) % 3;
    drawChatMenu();
  }
}

void startKeyboardInput() {
  currentState = KEYBOARD_INPUT;
  inputText = "";
  keyRow = 0;
  keyCol = 0;
  isInputting = true;
  drawKeyboard();
}

void drawKeyboard() {
  M5.Display.fillScreen(BLACK);
  M5.Display.setCursor(0, 0);
  M5.Display.setTextColor(CYAN);
  M5.Display.print("Input: ");
  M5.Display.setTextColor(WHITE);
  String displayText = inputText;
  if (displayText.length() > 20) {
    displayText = displayText.substring(displayText.length() - 20);
  }
  M5.Display.println(displayText);
  M5.Display.println("");
  
  for (int r = 0; r < 4; r++) {
    for (int c = 0; c < 10; c++) {
      if (r == keyRow && c == keyCol) {
        M5.Display.setTextColor(BLACK);
        M5.Display.fillRect(c * 13, (r + 2) * 15, 12, 14, YELLOW);
        M5.Display.setCursor(c * 13 + 1, (r + 2) * 15 + 1);
        M5.Display.setTextColor(BLACK);
      } else {
        M5.Display.setTextColor(WHITE);
        M5.Display.setCursor(c * 13, (r + 2) * 15);
      }
      M5.Display.print(keyboard[r][c]);
    }
  }
  
  M5.Display.setCursor(0, 100);
  M5.Display.setTextColor(GREEN);
  M5.Display.setTextSize(1);
  M5.Display.println("B:Right Hold:Space/Left");
  M5.Display.println("C:Up/Down Hold:Menu");
  M5.Display.println("A:Select Hold:Send");
  M5.Display.println("Serial input active");
}

void handleKeyboard() {
  static unsigned long btnAPress = 0;
  static unsigned long btnBPress = 0;
  static unsigned long btnCPress = 0;
  static bool btnAPressed = false;
  static bool btnBPressed = false;
  static bool btnCPressed = false;
  static unsigned long lastSerialCheck = 0;
  
  // Check serial input
  if (millis() - lastSerialCheck > 100) {
    lastSerialCheck = millis();
    if (Serial.available()) {
      String serialInput = Serial.readStringUntil('\n');
      serialInput.trim();
      if (serialInput.length() > 0) {
        inputText += serialInput;
        // Check if input ends with newline to auto-send
        if (serialInput.indexOf('\n') >= 0 || serialInput.indexOf('\r') >= 0) {
          String prompt = inputText;
          inputText = "";
          isInputting = false;
          askAI(prompt);
          return;
        }
        drawKeyboard();
      }
    }
  }
  
  // Handle C button (PWR) - Up/Down navigation or Menu on long press
  if (M5.BtnPWR.wasPressed() || M5.BtnC.wasPressed()) {
    btnCPress = millis();
    btnCPressed = true;
  }
  
  if (btnCPressed && (M5.BtnPWR.isPressed() || M5.BtnC.isPressed()) && (millis() - btnCPress > 700)) {
    // Long press C (PWR) = Return to Main Menu
    currentState = MAIN_MENU;
    inputText = "";
    scrollOffset = 0;
    drawMainMenu();
    btnCPressed = false;
    btnCPress = 0;
    return;
  } else if (btnCPressed && (!M5.BtnPWR.isPressed() && !M5.BtnC.isPressed()) && (millis() - btnCPress < 700)) {
    // Short press C = Navigate Up/Down
    keyRow = (keyRow + 1) % 4;
    drawKeyboard();
    btnCPressed = false;
    btnCPress = 0;
  }
  
  // Handle B button - Right (short), Space (hold 700ms), Left (hold longer then release)
  static bool bSpaceAdded = false;
  
  if (M5.BtnB.wasPressed()) {
    btnBPress = millis();
    btnBPressed = true;
    bSpaceAdded = false;
  }
  
  if (btnBPressed && M5.BtnB.isPressed()) {
    unsigned long holdTime = millis() - btnBPress;
    if (holdTime > 700 && !bSpaceAdded) {
      // Hold 700ms = Space (trigger once)
      inputText += " ";
      drawKeyboard();
      bSpaceAdded = true;
    }
  } else if (M5.BtnB.wasReleased() && btnBPressed) {
    unsigned long holdTime = millis() - btnBPress;
    if (holdTime < 700) {
      // Short press B = Right
      keyCol = (keyCol + 1) % 10;
      drawKeyboard();
    } else if (holdTime >= 1400) {
      // Hold >1400ms then release = Left
      keyCol = (keyCol - 1 + 10) % 10;
      drawKeyboard();
    }
    // If 700-1400ms, space was already added, nothing more to do
    btnBPressed = false;
    btnBPress = 0;
    bSpaceAdded = false;
  }
  
  // Handle A button (Select or Send on hold)
  if (M5.BtnA.wasPressed()) {
    btnAPress = millis();
    btnAPressed = true;
  }
  
  if (btnAPressed && M5.BtnA.isPressed() && (millis() - btnAPress > 700)) {
    // Holding A for 700ms = Send
    if (inputText.length() > 0) {
      String prompt = inputText;
      inputText = "";
      isInputting = false;
      askAI(prompt);
    }
    btnAPressed = false;
    btnAPress = 0;
  } else if (M5.BtnA.wasReleased() && btnAPressed && (millis() - btnAPress < 700)) {
    // Short press A = Select
    char selectedChar = keyboard[keyRow][keyCol];
    inputText += selectedChar;
    drawKeyboard();
    btnAPressed = false;
    btnAPress = 0;
  }
}

void askAI(String question) {
  showMessage("Sending...", 0);
  
  String fullPrompt = String(modePrompts[currentMode]) + question;
  lastTopic = question;
  
  WiFiClientSecure client;
  client.setInsecure();
  
  HTTPClient https;
  String questionJson = "\"" + fullPrompt + "\"";
  String url;
  
  if (currentModel == "gemini") {
    url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash:generateContent?key=" + geminiKey;
  } else if (currentModel == "deepseek") {
    url = "https://api.deepseek.com/v1/chat/completions";
  } else if (currentModel == "chatgpt") {
    url = "https://api.openai.com/v1/chat/completions";
  }
  
  if (https.begin(client, url)) {
    https.addHeader("Content-Type", "application/json");
    if (currentModel == "deepseek") {
      https.addHeader("Authorization", "Bearer " + deepseekKey);
    } else if (currentModel == "chatgpt") {
      https.addHeader("Authorization", "Bearer " + chatgptKey);
    }
    https.setTimeout(20000);
    
    String payload;
    if (currentModel == "gemini") {
      payload = "{\"contents\": [{\"parts\":[{\"text\":" + questionJson + "}]}],\"generationConfig\": {\"maxOutputTokens\": " + maxTokens + "}}";
    } else {
      String modelName = (currentModel == "chatgpt") ? "gpt-4" : "deepseek-chat";
      payload = "{\"model\":\"" + modelName + "\",\"messages\":[{\"role\":\"user\",\"content\":" + questionJson + "}],\"max_tokens\":" + maxTokens + "}";
    }
    
    int httpCode = https.POST(payload);
    
    if (httpCode == HTTP_CODE_OK) {
      String response = https.getString();
      
      DynamicJsonDocument doc(8192);
      deserializeJson(doc, response);
      
      String answer;
      if (currentModel == "gemini") {
        answer = doc["candidates"][0]["content"]["parts"][0]["text"] | "No response";
      } else {
        answer = doc["choices"][0]["message"]["content"] | "No response";
      }
      
      lastResponse = answer;
      scrollOffset = 0;
      showResponse(answer);
    } else {
      showMessage("Error: " + String(httpCode), 3000);
      currentState = CHAT_MENU;
      drawChatMenu();
    }
    
    https.end();
  }
}

void improveLastResponse() {
  String improvePrompt = "Improve and expand: " + lastResponse;
  askAI(improvePrompt);
}

void showResponse(String response) {
  M5.Display.fillScreen(BLACK);
  M5.Display.setCursor(0, 0);
  M5.Display.setTextColor(GREEN);
  M5.Display.println("Response:");
  M5.Display.println("");
  M5.Display.setTextColor(WHITE);
  M5.Display.setTextSize(1);
  
  // No wrapping - only break on actual newlines, truncate long lines
  int charsPerLine = 26;
  int maxLines = 8;
  
  // Split response by newlines only
  int lineStart = 0;
  int totalLines = 1;
  
  // Count total lines (only actual newlines)
  for (int i = 0; i < response.length(); i++) {
    if (response[i] == '\n') {
      totalLines++;
    }
  }
  
  maxScroll = max(0, totalLines - maxLines);
  int skipLines = scrollOffset;
  
  // Display lines without wrapping
  int currentLine = 0;
  int linesPrinted = 0;
  
  for (int i = 0; i <= response.length() && linesPrinted < maxLines; i++) {
    if (i == response.length() || response[i] == '\n') {
      // Line found (either newline or end of string)
      if (currentLine >= skipLines) {
        String line = response.substring(lineStart, i);
        // Clean and truncate line (no wrapping)
        String cleanLine = "";
        for (int j = 0; j < line.length(); j++) {
          char c = line[j];
          if (c == '\r') continue; // Skip carriage return
          if (c == '\t') cleanLine += ' ';
          else if (isprint(c)) cleanLine += c;
          
          // Truncate at char limit (no wrap)
          if (cleanLine.length() >= charsPerLine) {
            break;
          }
        }
        M5.Display.println(cleanLine);
        linesPrinted++;
      }
      currentLine++;
      lineStart = i + 1;
    }
  }
  
  // Display last line if it didn't end with newline
  if (lineStart < response.length() && currentLine >= skipLines && linesPrinted < maxLines) {
    String line = response.substring(lineStart);
    String cleanLine = "";
    for (int j = 0; j < line.length(); j++) {
      char c = line[j];
      if (c == '\r') continue;
      if (c == '\t') cleanLine += ' ';
      else if (isprint(c)) cleanLine += c;
      if (cleanLine.length() >= charsPerLine) break;
    }
    M5.Display.print(cleanLine);
  }
  
  M5.Display.setCursor(0, 120);
  M5.Display.setTextColor(YELLOW);
  M5.Display.println("A:New Q B:Up C:Down");
  M5.Display.setTextColor(GREEN);
  M5.Display.println("Hold C:Menu");
  
  Serial.println("\n=== FULL RESPONSE ===");
  Serial.println(response);
  Serial.println("=====================\n");
  
  waitForButtonScroll();
}

void waitForButtonScroll() {
  static unsigned long btnCPress = 0;
  static bool btnCPressed = false;
  
  while(true) {
    M5.update();
    
    // Handle C button (PWR) - long press returns to menu
    if (M5.BtnPWR.wasPressed() || M5.BtnC.wasPressed()) {
      btnCPress = millis();
      btnCPressed = true;
    }
    
    if (btnCPressed && (M5.BtnPWR.isPressed() || M5.BtnC.isPressed()) && (millis() - btnCPress > 700)) {
      // Long press C (PWR) = Main Menu
      currentState = MAIN_MENU;
      scrollOffset = 0;
      drawMainMenu();
      btnCPressed = false;
      btnCPress = 0;
      break;
    } else if (btnCPressed && (!M5.BtnPWR.isPressed() && !M5.BtnC.isPressed()) && (millis() - btnCPress < 700)) {
      // Short press C = Scroll Down
      if (scrollOffset < maxScroll) {
        scrollOffset++;
        showResponse(lastResponse);
      }
      btnCPressed = false;
      btnCPress = 0;
    }
    
    if (M5.BtnA.wasPressed()) {
      // Ask for new question - go back to keyboard input
      scrollOffset = 0;
      startKeyboardInput();
      break;
    }
    
    if (M5.BtnB.wasPressed()) {
      // Scroll Up
      if (scrollOffset > 0) {
        scrollOffset--;
        showResponse(lastResponse);
      }
    }
    
    delay(10);
  }
}

void tryAutoConnectWiFi() {
  if (knownNetworks.length() == 0) {
    return;
  }
  
  // Only try to connect if not already connected
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }
  
  // Scan for available networks
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  int n = WiFi.scanNetworks();
  if (n == 0) {
    return;
  }
  
  // Parse known networks (format: SSID1:Password1\nSSID2:Password2)
  String networks = knownNetworks;
  int startPos = 0;
  
  while (startPos < networks.length()) {
    int endPos = networks.indexOf('\n', startPos);
    if (endPos == -1) {
      endPos = networks.length();
    }
    
    String networkEntry = networks.substring(startPos, endPos);
    int colonPos = networkEntry.indexOf(':');
    
    if (colonPos > 0) {
      String networkSSID = networkEntry.substring(0, colonPos);
      String networkPass = networkEntry.substring(colonPos + 1);
      
      // Check if this network is in the scan results
      for (int i = 0; i < n; i++) {
        if (WiFi.SSID(i) == networkSSID) {
          // Try to connect
          Serial.println("Attempting to connect to: " + networkSSID);
          WiFi.begin(networkSSID.c_str(), networkPass.c_str());
          
          int attempts = 0;
          while (WiFi.status() != WL_CONNECTED && attempts < 10) {
            delay(500);
            attempts++;
          }
          
          if (WiFi.status() == WL_CONNECTED) {
            Serial.println("Connected to: " + networkSSID);
            Serial.println("IP address: " + WiFi.localIP().toString());
            ssid = networkSSID;
            password = networkPass;
            
            // Save to preferences
            preferences.begin("ai-config", false);
            preferences.putString("ssid", ssid);
            preferences.putString("password", password);
            preferences.end();
            
            return;
          }
        }
      }
    }
    
    startPos = endPos + 1;
  }
}

void startWebUI() {
  WiFi.softAP(ap_ssid.c_str(), ap_password.c_str());
  IPAddress IP = WiFi.softAPIP();
  
  server.on("/", handleRoot);
  server.on("/api/chat", HTTP_POST, handleAPIChat);
  server.on("/api/settings", HTTP_POST, handleSaveSettings);
  server.on("/api/settings", HTTP_GET, handleGetSettings);
  server.on("/api/status", HTTP_GET, handleGetStatus);
  server.on("/api/networks", HTTP_GET, handleGetNetworks);
  server.on("/api/networks", HTTP_POST, handleAddNetwork);
  server.on("/api/networks/remove", HTTP_POST, handleRemoveNetwork);
  server.on("/api/files", HTTP_GET, handleGetFiles);
  server.on("/api/upload", HTTP_POST, [](){server.send(200);}, handleFileUpload);
  server.on("/api/file", HTTP_GET, handleGetFile);
  server.begin();
  
  currentState = WEBUI_MODE;
  
  M5.Display.fillScreen(BLACK);
  M5.Display.setCursor(0, 0);
  M5.Display.setTextColor(GREEN);
  M5.Display.println("Edu-Sploit Active");
  M5.Display.setTextColor(WHITE);
  M5.Display.println("");
  M5.Display.print("SSID: ");
  M5.Display.println(ap_ssid);
  M5.Display.print("Pass: ");
  M5.Display.println(ap_password);
  M5.Display.print("IP: ");
  M5.Display.println(IP);
  M5.Display.println("");
  M5.Display.setTextColor(YELLOW);
  M5.Display.println("B: Stop WebUI");
}

void handleRoot() {
String html = 
"<!DOCTYPE html><html><head>"
"<meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no'>"
"<meta name='apple-mobile-web-app-capable' content='yes'>"
"<meta name='apple-mobile-web-app-status-bar-style' content='black-translucent'>"
"<link rel='apple-touch-icon' href='data:image/svg+xml,%3Csvg xmlns=%27http://www.w3.org/2000/svg%27 viewBox=%270 0 100 100%27%3E%3Crect fill=%27%2310a37f%27 width=%27100%27 height=%27100%27/%3E%3Ctext y=%27.9em%27 font-size=%2790%27%3E%F0%9F%A4%96%3C/text%3E%3C/svg%3E'>"
"<title>Edu-Sploit</title>"

"<style>*{margin:0;padding:0;box-sizing:border-box}body{font-family:-apple-system,BlinkMacSystemFont,Segoe UI,Roboto,sans-serif;background:#0d0d0d;color:#ececec;height:100vh;overflow-x:hidden;touch-action:pan-y;-webkit-overflow-scrolling:touch}"
".container{display:flex;height:100vh;position:relative}"
".sidebar{width:260px;background:#171717;border-right:1px solid #2d2d2d;display:flex;flex-direction:column;padding:12px;justify-content:space-between;position:fixed;left:0;top:0;bottom:0;z-index:1001;transition:transform 0.3s ease}"
".sidebar.hidden{transform:translateX(-100%)}"
".sidebar-overlay{display:none;position:fixed;top:0;left:0;right:0;bottom:0;background:rgba(0,0,0,0.5);z-index:1000;transition:opacity 0.3s ease}"
".sidebar-overlay.active{display:block}"
".main{flex:1;display:flex;flex-direction:column;margin-left:0;transition:margin-left 0.3s ease}"
"@media(min-width:769px){.sidebar{position:relative;transform:none!important}.sidebar.hidden{transform:none}.main{margin-left:0}.sidebar-toggle{display:none!important}.sidebar-overlay{display:none!important}.main{margin-left:260px}}"
".header{padding:16px;border-bottom:1px solid #2d2d2d;display:flex;align-items:center;gap:12px;position:sticky;top:0;background:#1a1a1a;z-index:999}"
".sidebar-toggle{background:#333;border:1px solid #444;color:#fff;padding:8px 12px;border-radius:6px;cursor:pointer;font-size:16px;display:flex;align-items:center;justify-content:center;width:36px;height:36px}"
".messages{flex:1;overflow-y:auto;padding:20px}"
".input-area{padding:16px;border-top:1px solid #2d2d2d}"
".new-chat{background:#10a37f;color:#fff;border:none;padding:12px;border-radius:8px;width:100%;font-size:14px;font-weight:600;cursor:pointer;margin-bottom:12px}"
".menu-item{padding:12px;border-radius:8px;cursor:pointer;font-size:14px;margin-bottom:4px;transition:background .2s}"
".menu-item:hover{background:#2d2d2d}"
".menu-item.active{background:#2d2d2d}"
".input-box{background:#2d2d2d;border:1px solid #404040;border-radius:12px;padding:12px;display:flex;gap:8px;align-items:flex-end}"
"textarea{background:transparent;border:none;color:#ececec;width:100%;resize:none;font-size:14px;font-family:inherit;outline:none;max-height:200px}"
"select{background:#2d2d2d;color:#ececec;border:1px solid #404040;padding:8px 12px;border-radius:8px;font-size:13px;outline:none;cursor:pointer;margin-bottom:8px}"
".send-btn{background:#10a37f;border:none;color:#fff;padding:10px 16px;border-radius:8px;cursor:pointer;font-size:14px;font-weight:600}"
".message{margin-bottom:20px;padding:16px;border-radius:12px}"
".user{background:#2d2d2d}"
".assistant{background:#1a1a1a}"
".logo{width:32px;height:32px;background:linear-gradient(135deg,#10a37f,#1a7f64);border-radius:8px;display:flex;align-items:center;justify-content:center;font-size:20px;cursor:pointer}"
".settings-panel,.files-panel{display:none;padding:20px}"
".settings-panel.active,.files-panel.active{display:block}"
".settings-group{margin-bottom:20px}"
".settings-label{font-size:13px;color:#8e8e8e;margin-bottom:8px}"
"input[type=text],input[type=password]{background:#2d2d2d;border:1px solid #404040;color:#ececec;padding:10px;border-radius:8px;width:100%;font-size:14px;outline:none}"
".save-btn{background:#10a37f;color:#fff;border:none;padding:12px 24px;border-radius:8px;cursor:pointer;font-size:14px;font-weight:600;margin-top:12px}"
".file-item{background:#2d2d2d;padding:12px;border-radius:8px;margin-bottom:8px;display:flex;justify-content:space-between;align-items:center}"
".upload-area{border:2px dashed #404040;border-radius:12px;padding:40px;text-align:center;cursor:pointer;margin-bottom:20px;transition:border-color .3s}"
".upload-area:hover{border-color:#10a37f}"
"input[type=file]{display:none}"
".close-sidebar{background:#404040;color:#fff;border:none;padding:12px;border-radius:8px;width:100%;font-size:14px;font-weight:600;cursor:pointer;margin-top:auto}"
"@media(max-width:768px){.main{margin-left:0}.sidebar-toggle{display:flex}}"
"</style></head><body>"

"<div class='container'>"
  "<div class='sidebar-overlay' id='sidebarOverlay' onclick='closeSidebar()'></div>"
  "<div class='sidebar hidden' id='sidebar'>"
    "<div>"
      "<div class='menu-item active' onclick='showDashboard();closeSidebar();'>📊 Dashboard</div>"
      "<div class='menu-item' onclick='showChat();closeSidebar();'>💬 Chat</div>"
      "<div class='menu-item' onclick='showFiles();closeSidebar();'>📁 Files</div>"
      "<div class='menu-item' onclick='showSettings();closeSidebar();'>⚙️ Settings</div>"
    "</div>"
    "<button class='close-sidebar' onclick='closeSidebar()'>✕ Close</button>"
  "</div>"

  "<div class='main'>"
    "<div class='header'>"
      "<button class='sidebar-toggle' onclick='toggleSidebar()' id='sidebarToggle'>☰</button>"
      "<h2 style='font-size:20px;font-weight:700;flex:1'>Edu-Sploit</h2>"
      "<button onclick='refreshDashboard()' style='background:#333;border:1px solid #444;color:#fff;padding:8px 16px;border-radius:8px;cursor:pointer;font-size:12px' id='refreshBtn'>🔄 Refresh</button>"
    "</div>"
    "<div id='dashboardView' style='display:grid;grid-template-columns:repeat(auto-fit,minmax(280px,1fr));gap:20px;padding:20px;padding-bottom:100px;overflow-y:auto;-webkit-overflow-scrolling:touch'>"
      "<div style='background:#1a1a1a;border:1px solid #333;border-radius:12px;padding:20px'>"
        "<div style='font-size:14px;color:#999;margin-bottom:12px;font-weight:600;text-transform:uppercase'>🔋 Battery</div>"
        "<div id='batteryPercent' style='font-size:32px;font-weight:700;margin-bottom:8px;color:#fff'>--%</div>"
        "<div id='batteryVoltage' style='font-size:12px;color:#666;margin-top:8px'>-- V</div>"
        "<div style='width:100%;height:8px;background:#333;border-radius:4px;overflow:hidden;margin-top:12px'><div id='batteryBar' style='height:100%;background:linear-gradient(90deg,#34C759,#30D158);transition:width 0.3s;width:0%'></div></div>"
      "</div>"
      "<div style='background:#1a1a1a;border:1px solid #333;border-radius:12px;padding:20px'>"
        "<div style='font-size:14px;color:#999;margin-bottom:12px;font-weight:600;text-transform:uppercase'>🤖 Current Model</div>"
        "<div id='currentModel' style='font-size:24px;font-weight:700;margin-bottom:16px;color:#fff'>--</div>"
        "<div style='display:flex;gap:8px;margin-top:16px'><button class='model-btn' onclick='changeModel(\"gemini\")' style='flex:1;padding:10px;background:#333;border:1px solid #444;border-radius:8px;color:#fff;cursor:pointer;font-size:12px'>Gemini</button><button class='model-btn' onclick='changeModel(\"chatgpt\")' style='flex:1;padding:10px;background:#333;border:1px solid #444;border-radius:8px;color:#fff;cursor:pointer;font-size:12px'>ChatGPT</button><button class='model-btn' onclick='changeModel(\"deepseek\")' style='flex:1;padding:10px;background:#333;border:1px solid #444;border-radius:8px;color:#fff;cursor:pointer;font-size:12px'>DeepSeek</button></div>"
        "<div style='font-size:12px;color:#666;margin-top:12px'>Max Tokens: <span id='maxTokens'>--</span></div>"
      "</div>"
      "<div style='background:#1a1a1a;border:1px solid #333;border-radius:12px;padding:20px'>"
        "<div style='font-size:14px;color:#999;margin-bottom:12px;font-weight:600;text-transform:uppercase'>🔑 API Keys Status</div>"
        "<div style='display:flex;justify-content:space-between;align-items:center;padding:12px 0;border-bottom:1px solid #333'><span style='font-size:14px;color:#fff'>🚀 Gemini</span><span id='geminiStatus' class='status-badge' style='padding:4px 12px;border-radius:12px;font-size:12px;font-weight:600'>--</span></div>"
        "<div style='display:flex;justify-content:space-between;align-items:center;padding:12px 0;border-bottom:1px solid #333'><span style='font-size:14px;color:#fff'>💬 ChatGPT</span><span id='chatgptStatus' class='status-badge' style='padding:4px 12px;border-radius:12px;font-size:12px;font-weight:600'>--</span></div>"
        "<div style='display:flex;justify-content:space-between;align-items:center;padding:12px 0'><span style='font-size:14px;color:#fff'>🧠 DeepSeek</span><span id='deepseekStatus' class='status-badge' style='padding:4px 12px;border-radius:12px;font-size:12px;font-weight:600'>--</span></div>"
      "</div>"
      "<div style='background:#1a1a1a;border:1px solid #333;border-radius:12px;padding:20px'>"
        "<div style='font-size:14px;color:#999;margin-bottom:12px;font-weight:600;text-transform:uppercase'>📡 Network</div>"
        "<div style='display:flex;justify-content:space-between;padding:8px 0;font-size:13px'><span style='color:#999'>SSID:</span><span id='wifiSSID' style='color:#fff;font-weight:600'>--</span></div>"
        "<div style='display:flex;justify-content:space-between;padding:8px 0;font-size:13px'><span style='color:#999'>IP:</span><span id='wifiIP' style='color:#fff;font-weight:600'>--</span></div>"
        "<div style='display:flex;justify-content:space-between;padding:8px 0;font-size:13px'><span style='color:#999'>RSSI:</span><span id='wifiRSSI' style='color:#fff;font-weight:600'>--</span></div>"
      "</div>"
    "</div>"
    "<div id='chatView' style='display:none;flex-direction:column;flex:1;padding:20px;padding-bottom:100px'>"
      "<div class='messages' id='messages'></div>"
      "<div class='input-area'>"
        "<select id='modelSelect'>"
          "<option value='gemini'>Gemini 2.0</option>"
          "<option value='chatgpt'>ChatGPT</option>"
          "<option value='deepseek'>DeepSeek</option>"
        "</select>"
        "<div class='input-box'>"
          "<textarea id='input' placeholder='Message AI...' rows='1' onkeydown='handleKeydown(event)'></textarea>"
          "<button class='send-btn' onclick='sendMessage()'>Send</button>"
        "</div>"
      "</div>"
    "</div>"

    "<div id='filesView' class='files-panel' style='display:none;padding:20px;padding-bottom:100px;overflow-y:auto;-webkit-overflow-scrolling:touch'>"
      "<h3 style='margin-bottom:20px'>File Manager</h3>"
      "<div class='upload-area' onclick='document.getElementById(\"fileInput\").click()'>"
        "Click or drag files here<br><span style='font-size:12px;color:#8e8e8e'>Supports .txt, .md, .json</span>"
      "</div>"
      "<input type='file' id='fileInput' accept='.txt,.md,.json' onchange='uploadFile()'>"
      "<div id='fileList'></div>"
    "</div>"

    "<div id='settingsView' class='settings-panel' style='display:none;padding:20px;max-width:600px;margin:0 auto;width:100%;padding-bottom:100px;overflow-y:auto;-webkit-overflow-scrolling:touch'>"
      "<h3 style='margin-bottom:24px;font-size:20px'>Settings</h3>"
      "<div class='settings-group' style='margin-bottom:24px'>"
        "<div class='settings-label' style='font-size:13px;color:#999;margin-bottom:8px;font-weight:600'>🚀 Gemini API Key</div>"
        "<input type='password' id='geminiKey' placeholder='Enter your Gemini API key' style='width:100%;padding:12px;background:#1a1a1a;border:1px solid #333;border-radius:8px;color:#fff;font-size:14px;outline:none'>"
      "</div>"
      "<div class='settings-group' style='margin-bottom:24px'>"
        "<div class='settings-label' style='font-size:13px;color:#999;margin-bottom:8px;font-weight:600'>💬 ChatGPT API Key</div>"
        "<input type='password' id='chatgptKey' placeholder='Enter your OpenAI API key' style='width:100%;padding:12px;background:#1a1a1a;border:1px solid #333;border-radius:8px;color:#fff;font-size:14px;outline:none'>"
      "</div>"
      "<div class='settings-group' style='margin-bottom:24px'>"
        "<div class='settings-label' style='font-size:13px;color:#999;margin-bottom:8px;font-weight:600'>🧠 DeepSeek API Key</div>"
        "<input type='password' id='deepseekKey' placeholder='Enter your DeepSeek API key' style='width:100%;padding:12px;background:#1a1a1a;border:1px solid #333;border-radius:8px;color:#fff;font-size:14px;outline:none'>"
      "</div>"
      "<div class='settings-group' style='margin-bottom:24px'>"
        "<div class='settings-label' style='font-size:13px;color:#999;margin-bottom:8px;font-weight:600'>Max Tokens</div>"
        "<input type='text' id='maxTokensInput' placeholder='300' value='300' style='width:100%;padding:12px;background:#1a1a1a;border:1px solid #333;border-radius:8px;color:#fff;font-size:14px;outline:none'>"
      "</div>"
      "<div class='settings-group' style='margin-bottom:24px'>"
        "<div class='settings-label' style='font-size:13px;color:#999;margin-bottom:8px;font-weight:600'>📡 WebUI AP Name</div>"
        "<input type='text' id='apName' placeholder='Edu-Sploit' style='width:100%;padding:12px;background:#1a1a1a;border:1px solid #333;border-radius:8px;color:#fff;font-size:14px;outline:none'>"
        "<div style='font-size:12px;color:#666;margin-top:8px'>Name of the WiFi hotspot created by WebUI</div>"
      "</div>"
      "<div class='settings-group' style='margin-bottom:24px'>"
        "<div class='settings-label' style='font-size:13px;color:#999;margin-bottom:8px;font-weight:600'>🌐 Known Networks</div>"
        "<div style='font-size:12px;color:#666;margin-bottom:12px'>Auto-connects on boot. Format: SSID:Password</div>"
        "<div id='knownNetworksList' style='max-height:200px;overflow-y:auto;margin-bottom:12px;background:#1a1a1a;border:1px solid #333;border-radius:8px;padding:12px'></div>"
        "<div style='display:flex;gap:8px;margin-bottom:12px'>"
          "<input type='text' id='newNetworkSSID' placeholder='SSID' style='flex:1;padding:10px;background:#333;border:1px solid #444;border-radius:6px;color:#fff;font-size:13px;outline:none'>"
          "<input type='password' id='newNetworkPass' placeholder='Password' style='flex:1;padding:10px;background:#333;border:1px solid #444;border-radius:6px;color:#fff;font-size:13px;outline:none'>"
          "<button onclick='addNetwork()' style='padding:10px 16px;background:#007AFF;border:none;border-radius:6px;color:#fff;cursor:pointer;font-size:13px;font-weight:600'>+ Add</button>"
        "</div>"
      "</div>"
      "<button onclick='saveSettings()' style='background:#007AFF;color:#fff;border:none;padding:12px 24px;border-radius:8px;cursor:pointer;font-size:14px;font-weight:600;margin-top:12px;width:100%'>💾 Save Settings</button>"
    "</div>"
  "</div>"
"</div>"

"<script>"
"let startTime=Date.now();"
"function toggleSidebar(){const sidebar=document.getElementById('sidebar');const overlay=document.getElementById('sidebarOverlay');sidebar.classList.toggle('hidden');overlay.classList.toggle('active');}function closeSidebar(){document.getElementById('sidebar').classList.add('hidden');document.getElementById('sidebarOverlay').classList.remove('active');}"
"function showDashboard(){document.querySelectorAll('.menu-item').forEach(item=>item.classList.remove('active'));if(document.querySelectorAll('.menu-item')[0])document.querySelectorAll('.menu-item')[0].classList.add('active');document.getElementById('dashboardView').style.display='grid';document.getElementById('chatView').style.display='none';document.getElementById('filesView').style.display='none';document.getElementById('settingsView').style.display='none';refreshDashboard();}"
"function showChat(){document.querySelectorAll('.menu-item').forEach(item=>item.classList.remove('active'));if(document.querySelectorAll('.menu-item')[1])document.querySelectorAll('.menu-item')[1].classList.add('active');document.getElementById('dashboardView').style.display='none';document.getElementById('chatView').style.display='flex';document.getElementById('filesView').style.display='none';document.getElementById('settingsView').style.display='none';}"
"function showFiles(){document.querySelectorAll('.menu-item').forEach(item=>item.classList.remove('active'));if(document.querySelectorAll('.menu-item')[2])document.querySelectorAll('.menu-item')[2].classList.add('active');document.getElementById('dashboardView').style.display='none';document.getElementById('chatView').style.display='none';document.getElementById('filesView').style.display='block';document.getElementById('settingsView').style.display='none';loadFiles();}"
"function showSettings(){document.querySelectorAll('.menu-item').forEach(item=>item.classList.remove('active'));if(document.querySelectorAll('.menu-item')[3])document.querySelectorAll('.menu-item')[3].classList.add('active');document.getElementById('dashboardView').style.display='none';document.getElementById('chatView').style.display='none';document.getElementById('filesView').style.display='none';document.getElementById('settingsView').style.display='block';loadSettings();loadKnownNetworks();}"
"async function refreshDashboard(){try{const res=await fetch('/api/status');const data=await res.json();if(data.battery){document.getElementById('batteryPercent').textContent=data.battery.percent+'%';document.getElementById('batteryVoltage').textContent=data.battery.voltage.toFixed(2)+'V';const bar=document.getElementById('batteryBar');bar.style.width=data.battery.percent+'%';bar.style.background=data.battery.percent<20?'linear-gradient(90deg,#FF3B30,#FF453A)':data.battery.percent<50?'linear-gradient(90deg,#FF9500,#FF9F0A)':'linear-gradient(90deg,#34C759,#30D158)';}if(data.currentModel){document.getElementById('currentModel').textContent=data.currentModel.charAt(0).toUpperCase()+data.currentModel.slice(1);document.querySelectorAll('.model-btn').forEach(btn=>{btn.classList.remove('active');if(btn.textContent.toLowerCase().includes(data.currentModel)){btn.style.background='#007AFF';btn.style.borderColor='#007AFF';}else{btn.style.background='#333';btn.style.borderColor='#444';}});}if(data.maxTokens)document.getElementById('maxTokens').textContent=data.maxTokens;if(data.apiKeys){document.getElementById('geminiStatus').textContent=data.apiKeys.gemini;document.getElementById('geminiStatus').style.background=data.apiKeys.gemini==='configured'?'#34C759':'#FF3B30';document.getElementById('geminiStatus').style.color=data.apiKeys.gemini==='configured'?'#000':'#fff';document.getElementById('chatgptStatus').textContent=data.apiKeys.chatgpt;document.getElementById('chatgptStatus').style.background=data.apiKeys.chatgpt==='configured'?'#34C759':'#FF3B30';document.getElementById('chatgptStatus').style.color=data.apiKeys.chatgpt==='configured'?'#000':'#fff';document.getElementById('deepseekStatus').textContent=data.apiKeys.deepseek;document.getElementById('deepseekStatus').style.background=data.apiKeys.deepseek==='configured'?'#34C759':'#FF3B30';document.getElementById('deepseekStatus').style.color=data.apiKeys.deepseek==='configured'?'#000':'#fff';}if(data.wifi){document.getElementById('wifiSSID').textContent=data.wifi.ssid||'--';document.getElementById('wifiIP').textContent=data.wifi.ip||'--';document.getElementById('wifiRSSI').textContent=(data.wifi.rssi||0)+' dBm';}}catch(e){console.error('Dashboard error:',e);}}"
"async function loadKnownNetworks(){try{const res=await fetch('/api/networks');const data=await res.json();const list=document.getElementById('knownNetworksList');list.innerHTML='';if(data.networks&&data.networks.length>0){data.networks.forEach(network=>{const div=document.createElement('div');div.style.cssText='display:flex;justify-content:space-between;align-items:center;padding:8px;background:#333;border-radius:6px;margin-bottom:6px';div.innerHTML='<span style=\"font-size:12px;color:#fff\">'+network.ssid+'</span><button onclick=\"removeNetwork(\\''+network.ssid+'\\')\" style=\"padding:4px 8px;background:#FF3B30;border:none;border-radius:4px;color:#fff;cursor:pointer;font-size:10px\">Remove</button>';list.appendChild(div);});}else{list.innerHTML='<div style=\"font-size:12px;color:#666;text-align:center;padding:12px\">No known networks</div>';}}catch(e){console.error('Load networks error:',e);}}"
"async function addNetwork(){const ssid=document.getElementById('newNetworkSSID').value.trim();const pass=document.getElementById('newNetworkPass').value.trim();if(!ssid||!pass){alert('Please enter both SSID and password');return;}await fetch('/api/networks',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid:ssid,password:pass})});document.getElementById('newNetworkSSID').value='';document.getElementById('newNetworkPass').value='';loadKnownNetworks();refreshDashboard();}"
"async function removeNetwork(ssid){if(!confirm('Remove '+ssid+' from known networks?'))return;await fetch('/api/networks/remove',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid:ssid})});loadKnownNetworks();refreshDashboard();}"
"async function changeModel(model){await fetch('/api/settings',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({model:model})});refreshDashboard();}"
"async function sendMessage(){const input=document.getElementById('input');const msg=input.value.trim();if(!msg)return;const model=document.getElementById('modelSelect').value;document.getElementById('messages').innerHTML+='<div style=\"background:#2a2a2a;padding:12px;border-radius:8px;margin-bottom:12px\"><strong>You:</strong> '+msg+'</div>';input.value='';const res=await fetch('/api/chat',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({message:msg,model:model})});const data=await res.json();document.getElementById('messages').innerHTML+='<div style=\"background:#1a1a1a;padding:12px;border-radius:8px;margin-bottom:12px\"><strong>AI:</strong> '+data.response+'</div>';}"
"async function loadSettings(){const res=await fetch('/api/settings');const data=await res.json();document.getElementById('geminiKey').value=data.geminiKey||'';document.getElementById('chatgptKey').value=data.chatgptKey||'';document.getElementById('deepseekKey').value=data.deepseekKey||'';document.getElementById('maxTokensInput').value=data.maxTokens||'300';document.getElementById('apName').value=data.apName||'Edu-Sploit';}"
"async function saveSettings(){const data={geminiKey:document.getElementById('geminiKey').value,chatgptKey:document.getElementById('chatgptKey').value,deepseekKey:document.getElementById('deepseekKey').value,maxTokens:document.getElementById('maxTokensInput').value,apName:document.getElementById('apName').value};await fetch('/api/settings',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(data)});alert('Settings saved! Restart WebUI for AP name to take effect.');refreshDashboard();}"
"async function loadFiles(){const res=await fetch('/api/files');const data=await res.json();const list=document.getElementById('fileList');list.innerHTML='';data.files.forEach(f=>{list.innerHTML+='<div style=\"background:#1a1a1a;padding:12px;border-radius:8px;margin-bottom:8px;display:flex;justify-content:space-between\"><span>'+f+'</span><button onclick=\"viewFile(\\''+f+'\\')\" style=\"background:#333;border:none;color:#fff;padding:6px 12px;border-radius:6px;cursor:pointer\">View</button></div>';});}"
"async function viewFile(name){const res=await fetch('/api/file?name='+encodeURIComponent(name));const data=await res.json();alert('File: '+name+'\\n\\n'+data.content);}"
"setInterval(()=>{if(document.getElementById('dashboardView').style.display==='grid')refreshDashboard();},5000);"
"window.onload=()=>{const isMobile=window.innerWidth<=768;const sidebar=document.getElementById('sidebar');if(isMobile){sidebar.classList.add('hidden');}else{sidebar.classList.remove('hidden');}showDashboard();loadKnownNetworks();};"
"window.addEventListener('resize',()=>{const isMobile=window.innerWidth<=768;const sidebar=document.getElementById('sidebar');if(isMobile&&!sidebar.classList.contains('hidden')){sidebar.classList.add('hidden');}});"
"</script></body></html>";


  server.send(200, "text/html", html);
}

void handleAPIChat() {
  if (server.hasArg("plain")) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, server.arg("plain"));
    
    String message = doc["message"] | "";
    String model = doc["model"] | "gemini";
    
    currentModel = model;
    
    String response = "Response from " + model + ": " + message;
    
    DynamicJsonDocument responseDoc(2048);
    responseDoc["response"] = response;
    
    String output;
    serializeJson(responseDoc, output);
    server.send(200, "application/json", output);
  }
}

void handleGetSettings() {
  DynamicJsonDocument doc(2048);
  doc["geminiKey"] = geminiKey;
  doc["chatgptKey"] = chatgptKey;
  doc["deepseekKey"] = deepseekKey;
  doc["maxTokens"] = maxTokens;
  doc["knownNetworks"] = knownNetworks;
  doc["apName"] = ap_ssid;
  
  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}

void handleSaveSettings() {
  if (server.hasArg("plain")) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, server.arg("plain"));
    
    if (doc.containsKey("model")) {
      currentModel = doc["model"] | currentModel;
      preferences.begin("ai-config", false);
      preferences.putString("model", currentModel);
      preferences.end();
    }
    
    if (doc.containsKey("geminiKey")) {
      geminiKey = doc["geminiKey"] | geminiKey;
    }
    if (doc.containsKey("chatgptKey")) {
      chatgptKey = doc["chatgptKey"] | chatgptKey;
    }
    if (doc.containsKey("deepseekKey")) {
      deepseekKey = doc["deepseekKey"] | deepseekKey;
    }
    if (doc.containsKey("maxTokens")) {
      maxTokens = doc["maxTokens"] | maxTokens;
    }
    if (doc.containsKey("knownNetworks")) {
      knownNetworks = doc["knownNetworks"] | knownNetworks;
    }
    if (doc.containsKey("apName")) {
      ap_ssid = doc["apName"] | ap_ssid;
    }
    
    preferences.begin("ai-config", false);
    if (doc.containsKey("geminiKey")) {
      preferences.putString("geminiKey", geminiKey);
    }
    if (doc.containsKey("chatgptKey")) {
      preferences.putString("chatgptKey", chatgptKey);
    }
    if (doc.containsKey("deepseekKey")) {
      preferences.putString("deepseekKey", deepseekKey);
    }
    if (doc.containsKey("maxTokens")) {
      preferences.putString("maxTokens", maxTokens);
    }
    if (doc.containsKey("knownNetworks")) {
      preferences.putString("knownNetworks", knownNetworks);
    }
    if (doc.containsKey("apName")) {
      preferences.putString("ap_ssid", ap_ssid);
    }
    preferences.end();
    
    server.send(200, "application/json", "{\"status\":\"saved\"}");
  }
}

void handleGetStatus() {
  DynamicJsonDocument doc(2048);
  
  // Battery info
  float batteryVoltage = M5.Power.getBatteryVoltage();
  int batteryPercent = M5.Power.getBatteryLevel();
  
  JsonObject battery = doc.createNestedObject("battery");
  battery["voltage"] = batteryVoltage;
  battery["percent"] = batteryPercent;
  
  // Current model
  doc["currentModel"] = currentModel;
  doc["maxTokens"] = maxTokens;
  
  // API key status
  JsonObject apiKeys = doc.createNestedObject("apiKeys");
  apiKeys["gemini"] = geminiKey.length() > 0 ? "configured" : "not configured";
  apiKeys["chatgpt"] = chatgptKey.length() > 0 ? "configured" : "not configured";
  apiKeys["deepseek"] = deepseekKey.length() > 0 ? "configured" : "not configured";
  
  // WiFi status
  JsonObject wifi = doc.createNestedObject("wifi");
  bool wifiConnected = (WiFi.status() == WL_CONNECTED);
  wifi["connected"] = wifiConnected;
  if (wifiConnected) {
    wifi["ssid"] = WiFi.SSID();
    wifi["ip"] = WiFi.localIP().toString();
    wifi["rssi"] = WiFi.RSSI();
  } else {
    wifi["ssid"] = "";
    wifi["ip"] = "";
    wifi["rssi"] = 0;
  }
  
  // Known networks count
  int knownCount = 0;
  if (knownNetworks.length() > 0) {
    int pos = 0;
    while ((pos = knownNetworks.indexOf('\n', pos)) != -1) {
      knownCount++;
      pos++;
    }
    knownCount++;
  }
  doc["knownNetworksCount"] = knownCount;
  
  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}

void handleGetNetworks() {
  DynamicJsonDocument doc(2048);
  JsonArray networks = doc.createNestedArray("networks");
  
  // Parse known networks
  if (knownNetworks.length() > 0) {
    String networksStr = knownNetworks;
    int startPos = 0;
    
    while (startPos < networksStr.length()) {
      int endPos = networksStr.indexOf('\n', startPos);
      if (endPos == -1) {
        endPos = networksStr.length();
      }
      
      String networkEntry = networksStr.substring(startPos, endPos);
      int colonPos = networkEntry.indexOf(':');
      
      if (colonPos > 0) {
        JsonObject network = networks.createNestedObject();
        network["ssid"] = networkEntry.substring(0, colonPos);
        network["password"] = networkEntry.substring(colonPos + 1);
        // Check if available (scan if needed)
        network["available"] = false;
      }
      
      startPos = endPos + 1;
    }
  }
  
  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}

void handleAddNetwork() {
  if (server.hasArg("plain")) {
    DynamicJsonDocument doc(512);
    deserializeJson(doc, server.arg("plain"));
    
    String networkSSID = doc["ssid"] | "";
    String networkPass = doc["password"] | "";
    
    if (networkSSID.length() > 0 && networkPass.length() > 0) {
      String networkEntry = networkSSID + ":" + networkPass;
      
      // Check if already exists
      if (knownNetworks.indexOf(networkEntry) == -1) {
        if (knownNetworks.length() > 0) {
          knownNetworks += "\n" + networkEntry;
        } else {
          knownNetworks = networkEntry;
        }
        
        preferences.begin("ai-config", false);
        preferences.putString("knownNetworks", knownNetworks);
        preferences.end();
      }
    }
    
    server.send(200, "application/json", "{\"status\":\"added\"}");
  }
}

void handleRemoveNetwork() {
  if (server.hasArg("plain")) {
    DynamicJsonDocument doc(512);
    deserializeJson(doc, server.arg("plain"));
    
    String networkSSID = doc["ssid"] | "";
    
    if (networkSSID.length() > 0) {
      // Remove from known networks
      String newNetworks = "";
      String networksStr = knownNetworks;
      int startPos = 0;
      
      while (startPos < networksStr.length()) {
        int endPos = networksStr.indexOf('\n', startPos);
        if (endPos == -1) {
          endPos = networksStr.length();
        }
        
        String networkEntry = networksStr.substring(startPos, endPos);
        int colonPos = networkEntry.indexOf(':');
        
        if (colonPos > 0) {
          String entrySSID = networkEntry.substring(0, colonPos);
          if (entrySSID != networkSSID) {
            if (newNetworks.length() > 0) {
              newNetworks += "\n";
            }
            newNetworks += networkEntry;
          }
        }
        
        startPos = endPos + 1;
      }
      
      knownNetworks = newNetworks;
      preferences.begin("ai-config", false);
      preferences.putString("knownNetworks", knownNetworks);
      preferences.end();
    }
    
    server.send(200, "application/json", "{\"status\":\"removed\"}");
  }
}

void handleGetFiles() {
  DynamicJsonDocument doc(2048);
  JsonArray files = doc.createNestedArray("files");
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  
  while (file) {
    if (!file.isDirectory()) {
      String name = String(file.name());
      if (name.startsWith("/")) {
        name = name.substring(1);
      }
      files.add(name);
    }
    file = root.openNextFile();
  }
  
  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}

void handleFileUpload() {
  HTTPUpload& upload = server.upload();
  
  if (upload.status == UPLOAD_FILE_START) {
    String filename = "/" + upload.filename;
    File file = SPIFFS.open(filename, "w");
    if (file) {
      file.close();
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    String filename = "/" + upload.filename;
    File file = SPIFFS.open(filename, "a");
    if (file) {
      file.write(upload.buf, upload.currentSize);
      file.close();
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    Serial.printf("Upload complete: %s\n", upload.filename.c_str());
  }
}

void handleGetFile() {
  if (server.hasArg("name")) {
    String filename = "/" + server.arg("name");
    File file = SPIFFS.open(filename, "r");
    
    if (file) {
      String content = file.readString();
      file.close();
      
      DynamicJsonDocument doc(4096);
      doc["content"] = content;
      
      String output;
      serializeJson(doc, output);
      server.send(200, "application/json", output);
    } else {
      server.send(404, "application/json", "{\"error\":\"Not found\"}");
    }
  }
}

void stopWebUI() {
  server.stop();
  WiFi.softAPdisconnect(true);
  currentState = FILES_MENU;
  drawFilesMenu();
}

void drawSettingsMenu() {
  M5.Display.fillScreen(BLACK);
  M5.Display.setCursor(0, 0);
  M5.Display.setTextColor(CYAN);
  M5.Display.setTextSize(2);
  M5.Display.println("SETTINGS");
  M5.Display.setTextSize(1);
  M5.Display.println("");
  
  // Get current status
  float batteryVoltage = M5.Power.getBatteryVoltage();
  int batteryPercent = M5.Power.getBatteryLevel();
  bool isCharging = M5.Power.isCharging();
  
  // Display options with current values
  M5.Display.setTextColor(settingsMenuIndex == 0 ? YELLOW : WHITE);
  M5.Display.print(settingsMenuIndex == 0 ? "> " : "  ");
  M5.Display.print("Model: ");
  M5.Display.setTextColor(settingsMenuIndex == 0 ? YELLOW : WHITE);
  String upperModel = currentModel;
  upperModel.toUpperCase();
  M5.Display.println(upperModel);
  
  M5.Display.setTextColor(settingsMenuIndex == 1 ? YELLOW : WHITE);
  M5.Display.print(settingsMenuIndex == 1 ? "> " : "  ");
  M5.Display.print("Battery: ");
  M5.Display.print(batteryPercent);
  M5.Display.print("% (");
  M5.Display.print(batteryVoltage, 2);
  M5.Display.println("V)");
  
  M5.Display.setTextColor(settingsMenuIndex == 2 ? YELLOW : WHITE);
  M5.Display.print(settingsMenuIndex == 2 ? "> " : "  ");
  M5.Display.print("Max Tokens: ");
  M5.Display.println(maxTokens);
  
  M5.Display.setTextColor(settingsMenuIndex == 3 ? YELLOW : WHITE);
  M5.Display.print(settingsMenuIndex == 3 ? "> " : "  ");
  M5.Display.print("Gemini Key: ");
  M5.Display.println(geminiKey.length() > 0 ? "Set" : "Not set");
  
  M5.Display.setTextColor(settingsMenuIndex == 4 ? YELLOW : WHITE);
  M5.Display.print(settingsMenuIndex == 4 ? "> " : "  ");
  M5.Display.print("ChatGPT Key: ");
  M5.Display.println(chatgptKey.length() > 0 ? "Set" : "Not set");
  
  M5.Display.setTextColor(settingsMenuIndex == 5 ? YELLOW : WHITE);
  M5.Display.print(settingsMenuIndex == 5 ? "> " : "  ");
  M5.Display.print("DeepSeek Key: ");
  M5.Display.println(deepseekKey.length() > 0 ? "Set" : "Not set");
  
  M5.Display.setTextColor(settingsMenuIndex == 6 ? YELLOW : WHITE);
  M5.Display.print(settingsMenuIndex == 6 ? "> " : "  ");
  M5.Display.print("WiFi: ");
  if (WiFi.status() == WL_CONNECTED) {
    M5.Display.println(WiFi.SSID());
  } else {
    M5.Display.println("Disconnected");
  }
  
  M5.Display.setTextColor(settingsMenuIndex == 7 ? YELLOW : WHITE);
  M5.Display.print(settingsMenuIndex == 7 ? "> " : "  ");
  M5.Display.println("Back");
  
  M5.Display.println("");
  M5.Display.setTextColor(GREEN);
  M5.Display.println("A:Select B:Down");
}

void handleSettingsMenu() {
  if (M5.BtnA.wasPressed()) {
    switch(settingsMenuIndex) {
      case 0:
        // Change LLM Model
        currentState = MODEL_SELECT;
        modelSelectIndex = (currentModel == "gemini") ? 0 : (currentModel == "chatgpt") ? 1 : 2;
        drawModelSelect();
        break;
      case 7:
        // Back to main menu
        currentState = MAIN_MENU;
        drawMainMenu();
        break;
      default:
        // Other options - show WebUI message
        showMessage("Use WebUI to configure", 2000);
        drawSettingsMenu();
        break;
    }
  }
  
  if (M5.BtnB.wasPressed()) {
    settingsMenuIndex = (settingsMenuIndex + 1) % 8;
    drawSettingsMenu();
  }
}

void drawModelSelect() {
  M5.Display.fillScreen(BLACK);
  M5.Display.setCursor(0, 0);
  M5.Display.setTextColor(CYAN);
  M5.Display.setTextSize(2);
  M5.Display.println("SELECT MODEL");
  M5.Display.setTextSize(1);
  M5.Display.println("");
  
  const char* models[] = {"Gemini", "ChatGPT", "DeepSeek"};
  
  for (int i = 0; i < 3; i++) {
    if (i == modelSelectIndex) {
      M5.Display.setTextColor(YELLOW);
      M5.Display.print("> ");
    } else {
      M5.Display.setTextColor(WHITE);
      M5.Display.print("  ");
    }
    
    M5.Display.print(models[i]);
    if ((i == 0 && currentModel == "gemini") ||
        (i == 1 && currentModel == "chatgpt") ||
        (i == 2 && currentModel == "deepseek")) {
      M5.Display.print(" [CURRENT]");
    }
    M5.Display.println("");
  }
  
  M5.Display.println("");
  M5.Display.setTextColor(GREEN);
  M5.Display.println("A:Select B:Down PWR:Back");
}

void handleModelSelect() {
  if (M5.BtnA.wasPressed()) {
    switch(modelSelectIndex) {
      case 0:
        currentModel = "gemini";
        break;
      case 1:
        currentModel = "chatgpt";
        break;
      case 2:
        currentModel = "deepseek";
        break;
    }
    
    // Save to preferences
    preferences.begin("ai-config", false);
    preferences.putString("model", currentModel);
    preferences.end();
    
    showMessage("Model saved!", 1000);
    currentState = SETTINGS_MENU;
    drawSettingsMenu();
  }
  
  if (M5.BtnB.wasPressed()) {
    modelSelectIndex = (modelSelectIndex + 1) % 3;
    drawModelSelect();
  }
  
  if (M5.BtnPWR.wasPressed()) {
    currentState = SETTINGS_MENU;
    drawSettingsMenu();
  }
}

void showMessage(String msg, int duration) {
  M5.Display.fillScreen(BLACK);
  M5.Display.setCursor(0, 0);
  M5.Display.setTextColor(YELLOW);
  M5.Display.println(msg);
  if (duration > 0) delay(duration);
}