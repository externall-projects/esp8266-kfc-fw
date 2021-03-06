
#include <Arduino_compat.h>
#include <array>
#include <assert.h>
#include <misc.h>
#include <PrintString.h>
#include "KFCForms.h"
#include "BootstrapMenu.h"
#include "EnumBase.h"
#include "EnumBitset.h"

typedef enum WiFiMode 
{
    WIFI_OFF = 0, WIFI_STA = 1, WIFI_AP = 2, WIFI_AP_STA = 3
} WiFiMode_t;

enum wl_enc_type { 
    ENC_TYPE_WEP  = 5,
    ENC_TYPE_TKIP = 2,
    ENC_TYPE_CCMP = 4,
    ENC_TYPE_NONE = 7,
    ENC_TYPE_AUTO = 8
};

typedef uint32_t ConfigFlags_t;

#define FLAGS_SYSLOG_UDP            0x00000200
#define FLAGS_SYSLOG_TCP            0x00000400
#define FLAGS_SYSLOG_TCP_TLS        (FLAGS_SYSLOG_UDP|FLAGS_SYSLOG_TCP)
#define FLAGS_MQTT_ENABLED          0x00000040
#define FLAGS_SECURE_MQTT           0x00008000

class TestForm : public Form {
public:
    TestForm(FormData *data) : Form(data) {
    }

    void createForm() {

        add<float>(F("a_float"), &a_float);

        add<WiFiMode_t>(F("mode"), wifi_mode, [this](WiFiMode_t value, FormField &, bool) {
            wifi_mode = value;
            return false;
        });
        addValidator(new FormRangeValidator(F("Invalid mode"), WIFI_OFF, WIFI_AP_STA));

        // string with length validation
        add<sizeof wifi_ssid>(F("wifi_ssid"), wifi_ssid);
        addValidator(new FormLengthValidator(1, sizeof(wifi_ssid) - 1));

        // select input with range
        add<uint8_t>(F("channel"), &soft_ap_channel);
        addValidator(new FormRangeValidator(1, 13));

        // select input with enumeration validator
        add<uint8_t>(F("encryption"), &encryption, [](uint8_t &value, FormField &field, bool isSetter) {
            //value *= 2;
            return false;
        });
        std::array<uint8_t, 5> my_array2{ENC_TYPE_NONE, ENC_TYPE_TKIP, ENC_TYPE_WEP, ENC_TYPE_CCMP, ENC_TYPE_AUTO};
        addValidator(new FormEnumValidator<uint8_t, 5>(F("Invalid encryption"), my_array2));

        // check box input with callback to assign/modify/validate the new data
        add<bool>(F("ap_hidden"), is_hidden_ssid, [this](bool value, FormField &, bool) {
            is_hidden_ssid = value ? 123 : 0;
            return false;
        }, FormField::INPUT_CHECK)->setFormUI(
            (new FormUI(FormUI::TypeEnum_t::SELECT, F("SSID")))->setBoolItems(F("Show SSID"), F("Hide SSID"))
        );

        // select input
        // bits in the array are addressed "0" to "sizeof(array) - 1" inside the form
        // 0 = FormBitValue_UNSET_ALL (none set), 1 = FLAGS_SYSLOG_UDP, 2 = FLAGS_SYSLOG_TCP, etc...
        std::array<ConfigFlags_t, 4> my_array3{ FormBitValue_UNSET_ALL, FLAGS_SYSLOG_UDP, FLAGS_SYSLOG_TCP, FLAGS_SYSLOG_TCP_TLS };
        add<ConfigFlags_t, 4>(F("syslog_enabled"), &flags, my_array3);

        // also a bit set, using the same variable. bits in the array must not overlap
        std::array<ConfigFlags_t, 3> my_array4{ FormBitValue_UNSET_ALL, FLAGS_MQTT_ENABLED, FLAGS_MQTT_ENABLED|FLAGS_SECURE_MQTT };
        add<ConfigFlags_t, 3>(F("mqtt_enabled"), &flags, my_array4);

        // input text
        add<sizeof mqtt_host>(F("mqtt_host"), mqtt_host);
        addValidator(new FormValidHostOrIpValidator());

        add(new FormObject<IPAddress&>(F("dns1"), dns1, FormField::INPUT_TEXT));
        add(new FormObject<IPAddress>(F("dns2"), dns2, [this](const IPAddress &addr, FormField &) {
            dns2 = addr;
        }));

        add<uint16_t>(F("mqtt_port"), &mqtt_port);
        addValidator(new FormTCallbackValidator<uint16_t>([](uint16_t value, FormField &field) {
            if (value == 0) {
                value = 1883;
                field.setValue(String(value));
            }
            return true;
        }));
        addValidator(new FormRangeValidator(F("Invalid port"), 1, 65535));

        finalize();

        PrintString str;
        setFormUI(F("Form Title"), F("Save Changes"));
        createHtml(str);
    }

