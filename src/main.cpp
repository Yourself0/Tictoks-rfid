#include <Arduino.h>
#define Build_Version "RFV1_001"
#include <FS.h>
#include <WiFi.h>
#include <Wire.h>
#include <EEPROM.h>
#include <SPIFFS.h>
#include <MFRC522.h> // RFID Library
#include <RTClib.h>  //For RTC DS3231
#include <HTTPClient.h>
#include "soc/rtc_wdt.h"
#include "MainPages.h"
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include <arduino-timer.h>
#include <ESPAsyncWebServer.h>
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
// AsyncWebSocket ws1("/Status");
// TaskHandle_t WiFiscanTaskHandle;
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
#define REDLED 13
#define Buzzer 15
#define GREENLED 12
#define ORANGELED 14
#define REDLED1 33
#define GREENLED1 25
// RFID pins
#define SS_1_PIN 2
#define SS_2_PIN 26
#define RST_PIN 27
#define NO_OF_READERS 2
byte ssPins[] = {SS_1_PIN, SS_2_PIN};
// Access Point SSID and PASSWORD ..
const char *ASSID = "Tictoks RF V2";
const char *APASS = "123456789";
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
// Static IP Address
// IPAddress staticIp(192, 168, 1, 123); // Assigning a static IP address 192.168.1.123
// IPAddress gateway(192, 168, 1, 1);    // Assigning a gateway IP address, assuming it's 192.168.1.1
// IPAddress subnet(255, 255, 255, 0);   // Assigning a subnet mask, such as 255.255.255.0

TaskHandle_t WiFiscanTaskHandleC;
TaskHandle_t WifiStatusDataC = NULL;
TaskHandle_t UpdateEmployeeDetailC = NULL;
TaskHandle_t OpenDoorC = NULL;
TaskHandle_t CloseDoorC = NULL;
TaskHandle_t SdOfflineDataC = NULL;
TaskHandle_t printTaskHandleC = NULL;
TaskHandle_t OrganisationStatusC = NULL;

void rfidInitialList();
void OfflineDataWrite(String empId);

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

