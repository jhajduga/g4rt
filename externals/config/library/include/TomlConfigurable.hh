/**
*
* \author B.Rachwal (brachwal@agh.edu.pl)
* \date 31.03.2023
*
*/

#ifndef TOML_CONFIGURABLE
#define TOML_CONFIGURABLE

#include "TomlConfigModule.hh"
#include "Configurable.hh"

///
class TomlConfigurable: public TomlConfigModule, public Configurable {
    public:
        ///
        TomlConfigurable() = delete;
        
        ///
        TomlConfigurable(const std::string& name):TomlConfigModule(name),Configurable(name){};

        ///
        ~TomlConfigurable() = default;
};


#endif // TOML_CONFIGURABLE