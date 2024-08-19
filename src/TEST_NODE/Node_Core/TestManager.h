#ifndef TEST_MANAGER_H
#define TEST_MANAGER_H
#include "Arduino.h"
#include "Settings.h"
#include "StateMachine.h"
#include "UPSTesterSetup.h"

using namespace Node_Core;
static bool check_ups_powerUp = false;

static TaskHandle_t ISR_MAINS_POWER_LOSS = NULL;
static TaskHandle_t ISR_UPS_POWER_GAIN = NULL;
static TaskHandle_t ISR_UPS_POWER_LOSS = NULL;
static TaskHandle_t switchTestTaskHandle = NULL;
static TaskHandle_t backupTimeTestTaskHandle = NULL;
static TaskHandle_t efficiencyTestTaskHandle = NULL;
static TaskHandle_t inputvoltageTestTaskHandle = NULL;
static TaskHandle_t waveformTestTaskHandle = NULL;
static TaskHandle_t tunepwmTestTaskHandle = NULL;

struct TaskSettings {

  BaseType_t testtaskCore;
  BaseType_t mainsISRtaskCore;
  BaseType_t upsISRtaskCore;

  UBaseType_t testtaskPr;
  UBaseType_t mainsISRtaskPr;
  UBaseType_t upsISRtaskPr;

  uint32_t testtaskStack;
  uint32_t mainsISRtaskStack;
  uint32_t upsISRtaskStack;
};

struct TaskParams {
  struct allTesttaskParams {
    const char* TestStandard;
    TestMode mode;
    bool flag_mains_power_loss;
    uint16_t inputVoltage_volt;
    uint16_t testVARating;
    unsigned long testDuration;
  } forallTest;
  struct SwitchTestTaskParams {
    bool flag_ups_power_gain;
  } forswitchTest;
  struct BackupTestTaskParams {
    bool flag_ups_power_loss;
    unsigned long maxBackupTime;
  } forbackupTimeTest;
};

struct SwitchTestTaskParams {

  bool flag_mains_power_loss;
  bool flag_ups_power_gain;
  uint16_t testVARating;
  unsigned long testDuration;

  BaseType_t switchtaskCore;
  BaseType_t mainsISRtaskCore;
  BaseType_t upsISRtaskCore;

  UBaseType_t switchtaskPr;
  UBaseType_t mainsISRtaskPr;
  UBaseType_t upsISRtaskPr;

  uint32_t switchtaskStack;
  uint32_t mainsISRtaskStack;
  uint32_t upsISRtaskStack;

  // Default constructor
  SwitchTestTaskParams() {
    init();  // Initialize with default values
  }

  // Copy constructor
  SwitchTestTaskParams(const SwitchTestTaskParams& other) {
    flag_mains_power_loss = other.flag_mains_power_loss;
    flag_ups_power_gain = other.flag_ups_power_gain;
    testVARating = other.testVARating;
    testDuration = other.testDuration;

    switchtaskCore = other.switchtaskCore;
    mainsISRtaskCore = other.mainsISRtaskCore;
    upsISRtaskCore = other.upsISRtaskCore;

    switchtaskPr = other.switchtaskPr;
    mainsISRtaskPr = other.mainsISRtaskPr;
    upsISRtaskPr = other.upsISRtaskPr;

    switchtaskStack = other.switchtaskStack;
    mainsISRtaskStack = other.mainsISRtaskStack;
    upsISRtaskStack = other.upsISRtaskStack;
  }

  // Function to initialize with default values
  void init() {
    flag_mains_power_loss = false;
    flag_ups_power_gain = false;
    testVARating = 4000;
    testDuration = 10000;

    switchtaskCore = 0;
    mainsISRtaskCore = ARDUINO_RUNNING_CORE;
    upsISRtaskCore = ARDUINO_RUNNING_CORE;

    switchtaskPr = 2;
    mainsISRtaskPr = 1;
    upsISRtaskPr = 1;

    switchtaskStack = 12000;
    mainsISRtaskStack = 12000;
    upsISRtaskStack = 12000;
  }

  // Set parameters using another instance
  void setParams(const SwitchTestTaskParams& newSwitchTestparam) {
    flag_mains_power_loss = newSwitchTestparam.flag_mains_power_loss;
    flag_ups_power_gain = newSwitchTestparam.flag_ups_power_gain;
    testVARating = newSwitchTestparam.testVARating;
    testDuration = newSwitchTestparam.testDuration;

    switchtaskCore = newSwitchTestparam.switchtaskCore;
    mainsISRtaskCore = newSwitchTestparam.mainsISRtaskCore;
    upsISRtaskCore = newSwitchTestparam.upsISRtaskCore;

    switchtaskPr = newSwitchTestparam.switchtaskPr;
    mainsISRtaskPr = newSwitchTestparam.mainsISRtaskPr;
    upsISRtaskPr = newSwitchTestparam.upsISRtaskPr;

    switchtaskStack = newSwitchTestparam.switchtaskStack;
    mainsISRtaskStack = newSwitchTestparam.mainsISRtaskStack;
    upsISRtaskStack = newSwitchTestparam.upsISRtaskStack;
  }