int fetchEmployeeCheck()
{
  Serial.println("FetchEmployeeCheck");
  if (WiFi.status() == WL_CONNECTED)
  {
    if (client.connect("wildfly.letzcheckin.com", 80))
    {
      int dataCount = 0;
      int startCount = 0;
      const int endCount = 15;
      int loop_count = 0;
      int httpCode = 0;
      int falseReturn = 0;
      HTTPClient http;
      DynamicJsonBuffer jsonBuffers;
      JsonObject &JSONEncoder = jsonBuffers.createObject();
      JSONEncoder["companyId"] = CompanyId;
      JSONEncoder["deviceId"] = DeviceId;
      JSONEncoder["startCount"] = startCount;
      JSONEncoder["endCount"] = endCount;
      char JsonMessageBuffer[500];
      JSONEncoder.printTo(JsonMessageBuffer, sizeof(JsonMessageBuffer));
      http.begin("https://wildfly.letzcheckin.com:443/EmployeeAttendenceAPI/RFIDAPI/SelectEmployeeInfo");
      Serial.println("http begin");
      http.addHeader("Content-Type", "application/json"); // Specify content-type header
      Serial.println("Post Main");
      httpCode = http.POST(JsonMessageBuffer); // Send the request
      Serial.print("HTTP Code ");
      Serial.println(httpCode);
      yield();
      String payload = http.getString();
      char payloadBuffer[1024]; // Adjust the size based on your requirement
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
            // No need to remove from root
            String dataCountStr = root["dataCount"].as<char *>();
            int dataCount = dataCountStr.toInt();
            http.end();
            return dataCount;
          }
          else
          {
            http.end();
            return -8;
          }
        }
      }
      else
      {
        Serial.println("Error Facing while Fetching");
        http.end();
        Serial.print("error Code:");
        Serial.println(httpCode);
        Serial.print("broken");
        return -1;
      }
    }
  }
  return 5;
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
    if (true)
    {
      if (WiFi.status() == WL_CONNECTED)
      {
        if (client.connect(servername, 80))
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





/*
bool fetchTimer_UpdateActivity(void *)
{
  if (!fetchRunning_update)
  {
    serverOn = true;
    if (WiFi.status() == WL_CONNECTED)
    {
      if (client.connect(servername, 80))
      {
        Serial.println("Fetch Timer Check 2");
        UpdateActivity();
      }
    }
    else
    {
      serverOn = false;
    }
    return true;
  }
  return true;
}
*/

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

// Scanning wifi in core
/*
void WiFiscanTask(void *pvParameters)
{
  for (;;)
  {
    // // Wait for notification to start scan
    // ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    // Serial.print("inside wifi scan task ");
    // Serial.println("Insider wifi scan request");
    // DynamicJsonBuffer jsonBuffer;
    // JsonArray &networks = jsonBuffer.createArray();
    // wifiScanRequested = false;
    // int n = WiFi.scanNetworks();
    // if (n == 0)
    // {
    //   Serial.println("No networks found.");
    //   JsonObject &network = networks.createNestedObject();
    //   network["ssid"] = "No networks found";
    // }
    // else
    // {
    //   for (int i = 0; i < n; ++i)
    //   {
    //     JsonObject &network = networks.createNestedObject();
    //     network["ssid"] = WiFi.SSID(i);
    //     network["rssi"] = WiFi.RSSI(i);
    //     network["encryption"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Encrypted";
    //   }
    // }
    // if (networks.size() >= 0)
    // {
    //   json = "";
    //   networks.printTo(json);
    //   Serial.print("Json Data :");
    //   Serial.println(json);
    //   serverOn = false;
    // }
    // Delay before next scan
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

*/

/*
void SpiffsOfflineData(void *parameter)
{
  for (;;)
  {
    if (SpiffsTimerStart)
    {
      SpiffsTimerStart = false;
      bool datasent = true;
      HTTPClient http; // Declare object of class HTTPClient
      Serial.println("Sending offline data to server");
      mountingSpiffs();
      if (WiFi.status() == WL_CONNECTED)
      {
        digitalWrite(ORANGELED, LOW);
        Network_status = false;
        if (client.connect(servername, 80))
        {
          Network_status = true;
          File csv = SPIFFS.open("/OfflineData.csv", "r");
          Serial.println("Connected to Internet ./");
          WifiStatusConnected();
          if (csv.available())
          {
            Serial.println("Sending Offline data to server");
            const byte BUFFER_SIZE = 200;
            char buffer[BUFFER_SIZE + 1];
            buffer[BUFFER_SIZE] = '\0';
            int j = 0; // Initialize j outside the loop
            int file_count = 0;
            int success_count = 0;
            Serial.print("Inside csv");
            while (csv.available())
            {
              file_count += 1;
              Serial.println("file read fun");
              Serial.print(csv.read());
              String line = csv.readStringUntil('\n');
              Serial.println("line");
              Serial.print(line);
              const byte BUFFER_SIZE = 200;
              char buffer[BUFFER_SIZE + 1];
              buffer[BUFFER_SIZE] = '\0';
              // Copy the line content to the buffer
              strncpy(buffer, line.c_str(), sizeof(buffer));
              char *ptr = strtok(buffer, ",");
              int j = 0;
              String smp_String;
              String EmpId;
              String Date;
              String Temp_Time;
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
                JSONencoder["date"] = Date; // String(now.year()) + String("-") + String(now.month(), DEC) + String("-") + String(now.day());
                JSONencoder["time"] = Time; // String(now.hour()) + String(":") + String(now.minute()) + String(":") + String(now.second());
                char JSONmessageBuffer[300];
                JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
                Serial.print("JSON MESSAGE server send");
                Serial.println(JSONmessageBuffer);
                // http.begin("http://13.126.195.214:8080/EmployeeAttendenceAPI/employee/EmployeeCheckInOut"); // Specify request destination
                http.begin("http://3.6.171.29:8080/EmployeeAttendenceAPI/employee/EmployeeCheckInOut");
                http.addHeader("Content-Type", "application/json"); // Specify content-type header
                Serial.println("Here after content type");
                int httpCode = http.POST(JSONmessageBuffer); // Send the request
                Serial.print("HttpCode:");
                Serial.println(httpCode);
                if (httpCode == 200)
                {
                  Serial.println("Data Send Successfully");
                  success_count += 1;
                }
                http.end();
              }
            }
            csv.close();
            SPIFFS.remove("/OfflineData.csv");
          }
        }
        else
        {
          WifiStatusConnected();
          Serial.println("Internet is Not Available ........ ... ... ..");
          // ws1.textAll("Internet Not Available");
          Network_status = false;
        }
      }
      else
      {
        Serial.println("WiFi was not connected offline data ");
        WifiStatusNotConnected();
        Network_status = false;
        digitalWrite(ORANGELED, HIGH);
        int stationCount = WiFi.softAPgetStationNum();
        if (stationCount <= 0)
        {
          WifiConnectCheck();
        }
      }
      Serial.println("End of send server ");
      serverOn = false;
    }
    vTaskDelay(10000 / portTICK_PERIOD_MS);
  }
}

*/

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
      if (WiFi.status() == WL_CONNECTED)
      {
        if (client.connect(servername, 80))
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
              // delay(1000); // Wait for 1 seconds before making the next request
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
        else
        {
          Serial.println("Internet was not connected");
        }
      }
      else
      {
        Serial.println("Wifi Was Not Connected");
      }
      digitalWrite(REDLED, LOW);
      digitalWrite(GREENLED, LOW);
      digitalWrite(REDLED1, LOW);
      digitalWrite(GREENLED1, LOW);
    }
    vTaskDelay(30000 / portTICK_PERIOD_MS);
  }
}

