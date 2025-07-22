/**
*
* \author B. Rachwal
* \date 05.05.2020
*
*/

#ifndef Dose3D_ACTIONINITIALIZATION_HH
#define Dose3D_ACTIONINITIALIZATION_HH

#include "G4VUserActionInitialization.hh"

///\class ActionInitialization
class ActionInitialization : public G4VUserActionInitialization {
  public:
  ///
  ActionInitialization() = default;

  ///
  virtual ~ActionInitialization() = default;

  ///
  void BuildForMaster() const override;

  ///
  void Build() const override;
};

#endif // Dose3D_ACTIONINITIALIZATION_HH
