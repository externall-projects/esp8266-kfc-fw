/**
* Author: sascha_lammers@gmx.de
*/

#include "FormCallbackValidator.h"
#include "FormField.h"
#include "FormBase.h"

FormCallbackValidator::FormCallbackValidator(Callback_t callback) : FormCallbackValidator(String(), callback) {
}

FormCallbackValidator::FormCallbackValidator(const String & message, Callback_t callback) : FormValidator(message) {
    _callback = callback;
}

bool FormCallbackValidator::validate() {
    return FormValidator::validate() && _callback(getField().getValue(), getField());
}
