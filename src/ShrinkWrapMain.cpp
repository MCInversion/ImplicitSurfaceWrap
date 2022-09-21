
#include "pmp/SurfaceMesh.h"

#include "sdf/SDF.h"

#include "ConversionUtils.h"

#include <filesystem>
#include <chrono>

// set up root directory
const std::filesystem::path fsRootPath = DROOT_DIR;
const auto fsDataDirPath = fsRootPath / "data\\";
const auto fsDataOutPath = fsRootPath / "output\\";
const std::string dataDirPath = fsDataDirPath.string();
const std::string dataOutPath = fsDataOutPath.string();

int main()
{
    // DISCLAIMER: the names need to match the models in "DROOT_DIR/data" except for the extension (which is always *.obj)
    const std::vector<std::string> meshNames{
        "armadillo",
        "BentChair",
        "blub",
        "bunny",
        "maxPlanck",
        "nefertiti",
        "ogre",
        "spot"
    };

    constexpr unsigned int nVoxelsPerMinDimension = 50;

    for (const auto& name : meshNames)
    {
	    pmp::SurfaceMesh mesh;
	    mesh.read(dataDirPath + name + ".obj");
        const auto meshBBox = mesh.bounds();
        const auto meshBBoxSize = meshBBox.max() - meshBBox.min();
        const float minSize = std::min({ meshBBoxSize[0], meshBBoxSize[1], meshBBoxSize[2] });
		const float cellSize = minSize / nVoxelsPerMinDimension;
	    const SDF::DistanceFieldSettings sdfSettings{
	        cellSize,
	        1.0f,
	        0.2,
	        SDF::KDTreeSplitType::Center,
	        SDF::SignComputation::VoxelFloodFill,
	        SDF::BlurPostprocessingType::None,
	        SDF::PreprocessingType::Octree
	    };
		SDF::ReportInput(mesh, sdfSettings, std::cout);

	    const auto startSDF = std::chrono::high_resolution_clock::now();
	    const auto sdf = SDF::DistanceFieldGenerator::Generate(mesh, sdfSettings);
	    const auto endSDF = std::chrono::high_resolution_clock::now();

		SDF::ReportOutput(sdf, std::cout);
	    const std::chrono::duration<double> timeDiff = endSDF - startSDF;
	    std::cout << "SDF Time: " << timeDiff.count() << " s\n";
		std::cout << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n";
	    ExportToVTI(dataOutPath + name + "SDF", sdf);
    }
}