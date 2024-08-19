#include "UPSTesterSetup.h"
#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>

namespace Node_Core {

UPSTesterSetup::UPSTesterSetup() {
  // Initialize LittleFS
  if (!LittleFS.begin()) {
    Serial.println("Failed to mount LittleFS");
  }
}

UPSTesterSetup::~UPSTesterSetup() { LittleFS.end(); }

void UPSTesterSetup::updateSettings(SettingType settingType,
                                    const void* newSetting) {
  switch (settingType) {
    case SettingType::SPEC:
      _spec = *static_cast<const UPSSpec*>(newSetting);
      if (_specSetCallback) {
        _specSetCallback(true, _spec);
      }
      break;

    case SettingType::TEST:
      _testSetting = *static_cast<const TestSetting*>(newSetting);
      if (_testSetCallback) {
        _testSetCallback(true, _testSetting);
      }
      break;

    case SettingType::NETWORK:
      _networkSetting = *static_cast<const NetworkSetting*>(newSetting);
      if (_commSetCallback) {
        _commSetCallback(true, _networkSetting);
      }
      break;

    case SettingType::REPORT:
      _reportSetting = *static_cast<const ReportSetting*>(newSetting);
      if (_reportSetCallback) {
        _reportSetCallback(true, _reportSetting);
      }
      break;

    default:
      Serial.println("Unknown setting type");
      break;
  }
  serializeSettings("/tester_settings.json");
}

void UPSTesterSetup::setSettings(SettingType settingType,
                                 const void* newSetting) {
  switch (settingType) {
    case SettingType::SPEC:
      _spec = *static_cast<const UPSSpec*>(newSetting);
      break;

    case SettingType::TEST:
      _testSetting = *static_cast<const TestSetting*>(newSetting);
      break;

    case SettingType::NETWORK:
      _networkSetting = *static_cast<const NetworkSetting*>(newSetting);
      break;

    case SettingType::REPORT:
      _reportSetting = *static_cast<const ReportSetting*>(newSetting);
      break;

    default:
      Serial.println("Unknown setting type");
      return;
  }

  serializeSettings("/tester_settings.json");

  // Notify all settings applied if applicable
  notifyAllSettingsApplied();
}

void UPSTesterSetup::loadSettings(SettingType settingType,
                                  const void* newSetting) {

  deserializeSettings("/tester_settings.json");
}

void UPSTesterSetup::serializeSettings(const char* filename) {
  DynamicJsonDocument doc(1024);

  // Fill JSON with current settings
  doc["spec"]["Rating_va"] = _spec.Rating_va;
  doc["spec"]["Voltage_volt"] = _spec.Voltage_volt;
  doc["spec"]["Current_amp"] = _spec.Current_amp;
  doc["spec"]["MinVoltage_volt"] = _spec.MinVoltage_volt;
  doc["spec"]["MaxVoltage_volt"] = _spec.MaxVoltage_volt;
  doc["spec"]["AvgSwitchTime_ms"] = _spec.AvgSwitchTime_ms;
  doc["spec"]["AvgBackupTime_ms"] = _spec.AvgBackupTime_ms;

  doc["test"]["TestStandard"] = _testSetting.TestStandard;
  doc["test"]["mode"] = static_cast<int>(_testSetting.mode);
  doc["test"]["inputVoltage_volt"] = _testSetting.inputVoltage_volt;
  doc["test"]["ToleranceSwitchTime_ms"] = _testSetting.ToleranceSwitchTime_ms;
  doc["test"]["ToleranceBackUpTime_ms"] = _testSetting.ToleranceBackUpTime_ms;
  doc["test"]["MaxRetest"] = _testSetting.MaxRetest;

  doc["network"]["ssid"] = _networkSetting.ssid;
  doc["network"]["pass"] = _networkSetting.pass;
  doc["network"]["localIP"] = _networkSetting.localIP.toString();
  doc["network"]["gateway"] = _networkSetting.gateway.toString();
  doc["network"]["subnet"] = _networkSetting.subnet.toString();
  doc["network"]["max_retry"] = _networkSetting.max_retry;
  doc["network"]["reconnectTimeout_ms"] = _networkSetting.reconnectTimeout_ms;
  doc["network"]["networkTimeout_ms"] = _networkSetting.networkTimeout_ms;
  doc["network"]["refreshConnectionAfter_ms"]
      = _networkSetting.refreshConnectionAfter_ms;
  doc["network"]["idModbusSlave1"] = _networkSetting.idModbusSlave1;
  doc["network"]["baudRateSlave1"] = _networkSetting.baudRateSlave1;
  doc["network"]["idModbusSlave2"] = _networkSetting.idModbusSlave2;
  doc["network"]["baudRateSlave2"] = _networkSetting.baudRateSlave2;

  doc["report"]["clientName"] = _reportSetting.clientName;
  doc["report"]["brandName"] = _reportSetting.brandName;
  doc["report"]["serialNumber"] = _reportSetting.serialNumber;
  doc["report"]["sampleNumber"] = _reportSetting.sampleNumber;

  // Open file for writing
  File file = LittleFS.open(filename, "w");
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  // Serialize JSON to file
  serializeJson(doc, file);
  file.close();
}

void UPSTesterSetup::deserializeSettings(const char* filename) {
  // Open file for reading
  File file = LittleFS.open(filename, "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.println("Failed to read file, using default settings");
    file.close();
    return;
  }
  file.close();

  // Load settings from JSON
  _spec.Rating_va = doc["spec"]["Rating_va"] | _spec.Rating_va;
  _spec.Voltage_volt = doc["spec"]["Voltage_volt"] | _spec.Voltage_volt;
  _spec.Current_amp = doc["spec"]["Current_amp"] | _spec.Current_amp;
  _spec.MinVoltage_volt
      = doc["spec"]["MinVoltage_volt"] | _spec.MinVoltage_volt;
  _spec.MaxVoltage_volt
      = doc["spec"]["MaxVoltage_volt"] | _spec.MaxVoltage_volt;
  _spec.AvgSwitchTime_ms
      = doc["spec"]["AvgSwitchTime_ms"] | _spec.AvgSwitchTime_ms;
  _spec.AvgBackupTime_ms
      = doc["spec"]["AvgBackupTime_ms"] | _spec.AvgBackupTime_ms;

  _testSetting.TestStandard
      = doc["test"]["TestStandard"] | _testSetting.TestStandard;
  _testSetting.mode = static_cast<TestMode>(
      doc["test"]["mode"] | static_cast<int>(_testSetting.mode));
  _testSetting.inputVoltage_volt
      = doc["test"]["inputVoltage_volt"] | _testSetting.inputVoltage_volt;
  _testSetting.ToleranceSwitchTime_ms = doc["test"]["ToleranceSwitchTime_ms"]
                                        | _testSetting.ToleranceSwitchTime_ms;
  _testSetting.ToleranceBackUpTime_ms = doc["test"]["ToleranceBackUpTime_ms"]
                                        | _testSetting.ToleranceBackUpTime_ms;
  _testSetting.MaxRetest = doc["test"]["MaxRetest"] | _testSetting.MaxRetest;

  _networkSetting.ssid = doc["network"]["ssid"] | _networkSetting.ssid;
  _networkSetting.pass = doc["network"]["pass"] | _networkSetting.pass;
  _networkSetting.localIP.fromString(
      doc["network"]["localIP"].as<const char*>());
  _networkSetting.gateway.fromString(
      doc["network"]["gateway"].as<const char*>());
  _networkSetting.subnet.fromString(doc["network"]["subnet"].as<const char*>());
  _networkSetting.max_retry
      = doc["network"]["max_retry"] | _networkSetting.max_retry;
  _networkSetting.reconnectTimeout_ms = doc["network"]["reconnectTimeout_ms"]
                                        | _networkSetting.reconnectTimeout_ms;
  _networkSetting.networkTimeout_ms
      = doc["network"]["networkTimeout_ms"] | _networkSetting.networkTimeout_ms;
  _networkSetting.refreshConnectionAfter_ms
      = doc["network"]["refreshConnectionAfter_ms"]
        | _networkSetting.refreshConnectionAfter_ms;
  _networkSetting.idModbusSlave1
      = doc["network"]["idModbusSlave1"] | _networkSetting.idModbusSlave1;
  _networkSetting.baudRateSlave1
      = doc["network"]["baudRateSlave1"] | _networkSetting.baudRateSlave1;
  _networkSetting.idModbusSlave2
      = doc["network"]["idModbusSlave2"] | _networkSetting.idModbusSlave2;
  _networkSetting.baudRateSlave2
      = doc["network"]["baudRateSlave2"] | _networkSetting.baudRateSlave2;

  _reportSetting.clientName
      = doc["report"]["clientName"] | _reportSetting.clientName;
  _reportSetting.brandName
      = doc["report"]["brandName"] | _reportSetting.brandName;
  _reportSetting.serialNumber
      = doc["report"]["serialNumber"] | _reportSetting.serialNumber;
  _reportSetting.sampleNumber
      = doc["report"]["sampleNumber"] | _reportSetting.sampleNumber;

  // Notify all settings applied if applicable
  notifyAllSettingsApplied();
}

void UPSTesterSetup::notifyAllSettingsApplied() {
  if (_allSettingCallback) {
    _allSettingCallback(true,
                        {_spec, _testSetting, _networkSetting, _reportSetting});
  }
}

// Register Callback Methods
void UPSTesterSetup::registerSpecCallback(OnSetupSpecCallback callback) {
  _specSetCallback = callback;
}

void UPSTesterSetup::registerTestCallback(OnSetupTestCallback callback) {
  _testSetCallback = callback;
}

void UPSTesterSetup::registerNetworkCallback(OnSetupNetworkCallback callback) {
  _commSetCallback = callback;
}

void UPSTesterSetup::registerReportCallback(OnSetupReportCallback callback) {
  _reportSetCallback = callback;
}

void UPSTesterSetup::registerAllSettingCallback(OnAllSettingCallback callback) {
  _allSettingCallback = callback;
}

void UPSTesterSetup::loadFactorySettings() {
  // Default factory settings for UPSSpec
  UPSSpec factorySpec = {1000.0f, 230.0f, 10.0f, 180.0f, 240.0f, 50.0f, 300.0f};
  updateSettings(SettingType::SPEC, &factorySpec);

  // Default factory settings for TestSetting
  TestSetting factoryTestSetting
      = {"Standard", TestMode::AUTO, 230.0f, 10.0f, 15.0f, 3};
  updateSettings(SettingType::TEST, &factoryTestSetting);

  // Default factory settings for NetworkSetting
  NetworkSetting factoryNetworkSetting = {"SSID",
                                          "password",
                                          IPAddress(192, 168, 1, 100),
                                          IPAddress(192, 168, 1, 1),
                                          IPAddress(255, 255, 255, 0),
                                          3,
                                          1000,
                                          5000,
                                          60000,
                                          1,
                                          9600,
                                          2,
                                          9600};
  updateSettings(SettingType::NETWORK, &factoryNetworkSetting);

  // Default factory settings for ReportSetting
  ReportSetting factoryReportSetting
      = {"ClientName", "BrandName", "SerialNumber", 1};
  updateSettings(SettingType::REPORT, &factoryReportSetting);
}

}  // namespace Node_Core
