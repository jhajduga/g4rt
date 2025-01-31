#include "LinacGeometry.hh"
#include "BeamCollimation.hh"
#include "MlcMillennium.hh"
#include "MlcSimplified.hh"
#include "MlcHD120.hh"
#include "Services.hh"

#include "G4LogicalVolume.hh"
#include "G4VPhysicalVolume.hh"
#include "G4SystemOfUnits.hh"
#include "G4PVPlacement.hh"
#include "G4ProductionCuts.hh"
#include "G4Tubs.hh"
#include "G4Box.hh"
#include "G4Cons.hh"

VMlc* BeamCollimation::m_mlc = nullptr;
// Note: the following values are in mm and taken from PRIMO setup, 
//       take into account that in PRIMO 0,0,0 is at the source position!
G4double BeamCollimation::AfterMLC = -300.25;   
G4double BeamCollimation::BeforeMLC  = -415.0;
G4double BeamCollimation::BeforeJaws  = -745.3;
G4double BeamCollimation::ParticleAngleTreshold = 50.0; // [deg]
////////////////////////////////////////////////////////////////////////////////
///
BeamCollimation::BeamCollimation() : IPhysicalVolume("BeamCollimation"){
    AcceptRunVisitor(Service<RunSvc>());
}

////////////////////////////////////////////////////////////////////////////////
///
BeamCollimation::~BeamCollimation() {
  Destroy();
}

////////////////////////////////////////////////////////////////////////////////
///
BeamCollimation *BeamCollimation::GetInstance() {
  static BeamCollimation instance;
  return &instance;
}

////////////////////////////////////////////////////////////////////////////////
///
void BeamCollimation::AcceptRunVisitor(RunSvc *visitor){
    visitor->RegisterRunComponent(this);
}

////////////////////////////////////////////////////////////////////////////////
///
void BeamCollimation::WriteInfo() {
  G4cout << "\n\n\tnominal beam energy: " << Service<ConfigSvc>()->GetValue<int>("RunSvc", "idEnergy") << G4endl;
  G4cout << "\tJaw X aperture: 1) "
         << m_apertures["Jaw1X"] / cm << "[cm]\t2) "
         << m_apertures["Jaw2X"] / cm << " [cm]" << G4endl;
  G4cout << "\tJaw Y aperture: 1) "
         << m_apertures["Jaw1Y"] / cm << "[cm]\t2) "
         << m_apertures["Jaw2Y"] / cm << " [cm]\n" << G4endl;
}

////////////////////////////////////////////////////////////////////////////////
///
void BeamCollimation::Destroy() {
  for (auto &ivolume : m_physicalVolume) {
    if (ivolume.second) {
      delete ivolume.second;
      ivolume.second = nullptr;
    }
  }
  // delete m_mlc; DBG double free or corruption (!prev)
  m_mlc = nullptr;
}

////////////////////////////////////////////////////////////////////////////////
///
void BeamCollimation::Construct(G4VPhysicalVolume *parentWorld) {
  m_parentPV = parentWorld;
  MLC();
}

////////////////////////////////////////////////////////////////////////////////
///
void BeamCollimation::Reset() {
  // TODO
}

void BeamCollimation::SetRunConfiguration(const ControlPoint* control_point){
  auto inputType = control_point->GetFieldType();
  auto model = Service<GeoSvc>()->GetMlcModel();
  m_apertures.clear();
  m_apertures["Jaw1X"] = control_point->GetJawAperture("X1");
  m_apertures["Jaw2X"] = control_point->GetJawAperture("X2");
  m_apertures["Jaw1Y"] = control_point->GetJawAperture("Y1");
  m_apertures["Jaw2Y"] = control_point->GetJawAperture("Y2");

  auto setCustomPositioning = [&](const std::string& name) {
    auto cRotation = new G4RotationMatrix();
    auto centre = m_physicalVolume[name]->GetTranslation();
    auto halfsize = svc::getHalfSize(m_physicalVolume[name]);
    SetJawAperture(name, centre, halfsize, cRotation);
    m_physicalVolume[name]->SetTranslation(centre);
    m_physicalVolume[name]->SetRotation(cRotation);
  };

  if((inputType=="CustomPlan" && (model != EMlcModel::Simplified))){
    setCustomPositioning("Jaw1X");
    setCustomPositioning("Jaw2X");
    setCustomPositioning("Jaw1Y");
    setCustomPositioning("Jaw2Y");
  }

}