    uint32_t flags;
    WiFiMode_t wifi_mode;
    uint8_t soft_ap_channel;
    uint8_t encryption;
    uint8_t is_hidden_ssid;
    char wifi_ssid[32];
    char mqtt_host[128];
    IPAddress dns1;
    IPAddress dns2;
    uint16_t mqtt_port;
    float a_float;
};



struct DimmerModule {
    float fade_time;
    float on_fade_time;
    float linear_correction;
    uint8_t max_temperature;
    uint8_t metrics_int;
    uint8_t report_temp;
    uint8_t restore_level;
};

struct DimmerModuleButtons {
    uint16_t shortpress_time;
    uint16_t longpress_time;
    uint16_t repeat_time;
    uint16_t shortpress_no_repeat_time;
    uint8_t min_brightness;
    uint8_t shortpress_step;
    uint8_t longpress_max_brightness;
    uint8_t longpress_min_brightness;
    float shortpress_fadetime;
    float longpress_fadetime;
};

struct DimmerModule dimmer_cfg;
struct DimmerModuleButtons dimmer_buttons_cfg;

class SettingsForm : public Form {
public:
    typedef std::vector<std::pair<String, String>> TokenVector;

    SettingsForm() : Form(new FormData()) 
    {
        auto *dimmer = &dimmer_cfg;

        // form.setFormUI(F("Dimmer Configuration"));
        // auto seconds = String(F("seconds"));

        add<float>(F("fade_time"), &dimmer->fade_time); //->setFormUI((new FormUI(FormUI::TEXT, F("Fade in/out time")))->setPlaceholder(String(5.0, 1))->setSuffix(seconds));

        add<float>(F("on_fade_time"), &dimmer->on_fade_time); //->setFormUI((new FormUI(FormUI::TEXT, F("Turn on/off fade time")))->setPlaceholder(String(7.5, 1))->setSuffix(seconds));

        add<float>(F("linear_correction"), &dimmer->linear_correction); //->setFormUI((new FormUI(FormUI::TEXT, F("Linear correction factor")))->setPlaceholder(String(1.0, 1)));

        add<uint8_t>(F("restore_level"), &dimmer->restore_level); //->setFormUI((new FormUI(FormUI::SELECT, F("On power failure")))->setBoolItems(F("Restore last brightness level"), F("Do not turn on")));

        add<uint8_t>(F("max_temperature"), &dimmer->max_temperature); //->setFormUI((new FormUI(FormUI::TEXT, F("Max. temperature")))->setPlaceholder(String(75))->setSuffix(F("&deg;C")));
        addValidator(new FormRangeValidator(F("Temperature out of range: %min%-%max%"), 45, 110));

        add<uint8_t>(F("metrics_int"), &dimmer->metrics_int); //->setFormUI((new FormUI(FormUI::TEXT, F("Metrics report interval")))->setPlaceholder(String(30))->setSuffix(seconds));
        addValidator(new FormRangeValidator(F("Invalid interval: %min%-%max%"), 10, 255));

#if 1

        auto *dimmer_buttons = &dimmer_buttons_cfg;
        //auto milliseconds = String(F("milliseconds"));
        //auto percent = String(F("&#37;"));

        add<uint16_t>(F("shortpress_time"), &dimmer_buttons->shortpress_time); //->setFormUI((new FormUI(FormUI::TEXT, F("Short press time")))->setPlaceholder(String(250))->setSuffix(milliseconds));
        addValidator(new FormRangeValidator(F("Invalid time"), 50, 1000));

        add<uint16_t>(F("longpress_time"), &dimmer_buttons->longpress_time); //->setFormUI((new FormUI(FormUI::TEXT, F("Long press time")))->setPlaceholder(String(600))->setSuffix(milliseconds));
        addValidator(new FormRangeValidator(F("Invalid time"), 250, 2000));

        add<uint16_t>(F("repeat_time"), &dimmer_buttons->repeat_time); //->setFormUI((new FormUI(FormUI::TEXT, F("Hold/repeat time")))->setPlaceholder(String(150))->setSuffix(milliseconds));
        addValidator(new FormRangeValidator(F("Invalid time"), 50, 500));

        add<uint8_t>(F("shortpress_step"), &dimmer_buttons->shortpress_step); //->setFormUI((new FormUI(FormUI::TEXT, F("Brightness steps")))->setPlaceholder(String(5))->setSuffix(percent));
        addValidator(new FormRangeValidator(F("Invalid level"), 1, 100));

        add<uint16_t>(F("shortpress_no_repeat_time"), &dimmer_buttons->shortpress_no_repeat_time); //->setFormUI((new FormUI(FormUI::TEXT, F("Short press down = off/no repeat time")))->setPlaceholder(String(800))->setSuffix(milliseconds));
        addValidator(new FormRangeValidator(F("Invalid time"), 250, 2500));

        add<uint8_t>(F("min_brightness"), &dimmer_buttons->min_brightness); //->setFormUI((new FormUI(FormUI::TEXT, F("Min. brightness")))->setPlaceholder(String(15))->setSuffix(percent));
        addValidator(new FormRangeValidator(F("Invalid brightness"), 0, 100));

        add<uint8_t>(F("longpress_max_brightness"), &dimmer_buttons->longpress_max_brightness); //->setFormUI((new FormUI(FormUI::TEXT, F("Long press up/max. brightness")))->setPlaceholder(String(100))->setSuffix(percent));
        addValidator(new FormRangeValidator(F("Invalid brightness"), 0, 100));

        add<uint8_t>(F("longpress_min_brightness"), &dimmer_buttons->longpress_min_brightness); //->setFormUI((new FormUI(FormUI::TEXT, F("Long press down/min. brightness")))->setPlaceholder(String(33))->setSuffix(percent));
        addValidator(new FormRangeValidator(F("Invalid brightness"), 0, 100));

        add<float>(F("shortpress_fadetime"), &dimmer_buttons->shortpress_fadetime); //->setFormUI((new FormUI(FormUI::TEXT, F("Short press fade time")))->setPlaceholder(String(1.0, 1))->setSuffix(seconds));

        add<float>(F("longpress_fadetime"), &dimmer_buttons->longpress_fadetime); //->setFormUI((new FormUI(FormUI::TEXT, F("Long press fade time")))->setPlaceholder(String(5.0, 1))->setSuffix(seconds));

#endif

        finalize();
    }

