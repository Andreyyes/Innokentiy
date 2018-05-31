#include "core.h"

//String serialBuffer = "";
register_t registerBuffer[REGISTER_COUNT];

void(*userSerialKeyValueCallback[SERIAL_CALLBACK_MAXCOUNT])(String, String);
void(*userSerialCommandCallback[SERIAL_CALLBACK_MAXCOUNT])(String);

String serialBuffer = "";

void coreSetup() {
  Serial1.begin(115200);
}

void coreLoop() {
  //delay(100);
  while (Serial1.available()) {
    char inChar = (char)Serial1.read();
    if (inChar == '\n') {
      handleSerialMsg(serialBuffer);
      serialBuffer = "";
    } else {
      serialBuffer += inChar;
    }
  }
  
//  printRegisters();/
}

//void printRegisters() {
//  for (int i = 0; i < REGISTER_COUNT; i++) {
//    if (registerBuffer[i].type != UNUSED) {
//      uint8_t hash = registerHash(registerBuffer[i]);
//      if (registerBuffer[i].hash != hash) {
//        for (int i = 0; i < serialsCount; i++) {
//          (*serials[i].serial).print("?register:");
//          (*serials[i].serial).print(registerBuffer[i].name);
//          (*serials[i].serial).print("=");
//          (*serials[i].serial).println(registerGetValue(&registerBuffer[i], true));
//        }
//        registerBuffer[i].hash = hash;
//      }
//    }
//  }
//}

void handleSerialMsg(String& msg) {
  if (msg.indexOf("?") != 0) {
    return;
  }

  if (msg.indexOf("=") != -1) {
    String key = msg.substring(1, msg.indexOf("="));
    String value = msg.substring(msg.indexOf("=") + 1);

    if (key.startsWith("register:")) {
      String regName = key.substring(key.indexOf(":")+1);
      register_t* reg = getRegister(regName);
      if (reg != NULL) {
        registerSetValue(reg, value);
        reg->hash = registerHash(*reg);
        Serial1.print(key);
        Serial1.print("=");
        Serial1.print(value);
        Serial1.println(" OK");
      } else {
        Serial1.println("Unknown register");
      }
    } else if (key == "newregister") {
      String regName = value.substring(0, value.indexOf(":"));
      value = value.substring(value.indexOf(":") + 1);
      String regType = value.substring(0, value.indexOf(":"));
      String regAccess = value.substring(value.indexOf(":") + 1);
      createRegister(regName, registerType(regType), registerAccess(regAccess));
      register_t* newReg = getRegister(regName);
      Serial1.print("Register created with name \"");
      Serial1.print(newReg->name);
      Serial1.print("\", type=");
      Serial1.print(registerType(newReg->type));
      Serial1.print(", access=");
      Serial1.println(registerAccess(newReg->access));
    }
    else {
      callHandleSerialKeyValue(key, value);
    }

  } else {
    String cmd = msg.substring(1);
    
    callHandleSerialCmd(cmd);
  }
}

void registerHandleSerialKeyValue(void(*callback)(String, String)) {
  for (int i = 0; i < SERIAL_CALLBACK_MAXCOUNT; i++) {
    if (userSerialKeyValueCallback[i] == NULL) {
      userSerialKeyValueCallback[i] = callback;
      return;
    }
  }
}

void registerHandleSerialCmd(void(*callback)(String)) {
  for (int i = 0; i < SERIAL_CALLBACK_MAXCOUNT; i++) {
    if (userSerialCommandCallback[i] == NULL) {
      userSerialCommandCallback[i] = callback;
      return;
    }
  }
}

void callHandleSerialKeyValue(String k, String v) {
  for (int i = 0; i < SERIAL_CALLBACK_MAXCOUNT; i++) {
    if (userSerialKeyValueCallback[i] != NULL) {
      userSerialKeyValueCallback[i](k, v);
      return; // прерывать выполнение других колл-беков
    }
  }
}

void callHandleSerialCmd(String cmd) {
  for (int i = 0; i < SERIAL_CALLBACK_MAXCOUNT; i++) {
    if (userSerialCommandCallback[i] != NULL) {
      userSerialCommandCallback[i](cmd);
      return; // прерывать выполнение других колл-беков
    }
  }
}

