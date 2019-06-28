/**
* Author: sascha_lammers@gmx.de
*/

#pragma once

#include <Arduino_compat.h>
#include <vector>

class Form;
class FormValidator;

class FormField {
public:
    enum FieldType_t {
        INPUT_NONE = 0,
        INPUT_CHECK,
        INPUT_SELECT,
        INPUT_TEXT,
    };

    FormField(const String &name);
    FormField(const String &name, const String &value);
    FormField(const String &name, const String &value, FieldType_t type);
    // FormField(const String &name, const String &value, bool optional) : FormField(name, value) {
    //     _optional = optional;
    // }
    virtual ~FormField();

    void setForm(Form *form);
    Form *getForm() const;

    // void setOptional(bool optional) {
    //     _optional = optional;
    // }
    // const bool isOptional() const {
    //     return _optional;
    // }

    const String &getName() const;

    /**
    * Returns the value of the initialized field or changes the user submitted
    **/
    const String &getValue();

    /**
    * Initialize the value of the field. Should only be used in the constructor.
    **/
    void initValue(const String &value);

    /**
    * This method is called when the user submits a form
    **/
    virtual bool setValue(const String &value);

    /*
    * This method is called when the user submits a valid form. The validated data is stored
    * in the value field as a string and can be retried using getValue()
    **/
    virtual void copyValue();

    const bool equals(FormField *field) const;

    const bool hasChanged() const;
    void setChanged(bool hasChanged);

    void setType(FieldType_t type);
    const FieldType_t getType() const;

    void addValidator(FormValidator *validator);
    const std::vector<FormValidator *> &getValidators() const;

private:
    String _name;
    String _value;
    std::vector <FormValidator *> _validators;
    FieldType_t _type;
    Form *_form;
    bool _hasChanged;
    // bool _optional;
    // bool _notSet;
};
