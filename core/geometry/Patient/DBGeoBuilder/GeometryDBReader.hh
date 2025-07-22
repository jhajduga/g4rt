#ifndef GEOMETRY_PARSER_HH
#define GEOMETRY_PARSER_HH

#include <string>
#include <vector>
#include <array>

#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <G4ThreeVector.hh>


namespace py = pybind11;


// Holds parsed geometry data for one component
struct GeometryData {
    std::string                     component;      // Component name
    std::string                     body;           // Body identifier
    G4ThreeVector                   com;            // Center of mass
    std::string                     sc_id;          // Scintilator identifier
    std::string                     mat;          // Material name
    std::vector<std::array<int,3>>  nodes;          // Node IDs
    std::vector<G4ThreeVector>      vertices;       // Vertex positions
    std::vector<G4ThreeVector>      normals;        // Vertex normals
};

// Parses geometry definitions from Excel using Python
class GeometryDBReader {
private:
    py::object m_parser;                                        // Python parser object
    std::vector<GeometryData> geoms_;                           // Parsed geometry entries

    struct CellInfo { 
        std::string sc_id; 
        G4ThreeVector com;
        CellInfo(const std::string& id, const G4ThreeVector& vec): sc_id(id), com(vec) {}
    };


    static std::vector<CellInfo> m_db_cells_positioning;        // Cells positioning data
    
    // Private constructor for singleton
    GeometryDBReader();
    ~GeometryDBReader() = default;

    public:
    // Singleton accessor
    static GeometryDBReader& Instance();

    void Finalize();

    // Deleted copy/move
    GeometryDBReader(const GeometryDBReader&) = delete;
    GeometryDBReader& operator=(const GeometryDBReader&) = delete;

    // Load and parse sheets from Excel and CSV
    void LoadDataBase( const std::string& path );
    
    std::string db_filename;
    std::string csv_filename;
    std::string sheet;
    // Access parsed geometry data
    const std::vector<GeometryData>& GetData() const { return geoms_; }

    // Access cells positioning data 
    const std::vector<CellInfo>& GetCellsPositioning() const { return m_db_cells_positioning; } 
};

#endif // GEOMETRY_PARSER_HH
