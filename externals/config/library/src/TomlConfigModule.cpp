#include "TomlConfigModule.hh"
#include "ConfigSvc.hh"
#include <iostream>

////////////////////////////////////////////////////////////////////////////////
///
void TomlConfigModule::SetTomlConfigFile(const std::string& file) {
    if(!file.empty())
        m_config_file = file;
    else{   // It's assumed that the config file is the same as the
            // one registered in ConfigSvc
        m_config_file = ConfigSvc::GetInstance()->GetTomlFile();
    }
}

////////////////////////////////////////////////////////////////////////////////
///
bool TomlConfigModule::IsTomlConfigExists() const {
    if(m_config_prefix.empty() )
        return false;
    if(m_config_prefix=="None")
        return false;
    return true;
}

////////////////////////////////////////////////////////////////////////////////
///
toml::parse_result TomlConfigModule::ParseTomlFile() const {
    auto configFile = GetTomlConfigFile();
    std::cout << "[INFO]:: TomlConfigModule::Parsing file::\n."<< configFile << "\n" << std::endl;

    auto configPrefix = GetTomlConfigPrefix();
    if(configPrefix.empty()){ // here it's assummed that the config data is given with prefixed name
        std::cout << "[ERROR]:: TomlConfigModule:: You are trying to using TomlConfigModule, but the Prefix is not defined!"<< std::endl;
        std::exit(EXIT_FAILURE);
    }
    return toml::parse_file(configFile);
}