    ~SettingsForm() {
        delete getFormData();
    }
};

#include <FixedCircularBuffer.h>

typedef FixedCircularBuffer<int, 5> CBuf;
typedef FixedCircularBuffer<short, 3> CBuf2;
CBuf* aaaa = new CBuf();
CBuf &b=*aaaa;

template<class T, size_t S>
void dis_b(FixedCircularBuffer<T,S> &b) {
    //auto iterator = b.begin();
    //while (iterator != b.end()) {
    //    Serial.printf("%u ", *iterator);
    //    ++iterator;
    //}
    //Serial.println();
    
    //Serial.printf("%u: %d %d %d - ", 
    //    b.getCount(), CBuf::distance(b.begin(), b.end() - 1),
    //    b.front(), b.back()
    //);

    auto iterator = b.begin();
    Serial.print("> ");
    while (iterator != b.end()) {
        Serial.printf("%u ", *iterator);
        ++iterator;
    }
    Serial.println();
    Serial.print("<");
    iterator = b.end();
    if (iterator != b.begin()) {
        do {
            --iterator;
            Serial.printf("%u ", *iterator);
        } while (iterator != b.begin());
    }
    Serial.println();

}

int n = 1;

void add(CBuf& b) {
    Serial.printf("add %u: ", n);
    b.push_back(n++);
    Serial.printf("distance %d\n", b.distance(b.begin(), b.end()));
}

