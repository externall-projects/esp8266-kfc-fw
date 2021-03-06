// httpheaders_test.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <PrintString.h>
#include <PrintHtmlEntitiesString.h>
#include "HttpHeaders.h"

void test(PrintHtmlEntities& o) {
    o.print("<br>");
}

int main()
{
    ESP._enableMSVCMemdebug();

    PrintHtmlEntitiesString str;
    test(str);
    str.setRawOutput(true);
    test(str);


    HttpHeaders httpHeaders(false);
    httpHeaders.addNoCache(true);
    httpHeaders.addNoCache(true);
    httpHeaders.add(new HttpSimpleHeader(F("Content-Disposition"), PrintString(F("attachment; filename=\"kfcfw_config_%s_%s.json\""), "yyy", "xxx")));
    httpHeaders.add(new HttpDispositionHeader("yyyyy.txt"));
    auto cookie = new HttpCookieHeader("test");
    cookie->setValue("val");
    httpHeaders.add(cookie);
    auto cookie2 = new HttpCookieHeader("test");
    *cookie2 = *cookie;
    cookie2->setExpires(12345);
    httpHeaders.add(cookie2);
    httpHeaders.dump(Serial);
}