  // Retrieve parameters via references
  void getParams(bool& mains_loss, bool& ups_gain, uint16_t& va_rating,
                 unsigned long& duration) const {
    mains_loss = flag_mains_power_loss;
    ups_gain = flag_ups_power_gain;
    va_rating = testVARating;
    duration = testDuration;
  }
};

struct BackupTimeTestParams {
  bool flag_mains_power_loss;
  bool flag_ups_power_shutdown;
  uint16_t testVARating;
  unsigned long maxBackupTime;

  BaseType_t testtaskCore;
  BaseType_t mainsISRtaskCore;
  BaseType_t upsISRtaskCore;

  UBaseType_t testtaskPr;
  UBaseType_t mainsISRtaskPr;
  UBaseType_t upsISRtaskPr;

  uint32_t testtaskStack;
  uint32_t mainsISRtaskStack;
  uint32_t upsISRtaskStack;

  // Default constructor
  BackupTimeTestParams() {
    init();  // Initialize with default values
  }

  // Copy constructor
  BackupTimeTestParams(const BackupTimeTestParams& other) {
    flag_mains_power_loss = other.flag_mains_power_loss;
    flag_ups_power_shutdown = other.flag_ups_power_shutdown;
    testVARating = other.testVARating;
    maxBackupTime = other.maxBackupTime;

    testtaskCore = other.testtaskCore;
    mainsISRtaskCore = other.mainsISRtaskCore;
    upsISRtaskCore = other.upsISRtaskCore;

    testtaskPr = other.testtaskPr;
    mainsISRtaskPr = other.mainsISRtaskPr;
    upsISRtaskPr = other.upsISRtaskPr;

    testtaskStack = other.testtaskStack;
    mainsISRtaskStack = other.mainsISRtaskStack;
    upsISRtaskStack = other.upsISRtaskStack;
  }

  // Function to initialize with default values
  void init() {
    flag_mains_power_loss = false;
    flag_ups_power_shutdown = false;
    testVARating = 0;
    maxBackupTime = 0;

    testtaskCore = 0;
    mainsISRtaskCore = ARDUINO_RUNNING_CORE;
    upsISRtaskCore = ARDUINO_RUNNING_CORE;

    testtaskPr = 2;
    mainsISRtaskPr = 1;
    upsISRtaskPr = 1;

    testtaskStack = 12000;
    mainsISRtaskStack = 12000;
    upsISRtaskStack = 12000;
  }

  // Set parameters using another instance
  void setParams(const BackupTimeTestParams& newBackupTimeTestParam) {
    flag_mains_power_loss = newBackupTimeTestParam.flag_mains_power_loss;
    flag_ups_power_shutdown = newBackupTimeTestParam.flag_ups_power_shutdown;
    testVARating = newBackupTimeTestParam.testVARating;
    maxBackupTime = newBackupTimeTestParam.maxBackupTime;

    testtaskCore = newBackupTimeTestParam.testtaskCore;
    mainsISRtaskCore = newBackupTimeTestParam.mainsISRtaskCore;
    upsISRtaskCore = newBackupTimeTestParam.upsISRtaskCore;

    testtaskPr = newBackupTimeTestParam.testtaskPr;
    mainsISRtaskPr = newBackupTimeTestParam.mainsISRtaskPr;
    upsISRtaskPr = newBackupTimeTestParam.upsISRtaskPr;

    testtaskStack = newBackupTimeTestParam.testtaskStack;
    mainsISRtaskStack = newBackupTimeTestParam.mainsISRtaskStack;
    upsISRtaskStack = newBackupTimeTestParam.upsISRtaskStack;
  }

  // Retrieve parameters via references
  void getParams(bool& mains_loss, bool& ups_shutdown, uint16_t& va_rating,
                 unsigned long& backup_time) const {
    mains_loss = flag_mains_power_loss;
    ups_shutdown = flag_ups_power_shutdown;
    va_rating = testVARating;
    backup_time = maxBackupTime;
  }
};

using namespace Node_Core;

class TestManager {
public:
  TestManager();
  ~TestManager();
  void runTests();
  void manageTests();
  void terminateTest();

private:
  StateMachine _stateManager;
  State _currentstate;
  SwitchTestTaskParams _switchTestConfig;
  BackupTimeTestParams _backupTimeTestConfig;
};

#endif