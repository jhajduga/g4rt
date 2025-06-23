//
// Created by brachwal on 20.07.2018.
//

#include "MaterialsSvc.hh"
#include "Services.hh"
#include "Types.hh"
#include "G4NistManager.hh"

////////////////////////////////////////////////////////////////////////////////
///
MaterialsSvc::MaterialsSvc() : Configurable("MaterialsSvc") {
  Configure();
}

////////////////////////////////////////////////////////////////////////////////
///
MaterialsSvc::~MaterialsSvc() {
  configSvc()->Unregister(thisConfig()->GetName());
}

////////////////////////////////////////////////////////////////////////////////
///
MaterialsSvc *MaterialsSvc::GetInstance() {
  static MaterialsSvc instance;
  return &instance;
}

////////////////////////////////////////////////////////////////////////////////
///
void MaterialsSvc::Configure() {
  //G4cout << "[INFO]:: MaterialsSvc :: Service default configuration " << G4endl;

  // NIST MATERIALS
  DefineUnit<G4MaterialSPtr>("G4_Galactic");
  DefineUnit<G4MaterialSPtr>("G4_Cu");
  DefineUnit<G4MaterialSPtr>("G4_Zn");
  DefineUnit<G4MaterialSPtr>("G4_Al");
  DefineUnit<G4MaterialSPtr>("G4_W");
  DefineUnit<G4MaterialSPtr>("G4_Be");
  DefineUnit<G4MaterialSPtr>("G4_KAPTON");
  DefineUnit<G4MaterialSPtr>("G4_MYLAR");
  DefineUnit<G4MaterialSPtr>("G4_POLYACRYLONITRILE");
  DefineUnit<G4MaterialSPtr>("G4_Fe");
  DefineUnit<G4MaterialSPtr>("G4_WATER");

  // MODIFIED NIST MATERIALS
  DefineUnit<G4MaterialSPtr>("Usr_G4AIR20C"); // G4_AIR air at 20C

  // OTHER MATERIALS
  DefineUnit<G4MaterialSPtr>("steel1");
  DefineUnit<G4MaterialSPtr>("steel2");
  DefineUnit<G4MaterialSPtr>("steel3");
  DefineUnit<G4MaterialSPtr>("tungstenAlloy1");
  DefineUnit<G4MaterialSPtr>("PMMA");
  DefineUnit<G4MaterialSPtr>("PLA");
  DefineUnit<G4MaterialSPtr>("Z-FLEX");
  DefineUnit<G4MaterialSPtr>("RW3");

  DefineUnit<G4MaterialSPtr>("PMMA03");
  DefineUnit<G4MaterialSPtr>("PLA05");
  DefineUnit<G4MaterialSPtr>("Z-FLEX05");

  DefineUnit<G4MaterialSPtr>("PMMA075");
  DefineUnit<G4MaterialSPtr>("PLA075");
  DefineUnit<G4MaterialSPtr>("Z-FLEX075");


  DefineUnit<G4MaterialSPtr>("RMPS470");
  DefineUnit<G4MaterialSPtr>("EPS"); // Expanded Polystyrene
  DefineUnit<G4MaterialSPtr>("Rubber"); 
  
  DefineUnit<G4MaterialSPtr>("TiO2");

  DefineUnit<G4MaterialSPtr>("BaritesConcrete");



  Configurable::DefaultConfig();   // setup the default configuration for all defined units/parameters
  //Configurable::PrintConfig();
}