/*

void initializeCoreWork()
{
  // Creating the WiFi scanning task on core 0


  xTaskCreatePinnedToCore(
      WiFiscanTask,
      "WiFiscan",
      8192,
      1,
      &WiFiscanTaskHandle,
      1
  );

  // Update Employee Details
  xTaskCreatePinnedToCore(
      updateEmployeeDetails,
      "UpdateEmployee",
      2048,
      NULL,
      2,
      &UpdateEmployeeDetail,
      1);

  // Spiffs Offline Data
  xTaskCreatePinnedToCore(
      SpiffsOfflineData,
      "OfflineData",
      3072,
      NULL,
      2,
      &SpiffsOfflineDataC,
      0);
  vTaskStartScheduler();

  //  xTaskNotify(SpiffsOfflineDataC, 1, eSetValueWithOverwrite); // Notify the task
}
*/

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

      if (WiFi.status() == WL_CONNECTED)
      {
        digitalWrite(ORANGELED, LOW);
        Network_status = false;
        if (client.connect("www.google.com", 80))
        {
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
        else
        {
          Serial.println("Internet is Not Available ........ ... ... ..");
          Network_status = false;
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
    }
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
  if (WiFi.status() == WL_CONNECTED)
  {
    if (client.connect(servername, 80))
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
              // String BioTempD = employeeInfoList[i]["biometricTemplateId1"].as<const char *>();
              // String BioRegs = employeeInfoList[i]["biometricFingerPrintStatus"].as<const char *>();
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
              // int Bioregis = BioRegs.toInt();
              // int BioTemps = BioTempD.toInt();
              // if (BioTempD == "")
              // {
              //   BioTemps = 0;
              // }
              // if (Bioregis == 2)
              // {
              //   Serial.print("Inside delete Finger");
              //   delay(1000);
              //   Serial.println(BioTemps);
              //   // deleteFingerprint(BioTemps);

              //   esp_task_wdt_reset();
              // }

              // if (BioRegs == "")
              // {
              //   Bioregis = 0;
              // }
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
          SPIFFS.remove("/Rfid.csv");
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

/*
void rfidInitialCheck()
{
  mountingSpiffs();
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
      Serial.print("File Removed");
    }
    else
    {
      Serial.print("File Already There");
      DataFile.close();
    }
  }
}

*/

void DeviceidFetch()
{
  HTTPClient http;
  int httpCode = 0;
  if (WiFi.status() == WL_CONNECTED)
  {
    if (client.connect("www.google.com", 80))
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
}

bool isInternetAvailable()
{
  WiFiClient client;
  return client.connect("www.google.com", 80); // Check by trying to connect to Google
}

// restart the esp32
void RestartEsp()
{
  ESP.restart();
}

int fetchEmployeeDetails()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    if (client.connect(servername, 80))
    {
      serverOn = true;
      mountingSpiffs();
      DeviceIdInitialize();
      int dataCount = 0;
      int startCount = 0;
      const int endCount = 15;
      int loop_count = 0;
      int httpCode = 0;
      int falseReturn = 0;
      File dataFile = SPIFFS.open("/EmpRfid.csv", "w");
      if (!dataFile)
      {
        Serial.println("Failed to open file for writing");
      }
      do
      {
        digitalWrite(REDLED, HIGH);
        digitalWrite(GREENLED, HIGH);
        digitalWrite(REDLED1, HIGH);
        digitalWrite(GREENLED1, HIGH);
        HTTPClient http; // Declare object of class HTTPClient
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
        esp_task_wdt_reset();
        Serial.println("Hello");
        http.begin("https://wildfly.letzcheckin.com:443/EmployeeAttendenceAPI/RFIDAPI/SelectEmployeeInfo");
        Serial.print("http begin");
        http.addHeader("Content-Type", "application/json"); // Specify content-type header
        Serial.println("Post Main");
        esp_task_wdt_reset();
        yield();
        // Sending requests startes here ..
        httpCode = http.POST(JSONmessageBuffer); // Send the request
        Serial.print("HTTP Code ");
        Serial.println(httpCode);
        esp_task_wdt_reset();
        yield();
        String payload = http.getString();
        esp_task_wdt_reset();
        char *payloadBuffer; // Adjust the size based on your requirement
        payload.toCharArray(payloadBuffer, sizeof(payloadBuffer));
        Serial.print("pay load :");
        Serial.println(payload);
        if (httpCode == 200)
        {
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
            int node_length = nodes.size();
            for (int i = 0; i < node_length; i++)
            {
              File dataFile = SPIFFS.open("/EmpRfid.csv", "a");
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
              dataFile.print(empData);
              Serial.println("Written");
              dataFile.close();
            }
            Serial.println("Forloop end ");
          }
          Serial.println("ELSe end ");
          // endCount += 15;
          yield();
          // delay(1000); // Wait for 1 seconds before making the next request
          Serial.println(httpCode);
          Serial.println("SucessFully Stored RFID Values");
        }
        else
        {
          falseReturn += 1;
          if (falseReturn > 2)
          {
            Serial.println("Http response Failed");
            digitalWrite(REDLED, LOW);
            digitalWrite(GREENLED, LOW);
            digitalWrite(REDLED1, LOW);
            digitalWrite(GREENLED1, LOW);
            http.end(); // Close connection
            serverOn = false;
            break;
          }
        }
        http.end(); // Close connection
      } while (dataCount >= startCount);
      EEPROM.write(510, 1);
      EEPROM.commit(); // Save data to EEPROM
      serverOn = false;
      return httpCode;
    }
    else
    {
      Serial.println("Internet was not connected");
      return 0;
    }
  }
  else
  {
    Serial.println("Wifi Was Not Connected");
    serverOn = false;
    return 0;
    // WifiStatusNotConnected();
  }
  digitalWrite(REDLED, LOW);
  digitalWrite(GREENLED, LOW);
  digitalWrite(REDLED1, LOW);
  digitalWrite(GREENLED1, LOW);
  serverOn = false;
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

  // Check Wi-Fi connection status and internet availability
  if (WiFi.status() == WL_CONNECTED && isInternetAvailable())
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
      rfidEntry["rfid"] = rfid;        // Assign the RFID value to the object
      rfidEntry["employeeId"] = Empid;
      registrationList.add(rfidEntry); // Add the object to the array
      //Convert to String ...
      String regsLists ="";
      registrationList.printTo(regsLists);
      // Build the main JSON object
      jsonDoc["companyId"] = CompanyId;               // Assuming CompanyId is globally defined
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
    // registerSendtoBios();

    // checking if Mainpage is properly intialized
    request->send(200, "text/html", Mainpage); 
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
  // server.on("/BiometricImg", HTTP_GET, [](AsyncWebServerRequest *request)
  //           { request->send(SPIFFS, "/Biometric.svg", "image/svg+xml"); });

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

  /*
  server.on("/quickSetting.js", HTTP_GET, [](AsyncWebServerRequest *request)
            {
        Serial.print("quick setup called");
        if (SPIFFS.exists("/quickSetting.js")) {
            request->send(SPIFFS, "/quickSetting.js", "text/plain");
        } else {
            request->send(404, "text/plain", "File not found");
        } });

        */
  // RFID Page

  server.on("/upload", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/upload.html", "text/html"); });
  server.on("/File Test", HTTP_GET, [](AsyncWebServerRequest *request) { // fileReadAndWrite();
    request->send(200, "Done", "text/plain");
  });
  server.on("/Biometric", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              RegistrationFinger = false;
              // fileReadAndWrite();
              // get the page

              // bool orgStatus = OrganizationStatus();
              // if(orgStatus){

              // }
              if (WiFi.status() != WL_CONNECTED)
              {
                request->send(200, "text/plane", "Connect Device to Network");
              }
              else
              {
                if (!(client.connect("www.google.com", 80)))
                {
                  request->send(200, "text/plane", "Check the Internet");
                }
              }
              if (!SPIFFS.exists("/Rfid.csv"))
              {
                SPIFFS.remove("/Rfid.csv");
                esp_restart();
              }
              if (!SPIFFS.exists("/BioRegs.csv"))
              {
                File datas = SPIFFS.open("/BioRegs.csv", "w");
                datas.close();
              }
              RfidRegisterPage = true;
              if (RfidRegisterPage)
              {
                RfidRegister = true;
                // rfidInitialCheck();
                request->send(SPIFFS, "/Biometric.html", "text/html");
              }
              else
              {
                request->send(200, "text/plane", "UnAuthorised Access");
              } });
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
  // server.on("/EmployeeLists", HTTP_GET, [](AsyncWebServerRequest *request)
  //           {
  //   File csv_File = SPIFFS.open("/Rfid.csv", "r");
  //   if (csv_File) {
  //     Serial.print("Opend csv File");
  //     request->send(SPIFFS, "/Rfid.csv", "text/csv");
  //     csv_File.close();
  //   }
  //   else {
  //     Serial.print("File not Found");
  //     request->send(404, "text/plain", "File Not Found");
  //   } });

  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request) {}, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
            {
    if (!index) {
      filename = "Rfid.csv";
      Serial.printf("UploadStart: %s\n", filename.c_str());
      if (SPIFFS.exists("/Rfid.csv")) {
        SPIFFS.remove("/Rfid.csv");
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

  // server.on("/EmployeeRfidLists", HTTP_GET, [](AsyncWebServerRequest *request)
  //           {
  //   File csv_File = SPIFFS.open("/EmpRfid.csv", "r");
  //   if (csv_File) {
  //     Serial.print("Opend csv File");
  //     request->send(SPIFFS, "/EmpRfid.csv", "text/csv");
  //     csv_File.close();
  //   }
  //   else {
  //     Serial.print("File not Found");
  //     request->send(404, "text/plain", "FIle Not Found");
  //   } });

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

  /*

  server.on("/Wifi_submit", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
{
    EEPROM.begin(512);  // Initialize EEPROM with 512 bytes.
    String ssid = "";
    String pass = "";
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.parseObject(data);

    if (json.success()) {
        ssid = json["SSID"].as<String>();  // Extract SSID
        pass = json["PASSWORD"].as<String>();  // Extract Password

        Serial.println("Received WiFi credentials:");
        Serial.println("SSID: " + ssid);
        Serial.println("Password: " + pass);

        if (ssid.length() > 0) {
            Serial.println("Clearing EEPROM...");
            for (int i = 0; i < 94; ++i) {  // Clear EEPROM where SSID and password will be stored.
                EEPROM.write(i, 0);
            }

            Serial.println("Writing SSID and Password to EEPROM...");
            for (int i = 0; i < ssid.length() && i < 32; ++i) {  // Write SSID to EEPROM, up to 32 characters.
                EEPROM.write(i, ssid[i]);
            }
            for (int i = 0; i < pass.length() && i < 32; ++i) {  // Write Password to EEPROM, up to 32 characters.
                EEPROM.write(i + 32, pass[i]);
            }

            EEPROM.commit();  // Save changes to EEPROM
            Serial.println("Credentials stored successfully.");

            WiFi.disconnect();  // Disconnect if connected
            WiFi.begin(ssid.c_str(), pass.c_str());  // Start connection with new credentials

            unsigned long startAttemptTime = millis();
            int count = 0;
            // Attempt to connect to WiFi for up to 10 seconds
            while (WiFi.status() != WL_CONNECTED && count ) {
                delay(500);
                Serial.print(".");
            }

            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("\nWiFi connected successfully.");
                request->send(200, "text/plain", "WiFi submitted and connected successfully.");
            } else {
                Serial.println("\nFailed to connect to WiFi.");
                request->send(404, "text/plain", "WiFi submitted but could not connect.");
            }
        } else {
            request->send(400, "text/plain", "Invalid SSID provided.");
        }
    } else {
        request->send(400, "text/plain", "Failed to parse JSON.");
    }
});
*/

  // Updated Rfid

  /*
  server.on("/RfiduidUpdate", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            { Serial.println("Rfid Update called"); });
  */
  /*
    server.on("/RfiduidUpdate", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
              {
      HTTPClient http;
      int httpCode = 0;
      RfidRegister = false;
      String lenSize = "";
      Serial.println("button Pressed");
      Serial.print("RFID reg");
      Serial.println(RfidRegister);
      String jsonString;
      DynamicJsonBuffer jsonBuffer;
      JsonArray& jsonArray = jsonBuffer.parseArray(data);
      jsonArray.printTo(Serial);//fetch Response
      JsonArray& jsonArrays = jsonBuffer.createArray();  // Replace jsonArrays with the variable/data you want to print
      int count = 0;
      if (jsonArray.success()) {
        for (JsonObject& json : jsonArray) {
          JsonObject &JSONencoder = jsonBuffer.createObject();
          String employeeId = json["employeeId"];
          String rfid = json["rfid"];
          String lengthSize = json["lenSize"];
          Serial.println(employeeId);
          JSONencoder["rfid"] = rfid;
          JSONencoder["employeeId"] = employeeId;
          Serial.println(rfid);
          jsonArrays.add(JSONencoder);
          lenSize = lengthSize;
          //jsonString += '\n';
        }
      }
      else {
        Serial.println("JSON parsing failed");
      }
      Serial.print("Array: ");
      String rfidArray;
      jsonArrays.printTo(rfidArray);
      Serial.println(rfidArray);
      Serial.print("Len Size: ");
      Serial.println(lenSize);
      int count = 0;
      while(lenSize.toInt() > count){
      JsonObject &JSONencoder = jsonBuffer.createObject();
      Serial.print("Json Array");
      jsonArrays.printTo(Serial);
      char JSONmessageBuffer[500];
      String rfidArray;
      jsonArrays.printTo(rfidArray);
      JSONencoder["companyId"] = CompanyId;
      JSONencoder["employeeId"] = "001";
      JSONencoder["registrationList"] =  rfidArray;
      JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
      Serial.print("Encoded message");
      Serial.println(JSONmessageBuffer);
      // http.begin("https://wildfly.tictoks.in:443/EmployeeAttendenceAPI/RFIDAPI/RegisterEmployeeRFIDInfo");
      // .letzcheckin.com:443
      http.begin("https://wildfly.letzcheckin.com:443/EmployeeAttendenceAPI/RFIDAPI/RegisterEmployeeRFIDInfo");
      http.addHeader("Content-Type", "application/json");
      httpCode = http.POST(JSONmessageBuffer);
      String payload = http.getString();
      Serial.println(payload);
      Serial.print("httpCode");
      Serial.println(httpCode);
      }

      /*
      JsonObject &JSONencoder = jsonBuffer.createObject();
      Serial.print("Json Array");
      jsonArrays.printTo(Serial);
      char JSONmessageBuffer[500];
      String rfidArray;
      jsonArrays.printTo(rfidArray);
      JSONencoder["companyId"] = CompanyId;
      JSONencoder["employeeId"] = "001";
      JSONencoder["registrationList"] =  rfidArray;
      JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
      Serial.print("Encoded message");
      Serial.println(JSONmessageBuffer);
      http.begin("https://wildfly.tictoks.in:443/EmployeeAttendenceAPI/RFIDAPI/RegisterEmployeeRFIDInfo");
      http.addHeader("Content-Type", "application/json");
      httpCode = http.POST(JSONmessageBuffer);
      String payload = http.getString();
      Serial.println(payload);
      Serial.print("httpCode");
      Serial.println(httpCode);
      ///
      request->send(200, "text/plain", "Rfid Updated"); });
   */

  /*
  server.on("/RfiduidUpdate", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
            {
    Serial.println("Button Pressed");
    DynamicJsonBuffer jsonBuffer;
    JsonArray& jsonArray = jsonBuffer.parseArray(data);

    if (!jsonArray.success()) {
        Serial.println("JSON parsing failed");
        request->send(400, "application/json", "{\"status\":\"JSON parsing failed\"}");
        return;
    }

    Serial.println("JSON Parsed Successfully");
    String lenSize;
    String companyId = "001";
    JsonArray& jsonArrays = jsonBuffer.createArray();

    // Parse the incoming JSON data
    for (JsonObject& json : jsonArray) {
        String employeeId = json["employeeId"];
        String rfid = json["rfid"];
        String lengthSize = json["lenSize"];
        lenSize = lengthSize;

        JsonObject& JSONencoder = jsonBuffer.createObject();
        JSONencoder["rfid"] = rfid;
        JSONencoder["employeeId"] = employeeId;
        jsonArrays.add(JSONencoder);
    }

    // Create the final JSON structure
    JsonObject& finalJson = jsonBuffer.createObject();
    finalJson["companyId"] = companyId;
    finalJson["employeeId"] = "001";  // Replace "001" with dynamic data if needed
    finalJson["registrationList"] = jsonArrays;

    // Convert JSON to string
    String jsonString;
    finalJson.printTo(jsonString);

    // Append the JSON string to the file
    File rfidregisterStatus = SPIFFS.open("/RfidRegisterStatus.json", FILE_APPEND);
    if (!rfidregisterStatus) {
        Serial.println("Failed to open file for appending");
        request->send(500, "text/plain", "Failed to open file");
        return;
    }

    if (rfidregisterStatus.print(jsonString + "\n")) {  // Add newline for readability
        Serial.println("Data written to file: ");
        Serial.println(jsonString);
    } else {
        Serial.println("Failed to write data to file");
    }

    rfidregisterStatus.close();  // Ensure file is closed properly

    // Respond to the client after processing all data
    request->send(200, "text/plain", "Rfid Updated"); });

  */
  // Finger validation check MAM

  /*
   server.on("/fingerValidCheck", HTTP_GET, [](AsyncWebServerRequest *request){
      Serial.println("Finger is check ");
      // FingerValidCheck = false;
      RegistrationFinger = false;
      int fingerCountsensor = 0;

      while(fingerCountsensor < 20){
      esp_task_wdt_reset();
      Serial.print("Finger sensor id: ");

      uint8_t p = finger.getImage();
      if (p != FINGERPRINT_OK)
      {
        fingerCountsensor ++;
        // Serial.println("Failed to take image");
          // Use -1 to indicate failure
      }

      p = finger.image2Tz();
      if (p != FINGERPRINT_OK)
      {
        Serial.println("Failed to convert image");
          // Use -1 to indicate failure
      }

      p = finger.fingerFastSearch();
      if (p == FINGERPRINT_OK)
      {
        Serial.println("Finger Already Registered");
        request->send(502,"Finger is Registered");
        break;
      }
      else if (p!= FINGERPRINT_OK){
          Serial.println("Print Not Matched");
          request->send(200,"Finger is Not Registered");
          if(fingerCountsensor > 10){
            break;
          }
        }

    }
    // request->send(502,"Finger Time Out");
  });


  server.on("/fingerValidCheck", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println("Finger is being checked");
      RegistrationFinger = false;
      int fingerCountsensor = 0;

      bool fingerFound = false;  // Flag to track if fingerprint is found

      while (fingerCountsensor < 20) {
          esp_task_wdt_reset();  // Reset watchdog timer

          Serial.print("Finger sensor id: ");
          uint8_t p = finger.getImage();
          if (p != FINGERPRINT_OK) {
              fingerCountsensor++;
              delay(100);  // Add a small delay to avoid rapid retries
              continue;
          }

          p = finger.image2Tz();
          if (p != FINGERPRINT_OK) {
              Serial.println("Failed to convert image");
              fingerCountsensor++;
              delay(100);
              continue;
          }

          p = finger.fingerFastSearch();
          if (p == FINGERPRINT_OK) {
              Serial.println("Finger Already Registered");
              fingerFound = true;  // Fingerprint is registered
              break;
          } else if (p != FINGERPRINT_OK) {
              Serial.println("Print Not Matched");
              if (fingerCountsensor > 10) {
                  break;  // Exit if too many failed attempts
              }
              delay(100);
          }
      }

      if (fingerFound) {
          request->send(200, "application/json", "{\"status\":\"Finger is Registered\"}");
      } else {
          request->send(200, "application/json", "{\"status\":\"Finger is Not Registered\"}");
      }
  });
  */

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
  server.begin();
}

bool pingServer(const char *host)
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

// Check WiFi and Internet connection
void CheckNetwork()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("WiFi Connected");
    if (pingServer(servername))
    {
      Serial.println("Internet is available");
    }
    else
    {
      Serial.println("Internet is not available");
    }
  }
  else
  {
    Serial.println("WiFi Not Connected");
  }
}

bool fetchTimer(void *)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    if (client.connect(servername, 80))
    {
      fetchRunning_update = true;
      int status = fetchEmployeeDetails();
      Serial.print("Status:");
      Serial.println(status);
      fetchRunning_update = false;
    }
  }
  return true;
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
  // delay(1000);
  return 1;
}

