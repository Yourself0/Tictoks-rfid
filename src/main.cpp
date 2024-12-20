#include <Arduino.h>
#define FirmwareVer "0.1"
#include <FS.h>
#include <SPI.h>
#include <WiFi.h>
#include <Wire.h>
#include <RTClib.h> //For RTC DS3231
#include <SPIFFS.h>
#include <EEPROM.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <HTTPClient.h>
#include "soc/rtc_wdt.h"
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <esp_task_wdt.h>
#include <arduino-timer.h>
#include <ESPAsyncWebServer.h>
#include <MFRC522.h>
#include <MFRC522Extended.h>

auto timer = timer_create_default(); // create a timer with default settings
bool Network_status = true;
bool SpiffsTimerStart = false;
bool serverOn = false;
String json;
// String EmployeeListsData;
RTC_DS3231 rtc;
// Create AsyncWebServer object on port 80
int serverUpdateCount = 0;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

TaskHandle_t UpdateEmployeeDetail;
TaskHandle_t SpiffsOfflineDataC;
bool EmployeeListsHigh = false;
bool wifiScanRequested = false;
bool UpdateEmployee = false;
String DeviceList;

long OtpVerifiy;
bool WifiPage = false;
bool quicksetupCld = false;
bool FingerAlreadyExist = false;
bool RfidRegisterPage = false;
bool CompanyPage = false;
bool DeviceidPage = false;
bool RfidRegister = false;
bool WebsocketConnected = false;
bool RegistrationFinger = true;
bool Fetching = true;
bool OpenDoors = false;
bool CloseDoors = false;
bool OrganizationStatuss;
int i = 0;
// String JsonStringss;
#define Relay 4
#define Buzzer 15
#define REDLED 12
#define REDLED1 25
#define GREENLED 13
#define ORANGELED 14
#define GREENLED1 33
// RFID pins
#define SS_1_PIN 2
#define SS_2_PIN 26
#define RST_PIN 27
#define NO_OF_READERS 2
byte ssPins[] = {SS_1_PIN, SS_2_PIN};

unsigned long previousMillis = 0;
unsigned long firmwareUpdateInterval = 86400000; // 1 hour (adjustable)

// unsigned long firmwareUpdateInterval = 600000; // Test one minute

WiFiClient espClient;
PubSubClient clients(espClient);

// Access Point SSID and PASSWORD ..

// need to work on versions
const char *ASSID = "Tictoks RF V2";
const char *APASS = "123456789";

const char *host = "www.google.com";

/*Git hub updated url*/

const int gitport = 443;
const char *gitHost = "raw.githubusercontent.com";
#define URL_fw_Version "https://raw.githubusercontent.com/Yourself0/Throughapps_rfid/main/firmware_version.txt"
#define URL_fw_Bin "https://raw.githubusercontent.com/Yourself0/Throughapps_rfid/main/firmware.bin"

String hostName = "tictoksrfid";

// Registering RFIDS

// SPIFFS File Decleration .
char EMPDETAILS[] = "/EmpRfid.csv";
String CompanyId = "";
String CompanyName = "";
String DeviceType = "RFID";
String DeviceId = "";
char servername[] = "www.google.com";
bool fetchRunning_update = false;
MFRC522 mfrc522[NO_OF_READERS];
WiFiClient client;
#define WATCHDOG_TIMEOUT_SECONDS 10

TaskHandle_t WiFiscanTaskHandleC;
TaskHandle_t WifiStatusDataC = NULL;
TaskHandle_t UpdateEmployeeDetailC = NULL;
TaskHandle_t OpenDoorC = NULL;
TaskHandle_t CloseDoorC = NULL;
TaskHandle_t SdOfflineDataC = NULL;
TaskHandle_t printTaskHandleC = NULL;
TaskHandle_t OrganisationStatusC = NULL;

void OfflineDataWrite(String empId);
void rfidInitialList();
void FirmwareUpdate();
bool pingServer();
void MDNSServer();

void printFreeHeap(const char *position)
{
  Serial.print(position);
  Serial.print(" Free Heap: ");
  Serial.println(ESP.getFreeHeap());
}

void setupWatchdogTimer()
{
  // Configure the Watchdog Timer timeout
  esp_task_wdt_init(WATCHDOG_TIMEOUT_SECONDS, true); // Set true to enable panic (reset) when timeout is reached
  esp_task_wdt_add(NULL);                            // NULL means the main task, add other tasks if needed
}

void WifiStatusNotConnected()
{
  Serial.print("Sent not connected");
  // ws1.textAll("Not Connected");
  digitalWrite(ORANGELED, HIGH);
}

void WifiStatusConnected()
{
  // ws1.textAll("Connected");
  Serial.println("Sent Connected");
  digitalWrite(ORANGELED, LOW);
}

void rfidInitialCheck()
{
  // mountinSD();
  if (SPIFFS.begin(true))
  {
    if (!SPIFFS.exists("/Rfid.csv"))
    {
      Serial.println("Rfid register File Doesn't Exist");
      rfidInitialList();
    }
    else
    {
      File DataFile = SPIFFS.open("/Rfid.csv");
      size_t fileSize = DataFile.size();
      Serial.println("Rfid Register File Exists :");
      Serial.println(fileSize);
      if (fileSize == 0)
      {
        DataFile.close();
        SPIFFS.remove("/Rfid.csv");
        Serial.println("File Removed");
      }
      else
      {
        Serial.print("File Already There");
        DataFile.close();
      }
    }
  }
  else
  {
    Serial.println("Sd is not There");
  }
}

void fileReadAndWrite()
{
  File data = SPIFFS.open("/EmpRfid.csv", "w");
  File datafile = SPIFFS.open("/Rfid.csv", FILE_READ);
  if (!datafile)
  {
    Serial.println("Failed to open file for reading");
    return;
  }
  String line;
  bool rfidFound = false;
  while (datafile.available())
  {
    line = datafile.readStringUntil('\n');
    int commaIndex = line.indexOf(',');
    int secondCommaIndex = line.indexOf(',', commaIndex + 1);
    int thirdCommaIndex = line.indexOf(',', secondCommaIndex + 1);
    int fourthCommaIndex = line.indexOf(',', thirdCommaIndex + 1);
    int fifthCommaIndex = line.indexOf(',', fourthCommaIndex + 1);
    int sixthCommaIndex = line.indexOf(',', fifthCommaIndex + 1);
    if (commaIndex == -1)
    {
      continue;
    }
    String Rfid = line.substring(0, commaIndex);
    // String DeviceIds = line.substring(commaIndex + 1, secondCommaIndex);
    String Empid = line.substring(secondCommaIndex + 1, thirdCommaIndex);
    // String BioTemplateId = line.substring(fifthCommaIndex + 1, sixthCommaIndex);
    // String BioRegsStatus = line.substring(sixthCommaIndex + 1);

    // Serial.println("BioRegsStatus: " + BioRegsStatus);
    // Serial.println("Bio Temp" + BioTemplateId);
    // int BioTemplateIntId = BioTemplateId.toInt();
    // int BioRegsIntStatus = BioRegsStatus.toInt();
    // Serial.print("bio Int id ");
    // Serial.println(BioTemplateIntId);
    esp_task_wdt_reset();
    if (Rfid.length() > 0)
    {
      if (Rfid != "-")
      {
        esp_task_wdt_reset();
        String content = Rfid + String(",") + Empid + "\n";
        Serial.print("Inside Content : ");
        Serial.println(content);
        data.print(content);
      }
    }
    // Serial.println("Rfid and Deviceids "+Rfid+" "+DeviceIds);
    // Process the first two fields (e.g., print them)
  }
  data.close();
  datafile.close();
}

void DeviceIdInitialize()
{
  EEPROM.begin(512);
  String deviceId = "";
  for (int i = 166; i <= 170; ++i)
  {
    if (EEPROM.read(i) != 0)
    {
      if (EEPROM.read(i) != 255)
      {
        deviceId += char(EEPROM.read(i));
        Serial.println("Device ID:");
        Serial.print(EEPROM.read(i));
        Serial.println(deviceId);
      }
    }
  }
  if (deviceId.length() > 0)
  {
    DeviceId = deviceId;
    Serial.println("Device Id was there");
  }
  else
  {
    Serial.println("Device id Not there");
  }
}

// Initiliaze RFID Reader
/*
void InitializeRfid()
{
  SPI.begin();
  for (uint8_t reader = 0; reader < NO_OF_READERS; reader++)
  {
    mfrc522[reader].PCD_Init(ssPins[reader], RST_PIN);
    Serial.println(F("RFID "));
    Serial.println(reader);
    Serial.println(F(": "));
    mfrc522[reader].PCD_DumpVersionToSerial();
  }
}

*/

/**/
void InitializeRfid()
{
  SPI.begin(); // Start SPI communication
  mfrc522->PCD_SoftPowerUp();
  for (uint8_t reader = 0; reader < NO_OF_READERS; reader++)
  {
    Serial.print(F("Initializing RFID reader "));
    Serial.println(reader);

    mfrc522[reader].PCD_Init(ssPins[reader], RST_PIN); // Initialize reader

    delay(100); // Allow the module to stabilize

    // Check firmware version
    Serial.print(F("RFID "));
    Serial.print(reader);
    Serial.print(F(": Firmware Version: "));
    byte version = mfrc522[reader].PCD_ReadRegister(MFRC522::VersionReg);

    if (version == 0x0)
    {
      Serial.println(F("Unknown (communication failure)"));
    }
    else
    {
      mfrc522[reader].PCD_DumpVersionToSerial();
    }
  }
}

void mountingSpiffs()
{
  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  else
  {
    Serial.println("Mount SPIFFS successfully");
  }
}

void Delete_activity(String lineToDelete, String empid)
{
  mountingSpiffs();
  const char *filename = "/EmpRfid.csv";
  if (!SPIFFS.exists(filename))
  {
    Serial.println("File does not exist");
    return;
  }
  File file = SPIFFS.open(filename, "r");
  if (!file)
  {
    Serial.println("Failed to open file for reading");
    return;
  }
  Serial.print("Delete Activity ->");
  Serial.println(String("RFID: ") + lineToDelete + String(" EMPID: ") + empid);
  String fileContent = "";
  bool lineFound = false;
  while (file.available())
  {
    String line = file.readStringUntil('\n');
    int commaIndex = line.indexOf(',');
    String empidExtract = line.substring(commaIndex + 1);
    if (empidExtract.equals(empid))
    {
      Serial.println("Found it was there so skipped it ");
      lineFound = true;
    }
    else
    {
      fileContent += line + "\n";
    }
  }
  file.close();
  if (lineFound)
  {
    file = SPIFFS.open(filename, "w");
    if (!file)
    {
      Serial.println("Failed to open file for writing");
      return;
    }
    file.print(fileContent);
    file.close();
    Serial.println("Line deleted successfully");
  }
  else
  {
    Serial.println("Line not found in the file");
  }
}

