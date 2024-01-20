// Copyright 2013-2021 the Polygon Mesh Processing Library developers.
// Distributed under a MIT-style license, see LICENSE.txt for details.

#pragma once

#include <cassert>

#include "pmp/Types.h"

namespace pmp {

//! Simple class for representing a bounding box.
//! \ingroup core
class BoundingBox
{
public:
    //! Construct infinite/invalid bounding box.
    BoundingBox()
        : min_(std::numeric_limits<Scalar>::max()),
          max_(-std::numeric_limits<Scalar>::max())
    {
    }

    //! Construct from min and max points.
    BoundingBox(const Point& min, const Point& max) : min_(min), max_(max) {}

    //! Construct from a vector of points.
    explicit BoundingBox(const std::vector<Point>& pts)
        : min_(std::numeric_limits<Scalar>::max()),
	      max_(-std::numeric_limits<Scalar>::max())
    {
        for (int pId = 0; const auto& p : pts)
		{
            for (int i = 0; i < 3; ++i)
            {
                if (p[i] < min_[i])
                    min_[i] = p[i];
                if (p[i] > max_[i])
                    max_[i] = p[i];
            }

            ++pId;
        }
    }

    //! Add point to the bounding box.
    BoundingBox& operator+=(const Point& p)
    {
        for (int i = 0; i < 3; ++i)
        {
            if (p[i] < min_[i])
                min_[i] = p[i];
            if (p[i] > max_[i])
                max_[i] = p[i];
        }
        return *this;
    }

    //! Add two bounding boxes.
    BoundingBox& operator+=(const BoundingBox& bb)
    {
        for (int i = 0; i < 3; ++i)
        {
            if (bb.min_[i] < min_[i])
                min_[i] = bb.min_[i];
            if (bb.max_[i] > max_[i])
                max_[i] = bb.max_[i];
        }
        return *this;
    }

    //! Get min point.
    Point& min() { return min_; }

    //! Get max point.
    Point& max() { return max_; }

    //! Get min point.
    [[nodiscard]] const Point& min() const { return min_; }

    //! Get max point.
    [[nodiscard]] const Point& max() const { return max_; }

    //! Get center point.
    Point center() const { return 0.5f * (min_ + max_); }

    //! Indicate if the bounding box is empty.
    bool is_empty() const
    {
        return (max_[0] < min_[0] || max_[1] < min_[1] || max_[2] < min_[2]);
    }

    //! Get the size of the bounding box.
    Scalar size() const
    {
        return is_empty() ? Scalar(0.0) : distance(max_, min_);
    }

    //! Get the intersection value of this box with another.
    [[nodiscard]] bool Intersects(const BoundingBox& other) const
    {        
        if (other.max_[0] < min_[0] || other.min_[0] > max_[0])
            return false;

        if (other.max_[1] < min_[1] || other.min_[1] > max_[1])
            return false;

        return (other.max_[2] >= min_[2] && other.min_[2] <= max_[2]);
    }

    //! Get the intersection box of this box with another.
    [[nodiscard]] BoundingBox Intersect(const BoundingBox& other) const
    {
        if (!Intersects(other))
            return {}; // empty intersection

        BoundingBox result;
        for (int i = 0; i < 3; i++)
        {
	        result.min_[i] = std::fmaxf(min_[i], other.min_[i]);
	        result.max_[i] = std::fminf(max_[i], other.max_[i]);
        }
        return result;
    }

    //! Verifies whether a point is contained within this bounding box.
    [[nodiscard]] bool Contains(const Point& pt) const
    {
        if (pt[0] < min_[0])
            return false;

        if (pt[0] > max_[0])
            return false;

        if (pt[1] < min_[1])
            return false;

        if (pt[1] > max_[1])
            return false;

        if (pt[2] < min_[2])
            return false;

        return (pt[2] <= max_[2]);
    }

    //! Expand the size of the bounding box.
    void expand(const float& x, const float& y, const float& z)
    {
        assert(x >= 0.0f && y >= 0.0f && z >= 0.0f);
        min_ -= Point(x, y, z);
        max_ += Point(x, y, z);
    }

    BoundingBox& operator*= (const mat4& mat)
    {
        min_ = affine_transform(mat, min_);
        max_ = affine_transform(mat, max_);
        return *this;
    }

private:
    Point min_, max_;
};

} // namespace pmp
