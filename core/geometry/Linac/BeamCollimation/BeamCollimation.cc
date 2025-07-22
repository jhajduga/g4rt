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
G4double BeamCollimation::BeforeJaws  = -1000;
G4double BeamCollimation::ParticleAngleTreshold = 50.0; // [deg]
////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Constructs a BeamCollimation instance and registers it with the run service.
 *
 * Initializes the BeamCollimation component and ensures it is registered for run management within the simulation framework.
 */
BeamCollimation::BeamCollimation() : IPhysicalVolume("BeamCollimation"){
    AcceptRunVisitor(Service<RunSvc>());
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Destructor for the BeamCollimation class.
 *
 * Cleans up allocated resources by calling Destroy().
 */
BeamCollimation::~BeamCollimation() {
  Destroy();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Returns the singleton instance of the BeamCollimation class.
 *
 * Ensures only one instance of BeamCollimation exists throughout the application.
 *
 * @return Pointer to the singleton BeamCollimation instance.
 */
BeamCollimation *BeamCollimation::GetInstance() {
  static BeamCollimation instance;
  return &instance;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Registers this beam collimation component with the provided run service.
 *
 * Enables the run service to manage or interact with this component during simulation runs.
 */
void BeamCollimation::AcceptRunVisitor(RunSvc *visitor){
    visitor->RegisterRunComponent(this);
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Outputs the nominal beam energy and current jaw aperture sizes to the console.
 *
 * Displays the nominal beam energy and the X and Y jaw apertures in centimeters for informational purposes.
 */
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
/**
 * @brief Deletes all jaw physical volumes managed by this instance and resets their pointers.
 *
 * Frees memory for all jaw physical volumes stored in the internal map and sets their pointers to nullptr. The MLC instance pointer is set to nullptr but not deleted to avoid double deletion.
 */
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
/**
 * @brief Constructs the multi-leaf collimator (MLC) geometry within the specified parent world volume.
 *
 * @param parentWorld The parent Geant4 physical volume in which the MLC geometry will be placed.
 */
void BeamCollimation::Construct(G4VPhysicalVolume *parentWorld) {
  MLC(parentWorld);
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Resets the state of the BeamCollimation component.
 *
 * This method is a placeholder for logic to reset internal state or geometry of the beam collimation system.
 */
void BeamCollimation::Reset() {
  // TODO
}

/**
 * @brief Updates jaw aperture settings and positions based on the provided control point.
 *
 * Clears and sets jaw aperture values from the control point. For custom plan fields with a supported MLC model, recalculates and applies jaw positions and rotations according to the new apertures.
 *
 * @param control_point The control point containing field type and jaw aperture information.
 */
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

  if((inputType=="CustomPlan" && (model != EMlcModel::Simplified && model != EMlcModel::None))){
    setCustomPositioning("Jaw1X");
    setCustomPositioning("Jaw2X");
    setCustomPositioning("Jaw1Y");
    setCustomPositioning("Jaw2Y");
  }

}


////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Filters and processes primary particle vertices based on angular and geometric collimation criteria.
 *
 * Removes primary vertices whose particle momentum exceeds the angular threshold or, for the simplified MLC model, are outside the MLC field. For other MLC models, adjusts particle positions to a plane before the jaws. Updates the simulation field mask with the filtered vertices.
 *
 * @param p_vrtx Vector of pointers to primary vertices to be filtered and processed. Modified in place.
 */

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
/**
 * @brief Repositions a primary vertex to a specified Z-plane along its momentum direction.
 *
 * Calculates the intersection point of the particle's trajectory with a plane at the given Z-coordinate and updates the vertex position accordingly.
 *
 * @param vrtx The primary vertex to reposition.
 * @param finalZ The Z-coordinate of the target plane.
 * @return G4ThreeVector The new position of the vertex on the specified Z-plane.
 */
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
/**
 * @brief Adjusts the position, size, and rotation of a jaw based on its aperture and geometric configuration.
 *
 * Updates the provided center position, half-size dimensions, and rotation matrix for the specified jaw to reflect the current aperture setting and isocenter geometry. Supports four jaw names: "Jaw1X", "Jaw2X", "Jaw1Y", and "Jaw2Y".
 *
 * @param name The identifier of the jaw ("Jaw1X", "Jaw2X", "Jaw1Y", or "Jaw2Y").
 * @param centre Reference to the jaw's center position, which will be updated.
 * @param halfSize Reference to the jaw's half-size vector, which will be updated.
 * @param cRotation Pointer to the jaw's rotation matrix, which will be updated.
 */
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
/**
 * @brief Constructs and places jaw collimator volumes within the parent world volume.
 *
 * Creates four tungsten jaw collimators (Jaw1X, Jaw2X, Jaw1Y, Jaw2Y) as Geant4 box volumes, assigns them to logical and physical volumes, and sets up production cut regions for each jaw.
 *
 * @param parentWorld The parent Geant4 physical volume in which the jaws are placed.
 * @return true upon successful construction and placement of all jaw volumes.
 */
bool BeamCollimation::Jaws(G4VPhysicalVolume *parentWorld) {
  auto jaw = [&](const std::string& name, G4ThreeVector centre, const G4ThreeVector& halfSize) {
    auto tungsten = Service<ConfigSvc>()->GetValue<G4MaterialSPtr>("MaterialsSvc", "G4_W");
    auto cRotation = new G4RotationMatrix();
    auto box = new G4Box(name + "Box", halfSize.getX(), halfSize.getY(), halfSize.getZ());
    auto logVol = new G4LogicalVolume(box, tungsten.get(), name + "LV", 0, 0, 0);
    if(m_physicalVolume[name]!=nullptr)
      delete m_physicalVolume[name];
    m_physicalVolume[name] = new G4PVPlacement(cRotation, centre, name + "PV", logVol, parentWorld, false, 0);

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
/**
 * @brief Constructs or resets the multi-leaf collimator (MLC) geometry based on the selected MLC model.
 *
 * Depending on the current MLC model configuration, this method instantiates and constructs the appropriate MLC geometry (Millennium, HD120, or Simplified) and ensures jaw collimators are present. If an MLC instance already exists, it is reset or replaced as needed. For the HD120 model, the geometry is explicitly constructed. If the model is set to None, the method returns immediately.
 *
 * @param parentWorld The parent physical volume in which the MLC and jaws are constructed.
 * @return true Always returns true after construction or reset.
 */
bool BeamCollimation::MLC(G4VPhysicalVolume *parentWorld) {

  auto model = Service<GeoSvc>()->GetMlcModel();

  if(model == EMlcModel::None)
    return true;

  if(!m_mlc){
    G4cout << "[INFO]:: BeamCollimation::MLC: Constructing the MLC model instantiation! " << G4endl;
    switch (model) {
      case EMlcModel::Millennium:
        Jaws(parentWorld);
        //m_mlc = std::make_unique<MlcMillennium>(parentWorld);
        break;
      case EMlcModel::HD120:
        Jaws(parentWorld);
        m_mlc = new MlcHd120();
        dynamic_cast<MlcHd120*>(m_mlc)->IPhysicalVolume::Construct(this);
        break;
      case EMlcModel::Simplified:
        INFO_GEO("Using Simplified type of MLC");
        m_mlc = new MlcSimplified;
        break;
    }
  } else {
    G4cout << "[INFO]:: BeamCollimation::MLC: RESET the MLC model instantiation! " << G4endl;
    switch (model) {
      case EMlcModel::Millennium:
        Jaws(parentWorld);
        //m_mlc.reset(new MlcMillennium(m_parentPV));
        break;
      case EMlcModel::HD120:
        Jaws(parentWorld);
        delete m_mlc;
        m_mlc = new MlcHd120();
        dynamic_cast<MlcHd120*>(m_mlc)->IPhysicalVolume::Construct(this);
        break;
      case EMlcModel::Simplified:
        delete m_mlc;
        INFO_GEO("Using Simplified type of MLC");
        m_mlc = new MlcSimplified();
        break;
    }
  }
  return true;
}