void Update_activity(String lineToDelete, String empid)
{
  const char *filename = "/EmpRfid.csv";
  mountingSpiffs();
  if (!SPIFFS.exists(filename))
  {
    Serial.println("File does not exist");
    return;
  }
  File file = SPIFFS.open(filename, "r");
  if (!file)
  {
    Serial.println("Failed to open file for reading");
    return;
  }
  Serial.print("Update Activity: ");
  Serial.print(String("RFID: ") + lineToDelete + String(" EMPID: ") + empid);
  String fileContent = "";
  bool lineFound = false;

  while (file.available())
  {
    String line = file.readStringUntil('\n');
    int commaIndex = line.indexOf(',');
    String empidExtract = line.substring(commaIndex + 1);

    if (empidExtract.equals(empid))
    {
      Serial.print("File Updated: ");
      lineFound = true;
      line = lineToDelete + String(",") + empid; // Update the line
    }

    fileContent += line + "\n";
  }

  file.close();

  if (!lineFound && empid.length() > 0)
  {
    fileContent += lineToDelete + String(",") + empid + "\n";
    Serial.println("Added at the last of the File ");
  }

  file = SPIFFS.open(filename, "w");
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }

  file.print(fileContent);
  file.close();

  if (lineFound)
  {
    Serial.println("Line updated successfully");
  }
  else
  {
    Serial.println("Line not found; Added data to file");
  }
}

void UpdateActivity(void *pvParameters)
{
  (void)pvParameters; // Unused parameter

  while (true)
  {
    Serial.println("first delay started");
    vTaskDelay(pdMS_TO_TICKS(600000)); // Delay for 10 minutes
    Serial.println("first delay Ended");
    Serial.println("second Delay");
    vTaskDelay(pdMS_TO_TICKS(600000)); // Delay for 10 minutes
    Serial.println("Second delay Ended");
    Serial.println("second Delay");
    vTaskDelay(pdMS_TO_TICKS(600000)); // Delay for 10 minutes

    // Test
    // vTaskDelay(pdMS_TO_TICKS(10000)); // Delay for 10 minutes

    if (true)
    {
      bool pingcheck = pingServer();
      if (pingcheck)
      {

        serverOn = true;
        mountingSpiffs();
        DeviceIdInitialize();
        int dataCount = 0;
        int startCount = 0;
        const int endCount = 5;
        int loop_count = 0;
        int httpCode = 0;
        int falseReturn = 0;
        DateTime now = rtc.now();
        int minutes_start = now.minute();
        Serial.print("Start Time :");
        Serial.print(minutes_start);
        do
        {
          digitalWrite(REDLED, HIGH);
          digitalWrite(GREENLED, HIGH);
          digitalWrite(REDLED1, HIGH);
          digitalWrite(GREENLED1, HIGH);
          HTTPClient http; // Declare object of class HTTPClient
          // Parse the JSON data
          DynamicJsonBuffer jsonBuffers;
          JsonObject &JSONEncoder = jsonBuffers.createObject();
          JSONEncoder["companyId"] = CompanyId;
          JSONEncoder["deviceId"] = DeviceId;
          JSONEncoder["startCount"] = startCount;
          JSONEncoder["endCount"] = endCount;
          char JSONmessageBuffer[500]; // Adjust the buffer size as needed
          JSONEncoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
          Serial.println(JSONmessageBuffer);
          Serial.println("Hello");
          http.begin("https://wildfly.letzcheckin.com:443/EmployeeAttendenceAPI/RFIDAPI/AutoSyncEmployeeData");
          Serial.println("http begin");
          http.addHeader("Content-Type", "application/json"); // Specify content-type header
          Serial.println("Post Main");
          // Sending requests startes here ..
          httpCode = http.POST(JSONmessageBuffer); // Send the request
          Serial.print("HTTP Code ");
          Serial.println(httpCode);
          yield();
          String payload = http.getString();
          char *payloadBuffer; // Adjust the size based on your requirement
          payload.toCharArray(payloadBuffer, sizeof(payloadBuffer));
          Serial.print("pay load :");
          Serial.println(payload);
          if (httpCode == 200)
          {
            // Convert the JSON object to a string and print it
            Serial.println("RFID response");
            JsonObject &root = jsonBuffers.parseObject(payload); // Parse the JSON payload dynamically
            payload = "";
            if (!root.success())
            {
              Serial.println("Parsing failed!");
            }
            if (root.success())
            {
              if (root.size() > 0)
              {
                Serial.println("Root success ahd have value Update");
              }
            }
            Serial.print("Json string : ");
            root.printTo(Serial);
            Serial.print("root size :");
            Serial.println(root.size());
            // JsonArray& empRfidList = root["empRfidList"]; to this employeeInfoList make sure u change ...
            JsonArray &empRfidList = root["employeeInfoList"];
            Serial.print("Data Count :");
            Serial.print(dataCount);
            Serial.print("emp rfid size:");
            Serial.print(empRfidList.size());
            Serial.println();
            Serial.print("JSON NODES");
            // root.remove("employeeInfoList");
            String jsonString;
            empRfidList.printTo(jsonString);
            JsonArray &nodes = jsonBuffers.parseArray(jsonString);
            // new writting code here
            if (!nodes.success())
            {
              Serial.println("parseObject() failed");
              jsonBuffers.clear();
            }
            else
            {
              String dataCountStr = root["dataCount"].as<char *>();
              dataCount = dataCountStr.toInt();
              startCount += endCount;
              int node_length = nodes.size();
              for (int i = 0; i < node_length; i++)
              {
                File dataFile = SPIFFS.open("/EmpRfid.csv", "a");
                Serial.printf("node-%i\nEmployee : ", i);
                String empId = nodes[i]["employeeId"].as<const char *>();
                String rfid = nodes[i]["rfidNo"].as<const char *>();
                String Activity = nodes[i]["activity"].as<const char *>();
                Serial.println("EmpID :");
                Serial.println(empId);
                Serial.print("RFID : ");
                Serial.println(rfid);
                if (Activity == "Add_Employee")
                {
                  if (rfid.equals(""))
                  {
                    rfid = "0";
                    Serial.println("Rfid Empty");
                  }
                  String empData = "";
                  // newly added ..
                  Serial.println("RFID:");
                  Serial.println(rfid.c_str());
                  empData = rfid + String(",") + empId + String("\n"); // Writting employee data to SPIFFS
                  Serial.println(empData.c_str());
                  dataFile.print(empData);
                  Serial.println("Written");
                  dataFile.close();
                }
                else if (Activity == "Update_Employee")
                {
                  // check file and update the employee
                  Update_activity(rfid, empId);
                }
                else if (Activity == "Deleted_Employee")
                {
                  Delete_activity(rfid, empId);
                }
                else if (Activity == "UnBlocked_Employee")
                {
                  Update_activity(rfid, empId);
                }
                else if (Activity == "Blocked_Employee")
                {
                  Delete_activity(rfid, empId);
                }
                else if (Activity == "UnLocked_Employee")
                {
                  Update_activity(rfid, empId);
                }
                else if (Activity == "Locked_Employee")
                {
                  Delete_activity(rfid, empId);
                }
              }
              Serial.println("Forloop end ");
            }
            Serial.println("ELSe end ");
            // Close connection
            yield();
            Serial.println(httpCode);
            Serial.println("SucessFully Stored RFID Values");
          }
          else
          {
            falseReturn += 1;
            if (falseReturn > 50)
            {
              Serial.println("Http response Failed");
              digitalWrite(REDLED, LOW);
              digitalWrite(GREENLED, LOW);
              digitalWrite(REDLED1, LOW);
              digitalWrite(GREENLED1, LOW);
              serverOn = false;
              http.end();
              break;
            }
          }
          http.end();
        } while (dataCount >= startCount);

        int minute_end = now.minute();
        int total_minutes = minutes_start - minute_end;
        Serial.print("total Time Took : ");
        Serial.println(total_minutes);
        // SPI.begin();         // Init SPI bus
        for (uint8_t reader = 0; reader < NO_OF_READERS; reader++)
        {
          mfrc522[reader].PCD_Reset(); // Perform a software reset
          mfrc522[reader].PCD_Init();  // Init MFRC522
        }
        EEPROM.write(510, 1);
        EEPROM.commit(); // Save data to EEPROM
                         // return httpCode;
      }
      digitalWrite(REDLED, LOW);
      digitalWrite(GREENLED, LOW);
      digitalWrite(REDLED1, LOW);
      digitalWrite(GREENLED1, LOW);
      serverOn = false;
    }
    vTaskDelay(30000 / portTICK_PERIOD_MS);
  }
}

bool SendOfflineDataSpiffs(void *)
{
  if (serverUpdateCount > 8)
  {
    serverOn = false;
  }
  Serial.print("SPIFFS Server update");
  Serial.println(serverOn);
  if (!serverOn)
  {
    Serial.println("spiff time true");
    SpiffsTimerStart = true;
    return true;
  }
  else
  {
    serverUpdateCount += 1;
    SpiffsTimerStart = false;
    return true;
  }
  return true;
}

void InitializeRTC()
{
  Wire.begin();
  if (!rtc.begin())
  {
    Serial.println("RTC is NOT RUNNING");
  }
  else
  {
    if (rtc.lostPower())
    {
      Serial.println("RTC is Turned off and Adjusted the time");
      rtc.adjust(DateTime(F(__DATE__), (F(__TIME__))));
    }
    Serial.println("RTC is RUNNING");
    DateTime now = rtc.now();
    Serial.print(now.year());
    Serial.print(now.month());
    Serial.print(now.day());
  }
}

void WifiConnectCheck()
{
  EEPROM.begin(512);
  String wssid = "";
  String wpass = "";
  for (int i = 0; i < 32; ++i)
  {
    if (EEPROM.read(i) != 255)
    {
      if (EEPROM.read(i) != 0)
      {
        wssid += char(EEPROM.read(i));
      }
    }
  }
  for (int i = 32; i < 94; ++i)
  {
    if (EEPROM.read(i) != 255)
    {
      if (EEPROM.read(i) != 0)
      {
        wpass += char(EEPROM.read(i));
      }
    }
  }
  if (wssid.length() > 0)
  {
    Serial.print("WiFi : ");
    Serial.println(wssid);
    Serial.print("Pass :");
    Serial.println(wpass);
    WiFi.begin(wssid, wpass);
    int count = 0;
    while (WiFi.status() != WL_CONNECTED && count < 12)
    {             // Adjust attempts as needed
      delay(500); // Reduced delay for faster attempts
      count += 1;
      Serial.println("Connecting to WiFi..");
      esp_task_wdt_reset();
    }
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("Connected to WiFi");
      Serial.print("IP Address ");
      Serial.println(WiFi.localIP());
      WiFi.scanDelete();
    }
    else
    {
      WiFi.scanDelete();
      Serial.println("Not Connected");
    }
  }
  else
  {
    Serial.println("WiFi was not registered");
  }
}