////////////////////////////////////////////////////////////////////////////////
///

void BeamCollimation::FilterPrimaries(std::vector<G4PrimaryVertex*>& p_vrtx) {
  Service<RunSvc>()->CurrentControlPoint()->MLC();

  auto model = Service<GeoSvc>()->GetMlcModel();
  for(int i=0; i < p_vrtx.size();++i){
    auto vrtx = p_vrtx.at(i);
    auto theta = vrtx->GetPrimary()->GetMomentum().theta();
    if (theta * 180.0 / M_PI > ParticleAngleTreshold ){
      delete vrtx;
      p_vrtx.at(i) = nullptr;
      continue;
    } 
    if(model == EMlcModel::Simplified){
        if(!m_mlc->IsInField(vrtx)) {
          delete vrtx;
          p_vrtx.at(i) = nullptr;
        }
    } else {
      BeamCollimation::SetParticlePositionBeforeCollimators(p_vrtx.at(i), BeforeJaws);
    }
  }
  p_vrtx.erase(std::remove_if(p_vrtx.begin(), p_vrtx.end(), [](G4PrimaryVertex* ptr) { return ptr == nullptr; }), p_vrtx.end());
  Service<RunSvc>()->CurrentControlPoint()->FillSimFieldMask(p_vrtx);
}


////////////////////////////////////////////////////////////////////////////////
///
G4ThreeVector BeamCollimation::SetParticlePositionBeforeCollimators(G4PrimaryVertex* vrtx, G4double finalZ) {
  G4ThreeVector position = vrtx->GetPosition();
  G4double deltaZ = finalZ - position.getZ();
  G4ThreeVector directionalVersor = vrtx->GetPrimary()->GetMomentumDirection();
  G4double zRatio = deltaZ / directionalVersor.getZ(); 
  G4double x = position.getX() + zRatio * directionalVersor.getX();
  G4double y = position.getY() + zRatio * directionalVersor.getY();
  vrtx->SetPosition(x, y, finalZ);
  return G4ThreeVector(x, y, finalZ);
}


////////////////////////////////////////////////////////////////////////////////
///
void BeamCollimation::SetJawAperture(const std::string& name, G4ThreeVector &centre, G4ThreeVector halfSize,
                                      G4RotationMatrix *cRotation) {
  G4double x = centre.getX();
  G4double y = centre.getY();
  G4double z = centre.getZ();
  G4double dx = halfSize.getX();
  G4double dy = halfSize.getY();
  G4double dz = halfSize.getZ();
  G4double aperture = m_apertures[name];
  G4double theta = fabs(atan(aperture / Service<ConfigSvc>()->GetValue<G4double>("GeoSvc", "isoCentre")));

  if (name=="Jaw1X") { // idJaw1XV2100:
      centre.set(z * sin(theta) + dx * cos(theta), y, z * cos(theta) - dx * sin(theta));
      cRotation->rotateY(-theta);
      halfSize.set(fabs(dx * cos(theta) + dz * sin(theta)), fabs(dy), fabs(dz * cos(theta) + dx * sin(theta)));
  } else if (name=="Jaw2X") {  // idJaw2XV2100:
      centre.set(-(z * sin(theta) + dx * cos(theta)), y, z * cos(theta) - dx * sin(theta));
      cRotation->rotateY(theta);
      halfSize.set(fabs(dx * cos(theta) + dz * sin(theta)), fabs(dy), fabs(dz * cos(theta) + dx * sin(theta)));
  } else if (name=="Jaw1Y") {  // idJaw1YV2100:
      centre.set(x, z * sin(theta) + dy * cos(theta), z * cos(theta) - dy * sin(theta));
      cRotation->rotateX(theta);
      halfSize.set(fabs(dx), fabs(dy * cos(theta) + dz * sin(theta)), fabs(dz * cos(theta) + dy * sin(theta)));
  } else if (name=="Jaw2Y") {  // idJaw2YV2100:
      centre.set(x, -(z * sin(theta) + dy * cos(theta)), z * cos(theta) - dy * sin(theta));
      cRotation->rotateX(-theta);
      halfSize.set(fabs(dx), fabs(dy * cos(theta) + dz * sin(theta)), fabs(dz * cos(theta) + dy * sin(theta)));
  }
}

