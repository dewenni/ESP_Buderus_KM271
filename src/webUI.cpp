#include <LittleFS.h>
#include <Update.h>
#include <basics.h>
#include <favicon.h>
#include <km271.h>
#include <language.h>
#include <message.h>
#include <oilmeter.h>
#include <sensor.h>
#include <simulation.h>
#include <webUI.h>
#include <webUIhelper.h>
#include <webUIupdates.h>

const int MAX_WS_CLIENT = 3;
const int CHUNK_SIZE = 1024;

/* P R O T O T Y P E S ********************************************************/
void webCallback(const char *elementId, const char *value);

/* D E C L A R A T I O N S ****************************************************/
static muTimer heartbeatTimer = muTimer();  // timer to refresh other values
static muTimer simulationTimer = muTimer(); // timer to refresh other values
static muTimer logReadTimer = muTimer();    // timer to refresh other values
static muTimer otaUpdateTimer = muTimer();  // timer to refresh other values
static muTimer onLoadTimer = muTimer();     // timer to refresh other values

EspWebUI webUI(80, faviconSvg);

static const char *TAG = "WEB"; // LOG TAG
static bool webInitDone = false;
static bool simulationInit = false;
static const size_t BUFFER_SIZE = 512;
static bool onLoadRequest = false;

// Web UI commands arrive on the AsyncTCP task and are executed on the loop
// task. This used to be a single element/value pair plus a flag, which lost
// any message that arrived before the previous one was consumed, and could
// hand webCallback() the element ID of one message with the value of the
// next - i.e. apply one control's value to a different config field or boiler
// command. Use a small ring buffer guarded by a critical section, the same
// shape already used for the MQTT command queue.
struct s_webCallbackMsg {
  char elementID[32];
  char value[256];
};
#define WEB_CALLBACK_QUEUE_LEN 8
static s_webCallbackMsg webCallbackQueue[WEB_CALLBACK_QUEUE_LEN];
static volatile uint8_t webCallbackHead = 0;
static volatile uint8_t webCallbackTail = 0;
static portMUX_TYPE webCallbackMux = portMUX_INITIALIZER_UNLOCKED;

// A web OTA that is cut off mid-upload (browser tab closed, WiFi dropped)
// produces no callback at all - ESPAsyncWebServer simply abandons the request.
// The OTA_BEGIN side effects would then stay in force forever: the watchdog
// disabled, ota.isActive() true (which freezes every value in the web UI), and
// Update.begin() never ended (which makes every later web OTA fail). Track the
// last sign of life and unwind the state if it stalls.
#define OTA_STALL_TIMEOUT 60000 // 1 minute without a progress callback
static unsigned long otaLastActivity = 0;

static auto &wdt = EspSysUtil::Wdt::getInstance();
static auto &ota = EspSysUtil::OTA::getInstance();

/**
 * *******************************************************************
 * @brief   cyclic call for webUI - creates all webUI elements
 * @param   none
 * @return  none
 * *******************************************************************/