long TokenVerifications()
{
  long randomNumber = random(100000, 1000000);
  return randomNumber;
}

void ResetWebserverPages()
{
  WifiPage = false;
  RfidRegisterPage = false;
  CompanyPage = false;
  DeviceidPage = false;
}

// Updating Employee Details --FetchEmployee--  ..
void updateEmployeeDetails(void *pvParameters)
{
  for (;;)
  {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    if (true)
    {
      bool pingcheck = pingServer();
      if (pingcheck)
      {

        mountingSpiffs();
        serverOn = true;
        DeviceIdInitialize();
        int dataCount = 0;
        int startCount = 0;
        const int endCount = 15;
        int loop_count = 0;
        int httpCode = 0;
        int falseReturn = 0;
        File dataFile = SPIFFS.open("/Rfid.csv", "w");
        if (!dataFile)
        {
          Serial.println("Failed to open file for writing");
        }
        if (dataFile)
        {
          Serial.println("File Opend SuccessFully");
        }
        do
        {
          digitalWrite(REDLED, HIGH);
          digitalWrite(GREENLED, HIGH);
          digitalWrite(REDLED1, HIGH);
          digitalWrite(GREENLED1, HIGH);
          HTTPClient http; // Declare object of class HTTPClient
          // Parse JSON object
          // Parse the JSON data
          DynamicJsonBuffer jsonBuffers;
          // DynamicJsonBuffer JSONbuffers_obj;
          JsonObject &JSONEncoder = jsonBuffers.createObject();
          JSONEncoder["companyId"] = CompanyId;
          JSONEncoder["deviceId"] = DeviceId;
          JSONEncoder["startCount"] = startCount;
          JSONEncoder["endCount"] = endCount;
          char JSONmessageBuffer[500]; // Adjust the buffer size as needed
          JSONEncoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
          Serial.println(JSONmessageBuffer);
          Serial.println("Hello");
          http.begin("https://wildfly.letzcheckin.com:443/EmployeeAttendenceAPI/RFIDAPI/SelectEmployeeInfo");
          Serial.print("http begin");
          http.addHeader("Content-Type", "application/json"); // Specify content-type header
          Serial.println("Post ");
          // Sending requests startes here ..
          httpCode = http.POST(JSONmessageBuffer); // Send the request
          Serial.print("HTTP Code ");
          Serial.println(httpCode);
          yield();
          String payload = http.getString();
          char *payloadBuffer; // Adjust the size based on your requirement
          payload.toCharArray(payloadBuffer, sizeof(payloadBuffer));
          Serial.print("pay load :");
          Serial.println(payload);
          if (httpCode == 200)
          {
            // Convert the JSON object to a string and print it
            Serial.println("RFID response");
            JsonObject &root = jsonBuffers.parseObject(payload); // Parse the JSON payload dynamically
            payload = "";
            if (!root.success())
            {
              Serial.println("Parsing failed!");
            }
            if (root.success())
            {
              if (root.size() > 0)
              {
                String dataCountStr = root["dataCount"].as<char *>();
                dataCount = dataCountStr.toInt();
                startCount += endCount;
              }
            }
            Serial.print("Json string : ");
            root.printTo(Serial);
            Serial.print("root size :");
            Serial.println(root.size());
            // JsonArray& empRfidList = root["empRfidList"]; to this employeeInfoList make sure u change ...
            JsonArray &empRfidList = root["employeeInfoList"];
            Serial.print("Data Count :");
            Serial.print(dataCount);
            Serial.print("emp rfid size:");
            Serial.print(empRfidList.size());
            Serial.println();
            Serial.print("JSON NODES");
            String jsonString;
            empRfidList.printTo(jsonString);
            JsonArray &nodes = jsonBuffers.parseArray(jsonString);
            // new writting code here
            if (!nodes.success())
            {
              Serial.println("parseObject() failed");
              jsonBuffers.clear();
            }
            else
            {
              int node_length = nodes.size();
              for (int i = 0; i < node_length; i++)
              {
                File dataFile = SPIFFS.open("/Rfid.csv", "a");
                Serial.printf("node-%i\nEmployee : ", i);
                String empId = nodes[i]["employeeId"].as<const char *>();
                String rfid = nodes[i]["rfidNo"].as<const char *>();
                Serial.println("EmpID :");
                Serial.println(empId);
                Serial.print("RFID : ");
                Serial.println(rfid);
                if (rfid.equals(""))
                {
                  rfid = "0";
                  Serial.println("Rfid Empty");
                }
                String empData = "";
                // newly added ..
                Serial.println("RFID:");
                Serial.println(rfid.c_str());
                empData = rfid + String(",") + empId + String("\n"); // Writting employee data to SPIFFS
                Serial.println(empData.c_str());
                // csv.addField(empData);
                // csv.addLine();
                dataFile.print(empData);
                dataFile.close();
                Serial.println("Written");
              }
              Serial.println("Forloop end ");
            }
            Serial.println("ELSe end ");
            // endCount += 15;
            http.end(); // Close connection
            yield();
            Serial.println(httpCode);
          }
          else
          {
            falseReturn += 1;
            if (falseReturn > 2)
            {
              Serial.println("Http response Failed");
              serverOn = false;
              digitalWrite(REDLED, LOW);
              digitalWrite(GREENLED, LOW);
              digitalWrite(REDLED1, LOW);
              digitalWrite(GREENLED1, LOW);
              http.end();
              serverOn = false;
              break;
            }
          }
        } while (dataCount >= startCount);
        Serial.println("SucessFully Stored RFID Values");
        serverOn = false;
        EEPROM.write(510, 1);
        EEPROM.commit(); // Save data to EEPROM
        UpdateEmployee = false;
        fileReadAndWrite();
      }
    }

    digitalWrite(REDLED, LOW);
    digitalWrite(GREENLED, LOW);
    digitalWrite(REDLED1, LOW);
    digitalWrite(GREENLED1, LOW);
  }
  vTaskDelay(30000 / portTICK_PERIOD_MS);
}

void SdOfflineData(void *parameter)
{
  while (true)
  {
    SpiffsTimerStart = true;
    Serial.println("Spiffs Start :" + String(SpiffsTimerStart));
    if (SpiffsTimerStart)
    {
      Serial.println("Sending offline data to server");
      Serial.println("Wifi status connected :" + String(WebsocketConnected));
      bool pingcheck = pingServer();
      if (pingcheck)
      {
        digitalWrite(ORANGELED, LOW);
        Network_status = false;

        HTTPClient http; // Declare object of class HTTPClient

        Network_status = true;
        if (SPIFFS.exists("/OfflineData.csv"))
        {
          File csv = SPIFFS.open("/OfflineData.csv", "r");
          if (!csv)
          {
            Serial.println("Failed to open file");
            continue;
          }

          Serial.println("Connected to Internet ./");
          Serial.println("Wifi status connected " + String(WebsocketConnected));
          if (csv.available())
          {
            Serial.println("Sending Offline data to server");
            const byte BUFFER_SIZE = 200;
            char buffer[BUFFER_SIZE + 1];
            buffer[BUFFER_SIZE] = '\0';
            int file_count = 0;
            int success_count = 0;

            while (csv.available())
            {
              int count = 0;
              file_count += 1;
              Serial.println("file read fun");
              String line = csv.readStringUntil('\n');
              Serial.println("line");
              Serial.print(line);

              strncpy(buffer, line.c_str(), sizeof(buffer));
              char *ptr = strtok(buffer, ",");
              int j = 0;
              String EmpId, Date, Temp_Time;
              while (ptr != NULL)
              {
                if (j == 0)
                {
                  EmpId = ptr;
                }
                else if (j == 1)
                {
                  Date = ptr;
                }
                else if (j == 2)
                {
                  Temp_Time = ptr;
                }
                j++;
                ptr = strtok(NULL, ",");
              }

              Serial.println("");
              Serial.print("Company id: ");
              Serial.println(CompanyId);
              Serial.print("EmployeeID: ");
              Serial.println(EmpId);
              Serial.print("Date: ");
              Serial.println(Date);
              Serial.print("Time: ");
              Serial.println(Temp_Time);

              if (EmpId != "")
              {
                String Time = "";
                for (int i = 0; i < Temp_Time.length(); i++)
                {
                  char currentChar = Temp_Time.charAt(i);
                  if (currentChar != '\r')
                  {
                    Time += currentChar;
                  }
                }

                DynamicJsonBuffer JSONbuffer;
                JsonObject &JSONencoder = JSONbuffer.createObject();
                JSONencoder["employeeId"] = EmpId;
                JSONencoder["companyId"] = CompanyId;
                JSONencoder["deviceType"] = DeviceType;
                JSONencoder["date"] = Date;
                JSONencoder["time"] = Time;
                char JSONmessageBuffer[300];
                JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
                Serial.print("JSON MESSAGE server send");
                Serial.println(JSONmessageBuffer);

                http.begin("http://3.6.171.29:8080/EmployeeAttendenceAPI/employee/EmployeeCheckInOut");
                http.addHeader("Content-Type", "application/json");
                Serial.println("Here after content type");
                int httpCode = http.POST(JSONmessageBuffer);
                Serial.print("HttpCode:");
                Serial.println(httpCode);

                if (httpCode == 200)
                {
                  Serial.println("Data Send Successfully");
                  success_count += 1;
                }
                else
                {
                  count++;
                }
                http.end();
                if (count > 3)
                {
                  break;
                }
              }
            }
            csv.close();
            SPIFFS.remove("/OfflineData.csv");
          }
        }
        else
        {
          Serial.println("File Does Not Exist");
        }
      }
    }
    else
    {
      Serial.println("WiFi was not connected offline data ");
      Serial.println("Websocket status: " + String(WebsocketConnected));
      Network_status = false;
      digitalWrite(ORANGELED, HIGH);
      int stationCount = WiFi.softAPgetStationNum();
      Serial.print("Station Count");
      Serial.println(stationCount);
      if (stationCount == 0)
      {
        WifiConnectCheck();
      }
    }
    Serial.println("End of send server ");
    vTaskDelay(pdMS_TO_TICKS(60000));
  }
}

void wifiStatusData(void *pvParameters)
{
  (void)pvParameters;
  while (true)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      if (client.connect("www.google.com", 80))
      {
        WifiStatusConnected();
        client.stop(); // Close the connection
      }
      else
      {
        client.stop(); // Close the connection
        // ws1.textAll("Internet Not Available");
      }
    }
    else
    {
      WifiStatusNotConnected();
    }

    vTaskDelay(pdMS_TO_TICKS(10000)); // Delay for 10 seconds
  }
}

