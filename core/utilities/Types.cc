#include "Types.hh"


std::string Scoring::to_string(Scoring::Type type){
    switch (type){
      case Type::Cell:
        return "Cell";
        break;
      case Type::Voxel:
        return "Voxel";
        break;
      default:
        return std::string();
        break;
    }
  }