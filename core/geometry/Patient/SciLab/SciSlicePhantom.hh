/**
*
* \author B.Rachwal (brachwal@agh.edu.pl)
* \date 14.06.2021
*
*/

#ifndef SCISLICE_PHANTOM_HH
#define SCISLICE_PHANTOM_HH

#include "G4PVPlacement.hh"
#include "VPatient.hh"

///\class SciSlicePhantom
///\brief The Phantom filled with scintilator
class SciSlicePhantom : public VPatient {
  public:
    ///
    SciSlicePhantom();

    ///
    ~SciSlicePhantom();

    ///
    void Construct(G4VPhysicalVolume *parentPV) override;

    ///
    void Destroy() override;

    ///
    G4bool Update() override;

    /**
 * @brief Reset the phantom to its initial state.
 *
 * This method is a placeholder and not yet implemented. Currently it writes
 * "Implement me." to G4cout; when implemented it should restore the phantom's
 * internal state and geometry to defaults so it can be reused or reconstructed.
 */
    void Reset() override { G4cout << "Implement me." << G4endl; }

    ///
    void WriteInfo() override;

    ///
    void DefineSensitiveDetector() override;

    /**
 * @brief Parses TOML configuration for the SciSlicePhantom.
 *
 * This method is overridden but intentionally left empty, as no TOML configuration is required for this class.
 */
    void ParseTomlConfig() override {}

};

#endif  // SCISLICE_PHANTOM_HH