// Open Door
void OpenDoor(void *pvParameters)
{
  (void)pvParameters; // Unused parameter

  pinMode(GREENLED, OUTPUT);
  pinMode(GREENLED1, OUTPUT);
  pinMode(Relay, OUTPUT);
  pinMode(Buzzer, OUTPUT);

  while (true)
  {
    if (OpenDoors)
    {
      Serial.println("Inside Open Door ");
      OpenDoors = false;
      digitalWrite(GREENLED, HIGH);
      digitalWrite(GREENLED1, HIGH);
      digitalWrite(Relay, HIGH);
      digitalWrite(Buzzer, HIGH);
      vTaskDelay(pdMS_TO_TICKS(300));
      vTaskDelay(pdMS_TO_TICKS(200));
      digitalWrite(Buzzer, LOW);
      vTaskDelay(pdMS_TO_TICKS(4000));
      digitalWrite(GREENLED, LOW);
      digitalWrite(GREENLED1, LOW);
      digitalWrite(Relay, LOW);
    }
    else
    {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }
}

// Close Door
void CloseDoor(void *pvParameters)
{
  (void)pvParameters; // Unused parameter

  pinMode(REDLED, OUTPUT);
  pinMode(REDLED1, OUTPUT);
  pinMode(Relay, OUTPUT);
  pinMode(Buzzer, OUTPUT);

  while (true)
  {
    if (CloseDoors)
    {
      Serial.println("Inside Close Door");
      digitalWrite(REDLED, HIGH);
      digitalWrite(REDLED1, HIGH);
      digitalWrite(GREENLED1, LOW);
      digitalWrite(GREENLED, LOW);
      CloseDoors = false;
      for (int i = 0; i < 2; i++)
      {                    // Repeat the pattern 3 times
        tone(Buzzer, 800); // Play the beep tone
        delay(500);        // Wait for the specified duration
        noTone(Buzzer);    // Stop the tone
        delay(500);
      }
      vTaskDelay(pdMS_TO_TICKS(2000));
      digitalWrite(REDLED, LOW);
      digitalWrite(REDLED1, LOW);
      // UnauthorizedAccess();
    }
    else
    {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }
}

void initializeCoreWork()
{
  // Check WiFi status data task, assigned highest priority
  xTaskCreate(
      wifiStatusData,   // Task function
      "WifiStatusData", // Task name
      8192,             // Stack size (bytes)
      NULL,             // Task input parameter
      2,                // Lower Priority
      &WifiStatusDataC  // Task handle
  );

  // Offline Data Sync, lower priority now
  xTaskCreate(
      SdOfflineData,   // Task function
      "SdOfflineData", // Task name
      8192,            // Stack size (bytes)
      NULL,            // Task input parameter
      2,
      &SdOfflineDataC // Task handle
  );

  // Open Door task, high priority since it deals with physical security
  xTaskCreate(
      OpenDoor,   // Task function
      "OpenDoor", // Task name
      2048,       // Stack size (bytes)
      NULL,       // Task input parameter
      4,          // Priority (High)
      &OpenDoorC  // Task handle
  );

  // Close Door task, slightly lower priority than open door
  xTaskCreate(
      CloseDoor,   // Task function
      "CloseDoor", // Task name
      2048,        // Stack size (bytes)
      NULL,        // Task input parameter
      4,           // Priority (Mid-High)
      &CloseDoorC  // Task handle
  );

  // Organization status task, standard priority

  // Uncomment this if needed, with appropriate priority
  xTaskCreate(
      // updateEmployeeDetails,  // Task function
      UpdateActivity,
      "UpdateEmployee",      // Task name
      8192,                  // Stack size (bytes)
      NULL,                  // Task input parameter
      2,                     // Priority (Lower)
      &UpdateEmployeeDetailC // Task handle
  );
}

// WebSocket event Handle
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
               AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    SpiffsTimerStart = false;
    RfidRegister = true;
    break;
  case WS_EVT_DISCONNECT:
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    break;
    RfidRegister = false;

  case WS_EVT_DATA:
    Serial.print("handle incomming data");
    // Handle incoming data
    break;
  case WS_EVT_PONG:
    Serial.print("Pong message");
    // Handle a pong message
    break;
  case WS_EVT_ERROR:
    Serial.print("error ");
    // Handle an error
    break;
  }
}

// wifi Status
void wifiStatusEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                     AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    serverOn = false;

    fetchRunning_update = true;
    break;
  case WS_EVT_DISCONNECT:
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    fetchRunning_update = false;

    break;
  case WS_EVT_DATA:
    Serial.print("handle incomming data");
    // Handle incoming data
    break;
  case WS_EVT_PONG:
    Serial.print("Pong message");
    // Handle a pong message
    break;
  case WS_EVT_ERROR:
    Serial.print("error ");
    // Handle an error
    break;
  }
}

void UnauthorizedAccess()
{
  for (int i = 0; i < 2; i++)
  {                    // Repeat the pattern 3 times
    tone(Buzzer, 500); // Play the beep tone
    delay(100);        // Wait for the specified duration
    noTone(Buzzer);    // Stop the tone
    delay(100);
  }
}

void CompanyIdCheck()
{
  EEPROM.begin(512);
  Serial.println("companyid check");
  String cmpid;
  String cmpName;
  for (int i = 96; i < 100; ++i)
  {
    if (EEPROM.read(i) != 255)
    {
      if (EEPROM.read(i) != 0)
      {
        cmpid += char(EEPROM.read(i));
      }
    }
    Serial.println(EEPROM.read(i));
  }
  for (int i = 100; i < 164; ++i)
  {
    if (EEPROM.read(i) != 255)
    {
      if (EEPROM.read(i) != 0)
      {
        cmpName += char(EEPROM.read(i));
      }
    }
  }
  if (cmpid.length() <= 0)
  {
    Serial.println("Company Id is empty");
  }
  else
  {
    Serial.print("Company Id : ");
    Serial.println(cmpid);
    Serial.print("Company Name : ");
    Serial.println(cmpName);
    CompanyId = cmpid;
    CompanyName = cmpName;
    Serial.println("here");
  }
}

void rfidInitialList()
{
  rtc_wdt_protect_off();
  String rfidList = "";
  serverOn = true;
  Serial.print("Rfid Initial Lists ");
  bool pingcheck = pingServer();
  if (pingcheck)
  {

    HTTPClient http;
    int dataCount = 0;
    int startCount = 0;
    int endCount = 15; // Max size it can handel 27(2.23kb) single fetch including parsing
    mountingSpiffs();
    int falseReturn = 0;
    File dataFile = SPIFFS.open("/Rfid.csv", "w");
    do
    {
      digitalWrite(REDLED, HIGH);
      digitalWrite(GREENLED, HIGH);
      digitalWrite(REDLED1, HIGH);
      digitalWrite(GREENLED1, HIGH);
      Serial.print("inside do while loop");
      DynamicJsonBuffer jsonBuffer;
      JsonObject &JSONEncoder = jsonBuffer.createObject();
      JSONEncoder["companyId"] = CompanyId;
      JSONEncoder["startCount"] = startCount;
      JSONEncoder["endCount"] = endCount;
      char JSONmessageBuffer[500];
      JSONEncoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
      Serial.print("RFID Initial list");
      esp_task_wdt_reset();
      yield();
      Serial.println(JSONmessageBuffer);
      http.begin("https://wildfly.letzcheckin.com:443/EmployeeAttendenceAPI/RFIDAPI/SelectAllEmployeeInfo");
      yield();
      // http.setTimeout(10000);  // Set a timeout of 10 seconds (adjust as needed)
      esp_task_wdt_reset();
      http.addHeader("Content-type", "application/json");
      int response = http.POST(JSONmessageBuffer);
      Serial.print("HTTP code");
      Serial.println(response);
      String payload = http.getString();
      if (response == 200)
      {
        yield();
        esp_task_wdt_reset();
        Serial.print("Payload:");
        Serial.println(payload);
        esp_task_wdt_reset(); // Reset the Watchdog Timer  // Reset the Watchdog Timer
        char *payloadBuffer;  // Adjust the size based on your requirement
        payload.toCharArray(payloadBuffer, sizeof(payloadBuffer));
        esp_task_wdt_reset();
        Serial.println("response:" + payload);
        JsonObject &root = jsonBuffer.parseObject(payload); // Parse the JSON payload dynamically
        yield();
        if (root.success() && root.size() > 0)
        {
          String dataCountStr = root["dataCount"].as<char *>();
          esp_task_wdt_reset();
          dataCount = dataCountStr.toInt();
          startCount += endCount;
          Serial.print("root Size :");
          Serial.print(root.size());
          Serial.print("root:");
          root.printTo(Serial);
          Serial.println("");
          JsonArray &employeeInfoList = root["employeeInfoList"];
          Serial.print("Emp rfid size ");
          root.remove("employeeInfoList");
          Serial.println(employeeInfoList.size());
          int node_length = employeeInfoList.size();
          for (int i = 0; i < node_length; i++)
          {
            File dataFile = SPIFFS.open("/Rfid.csv", "a");
            String empId = employeeInfoList[i]["employeeId"].as<const char *>();
            String Name = employeeInfoList[i]["name"].as<const char *>();
            // String DeviceIdM = employeeInfoList[i]["deviceId"].as<const char *>();                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    y"].as<const char *>();
            String Status = employeeInfoList[i]["status"].as<const char *>();
            String Department = employeeInfoList[i]["department"].as<const char *>();
            String rfidNo = employeeInfoList[i]["rfidNo"].as<const char *>();
            char buf[sizeof(Status)];
            if (rfidNo == "" || rfidNo == " ")
            {
              rfidNo = "-";
            }
            int Matched = 0;
            Serial.print("Device Id: ");
            Serial.println(Status);
            Matched = Status.toInt();
            Serial.print("Matched Value: ");
            Serial.println(Matched);
            Serial.print("Device ID: ");
            Serial.println("");

            String empData = rfidNo + String(",") + Matched + String(",") + empId + String(",") + Name + String(",") + Department + String("\n"); //+ String(",") + String(BioTemps) + String(",") + String(Bioregis) + String("\n");
            Serial.print("Empid data: ");
            Serial.println(empData);
            dataFile.print(empData);
            dataFile.close();
          }
        }
        else
        {
          Serial.print("root not success");
        }
      }
      else
      {
        Serial.print("HTTP request failed :");
        falseReturn += 1;
        esp_task_wdt_reset();
        Serial.println(response);
      }
      if (falseReturn > 2)
      {
        // SPIFFS.remove("/Rfid.csv");
        esp_task_wdt_reset();
        Serial.print("Getting Error SPIFFS File Rfid removed");
        serverOn = false;

        digitalWrite(REDLED, LOW);
        digitalWrite(GREENLED, LOW);
        digitalWrite(REDLED1, LOW);
        digitalWrite(GREENLED1, LOW);
        serverOn = false;
        dataFile.close();
        break;
      }
      http.end();
    } while (dataCount >= startCount - 1);
    dataFile.close();
  }

  else
  {
    Serial.print("Not connected");
  }
  Serial.print("Rfid List");
  serverOn = false;
  digitalWrite(REDLED, LOW);
  digitalWrite(GREENLED, LOW);
  digitalWrite(REDLED1, LOW);
  digitalWrite(GREENLED1, LOW);
}

