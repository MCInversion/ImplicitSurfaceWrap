//=============================================================================
// Copyright (C) 2017-2019 The pmp-library developers
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
// =============================================================================

#include "gtest/gtest.h"

#include <pmp/algorithms/SurfaceSimplification.h>
#include <pmp/algorithms/SurfaceFeatures.h>

using namespace pmp;

class SurfaceSimplificationTest : public ::testing::Test
{
public:
    SurfaceSimplificationTest()
    {
        EXPECT_TRUE(mesh.read("pmp-data/off/icosahedron_subdiv.off"));
    }
    SurfaceMesh mesh;
};

// plain simplification test
TEST_F(SurfaceSimplificationTest, simplification)
{
    mesh.clear();
    mesh.read("pmp-data/off/bunny_adaptive.off");
    SurfaceSimplification ss(mesh);
    ss.initialize(5, // aspect ratio
                  0.01, // edge length
                  10, // max valence
                  10, // normal deviation
                  0.001); // Hausdorff
    ss.simplify(mesh.n_vertices() * 0.1);
    EXPECT_EQ(mesh.n_vertices(),size_t(3800));
    EXPECT_EQ(mesh.n_faces(),size_t(7596));
}

// simplify with feature edge preservation enabled
TEST_F(SurfaceSimplificationTest, simplification_with_features)
{
    SurfaceFeatures sf(mesh);
    sf.detect_angle(25);

    SurfaceSimplification ss(mesh);
    ss.initialize(5); // aspect ratio
    ss.simplify(mesh.n_vertices() * 0.1);
    EXPECT_EQ(mesh.n_vertices(),size_t(64));
}