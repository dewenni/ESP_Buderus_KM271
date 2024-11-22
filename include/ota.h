class OTAState {
private:
  bool otaState; // OTA-Zustand

  OTAState() : otaState(false) {}

public:
  static OTAState &getInstance() {
    static OTAState instance;
    return instance;
  }

  OTAState(const OTAState &) = delete;
  OTAState &operator=(const OTAState &) = delete;

  void setActive(bool active) { otaState = active; }

  bool isActive() const { return otaState; }
};
