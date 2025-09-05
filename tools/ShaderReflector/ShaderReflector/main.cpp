#include <vector>
#include <stdint.h>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <Windows.h>
#include <variant>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <set>
#include <format>

#include <spirv-reflect/spirv_reflect.h>
#include <vulkan/vulkan.hpp>


using namespace std;



static std::vector<std::string> getAllFilesInDirectory(const std::filesystem::path& path) {
	std::vector<std::string> files;

	// Check if path exists and is a directory.
	if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
		for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
			if (std::filesystem::is_regular_file(entry.path())) {
				files.push_back(std::filesystem::absolute(entry.path()).string());
			}
		}
	}

	return files;
}

static std::filesystem::path get_executable_directory() {
	wchar_t buffer[MAX_PATH];
	HMODULE hModule = GetModuleHandle(nullptr);
	if (GetModuleFileName(hModule, buffer, MAX_PATH)) {
		std::filesystem::path exePath(buffer);
		return exePath.parent_path();
	}
	return L"";
}

static std::string makePathAbsolute(std::filesystem::path originatingPath, std::string relativePath) {
	// Specifying a relative path
	std::filesystem::path _relativePath(relativePath);

	// Converting to an absolute path
	std::filesystem::path absolutePath = originatingPath / _relativePath;
	absolutePath = std::filesystem::absolute(absolutePath);
	absolutePath = std::filesystem::canonical(absolutePath);  // Normalize the path
	return absolutePath.string();
}


static std::string getFileRawName(std::string fullPath) { // not actually raw. Just file without extension
	std::string name = std::filesystem::path(fullPath).filename().string();
	size_t lastindex = name.find_last_of(".");
	std::string rawname = name.substr(0, lastindex);
	return rawname;
}

static std::vector<uint8_t> readFile(const std::string& filename) {

	if (std::filesystem::exists(std::filesystem::path(filename)) == false) {
		std::cout << "file not found: " << filename << std::endl;
		throw std::runtime_error("failed to open file!");
	}

	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<uint8_t> buffer(fileSize);

	file.seekg(0);
	file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

	file.close();

	return buffer;
}


// todo:
// get shader buffer bindings
// get shader shader global texture binding usage
// assert std140 before using std140 struct types
// support push constant types
// struct padding for allignement in array
// equality must check class type and type array stride

// sorted by size
enum class primitiveShaderType {
	e_int = 0,
	e_uint,
	e_float,
	e_vec2,
	e_vec3,
	e_vec4,
	e_mat2,
	e_mat3,
	e_mat4
};

enum class shaderTypeClassification {
	unknown,
	uniform,
	ssbo,
	structure,
	pushConstant
};

enum class shaderAlignment {
	std140,
	std430
};

struct memberArrayTraits {
	uint32_t dims_count = 0;
	uint32_t dims[SPV_REFLECT_MAX_ARRAY_DIMS];
	uint32_t stride = 0;
	bool runtimeAr = false;
};

struct shaderMember {
	std::string name;
	// If primitive -> primitiveShaderType.
	// If struct     -> std::string holding the canonical struct type name.
	std::variant<primitiveShaderType, std::string> type;
	memberArrayTraits array{};

	uint32_t size;
	uint32_t offset;
};

struct shaderType {
	std::string memberName; // optional, not used for canonical entries
	std::string typeName;   // canonical key
	shaderTypeClassification typeClass;
	std::vector<shaderMember> members;
	uint32_t arrayStride;

	std::set<string> shaderAttributions;
};

struct shaderPushConstant {
	std::string shaderName;
	std::string name;
	std::vector<shaderMember> members;
};

std::vector<std::string> typeInsertionOrder;
map<string, shaderType> allTypes;

// ===== Additional globals: record which buffers use which struct =====


struct bufferInfo {
	string bufferName; // descriptor binding name (if any)
	string typeName;   // root struct type of the buffer
	uint32_t set{};
	uint32_t binding{};
	SpvReflectDescriptorType descriptorType{};
};
//vector<bufferInfo> allBuffers;

bool bufferInfoCompare(bufferInfo a, bufferInfo b) {
	bool same = true;
	same &= a.bufferName == b.bufferName;
	same &= a.typeName == b.typeName;
	same &= a.descriptorType == b.descriptorType;
	return same;
}

//
map<string, vector<bufferInfo>> pipelineBufferUsageGFX;
map<string, vector<bufferInfo>> pipelineBufferUsageCOMP;

struct textureInfo {
	string name;
	uint32_t binding = 0;
};

map<string, vector<textureInfo>> pipelineTextureUsageGFX;
map<string, vector<textureInfo>> pipelineTextureUsageCOMP;

// ===== Helpers =====

static inline memberArrayTraits CopyArrayTraits(const SpvReflectArrayTraits& a, SpvOp op) {
	memberArrayTraits t{};
	t.dims_count = a.dims_count;
	t.stride = a.stride;
	for (uint32_t i = 0; i < a.dims_count && i < SPV_REFLECT_MAX_ARRAY_DIMS; ++i)
		t.dims[i] = a.dims[i];
	if (op == SpvOpTypeRuntimeArray)
		t.runtimeAr = true;
	return t;
}

