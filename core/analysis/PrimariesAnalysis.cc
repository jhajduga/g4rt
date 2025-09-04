//
// Created by brachwal on 24.05.2020.
//

#include "PrimariesAnalysis.hh"
#include "G4Event.hh"
#include "G4Run.hh"
#include "G4AnalysisManager.hh"
#include "G4PrimaryVertex.hh"
#include "G4PrimaryParticle.hh"
#include "G4Gamma.hh"
#include "G4Proton.hh"
#include "G4Positron.hh"
#include "G4Electron.hh"
#include "G4Neutron.hh"
#include "Services.hh"

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Returns the global singleton instance of PrimariesAnalysis.
 *
 * Provides access to the single, shared PrimariesAnalysis object used by the application.
 * The instance is created on first use and has static lifetime (Meyers singleton); initialization
 * is thread-safe in conforming C++11 (and later) implementations.
 *
 * @return PrimariesAnalysis* Pointer to the singleton PrimariesAnalysis instance.
 */
PrimariesAnalysis *PrimariesAnalysis::GetInstance() {
  static PrimariesAnalysis instance = PrimariesAnalysis();
  return &instance;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Initialize analysis and book the "PrimariesTree" ntuple at the start of a Geant4 run.
 *
 * Books an ntuple titled "PrimariesTree" containing integer count columns for gamma, electron,
 * positron, neutron, proton and total primaries, and double/vector columns for per-primary
 * energies, kinematic components, vertex coordinates, momentum magnitude/components, polar angle,
 * vertex index, and weight. Resets all per-run counters to zero and binds internal vectors to the
 * ntuple columns so they will be written when rows are added during event processing.
 *
 * @param runPtr Pointer to the current G4Run (may be null in some contexts).
 * @param isMaster True when called for the master thread/process; currently unused.
 */
void PrimariesAnalysis::BeginOfRun(const G4Run* runPtr, G4bool isMaster){

  auto analysisManager =  G4AnalysisManager::Instance();

  // Book Primaries data Ntuple
  //------------------------------------------
  std::string treeName = "PrimariesTree";
  ANA_INFO("PrimariesAnalysis::Defining TTree: {}",treeName);
  auto ntupleId = analysisManager->CreateNtuple(treeName,"Primaries data");
  m_ntupleId.Put(ntupleId);
  m_gammaN.Put(0);
  m_electronN.Put(0);
  m_positronN.Put(0);
  m_neutronN.Put(0);
  m_protonN.Put(0);
  m_primaryN.Put(0);
  analysisManager->CreateNtupleIColumn(ntupleId, "gammaN");                               // column Id = 0
  analysisManager->CreateNtupleDColumn(ntupleId, "gammaEnergy",m_gammaE.Get());        // column Id = 1
  analysisManager->CreateNtupleIColumn(ntupleId, "electronN");                            // column Id = 2
  analysisManager->CreateNtupleDColumn(ntupleId, "electronEnergy",m_electronE.Get());  // column Id = 3
  analysisManager->CreateNtupleIColumn(ntupleId, "positronN");                            // column Id = 4
  analysisManager->CreateNtupleDColumn(ntupleId, "positronEnergy",m_positronE.Get());  // column Id = 5
  analysisManager->CreateNtupleIColumn(ntupleId, "neutronN");                             // column Id = 6
  analysisManager->CreateNtupleDColumn(ntupleId, "neutronEnergy",m_neutronE.Get());    // column Id = 7
  analysisManager->CreateNtupleIColumn(ntupleId, "protonN");                              // column Id = 8
  analysisManager->CreateNtupleDColumn(ntupleId, "protonEnergy",m_protonE.Get());      // column Id = 9
  analysisManager->CreateNtupleIColumn(ntupleId, "primaryN");                             // column Id = 10
  analysisManager->CreateNtupleDColumn(ntupleId, "primaryE",m_primaryE.Get());         // column Id = 11
  analysisManager->CreateNtupleDColumn(ntupleId, "primaryEx",m_primaryEx.Get());       // column Id = 12
  analysisManager->CreateNtupleDColumn(ntupleId, "primaryEy",m_primaryEy.Get());       // column Id = 13
  analysisManager->CreateNtupleDColumn(ntupleId, "primaryX",m_primaryX.Get());         // column Id = 14
  analysisManager->CreateNtupleDColumn(ntupleId, "primaryY",m_primaryY.Get());         // column Id = 15
  analysisManager->CreateNtupleDColumn(ntupleId, "primaryZ",m_primaryZ.Get());         // column Id = 16
  analysisManager->CreateNtupleDColumn(ntupleId, "primaryP",m_primaryP.Get());         // column Id = 17
  analysisManager->CreateNtupleDColumn(ntupleId, "primaryPx",m_primaryPx.Get());         // column Id = 18
  analysisManager->CreateNtupleDColumn(ntupleId, "primaryPy",m_primaryPy.Get());         // column Id = 19
  analysisManager->CreateNtupleDColumn(ntupleId, "primaryPz",m_primaryPz.Get());         // column Id = 20
  analysisManager->CreateNtupleDColumn(ntupleId, "primaryTheta",m_primaryTheta.Get());         // column Id = 21
  analysisManager->CreateNtupleDColumn(ntupleId, "primaryVtxId",m_primaryVertexId.Get());         // column Id = 22
  analysisManager->CreateNtupleDColumn(ntupleId, "primaryWeight",m_primaryWeight.Get());         // column Id = 23
  analysisManager->FinishNtuple(ntupleId);

}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Finalize per-event primary data: write to ntuple and clear event buffers.
 *
 * Writes the currently collected primary-particle information into the booked ntuple
 * and then resets all per-event counters and containers so the object is ready for
 * the next event.
 *
 * @param evt Pointer to the finished G4 event; not accessed by this implementation
 *            (present to match the action interface).
 */
void PrimariesAnalysis::EndOfEventAction(const G4Event *evt){
  FillPrimariesNTuple();
  ClearPrimariesEventData();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Reset per-event primary counters and clear stored primary data.
 *
 * @details Sets all per-event integer counters (gamma, electron, positron,
 * neutron, proton, and total primaries) to zero and clears all vectors that
 * hold per-primary energies, momentum components, positions, angles,
 * vertex indices, and weights. Call at the end of an event to prepare for
 * collecting data from the next event.
 */
void PrimariesAnalysis::ClearPrimariesEventData(){
  m_gammaN.Put(0);
  m_electronN.Put(0);
  m_positronN.Put(0);
  m_neutronN.Put(0);
  m_protonN.Put(0);
  m_primaryN.Put(0);
  m_gammaE.Clear();
  m_electronE.Clear();
  m_positronE.Clear();
  m_neutronE.Clear();
  m_protonE.Clear();
  m_primaryE.Clear();
  m_primaryEx.Clear();
  m_primaryEy.Clear();
  m_primaryP.Clear();
  m_primaryPx.Clear();
  m_primaryPy.Clear();
  m_primaryPz.Clear();
  m_primaryX.Clear();
  m_primaryY.Clear();
  m_primaryZ.Clear();
  m_primaryTheta.Clear();
  m_primaryVertexId.Clear();
  m_primaryWeight.Clear();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Extracts primary particle information from a Geant4 event and stores it for later ntuple filling.
 *
 * Iterates over all primary vertices in the provided event, processes the first primary at each vertex, and
 * updates per-type counters (gamma, electron, positron, neutron, proton) and per-event storage vectors with
 * kinematic and vertex information. For each processed primary the function records total energy, mass-related
 * energy components (Ex, Ey), momentum magnitude and components, polar angle (theta), transformed vertex
 * coordinates (isocenter-to-simulation shift applied and converted to cm), vertex index, and primary weight.
 *
 * Note: The isocenter-to-simulation translation is obtained from the configuration service; only the first
 * primary of each vertex (index 0) is used.
 *
 * @param evt Pointer to the Geant4 event containing primary vertices and particles.
 */
void PrimariesAnalysis::FillPrimaries(const G4Event *evt){
// void PrimariesAnalysis::FillPrimaries(G4PrimaryVertex* vertex){
  auto nVertex = evt->GetNumberOfPrimaryVertex();
  m_primaryN.Put(nVertex);
  auto isoToSim = Service<ConfigSvc>()->GetValue<G4ThreeVector>("WorldConstruction", "IsoToSimTransformation");
  if(nVertex>0){
    for(int n=0;n<nVertex;++n){
      auto vertex = evt->GetPrimaryVertex(n);
      if(vertex) {
        auto particle = vertex->GetPrimary(0); // vertex is supposed to have one primary (temporary)
        auto partDef = particle->GetParticleDefinition();
        if (partDef == G4Gamma::Definition()) {
          m_gammaN.Put(m_gammaN.Get() + 1);
          m_gammaE.Push_back(particle->GetTotalEnergy());
        } else if (partDef == G4Electron::Definition()) {
          m_electronN.Put(m_electronN.Get() + 1);
          m_electronE.Push_back(particle->GetTotalEnergy());
        } else if (partDef == G4Positron::Definition()) {
          m_positronN.Put(m_positronN.Get() + 1);
          m_positronE.Push_back(particle->GetTotalEnergy());
        } else if (partDef == G4Neutron::Definition()) {
          m_neutronN.Put(m_neutronN.Get() + 1);
          m_neutronE.Push_back(particle->GetTotalEnergy());
        } else if (partDef == G4Proton::Definition()) {
          m_protonN.Put(m_protonN.Get() + 1);
          m_protonE.Push_back(particle->GetTotalEnergy());
        }

        // Merge all particles info together
        m_primaryE.Push_back(particle->GetTotalEnergy());
        auto mass = particle->GetMass();
        auto px = particle->GetPx();
        auto py = particle->GetPy();
        auto weight = particle->GetWeight();
        m_primaryEx.Push_back(std::sqrt(mass * mass + px * px));
        m_primaryEy.Push_back(std::sqrt(mass * mass + py * py));
        m_primaryX.Push_back((vertex->GetX0() - isoToSim.x()) / cm);
        m_primaryY.Push_back((vertex->GetY0() - isoToSim.y()) / cm);
        m_primaryZ.Push_back((vertex->GetZ0() - isoToSim.z()) / cm);

        m_primaryP.Push_back(particle->GetMomentum().mag());
        m_primaryPx.Push_back(px);
        m_primaryPy.Push_back(py);
        m_primaryPz.Push_back(particle->GetPz());

        m_primaryTheta.Push_back(particle->GetMomentum().theta());
        m_primaryVertexId.Push_back(n);
        m_primaryWeight.Push_back(weight);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Writes primary particle count data to the ntuple.
 *
 * Fills the ntuple integer columns with the current event's counts of gamma, electron, positron, neutron, proton, and total primary particles, then adds a new row to the ntuple.
 */
void PrimariesAnalysis::FillPrimariesNTuple() {
  auto analysisManager = G4AnalysisManager::Instance();
  auto ntupleId = m_ntupleId.Get();
  analysisManager->FillNtupleIColumn( ntupleId,0, m_gammaN.Get());
  analysisManager->FillNtupleIColumn( ntupleId,2, m_electronN.Get());
  analysisManager->FillNtupleIColumn( ntupleId,4, m_positronN.Get());
  analysisManager->FillNtupleIColumn( ntupleId,6, m_neutronN.Get());
  analysisManager->FillNtupleIColumn( ntupleId,8, m_protonN.Get());
  analysisManager->FillNtupleIColumn( ntupleId,10, m_primaryN.Get());
  analysisManager->AddNtupleRow(ntupleId);
}