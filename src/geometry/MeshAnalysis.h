#pragma once

#include "pmp/SurfaceMesh.h"

namespace Geometry
{
	/// \brief Computes minimum internal angle per triangle averaged for each vertex over adjacent triangles
	///        & stores the values as vertex scalar data.
	///        This metric takes values from [0, M_PI / 3]. Preferred values are from [M_PI / 6, M_PI / 3].
	[[nodiscard]] bool ComputeTriangleMinAngleVertexValues(pmp::SurfaceMesh& mesh);

	/// \brief Computes maximum internal angle per triangle averaged for each vertex over adjacent triangles
	///        & stores the values as vertex scalar data.
	///        This metric takes values from [M_PI / 3, M_PI]. Preferred values are from [M_PI / 3, M_PI / 2].
	[[nodiscard]] bool ComputeTriangleMaxAngleVertexValues(pmp::SurfaceMesh& mesh);

	/// \brief Computes the condition number of each triangle's Jacobian averaged for each vertex over adjacent triangles
	///        & stores the values as vertex scalar data.
	///        This metric takes values from [1, infinity). Preferred values are from [1, 1.3].
	[[nodiscard]] bool ComputeTriangleJacobianConditionNumberVertexValues(pmp::SurfaceMesh& mesh);

	/// \brief Computes the condition number of each triangle's equilateral Jacobian averaged for each vertex over adjacent triangles
	///        & stores the values as vertex scalar data.
	///        This metric takes values from [1, infinity). Preferred values are from [1, 1.3].
	[[nodiscard]] bool ComputeEquilateralTriangleJacobianConditionNumbers(pmp::SurfaceMesh& mesh);

	/// \brief Computes the conditioning of the stiffness matrix of a spring model of triangles according to [ch. 3, Schewchuk, 2002]
	///        & stores the values as vertex scalar data.
	[[nodiscard]] bool ComputeStiffnessMatrixConditioningVertexValues(pmp::SurfaceMesh& mesh);

	/// \brief verifies whether a metric with a given name is registered.
	[[nodiscard]] bool IsMetricRegistered(const std::string& name);


	/// \brief triangle metric computation function.
	using TriMetricFunction = std::function<bool(pmp::SurfaceMesh&)>;

	/// \brief provides a metric function for a given metricName.
	[[nodiscard]] TriMetricFunction IdentifyMetricFunction(const std::string& metricName);

	/// \brief Computes dihedral angle for mesh vertices, averaged from mesh edges.
	void ComputeEdgeDihedralAngles(pmp::SurfaceMesh& mesh);

	/// \brief Computes vertex principal curvatures and mean curvature.
	void ComputeVertexCurvatures(pmp::SurfaceMesh& mesh, const float& principalCurvatureFactor = 2.0f);

	/// \brief Computes z-level elevation values for each vertex.
	void ComputeZLevelElevations(pmp::SurfaceMesh& mesh);

	/// \brief A test function for subdivision mesh counts estimation.
	/// See: "Cavarga, Mesh Primitive Counting Formula for Subdivision Surfaces, SCG 2023".
	[[nodiscard]] std::pair<std::vector<size_t>, std::vector<size_t>> GetEdgeVertCountsTheoreticalEstimate(const pmp::SurfaceMesh& mesh, const size_t& maxSubdivLevel, const bool& evalOutput = false);

	/// \brief Counts self-intersecting faces of the input mesh.
	/// \param[in] mesh               the evaluated SurfaceMesh.
	///	\param[in] setFaceProperty    if true, the evaluated mesh will be given a face property identifying the self-intersecting faces.
	///	\return the total number of faces which intersect another face.
	[[nodiscard]] size_t CountPMPSurfaceMeshSelfIntersectingFaces(pmp::SurfaceMesh& mesh, const bool& setFaceProperty = false);

	/// \brief A fast verification for the presence of self-intersecting faces of the input mesh.
	[[nodiscard]] bool PMPSurfaceMeshHasSelfIntersections(pmp::SurfaceMesh& mesh);

} // namespace Geometry