////////////////////////////////////////////////////////////////////////////////
///
void MaterialsSvc::DefaultConfig(const std::string &unit) {

  G4double d;
  auto G4NISTManager = G4NistManager::Instance();

  // Config volume name
  if (unit.compare("Label") == 0) thisConfig()->SetValue(unit, std::string("Materials Service"));

  // NIST MATERIALS / IMPORT FROM G4 DATABASE
  if (unit.find("G4_") != std::string::npos) {
    auto g4_material = G4NISTManager->FindOrBuildMaterial(unit);
    thisConfig()->SetValue(unit, std::shared_ptr<G4Material>(g4_material));
  }

  if (unit.compare("Usr_G4AIR20C") == 0) {
    d = 0.0012041 * g / cm3;  // as it is air at 20C
    auto usrAir = G4NISTManager->BuildMaterialWithNewDensity(unit, "G4_AIR", d,295.15);
    thisConfig()->SetValue(unit, std::shared_ptr<G4Material>(usrAir));
  }

  if (unit.compare("steel1") == 0) {
    d = 7.76 * g / cm3;
    const std::vector<G4String> elm{"Fe", "S", "Mn", "C"};
    const std::vector<G4double> weight{0.935, 0.01, 0.05, 0.005};
    auto steel1 = G4NISTManager->ConstructNewMaterial("steel1", elm, weight, d);
    thisConfig()->SetValue(unit, std::shared_ptr<G4Material>(steel1));
  }

  if (unit.compare("steel2") == 0) {
    d = 8.19 * g / cm3;
    const std::vector<G4String> elm{"Fe", "Ni", "Si", "Cr", "P"};
    const std::vector<G4double> weight{0.759, 0.11, 0.01, 0.12, 0.001};
    auto steel2 = G4NISTManager->ConstructNewMaterial("steel2", elm, weight, d);
    thisConfig()->SetValue(unit, std::shared_ptr<G4Material>(steel2));
  }

  if (unit.compare("steel3") == 0) {
    d = 8.19 * g / cm3;
    const std::vector<G4String> elm{"Fe", "Ni", "Si", "Cr", "Mn"};
    const std::vector<G4double> weight{0.69, 0.1, 0.01, 0.18, 0.02};
    auto steel3 = G4NISTManager->ConstructNewMaterial("steel3", elm, weight, d);
    thisConfig()->SetValue(unit, std::shared_ptr<G4Material>(steel3));
  }

  if (unit.compare("tungstenAlloy1") == 0) {
      d = 18.0 * g / cm3;
      const std::vector<G4String> elm{"W", "Ni", "Fe"};
      const std::vector<G4double> weight{0.95, 0.034, 0.016,};
      auto tungstenAlloy1 = G4NISTManager->ConstructNewMaterial("tungstenAlloy1", elm, weight, d);
      thisConfig()->SetValue(unit, std::shared_ptr<G4Material>(tungstenAlloy1));
  }

  if (unit.compare("PMMA") == 0) {
      d = 1.190 * g / cm3;
      const std::vector<G4String> elements{"C", "H", "O"};
      const std::vector<G4int> natoms{5,8,2};
      auto pmma = G4NISTManager->ConstructNewMaterial("PMMA", elements, natoms, d, true, kStateSolid, 295.15);
      thisConfig()->SetValue(unit, std::shared_ptr<G4Material>(pmma));
  }
  if (unit.compare("PMMA075") == 0) {
      d =0.75 * 1.190 * g / cm3;
      const std::vector<G4String> elements{"C", "H", "O"};
      const std::vector<G4int> natoms{5,8,2};
      auto pmma = G4NISTManager->ConstructNewMaterial("PMMA075", elements, natoms, d);
      thisConfig()->SetValue(unit, std::shared_ptr<G4Material>(pmma));
  }
  if (unit.compare("PMMA03") == 0) {
      d =0.3 * 1.190 * g / cm3;
      const std::vector<G4String> elements{"C", "H", "O"};
      const std::vector<G4int> natoms{5,8,2};
      auto pmma03 = G4NISTManager->ConstructNewMaterial("PMMA03", elements, natoms, d, true, kStateSolid, 295.15);
      thisConfig()->SetValue(unit, std::shared_ptr<G4Material>(pmma03));
  }


  if (unit.compare("PLA") == 0) {
      d = 1.24 * g / cm3;
      const std::vector<G4String> elements{"C", "H", "O"};
      const std::vector<G4int> natoms{3,4,2};
      auto pla = G4NISTManager->ConstructNewMaterial("PLA", elements, natoms, d);
      thisConfig()->SetValue(unit, std::shared_ptr<G4Material>(pla));
  }

  if (unit.compare("PLA075") == 0) {
      d =0.75 * 1.24 * g / cm3;
      const std::vector<G4String> elements{"C", "H", "O"};
      const std::vector<G4int> natoms{3,4,2};
      auto pla = G4NISTManager->ConstructNewMaterial("PLA075", elements, natoms, d);
      thisConfig()->SetValue(unit, std::shared_ptr<G4Material>(pla));
  }

  if (unit.compare("PLA05") == 0) {
      d =0.5 * 1.24 * g / cm3;
      const std::vector<G4String> elements{"C", "H", "O"};
      const std::vector<G4int> natoms{3,4,2};
      auto pla = G4NISTManager->ConstructNewMaterial("PLA05", elements, natoms, d);
      thisConfig()->SetValue(unit, std::shared_ptr<G4Material>(pla));
  }

  if (unit.compare("Z-FLEX") == 0) {
      d = 0.815 * g / cm3;
      const std::vector<G4String> elements{"C", "H", "O"};
      const std::vector<G4int> natoms{32,48,13};
      auto zflex = G4NISTManager->ConstructNewMaterial("Z-FLEX", elements, natoms, d);
      thisConfig()->SetValue(unit, std::shared_ptr<G4Material>(zflex));
  }

  if (unit.compare("RW3") == 0) {
      d = 1.045 * g / cm3;
      const std::vector<G4String> elm{"H", "C", "O","Ti"};
      const std::vector<G4double> weight{0.0759, 0.9041, 0.008, 0.012};
      auto RW3 = G4NISTManager->ConstructNewMaterial("RW3", elm, weight, d);
      thisConfig()->SetValue(unit, std::shared_ptr<G4Material>(RW3));
  }

  if (unit.compare("Z-FLEX075") == 0) {
      d =0.75* 0.815 * g / cm3;
      const std::vector<G4String> elements{"C", "H", "O"};
      const std::vector<G4int> natoms{32,48,13};
      auto zflex = G4NISTManager->ConstructNewMaterial("Z-FLEX075", elements, natoms, d);
      thisConfig()->SetValue(unit, std::shared_ptr<G4Material>(zflex));
  }
  if (unit.compare("Z-FLEX05") == 0) {
      d =0.5* 0.815 * g / cm3;
      const std::vector<G4String> elements{"C", "H", "O"};
      const std::vector<G4int> natoms{32,48,13};
      auto zflex = G4NISTManager->ConstructNewMaterial("Z-FLEX05", elements, natoms, d);
      thisConfig()->SetValue(unit, std::shared_ptr<G4Material>(zflex));
  }

  if (unit.compare("TiO2") == 0) {
      d = 4.23 * g / cm3;
      const std::vector<G4String> elements{"Ti", "O"};
      const std::vector<G4int> natoms{1,2};
      auto tio2 = G4NISTManager->ConstructNewMaterial("TiO2", elements, natoms, d);
      thisConfig()->SetValue(unit, std::shared_ptr<G4Material>(tio2));
  }

  if (unit.compare("RMPS470") == 0) {
      d = 1.21 * g / cm3;
      const std::vector<G4String> elements{"H","C","N","O"};
      // const std::vector<G4double> masses{5.925*perCent,77.957*perCent,0.345*perCent,15.774*perCent,0.0*perCent};
      const std::vector<G4int> natoms{239,264,1,40};
      auto rmps_470 = G4NISTManager->ConstructNewMaterial("RMPS470", elements, natoms, d, true, kStateSolid, 299.15);
      thisConfig()->SetValue(unit, std::shared_ptr<G4Material>(rmps_470));
  }

  if (unit.compare("EPS") == 0) {
    d = 0.02 * g / cm3;
    const std::vector<G4String> elements{"H","C"};
    const std::vector<G4int> natoms{8,8};
    auto eps = G4NISTManager->ConstructNewMaterial("EPS", elements, natoms, d, true, kStateSolid, 299.15);
    thisConfig()->SetValue(unit, std::shared_ptr<G4Material>(eps));
}

if (unit.compare("Rubber") == 0) {
  d = 1.22 * g / cm3;
  const std::vector<G4String> elements{"H","C"};
  const std::vector<G4int> natoms{8,5};
  auto rubber = G4NISTManager->ConstructNewMaterial("Rubber", elements, natoms, d, true, kStateSolid, 299.15);
  thisConfig()->SetValue(unit, std::shared_ptr<G4Material>(rubber));
}

  if (unit.compare("BaritesConcrete") == 0) {
      d = 3.3 * g / cm3;
      const std::vector<G4String> elements{"H","O","Si","Al","Ca","Fe","Mg","S","Ba"};
      const std::vector<G4int> natoms{76,282,21,1,14,21,2,38,37};
      auto carites_concrete = G4NISTManager->ConstructNewMaterial("BaritesConcrete", elements, natoms, d, true, kStateSolid, 299.15);
      thisConfig()->SetValue(unit, std::shared_ptr<G4Material>(carites_concrete));
  }


}

////////////////////////////////////////////////////////////////////////////////
///
