#include "GeometryConversionUtils.h"

#include "utils/StringUtils.h"

#include <set>
#include <fstream>
#include <numeric>
#include <random>
#include <thread>

#ifdef _WINDOWS
// Windows-specific headers
#include <windows.h>
#include <fcntl.h>
#else
// Unsupported platform
#error "Unsupported platform"
#endif

// TODO: different representations, e.g.: ProgressiveMeshData which would then be exported into a disk file

namespace
{
	/**
	 * \brief a thread-specific wrapper for mesh data.
	 */
	struct ChunkData
	{
		std::vector<pmp::vec3> Vertices;
		std::vector<pmp::vec3> VertexNormals;
		std::vector<std::vector<unsigned int>> PolyIndices;
	};

	/**
	 * \brief Parses a chunk of OBJ data. This function is run for each thread.
	 * \param start     start address for this thread.
	 * \param end       end address for this thread.
	 * \param data      preallocated chunk data.
	 */
	void ParseChunk(const char* start, const char* end, ChunkData& data)
	{
		const char* cursor = start;

		while (cursor < end)
		{
			// If the current line is incomplete, skip to the next line
			if (*cursor == '\n')
			{
				cursor++;
				continue;
			}

			// If it's a vertex or normal, parse the three floats
			if (strncmp(cursor, "v ", 2) == 0 || strncmp(cursor, "vn ", 3) == 0)
			{
				const bool isNormal = strncmp(cursor, "vn ", 3) == 0;
				cursor += isNormal ? 3 : 2; // skip "v " or "vn "

				pmp::vec3 vec;
				char* tempCursor;
				vec[0] = std::strtof(cursor, &tempCursor);
				cursor = tempCursor;

				vec[1] = std::strtof(cursor, &tempCursor);
				cursor = tempCursor;

				vec[2] = std::strtof(cursor, &tempCursor);
				cursor = tempCursor;

				if (isNormal)
					data.VertexNormals.push_back(vec);
				else
					data.Vertices.push_back(vec);
			}
			// If it's a face, parse the vertex indices
			else if (strncmp(cursor, "f ", 2) == 0)
			{
				cursor += 2; // skip "f "
				std::vector<unsigned int> faceIndices;

				while (*cursor != '\n' && cursor < end)
				{
					char* tempCursor;
					// Parse the vertex index
					const unsigned int vertexIndex = std::strtoul(cursor, &tempCursor, 10);
					if (cursor == tempCursor) // No progress in parsing, break to avoid infinite loop
						break;

					cursor = tempCursor;
					if (vertexIndex == 0) {
						// Underflow occurred, meaning strtoul failed to parse a valid number
						break;
					}
					faceIndices.push_back(vertexIndex - 1);

					if (*cursor == '/') // Check for additional indices
					{
						cursor++; // Skip the first '/'
						if (*cursor != '/') // Texture coordinate index is present
						{
							std::strtoul(cursor, &tempCursor, 10); // Parse and discard the texture index
							cursor = tempCursor;
						}

						if (*cursor == '/') // Normal index is present
						{
							cursor++; // Skip the second '/'
							std::strtoul(cursor, &tempCursor, 10); // Parse and discard the normal index
							cursor = tempCursor;
						}
					}

					// Skip to next index, newline, or end
					while (*cursor != ' ' && *cursor != '\n' && cursor < end)
						cursor++;

					if (*cursor == ' ')
						cursor++; // Skip space to start of next index
				}

				if (!faceIndices.empty())
					data.PolyIndices.push_back(faceIndices);

				// Move to the next line
				while (*cursor != '\n' && cursor < end)
					cursor++;
				if (cursor < end)
					cursor++; // Move past the newline character
			}
			else
			{
				// Skip to the next line if the current line isn't recognized
				while (*cursor != '\n' && cursor < end)
					cursor++;
			}
		}
	}

	// Assume average bytes per vertex and face line (these are just example values)
	//constexpr size_t avgBytesPerVertex = 13; // Example average size of a vertex line in bytes
	//constexpr size_t avgBytesPerFace = 7; // Example average size of a face line in bytes