void DeviceidFetch()
{
  HTTPClient http;
  int httpCode = 0;
  bool pingcheck = pingServer();
  if (pingcheck)
  {

    RegistrationFinger = false;
    DynamicJsonBuffer JsonBuffer;
    Serial.println("Device Id Fetching");
    esp_task_wdt_reset();
    JsonObject &JsonEncoder = JsonBuffer.createObject();
    char JSONcharMessage[500];
    JsonEncoder["companyId"] = CompanyId;
    JsonEncoder["deviceType"] = DeviceType;
    JsonEncoder.printTo(JSONcharMessage, sizeof(JSONcharMessage));
    Serial.print("Device id data: ");
    Serial.println(JSONcharMessage);
    http.begin("https://wildfly.letzcheckin.com:443/EmployeeAttendenceAPI/RFIDAPI/GetRFIDUnMappedDeviceList");
    esp_task_wdt_reset();
    http.addHeader("Content-type", "application/json");
    httpCode = http.POST(JSONcharMessage);
    String payload = http.getString();
    esp_task_wdt_reset();
    Serial.print("Http Response:");
    Serial.println(httpCode);
    Serial.print("Device Id: ");
    Serial.println(payload);
    DeviceList = "";
    if (httpCode == 200)
    {
      JsonObject &root = JsonBuffer.parseObject(payload);
      if (root.success())
      {
        JsonArray &rfidDeviceList = root["rfidDeviceList"];
        rfidDeviceList.printTo(DeviceList);
      }
    }
    http.end();
  }
}

// restart the esp32
void RestartEsp()
{
  ESP.restart();
}

void registerSendtoRfid()
{
  Serial.println("Reading Bios from file");

  // Mount SPIFFS if not mounted already
  if (!SPIFFS.begin(true))
  { // true to format if failed to mount
    Serial.println("Failed to mount SPIFFS");
    return;
  }
  bool pingcheck = pingServer();

  // Check Wi-Fi connection status and internet availability
  if (pingcheck)
  {
    HTTPClient http; // HTTP client instance

    // Open the RfidRegisterStatus file for reading
    File regsFilesQs = SPIFFS.open("/RfidRegisterStatus.csv", "r");
    if (!regsFilesQs)
    {
      Serial.println("Failed to open file for reading");
      return;
    }

    bool allDataSent = true; // To track if all data was sent successfully

    // Read and print each line of the file
    while (regsFilesQs.available())
    {
      String line = regsFilesQs.readStringUntil('\n');
      Serial.print("Line: ");
      Serial.println(line);

      String rfid = "";  // Placeholder for the RFID value
      String Empid = ""; // Placeholder for the employee ID

      // Parse the line, assuming CSV format
      int commaIndex = line.indexOf(',');
      if (commaIndex == -1)
      {
        continue; // Skip lines that don't have a comma
      }

      // Extract RFID and Empid from the line
      rfid = line.substring(0, commaIndex);
      Empid = line.substring(commaIndex + 1); // Assuming Empid is the second value
      Serial.println("Empid check: " + Empid);

      DynamicJsonBuffer jsonBuffer;
      JsonObject &jsonDoc = jsonBuffer.createObject();

      // Prepare a JSON array for registrationList
      JsonArray &registrationList = jsonBuffer.createArray();
      // Create an object for the RFID entry
      JsonObject &rfidEntry = jsonBuffer.createObject();
      rfidEntry["rfid"] = rfid; // Assign the RFID value to the object
      rfidEntry["employeeId"] = Empid;
      registrationList.add(rfidEntry); // Add the object to the array
      // Convert to String ...
      String regsLists = "";
      registrationList.printTo(regsLists);
      // Build the main JSON object
      jsonDoc["companyId"] = CompanyId; // Assuming CompanyId is globally defined
      // jsonDoc["employeeId"] = "001";                  // Employee ID from file
      jsonDoc["registrationList"] = regsLists; // Assign the array to the JSON object
      String JSONmessageBuffer;
      jsonDoc.printTo(JSONmessageBuffer);

      Serial.println("JSON Payload: ");
      Serial.println(JSONmessageBuffer);

      // Send HTTP POST request
      http.begin("https://wildfly.letzcheckin.com:443/EmployeeAttendenceAPI/RFIDAPI/RegisterEmployeeRFIDInfo");
      http.addHeader("Content-Type", "application/json");

      int responseCode = http.POST(JSONmessageBuffer); // Send the request
      String responseBody = http.getString();          // Get the response

      // Print the server response
      Serial.print("HTTP Response code: ");
      Serial.println(responseCode);
      Serial.print("Response body: ");
      Serial.println(responseBody);

      // Check if the POST was successful
      if (responseCode != 200)
      {
        Serial.println("Failed to send data");
        allDataSent = false; // Mark as false if any request fails
        break;               // Stop further processing on error
      }
    }

    regsFilesQs.close(); // Close the file after reading

    // If all data was successfully sent, delete the file
    if (allDataSent)
    {
      if (SPIFFS.remove("/RfidRegisterStatus.csv"))
      {
        Serial.println("File successfully deleted after sending all data.");
      }
      else
      {
        Serial.println("Failed to delete the file.");
      }
    }
  }
  else
  {
    Serial.println("Wi-Fi not connected or no internet access.");
  }
}