////////////////////////////////////////////////////////////////////////////////
///
bool BeamCollimation::Jaws() {
  auto jaw = [&](const std::string& name, G4ThreeVector centre, const G4ThreeVector& halfSize) {
    auto tungsten = Service<ConfigSvc>()->GetValue<G4MaterialSPtr>("MaterialsSvc", "G4_W");
    auto cRotation = new G4RotationMatrix();
    auto box = new G4Box(name + "Box", halfSize.getX(), halfSize.getY(), halfSize.getZ());
    auto logVol = new G4LogicalVolume(box, tungsten.get(), name + "LV", 0, 0, 0);
    if(m_physicalVolume[name]!=nullptr)
      delete m_physicalVolume[name];
    m_physicalVolume[name] = new G4PVPlacement(cRotation, centre, name + "PV", logVol, m_parentPV, false, 0);

    // Region for cuts
    auto regVol = new G4Region(name + "R");
    auto *cuts = new G4ProductionCuts;
    cuts->SetProductionCut(0.001 * mm);
    regVol->SetProductionCuts(cuts);
    logVol->SetRegion(regVol);
    regVol->AddRootLogicalVolume(logVol);
  };
  jaw("Jaw1X",G4ThreeVector(0., 0., 320.*mm),G4ThreeVector(55.*mm, 100.*mm, 90./2.*mm));
  jaw("Jaw2X",G4ThreeVector(0., 0., 320.*mm),G4ThreeVector(55.*mm, 100.*mm, 90./2.*mm));
  jaw("Jaw1Y",G4ThreeVector(0., 0., 450.*mm),G4ThreeVector(100.*mm, 45.*mm, 90./2.*mm));
  jaw("Jaw2Y",G4ThreeVector(0., 0., 450.*mm),G4ThreeVector(100.*mm, 45.*mm, 90./2.*mm));

  return true;
}

////////////////////////////////////////////////////////////////////////////////
///
bool BeamCollimation::MLC() {

  auto model = Service<GeoSvc>()->GetMlcModel();

  if(model == EMlcModel::None)
    return true;

  if(!m_mlc){
    G4cout << "[INFO]:: BeamCollimation::MLC: Constructing the MLC model instantiation! " << G4endl;
    switch (model) {
      case EMlcModel::Millennium:
        Jaws();
        //m_mlc = std::make_unique<MlcMillennium>(m_parentPV);
        break;
      case EMlcModel::HD120:
        Jaws();
        m_mlc = new MlcHd120(m_parentPV);
        break;
      case EMlcModel::Simplified:
        // LOGSVC_INFO("Using Simplified type of MLC");
        m_mlc = new MlcSimplified;
        break;
    }
  } else {
    G4cout << "[INFO]:: BeamCollimation::MLC: RESET the MLC model instantiation! " << G4endl;
    switch (model) {
      case EMlcModel::Millennium:
        Jaws();
        //m_mlc.reset(new MlcMillennium(m_parentPV));
        break;
      case EMlcModel::HD120:
        Jaws();
        delete m_mlc;
        m_mlc = new MlcHd120(m_parentPV);
        break;
      case EMlcModel::Simplified:
        delete m_mlc;
        // LOGSVC_INFO("Using Simplified type of MLC");
        m_mlc = new MlcSimplified();
        break;
    }

  }
  return true;
}
