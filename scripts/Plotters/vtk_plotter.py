import os
from typing import Dict

import numpy as np
import vtk
from vtk.util import numpy_support
from loguru import logger
import polars as pl  # Upewnij się, że masz zainstalowaną bibliotekę polars

# Konfiguracja loguru
logger.add("debug.log", level="DEBUG", rotation="10 MB", compression="zip", backtrace=True, diagnose=True)


def create_vtk_image_data(cell_df: pl.DataFrame, voxel_side_len: float) -> vtk.vtkImageData:
    """
    Tworzy obiekt vtkImageData na podstawie danych zawartych w cell_df.
    Jeśli dostępna jest kolumna "NormalizedDose", używa jej do wypełnienia voxelów.
    """
    logger.info("Tworzenie vtkImageData...")
    cell_df = cell_df.sort(['Y [mm]'])

    # Znajdowanie zakresów współrzędnych
    x_min: float = cell_df["X [mm]"].min()
    x_max: float = cell_df["X [mm]"].max()
    y_min: float = cell_df["Y [mm]"].min()
    y_max: float = cell_df["Y [mm]"].max()
    z_min: float = cell_df["Z [mm]"].min()
    z_max: float = cell_df["Z [mm]"].max()
    
    print(x_min, x_max, y_min, y_max, z_min, z_max)

    # Obliczanie wymiarów siatki (dodanie 1, aby uwzględnić ostatni voxel)
    x_dim: int = int((x_max - x_min) / voxel_side_len) + 1
    y_dim: int = int((y_max - y_min) / voxel_side_len) + 1
    z_dim: int = int((z_max - z_min) / voxel_side_len) + 1
    logger.info(f"Wymiary siatki: x_dim={x_dim}, y_dim={y_dim}, z_dim={z_dim}")

    # Tworzenie vtkImageData
    imageData: vtk.vtkImageData = vtk.vtkImageData()
    imageData.SetDimensions(x_dim, y_dim, z_dim)
    imageData.SetSpacing(voxel_side_len, voxel_side_len, voxel_side_len)
    imageData.SetOrigin(x_min - voxel_side_len / 2, y_min - voxel_side_len / 2, z_min - voxel_side_len / 2)
    imageData.AllocateScalars(vtk.VTK_FLOAT, 1)

    # Przygotowanie macierzy scalar_data
    scalar_data: np.ndarray = np.zeros((z_dim, y_dim, x_dim), dtype=np.float32)

    # Pobieramy kolumny jako tablice NumPy
    x_arr: np.ndarray = cell_df["X [mm]"].to_numpy()
    y_arr: np.ndarray = cell_df["Y [mm]"].to_numpy()
    z_arr: np.ndarray = cell_df["Z [mm]"].to_numpy()
    # Używamy znormalizowanej dawki, jeśli jest dostępna
    if "NormalizedDose" in cell_df.columns:
        logger.info("Używanie znormalizowanej dawki do wypełnienia voxelów.")
        dose_arr: np.ndarray = cell_df["NormalizedDose"].to_numpy().astype(np.float32)
    else:
        logger.info("Używanie oryginalnych wartości dawki do wypełnienia voxelów.")
        dose_arr: np.ndarray = cell_df["Dose [Gy]"].to_numpy().astype(np.float32)

    # Wektoryzowane obliczenie indeksów voxelowych
    x_idx: np.ndarray = ((x_arr - x_min) / voxel_side_len).astype(np.int32)
    y_idx: np.ndarray = ((y_arr - y_min) / voxel_side_len).astype(np.int32)
    z_idx: np.ndarray = ((z_arr - z_min) / voxel_side_len).astype(np.int32)

    # Maskowanie indeksów poza zakresem
    valid_mask: np.ndarray = (
        (x_idx >= 0) & (x_idx < x_dim) &
        (y_idx >= 0) & (y_idx < y_dim) &
        (z_idx >= 0) & (z_idx < z_dim)
    )
    if not np.all(valid_mask):
        logger.warning("Niektóre obliczone indeksy voxelowe są poza zakresem.")

    # Wypełnienie macierzy – w przypadku kolizji przyjmujemy ostatnią wartość
    scalar_data[z_idx[valid_mask], y_idx[valid_mask], x_idx[valid_mask]] = dose_arr[valid_mask]

    # Konwersja macierzy NumPy na vtkDataArray
    vtk_data_array = numpy_support.numpy_to_vtk(scalar_data.ravel(), deep=True, array_type=vtk.VTK_FLOAT)
    imageData.GetPointData().SetScalars(vtk_data_array)

    logger.info("vtkImageData utworzone.")
    return imageData