	/**
	 * \brief A simple estimation utility for large OBJ files
	 * \param fileSize        file size in bytes.
	 * \param expectNormals   if true we expect the OBJ file to also contain vertex normals.
	 * \return pair { # of estimated vertices, # of estimated faces }.
	 */
	//std::pair<size_t, size_t> EstimateVertexAndFaceCapacitiesFromOBJFileSize(const size_t& fileSize, const bool& expectNormals = false)
	//{

	//	// Estimate total number of vertex and face lines
	//	const size_t estimatedTotalLines = fileSize / (((expectNormals ? 2 : 1) * avgBytesPerVertex + avgBytesPerFace) / 2);

	//	// Estimate number of vertices based on Botsch 2010 ratio N_F = 2 * N_V
	//	size_t estimatedVertices = estimatedTotalLines / (expectNormals ? 4 : 3); // Since N_F + N_V = 3 * N_V and N_V = N_N
	//	size_t estimatedFaces = 2 * estimatedVertices;

	//	return std::make_pair(estimatedVertices, estimatedFaces);
	//}

	/**
	 * \brief Parses a chunk of PLY point data. This function is run for each thread.
	 * \param start     start address for this thread.
	 * \param end       end address for this thread.
	 * \param data      preallocated chunk data.
	 */
	void ParsePointCloudChunk(const char* start, const char* end, std::vector<pmp::vec3>& data) {
		const char* cursor = start;

		while (cursor < end) 
		{
			// Skip any leading whitespace
			while ((*cursor == ' ' || *cursor == '\n') && cursor < end)
			{
				cursor++;
			}

			if (cursor >= end)
			{
				break; // Reached the end of the chunk
			}

			// Start of a new line
			const char* lineStart = cursor;

			// Find the end of the line
			while (*cursor != '\n' && cursor < end) 
			{
				cursor++;
			}

			// Check if the end of the line is within the chunk
			if (*cursor != '\n' && cursor == end) 
			{
				break; // The line is incomplete, so stop parsing
			}

			// Extract the line
			std::string line(lineStart, cursor);

			// Remove carriage return if present
			if (!line.empty() && line.back() == '\r') 
			{
				line.pop_back();
			}

			// Increment cursor to start the next line in the next iteration
			if (cursor < end) 
			{
				cursor++;
			}

			// Parse the line to extract vertex coordinates
			std::istringstream iss(line);
			pmp::vec3 vertex;
			if (!(iss >> vertex[0] >> vertex[1] >> vertex[2])) 
			{
				std::cerr << "ParsePointCloudChunk: Error parsing line: " << line << std::endl;
				continue;
			}

			data.push_back(vertex);
		}
	}

	/**
	 * \brief Reads the header of the PLY file for vertices.
	 * \param start     header start position in memory.
	 * \return pair { number of vertices read from the header, the memory position where the point data starts }
	 */
	[[nodiscard]] std::pair<size_t, char*> ReadPLYVertexHeader(const char* start)
	{
		const char* cursor = start;
		std::string line;
		size_t vertexCount = 0;

		while (*cursor != '\0') 
		{
			// Extract the line
			const char* lineStart = cursor;
			while (*cursor != '\n' && *cursor != '\0') 
			{
				cursor++;
			}
			line.assign(lineStart, cursor);

			// Trim trailing carriage return if present
			if (!line.empty() && line.back() == '\r') 
			{
				line.pop_back();
			}

			// Move to the start of the next line
			if (*cursor != '\0') 
			{
				cursor++;
			}

			// Check for the vertex count line
			if (line.rfind("element vertex", 0) == 0) 
			{
				std::istringstream iss(line);
				std::string element, vertex;
				if (!(iss >> element >> vertex >> vertexCount))
				{
					std::cerr << "ReadPLYVertexHeader [WARNING]: Failed to parse vertex count line: " << line << "\n";
					continue;
				}
				// Successfully parsed the vertex count line. Continue to look for the end of the header.
			}

			// Check for the end of the header
			if (line == "end_header") 
			{
				break;
			}
		}

		if (vertexCount == 0) 
		{
			std::cerr << "ReadPLYVertexHeader [ERROR]: Vertex count not found in the header.\n";
		}

		return { vertexCount, const_cast<char*>(cursor) }; // Return the vertex count and the cursor position
	}


} // anonymous namespace

