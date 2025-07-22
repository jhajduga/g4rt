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
  /**
 * @brief Constructs an ActionInitialization object with default settings.
 */
  ActionInitialization() = default;

  /**
 * @brief Destroys the ActionInitialization instance.
 *
 * Default virtual destructor for proper cleanup in derived classes.
 */
  virtual ~ActionInitialization() = default;

  ///
  void BuildForMaster() const override;

  ///
  void Build() const override;
};

#endif // Dose3D_ACTIONINITIALIZATION_HH
