/**
*
* \author B.Rachwal (brachwal@agh.edu.pl)
* \date 10.03.2018
*
*/

#ifndef Dose3D_UIMANAGER_H
#define Dose3D_UIMANAGER_H

#include <memory>
#include "G4UImanager.hh"

////////////////////////////////////////////////////////////////////////////////
///
///\class UIManager
///\brief The class wrapper for G4UImanager.
/// Among others, in order to handle the Geant4 fashion of .mac files reading executing them.
class UIManager {
  private:
  // Standard constructor
  UIManager();

  // Delete the copy and move constructors
  UIManager(const UIManager &) = delete;

  UIManager &operator=(const UIManager &) = delete;

  UIManager(UIManager &&) = delete;

  UIManager &operator=(UIManager &&) = delete;

  // Standard destructor
  ~UIManager() = default;

  // Indicates different stages of application run chain
  enum ProcessingFlag {
    preBeamOn,
    postBeamOn
  };

  G4UImanager *UIG4Manager;

  // Control the G4 kernel initialization
  G4bool m_isG4kernelInitialized;

  // List of commands being read from .mac file
  // commands to be applied >> BEFORE << beamOn execution
  std::vector<G4String> PreBeamOnCommands;
  // commands to be applied >> AFTER << beamOn execution
  std::vector<G4String> PostBeamOnCommands;

  // Process the .mac files container
  void ReadMacros(const std::vector<G4String> &m_MacFiles);

  // Read the signle .mac file
  void ReadMacro(const G4String &m_MacFile, const ProcessingFlag m_WhenToApply);

  public:
  static UIManager *GetInstance();

  void InitializeG4kernel();

  void UserRunInitialization();

  void UserRunFinalization();

  // Simple proxy method for the one from G4UImanager
  void ApplyCommand(const G4String &m_command);
};

#endif