namespace Geometry
{
	pmp::SurfaceMesh ConvertBufferGeomToPMPSurfaceMesh(const BaseMeshGeometryData& geomData)
	{
		pmp::SurfaceMesh result;

		// count edges
		std::set<std::pair<unsigned int, unsigned int>> edgeIdsSet;
		for (const auto& indexTuple : geomData.PolyIndices)
		{
			for (unsigned int i = 0; i < indexTuple.size(); i++)
			{
				unsigned int vertId0 = indexTuple[i];
				unsigned int vertId1 = indexTuple[(static_cast<size_t>(i) + 1) % indexTuple.size()];

				if (vertId0 > vertId1) std::swap(vertId0, vertId1);

				edgeIdsSet.insert({ vertId0, vertId1 });
			}
		}

		result.reserve(geomData.Vertices.size(), edgeIdsSet.size(), geomData.PolyIndices.size());
		for (const auto& v : geomData.Vertices)
			result.add_vertex(pmp::Point(v[0], v[1], v[2]));

		if (!geomData.VertexNormals.empty())
		{
			auto vNormal = result.vertex_property<pmp::Normal>("v:normal");
			for (auto v : result.vertices())
				vNormal[v] = geomData.VertexNormals[v.idx()];
		}

        for (const auto& indexTuple : geomData.PolyIndices)
        {
			std::vector<pmp::Vertex> vertices;
			vertices.reserve(indexTuple.size());
			for (const auto& vId : indexTuple)
				vertices.emplace_back(pmp::Vertex(vId));

			result.add_face(vertices);	        
        }

		return result;
	}

	pmp::SurfaceMesh ConvertMCMeshToPMPSurfaceMesh(const MC_Mesh& mcMesh)
	{
		pmp::SurfaceMesh result;

		// count edges
		std::set<std::pair<unsigned int, unsigned int>> edgeIdsSet;
		for (unsigned int i = 0; i < mcMesh.faceCount * 3; i += 3)
		{
			for (unsigned int j = 0; j < 3; j++)
			{
				unsigned int vertId0 = mcMesh.faces[i + j];
				unsigned int vertId1 = mcMesh.faces[i + (j + 1) % 3];

				if (vertId0 > vertId1) std::swap(vertId0, vertId1);

				edgeIdsSet.insert({ vertId0, vertId1 });
			}
		}

		result.reserve(mcMesh.vertexCount, edgeIdsSet.size(), mcMesh.faceCount);
		// MC produces normals by default
		auto vNormal = result.vertex_property<pmp::Normal>("v:normal");

		for (unsigned int i = 0; i < mcMesh.vertexCount; i++)
		{
			result.add_vertex(
			   pmp::Point(
				   mcMesh.vertices[i][0],
				   mcMesh.vertices[i][1],
				   mcMesh.vertices[i][2]
			));
			vNormal[pmp::Vertex(i)] = pmp::Normal{
				mcMesh.normals[i][0],
				mcMesh.normals[i][1],
				mcMesh.normals[i][2]
			};
		}

		for (unsigned int i = 0; i < mcMesh.faceCount * 3; i += 3)
		{
			std::vector<pmp::Vertex> vertices;
			vertices.reserve(3);
			for (unsigned int j = 0; j < 3; j++)
			{
				vertices.emplace_back(pmp::Vertex(mcMesh.faces[i + j]));
			}

			result.add_face(vertices);
		}

		return result;
	}

	bool ExportBaseMeshGeometryDataToOBJ(const BaseMeshGeometryData& geomData, const std::string& absFileName)
	{
		std::ofstream file(absFileName);
		if (!file.is_open())
		{
			std::cerr << "Failed to open file for writing: " << absFileName << std::endl;
			return false;
		}

		// Write vertices
		for (const auto& vertex : geomData.Vertices)
		{
			file << "v " << vertex[0] << ' ' << vertex[1] << ' ' << vertex[2] << '\n';
		}

		// Optionally, write vertex normals
		if (!geomData.VertexNormals.empty())
		{
			for (const auto& normal : geomData.VertexNormals)
			{
				file << "vn " << normal[0] << ' ' << normal[1] << ' ' << normal[2] << '\n';
			}
		}

		// Write faces
		for (const auto& indices : geomData.PolyIndices)
		{
			file << "f";
			for (unsigned int index : indices)
			{
				// OBJ indices start from 1, not 0
				file << ' ' << (index + 1);
			}
			file << '\n';
		}

		file.close();
		return true;
	}