// Function For Sending data to Backend
/*
void ServerSend(String empId, String companyId)
{
  Serial.print("EMPID");
  Serial.println(empId);
  Serial.print("COMPANY ID");
  Serial.println(companyId);
  DynamicJsonBuffer JSONbuffer(300);
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
  JSONencoder["employeeId"] = empId;
  JSONencoder["companyId"] = companyId;
  JSONencoder["deviceType"] = DeviceType;
  JSONencoder["date"] = String(now.year()) + String("-") + String(now.month(), DEC) + String("-") + String(now.day());
  JSONencoder["time"] = String(now.hour()) + String(":") + String(now.minute()) + String(":") + String(now.second());
  Serial.println(String(now.hour()) + String(":") + String(now.minute()) + String(":") + String(now.second()));
  char JSONmessageBuffer[300];
  JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  Serial.print("JSON MESSAGE server send");
  Serial.println(JSONmessageBuffer);
  // http.begin("http://13.126.195.214:8080/EmployeeAttendenceAPI/employee/EmployeeCheckInOut"); // Specify request destination
  http.begin("http://3.6.171.29:8080/EmployeeAttendenceAPI/employee/EmployeeCheckInOut");
  http.addHeader("Content-Type", "application/json"); // Specify content-type header
  Serial.println("Here after content type");
  int httpCode = http.POST(JSONmessageBuffer); // Send the request
  Serial.print("HttpCode:");
  Serial.println(httpCode);
  if (httpCode == 200)
  {
    Serial.println("inside http code ");
    // const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 600;
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
    // Serial.println(http.getString()); dont use this line it won't work
    Serial.print("Code return element = ");
    Serial.println("EmployeeId ");
    Serial.println(code);
    Serial.println("Employee name ");
    Serial.println(userName);
    Serial.println("Status name ");
    Serial.println(printStatus);
    if ((strcmp(retStatus, "CHECKIN") == 0))
    {
      digitalWrite(GREENLED, HIGH);  // turn the LED off.
      digitalWrite(GREENLED1, HIGH); // turn the LED off.
      digitalWrite(Relay, HIGH);
      digitalWrite(Buzzer, HIGH);
      delay(300);
      digitalWrite(GREENLED, LOW); // turn the LED off.
      digitalWrite(GREENLED1, LOW);
      delay(200);
      digitalWrite(GREENLED, HIGH);  // turn the LED off.
      digitalWrite(GREENLED1, HIGH); // turn the LED off.
      digitalWrite(Buzzer, LOW);
      delay(4000);
      digitalWrite(Relay, LOW);
    }
    else if ((strcmp(retStatus, "CHECKOUT") == 0))
    {
      digitalWrite(GREENLED, HIGH); // turn the LED off.
      digitalWrite(GREENLED1, HIGH);
      digitalWrite(Relay, HIGH);
      digitalWrite(Buzzer, HIGH);
      delay(300);
      digitalWrite(GREENLED, LOW); // turn the LED off.s
      digitalWrite(GREENLED1, LOW);
      delay(200);
      digitalWrite(GREENLED, HIGH); // turn the LED off.
      digitalWrite(GREENLED1, HIGH);
      digitalWrite(Buzzer, LOW);
      delay(4000);
      digitalWrite(Relay, LOW);
    }
    else if (strcmp(retStatus, "SAME_TIME") == 0)
    {
      digitalWrite(GREENLED, HIGH); // turn the LED off.
      digitalWrite(GREENLED1, HIGH);
      digitalWrite(Relay, HIGH);
      digitalWrite(Buzzer, HIGH);
      delay(300);
      digitalWrite(GREENLED, LOW); // turn the LED off.
      digitalWrite(GREENLED1, LOW);
      digitalWrite(Buzzer, LOW);
      delay(4000);
      digitalWrite(Relay, LOW);
    }
    else if ((strcmp(retStatus, "BLOCKED") == 0))
    {
      UnauthorizedAccess();
      digitalWrite(REDLED, HIGH); // turn the LED off.
      digitalWrite(REDLED1, HIGH);
    }
    else if ((strcmp(retStatus, "NOT_VAILD") == 0))
    {
      UnauthorizedAccess();
      digitalWrite(REDLED, HIGH); // turn the LED off.
      digitalWrite(REDLED1, HIGH);
    }
    else
    {
      Serial.print("Wrong method followed");
    }
    Serial.println(httpCode); // Print HTTP return code
  }
  // NOT_VAILD
  else
  {
    Serial.print("could not send back to server ");
    ServerSend(empId, companyId);
  }
  http.end(); // Close connection
  Serial.println("SuucessFully Send Data To BackEnd");
  i = 0;
  digitalWrite(GREENLED, LOW);
  digitalWrite(GREENLED1, LOW);
  digitalWrite(REDLED, LOW);
  digitalWrite(REDLED1, LOW);
  // added green led
  digitalWrite(GREENLED, HIGH); // turn the LED off.
  digitalWrite(GREENLED1, HIGH);
  delay(1000);
}
*/
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
  if (validRfid)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      if (client.connect("www.google.com", 80))
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
      Serial.println("WiFi not connected");
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

