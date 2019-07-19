// test_config.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Arduino_compat.h>
#include <Buffer.h>
#include <TableDumper.h>
#include <algorithm>
#include <map>
#include "Configuration.h"
#include "ConfigurationParameter.h"
#include "ConfigurationParameterReference.h"

char tmp[] = { 0x00, 0x00 };

EEPROMFile EEPROM;

typedef struct {
    uint8_t flag1 : 1;
    uint8_t flag2 : 1;
    uint8_t flag3 : 1;
} ConfigSub2_t;

typedef uint8_t blob_t[10];

// structure is used for code completion only
typedef struct {

    char string1[100];
    char blob[10];
    int32_t int32;
    uint8_t val1;
    uint16_t val2;
    uint32_t val3;
    uint64_t val4;
    uint32_t val5;
    IPAddress ip;
    struct {
        char string1[100];
        char string2[100];
    } sub1;
    ConfigSub2_t sub2;
    blob_t blob2;
    
} Configuration_t;

void test(Configuration &config) {
    auto str1 = ConfigurationParameterFactory::get(config, config.getWritableParameter<char *>(_H(Configuration_t().string1)));
    auto str2 = ConfigurationParameterFactory::get(config, config.getWritableParameter<char *>(_H(Configuration_t().sub1.string2), 10));
    auto str3 = ConfigurationParameterFactory::get(config, config.getParameter<char *>(_H(Configuration_t().sub1.string1)));

    printf("str3: %s\n", str3.get());

    auto str4 = ConfigurationParameterFactory::get(config, config.getWritableParameter<char *>(_H(Configuration_t().sub1.string1), 10));
    str4.set("update");

    auto str5 = ConfigurationParameterFactory::get(config, config.getParameter<char *>(_H(Configuration_t().sub1.string1)));
    printf("str5: %s\n", str5.get());

    str4.set("update2");
    printf("str5: %s\n", str5.get());

    auto str6 = ConfigurationParameterFactory::get(config, config.getWritableParameter<void *>(_H(Configuration_t().blob), sizeof(blob_t)));

    str6.set("blob_t\x1\x2\x3", 9);

    auto sub2 = ConfigurationParameterFactory::get(config, config.getWritableParameter<ConfigSub2_t>(_H(Configuration_t().sub2)));

    auto &sub2Ref = sub2.getWriteable();
    
    sub2Ref.flag1 = 1;
    sub2Ref.flag2 = 0;
    sub2Ref.flag3 = 1;

    printf("sub2 %d %d %d\n", sub2Ref.flag1, sub2Ref.flag2, sub2Ref.flag3);

    //printf("%d\n%d\n", str4.getLength(), sub2.getLength());

    //str2.set("test");

    if (config.isDirty()) {
        config.write();
    }

    printf("%s\n", str1.get());

    printf("%s\n%s\n%s\n", (const char *)str1, (const char *)str2, str3.get());
}

int main() {

    ESP._enableMSVCMemdebug();

#if 0
    memcpy(EEPROM.getDataPtr(), tmp, sizeof(tmp));
    EEPROM.commit();
    Configuration config2(28, 4096);
    if (!config2.read()) {
        printf("Failed to read configuration\n");
    }
    config2.dump(Serial);
    return 0;
#endif

#if 1
    EEPROM.clear();
#endif
    EEPROM.begin();

    Configuration config(0, 256);

    if (!config.read()) {
        printf("Failed to read configuration\n");
    }

    config.dump(Serial);

    auto ptr = config._H_STR(Configuration_t().string1);
    printf("%s\n", ptr);
    if (config.exists<ConfigSub2_t>(_H(Configuration_t().sub2))) {
        auto data = config.get<uint8_t>(_H(Configuration_t().sub2));
        printf("%d\n", data);
    }
     
    config.release();

    /*test(config);
    config.dump(Serial);
*/
    config.setString(_H(Configuration_t().string1), "bla2");
    ConfigSub2_t s = { 1, 0 , 1 };
    config.set(_H(Configuration_t().sub2), s);

    config.setBinary(_H(Configuration_t().blob), "blob\x1\x2\x3", 7);

    char *writeableStr = config.getWriteableString(_H(Configuration_t().sub1.string1), sizeof Configuration_t().sub1.string1);
    strcpy(writeableStr, "dummy");

    auto &int32 = config.getWriteable<int32_t>(_H(Configuration_t().int32));
    int32 = -12345678;

    // _H_W_GET_IP is uint32_t
    auto &ip = config._H_W_GET_IP(Configuration_t().ip);
    ip = (uint32_t)IPAddress(1, 2, 3, 4);

    auto test = config._H_GET(Configuration_t().int32);
    printf("%d\n", test);

    // type mismatch and will fail if IPAddress isn't the same type as _H_W_GET_IP, but in this case it is both uint32_t
    IPAddress ip2 = config._H_GET_IP(Configuration_t().ip);
    printf("%s\n", ip2.toString().c_str());

    config._H_SET(Configuration_t().val1, 0x80);
    config._H_SET(Configuration_t().val2, 0x8070);
    config._H_SET(Configuration_t().val3, 0x80706050);
    config._H_SET(Configuration_t().val4, 0x8070605040302010);

    config.dump(Serial);

    if (config.isDirty()) {
        config.write();
    }

    config.dump(Serial);

    config.release();

    config.dump(Serial);

    return 0;
}
