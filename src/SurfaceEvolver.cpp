#include "SurfaceEvolver.h"

#include <Eigen/Dense>
#include <Eigen/Sparse>

#include "pmp/algorithms/Remeshing.h"
#include "pmp/algorithms/Normals.h"
#include "pmp/algorithms/DifferentialGeometry.h"

#include "geometry/GridUtil.h"
#include "geometry/IcoSphereBuilder.h"

/// \brief a magic multiplier computing the radius of an ico-sphere that fits into the field's box.
constexpr float ICO_SPHERE_RADIUS_FACTOR = 0.4f;

constexpr unsigned int N_ICO_VERTS_0 = 12; // number of vertices in an icosahedron.
constexpr unsigned int N_ICO_EDGES_0 = 30; // number of edges in an icosahedron.

/**
 * \brief Computes scaling factor for stabilizing the finite volume method on assumed spherical surface meshes based on time step.
 * \param timeStep               time step size.
 * \param icoRadius              radius of an evolving geodesic icosahedron.
 * \param icoSubdiv              subdivision level of an evolving geodesic icosahedron.
 * \param stabilizationFactor    a multiplier for stabilizing mean co-volume area.
 * \return scaling factor for mesh and scalar grid.
 */
[[nodiscard]] float GetStabilizationScalingFactor(const double& timeStep, const float& icoRadius, const unsigned int& icoSubdiv, const float& stabilizationFactor = 1.0f)
{
	const unsigned int expectedVertexCount = (N_ICO_EDGES_0 * static_cast<unsigned int>(pow(4, icoSubdiv) - 1) + 3 * N_ICO_VERTS_0) / 3;
	const float expectedMeanCoVolArea = stabilizationFactor * (4.0f * static_cast<float>(M_PI) * icoRadius * icoRadius / static_cast<float>(expectedVertexCount));
	return pow(static_cast<float>(timeStep) / expectedMeanCoVolArea, 1.0f / 3.0f);
}

void SurfaceEvolver::Preprocess()
{
	// prepare dimensions & origin
	const auto& fieldBox = m_Field->Box();
	const auto origin = fieldBox.center();
	const auto fieldBoxSize = fieldBox.max() - fieldBox.min();
	const float minDim = std::min({ fieldBoxSize[0], fieldBoxSize[1], fieldBoxSize[2] });

	// build ico-sphere
	const float icoSphereRadius = ICO_SPHERE_RADIUS_FACTOR * minDim;
	const unsigned int icoSphereSubdiv = m_EvolSettings.IcoSphereSubdivisionLevel;
	Geometry::IcoSphereBuilder icoBuilder({ m_EvolSettings.IcoSphereSubdivisionLevel, icoSphereRadius });
	icoBuilder.BuildBaseData();
	icoBuilder.BuildPMPSurfaceMesh();
	m_EvolvingSurface = std::make_shared<pmp::SurfaceMesh>(icoBuilder.GetPMPSurfaceMeshResult());

	// transform mesh and grid
	// >>> uniform scale to ensure numerical method's stability.
	// >>> translation to origin for fields not centered at (0,0,0).
	const float scalingFactor = GetStabilizationScalingFactor(m_EvolSettings.TimeStep, icoSphereRadius, icoSphereSubdiv);
	pmp::mat4 transfMatrix{
		scalingFactor, 0.0f, 0.0f, -origin[0],
		0.0f, scalingFactor, 0.0f, -origin[1],
		0.0f, 0.0f, scalingFactor, -origin[2],
		0.0f, 0.0f, 0.0f, 1.0f
	};
	m_TransformToOriginal = inverse(transfMatrix);

	(*m_EvolvingSurface) *= transfMatrix;
	(*m_Field) *= transfMatrix;
}

/// \brief identifier for sparse matrix.
using SparseMatrix = Eigen::SparseMatrix<double>;

/// \brief A utility for converting Eigen::ComputationInfo to a string message.
[[nodiscard]] std::string InterpretSolverErrorCode(const Eigen::ComputationInfo& cInfo)
{
	if (cInfo == Eigen::Success)
		return "Eigen::Success";

	if (cInfo == Eigen::NumericalIssue)
		return "Eigen::NumericalIssue";

	if (cInfo == Eigen::NoConvergence)
		return "Eigen::NoConvergence";

	return "Eigen::InvalidInput";
}