// Function for checking rfid with employee rfid and getting employee's info
/*
bool matchRfid(String rfid)
{
  if (!SPIFFS.begin())
  {
    Serial.println("Failed to mount SPIFFS");
    return false;
  }
  File file = SPIFFS.open("/EmpRfid.csv", "r");
  if (!file)
  {
    Serial.println("Failed to open file");
    return false;
  }
  Serial.println("File opened successfully");
  String line;
  Serial.println(rfid);
  bool rfidFound = false;
  while (file.available())
  {
    line = file.readStringUntil('\n');
    Serial.println(line);
    if (line.indexOf(rfid) != -1)
    {
      rfidFound = true;
      Serial.print("FOund RFID");
      break;
    }
  }
  file.close();
  if (rfidFound)
  {
    const byte BUFFER_SIZE = 200;
    char buffer[BUFFER_SIZE + 1];
    buffer[BUFFER_SIZE] = '\0';
    // Copy the line content to the buffer ;;
    strncpy(buffer, line.c_str(), sizeof(buffer));
    char *ptr = strtok(buffer, ",");
    int j = 0;
    String Rfid;
    String EmpId;
    while (ptr != NULL)
    {
      if (j == 0)
      {
        Rfid = ptr;
      }
      else if (j == 1)
      {
        EmpId = ptr;
      }
      j++;
      ptr = strtok(NULL, ",");
    }
    Serial.print("RFID: ");
    Serial.println(Rfid);
    Serial.print("EmployeeID: ");
    Serial.println(EmpId);
    if (Network_status == true)
    {
      if (client.connect(servername, 80))
      {
        ServerSend(EmpId, CompanyId);
      }
      else
      {
        Serial.println("OFFLINE Preparation");
        Serial.println("DOOR OPEN");
        digitalWrite(Relay, HIGH);
        digitalWrite(GREENLED, HIGH);
        digitalWrite(GREENLED1, HIGH);
        digitalWrite(Buzzer, HIGH);
        delay(300);
        digitalWrite(Buzzer, LOW);
        digitalWrite(GREENLED, LOW);
        digitalWrite(GREENLED1, LOW);
        delay(200);
        digitalWrite(GREENLED, HIGH); // turn the LED off.
        digitalWrite(GREENLED1, HIGH);
        delay(4000);
        digitalWrite(Relay, LOW);
        OfflineDataWrite(EmpId);
      }
    }
    else
    {
      Serial.println("OFFLINE Preparation");
      Serial.println("DOOR OPEN");
      digitalWrite(Relay, HIGH);
      digitalWrite(GREENLED, HIGH);
      digitalWrite(GREENLED1, HIGH);
      digitalWrite(Buzzer, HIGH);
      delay(300);
      digitalWrite(Buzzer, LOW);
      digitalWrite(GREENLED, LOW);
      digitalWrite(GREENLED1, LOW);
      delay(4000);
      digitalWrite(Relay, LOW);
      OfflineDataWrite(EmpId);
      // offline send data should be return ..
    }
    // Perform actions with the parsed RFID and EmployeeID here
  }
  else
  {
    Serial.println("RFID not found in CSV");
    digitalWrite(REDLED, HIGH);
    digitalWrite(REDLED1, HIGH);
    UnauthorizedAccess();
    delay(2000);
    digitalWrite(REDLED, LOW);
    digitalWrite(REDLED1, LOW);
  }
  return rfidFound;
}
*/
// TODO :
void softAp()
{
  // WiFi.mode(WIFI_AP);
  WiFi.softAP(ASSID, APASS);
  Serial.println("AP Started");
  Serial.print("IP Address:");
  Serial.println(WiFi.softAPIP());
}