void createRegister(String name, registertype_t type, registeraccess_t access) {
  createRegister(name.c_str(), type, access);
}

void createRegister(const char name[REGISTER_NAME_LENGTH], registertype_t type, registeraccess_t access) {
  for (int i = 0; i < REGISTER_COUNT; i++) {
    if (registerBuffer[i].type == UNUSED) {
      memcpy(registerBuffer[i].name, name, REGISTER_NAME_LENGTH);
      registerBuffer[i].type = type;
      registerBuffer[i].access = access;
      return;
    }
  }
}

register_t * getRegister(String name) {
  return getRegister(name.c_str());
}

register_t * getRegister(const char name[REGISTER_NAME_LENGTH]) {
  for (int i = 0; i < REGISTER_COUNT; i++) {
    if (strcmp(registerBuffer[i].name, name) == 0) {
      return &registerBuffer[i];
    }
  }
  
  return NULL;
}

String registerGetValue(register_t* reg, boolean quoteString) {
  switch(reg->type) {
    case BOOL: return String(reg->boolValue);
    case BYTE: return String(reg->byteValue);
    case INT: return String(reg->intValue);
    case FLOAT: String(reg->floatValue);
    case STRING: 
      if (quoteString)
        return "\"" + reg->stringValue + "\"";
      else
        return reg->stringValue;
    default: if (quoteString) return "\"\""; else return "";
  }
}

void registerSetValue(register_t* reg, String valueStr) {
  switch(reg->type) {
    case BOOL:
      valueStr.toLowerCase();
      reg->boolValue = (valueStr == "yes" || valueStr == "on" || valueStr == "true" || valueStr == "1");
      return;
    case BYTE: reg->byteValue = (uint8_t)valueStr.toInt(); return;
    case INT: reg->intValue = valueStr.toInt(); return;
    case FLOAT: reg->floatValue = valueStr.toFloat(); return;
    case STRING: reg->stringValue = valueStr; return;
    default: 
      return;
  }
}

uint8_t registerHash(register_t reg) {
//  boolean boolValue;
//  uint8_t byteValue;
//  int32_t intValue;
//  float floatValue;
//  String stringValue;

  uint8_t * regMem = (uint8_t*)malloc(sizeof(boolean) + sizeof(uint8_t) + sizeof(int32_t) + sizeof(float));
  memcpy(regMem, &reg.boolValue, sizeof(boolean));
  memcpy(regMem + sizeof(boolean), &reg.byteValue, sizeof(uint8_t));
  memcpy(regMem + sizeof(boolean) + sizeof(uint8_t), &reg.intValue, sizeof(int32_t));
  memcpy(regMem + sizeof(boolean) + sizeof(uint8_t) + sizeof(int32_t), &reg.floatValue, sizeof(float));

  uint8_t hash = 0;
  uint8_t *regCur = regMem;
  for (int i = 0; i < sizeof(boolean) + sizeof(uint8_t) + sizeof(int32_t) + sizeof(float); i++) {
    hash ^= *regCur;
    regCur++;
  }

  for (int i = 0; i < reg.stringValue.length(); i++) {
    hash ^= reg.stringValue[i];
  }

  free(regMem);

  return hash;
}

registertype_t registerType(String regType) {
  if (regType == "bool") {
    return BOOL;
  } else if (regType == "byte") {
    return BYTE;
  } else if (regType == "int") {
    return INT;
  } else if (regType == "float") {
    return FLOAT;
  } else if (regType == "string") {
    return STRING;
  } else {
    return UNUSED;
  }
}

String registerType(registertype_t regType) {
  switch (regType) {
    case BOOL: return "bool";
    case BYTE: return "byte";
    case INT: return "int";
    case FLOAT: return "float";
    case STRING: return "string";
    default: return "unused";
  }
}

registeraccess_t registerAccess(String regAccess) {
  registertype_t access;
  if (regAccess == "rw" || regAccess == "readwrite" || regAccess == "read-write") {
    return READWRITE;
  } else {
    return READONLY;
  }
}

String registerAccess(registeraccess_t regAccess) {
  switch (regAccess) {
    case READWRITE: return "read-write";
    default: return "read-only";
  }
}