// Return the underlying struct SpvReflectTypeDescription* if 'td' is (array of) struct; else nullptr.
static SpvReflectTypeDescription* UnderlyingStruct(const SpvReflectTypeDescription* td) {
	if (!td) return nullptr;
	if (td->type_flags & SPV_REFLECT_TYPE_FLAG_STRUCT)
		return const_cast<SpvReflectTypeDescription*>(td);
	if ((td->type_flags & SPV_REFLECT_TYPE_FLAG_ARRAY) && td->struct_type_description &&
		(td->struct_type_description->type_flags & SPV_REFLECT_TYPE_FLAG_STRUCT))
		return td->struct_type_description;
	return nullptr;
}

// Prefer the SPIRV-Reflect provided helper for block variable type name; fall back to type_description->type_name.
static string BlockVariableTypeName(const SpvReflectBlockVariable& v) {
	const char* n = spvReflectBlockVariableTypeName(&v); // may be null
	if (n && *n) return string(n);
	if (v.type_description && v.type_description->type_name && *v.type_description->type_name)
		return string(v.type_description->type_name);
	return {};
}

// Convert a SpvReflectTypeDescription to our primitiveShaderType. Only float vec/mat are supported here.
static primitiveShaderType ToPrimitive(const SpvReflectTypeDescription& td) {
	const bool is_int = (td.type_flags & SPV_REFLECT_TYPE_FLAG_INT) != 0;
	const bool is_float = (td.type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT) != 0;
	const bool is_vec = (td.type_flags & SPV_REFLECT_TYPE_FLAG_VECTOR) != 0;
	const bool is_mat = (td.type_flags & SPV_REFLECT_TYPE_FLAG_MATRIX) != 0;

	if (!is_vec && !is_mat) {
		if (is_int) return td.traits.numeric.scalar.signedness ? primitiveShaderType::e_int
			: primitiveShaderType::e_uint;
		if (is_float) return primitiveShaderType::e_float;
		throw std::runtime_error("Unsupported primitive scalar type (bools or others not mapped).");
	}

	if (is_vec) {
		if (!is_float)
			throw std::runtime_error("Only float vectors are supported by primitiveShaderType mapping.");
		const uint32_t n = td.traits.numeric.vector.component_count;
		switch (n) {
		case 2: return primitiveShaderType::e_vec2;
		case 3: return primitiveShaderType::e_vec3;
		case 4: return primitiveShaderType::e_vec4;
		default: break;
		}
		throw std::runtime_error("Unsupported vector size (expected 2/3/4).");
	}

	if (is_mat) {
		if (!is_float)
			throw std::runtime_error("Only float matrices are supported by primitiveShaderType mapping.");
		const uint32_t r = td.traits.numeric.matrix.row_count;
		const uint32_t c = td.traits.numeric.matrix.column_count;
		if (r == 2 && c == 2) return primitiveShaderType::e_mat2;
		if (r == 3 && c == 3) return primitiveShaderType::e_mat3;
		if (r == 4 && c == 4) return primitiveShaderType::e_mat4;
		throw std::runtime_error("Unsupported matrix shape (only square 2/3/4 are supported).");
	}

	throw std::runtime_error("Unknown primitive type mapping.");
}

// Structural equality of two shaderType definitions (ignores 'memberName' on the canonical types)
static bool EqualTypes(const shaderType& a, const shaderType& b);

// Compare two member arrays (variant + array trait)
static bool EqualMemberAndArray(const std::variant<shaderType, primitiveShaderType>& va,
	const std::variant<shaderType, primitiveShaderType>& vb,
	const memberArrayTraits& at_a,
	const memberArrayTraits& at_b) {
	if (at_a.dims_count != at_b.dims_count) return false;
	for (uint32_t i = 0; i < at_a.dims_count; ++i)
		if (at_a.dims[i] != at_b.dims[i]) return false;

	if (va.index() != vb.index()) return false;

	if (std::holds_alternative<primitiveShaderType>(va)) {
		return std::get<primitiveShaderType>(va) == std::get<primitiveShaderType>(vb);
	}
	else {
		const shaderType& sa = std::get<shaderType>(va);
		const shaderType& sb = std::get<shaderType>(vb);
		// memberName must match for members
		if (sa.memberName != sb.memberName) return false;
		return EqualTypes(sa, sb);
	}
}

