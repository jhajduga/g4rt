#!/usr/bin/env python3
import os
import sys
import argparse
from pathlib import Path
from itertools import product
from loguru import logger

# === LOGGER SETUP ===
# File logger - logs all details to a rotating log file
logger.remove()
logger.add("job_generator.log", level="DEBUG", rotation="1 MB", backtrace=True, diagnose=True)

# Global exception catcher - logs uncaught exceptions
def _handle_exception(exc_type, exc_value, exc_traceback):
    if issubclass(exc_type, KeyboardInterrupt):
        sys.__excepthook__(exc_type, exc_value, exc_traceback)
        return
    logger.opt(exception=(exc_type, exc_value, exc_traceback)).error("Uncaught exception")

sys.excepthook = _handle_exception

# Console logger setup (colorized, minimal format)
def configure_console_logger(level):
    logger.remove()
    logger.add(
        sys.stderr,
        format="<green>{time:HH:mm:ss}</green> | <level>{level:<8}</level> | {message}",
        level=level.upper(),
        colorize=True
    )

# === TEMPLATE DEFINING STRUCTURE OF A JOB FILE ===
# You can add or remove sections here depending on the simulation input requirements
BASE_TEMPLATE = """[RunSvc]
JobName = "{jobname}"
BeamType = "IAEA"
PhspInputFileName = "{phsp_path}" 
PhspEvtVrtxMultiplicityTreshold = 10
PrintProgressFrequency = 0.0005
Physics = "{physics}"
GenerateCT = false
PrimariesAnalysis = false
NTupleAnalysis = false
RunAnalysis = true

[GeoSvc]
BuildLinac = true
BuildPatient = true

[PatientGeometry]
Type = "D3DDetector"
PatientIsocentreX = 0.0
PatientIsocentreY = 0.0
PatientIsocentreZ = 0.0
EnviromentSizeX = 680.0
EnviromentSizeY = 710.0
EnviromentSizeZ = 460.0
EnviromentMedium = "Usr_G4AIR20C"
EnviromentPatientEnvelop = "{envelop}"

[GeometryBuilder]
ExcludeObjList = {exclude_list}
Position = [0.0, 0.0, 0.0]
Rotation = {rotation}

[D3DDetector_Cell]
Medium = "RMPS470"
Voxelization = [5,5,5]

[RunSvc_Plan]
nControlPoints = 1
BeamRotation = 0
nParticles = {n_particles}
PlanInputFile = ["{plan_path}"]
"""

# === JOB GENERATOR FUNCTION ===
def generate_jobs(output_dir, phsp_path, log_level="INFO"):
    configure_console_logger(log_level)

    # === PARAMETER LISTS (CARTESIAN PRODUCT) ===
    # Each of these lists represents a dimension of the parameter space.
    # To add new parameters:
    #   1. Add a new list with values.
    #   2. Add that list to the product(...) call below.
    #   3. Add a corresponding template placeholder in BASE_TEMPLATE.
    #   4. Pass the value to `.format()` with a suitable name.

    plan_paths = [
        "plan/custom/cp10x10.dat",
        "plan/custom/cp20x20.dat"
    ]

    envelops = [
        "ModularWaterPhantom_3mf",
        "SimpleBox_3mf"
    ]

    rotations = [
        [0.0, 0.0, 0.0],
        [0.0, 90.0, 0.0]
    ]

    n_particles_list = [1000, 5000]

    physics_models = ["LowE_Livermore", "LowE_Penelope"]

    exclude_lists = [
        [],
        ["WaterPhantom v8|Cap v3:1"],
        ["WaterPhantom v8|Cap v3:1", "WaterPhantom v8|Cap v3:2"]
    ]

    # Create output directory if it doesn't exist
    output_path = Path(output_dir)
    output_path.mkdir(parents=True, exist_ok=True)
    logger.info(f"Saving job files to: {output_path.resolve()}")

    try:
        # Create Cartesian product of all parameter options
        for i, (plan_path, envelop, rotation, n_particles, physics, exclude) in enumerate(
            product(plan_paths, envelops, rotations, n_particles_list, physics_models, exclude_lists)
        ):
            jobname = f"job_{i:03d}"

            # Format the ExcludeObjList TOML array
            exclude_str = "[\n" + "\n".join(f'    "{e}",' for e in exclude) + "\n]" if exclude else "[]"

            # Fill in template
            content = BASE_TEMPLATE.format(
                jobname=jobname,
                phsp_path=phsp_path,
                physics=physics,
                envelop=envelop,
                exclude_list=exclude_str,
                rotation=str(rotation),
                n_particles=n_particles,
                plan_path=plan_path
            )

            # Save to file
            file_path = output_path / f"{jobname}.toml"
            file_path.write_text(content)
            logger.debug(f"Wrote job file: {file_path.name}")
        
        logger.success(f"Generated {i+1} job files.")

    except Exception as e:
        logger.exception("Job generation failed.")


# === ARGUMENT PARSER ===
def main():
    parser = argparse.ArgumentParser(description="Generate .toml job files for G4RT-like simulations.")
    parser.add_argument(
        "-o", "--output", type=str, required=True,
        help="Output directory for generated jobs"
    )
    parser.add_argument(
        "-p", "--phsp", type=str, required=True,
        help="Path to PHSP input file (same for all jobs)"
    )
    parser.add_argument(
        "-l", "--loglevel", type=str, default="INFO",
        help="Console log level (DEBUG, INFO, WARNING, etc.)"
    )

    args = parser.parse_args()
    generate_jobs(output_dir=args.output, phsp_path=args.phsp, log_level=args.loglevel)

if __name__ == "__main__":
    main()