// AsyncWebServer code for webpage
void WebServerRoutes()
{
  if (SPIFFS.begin(true))
  {
    Serial.println("SPIFFS Beginned");
  }

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("Main page requested");
    // Debug Information 
    Serial.print("Rfid Register: ");
    Serial.println(RfidRegister);
    RfidRegister = false;
    RegistrationFinger = true;
    WebsocketConnected = true;
    OtpVerifiy = TokenVerifications();
    Serial.print("Otp Verifiy: ");
    Serial.println(OtpVerifiy);
    ResetWebserverPages();
    registerSendtoRfid();
    Serial.println("Reset Webserver pages called");
    request->send(SPIFFS, "/MainPage.html", "text/html");
    // request->send(200, "text/html", Mainpage); 
    Serial.println("Response sent"); });

  // Images
  server.on("/deleteImg", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/deleteImg.png", "image/png"); });
  server.on("/companyImg", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/companyImg.png", "image/png"); });
  server.on("/back", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/back.png", "image/png"); });
  server.on("/popImg", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/popImg.png", "image/png"); });
  server.on("/resetImg", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/resetImg.png", "image/png"); });
  server.on("/restartImg", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/restartImg.png", "image/png"); });
  server.on("/RFIDImg", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/RFIDImg.png", "image/png"); });
  server.on("/successImg", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/successImg.png", "image/png"); });
  server.on("/TictokLogo", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/TictokLogo.png", "image/png"); });
  server.on("/updateImg", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/updateImg.png", "image/png"); });
  server.on("/wifiImg", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/wifiImg.png", "image/png"); });
  server.on("/QuickSettings", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/QuickSettings.png", "image/png"); });

  server.on("/BiometricImg", HTTP_GET, [](AsyncWebServerRequest *request)
            { 
              esp_task_wdt_reset();
              request->send(SPIFFS, "/Biometric.png", "image/png"); });

  server.on("/quickSetting.js", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("quick setup called");
    esp_task_wdt_reset();
    if (SPIFFS.exists("/quickSetting.js")) {
      esp_task_wdt_reset();
        request->send(SPIFFS, "/quickSetting.js", "application/javascript");
    } else {
        request->send(404, "text/plain", "File not found");
        } });

  // CSS Pages Com.
  server.on("/MainPageCss", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/MainPageCss.css", "text/css"); });
  // Device Id
  server.on("/Deviceid", HTTP_GET, [](AsyncWebServerRequest *request)
            { 
              if(DeviceidPage){
              DeviceidFetch();
             request->send(SPIFFS, "/Deviceid.html", "text/html");}
             else{
              request->send(200,"text/plane","UnAuthorised Access");
             } });
  server.on("/DeviceIDList", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plane", DeviceList); });
  server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/styles.css", "text/css"); });
  server.on("/popupCss", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/popupCss.css", "text/css"); });
  server.on("/OtpVerify", HTTP_GET, [](AsyncWebServerRequest *request)

            { request->send(200, "text/plain", String(OtpVerifiy)); });
  server.on("/OtpVerificationChkQuick", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            {
    String VerificationsMsg = "Message Verified";
    String OtpVerificationCheck;
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.parseObject(data);
    if (json.success()) {
      OtpVerificationCheck = json["OtpVerify"].as<char*>();
      if (String(OtpVerifiy).equals(OtpVerificationCheck)) {
        Serial.println("Strings are equal quick setup");
        request->send(200,"text/json","Ok");
        quicksetupCld = true;
        if(quicksetupCld){
          WifiPage = true;
          CompanyPage = true;
          DeviceidPage = true;
        }

    } else {
        request->send(400,"text/json","Not_Ok");
        WifiPage = false;
        CompanyPage = false;
        DeviceidPage = false;
        Serial.println("Strings are not equal");
    }
    } });
  server.on("/OtpVerificationChkWifiPage", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            {
    String VerificationsMsg = "Message Verified";
    String OtpVerificationCheck;
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.parseObject(data);
    Serial.println("Called inside the wifi page ");
    if (json.success()) {
      OtpVerificationCheck = json["OtpVerify"].as<char*>();
      if (String(OtpVerifiy).equals(OtpVerificationCheck)) {
        Serial.println("Strings are equal Wifi page");
        request->send(200,"text/json","Ok");
        WifiPage = true;
    } else {
        request->send(400,"text/json","Not_Ok");
        Serial.println("Strings are not equal");
    }
    } });
  server.on("/OtpVerificationChkCompanyPage", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            {
    String VerificationsMsg = "Message Verified";
    String OtpVerificationCheck;
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.parseObject(data);
    if (json.success()) {
      OtpVerificationCheck = json["OtpVerify"].as<char*>();
      if (String(OtpVerifiy).equals(OtpVerificationCheck)) {
        Serial.println("Strings are equal Company page");
        request->send(200,"text/json","Ok");
        CompanyPage = true;
    } else {
        request->send(400,"text/json","Not_Ok");
        Serial.println("Strings are not equal");
    }

    } });
  server.on("/OtpVerificationChkDeviceidPage", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)

            {

    String VerificationsMsg = "Message Verified";

    String OtpVerificationCheck;

    DynamicJsonBuffer jsonBuffer;

    JsonObject& json = jsonBuffer.parseObject(data);

    if (json.success()) {

      OtpVerificationCheck = json["OtpVerify"].as<char*>();

      if (String(OtpVerifiy).equals(OtpVerificationCheck)) {

        Serial.println("Strings are equal Device id page");

        request->send(200,"text/json","Ok");

        DeviceidPage = true;

    } else {

        request->send(400,"text/json","Not_Ok");

        Serial.println("Strings are not equal");

    }

    } });
  server.on("/OtpVerificationChkRfidRegisterPage", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)

            {

    String VerificationsMsg = "Message Verified";

    String OtpVerificationCheck;

    DynamicJsonBuffer jsonBuffer;

    JsonObject& json = jsonBuffer.parseObject(data);

    if (json.success()) {

      OtpVerificationCheck = json["OtpVerify"].as<char*>();

      if (String(OtpVerifiy).equals(OtpVerificationCheck)) {
        esp_task_wdt_reset();
        Serial.println("Strings are equal Rfid register page");

        request->send(200,"text/json","Ok");

        RfidRegisterPage = true;



    } else {

        request->send(400,"text/json","Not_Ok");

        Serial.println("Strings are not equal");

    }

    } });
  // server.on("/Update", HTTP_GET, [](AsyncWebServerRequest *request)
  // Wifi Settings Html and Css

  server.on("/WifiSetting", HTTP_GET, [](AsyncWebServerRequest *request)
            { 
              if(WifiPage){
              request->send(SPIFFS, "/WifiSetting.html", "text/html");
              }
              else{

                request->send(200,"text/plane","Unauthorised access");
              } });
  server.on("/WifiCss.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/WifiCss.css", "text/css"); });
  // Company setting page
  server.on("/CompanySetting", HTTP_GET, [](AsyncWebServerRequest *request)

            {

              if(CompanyPage){

              request->send(SPIFFS, "/CompanySetting.html", "text/html"); }

              else{

                request->send(200,"text/plain","Unauthorised Access");

              } });
  server.on("/CompanySettingCss.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/CompanySettingCss.css", "text/css"); });
  server.on("/Reset", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/Reset.html", "text/html"); });

  // RFID Page

  server.on("/upload", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/upload.html", "text/html"); });
  server.on("/File Test", HTTP_GET, [](AsyncWebServerRequest *request) { // fileReadAndWrite();
    request->send(200, "Done", "text/plain");
  });

  server.on("/RFID.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/RFID.css", "text/css"); });
  server.on("/RFID", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/RFID.html", "text/html"); });
  // Restart Page ...
  server.on("/Restart", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    RestartEsp();
    request->send(200, "text/json", "Restart"); });
  // Reset page ...
  server.on("/Resets", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              Serial.println("Inside reset page ");
              EEPROM.begin(512);
              // mountinSD();
              // Clearing specific sections of EEPROM memory
              for (int i = 0; i <= 95; ++i)
              {
                EEPROM.write(i, 0);
              }
              EEPROM.commit();
              for (int i = 99; i <= 170; ++i)
              {
                EEPROM.write(i, 0);
              }
              EEPROM.commit();
              if (SPIFFS.exists("/EmpRfid.csv"))
              {
                if (SPIFFS.remove("/EmpRfid.csv"))
                {
                  Serial.println("EmpRfid.csv Deleted Successfully");
                }
                else
                {
                  Serial.println("Failed to Delete EmpRfid.csv");
                }
              }
              else
              {
                Serial.println("EmpRfid.csv does not exist");
              }
              if (SPIFFS.exists("/Rfid.csv"))
              {
                if (SPIFFS.remove("/Rfid.csv"))
                {
                  Serial.println("Rfid.csv Deleted Successfully");
                }
                else
                {
                  Serial.println("Failed to Delete Rfid.csv");
                }
              }
              else
              {
                Serial.println("Rfid.csv does not exist");
              }
              request->send(200, "text/json", "reset");
              delay(1000);
              RestartEsp(); });
  server.on("/Reset", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("Inside reset page ");
    EEPROM.begin(512);
    // mountinSD();
    // Clearing specific sections of EEPROM memory
    for (int i = 0; i <= 95; ++i) {
      EEPROM.write(i, 0);
    }
    EEPROM.commit();
    for (int i = 99; i <= 170; ++i) {
      EEPROM.write(i, 0);
    }
    EEPROM.commit();
    // Attempting to remove files from SPIFFS
    if (SPIFFS.exists("/EmpRfid.csv")) {
      if (SPIFFS.remove("/EmpRfid.csv")) {
        Serial.println("EmpRfid.csv Deleted Successfully");
      } else {
        Serial.println("Failed to Delete EmpRfid.csv");
      }
    } else {
      Serial.println("EmpRfid.csv does not exist");
    }
    if (SPIFFS.exists("/Rfid.csv")) {
      if (SPIFFS.remove("/Rfid.csv")) {
        Serial.println("Rfid.csv Deleted Successfully");
      } else {
        Serial.println("Failed to Delete Rfid.csv");
      }
    } else {
      Serial.println("Rfid.csv does not exist");
    }
    RestartEsp();
    request->send(200, "text/json", "Restart"); });
  // Update the Employee Details ...
  server.on("/Update", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    int statuss = 1;//fetchEmployeeDetails();
    Serial.print("Status");
    Serial.println(statuss);
    if (statuss > 0) {
              SPIFFS.remove("/EmpRfid.csv");
              SPIFFS.remove("/Rfid.csv");
              request->send(200, "text/json", "Update");
              delay(1000);
              RestartEsp();
      
    } });
  // Submit data for companyreg id...
  server.on("/submit-data", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            {
    EEPROM.begin(512);
    String companyId = "";
    String companyName = "";
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.parseObject(data);
    if (json.success()) {
      String dates = json["date"];
      companyId = json["companyId"].as<char*>();
      companyName = json["companyName"].as<char*>();
      Serial.println("Submit button pressed");
      Serial.println("Received date: " + dates);
      Serial.println("Received companyId: " + companyId);
      Serial.println("Received companyName: " + companyName);
      // Inside your POST handler
      String Time = dates;
      int first = Time.indexOf('-');
      int secon = Time.indexOf('-', first + 1);
      int firstT = Time.indexOf('T');
      int firsCol = Time.indexOf(':');
      int secoCol = Time.indexOf(':', firsCol + 1);
      String year = Time.substring(0, first);
      String month = Time.substring(first + 1, secon);
      String day = Time.substring(secon + 1, firstT);
      String hour = Time.substring(firstT + 1, firsCol);
      String minute = Time.substring(firsCol + 1, secoCol);
      String seconds = Time.substring(secoCol + 1);
      Serial.println(year);
      Serial.println(month);
      Serial.println(day);
      Serial.println(hour);
      Serial.println(minute);
      Serial.println(seconds);
      // Assuming year, month, day, hour, minute, and second are String objects
      int yearInt = year.toInt();
      int monthInt = month.toInt();
      int dayInt = day.toInt();
      int hourInt = hour.toInt();
      int minuteInt = minute.toInt();
      int secondInt = seconds.toInt();
      // Wire.begin();
      if (rtc.begin()) {
        Serial.println("RTC is RUNNING");
        DateTime now = rtc.now();
        rtc.adjust(DateTime(yearInt, monthInt, dayInt, hourInt, minuteInt, secondInt));
        Serial.print("RTC Time :");
        //DateTime now =File opened successfully
        rtc.now();
        Serial.println(now.year());
        Serial.println(now.month());
        Serial.println(now.day());
        Serial.println(now.hour());
        Serial.println(now.minute());
        Serial.println(now.second());
      }
      else {
        Serial.println("RTC is NOT RUNNING");
      }
      Serial.println("about");
      //      String CmpId="";
      //      String CompanyName;
      bool cmpIdnotEmpty = false;
      Serial.println("Clearing EEPROM");
      for (int i = 96; i < 165; ++i) {
        EEPROM.write(i, 0);
      }
      if (companyId.length() > 0) {
        for (int i = 0; i < 3; ++i) {
          EEPROM.write(i + 96, companyId[i]);
          Serial.print(i);
          Serial.println(companyId[i]);
          //cmpIdnotEmpty = true;
        }
      }
      if (companyName.length() > 0) {
        for (int i = 0; i < companyName.length(); ++i) {
          EEPROM.write(i + 100, companyName[i]);
          Serial.println(companyName[i]);
        }
      }
      EEPROM.commit();
      Serial.println("Reading from EEPROM:");
      for (int i = 96; i < 165; ++i) {
        Serial.print(EEPROM.read(i));
        Serial.print(" ");
      }
      CompanyIdCheck();
      Serial.print("Company Id :");
      Serial.println(CompanyId);
      Serial.print("Company Name :");
      Serial.println(CompanyName);
      request->send(200, "application/json", "{\"message\":\"Data received successfully\"}");
      DeviceidFetch();
    }
    else {
      request->send(400, "application/json", "{\"message\":\"Parsing JSON failed\"}");
    } });
  // Device ID Submit & Fetching employee
  server.on("/Device_id", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            {
        Serial.println("button pressed");
        HTTPClient http;
        int httpCount = 0;
        DynamicJsonBuffer jsonBuffer;
        JsonObject &json = jsonBuffer.parseObject(data);
        if (json.success())
        {
          String deviceId = json["DeviceId"];
          Serial.println(deviceId);
          Serial.print(deviceId.length());
          EEPROM.begin(512);
          if (deviceId.length() > 0)
          {
            Serial.print("here");
            for (int i = 166; i <= 170; ++i)
            {
              Serial.print("here1");
              EEPROM.write(i, 0);
            }
            EEPROM.commit();
            for (int i = 0; deviceId.length() > i; ++i)
            {
              Serial.print("here2");
              EEPROM.write(i + 166, deviceId[i]);
              Serial.print("Device Id :");
              Serial.println(deviceId[i]);
            }
            EEPROM.commit();
          }
          Serial.print("Calling Function");
          DeviceIdInitialize();
          DynamicJsonBuffer jsonBuffers;
          JsonObject &JsonEncoder = jsonBuffers.createObject();
          JsonEncoder["companyId"] = CompanyId;
          JsonEncoder["deviceId"] = deviceId;
          char JsonBufferMessage[500];
           JsonEncoder.printTo(JsonBufferMessage, sizeof(JsonBufferMessage));
          Serial.println(JsonBufferMessage);
          http.begin("https://wildfly.letzcheckin.com:443/EmployeeAttendenceAPI/RFIDAPI/RFIDDoorMapping");
          http.addHeader("Content-type", "application/json");
          httpCount = http.POST(JsonBufferMessage);
          String Payload = http.getString();
          Serial.print("Payload :");
          Serial.println(Payload);
          if (httpCount == 200)
          {
            Serial.println("Successfully sent");
            UpdateEmployee = true;
          }
          else
          {
            Serial.println("Not sent");
          }
        }
        else
        {
          Serial.println("Device id Was not entered");
        }
        http.end();
        request->send(200, "text/plane", "Got the Device id"); });
  // Sends WiFi details ...
  server.on("/get-wifi-data", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("Received /get-wifi-data request");
     // Set the request flag
     String data = json;
     // xTaskNotify(WiFiscanTaskHandle, 1, eSetValueWithOverwrite); // Notify the task      wifiScantaskDelete();
      request->send(200, "text/plain", data); });

  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request) {}, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
            {
    if (!index) {
      filename = "Rfid.csv";
      Serial.printf("UploadStart: %s\n", filename.c_str());
      if (SPIFFS.exists("/Rfid.csv")) {
        // SPIFFS.remove("/Rfid.csv");
      }
      request->_tempFile = SPIFFS.open("/Rfid.csv", "w");
    }
    if (request->_tempFile) {
      if (len) {
        request->_tempFile.write(data, len);
      }
      if (final) {
        request->_tempFile.close();
        Serial.printf("UploadEnd: %s, %u B\n", filename.c_str(), index + len);
        request->send(200, "text/plain", "File Uploaded Successfully");
      }
    } else {
      request->send(500, "text/plain", "File Upload Failed");
    } });

  server.on("/EmployeeLists", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    File csv_File = SPIFFS.open("/Rfid.csv", "r");
    if (csv_File) {
      Serial.print("Opend csv File");
      request->send(SPIFFS, "/Rfid.csv", "text/csv");
      csv_File.close();
    }
    else {
      Serial.print("File not Found");
      request->send(404, "text/plain", "File Not Found");
    } });

  /// BioRegs.csv
  server.on("/EmployeeBioLists", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    File csv_File = SPIFFS.open("/BioRegs.csv", "r");
    if (csv_File) {
      Serial.print("Opend csv File");
      request->send(SPIFFS, "/BioRegs.csv", "text/csv");
      csv_File.close();
    }
    else {
      Serial.print("File not Found");
      request->send(404, "text/plain", "FIle Not Found");
    } });
  server.on("/OfflineEmpLists", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    File csv_File = SPIFFS.open("/OfflineData.csv", "r");
    if (csv_File) {
      Serial.print("Opend csv File");
      request->send(SPIFFS, "/OfflineData.csv", "text/csv");
      csv_File.close();
    }
    else {
      Serial.print("File not Found");
      request->send(404, "text/plain", "FIle Not Found");
    } });
  server.on("/DeleteSpiffsFiles", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              SPIFFS.remove("/EmpRfid.csv");
              SPIFFS.remove("/Rfid.csv");
              request->send(200, "text/plane", "Deleted"); });
  server.on("/DeleteOfflineFiles", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              SPIFFS.remove("/OfflineData.csv");
              // SPIFFS.remove("/Rfid.csv");
              request->send(200, "text/plane", "Deleted"); });

  server.on("/DeleteBioRegsFiles", HTTP_GET, [](AsyncWebServerRequest *request)
            {
            SPIFFS.remove("/BioRegs.csv");
            // SPIFFS.remove("/Rfid.csv");
            request->send(200, "text/plane", "Deleted"); });

  server.on("/Wifi_submit", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            {
              EEPROM.begin(512);
              String ssid = "";
              String pass = "";
              DynamicJsonBuffer jsonBuffer;
              JsonObject &json = jsonBuffer.parseObject(data);
              if (json.success())
              {
                ssid = json["SSID"].as<char *>();
                pass = json["PASSWORD"].as<char *>();
                Serial.println(ssid);
                Serial.println(pass);
                if (ssid.length() > 0)
                {
                  Serial.println("Clearing EEPROM");
                  for (int i = 0; i < 94; ++i)
                  {
                    EEPROM.write(i, 0);
                  }
                  Serial.println("Writting SSID and Password to eeprom");
                  for (int i = 0; i < ssid.length(); ++i)
                  {
                    EEPROM.write(i, ssid[i]);
                  }
                  for (int i = 0; i < pass.length(); ++i)
                  {
                    EEPROM.write(i + 32, pass[i]);
                  }
                }
                EEPROM.commit();
                request->send(200, "text/plain", "WiFi submitted and connected successfully.");
                delay(1000);
                if (WiFi.status() != WL_CONNECTED)
                {
                  RestartEsp();
                }
              }
              else
              {
                request->send(400, "text/plain", "Invalid SSID provided.");
              } });

  // WiFi Page Start ..

  server.on("/RfiduidUpdate", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            {
    Serial.println("Received CSV Data");

    String csvData = String((char*)data);
    Serial.println("CSV Content:");
    Serial.println(csvData);

    // Open the file for appending
    File csvFile = SPIFFS.open("/RfidRegisterStatus.csv", FILE_WRITE);
    if (!csvFile) {
        Serial.println("Failed to open file for appending");
        request->send(500, "text/plain", "Failed to open file");
        return;
    }

    // Write the CSV data to the file
    if (csvFile.print(csvData)) {
        Serial.println("CSV data written successfully");
    } else {
        Serial.println("Failed to write CSV data");
    }

    csvFile.close(); // Ensure file is closed properly

    // Respond to the client
    request->send(200, "text/plain", "CSV Data Updated"); });

  server.on("/EmployeeRfidLists", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    File csv_File = SPIFFS.open("/EmpRfid.csv", "r");
    if (csv_File) {
      Serial.print("Opend csv File");
      request->send(SPIFFS, "/EmpRfid.csv", "text/csv");
      csv_File.close();
    }
    else {
      Serial.print("File not Found");
      request->send(404, "text/plain", "FIle Not Found");
    } });

  server.on("/EmployeeRegStatus", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    File csv_File = SPIFFS.open("/RfidRegisterStatus.csv", "r");
    if (csv_File) {
      Serial.print("Opend csv File");
      request->send(SPIFFS, "/RfidRegisterStatus.csv", "text/csv");
      csv_File.close();
    }
    else {
      Serial.print("File not Found");
      request->send(404, "text/plain", "FIle Not Found");
    } });

  server.on("/FirmwareVersion", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", FirmwareVer); });
  server.begin();
}

