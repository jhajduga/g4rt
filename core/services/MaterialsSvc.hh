/**
*
* \author B.Rachwal (brachwal@agh.edu.pl)
* \date 20.07.2018
*
*/

#ifndef LINASIMU_MATERIALS_SVC_HH
#define LINASIMU_MATERIALS_SVC_HH

#include "Configurable.hh"

////////////////////////////////////////////////////////////////////////////////
///
///\class MaterialsSvc
///\brief The material management service.
/// It is a singleton type the pointer of which can be asses trough the templated method:
/// Service<MaterialsSvc>()
class MaterialsSvc : public Configurable {
  private:
  MaterialsSvc();

  ~MaterialsSvc();

  // Delete the copy and move constructors
  MaterialsSvc(const MaterialsSvc &) = delete;

  MaterialsSvc &operator=(const MaterialsSvc &) = delete;

  MaterialsSvc(MaterialsSvc &&) = delete;

  MaterialsSvc &operator=(MaterialsSvc &&) = delete;

  ///\brief Virtual method implementation defining the list of configuration units for this module.
  void Configure() override;

  public:
  ///\brief Static method to get instance of this singleton object.
  static MaterialsSvc *GetInstance();

  ///\brief Virtual method implementation defining the default units configuration.
  void DefaultConfig(const std::string &unit) override;
};

#endif  // LINASIMU_MATERIALS_SVC_HH