	std::optional<BaseMeshGeometryData> ImportOBJMeshGeometryData(const std::string& absFileName, const bool& importInParallel, std::optional<std::vector<float>*> chunkIdsVertexPropPtrOpt)
	{
		const auto extension = Utils::ExtractLowercaseFileExtensionFromPath(absFileName);
		if (extension != "obj")
			return {};

		const char* file_path = absFileName.c_str();

		const HANDLE file_handle = CreateFile(file_path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

		if (file_handle == INVALID_HANDLE_VALUE) 
		{
			std::cerr << "ImportOBJMeshGeometryData [ERROR]: Failed to open the file.\n";
			return {};
		}

		// Get the file size
		const DWORD file_size = GetFileSize(file_handle, nullptr);

		// Create a file mapping object
		const HANDLE file_mapping = CreateFileMapping(file_handle, nullptr, PAGE_READONLY, 0, 0, nullptr);
		if (file_mapping == nullptr) 
		{
			std::cerr << "ImportOBJMeshGeometryData [ERROR]: Failed to create file mapping.\n";
			CloseHandle(file_handle);
			return {};
		}

		// Map the file into memory
		const LPVOID file_memory = MapViewOfFile(file_mapping, FILE_MAP_READ, 0, 0, 0);
		if (file_memory == nullptr) 
		{
			std::cerr << "ImportOBJMeshGeometryData [ERROR]: Failed to map the file.\n";
			CloseHandle(file_mapping);
			CloseHandle(file_handle);
			return {};
		}

		BaseMeshGeometryData resultData;

		// Determine the number of threads
		const size_t thread_count = importInParallel ? std::thread::hardware_concurrency() : 1;
		const size_t chunk_size = file_size / thread_count;
		std::vector<std::thread> threads(thread_count);
		std::vector<ChunkData> threadResults(thread_count);

		char* file_start = static_cast<char*>(file_memory);
		char* file_end = file_start + file_size;

		for (size_t i = 0; i < thread_count; ++i) {
			char* chunk_start = file_start + (i * chunk_size);
			char* chunk_end = (i == thread_count - 1) ? file_end : chunk_start + chunk_size;

			// Adjust chunk_end to point to the end of a line
			while (*chunk_end != '\n' && chunk_end < file_end) {
				chunk_end++;
			}
			if (chunk_end != file_end) {
				chunk_end++;  // move past the newline character
			}

			// Start a thread to process this chunk
			threads[i] = std::thread(ParseChunk, chunk_start, chunk_end, std::ref(threadResults[i]));
		}

		// Wait for all threads to finish
		for (auto& t : threads) {
			t.join();
		}

		if (chunkIdsVertexPropPtrOpt.has_value() && !chunkIdsVertexPropPtrOpt.value()->empty())
		{
			chunkIdsVertexPropPtrOpt.value()->clear();
		}
		for (int threadId = 0; const auto& result : threadResults)
		{
			if (chunkIdsVertexPropPtrOpt.has_value())
			{
				const auto nVertsPerChunk = result.Vertices.size();
				chunkIdsVertexPropPtrOpt.value()->insert(chunkIdsVertexPropPtrOpt.value()->end(), nVertsPerChunk, static_cast<float>(threadId));
			}
			resultData.Vertices.insert(resultData.Vertices.end(), result.Vertices.begin(), result.Vertices.end());
			resultData.PolyIndices.insert(resultData.PolyIndices.end(), result.PolyIndices.begin(), result.PolyIndices.end());
			resultData.VertexNormals.insert(resultData.VertexNormals.end(), result.VertexNormals.begin(), result.VertexNormals.end());

			++threadId;
		}

		// Clean up
		UnmapViewOfFile(file_memory);
		CloseHandle(file_mapping);
		CloseHandle(file_handle);

		return std::move(resultData);
	}

	std::optional<std::vector<pmp::vec3>> ImportPLYPointCloudData(const std::string& absFileName, const bool& importInParallel)
	{
		const auto extension = Utils::ExtractLowercaseFileExtensionFromPath(absFileName);
		if (extension != "ply")
			return {};

		const char* file_path = absFileName.c_str();

		const HANDLE file_handle = CreateFile(file_path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

		if (file_handle == INVALID_HANDLE_VALUE)
		{
			std::cerr << "ImportPLYPointCloudData [ERROR]: Failed to open the file.\n";
			return {};
		}

		// Get the file size
		const DWORD file_size = GetFileSize(file_handle, nullptr);

		// Create a file mapping object
		const HANDLE file_mapping = CreateFileMapping(file_handle, nullptr, PAGE_READONLY, 0, 0, nullptr);
		if (file_mapping == nullptr)
		{
			std::cerr << "ImportPLYPointCloudData [ERROR]: Failed to create file mapping.\n";
			CloseHandle(file_handle);
			return {};
		}

		// Map the file into memory
		const LPVOID file_memory = MapViewOfFile(file_mapping, FILE_MAP_READ, 0, 0, 0);
		if (file_memory == nullptr)
		{
			std::cerr << "ImportPLYPointCloudData [ERROR]: Failed to map the file.\n";
			CloseHandle(file_mapping);
			CloseHandle(file_handle);
			return {};
		}

		std::vector<pmp::vec3> resultData;

		char* file_start = static_cast<char*>(file_memory);
		char* file_end = file_start + file_size;

		// Read the PLY header to get the number of vertices and start position of vertex data
		const auto [vertexCount, vertexDataStart] = ReadPLYVertexHeader(file_start);
		if (vertexDataStart == 0)
		{
			std::cerr << "ImportPLYPointCloudData [ERROR]: Failed to read PLY header or no vertices found.\n";
			UnmapViewOfFile(file_memory);
			CloseHandle(file_mapping);
			CloseHandle(file_handle);
			return {};
		}

		// Adjust file_start to point to the beginning of vertex data
		file_start = vertexDataStart;

		// Ensure there is vertex data to process
		if (file_start >= file_end) 
		{
			std::cerr << "ImportPLYPointCloudData [ERROR]: No vertex data to process.\n";
			UnmapViewOfFile(file_memory);
			CloseHandle(file_mapping);
			CloseHandle(file_handle);
			return {};
		}

		// Adjust the file size for chunk calculation
		const size_t adjusted_file_size = file_end - file_start;

		// Determine the number of threads and chunk size
		const size_t thread_count = importInParallel ? std::thread::hardware_concurrency() : 1;
		const size_t chunk_size = std::max<size_t>(adjusted_file_size / thread_count, 1ul); // Ensure chunk size is at least 1
		std::vector<std::thread> threads(thread_count);
		std::vector<std::vector<pmp::vec3>> threadResults(thread_count);

		for (size_t i = 0; i < thread_count; ++i) 
		{
			char* chunk_start = file_start + (i * chunk_size);
			char* chunk_end = (i == thread_count - 1) ? file_end : chunk_start + chunk_size;

			// Adjust chunk_start to the beginning of a line (for all chunks except the first)
			if (i > 0) 
			{
				while (*chunk_start != '\n' && chunk_start < file_end) 
				{
					chunk_start++;
				}
				if (chunk_start < file_end)
				{
					chunk_start++;  // Move past the newline character
				}
			}

			// Adjust chunk_end to the end of a line
			while (*chunk_end != '\n' && chunk_end < file_end)
			{
				chunk_end++;
			}
			if (chunk_end < file_end)
			{
				chunk_end++;  // Move past the newline character
			}

			// Start a thread to process this chunk
			threads[i] = std::thread(ParsePointCloudChunk, chunk_start, chunk_end, std::ref(threadResults[i]));
		}

		// Wait for all threads to finish
		for (auto& t : threads) 
		{
			t.join();
		}

		for (const auto& result : threadResults)
		{
			resultData.insert(resultData.end(), result.begin(), result.end());
		}

		// Clean up
		UnmapViewOfFile(file_memory);
		CloseHandle(file_mapping);
		CloseHandle(file_handle);

		return std::move(resultData);
	}

	std::optional<std::vector<pmp::vec3>> ImportPLYPointCloudDataMainThread(const std::string& absFileName)
	{
		const auto extension = Utils::ExtractLowercaseFileExtensionFromPath(absFileName);
		if (extension != "ply")
		{
			std::cerr << absFileName << " has invalid extension!" << std::endl;
			return {};
		}

		std::ifstream file(absFileName);
		if (!file.is_open()) 
		{
			std::cerr << "Failed to open the file." << std::endl;
			return {};
		}

		std::string line;
		size_t vertexCount = 0;
		bool headerEnded = false;

		// Read header to find the vertex count
		while (std::getline(file, line) && !headerEnded) 
		{
			std::istringstream iss(line);
			std::string token;
			iss >> token;

			if (token == "element") {
				iss >> token;
				if (token == "vertex") {
					iss >> vertexCount;
				}
			}
			else if (token == "end_header") {
				headerEnded = true;
			}
		}

		if (!headerEnded || vertexCount == 0) {
			std::cerr << "Invalid PLY header or no vertices found." << std::endl;
			return {};
		}

		std::vector<pmp::vec3> vertices;
		vertices.reserve(vertexCount);

		// Read vertex data
		while (std::getline(file, line)) {
			std::istringstream iss(line);
			pmp::vec3 vertex;
			if (!(iss >> vertex[0] >> vertex[1] >> vertex[2])) {
				std::cerr << "Error parsing vertex data: " << line << std::endl;
				continue;
			}

			vertices.push_back(vertex);
		}

		return vertices;
	}

	bool ExportSampledVerticesToPLY(const BaseMeshGeometryData& meshData, size_t nVerts, const std::string& absFileName)
	{
		const auto extension = Utils::ExtractLowercaseFileExtensionFromPath(absFileName);
		if (extension != "ply")
		{
			std::cerr << absFileName << " has invalid extension!" << std::endl;
			return false;
		}

		std::ofstream file(absFileName);
		if (!file.is_open())
		{
			std::cerr << "Failed to open file for writing: " << absFileName << std::endl;
			return false;
		}

		// Generate nVerts random indices
		std::vector<size_t> indices;
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> distrib(0, meshData.Vertices.size() - 1);

		for (size_t i = 0; i < nVerts; ++i)
		{
			indices.push_back(distrib(gen));
		}

		// Write to a .ply file
		std::ofstream outFile(absFileName);
		if (!outFile.is_open()) 
		{
			std::cerr << "Unable to open file: " << absFileName << std::endl;
			return false;
		}

		// PLY header
		outFile << "ply\n";
		outFile << "format ascii 1.0\n";
		outFile << "element vertex " << nVerts << "\n";
		outFile << "property float x\n";
		outFile << "property float y\n";
		outFile << "property float z\n";
		outFile << "end_header\n";

		// Write sampled vertices
		for (auto idx : indices) 
		{
			const auto& vertex = meshData.Vertices[idx];
			outFile << vertex[0] << " " << vertex[1] << " " << vertex[2] << "\n";
		}

		outFile.close();
		return true;
	}

	bool ExportPolylinesToOBJ(const std::vector<std::vector<pmp::vec3>>& polylines, const std::string& absFileName)
	{
		const auto extension = Utils::ExtractLowercaseFileExtensionFromPath(absFileName);
		if (extension != "obj")
		{
			std::cerr << absFileName << " has invalid extension!" << std::endl;
			return false;
		}

		std::ofstream file(absFileName);
		if (!file.is_open()) 
		{
			std::cerr << "Failed to open file for writing: " << absFileName << std::endl;
			return false;
		}

		// Write vertices
		for (const auto& polyline : polylines)
		{
			for (const auto& vertex : polyline) 
			{
				file << "v " << vertex[0] << " " << vertex[1] << " " << vertex[2] << "\n";
			}
		}

		// Write polyline connections as lines
		size_t indexOffset = 1; // OBJ files are 1-indexed
		for (const auto& polyline : polylines)
		{
			if (polyline.size() < 2) continue; // Ensure there are at least two points to form a segment
			for (size_t i = 0; i < polyline.size() - 1; ++i)
			{
				file << "l " << (i + indexOffset) << " " << (i + indexOffset + 1) << "\n";
			}
			indexOffset += polyline.size();
		}

		file.close();
		return true;
	}

} // namespace Geometry