def main() -> None:
    csv_path: str = '/home/jackie/work/dose3d/g4rt/output/imrt_test_3/sim/cp10x10/cp10x10_ct_dose_voxel.csv'
    if not os.path.exists(csv_path):
        logger.error(f"Plik CSV nie znaleziony w {csv_path}")
        return

    logger.info("Wczytywanie pliku CSV...")

    # Definicja typów kolumn przy wczytywaniu (używamy nowego argumentu schema_overrides)
    schema_overrides: Dict[str, type] = {
        "X [mm]": pl.Float64,
        "Y [mm]": pl.Float64,
        "Z [mm]": pl.Float64,
        "Dose [Gy]": pl.Float64,
    }

    # Wczytujemy tylko potrzebne kolumny
    cell_df: pl.DataFrame = pl.read_csv(
        csv_path,
        columns=["X [mm]", "Y [mm]", "Z [mm]", "Dose [Gy]"],
        schema_overrides=schema_overrides,
        infer_schema_length=10000,
    )
    logger.info(f"Liczba wierszy w CSV: {cell_df.height}")

    if cell_df.is_empty():
        logger.error("DataFrame jest pusty. Kończenie programu.")
        return

    voxel_side_len: float = 1.0

    # Normalizacja dawki do przedziału [0, 1]
    dose_min: float = cell_df["Dose [Gy]"].min()
    dose_max: float = cell_df["Dose [Gy]"].max()

    if dose_min == dose_max:
        logger.warning("Wszystkie wartości dawki są identyczne. Normalizacja ustawiona na 0.")
        cell_df = cell_df.with_columns(pl.lit(0).alias("NormalizedDose"))
    else:
        normalized = (cell_df["Dose [Gy]"] - dose_min) / (dose_max - dose_min)
        cell_df = cell_df.with_columns(normalized.alias("NormalizedDose"))
        logger.info(f"Dawki znormalizowane: min={dose_min}, max={dose_max}")

    # Tworzenie danych voxelowych
    imageData: vtk.vtkImageData = create_vtk_image_data(cell_df, voxel_side_len)

    # Konfiguracja mappera i aktora (Volume)
    mapper: vtk.vtkFixedPointVolumeRayCastMapper = vtk.vtkFixedPointVolumeRayCastMapper()
    mapper.SetInputData(imageData)

    volume: vtk.vtkVolume = vtk.vtkVolume()
    volume.SetMapper(mapper)

    # Definicja transfer function dla kolorów i przezroczystości (zakładając zakres [0, 1])
    colorFunc: vtk.vtkColorTransferFunction = vtk.vtkColorTransferFunction()
    colorFunc.AddRGBPoint(0.0, 0.0, 0.0, 1.0)   # niebieski dla zera
    colorFunc.AddRGBPoint(0.5, 0.0, 1.0, 0.0)   # zielony dla połowy zakresu
    colorFunc.AddRGBPoint(1.0, 1.0, 0.0, 0.0)   # czerwony dla maksimum

    opacityFunc: vtk.vtkPiecewiseFunction = vtk.vtkPiecewiseFunction()
    opacityFunc.AddPoint(0.0, 0.0)  # pełna przezroczystość dla 0
    opacityFunc.AddPoint(1.0, 1.0)  # pełna nieprzezroczystość dla 1

    volume.GetProperty().SetColor(colorFunc)
    volume.GetProperty().SetScalarOpacity(opacityFunc)
    volume.GetProperty().SetInterpolationTypeToNearest()
    # Opcjonalnie: ustawienie jednostkowej odległości dla opacity (może pomóc w widoczności)
    # volume.GetProperty().SetScalarOpacityUnitDistance(0.1)

    # Konfiguracja renderera i okna renderowania
    renderer: vtk.vtkRenderer = vtk.vtkRenderer()
    renderer.AddVolume(volume)
    renderer.SetBackground(1, 1, 1)

    axes: vtk.vtkAxesActor = vtk.vtkAxesActor()
    axes.SetTotalLength(10, 10, 10)
    renderer.AddActor(axes)

    renderWindow: vtk.vtkRenderWindow = vtk.vtkRenderWindow()
    renderWindow.AddRenderer(renderer)
    renderWindowInteractor: vtk.vtkRenderWindowInteractor = vtk.vtkRenderWindowInteractor()
    renderWindowInteractor.SetRenderWindow(renderWindow)

    renderWindow.Render()
    renderWindowInteractor.Start()


if __name__ == "__main__":
    main()