#include <PrintString.h>

//class EnumTest3 : public BitsetEnum<>

typedef_enum_bitset(TestEnumClass, uint8_t,
    A = 1,
    B = 2,
    C = 4,
    D,
    E,
    F,
);

define_enum_bitset(TestEnumClass);

typedef_enum(TestEnumClass2, int32_t,
    AA = 6,
    BB = 7,
    CC = 8,
    DD,
    EE,
    FF,
    );


define_enum(TestEnumClass2);

typedef_enum_bitset(Mpr121TouchpadEventType, uint8_t,
    TOUCH = 0x0001,
    RELEASED = 0x0002,
    MOVE = 0x0004,
    TAP = 0x0008,
    PRESS = 0x0010,
    HOLD = 0x0020,
    SWIPE = 0x0040,
    DRAG = 0x0080
);

define_enum_bitset(Mpr121TouchpadEventType);

int main()
{
    ESP._enableMSVCMemdebug();

    Mpr121TouchpadEventType type;

    type = Mpr121TouchpadEventType::BIT::TOUCH;
    type.dump(Serial);


    Serial.println(type.toString());




    return 0;

    typedef TestEnumClass ECT;
    typedef TestEnumClass2 E2;
    ECT ec;
    ec.dump(Serial);

    E2 e2(E2::ENUM::AA);

    e2 = E2::ENUM::AA;
    e2.dump(Serial);

    auto z = ECT::BIT::A;


    ec.dump(Serial);
    ec += z;
    ec.dump(Serial);
    ec += ECT::BIT::A;

    ec = ECT::ANY;
    ec.dump(Serial);

    ec += ECT::BIT::A;
    ec -= ec;
    ec += ECT::BIT::B;
    ec = ECT::NONE;
    ec += ECT::BIT::B;
    ec += ECT::BIT::A;

    ec = ECT::BIT::A | ECT::BIT::B | ECT::BIT::C;

    ec.dump(Serial);

    ec = ECT::BIT::A;
    if (ec == ECT::BIT::A) {
        Serial.println("&A only");
    }
    if (ec & ECT::BIT::A) {
        Serial.println("&A");
    }
    if (ec & ECT::BIT::B) {
        Serial.println("&B");
    }
    if (ec & ECT::BIT::C) {
        Serial.println("&C");
    }


    //Serial.println(ECT::toString(ECT::ENUM::A));
    Serial.println(ec.toString());


    return 0;

 /*   PrintString s;
    double start = 0.00015;
    double tmp = start;
    s = String(); s.print(tmp, 3, true); Serial.println(s); tmp *= 10;
    s = String(); s.print(tmp, 3, true); Serial.println(s); tmp *= 10;
    s = String(); s.print(tmp, 3, true); Serial.println(s); tmp *= 10;
    s = String(); s.print(tmp, 3, true); Serial.println(s); tmp *= 10;
    s = String(); s.print(tmp, 3, true); Serial.println(s); tmp *= 10;
    s = String(); s.print(tmp, 3, true); Serial.println(s); tmp *= 10;
    s = String(); s.print(tmp, 3, true); Serial.println(s); tmp *= 10;
    tmp += 0.123456789;
    s = String(); s.print(tmp, 3, true); Serial.println(s); tmp *= 10;
    s = String(); s.print(tmp, 4, true); Serial.println(s); tmp *= 10;

    s = String(); s.print(tmp, 200, true); Serial.println(s); tmp *= 10;

    tmp = 5.5;
    s = String(); s.print(tmp, 0, true); Serial.println(s);
    s = String(); s.print(tmp, 1, true); Serial.println(s);
    s = String(); s.print(tmp, 2, true); Serial.println(s);

    tmp = 5.0;
    s = String(); s.print(tmp, 0, true); Serial.println(s);
    s = String(); s.print(tmp, 1, true); Serial.println(s);
    s = String(); s.print(tmp, 2, true); Serial.println(s);


    return 0;*/

    auto it = b.begin();

    //add(b);
    //add(b);
    //b.pop_front();
    //add(b);
    //add(b);
    //b.pop_front();
    //add(b);
    //add(b);
    //b.pop_front();
    //add(b);
    //add(b);
    //b.pop_front();
    //add(b);
    //add(b);
    //b.pop_front();

    //auto val = b.pop_front();
    //Serial.printf("front=%u end=%u\n", val, b.begin() == b.end());
    //val = b.pop_front();
    //Serial.printf("front=%u end=%u\n", val, b.begin() == b.end());

    for (int i = 0; i < 13; i++) {
        dis_b(b);
        add(b);
        Serial.printf("size %u\n", b.size());
    }

    CBuf2 test2;
    b.copy(test2, b.begin(), b.end());

    CBuf aa = b.slice(b.begin(), b.end());
    dis_b(aa);
    aa.shrink(aa.end() - 2, aa.end());
    aa.push_back(99);
    //aa.shrink(aa.end() - 2, aa.end() - 1);
    //aa.shrink(aa.begin(), aa.end());
    dis_b(aa);
    //dis_b(aa);
    return 0;

    Serial.println("b");
    dis_b(b);
    Serial.println("test2");
    dis_b(test2);
    return 0;

    while (b.begin() != b.end()) {
        Serial.printf("%u\n", b.pop_front());        
        Serial.printf("size %u\n", b.size());
    }

    //b.pop_front();
//    b.pop_front();
    if (b.size() >= 2) {


        CBuf a = b.slice(b.begin() + 1, b.end());
        b = std::move(b.slice(b.end() - 1, b.end()));

        dis_b(a);
        dis_b(b);

    }

    delete aaaa;

    return 0;

#if 1

    SettingsForm form;
    auto data = form.getFormData();

    data->set("max_temperature", "156");

    if (form.validate()) {

    }
    else {
        form.dump(Serial, "");
    }

#endif

#if 0

    BootstrapMenu bootstrapMenu;

    auto id = bootstrapMenu.addMenu(F("Device"));
    bootstrapMenu.addSubMenu(String("test"), String("test.html"), id);

    id = bootstrapMenu.addMenu(F("Home"));
    bootstrapMenu.getItem(id)->setURI(F("index.html"));

    bootstrapMenu.addSubMenu(F("Home"), F("index.html"), id);
    bootstrapMenu.addSubMenu(F("Status"), F("status.html"), id);
    bootstrapMenu.addSubMenu(F("Manage WiFi"), F("wifi.html"), id);
    bootstrapMenu.addSubMenu(F("Configure Network"), F("network.html"), id);
    bootstrapMenu.addSubMenu(F("Change Password"), F("password.html"), id);
    bootstrapMenu.addSubMenu(F("Reboot Device"), F("reboot.html"), id);
    bootstrapMenu.addSubMenu(F("About"), F("about.html"), id);

    bootstrapMenu.addSubMenu(F("WebUI"), F("webui.html"), id, bootstrapMenu.findMenuByLabel(F("Home"), id));

    id = bootstrapMenu.addMenu(F("Status"));
    bootstrapMenu.getItem(id)->setURI(F("status.html"));

    id = bootstrapMenu.addMenu(F("Configuration"));
    bootstrapMenu.addSubMenu(F("WiFi"), F("wifi.html"), id);
    bootstrapMenu.addSubMenu(F("Network"), F("network.html"), id);
    bootstrapMenu.addSubMenu(F("Remote Access"), F("remote.html"), id);

    id = bootstrapMenu.addMenu(F("Admin"));
    bootstrapMenu.addSubMenu(F("Change Password"), F("password.html"), id);
    bootstrapMenu.addSubMenu(F("Reboot Device"), F("reboot.html"), id);
    bootstrapMenu.addSubMenu(F("Restore Factory Settings"), F("factory.html"), id);
    bootstrapMenu.addSubMenu(F("Update Firmware"), F("update_fw.html"), id);


    bootstrapMenu.html(Serial);

    bootstrapMenu.htmlSubMenu(Serial, bootstrapMenu.findMenuByLabel(F("home")), 0);

#endif

#if 0
    FormData data;


    // user submitted data
    data.clear();
    data.set("mode", "1");
    data.set("wifi_ssid", "MYSSID");
    data.set("channel", "5");
    data.set("encryption", "8");
    data.set("ap_hidden", "1");
    data.set("syslog_enabled", "2"); // FLAGS_SYSLOG_TCP
    data.set("mqtt_enabled", "1");  // FLAGS_MQTT_ENABLED
    data.set("mqtt_host", "google.de");
    data.set("dns1", "8.8.8.8");
    data.set("dns2", "8.8.4.4");
    data.set("mqtt_port", "");
    data.set("a_float", "1.234");

    // creating form object with submitted data
    TestForm f1(&data);

    // assign values
    f1.wifi_mode = WIFI_OFF;
    f1.flags = FLAGS_SYSLOG_TCP_TLS|FLAGS_MQTT_ENABLED|FLAGS_SECURE_MQTT;
    f1.soft_ap_channel = 7;
    f1.encryption = 2;
    f1.is_hidden_ssid = true;
    strcpy(f1.wifi_ssid, "OLDSSID");
    strcpy(f1.mqtt_host, "google.com");
    f1.dns1 = IPAddress(4, 4, 4, 1);
    f1.dns2 = IPAddress(4, 3, 2, 1);
    f1.mqtt_port = 0;
    f1.a_float = 1.234f;

    // create form with assigned values
    f1.createForm(); 

    // verify the converted data of the form

    assert(String(f1.process("MODE")) == "0");

    assert(String(f1.process("CHANNEL")) == "7"); // testing value
    assert(String(f1.process("CHANNEL_6")) == ""); // testing unselected value
    assert(String(f1.process("CHANNEL_7")).indexOf("selected") != -1); // testing selected value
    assert(String(f1.process("CHANNEL_8")) == ""); // testing unselected value

    assert(String(f1.process("WIFI_SSID")) == "OLDSSID");
    assert(String(f1.process("ENCRYPTION")) == "2");

    assert(String(f1.process("AP_HIDDEN")).indexOf("checked") != -1);

    assert(String(f1.process("SYSLOG_ENABLED")) == "3");
    assert(String(f1.process("MQTT_ENABLED")) == "2");

    assert(String(f1.process("DNS1")) == "4.4.4.1");
    assert(String(f1.process("DNS2")) == "4.3.2.1");

    assert(String(f1.process("MQTT_PORT")) == "0");

    assert(String(f1.process("A_FLOAT")) == "1.234");

    // submit user data to form without errors using the validate method

    assert(f1.validate());
    assert(f1.isValid());
    assert(f1.flags == (FLAGS_SYSLOG_TCP|FLAGS_MQTT_ENABLED));
    assert(f1.soft_ap_channel == 5);
    assert(strcmp(f1.wifi_ssid, "MYSSID") == 0); 
    assert(f1.encryption == 8);
    assert(f1.is_hidden_ssid == 123);
    assert(f1.dns1.toString() == "8.8.8.8");
    assert(f1.dns2.toString() == "8.8.4.4");
    assert(f1.mqtt_port == 1883);
    assert(f1.a_float == 1.234f);

    // submit invalid data

    data.set("channel", "100");
    data.set("encryption", "101");
    data.set("wifi_ssid", "1234567890123456789012345678901234567890");
    assert(f1.validate() == false);
    assert(f1.isValid() == false);
    assert(f1.hasError(f1.getField("channel")));
    assert(f1.hasError(f1.getField("encryption")));
    assert(f1.hasError(f1.getField("wifi_ssid")));
    assert(f1.getErrors().size() == 3);

    // dump errors
    for (auto error : f1.getErrors()) {
        printf("%s = %s\n", error.getName().c_str(), error.getMessage().c_str());        
    }

    f1.clearErrors();
    assert(f1.getErrors().size() == 0);

#endif

    return 0;
}
