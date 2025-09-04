#ifndef M_GEOMRTRY_TOLERANCE_HH
#define M_GEOMRTRY_TOLERANCE_HH
#include "G4GeometryTolerance.hh"

class MyGeometryTolerance : public G4GeometryTolerance {
  public:
    /**
     * @brief Set the global surface tolerance from a world-extent value.
     *
     * Updates the global G4GeometryTolerance singleton by setting its surface tolerance
     * using the provided worldExtent. This affects geometry tolerance calculations globally.
     *
     * @param worldExtent World extent used to compute and set the surface tolerance.
     *
     * @note This function assumes the global geometry-tolerance singleton is an instance
     * of MyGeometryTolerance and performs a direct downcast without runtime checks.
     * Calling this when the singleton is absent or of a different type is undefined behavior.
     */
    static void ResetSurfaceTolerance(G4double worldExtent) {
      G4GeometryTolerance* baseTol = G4GeometryTolerance::GetInstance();
      MyGeometryTolerance* tol = static_cast<MyGeometryTolerance*>(baseTol);
      tol->SetSurfaceTolerance(worldExtent);
    }
  };
#endif  // M_GEOMRTRY_TOLERANCE_HH
