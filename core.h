#ifndef CORE_H
#define CORE_H

//#include <ESP8266WiFi.h>
#include <Arduino.h>

#define NULL nullptr
#define REGISTER_NAME_LENGTH 32
#define REGISTER_COUNT 8

typedef enum { UNUSED = 0, BOOL = 1, BYTE = 2, INT = 3, FLOAT = 4, STRING = 5 } registertype_t;
typedef enum { READONLY = 0, READWRITE = 1 } registeraccess_t;

typedef struct {
  char name[REGISTER_NAME_LENGTH];
  registertype_t type;
  registeraccess_t access;
  
  boolean boolValue;
  uint8_t byteValue;
  int32_t intValue;
  float floatValue;
  String stringValue;

  uint8_t hash;
} register_t;

#define SERIAL_CALLBACK_MAXCOUNT 10

//namespace std {
//  template<class T> class function; // undefined
//  template<class Ret, class... Args> class function<Ret(Args...)>;
//  // Null pointer comparisons:
//  template <class Ret, class... Args>
//  bool operator==(const function<Ret(Args...)>&, nullptr_t) noexcept;
//  template <class Ret, class... Args>
//  bool operator==(nullptr_t, const function<Ret(Args...)>&) noexcept;
//  template <class Ret, class... Args>
//  bool operator!=(const function<Ret(Args...)>&, nullptr_t) noexcept;
//  template <class Ret, class... Args>
//  bool operator!=(nullptr_t, const function<Ret(Args...)>&) noexcept;
//}

//typedef std::function<void(String, String)> SERIAL_CALLBACK_KV;
//typedef std::function<void(String)> SERIAL_CALLBACK_CMD;

void coreSetup();
void coreLoop();
void addSerial(HardwareSerial* serial);
void printRegisters();
void handleSerialMsg(String& msg);
void registerHandleSerialKeyValue(void(*callback)(String, String));
void registerHandleSerialCmd(void(*callback)(String));
void callHandleSerialKeyValue(String k, String v);
void callHandleSerialCmd(String cmd);
void createRegister(String name, registertype_t type, registeraccess_t access);
void createRegister(const char name[REGISTER_NAME_LENGTH], registertype_t type, registeraccess_t access);
register_t * getRegister(String name);
register_t * getRegister(const char name[REGISTER_NAME_LENGTH]);
String registerGetValue(register_t* reg, boolean quoteString);
void registerSetValue(register_t* reg, String value);
uint8_t registerHash(register_t reg);
registertype_t registerType(String regType);
String registerType(registertype_t regType);
registeraccess_t registerAccess(String regAccess);
String registerAccess(registeraccess_t regAccess);

#endif


