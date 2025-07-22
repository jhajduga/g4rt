#ifndef M_GEOMRTRY_TOLERANCE_HH
#define M_GEOMRTRY_TOLERANCE_HH
#include "G4GeometryTolerance.hh"

class MyGeometryTolerance : public G4GeometryTolerance {
  public:
    /**
     * @brief Resets the global surface tolerance based on the specified world extent.
     *
     * Updates the singleton geometry tolerance instance with a new surface tolerance value derived from the provided world extent.
     *
     * @param worldExtent The extent of the world geometry used to set the new surface tolerance.
     */
    static void ResetSurfaceTolerance(G4double worldExtent) {
      G4GeometryTolerance* baseTol = G4GeometryTolerance::GetInstance();
      MyGeometryTolerance* tol = static_cast<MyGeometryTolerance*>(baseTol);
      tol->SetSurfaceTolerance(worldExtent);
    }
  };
#endif  // M_GEOMRTRY_TOLERANCE_HH