bool pingServer()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    if (client.connect(host, 80))
    {
      client.stop();
      return true;
    }
    else
    {
      client.stop();
      return false;
    }
  }
  return false;
}

void InitialSpiffs()
{
  if (!SPIFFS.begin())
  {
    Serial.println("SPIFFS initialization failed.");
  }
  else
  {
    Serial.println("SPIFFS initialized");
  }
}

// we will be using websocke to communicate with web for RFID

// Sending message to WebSocket Client .. /
void sendMessageToWsClient(String rfid)
{
  String message = "{\"rfid\":\"" + rfid + "\"}";
  ws.textAll(message);
}

int ServerSend(String Empid, String companyId)
{
  Serial.print("Empid ");
  Serial.println(Empid);
  Serial.print("COMPANY ID ");
  Serial.println(companyId);
  DynamicJsonBuffer JSONbuffer;
  JsonObject &JSONencoder = JSONbuffer.createObject();
  DateTime now = rtc.now();
  Serial.print("Current time: ");
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
  Serial.println("Sending data TO Backend");
  HTTPClient http; // Declare object of class HTTPClient
  JSONencoder["employeeId"] = Empid;
  JSONencoder["rfid"] = "-";
  JSONencoder["deviceId"] = DeviceId;
  JSONencoder["companyId"] = companyId;
  JSONencoder["deviceType"] = DeviceType;
  JSONencoder["date"] = String(now.year()) + String("-") + String(now.month(), DEC) + String("-") + String(now.day());
  JSONencoder["time"] = String(now.hour()) + String(":") + String(now.minute()) + String(":") + String(now.second());
  Serial.println(String(now.hour()) + String(":") + String(now.minute()) + String(":") + String(now.second()));
  char JSONmessageBuffer[300];
  JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  Serial.print("JSON MESSAGE server send");
  Serial.println(JSONmessageBuffer);
  http.begin("http://3.6.171.29:8080/EmployeeAttendenceAPI/employee/EmployeeCheckInOut"); // Specify request destination
  http.addHeader("Content-Type", "application/json");                                     // Specify content-type header
  Serial.println("Here after content type");
  int httpCode = http.POST(JSONmessageBuffer); // Send the request
  Serial.print("HttpCode:");
  Serial.println(httpCode);
  if (httpCode == 200)
  {
    Serial.println("inside http code ");
    DynamicJsonBuffer jsonBuffer(300);
    // Parse JSON object
    Serial.println("parse json");
    JsonObject &root = jsonBuffer.parseObject(http.getString());
    root.printTo(Serial);
    Serial.println("json object");
    const char *code = root["employeeId"];
    const char *department = root["department"];
    const char *retStatus = root["status"];
    const char *printStatus = root["status"];
    const char *userName = root["employeeName"];
    const char *OrganizationStatusF = root["organizationStatus"];
    Serial.print("Code return element = ");
    Serial.println("EmployeeId ");
    Serial.println(code);
    Serial.println("Employee name ");
    Serial.println(userName);
    Serial.println("Status name ");
    Serial.println(printStatus);
    Serial.print("Org Status");
    Serial.print(OrganizationStatusF);
    if (strcmp(OrganizationStatusF, "Active") == 0)
    {
      Serial.println("inside the organization status");
      if ((strcmp(retStatus, "CHECKIN") == 0))
      {
        Serial.println("Inside Open Door send Server");
        OpenDoors = true;
        return -1;
        // Fetching = true;
      }
      else if ((strcmp(retStatus, "CHECKOUT") == 0))
      {
        // OpenDoor();
        OpenDoors = true;
        return -1;
        // Fetching = true;
      }
      else if (strcmp(retStatus, "SAME_TIME") == 0)
      {
        OpenDoors = true;
        // Fetching = true;
        return -1;
      }
      else if ((strcmp(retStatus, "BLOCKED") == 0))
      {
        UnauthorizedAccess();
        CloseDoors = true;
        digitalWrite(REDLED, HIGH); // turn the LED off.
        digitalWrite(REDLED1, HIGH);
        return -1;
        // Fetching = true;
      } //                          NOT_VAILD
      else if ((strcmp(retStatus, "NOT_VAILD") == 0))
      {
        UnauthorizedAccess();
        digitalWrite(REDLED, HIGH); // turn the LED off.
        digitalWrite(REDLED1, HIGH);
        CloseDoors = true;
        return -1;
        // Fetching = true;
      }
      else if ((strcmp(retStatus, "Employee_Not_Assigned_To_The_Device") == 0))
      {
        UnauthorizedAccess();
        CloseDoors = true;
        digitalWrite(REDLED, HIGH); // turn the LED off.
        digitalWrite(REDLED1, HIGH);
        // Fetching = true;
        return -1;
      }
      else if ((strcmp(retStatus, "RFID_NO_Is_Not_Mapped_To_Any_Employee") == 0))
      {
        UnauthorizedAccess();
        CloseDoors = true;
        digitalWrite(REDLED, HIGH); // turn the LED off.
        digitalWrite(REDLED1, HIGH);
        // Fetching = true;
        return -1;
      }
      else
      {
        Serial.print("Wrong method followed");
        // Fetching = true;
        return -1;
      }
      Serial.println(httpCode); // Print HTTP return code
    }
    else
    {
      // Organization is not Active
    }
  }
  // NOT_VAILD
  else
  {
    Serial.println("could not send back to server ");
    OfflineDataWrite(Empid); // if response failed but valid employee
    OpenDoors = true;
  }

  http.end(); // Close connection
  Serial.println("Succesfully Send Data To BackEnd");
  return 1;
}