/**
 * \brief Weight function for Laplacian flow term, inspired by [Huska, Medla, Mikula, Morigi 2021]. 
 * \param distanceAtVertex          the value of distance from evolving mesh vertex to target mesh.
 * \return weight function value.
 */
[[nodiscard]] double LaplacianDistanceWeightFunction(const double& distanceAtVertex)
{
	return (1.0 - exp(-(distanceAtVertex * distanceAtVertex)));
}

/// \brief tolerance value for point norm.
constexpr float NORM_EPSILON = 1e-6f;

/**
 * \brief Weight function for advection flow term, inspired by [Huska, Medla, Mikula, Morigi 2021].
 * \param distanceAtVertex          the value of distance from evolving mesh vertex to target mesh.
 * \param negDistanceGradient       negative gradient vector of distance field at vertex position.
 * \param vertexNormal              unit normal to vertex.
 * \return weight function value.
 */
[[nodiscard]] double AdvectionDistanceWeightFunction(const double& distanceAtVertex, 
	const pmp::dvec3& negDistanceGradient, const pmp::Point& vertexNormal)
{
	assert(std::fabs(pmp::norm(vertexNormal) - 1.0f) < NORM_EPSILON);
	const auto negGradDotNormal = pmp::ddot(negDistanceGradient, vertexNormal);
	return distanceAtVertex * (negGradDotNormal - sqrt(1.0 - negGradDotNormal * negGradDotNormal));
}

// ================================================================================================

#define REPORT_EVOL_STEPS true // Note: may affect performance

