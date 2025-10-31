# Edu-Sploit - AI-Powered Cheating Device (Proof of Concept)

⚠️ **EDUCATIONAL PROOF OF CONCEPT ONLY** ⚠️

This project is a proof-of-concept demonstration of how AI technology could potentially be misused as a cheating device in educational settings. It is intended **solely for educational and security research purposes** to raise awareness about academic integrity challenges in the age of AI.

## ⚠️ Disclaimer

**DO NOT USE THIS FOR ACTUAL CHEATING.** Using this device or similar technology to cheat on exams, assignments, or any academic work is:
- Unethical and violates academic integrity policies
- May result in severe academic penalties including expulsion
- Could have long-term consequences for your education and career
- Undermines the value of education and personal learning

This project exists to demonstrate security vulnerabilities and promote discussions about academic integrity in modern education.

## 📱 What It Does

Edu-Sploit is a compact, pocket-sized device built on the M5StickC Plus2 platform that provides:

- **Multi-Model AI Chat**: Access to Gemini 2.0, ChatGPT, and DeepSeek AI models
- **Portable Interface**: Small LCD screen with on-device keyboard input
- **Serial Input Support**: Quick text entry via USB serial connection
- **Web UI**: Full-featured web interface accessible via WiFi hotspot
- **File Storage**: Upload and view reference materials on SPIFFS
- **Multi-Mode Responses**: Direct Q&A, blog posts, summaries, code help, and more
- **Auto-Connect**: Automatically connects to known WiFi networks on boot
- **Stealth Design**: Fits in a pocket, looks like a regular electronic device

## 🔧 Hardware Requirements

- M5StickC Plus2 (ESP32-based device)
- USB-C cable for programming and serial input
- Battery power for portable operation

## 🚀 Features

### On-Device Interface
- Menu-driven navigation with physical buttons
- Virtual keyboard for text input
- Scrollable response viewing
- File manager for uploaded documents
- Settings configuration
- Model selection (Gemini/ChatGPT/DeepSeek)

### Web UI (Access Point Mode)
- Modern, responsive web interface
- Dashboard with battery, API status, and network info
- Chat interface with model selection
- File upload and management
- Settings panel for API keys and configuration
- Known networks manager with auto-connect
- Mobile and desktop optimized

### Multi-Model Support
- **Gemini 2.0 Flash**: Fast, efficient responses
- **ChatGPT (GPT-4)**: Advanced reasoning and analysis
- **DeepSeek**: Alternative AI perspective

### Response Modes
1. **Direct Q&A**: Straightforward answers
2. **Blog Post**: Detailed, engaging content
3. **Email Writer**: Professional correspondence
4. **Social Media**: Engaging posts
5. **Code Helper**: Programming assistance with comments
6. **Summary**: Concise information extraction

## 📋 Setup Instructions

### 1. Hardware Setup
```bash
# Install Arduino IDE and M5StickC Plus2 board support
# Install required libraries:
- M5StickCPlus2
- ArduinoJson
- WiFi
- HTTPClient
- WebServer
- Preferences
- SPIFFS
```

### 2. Configuration
1. Obtain API keys:
   - [Google AI Studio](https://makersuite.google.com/app/apikey) for Gemini
   - [OpenAI Platform](https://platform.openai.com/api-keys) for ChatGPT
   - [DeepSeek](https://platform.deepseek.com/) for DeepSeek

2. Configure in code or via WebUI:
   - WiFi credentials
   - API keys for desired models
   - Access point name/password
   - Max token limits

### 3. Upload to Device
```bash
# Compile and upload via Arduino IDE
# Monitor serial output for connection status
```

## 🌐 Usage

### On-Device Mode
1. **Power on** the device
2. Navigate menus with **Button A** (select) and **Button B** (down)
3. **Chat Menu**: Start new conversation or continue previous
4. **Files Menu**: Access WebUI or view uploaded files
5. **Settings Menu**: Check status and change AI model
6. **Hold Button C (PWR)**: Return to main menu from anywhere

### Keyboard Input
- Navigate with buttons B (horizontal) and C (vertical)
- Button A: Select character (hold to send message)
- Button B: Move right (hold for space, hold longer for left)
- Serial input: Type directly via USB connection

### Web UI Mode
1. Select **Files → WebUI**
2. Connect to WiFi hotspot (default: "Edu-Sploit" / "12345678")
3. Navigate to displayed IP address
4. Access full dashboard, chat, files, and settings

## 🔒 Security & Privacy Notes

- API keys are stored in device preferences (encrypted by ESP32)
- All AI communication happens over HTTPS
- No conversation history stored on device
- WiFi credentials saved for auto-connect
- Web UI accessible only when explicitly activated

## 🎓 Educational Implications

This proof of concept highlights several critical issues:

1. **AI Accessibility**: Modern AI APIs are easily integrated into portable devices
2. **Detection Challenges**: Small form factor makes physical detection difficult
3. **Network Security**: Requires internet/API access, creating detectable network traffic
4. **Policy Gaps**: Traditional anti-cheating measures don't address AI-enabled devices
5. **Academic Integrity**: Need for updated honor codes and integrity education

## 🛡️ Countermeasures for Educators

To prevent misuse of devices like this:

- **Signal blocking**: Use Faraday bags or RF-blocking exam rooms
- **Device policies**: Ban all electronic devices during assessments
- **Network monitoring**: Detect unusual API traffic patterns
- **Physical checks**: Metal detectors, visual inspection
- **Assessment redesign**: Focus on in-person, practical demonstrations
- **AI-resistant questions**: Open-ended, creative, or application-based problems
- **Proctoring**: In-person supervision with clear line of sight

## 📚 Legal & Ethical Considerations

- Using this for actual cheating violates academic integrity policies
- May violate computer fraud and abuse laws depending on jurisdiction
- API usage subject to terms of service from providers
- Educational institutions may pursue legal action against cheaters
- This repository is for **security research and awareness only**

## 🤝 Contributing

This is a proof-of-concept for research purposes. If you have ideas for:
- Detection methods
- Countermeasures
- Educational applications
- Security improvements

Please open an issue or pull request.

## 🙏 Acknowledgments

- M5Stack for the M5StickC Plus2 platform
- OpenAI, Google, and DeepSeek for AI APIs
- Security researchers raising awareness about academic integrity

---

**Remember**: The goal of education is learning, not grades. Cheating only cheats yourself out of knowledge and skills you'll need in life. Use AI as a learning tool, not a crutch.
