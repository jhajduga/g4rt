#include "IaeaPrimaryGenerator.hh"
#include "G4IAEAphspReader.hh"
#include "Services.hh"
#include "PrimariesAnalysis.hh"
#include "G4Event.hh"


G4IAEAphspReader* IaeaPrimaryGenerator::m_iaeaFileReader = nullptr;

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Initializes the IaeaPrimaryGenerator with a phase-space file.
 *
 * If the static phase-space file reader is not yet initialized, creates it using the provided file name, sets the number of allowed recycles to zero, positions the isocenter at the origin, and applies a configurable Z-axis shift to the phase-space data.
 *
 * @param fileName Path to the IAEA phase-space file to be used for primary particle generation.
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
 * @brief Retrieves all primary vertices for a given event from the phase-space file.
 *
 * @param evt Pointer to the Geant4 event for which primary vertices are generated.
 * @return std::vector<G4PrimaryVertex*> Vector of primary vertices corresponding to the event.
 */
std::vector<G4PrimaryVertex*> IaeaPrimaryGenerator::GeneratePrimaryVertexVector(G4Event *evt) {
  return m_iaeaFileReader->ReadThisEventPrimaryVertexVector(evt->GetEventID());
}