// spiffs file check
void spiffsFileCheck()
{
  mountingSpiffs();
  if (SPIFFS.exists("/EmpRfid.csv"))
  {
    File DataFile = SPIFFS.open("/EmpRfid.csv");
    size_t fileSize = DataFile.size();
    Serial.print("Rfid File Exists");
    Serial.println(fileSize);
    if (fileSize == 0)
    {
      DataFile.close();
      SPIFFS.remove("/EmpRfid.csv");
      Serial.print("File Removed");
    }
    else
    {
      Serial.print("File Already There");
      DataFile.close();
    }
  }
  else
  {
    Serial.println("File does not exist!");
    Serial.println("Fetching Started");
    int status = fetchEmployeeDetails();
    Serial.print(status);
  }
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

// *Setup
void setup()
{
  Serial.begin(115200);
  EEPROM.read(512);
  printFreeHeap("initialiazation");
  initialavailableWifi();
  //  wifiScanRequested = true; // Set the request flag
  printFreeHeap("After Core Work");
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
  InitializeRfid();
  WebServerRoutes();
  rfidInitialCheck();
  // rfidInitialList();
  // spiffsFileCheck();
  fileReadAndWrite();

  registerSendtoRfid();
  initializeCoreWork();
  digitalWrite(GREENLED1, LOW);
  digitalWrite(GREENLED, LOW);
  digitalWrite(REDLED1, LOW);
  digitalWrite(REDLED, LOW);
  ws.onEvent(onWsEvent);
  // ws1.onEvent(wifiStatusEvent);
  server.addHandler(&ws);
  // server.addHandler(&ws1);
  printFreeHeap("After initiliazation");
  // timer.every(300000, fetchTimer_UpdateActivity);
  // timer.every(10000, SendOfflineDataSpiffs);
  Serial.println("");
  Serial.print("Build Version:");
  Serial.println(Build_Version);
  // Serial.print("Free heap memory: ");
  // Serial.println(heap_caps_get_free_size(MALLOC_CAP_8BIT));
  // heap_caps_print_heap_info(MALLOC_CAP_DEFAULT);
}

// RFID card reading loop.
void loop()
{
  ws.cleanupClients();
  // Serial.println("Loop");
  // Reset the Watchdog Timer
  esp_task_wdt_reset();
  for (uint8_t reader = 0; reader < NO_OF_READERS; reader++)
  {
    // Serial.println("Rfid reader check");
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
  timer.tick();

  wifiConnectedCheckerMin();
}

// /*
//    Full functionality test
//    find error and solve for solution
//    Find opmization
//    Add Message in Device id if already added
// */

// //#include <WiFi.h>
// #include <FS.h>
// #include <SPIFFS.h>

// //const char* ssid = "your_wifi_ssid";
// //const char* password = "your_wifi_password";

// const char* userListFile = "/user_list.txt";

// // Function to initialize WiFi connection
// //void connectWiFi() {
// //  Serial.println("Connecting to WiFi...");
// //  WiFi.begin(ssid, password);
// //  while (WiFi.status() != WL_CONNECTED) {
// //    delay(1000);
// //    Serial.println("Connecting to WiFi...");
// //  }
// //  Serial.println("Connected to WiFi!");
// //}

// // Function to check if user exists in the user list file
// bool userExists(String userId) {
//   File file = SPIFFS.open(userListFile, "r");
//   if (!file) {
//     Serial.println("Failed to open user list file!");
//     return false;
//   }

//   while (file.available()) {
//     String line = file.readStringUntil('\n');
//     if (line.startsWith(userId + ",")) {
//       file.close();
//       return true;
//     }
//   }
//   file.close();

//   return false;
// }

// void setup() {
//   Serial.begin(115200);
// //  connectWiFi();

//   // Initialize SPIFFS
//   if (!SPIFFS.begin()) {
//     Serial.println("Failed to initialize SPIFFS!");
//     while (1);
//   }

//   // Create user list file if not exists
//   if (!SPIFFS.exists(userListFile)) {
//     File file = SPIFFS.open(userListFile, "w");
//     if (!file) {
//       Serial.println("Failed to create user list file!");
//       while (1);
//     }
//     file.close();
//   }
// }

// void loop() {
//   Serial.println("Enter user ID:");
//   while (!Serial.available()) {}
//   String userId = Serial.readString();

//   if (userExists(userId)) {
//     Serial.println("User already exists!");
//   } else {
//     // Get user info
//     Serial.println("Enter user info (FirstName LastName Age):");
//     while (!Serial.available()) {}
//     String firstName = Serial.readString();
//     while (!Serial.available()) {}
//     String lastName = Serial.readString();
//     while (!Serial.available()) {}
//     int age = Serial.parseInt();

//     // Insert user info into user list file
//     File file = SPIFFS.open(userListFile, "a");
//     if (!file) {
//       Serial.println("Failed to open user list file for writing!");
//       return;
//     }
//     file.println(userId + "," + firstName + "," + lastName + "," + String(age));
//     file.close();

//     Serial.println("User info added successfully!");
//   }

//   delay(1000); // Wait for 1 second before next iteration
// }