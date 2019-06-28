/**
* Author: sascha_lammers@gmx.de
*/

#pragma once

#include <Arduino_compat.h>
#include <functional>
#include <vector>
#include "FormField.h"
#include "FormValue.h"
#include "FormObject.h"
#include "FormBitValue.h"
#include "FormString.h"

class FormError;
class FormData;

class Form {
public:
    Form();
    Form(FormData *data);
    virtual ~Form();

    void clearForm();
    void setFormData(FormData *data);
    void setInvalidMissing(bool invalidMissing);

    int add(FormField *field);
    FormField *_add(FormField *field);
    FormField *add(const String &name, const String &value, FormField::FieldType_t type = FormField::INPUT_TEXT);

    template <typename T>
    FormField *add(const String &name, T value, std::function<void(T, FormField *)> setter = nullptr, FormField::FieldType_t type = FormField::INPUT_SELECT) {
        return _add(new FormValue<T>(name, value, setter, type));
    }

    template <typename T, size_t N>
    FormField *add(const String &name, T *value, std::array<T, N> bitmask, FormField::FieldType_t type = FormField::INPUT_SELECT) {
        return _add(new FormBitValue<T, N>(name, value, bitmask, type));
    }

    template <typename T>
    FormField *add(const String &name, T *value, FormField::FieldType_t type = FormField::INPUT_SELECT) {
        return _add(new FormValue<T>(name, value, type));
    }

    template <size_t size>
    FormField *add(const String &name, char *value, FormField::FieldType_t type = FormField::INPUT_TEXT) {
        return _add(new FormString<size>(name, value, type));
    }

    template <class C, bool O>
    FormField *add(const String &name, C *obj, FormField::FieldType_t type = FormField::INPUT_TEXT) {
        return _add(new FormObject<C>(name, obj, type));
    }

    FormValidator *addValidator(int index, FormValidator *validator);
    FormValidator *addValidator(FormValidator *validator);
    FormValidator *addValidator(const String &name, FormValidator *validator);

    FormField *getField(const String &name) const;
    FormField *getField(int index) const;
    const size_t hasFields() const;

    void clearErrors();
    bool validate();
    bool validateOnly();
    const bool isValid() const;
    const bool hasChanged() const;
    const bool hasError(FormField *field) const;
    void copyValidatedData();
    const std::vector<FormError> &getErrors() const;
    void finalize() const;

    const char *process(const String &name) const;
    void createJavascript(Print &out);

    void dump(Print &out, const String &prefix) const;

private:
    FormData *_data;
    std::vector<FormField *> _fields;
    std::vector<FormError> _errors;
    bool _invalidMissing;
    bool _hasChanged;
};