static bool EqualTypes(const shaderType& a, const shaderType& b) {
	if (a.typeName != b.typeName) return false;
	if (a.members.size() != b.members.size()) return false;

	for (size_t i = 0; i < a.members.size(); ++i) {
		const auto& ma = a.members[i];
		const auto& mb = b.members[i];

		if (ma.name != mb.name) return false;

		if (ma.array.dims_count != mb.array.dims_count) return false;
		for (uint32_t d = 0; d < ma.array.dims_count; ++d)
			if (ma.array.dims[d] != mb.array.dims[d]) return false;

		if (ma.type.index() != mb.type.index()) return false;

		if (std::holds_alternative<primitiveShaderType>(ma.type)) {
			if (std::get<primitiveShaderType>(ma.type) != std::get<primitiveShaderType>(mb.type))
				return false;
		}
		else {
			// struct-by-name
			if (std::get<std::string>(ma.type) != std::get<std::string>(mb.type))
				return false;
		}
	}
	return true;
}

// Maintain a guard against accidental recursion (shouldn't occur; SPIR-V forbids recursive structs)
static unordered_set<string> g_inProgress;

// Build a full struct definition from a BlockVariable that represents a struct or array-of-struct.
static shaderType BuildStructFromBlockVariable(const SpvReflectBlockVariable& v);

// Ensure that a canonical definition for 't.typeName' exists in allTypes and matches structurally if already present.
static void EnsureCanonical(const shaderType& t, string shaderSourceName) {
	auto it = allTypes.find(t.typeName);
	if (it == allTypes.end()) {
		allTypes.emplace(t.typeName, t);
		typeInsertionOrder.push_back(t.typeName);
	}
	else if (!EqualTypes(it->second, t)) {
		throw std::runtime_error("Conflicting struct definitions with the same name: " + t.typeName);
	}
	allTypes[t.typeName].shaderAttributions.insert(shaderSourceName);
}

static shaderType BuildStructFromBlockVariable(const SpvReflectBlockVariable& v, shaderTypeClassification typeClass, string shaderSourceName) {
	// Determine the struct's declared type name
	shaderType out{};
	out.typeName = BlockVariableTypeName(v);
	out.typeClass = typeClass;
	out.memberName.clear();
	out.arrayStride = v.type_description->traits.array.stride;

	if (out.typeName.empty()) {
		throw std::runtime_error("Encountered a struct with no type name (unnamed structs are not supported).");
	}

	// Recursion guard
	if (g_inProgress.find(out.typeName) != g_inProgress.end()) {
		// This should not happen with SPIR-V (no recursive types); treat as an error
		throw std::runtime_error("Recursive or cyclic struct reference detected for type: " + out.typeName);
	}
	g_inProgress.insert(out.typeName);

	out.members.reserve(v.member_count);

	for (uint32_t i = 0; i < v.member_count; ++i) {
		const SpvReflectBlockVariable& m = v.members[i];

		shaderMember mem{};
		mem.name = m.name ? std::string(m.name) : std::string();
		mem.array = CopyArrayTraits(m.array, m.type_description->op);
		mem.size = m.size;
		mem.offset = m.offset;

		if (UnderlyingStruct(m.type_description)) {
			// Build nested canonical definition (throws on conflict)
			shaderType nested = BuildStructFromBlockVariable(m, shaderTypeClassification::structure, shaderSourceName);
			nested.memberName.clear(); // canonical form ignores memberName

			EnsureCanonical(nested, shaderSourceName);   // inserts or verifies allTypes[nested.typeName]
			mem.type = nested.typeName; // <-- store only the type name
		}
		else {
			if (!m.type_description) throw std::runtime_error("Member has no type_description.");
			mem.type = ToPrimitive(*m.type_description);
		}

		out.members.emplace_back(std::move(mem));
	}

	// Insert/validate canonical definition now
	EnsureCanonical(out, shaderSourceName);

	g_inProgress.erase(out.typeName);
	return out;
}

enum class pipelineType {
	GFX,
	Compute
};

string pipelineName(string shaderName) {
	std::size_t pos = shaderName.find('_');
	if (pos != std::string::npos) {
		return shaderName.substr(0, pos);
	}
	throw std::runtime_error("invalid shader name");
	return "";
}

pipelineType getPipelineType(string shaderName) {
	std::size_t pos = shaderName.find('_');
	if (pos != std::string::npos) {
		string s = shaderName.substr(pos + 1, shaderName.length());
		if (s == "vert" || s == "frag")
			return  pipelineType::GFX;
		else if (s == "comp")
			return  pipelineType::Compute;

		throw std::runtime_error("invalid shader name");
	}
	throw std::runtime_error("invalid shader name");
	return  pipelineType::GFX;
}


// ===== Main entry point =====