// Function for writting data in SPIFFS in offline data
void OfflineDataWrite(String empId)
{
  InitialSpiffs();
  String Date;
  String Time;
  // String csvData;
  Serial.println("Offline Data Write Inside");
  File file = SPIFFS.open("/OfflineData.csv", "a"); // Open the file in write mode (overwrite existing content)
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }
  Serial.println("Opens CSV file");
  DateTime now = rtc.now();
  String currentDate = String(now.year()) + "-" + String(now.month(), DEC) + "-" + String(now.day());
  String currentTime = String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second());
  String csvData = empId + String(",") + currentDate + String(",") + currentTime + String("\n");
  Serial.println("Data written");
  Serial.print("CSV Data ");
  Serial.println(csvData);
  file.println(csvData); // Write data to a new line
  file.close();
  Serial.println("File closed");
}

// Match rfid File

bool matchRfid(String MatchedRfid)
{
  bool validRfid = false;
  String Empid;

  Serial.println("Inside matchRfid function");
  Serial.print("RFID ID: ");
  Serial.println(MatchedRfid);

  // Open the file from SPIFFS
  File datafilesRfid = SPIFFS.open("/EmpRfid.csv", "r");
  if (!datafilesRfid)
  {
    Serial.println("Failed to open file for reading");
    return false;
  }

  // Read the file line by line
  while (datafilesRfid.available())
  {
    String line = datafilesRfid.readStringUntil('\n');
    Serial.print("Line: ");
    Serial.println(line);

    // Find the comma separating ID and Empid
    int commaIndex = line.indexOf(',');
    if (commaIndex == -1)
    {
      continue; // Skip lines that don't have a comma
    }

    // Extract ID and Empid from the line
    String rfidfile = line.substring(0, commaIndex);
    Empid = line.substring(commaIndex + 1); // Assuming Empid is the second value
    Serial.println("Empid check: " + Empid);

    // Check if the fingerprint ID matches
    if (MatchedRfid == rfidfile)
    {
      Serial.println("ID Found");
      validRfid = true;
      digitalWrite(GREENLED, HIGH);
      digitalWrite(GREENLED1, HIGH);
      break; // Exit the loop if ID is found
    }
  }

  // Close the file
  datafilesRfid.close();

  // Handle the result
  Serial.println("Checking conditions for door open");
  Serial.print("rfidFound: ");
  Serial.println(validRfid);
  esp_task_wdt_reset();
  bool pingcheck = pingServer();
  if (validRfid)
  {
    if (pingcheck)
    {

      esp_task_wdt_reset();
      Serial.println("Connected to server, sending data");
      ServerSend(Empid, CompanyId); // Implement this function based on your needs
    }
    else
    {
      Serial.println("Failed to connect to server");
      Serial.println("OFFLINE Preparation");
      Serial.println("DOOR OPEN");
      OfflineDataWrite(Empid); // Implement this function based on your needs
      OpenDoors = true;
    }
  }
  else
  {
    Serial.println("Unauthorized access here");
    digitalWrite(REDLED, HIGH);
    digitalWrite(REDLED1, HIGH);
    digitalWrite(GREENLED1, LOW);
    digitalWrite(GREENLED, LOW);
    UnauthorizedAccess(); // Implement this function based on your needs
    vTaskDelay(pdMS_TO_TICKS(300));
    digitalWrite(REDLED, LOW);
    digitalWrite(REDLED1, LOW);
  }

  vTaskDelay(pdMS_TO_TICKS(2000));
  return validRfid;
}

// TODO :
void softAp()
{
  // WiFi.mode(WIFI_AP);
  WiFi.softAP(ASSID, APASS);
  Serial.println("AP Started");
  Serial.print("IP Address:");
  Serial.println(WiFi.softAPIP());
  MDNSServer();
}

void initializePinMode()
{
  pinMode(Relay, OUTPUT);
  pinMode(Buzzer, OUTPUT);
  pinMode(REDLED, OUTPUT);
  pinMode(GREENLED, OUTPUT);
  pinMode(REDLED1, OUTPUT);
  pinMode(GREENLED1, OUTPUT);
  pinMode(ORANGELED, OUTPUT);
}

void initialavailableWifi()
{
  //  WiFi.mode(WIFI_STA);
  DynamicJsonBuffer jsonBuffer;
  JsonArray &networks = jsonBuffer.createArray();
  int n = WiFi.scanNetworks();
  if (n == 0)
  {
    Serial.println("No networks found.");
    JsonObject &network = networks.createNestedObject();
    network["ssid"] = "No networks found";
  }
  else
  {
    for (int i = 0; i < n; ++i)
    {
      JsonObject &network = networks.createNestedObject();
      network["ssid"] = WiFi.SSID(i);
      network["rssi"] = WiFi.RSSI(i);
      network["encryption"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Encrypted";
    }
  }
  json = "";
  networks.printTo(json);
  Serial.print("Json Data :");
  Serial.println(json);
  serverOn = false;
  WiFi.scanDelete();
}

void wifiConnectedCheckerMin()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    int stationConnect = WiFi.softAPgetStationNum();
    if (stationConnect > 0)
    {
      WiFi.mode(WIFI_AP);
    }
  }
}

void MDNSServer()
{
  if (!MDNS.begin(hostName))
  {
    Serial.println("Error setting up MDNS responder!");
    esp_restart();
  }
  Serial.println("mDNS responder started");
}
// *Setup
void setup()
{

  Serial.begin(115200);
  EEPROM.read(512);
  // while (!Serial); // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  // pinMode(SS_1_PIN, HIGH);
  // digitalWrite(SS_2_PIN, HIGH);
  // digitalWrite(SS_1_PIN, HIGH);
  // digitalWrite(SS_2_PIN, HIGH);
  // delay(1000);
  printFreeHeap("initialiazation");
  // digitalWrite(SS_1_PIN, LOW);
  // digitalWrite(SS_2_PIN, LOW);
  delay(1000);
  initialavailableWifi();
  InitializeRfid();
  // printFreeHeap("After Core Work");
  WiFi.mode(WIFI_AP_STA);
  // WIFI Access point Initilaization ..
  softAp();
  Serial.print("Json :");
  Serial.println(json);
  initializePinMode();
  Serial.print("CompanyId");
  Serial.println(CompanyId);
  InitializeRTC();
  mountingSpiffs();
  CompanyIdCheck();
  DeviceIdInitialize();
  WifiConnectCheck();
  WebServerRoutes();
  rfidInitialCheck();
  fileReadAndWrite();
  registerSendtoRfid();
  FirmwareUpdate();
  initializeCoreWork();
  digitalWrite(GREENLED1, LOW);
  digitalWrite(GREENLED, LOW);
  digitalWrite(REDLED1, LOW);
  digitalWrite(REDLED, LOW);
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  printFreeHeap("After initiliazation");
  Serial.println("");
  Serial.print("Firmware Version:");
  Serial.println(FirmwareVer);
}

// RFID card reading loop.
void loop()
{
  // ws.cleanupClients();
  // Serial.println("Loop");
  // Reset the Watchdog Timer
  unsigned long currentMillis = millis();

  // Call setClock() and FirmwareUpdate() at specific intervals
  if ((currentMillis - previousMillis) >= firmwareUpdateInterval)
  {
    previousMillis = currentMillis;
    FirmwareUpdate(); // Update firmware periodically
  }
  esp_task_wdt_reset();
  for (uint8_t reader = 0; reader < NO_OF_READERS; reader++)
  {
    String content = "";
    if (mfrc522[reader].PICC_IsNewCardPresent() && mfrc522[reader].PICC_ReadCardSerial())
    {
      Serial.print(F("RFID "));
      Serial.print(reader);
      Serial.println();
      Serial.print(F("Card UID:"));
      for (byte i = 0; i < mfrc522[reader].uid.size; i++)
      {
        Serial.print(mfrc522[reader].uid.uidByte[i] < 0x10 ? "0" : " ");
        Serial.print(mfrc522[reader].uid.uidByte[i], HEX);
        content.concat(String(mfrc522[reader].uid.uidByte[i] < 0x10 ? "0" : " "));
        content.concat(String(mfrc522[reader].uid.uidByte[i], HEX));
      }
      Serial.println();
      content.toUpperCase();
      Serial.println("String:");
      Serial.println(content.substring(1));
      mfrc522[reader].PICC_HaltA();
      mfrc522[reader].PCD_StopCrypto1();
      Serial.print("RFID true ");
      Serial.println(RfidRegister);
      if (RfidRegister == true)
      {

        sendMessageToWsClient(content.substring(1));
      }
      else
      {
        if (content.length() > 0)
        {
          matchRfid(content.substring(1));
        }
        else
        {
          Serial.print("Invalid Card");
        }
      }
    }
  }
  // timer.tick();

  wifiConnectedCheckerMin();
}

/*
spiffsFileCheck was removed
*/

/*                                 Git Update Start                                   */

void FirmwareUpdate()
{
  digitalWrite(LED_BUILTIN, OUTPUT);
  WiFiClientSecure clients;
  clients.setInsecure();
  String payload = "";
  if (!clients.connect(gitHost, gitport))
  {
    Serial.println("Connection failed");
    return;
  }
  clients.print(String("GET ") + URL_fw_Version + " HTTP/1.1\r\n" +
                "Host: " + host + "\r\n" +
                "User-Agent: BuildFailureDetectorESP8266\r\n" +
                "Cache-Control: no-cache\r\n" +
                "Connection: close\r\n\r\n");
  while (true)
  {
    String line = clients.readStringUntil('\n');
    Serial.print("Line: ");
    Serial.println(line);
    if (line == "\r")
    {
      // Serial.println("Headers received");
      break;
    }
  }
  payload = clients.readStringUntil('\n');

  payload.trim();
  Serial.print("Payload: ");
  Serial.println(payload);
  Serial.print("Firmware installed: ");
  Serial.println(FirmwareVer);
  if (payload == FirmwareVer)
  {
    Serial.println("Device already on latest firmware version");
  }
  else
  {
    Serial.println("New firmware detected");
    httpUpdate.setLedPin(LED_BUILTIN, LOW);

    digitalWrite(GREENLED, HIGH);
    digitalWrite(GREENLED1, HIGH);
    digitalWrite(REDLED, HIGH);
    digitalWrite(REDLED1, HIGH);
    digitalWrite(ORANGELED, HIGH);

    t_httpUpdate_return ret = httpUpdate.update(clients, URL_fw_Bin);

    switch (ret)
    {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;

    case HTTP_UPDATE_OK:
      Serial.println("HTTP_UPDATE_OK");
      break;
    }
  }
  clients.stop();
}

/*                                 Git Update End                                        */
