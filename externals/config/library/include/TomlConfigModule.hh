/**
*
* \author B.Rachwal (brachwal@agh.edu.pl)
* \date 31.03.2023
*
*/

#ifndef TOML_CONFIG_MODULE
#define TOML_CONFIG_MODULE

#include <string>
#include "toml.hh"

// TOML-like specific configuration that can be defined for any object.
// Parsing such configuration is delegated to final inteface of given object.
// See TOML docs: https://learnxinyminutes.com/docs/toml/ what you can use.
// However, the main idea is that the given ConfigModule object is supposed to have 
// Units defined in order to get reference, YourConfigurableObejct::Configure()
// DefineUnit<std::string>("ConfigFile"); // the name can be customized
// DefineUnit<std::string>("ConfigPrefix"); // the name can be customized
// Once the "ConfigFile" is left as "None" the "ConfigPrefix" configuration 
// is being searched in the main TOML that is registered trough the ConfigSvc
class TomlConfigModule {
    private:
        ///
        std::string m_config_file;

        ///
        std::string m_config_prefix;

        ///
        bool m_config_flag = false;

    public:
        ///
        explicit TomlConfigModule(const std::string& config_prefix):m_config_prefix(config_prefix){}

        ///
        ~TomlConfigModule() = default;

        ///
        void SetTomlConfigFile(const std::string& file=std::string());

        ///
        std::string GetTomlConfigFile() const { return m_config_file; }

        ///
        std::string GetTomlConfigPrefix() const { return m_config_prefix; }

        ///
        virtual void ParseTomlConfig() = 0;

        ///
        toml::parse_result ParseTomlFile() const;

        ///
        bool IsTomlConfigExists() const;

        ///
        void TomlConfig(bool flag) {m_config_flag = flag; }

        //
        bool TomlConfig() const { return m_config_flag; }
};

#endif // TOML_CONFIG_MODULE