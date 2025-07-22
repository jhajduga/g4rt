# Basic TOML job structure

The job definition is being wrapped into TOML-like objects in relation to the building blocks of the G4RT application, e.g. the `RunSvc`. To understand the job toml-macro it worth to link to the implemented mechanisms within the `ConfigSvc` library.

## TOML and the ConfigSvc
Within the `ConfigSvc` library, once any class inherits from the `Configurable` base class we can utilize the power of this service - see [Github-ConfigSvc-Configurable.hh](https://github.com/barachwal/ConfigSvc/blob/master/library/include/Configurable.hh).

In addition, once the class within the application which inherits from the `TomlConfigurable` or `TomlConfigModule` base classes can be managed trough the TOML interface. 

The `TomlConfigModule` gives functionality of the `ParseTomlConfig()` which is customized parsing of the TOML-like information within a given context of `this` object, once the `TomlConfigurable` enables to utilize both the `Configurable` and the `TomlConfigModule`. Hence, we can define the global and contextual configuration.

## Global vs contextual configuration
The `global` configuration corresponds to the interface inherited from the `TomlConfigModule`. In the case of the `G4RT` application it's being dedicated to the  `RunSvc` and any patient definition (inheriting from the `VPatient`) `WaterPhantom`, `Dose3D`. In this case, once we would like to vary any attribute being exposed in these classes we simply refer to the class of interest and the attribute, e.g.:  
```
[RunSvc]
JobName = "Single file tompl run test"
BeamType = "IAEA"
```  
Note: the type of values matters!

As mentioned earlier, once the class of interest inherits from the `TomlConfigurable` or `TomlConfigModule` we can refer to it trough the so called contextual configuration. The reference to such an object is defiend with the prefix of (typically this is the class name), e.g.:
```
[WaterPhantom_Detector]
Voxelization = [1,1,1]         # Number of cels in every direction
Size = [400.0,400.0,400.0]     # In mm
Medium = "G4_WATER"

[WaterPhantom_Scoring]
FullVolume = false
FarmerDoseCalibration = true
```
Note: Each contextual group of attributes (in the example above `_Detector`, `_Scoring`) is being defined in the `ParseTomlConfig()` function with is being overrided in the final class of itnerest.