void SurfaceEvolver::Evolve()
{
	if (!m_Field)
		throw std::invalid_argument("SurfaceEvolver::Evolve: m_Field not set! Terminating!\n");
	if (!m_Field->IsValid())
		throw std::invalid_argument("SurfaceEvolver::Evolve: m_Field is invalid! Terminating!\n");

	Preprocess();

	if (!m_EvolvingSurface)
		throw std::invalid_argument("SurfaceEvolver::Evolve: m_EvolvingSurface not set! Terminating!\n");

#if REPORT_EVOL_STEPS
	const auto bds = m_EvolvingSurface->bounds();
	std::cout << "IcoSphere Bounds Size: {"
		<< (bds.max()[0] - bds.min()[0]) << ", "
		<< (bds.max()[1] - bds.min()[1]) << ", "
		<< (bds.max()[2] - bds.min()[2]) << "},\n";
#endif

	const auto fieldNegGradient = Geometry::ComputeNormalizedNegativeGradient(*m_Field);

	const auto& NSteps = m_EvolSettings.NSteps;
	const auto& tStep = m_EvolSettings.TimeStep;
	const auto minEdgeLength = static_cast<pmp::Scalar>(sqrt(tStep));
	const auto maxEdgeLength = 5 * minEdgeLength;

	auto NVertices = static_cast<unsigned int>(m_EvolvingSurface->n_vertices());
	SparseMatrix sysMat(NVertices, NVertices);
	Eigen::MatrixXd sysRhs(NVertices, 3);

	// property container for surface vertex normals
	pmp::VertexProperty<pmp::Point> vNormalsProp{};

	// ----------- System fill function --------------------------------
	const auto fillMatrixAndRHSTriplesFromMesh = [&]()
	{
		for (const auto v : m_EvolvingSurface->vertices())
		{
			const auto vPos = m_EvolvingSurface->position(v);

			const double vDistanceToTarget = Geometry::TrilinearInterpolateScalarValue(vPos, *m_Field);
			const auto vNegGradDistanceToTarget = Geometry::TrilinearInterpolateVectorValue(vPos, fieldNegGradient);

			const auto vNormal = vNormalsProp[v]; // vertex unit normal

			const double epsilonCtrlWeight = LaplacianDistanceWeightFunction(vDistanceToTarget - m_EvolSettings.FieldIsoLevel);
			const double etaCtrlWeight = AdvectionDistanceWeightFunction(vDistanceToTarget - m_EvolSettings.FieldIsoLevel, vNegGradDistanceToTarget, vNormal);

			const auto vPosToUpdate = m_EvolvingSurface->position(v);

			const Eigen::Vector3d vertexRhs = vPosToUpdate + tStep * etaCtrlWeight * vNormal /* + tStep * tanRedistWeight * vTanVelocity */;
			sysRhs.row(v.idx()) = vertexRhs;

			const auto laplaceWeightInfo = pmp::laplace_implicit(*m_EvolvingSurface, v); // Laplacian weights
			sysMat.coeffRef(v.idx(), v.idx()) = 1.0 + tStep * epsilonCtrlWeight * static_cast<double>(laplaceWeightInfo.weightSum);

			for (const auto& [w, weight] : laplaceWeightInfo.vertexWeights)
			{
				sysMat.coeffRef(v.idx(), w.idx()) = -1.0 * tStep * epsilonCtrlWeight * static_cast<double>(weight);
			}
		}
	};
	// -----------------------------------------------------------------

	// main loop
	for (unsigned int ti = 0; ti < NSteps; ti++)
	{
#if REPORT_EVOL_STEPS
		std::cout << "time step id: " << ti << "/" << NSteps << ", time: " << tStep * ti << "/" << tStep * NSteps << "\n";
		std::cout << "pmp::Normals::compute_vertex_normals ... ";
#endif
		pmp::Normals::compute_vertex_normals(*m_EvolvingSurface);
		vNormalsProp = m_EvolvingSurface->vertex_property<pmp::Point>("v:normal");
#if REPORT_EVOL_STEPS
		std::cout << "done\n";
		std::cout << "fillMatrixAndRHSTriplesFromMesh for " << NVertices << " vertices ... ";
#endif

		// prepare matrix & rhs
		fillMatrixAndRHSTriplesFromMesh();

#if REPORT_EVOL_STEPS
		std::cout << "done\n";
		std::cout << "Solving linear system ... ";
#endif
		// solve
		Eigen::SimplicialLDLT solver(sysMat);
		Eigen::MatrixXd x = solver.solve(sysRhs);
		if (solver.info() != Eigen::Success)
		{
			const std::string msg = "SurfaceEvolver::Evolve: solver.info() != Eigen::Success for time step id: "
				+ std::to_string(ti) + ", Error code: " + InterpretSolverErrorCode(solver.info()) + "\n";
			throw std::runtime_error(msg);
		}
#if REPORT_EVOL_STEPS
		std::cout << "done\n";
		std::cout << "Updating vertex positions ... ";
#endif

		// update vertex positions
		for (unsigned int i = 0; i < NVertices; i++)
		{
			m_EvolvingSurface->position(pmp::Vertex(i)) = x.row(i);
		}

#if REPORT_EVOL_STEPS
		std::cout << "done\n";
		std::cout << "pmp::Remeshing::adaptive_remeshing(minEdgeLength: " << minEdgeLength << ", maxEdgeLength: " << maxEdgeLength << ") ... ";
#endif

		// remeshing
		pmp::Remeshing remeshing(*m_EvolvingSurface);
		remeshing.adaptive_remeshing(minEdgeLength, maxEdgeLength, minEdgeLength);

		if (m_EvolSettings.ExportSurfacePerTimeStep)
		{
			m_EvolvingSurface->write(m_EvolSettings.OutputPath + "Evol_" + std::to_string(ti) + ".obj");
		}

		// update linear system dims:
		if (ti < NSteps - 1)
		{
			NVertices = static_cast<unsigned int>(m_EvolvingSurface->n_vertices());
			sysMat = SparseMatrix(NVertices, NVertices);
			sysRhs = Eigen::MatrixXd(NVertices, 3);
		}

#if REPORT_EVOL_STEPS
		std::cout << "done\n";
		std::cout << ">>> Time step " << ti << " finished.\n";
		std::cout << "----------------------------------------------------------------------\n";
#endif

	} // end main loop


}

void ReportInput(const SurfaceEvolutionSettings& evolSettings, std::ostream& os)
{
	os << "======================================================================\n";
	os << "> > > > > > > > > > Initiating SurfaceEvolver: < < < < < < < < < < < <\n";
	os << "NSteps: " << evolSettings.NSteps << ",\n";
	os << "TimeStep: " << evolSettings.TimeStep << ",\n";
	os << "FieldIsoLevel: " << evolSettings.FieldIsoLevel << ",\n";
	os << "IcoSphereSubdivisionLevel: " << evolSettings.IcoSphereSubdivisionLevel << ",\n";
	os << "----------------------------------------------------------------------\n";
}
