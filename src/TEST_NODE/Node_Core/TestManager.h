#ifndef TEST_MANAGER_H
#define TEST_MANAGER_H
#include "Arduino.h"
#include "Settings.h"
#include "StateMachine.h"
#include "UPSTesterSetup.h"

static TaskHandle_t switchTestTaskHandle = NULL;
static TaskHandle_t backupTimeTestTaskHandle = NULL;
static TaskHandle_t efficiencyTestTaskHandle = NULL;
static TaskHandle_t inputvoltageTestTaskHandle = NULL;
static TaskHandle_t waveformTestTaskHandle = NULL;
static TaskHandle_t tunepwmTestTaskHandle = NULL;

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
};

#endif