void PopulateShaderInfo(const std::vector<uint8_t>& shaderSrc, string shaderName) {
	SpvReflectShaderModule spvModule;
	SpvReflectResult result =
		spvReflectCreateShaderModule(shaderSrc.size(),
			reinterpret_cast<const uint32_t*>(shaderSrc.data()),
			&spvModule);
	if (result != SPV_REFLECT_RESULT_SUCCESS) {
		throw std::runtime_error("Shader reflection failed");
	}

	// Enumerate all descriptor bindings and start only from buffers (UBO/SSBO).
	uint32_t bindingCount = 0;
	result = spvReflectEnumerateDescriptorBindings(&spvModule, &bindingCount, nullptr);
	if (result != SPV_REFLECT_RESULT_SUCCESS) {
		spvReflectDestroyShaderModule(&spvModule);
		throw std::runtime_error("Shader reflection failed during binding count");
	}

	if (bindingCount == 0) {
		spvReflectDestroyShaderModule(&spvModule);
		return;
	}

	std::vector<SpvReflectDescriptorBinding*> bindings(bindingCount);
	result = spvReflectEnumerateDescriptorBindings(&spvModule, &bindingCount, bindings.data());
	if (result != SPV_REFLECT_RESULT_SUCCESS) {
		spvReflectDestroyShaderModule(&spvModule);
		throw std::runtime_error("Shader reflection failed during binding enumeration");
	}

	for (const auto* binding : bindings) {
		if (!binding) continue;
		
		if (binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
			string plname = pipelineName(shaderName);
			pipelineType pltype = getPipelineType(shaderName);
			auto& usage = pltype == pipelineType::GFX ? pipelineTextureUsageGFX[plname] : pipelineTextureUsageCOMP[plname];
			usage.push_back(textureInfo{ .name = binding->name, .binding = binding->binding });
			continue;
		}
		else if (binding->descriptor_type != SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER &&
			binding->descriptor_type != SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
			continue; // Only buffers participate in our type discovery
		}

		// The descriptor's block represents the buffer's root struct
		const SpvReflectBlockVariable& blk = binding->block;

		// Sanity: ensure this is actually a struct (or array-of-struct)
		SpvReflectTypeDescription* struct_td = UnderlyingStruct(blk.type_description);
		if (!struct_td) {
			// Some shaders may bind raw byte-address buffers; ignore silently or handle as needed.
			continue;
		}

		shaderTypeClassification typeClass = shaderTypeClassification::unknown;
		if (binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
			typeClass = shaderTypeClassification::uniform;
		else if (binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER)
			typeClass = shaderTypeClassification::ssbo;
		else {
			cout << "Warning: unknown shader type classification " << blk.name << "\n";
		}

		// Build the root struct (recursively discovers & catalogs nested structs)
		shaderType root = BuildStructFromBlockVariable(blk, typeClass, shaderName);

		// Record that this buffer uses this struct
		bufferInfo bi{};
		bi.bufferName = (binding->name ? string(binding->name) : string());
		bi.typeName = BlockVariableTypeName(binding->block);
		bi.set = binding->set;
		bi.binding = binding->binding;
		bi.descriptorType = binding->descriptor_type;
		//allBuffers.emplace_back(std::move(bi));

		string plname = pipelineName(shaderName);
		pipelineType pltype = getPipelineType(shaderName);

		auto& usage = pltype == pipelineType::GFX ? pipelineBufferUsageGFX[plname] : pipelineBufferUsageCOMP[plname];

		bool alreadyFound = false;
		for (auto& bia : usage)
		{
			if (bufferInfoCompare(bi, bia)) {

				if (bi.binding != bia.binding) {
					throw std::runtime_error("bindings in buffer " + bi.typeName + " do not match across shader " + plname + " source files");
				}
				if (bi.set != bia.set) {
					throw std::runtime_error("sets in buffer " + bi.typeName + " do not match across shader " + plname + " source files");
				}

				alreadyFound = true;
				break;
			}
		}
		if (!alreadyFound) {
			usage.push_back(bi);
		}
	}

	// push constant
	{

		uint32_t blockCount = 0;
		result = spvReflectEnumeratePushConstantBlocks(&spvModule, &blockCount, nullptr);
		if (result != SPV_REFLECT_RESULT_SUCCESS) {
			throw std::runtime_error("Shader reflection failed");
			spvReflectDestroyShaderModule(&spvModule);
			return;
		}

		if (blockCount > 0) {
			std::vector<SpvReflectBlockVariable*> pushConstants(blockCount);
			result = spvReflectEnumeratePushConstantBlocks(&spvModule, &blockCount, pushConstants.data());
			if (result != SPV_REFLECT_RESULT_SUCCESS) {
				throw std::runtime_error("Shader reflection failed");
				spvReflectDestroyShaderModule(&spvModule);
				return;
			}

			for (const auto& block : pushConstants) {

				// Build the root struct (recursively discovers & catalogs nested structs)
				shaderType root = BuildStructFromBlockVariable(*block, shaderTypeClassification::pushConstant, shaderName);
			}
		}
	}

	spvReflectDestroyShaderModule(&spvModule);
}




struct buffer_info {
	uint32_t set;
	uint32_t binding;
	vk::DescriptorType type;
};

struct push_constant_info {
	uint32_t size;
};

struct spec_constant_info {
	uint32_t constantID;
};



static void GetShaderBufferBindings(
	const std::vector<uint8_t>& shaderSrc,
	std::vector<buffer_info>& bufferInfos,
	push_constant_info& pushInfo,
	std::vector<spec_constant_info>& specInfos
) {

	SpvReflectShaderModule spvModule;
	SpvReflectResult result = spvReflectCreateShaderModule(shaderSrc.size(), reinterpret_cast<const uint32_t*>(shaderSrc.data()), &spvModule);
	if (result != SPV_REFLECT_RESULT_SUCCESS) {
		throw std::runtime_error("Shader reflection failed");
		return;
	}

	// buffer bindings
	{

		uint32_t bindingCount = 0;
		result = spvReflectEnumerateDescriptorBindings(&spvModule, &bindingCount, nullptr);
		if (result != SPV_REFLECT_RESULT_SUCCESS) {
			throw std::runtime_error("Shader reflection failed");
			spvReflectDestroyShaderModule(&spvModule);
			return;
		}

		if (bindingCount > 0) {
			std::vector<SpvReflectDescriptorBinding*> bindings(bindingCount);
			spvReflectEnumerateDescriptorBindings(&spvModule, &bindingCount, bindings.data());

			for (const auto& binding : bindings) {
				// Filter out if this is not a buffer
				if (binding->descriptor_type != SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER &&
					binding->descriptor_type != SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
					continue;
				}

				bufferInfos.push_back(buffer_info{
					.set = binding->set,
					.binding = binding->binding,
					.type = binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER ? vk::DescriptorType::eUniformBuffer : vk::DescriptorType::eStorageBuffer
					});
			}
		}
	}

	{
		//spvReflectEnumerate
	}

	// push constant
	{
		pushInfo.size = 0;

		uint32_t blockCount = 0;
		result = spvReflectEnumeratePushConstantBlocks(&spvModule, &blockCount, nullptr);
		if (result != SPV_REFLECT_RESULT_SUCCESS) {
			throw std::runtime_error("Shader reflection failed");
			spvReflectDestroyShaderModule(&spvModule);
			return;
		}

		if (blockCount > 0) {
			std::vector<SpvReflectBlockVariable*> pushConstants(blockCount);
			result = spvReflectEnumeratePushConstantBlocks(&spvModule, &blockCount, pushConstants.data());
			if (result != SPV_REFLECT_RESULT_SUCCESS) {
				throw std::runtime_error("Shader reflection failed");
				spvReflectDestroyShaderModule(&spvModule);
				return;
			}

			for (const auto& block : pushConstants) {

				// only support one push constant per pipeline
				assert(pushInfo.size == 0);

				pushInfo.size = block->size;
			}
		}
	}


	spvReflectDestroyShaderModule(&spvModule);
}


string getShaderTypeStr(primitiveShaderType t, bool std140Array) {
	switch (t)
	{
	case primitiveShaderType::e_int:
		return std140Array ? "std140_intArray" : "int32_t";
	case primitiveShaderType::e_uint:
		return std140Array ? "std140_uintArray" : "uint32_t";
	case primitiveShaderType::e_float:
		return std140Array ? "std140_floatArray" : "float";
	case primitiveShaderType::e_vec2:
		return std140Array ? "std140_vec2Array" : "glm::vec2";
	case primitiveShaderType::e_vec3:
		return std140Array ? "std140_vec3Array" : "glm::vec3";
	case primitiveShaderType::e_vec4:
		return std140Array ? "std140_vec4Array" : "glm::vec4";
	case primitiveShaderType::e_mat2:
		return "std140_mat2";
	case primitiveShaderType::e_mat3:
		return "std140_mat3";
	case primitiveShaderType::e_mat4:
		return "std140_mat4";
	default:
		throw std::runtime_error("invlalid shader primitive");
	}
}

// finds the smallest integer divisor of n greater than d
int nextDivor(int n, int d) {
	assert(d <= n);

	if (d == n)
		return d;

	do {
		d++;
	} while (n % d != 0 && d < n);
	return d;
}

// formats the brackets at the end of a mamber name eg. int num[12, 43];
string formatArray(memberArrayTraits traits) {
	if (traits.dims_count == 0) {
		if (traits.runtimeAr)
			return "[]";
		return "";
	}
	string res = "[";
	for (size_t i = 0; i < traits.dims_count; i++)
	{
		res += to_string(traits.dims[i]);
		if (i + 1 < traits.dims_count)
			res += ",";
	}
	res += "]";
	return res;
}

int currentIndent = 0;
string getIndent() {
	string out = "";
	for (int i = 0; i < currentIndent; i++)
	{
		out += "\t";
	}
	return out;
}


bool isStd140Array(primitiveShaderType p, const shaderMember& m) {
	return p < primitiveShaderType::e_vec4 && m.array.stride == 16;
}

enum class bindingType {
	buffer,
	texture,
	specConstant
};

struct resourceParam {
	string name;
	uint32_t set;
	uint32_t binding;
	bindingType bt;
};
map<string, vector<resourceParam>> gfxParams;
map<string, vector<resourceParam>> compParams;

int main()
{
	auto exePath = get_executable_directory();
	auto shaderPath = makePathAbsolute(exePath, "../../../../shaders/compiled/debug");
	auto outpath = makePathAbsolute(exePath, "../../../../shared");

	vector<std::string> files = getAllFilesInDirectory(shaderPath);

	for (auto& file : files)
	{
		auto srcData = readFile(file);

		string shaderName = getFileRawName(file);

		try
		{
			PopulateShaderInfo(srcData, shaderName);
		}
		catch (const std::runtime_error& e)
		{
			std::cerr << "Error: " << e.what() << std::endl;
			return 1;
		}

	}


	// some extra checks
	{

		for (auto& [name, bv] : pipelineBufferUsageGFX) {
			set<uint64_t> usedBindings;
			for (auto& b : bv)
			{
				uint64_t c = ((uint64_t)b.binding) | ((uint64_t)b.set) << 32;
				if (usedBindings.contains(c)) {
					std::cerr << "Error: pipeline " + name + " contains buffer with set " + to_string(b.set) + " binding " + to_string(b.binding) + " with different names accross shader stages";
					return 1;
				}
				usedBindings.insert(c);
			}
		}
	}



	{
		for (auto& [name, bv] : pipelineBufferUsageGFX) {
			for (auto& b : bv)
			{
				resourceParam p;
				p.name = b.typeName;
				p.set = b.set;
				p.binding = b.binding;
				p.bt = bindingType::buffer;
				gfxParams[name].push_back(p);
			}
		}

		for (auto& [name, bv] : pipelineTextureUsageGFX) {
			for (auto& b : bv) {
				resourceParam p;
				p.name = b.name;
				p.bt = bindingType::texture;
				p.binding = b.binding;
				gfxParams[name].push_back(p);
			}
		}

		for (auto& [name, bv] : pipelineBufferUsageCOMP) {
			for (auto& b : bv)
			{
				resourceParam p;
				p.name = b.typeName;
				p.set = b.set;
				p.binding = b.binding;
				p.bt = bindingType::buffer;
				gfxParams[name].push_back(p);
			}
		}

		for (auto& [name, bv] : pipelineTextureUsageCOMP) {
			for (auto& b : bv) {
				resourceParam p;
				p.name = b.name;
				p.bt = bindingType::texture;
				p.binding = b.binding;
				gfxParams[name].push_back(p);
			}
		}
	}
	
	// ShaderTypes.h
	{

		std::ostringstream out;
		out <<
			R"(
#pragma once

/*
    Generated by ShaderReflector tool
*/

#include <stdint.h>
#include <glm/glm.hpp>
#include "std140.h"

namespace ShaderTypes
{
)";

		currentIndent++;

		for (auto& s : typeInsertionOrder)
		{
			const auto& v = allTypes[s];

			uint32_t typeSize = v.members.back().offset + v.members.back().size;

			// comment block
			{
				out << getIndent() << "/*\n";
				currentIndent++;
				out << getIndent();
				if (v.typeClass == shaderTypeClassification::uniform)
					out << "Uniform buffer ";
				else if (v.typeClass == shaderTypeClassification::ssbo)
					out << "Storage buffer ";
				else if (v.typeClass == shaderTypeClassification::structure)
					out << "Shader structure ";
				else if (v.typeClass == shaderTypeClassification::pushConstant)
					out << "Push constant ";
				else if (v.typeClass == shaderTypeClassification::unknown)
					out << "Unknown type ";
				out << v.typeName << "\n";
				out << getIndent() << "Used in:\n";
				currentIndent++;
				for (auto& s : v.shaderAttributions)
					out << getIndent() << "- " << s << "\n";
				currentIndent--;



				out << getIndent() << "Type size: " << to_string(typeSize) << "\n";
				if (v.typeClass == shaderTypeClassification::structure) {
					out << getIndent() << "Array stride: " << to_string(v.arrayStride) << "\n";
				}
				currentIndent--;
				out << getIndent() << "*/\n";
			}

			out << getIndent() << "struct ";

			if (v.typeClass == shaderTypeClassification::structure) {

				// must be std140
				if (v.arrayStride != typeSize) {
					assert(v.arrayStride > typeSize);

					out << "alignas(" << 16 << ") ";
				}

			}

			out << v.typeName << "\n";
			out << getIndent() << "{\n";
			currentIndent++;

			vector<string> prefixes;
			vector<string> suffixes;

			// also handle memory alignment, agnostic of whether struct is std140 or std430
			uint32_t currentOffset = 0;
			for (const shaderMember& m : v.members) {

				string prefix = "";

				// not really analytical, but should work. Doesn't do matrices yet
				if (currentOffset != m.offset) {
					if (m.offset % 16 == 0 && ((currentOffset / 16) + 1) * 16 == m.offset)
						prefix += "alignas(16) ";
					else if (m.offset % 8 == 0 && ((currentOffset / 8) + 1) * 8 == m.offset)
						prefix += "alignas(8) ";
					else {
						cout << "Error: auto alignment for type " << s << " failed\n";
						return -1;
					}

				}

				// m.name is always the field name
				if (std::holds_alternative<primitiveShaderType>(m.type)) {
					primitiveShaderType p = std::get<primitiveShaderType>(m.type);

					if (m.array.dims_count > 0) {
						// should be set to the size if std430, or size of vec4 is std140
						assert(m.array.stride == m.size || m.array.stride == 16);


					}
					bool std140Array = isStd140Array(p, m);

					prefix += getShaderTypeStr(p, std140Array);
				}
				else {
					const shaderType& nested = allTypes[std::get<string>(m.type)];
					// nested members have their own m2.name, etc.
					prefix += nested.typeName;
				}

				//// just make the type a pointer instead of an unspecified array []
				//if (m.array.dims_count == 0 && m.array.runtimeAr)
				//    prefix += "*";


				prefixes.push_back(prefix);
				suffixes.push_back(m.name + formatArray(m.array));

				currentOffset += m.size;
			}

			uint32_t longestPrefix = 0;
			for (size_t i = 0; i < prefixes.size(); i++)
				longestPrefix = std::max((size_t)longestPrefix, prefixes[i].size());

			// padding
			for (size_t i = 0; i < prefixes.size(); i++)
			{
				std::string formatted_left = std::format("{:<{}}", prefixes[i], longestPrefix);
				out << getIndent() << formatted_left << " " << suffixes[i] << ";\n";
			}

			currentIndent--;
			out << getIndent() << "};\n\n";
		}

		out <<
			R"(
} // namespace
    )";



		std::ofstream file(string(outpath.c_str()) + "/ShaderTypes.h");
		if (file.is_open()) {
			file << out.str();
			file.close();
			std::cout << "ShaderTypes.h\n";
		}
		else {
			std::cout << "failed to write header\n";
			return -1;
		}

	}

	currentIndent = 0;

	// ShaderUtility.h
	{
		std::ostringstream out;
		out <<
			R"(
#pragma once

#include <stdint.h>

#include "VKEngine.h"
#include "pipeline.h"
#include "ShaderTypes.h"

/*
    Generated by ShaderReflector tool
*/

namespace ShaderUtil
{

	template<typename T>
	void CreateMappedInstanceBuffer(VKEngine* engine, uint32_t instanceCount, MappedDoubleBuffer<T>& buffer);

	template<typename T>
	void CreateMappedInstanceBuffer(VKEngine* engine, MappedDoubleBuffer<T>& buffer);

)";


		currentIndent++;

		for (auto& [name, bv] : gfxParams)
		{
			vector<string> prefixes;
			vector<string> suffixes;
			int maxPrefixLen = 0;

			out << getIndent() << "PipelineResourceConfig " << name << "_CreateBufferBindings(\n";
			currentIndent++;

			for (int i = 0; i < bv.size(); i++)
			{
				string prefix = "";
				string suffix = "";

				if (bv[i].bt == bindingType::buffer) {
					prefix += "LinkedBindableBuffer<ShaderTypes::" + bv[i].name + "> ";
					suffix += "_" + bv[i].name;
				}
				else if (bv[i].bt == bindingType::texture) {
					prefix += "GlobalImageDescriptor* ";
					suffix += "_" + bv[i].name;
				}
				else {
					assert(false);
				}

				if (i + 1 < bv.size())
					suffix += ",\n";
				else
					suffix += "\n";

				prefixes.push_back(prefix);
				suffixes.push_back(suffix);
				maxPrefixLen = std::max(maxPrefixLen, (int)prefix.length());
			}

			for (size_t i = 0; i < prefixes.size(); i++)
			{
				std::string formatted_left = std::format("{:<{}}", prefixes[i], maxPrefixLen);
				out << getIndent() << formatted_left << " " << suffixes[i];// << ";\n";
			}

			currentIndent--;
			out << getIndent() << ");\n\n";

		}

		currentIndent--;

		//out << "\n\n";

		out <<
			R"(
} // namespace
        )";


		std::ofstream file(string(outpath.c_str()) + "/ShaderUtility.h");
		if (file.is_open()) {
			file << out.str();
			file.close();
			std::cout << "ShaderUtil.h\n";
		}
		else {
			std::cout << "failed to write header\n";
			return -1;
		}
	}

	currentIndent = 0;

	// ShaderUtility.cpp
	{

		std::ostringstream out;
		out <<
			R"(
#include "stdafx.h"

#include <stdint.h>

#include <VKEngine.h>
#include "pipeline.h"
#include "std140.h"
#include "ShaderTypes.h"

#include "ShaderUtility.h"

/*
    Generated by ShaderReflector tool
*/

namespace ShaderUtil
{
)";


		currentIndent++;

		for (auto& s : typeInsertionOrder)
		{
			const auto& v = allTypes[s];

			// only generate for bound buffers
			string usageEnum = "";
			if (v.typeClass == shaderTypeClassification::ssbo)
				usageEnum = "eStorageBuffer";
			else if (v.typeClass == shaderTypeClassification::uniform)
				usageEnum = "eUniformBuffer";
			else
				continue;

			// only generate for buffer types with a single array member of unspecified size
			if (v.members.size() != 1)
				continue;
			const auto& m = v.members[0];
			if (m.array.runtimeAr == false && m.array.dims_count != 1)
				continue;

			string memberTypeName = "";

			if (std::holds_alternative<primitiveShaderType>(m.type)) {
				primitiveShaderType p = std::get<primitiveShaderType>(m.type);

				bool std140Array = isStd140Array(p, m);
				memberTypeName += getShaderTypeStr(p, std140Array);
			}
			else {
				const shaderType& nested = allTypes[std::get<string>(m.type)];
				memberTypeName = "ShaderTypes::" + nested.typeName;
			}

			out << getIndent() << "template<>\n";
			if (m.array.runtimeAr) {
				out << getIndent() << "void CreateMappedInstanceBuffer<ShaderTypes::" << v.typeName << ">(VKEngine* engine, uint32_t instanceCount, MappedDoubleBuffer<ShaderTypes::" << v.typeName << ">& buffer)\n";
				out << getIndent() << "{\n";
				currentIndent++;
				out << getIndent() << "engine->createMappedBuffer(sizeof(" << memberTypeName << ") * instanceCount, vk::BufferUsageFlagBits::" << usageEnum << ", buffer);\n";
			}
			else {
				out << getIndent() << "void CreateMappedInstanceBuffer<ShaderTypes::" << v.typeName << ">(VKEngine* engine, MappedDoubleBuffer<ShaderTypes::" << v.typeName << ">& buffer)\n";
				out << getIndent() << "{\n";
				currentIndent++;
				out << getIndent() << "engine->createMappedBuffer(sizeof(" << memberTypeName << ") * " << to_string(m.array.dims[0]) << ", vk::BufferUsageFlagBits::" << usageEnum << ", buffer);\n";
			}

			currentIndent--;
			out << getIndent() << "}\n\n";
		}



		for (auto& [name, bv] : gfxParams)
		{
			vector<string> prefixes;
			vector<string> suffixes;
			int maxPrefixLen = 0;

			out << getIndent() << "PipelineResourceConfig " << name << "_CreateBufferBindings(\n";
			currentIndent++;

			for (int i = 0; i < bv.size(); i++)
			{
				string prefix = "";
				string suffix = "";

				if (bv[i].bt == bindingType::buffer) {
					prefix += "LinkedBindableBuffer<ShaderTypes::" + bv[i].name + "> ";
					suffix += "_" + bv[i].name;
				}
				else if (bv[i].bt == bindingType::texture) {
					prefix += "GlobalImageDescriptor* ";
					suffix += "_" + bv[i].name;
				}
				else {
					assert(false);
				}

				if (i + 1 < bv.size())
					suffix += ",\n";
				else
					suffix += "\n";

				prefixes.push_back(prefix);
				suffixes.push_back(suffix);
				maxPrefixLen = std::max(maxPrefixLen, (int)prefix.length());
			}

			for (size_t i = 0; i < prefixes.size(); i++)
			{
				std::string formatted_left = std::format("{:<{}}", prefixes[i], maxPrefixLen);
				out << getIndent() << formatted_left << " " << suffixes[i];// << ";\n";
			}

			int buffercount = 0;
			int globalDCount = 0;
			for (auto& b : bv)
			{
				if (b.bt == bindingType::buffer)
					buffercount++;
				else if (b.bt == bindingType::texture)
					globalDCount++;
			}

			currentIndent--;
			out << getIndent() << "){\n";
			currentIndent++;
			out << getIndent() << "PipelineResourceConfig con;\n";
			if (buffercount > 0) {
				out << getIndent() << "con.bufferBindings = std::vector<BufferBinding> {\n";
				currentIndent++;
				for (int i = 0; i < bv.size(); i++) {
					if (bv[i].bt == bindingType::buffer) {
						out << getIndent() << "BufferBinding(" << bv[i].set << ", " << bv[i].binding << ", _" << bv[i].name << "),\n";
					}
				}
				currentIndent--;
				out << getIndent() << "};\n";
			}
			if (globalDCount > 0) {
				out << getIndent() << "con.globalDescriptors = std::vector<GlobalDescriptorBinding>{\n";
				currentIndent++;
				for (int i = 0; i < bv.size(); i++) {
					if (bv[i].bt == bindingType::texture) {
						out << getIndent() << "{" << 0 << ", _" << bv[i].name << "},\n";
					}
				}
				currentIndent--;
				out << getIndent() << "};\n";
			}
			out << getIndent() << "return con;\n";
			currentIndent--;
			out << getIndent() << "}\n\n";


		}




		out <<
			R"(
} // namespace
    )";

		std::ofstream file(string(outpath.c_str()) + "/ShaderUtility.cpp");
		if (file.is_open()) {
			file << out.str();
			file.close();
			std::cout << "ShaderUtil.cpp\n";
		}
		else {
			std::cout << "failed to write header\n";
			return -1;
		}
	}


	return  0;
}