void webUISetup() {
  webUI.setCallbackOta([](EspWebUI::otaStatus otaState, const char *msg) {
    switch (otaState) {
    case EspWebUI::OTA_BEGIN:
      ota.setActive(true);
      wdt.disable();
      otaLastActivity = millis();
      break;
    case EspWebUI::OTA_PROGRESS:
      webUI.wsUpdateOTAprogress(msg);
      otaLastActivity = millis();
      break;
    case EspWebUI::OTA_FINISH:
      ota.setActive(false);
      wdt.enable();
      webUI.wsUpdateOTAprogress("100");
      webUI.wsUpdateWebDialog("ota_update_done_dialog", "open");
      break;
    case EspWebUI::OTA_ERROR:
      ota.setActive(false);
      wdt.enable();
      webUI.wsUpdateWebText("p00_ota_upd_err", msg, false);
      webUI.wsUpdateWebDialog("ota_update_failed_dialog", "open");
      break;
    }
  });

  webUI.setCallbackUpload([](EspWebUI::uploadStatus uploadState, const char *msg) {
    switch (uploadState) {
    case EspWebUI::UPLOAD_BEGIN:
      webUI.wsUpdateWebText("upload_status_txt", msg, false);
      break;
    case EspWebUI::UPLOAD_FINISH:
      webUI.wsUpdateWebText("upload_status_txt", msg, false);
      configLoadFromFile(); // load configuration
      webUI.wsUpdateWebLanguage(LANG::CODE[config.lang]);
      webUI.wsLoadConfigWebUI(); // update webUI settings
      break;
    case EspWebUI::UPLOAD_ERROR:
      webUI.wsUpdateWebText("upload_status_txt", msg, false);
      break;
    }
  });

  // callback for reload
  webUI.setCallbackReload([]() { onLoadRequest = true; });

  // callback for web elements - queue elementID and value, executed by webCallback() in the cyclic loop
  webUI.setCallbackWebElement([](const char *elementID, const char *elementValue) {
    if (elementID == nullptr || elementValue == nullptr) {
      return;
    }
    portENTER_CRITICAL(&webCallbackMux);
    uint8_t next = (webCallbackHead + 1) % WEB_CALLBACK_QUEUE_LEN;
    bool full = (next == webCallbackTail);
    if (!full) {
      snprintf(webCallbackQueue[webCallbackHead].elementID, sizeof(webCallbackQueue[0].elementID), "%s", elementID);
      snprintf(webCallbackQueue[webCallbackHead].value, sizeof(webCallbackQueue[0].value), "%s", elementValue);
      webCallbackHead = next;
    }
    portEXIT_CRITICAL(&webCallbackMux);
    if (full) {
      ESP_LOGE(TAG, "web callback queue full - command dropped");
    }
  });

  webUI.setCredentials(config.auth.user, config.auth.password);
  webUI.setAuthentication(config.auth.enable);

  webUI.begin();

} // END SETUP

/**
 * *******************************************************************
 * @brief   cyclic call for webUI - refresh elements by change
 * @param   none
 * @return  none
 * *******************************************************************/
void webUICyclic() {

  webUI.loop();

  // request for update alle elements - not faster than every 1s
  if (onLoadRequest && onLoadTimer.cycleTrigger(1000)) {
    updateAllElements();
    onLoadRequest = false;
    ESP_LOGD(TAG, "updateAllElements()");
  }

  // handling of update webUI elements
  webUIupdates();

  // handling of callback information - drain everything queued since last time
  while (true) {
    s_webCallbackMsg msg;
    portENTER_CRITICAL(&webCallbackMux);
    bool haveMsg = (webCallbackTail != webCallbackHead);
    if (haveMsg) {
      msg = webCallbackQueue[webCallbackTail];
      webCallbackTail = (webCallbackTail + 1) % WEB_CALLBACK_QUEUE_LEN;
    }
    portEXIT_CRITICAL(&webCallbackMux);
    if (!haveMsg) {
      break;
    }
    webCallback(msg.elementID, msg.value);
  }

  // Unwind a web OTA that was cut off mid-upload. Without this the watchdog
  // stays disabled, the web UI keeps showing frozen values that still look
  // plausible, and every later web OTA fails with "OTA could not begin" -
  // all silently, and precisely when an update is most needed.
  if (ota.isActive() && (millis() - otaLastActivity) > OTA_STALL_TIMEOUT) {
    ESP_LOGE(TAG, "OTA upload stalled - aborting and restoring normal operation");
    Update.abort();
    ota.setActive(false);
    if (!setupMode) {
      wdt.enable();
    }
    webUI.wsUpdateWebText("p00_ota_upd_err", "upload aborted (timeout)", false);
    webUI.wsUpdateWebDialog("ota_update_failed_dialog", "open");
  }

  // in simulation mode, load simdata and display simModeBar
  if (simulationTimer.delayOn(config.sim.enable && !simulationInit && !setupMode, 5000)) {
    simulationInit = true;
    webUI.wsShowElementClass("simModeBar", true);
    startSimData();
  }

  webInitDone = true; // init done
}
