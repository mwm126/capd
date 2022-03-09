// Parse the config file /etc/capd/capd.toml

#include <iostream>
#include <toml++/toml.h>

extern "C"
{
#include "parse_config.h"
}

config_t init_config(char *toml_fname)
{
    config_t cfg = {"1.1.1.3"};
    toml::table tbl;
    try
    {
        tbl = toml::parse_file(toml_fname);
        strcpy(cfg.address, tbl["address"].value_or(""));
    }
    catch (const toml::parse_error &err)
    {
        std::cerr << "Parsing failed:\n" << err << "\n";
    }
    return cfg;
}
