#pragma once

#include "esp_sdcard.h"


namespace esp
{

class app
{

public:

    app();
    ~app();

    void dispatch();
    void exec();

private:

    void hello();

    sd_card _sd_card;
};

}
