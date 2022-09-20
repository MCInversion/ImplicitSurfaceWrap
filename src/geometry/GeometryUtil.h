#pragma once

#include "pmp/MatVec.h"

// forward declarations
namespace pmp
{
	class BoundingBox;
}

namespace SDF
{
	struct Ray;
}

namespace Geometry
{
	/**
	 * \brief Computes the squared distance from point to a triangle.
	 * \param vertices     list of (three) vertices of a triangle.
	 * \param point        point from which the distance is to be computed.
	 * \return squared distance from point to triangle.
	 */
	[[nodiscard]] double GetDistanceToTriangleSq(const std::vector<pmp::vec3>& vertices, const pmp::vec3& point);

	/**
	 * \brief An intersection test between a triangle and a box.
	 * \param vertices     list of (three) vertices of a triangle.
	 * \param boxCenter    center of the box to be queried.
	 * \param boxHalfSize  half-size of the box to be queried.
	 * \return true if the box intersects the triangle.
	 */
	[[nodiscard]] bool TriangleIntersectsBox(const std::vector<pmp::vec3>& vertices, const pmp::vec3& boxCenter, const pmp::vec3& boxHalfSize);

	// ======================================================================

	/// \brief a wrapper for the parameters of a ray intersecting KD-tree boxes.
	struct Ray
	{
		pmp::vec3 StartPt{};
		pmp::vec3 Direction{ 1.0f, 0.0f, 0.0f };
		pmp::vec3 InvDirection{ 1.0f, FLT_MAX, FLT_MAX };
		float ParamMin{ 0.0f };
		float ParamMax{ FLT_MAX };
		float HitParam{ FLT_MAX };

		// dir vector dimension indices:
		unsigned int kx{ 0 };
		unsigned int ky{ 0 };
		unsigned int kz{ 0 };

		// shear constants:
		float Sx{ 0.0f };
		float Sy{ 0.0f };
		float Sz{ 0.0f };

		/**
		 * \brief Constructor. Construct from startPt and direction vector as in [Woop, Benthin, Wald, 2013].
		 * \param startPt    start point of the ray.
		 * \param dir        (normalized) direction vector of the ray.
		 * \throw std::logic_error if dir is not normalized.
		 */
		Ray(const pmp::vec3& startPt, const pmp::vec3& dir);
	};

	/**
	 * \brief An intersection test between a triangle and a ray. [Moller, Trumbore, 1997].
	 * \param ray           intersecting ray (with modifiable hit param).
	 * \param triVertices   list of (three) vertices of a triangle.
	 * \return true if the ray intersects the triangle.
	 */
	[[nodiscard]] bool RayIntersectsTriangle(Ray& ray, const std::vector<pmp::vec3>& triVertices);

	/**
	 * \brief A watertight intersection test between a triangle and a ray. [Woop, Benthin, Wald, 2013].
	 * \param ray           intersecting ray (with modifiable hit param).
	 * \param triVertices   list of (three) vertices of a triangle.
	 * \return true if the ray intersects the triangle.
	 */
	[[nodiscard]] bool RayIntersectsTriangleWatertight(Ray& ray, const std::vector<pmp::vec3>& triVertices);

	/**
	 * \brief Ray-box intersection test.
	 * \param ray           intersecting ray.
	 * \param box           intersected box.
	 * \return true if the ray intersects the box.
	 */
	[[nodiscard]] bool RayIntersectsABox(const Ray& ray, const pmp::BoundingBox& box);

} // namespace Geometry