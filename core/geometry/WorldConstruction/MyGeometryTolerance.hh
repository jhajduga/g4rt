#ifndef M_GEOMRTRY_TOLERANCE_HH
#define M_GEOMRTRY_TOLERANCE_HH
#include "G4GeometryTolerance.hh"

class MyGeometryTolerance : public G4GeometryTolerance {
  public:
    static void ResetSurfaceTolerance(G4double worldExtent) {
      G4GeometryTolerance* baseTol = G4GeometryTolerance::GetInstance();
      MyGeometryTolerance* tol = static_cast<MyGeometryTolerance*>(baseTol);
      tol->SetSurfaceTolerance(worldExtent);
    }
  };
#endif  // M_GEOMRTRY_TOLERANCE_HH
