#include "Grid.h"

namespace Geometry
{
	/// \brief default value to initialize a scalar grid with.
	constexpr double DEFAULT_SCALAR_GRID_INIT_VAL = 1e+9;
	/// \brief default value to initialize a vector grid with.
	constexpr double DEFAULT_VECTOR_GRID_INIT_VAL = 0.0;

	ScalarGrid::ScalarGrid(const float& cellSize, const pmp::BoundingBox& box)
		: m_CellSize(cellSize)
	{
		// compute global grid bounds
		const int nXMinus = std::floor(box.min()[0] / m_CellSize);
		const int nXPlus = std::ceil(box.max()[0] / m_CellSize);

		const int nYMinus = std::floor(box.min()[1] / m_CellSize);
		const int nYPlus = std::ceil(box.max()[1] / m_CellSize);

		const int nZMinus = std::floor(box.min()[2] / m_CellSize);
		const int nZPlus = std::ceil(box.max()[2] / m_CellSize);

		// adjust box
		const auto minVec = pmp::vec3(nXMinus, nYMinus, nZMinus) * m_CellSize;
		const auto maxVec = pmp::vec3(nXPlus, nYPlus, nZPlus) * m_CellSize;
		m_Box = pmp::BoundingBox(minVec, maxVec);

		m_Dimensions = GridDimensions{
			static_cast<size_t>(nXPlus - nXMinus),
			static_cast<size_t>(nYPlus - nYMinus),
			static_cast<size_t>(nZPlus - nZMinus)
		};

		const size_t nValues = m_Dimensions.Nx * m_Dimensions.Ny * m_Dimensions.Nz;
		m_Values = std::vector(nValues, DEFAULT_SCALAR_GRID_INIT_VAL);
		m_FrozenValues = std::vector(nValues, false);
	}

	ScalarGrid::ScalarGrid(const float& cellSize, const pmp::BoundingBox& box, const double& initVal)
		: m_CellSize(cellSize)
	{
		// compute global grid bounds
		const int nXMinus = std::floor(box.min()[0] / m_CellSize);
		const int nXPlus = std::ceil(box.max()[0] / m_CellSize);

		const int nYMinus = std::floor(box.min()[1] / m_CellSize);
		const int nYPlus = std::ceil(box.max()[1] / m_CellSize);

		const int nZMinus = std::floor(box.min()[2] / m_CellSize);
		const int nZPlus = std::ceil(box.max()[2] / m_CellSize);

		// adjust box
		const auto maxVec = pmp::vec3(nXPlus, nYPlus, nZPlus) * m_CellSize;
		const auto minVec = pmp::vec3(nXMinus, nYMinus, nZMinus) * m_CellSize;
		m_Box = pmp::BoundingBox(minVec, maxVec);

		m_Dimensions = GridDimensions{
			static_cast<size_t>(nXPlus - nXMinus),
			static_cast<size_t>(nYPlus - nYMinus),
			static_cast<size_t>(nZPlus - nZMinus)
		};

		const size_t nValues = m_Dimensions.Nx * m_Dimensions.Ny * m_Dimensions.Nz;
		m_Values = std::vector(nValues, initVal);
		m_FrozenValues = std::vector(nValues, false);
	}

	VectorGrid::VectorGrid(const float& cellSize, const pmp::BoundingBox& box)
	{
		// compute global grid bounds
		const int nXMinus = std::floor(box.min()[0] / m_CellSize);
		const int nXPlus = std::ceil(box.max()[0] / m_CellSize);

		const int nYMinus = std::floor(box.min()[1] / m_CellSize);
		const int nYPlus = std::ceil(box.max()[1] / m_CellSize);

		const int nZMinus = std::floor(box.min()[2] / m_CellSize);
		const int nZPlus = std::ceil(box.max()[2] / m_CellSize);

		// adjust box
		const auto minVec = pmp::vec3(nXMinus, nYMinus, nZMinus) * m_CellSize;
		const auto maxVec = pmp::vec3(nXPlus, nYPlus, nZPlus) * m_CellSize;
		m_Box = pmp::BoundingBox(minVec, maxVec);

		m_Dimensions = GridDimensions{
			static_cast<size_t>(nXPlus - nXMinus),
			static_cast<size_t>(nYPlus - nYMinus),
			static_cast<size_t>(nZPlus - nZMinus)
		};

		const size_t nValues = m_Dimensions.Nx * m_Dimensions.Ny * m_Dimensions.Nz;
		m_ValuesX = std::vector(nValues, DEFAULT_VECTOR_GRID_INIT_VAL);
		m_ValuesY = std::vector(nValues, DEFAULT_VECTOR_GRID_INIT_VAL);
		m_ValuesZ = std::vector(nValues, DEFAULT_VECTOR_GRID_INIT_VAL);
		m_FrozenValues = std::vector(nValues, false);
	}

	VectorGrid::VectorGrid(const float& cellSize, const pmp::BoundingBox& box, const pmp::vec3& initVal)
	{
		// compute global grid bounds
		const int nXMinus = std::floor(box.min()[0] / m_CellSize);
		const int nXPlus = std::ceil(box.max()[0] / m_CellSize);

		const int nYMinus = std::floor(box.min()[1] / m_CellSize);
		const int nYPlus = std::ceil(box.max()[1] / m_CellSize);

		const int nZMinus = std::floor(box.min()[2] / m_CellSize);
		const int nZPlus = std::ceil(box.max()[2] / m_CellSize);

		// adjust box
		const auto maxVec = pmp::vec3(nXPlus, nYPlus, nZPlus) * m_CellSize;
		const auto minVec = pmp::vec3(nXMinus, nYMinus, nZMinus) * m_CellSize;
		m_Box = pmp::BoundingBox(minVec, maxVec);

		m_Dimensions = GridDimensions{
			static_cast<size_t>(nXPlus - nXMinus),
			static_cast<size_t>(nYPlus - nYMinus),
			static_cast<size_t>(nZPlus - nZMinus)
		};

		const size_t nValues = m_Dimensions.Nx * m_Dimensions.Ny * m_Dimensions.Nz;
		m_ValuesX = std::vector(nValues, static_cast<double>(initVal[0]));
		m_ValuesY = std::vector(nValues, static_cast<double>(initVal[1]));
		m_ValuesZ = std::vector(nValues, static_cast<double>(initVal[2]));
		m_FrozenValues = std::vector(nValues, false);
	}
} // namespace Geometry
