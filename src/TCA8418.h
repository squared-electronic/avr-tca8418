#ifndef TCA8418_h
#define TCA8418_h

class TCA8418 {
 public:
  void begin();
  void setKeys();
  void handleInterupt();
};

#endif