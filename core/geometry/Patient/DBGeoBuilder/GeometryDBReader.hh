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
        /**
 * @brief Initialize a CellInfo with a scintillator identifier and its center of mass.
 *
 * Constructs a CellInfo by copying the provided scintillator ID and center-of-mass vector into
 * the struct's members.
 *
 * @param id Scintillator identifier.
 * @param vec Center-of-mass position as a G4ThreeVector.
 */
CellInfo(const std::string& id, const G4ThreeVector& vec): sc_id(id), com(vec) {}
    };


    static std::vector<CellInfo> m_db_cells_positioning;        // Cells positioning data
    
    // Private constructor for singleton
    GeometryDBReader();
    /**
 * @brief Destroys the GeometryDBReader instance.
 *
 * Cleans up resources associated with the GeometryDBReader singleton.
 */
~GeometryDBReader() = default;

    public:
    // Singleton accessor
    static GeometryDBReader& Instance();

    void Finalize();

    /**
 * @brief Copy constructor is deleted to enforce singleton behavior.
 */
    GeometryDBReader(const GeometryDBReader&) = delete;
    /**
 * @brief Deleted copy assignment operator to enforce singleton behavior.
 *
 * Prevents copying of the GeometryDBReader instance.
 */
GeometryDBReader& operator=(const GeometryDBReader&) = delete;

    // Load and parse sheets from Excel and CSV
    void LoadDataBase( const std::string& path );
    
    std::string db_filename;
    std::string csv_filename;
    std::string sheet;
    /**
 * @brief Access the parsed geometry entries.
 *
 * Returns a const reference to the internal vector of GeometryData populated by LoadDataBase().
 * The reference points to the singleton's internal storage and remains valid until the
 * GeometryDBReader is finalized or destroyed.
 *
 * @return const std::vector<GeometryData>& Const reference to the parsed geometry data.
 */
    const std::vector<GeometryData>& GetData() const { return geoms_; }

    /**
 * @brief Returns the collection of cell positioning data.
 *
 * @return Reference to a vector of CellInfo structs containing scintillator IDs and center of mass positions for each cell.
 */
    const std::vector<CellInfo>& GetCellsPositioning() const { return m_db_cells_positioning; } 
};

#endif // GEOMETRY_PARSER_HH
