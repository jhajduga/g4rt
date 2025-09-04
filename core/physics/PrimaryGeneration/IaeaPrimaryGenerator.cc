#include "IaeaPrimaryGenerator.hh"
#include "G4IAEAphspReader.hh"
#include "Services.hh"
#include "PrimariesAnalysis.hh"
#include "G4Event.hh"


G4IAEAphspReader* IaeaPrimaryGenerator::m_iaeaFileReader = nullptr;

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Construct and initialize the shared IAEA phase-space reader.
 *
 * Initializes the static G4IAEAphspReader on first construction using the supplied
 * phase-space file. Initialization configures the reader to not recycle phase-space
 * entries (SetTimesRecycled(0)), sets the isocenter to the origin, and applies a
 * global translation along Z using the runtime configuration value "RunSvc.phspShiftZ".
 * Subsequent constructions are no-ops (the already-initialized static reader is reused).
 *
 * @param fileName Path to the IAEA phase-space file used to create the shared reader.
 */
IaeaPrimaryGenerator::IaeaPrimaryGenerator(const G4String& fileName) {

  if(!m_iaeaFileReader){ 

    // auto id_thread = G4Threading::G4GetThreadId();
    // // auto max_thread = G4Threading::GetNumberOfRunningWorkerThreads();
    // auto max_thread = Service<ConfigSvc>()->GetValue<int>("RunSvc", "NumberOfThreads");

    m_iaeaFileReader = new G4IAEAphspReader(fileName.data());
    // m_iaeaFileReader->SetParallelRun(1 + id_thread);
    // m_iaeaFileReader->SetTotalParallelRuns(max_thread);

    m_iaeaFileReader->SetTimesRecycled(0); // default -> 0 


    // // TODO: Check these func!!!
    // m_iaeaFileReader->SetGantryAngle();
    // m_iaeaFileReader->SetCollimatorAngle();
    // m_iaeaFileReader->SetIsocenterPosition();

    // // TODO: Check how to setup multiple PHSP files!
    

    // define the position of the isocenter
    // it’s mandatory before rotations around this point
    m_iaeaFileReader->SetIsocenterPosition(G4ThreeVector(0., 0., 0. * cm));
  
    // phase-space plane shift
    m_phspShiftZ = Service<ConfigSvc>()->GetValue<double>("RunSvc", "phspShiftZ");
    m_iaeaFileReader->SetGlobalPhspTranslation(G4ThreeVector(0.,0.,-m_phspShiftZ));
    G4cout << "[INFO]:: IaeaPrimaryGenerator:: Setting phase space Z position " << m_phspShiftZ / cm  << " [cm]"<< G4endl;
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Generates a primary vertex for the given event using the IAEA phase-space reader.
 *
 * Delegates the creation of the primary vertex to the shared `G4IAEAphspReader` instance for the specified Geant4 event.
 */
void IaeaPrimaryGenerator::GeneratePrimaryVertex(G4Event *evt) {
  // the actual generation is defined in G4IAEAphspReader, so it's being delegated:
  m_iaeaFileReader->GeneratePrimaryVertex(evt);
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Retrieve all primary vertices for the given Geant4 event from the shared IAEA phase-space reader.
 *
 * The function queries the shared phase-space reader using the event's ID and returns the
 * vector of G4PrimaryVertex pointers produced for that event.
 *
 * @param evt Geant4 event whose primary-vertex vector will be retrieved (must be non-null).
 * @return std::vector<G4PrimaryVertex*> Vector of primary vertices for the event; may be empty if the reader produces none.
 */
std::vector<G4PrimaryVertex*> IaeaPrimaryGenerator::GeneratePrimaryVertexVector(G4Event *evt) {
  return m_iaeaFileReader->ReadThisEventPrimaryVertexVector(evt->GetEventID());
}