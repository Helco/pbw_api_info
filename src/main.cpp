#include "pbw_api_info.h"

#include <iostream>
#include <string>
#include <algorithm>

#include <sys/types.h>
#include <sys/stat.h>

#define INDENT_CHARACTER ' '
#define INDENT_WIDTH 2

enum ArgPlatform {
	ArgPlatform_Aplite = 0,
	ArgPlatform_Basalt,
	ArgPlatform_Diorite,
	ArgPlatform_Chalk,
	ArgPlatform_Emery,

	ArgPlatformCount
};

static const char* PlatformNames[ArgPlatformCount] = {
	"aplite", "basalt", "diorite", "chalk", "emery"
};

struct ProgramArguments {
	bool verbose = false;
#ifndef WIN32
	std::string sdkroot = "~/.pebble-sdk/SDKs/current/sdk-core/";
#else
	std::string sdkroot = "";
#endif
	bool defaultSdkroot = true;
	bool mapLibFunctions = false;
	std::string inputFile = "null";
	std::string outputFile; // if "" then output to stdout
	std::string libPath[ArgPlatformCount];
};

struct ProgramArgumentParser {
	int argc;
	int argi;
	char** argv;
};

void printHelp() {
	std::cerr
		<< "usage: pbw_api_info [options] [inputfile|'null'] [outputfile]" << std::endl << std::endl
		<< "options:" << std::endl
		<< "  -h --help             -> Shows this help screen and exits the program" << std::endl
		<< "  --sdkroot             -> Sets the path of the *core* sdk" << std::endl
		<< "  --libpath-<platform>  -> Sets the path of a single platform import library" << std::endl
		<< "    <platform> may be: aplite, basalt, diorite, chalk, emery" << std::endl
		<< "  --map-lib-functions   -> Outputs all functions of the libraries" << std::endl
		<< "  -v --verbose          -> Prints detailed progress information to stderr" << std::endl
		<< std::endl;
}

/**
 * tests if current argument is <testArg>=<value> or <testArg> <value> and copies the value
 * if testArg is used, but no value is given, an error message is printed
 * returns true if the argument is indeed testArg even if no value could be determined
 */
bool isValueArgument(ProgramArgumentParser& parser, const char* testArg, std::string& value) {
	value = "";
	const char* curArg = parser.argv[parser.argi - 1]; // we already incremented the index
	if (strstr(curArg, testArg) == curArg) {
		curArg += strlen(testArg);

		// confirm <testArg> <value>
		if (curArg[0] == '\0' && parser.argi < parser.argc && *parser.argv[parser.argi] != '-')
			value.assign(parser.argv[parser.argi++]);
		// confirm <testArg>=<value>
		else if (curArg[0] == '=' && curArg[1] != '\0')
			value.assign(curArg + 2);
		else
			std::cerr << "expected a value for option \"" << testArg << "\"" << std::endl;
		
		return true;
	}
	else
		return false;
}

/**
 * takes the console arguments and parses them into the ProgramArguments structure
 * @returns true if the program should continue
 */
bool parseArguments(ProgramArguments& args, int argc, char* argv[]) {
	std::string optionValue;
	ProgramArgumentParser parser;
	parser.argc = argc;
	parser.argv = argv;
	parser.argi = 1; // the first argument is the executable

	if (parser.argc == 1) {
		printHelp();
		return false;
	}

	// Read options
	while (parser.argi < parser.argc) {
		const char* curArg = parser.argv[parser.argi++];
		if (curArg[0] != '-') {
			parser.argi--; // we did not consume it
			break;
		}
		else if (strcmp(curArg, "-h") == 0 || strcmp(curArg, "--help") == 0) {
			printHelp();
			return false;
		}
		else if (strcmp(curArg, "-v") == 0 || strcmp(curArg, "--verbose") == 0)
			args.verbose = true;
		else if (isValueArgument(parser, "--sdkroot", optionValue)) {
			args.sdkroot = optionValue;
			args.defaultSdkroot = false;
		}
		else if (isValueArgument(parser, "--libpath-aplite", optionValue))
			args.libPath[ArgPlatform_Aplite] = optionValue;
		else if (isValueArgument(parser, "--libpath-basalt", optionValue))
			args.libPath[ArgPlatform_Basalt] = optionValue;
		else if (isValueArgument(parser, "--libpath-diorite", optionValue))
			args.libPath[ArgPlatform_Diorite] = optionValue;
		else if (isValueArgument(parser, "--libpath-chalk", optionValue))
			args.libPath[ArgPlatform_Chalk] = optionValue;
		else if (isValueArgument(parser, "--libpath-emery", optionValue))
			args.libPath[ArgPlatform_Emery] = optionValue;
		else if (strcmp(curArg, "--map-lib-functions") == 0)
			args.mapLibFunctions = true;
		else {
			std::cerr << "unknown option \"" << curArg << "\"" << std::endl;
			return false;
		}
	}

	// Read input/output paths
	if (parser.argi >= parser.argc && !args.mapLibFunctions) {
		std::cerr << "expected input file path" << std::endl;
		return false;
	}
	args.inputFile.assign(parser.argv[parser.argi++]);

	if (parser.argi < parser.argc)
		args.outputFile.assign(parser.argv[parser.argi++]);

	// Finish
	if (parser.argi < parser.argc)
		std::cerr << "unexpected arguments starting at \"" << parser.argv[parser.argi] << std::endl;

	return true;
}

/**
 * Platform informtion
 */

struct Platform {
	ArArchive libArchive;
	PblLibrary library;

	Platform(const char* name) : library(name) {
	}
};
typedef std::vector<Platform*> PlatformList;

PlatformList::iterator findPlatform(PlatformList& platforms, const char* name) {
	auto itPlatform = platforms.begin();
	for (; itPlatform != platforms.end(); ++itPlatform) {
		if (strcmp((*itPlatform)->library.getPlatformName(), name) == 0)
			return itPlatform;
	}
	return platforms.end();
}

bool loadPlatform(PlatformList& platforms, const char* platformName, const char* filename, bool verbose) {
	if (findPlatform(platforms, platformName) != platforms.end())
		return false;

	Platform* platform = new Platform(platformName);
	if (!platform->libArchive.load(filename))
		return false;
	if (!platform->library.loadFromArArchive(platform->libArchive, verbose))
		return false;
	platforms.push_back(platform);

	return true;
}

void cleanPlatforms(PlatformList& platforms) {
	auto itPlatform = platforms.begin();
	for (; itPlatform != platforms.end(); ++itPlatform) {
		delete *itPlatform;
	}
	platforms.clear();
}

/**
 * file utils
 */

bool isDirectory(const char* filename) {
	struct stat s;
	if (stat(filename, &s) < 0)
		return false;
	else
		return (s.st_mode & S_IFDIR) > 0;
}

bool isFile(const char* filename) {
	struct stat s;
	if (stat(filename, &s) < 0)
		return false;
	else
		return (s.st_mode & S_IFREG) > 0;
}

// cares about unix/windows path variants
std::string joinPath(const std::string& base, const char* spec) {
	std::string result = base;
	if (spec != nullptr) {
		if (*spec == '/' || *spec == '\\')
			spec++;
		if (base[base.length() - 1] != '/' && base[base.length() - 1] != '\\')
			result += std::string("/") + spec;
		else
			result += spec;
	}
#ifdef WIN32
	std::replace(result.begin(), result.end(), '/', '\\');
#else
	std::replace(result.begin(), result.end(), '\\', '/');

	if (result[0] == '~') {
		const char *homedir = getenv("HOME");
		if (homedir != nullptr) {
			result.erase(result.begin());
			result.insert(0, homedir);
		}
	}
#endif
	return result;
}

/**
 * Output helper
 */
void outputIndent(std::ostream& output, uint32_t indent) {
	while (indent-- > 0)
		output << INDENT_CHARACTER;
}

void outputBinary(std::ostream& output, PblAppBinary* binary, uint32_t indent) {
	output << "{" << std::endl;
	outputIndent(output, indent + INDENT_WIDTH);
	output << "\"usedAPIs\": [" << std::endl;
	for (uint32_t i = 0; i < binary->getUsedFunctionCount(); i++) {
		if (i > 0)
			output << "," << std::endl;
		outputIndent(output, indent + 2*INDENT_WIDTH);
		output << "\"" << binary->getUsedFunctionName(i) << "\"";
	}
	output << std::endl;
	outputIndent(output, indent + INDENT_WIDTH);
	output << "]" << std::endl;
	outputIndent(output, indent);
	output << "}";
}

void outputFunctions(std::ostream& output, PlatformList& platforms, uint32_t indent) {
	// merge platforms
	std::vector<std::vector<std::string>> functions; // symbol table with array of function names
	auto itPlatform = platforms.begin();
	for (; itPlatform != platforms.end(); ++itPlatform) {
		for (uint32_t i = 0; i < (*itPlatform)->library.getFunctionCount(); i++) {
			uint32_t symbolTableOff = (*itPlatform)->library.getFunctionSymbolTableOffset(i);
			if (symbolTableOff == UINT32_MAX)
				continue;
			functions.resize(symbolTableOff + 1);
			functions[symbolTableOff].push_back((*itPlatform)->library.getFunctionName(i));
		}
	}

	// output
	output << "[" << std::endl;
	auto itFunction = functions.begin();
	for (; itFunction != functions.end(); ++itFunction) {
		if (itFunction != functions.begin())
			output << "," << std::endl;
		outputIndent(output, indent + INDENT_WIDTH);
		
		if (itFunction->size() == 0)
			output << "[]";
		else if (itFunction->size() == 1)
			output << "\"" << itFunction->at(0) << "\"";
		else {
			output << "[" << std::endl;
			for (uint32_t i = 0; i < itFunction->size(); i++) {
				if (i > 0)
					output << "," << std::endl;
				outputIndent(output, indent + 2 * INDENT_WIDTH);
				output << "\"" << itFunction->at(i) << "\"";
			}
		}
	}
	output << std::endl;
	outputIndent(output, indent);
	output << "]";
}

/**
 * The entrypoint to this program
 */
int main(int argc, char* argv[]) {
	ProgramArguments args;
	if (!parseArguments(args, argc, argv))
		return 1;
	if (args.inputFile == "null" && !args.mapLibFunctions) {
		std::cerr << "Nothing to do." << std::endl;
		return 6;
	}

	PlatformList platforms;

	// Load from overwritten library paths
	for (int i = 0; i < ArgPlatformCount; i++) {
		if (args.libPath[i] != "") {
			if (!isFile(args.libPath[i].c_str()))
				std::cerr << "Could not open library for " << PlatformNames[i] << std::endl;
			else if (!loadPlatform(platforms, PlatformNames[i], args.libPath[i].c_str(), args.verbose))
				std::cerr << "Could not load library for " << PlatformNames[i] << std::endl;
		}
	}

	// Load from SDK root
	if (args.sdkroot != "") {
		std::string sdkRoot = joinPath(args.sdkroot, nullptr);
		if (!isDirectory(sdkRoot.c_str())) {
			if (args.defaultSdkroot)
				args.verbose && std::cerr << "Could not find any installed core sdk" << std::endl;
			else
				std::cerr << "Could not find specified core sdk" << std::endl;
		}
		else {
			bool foundSomeLib = false;
			std::string libDirPath = joinPath(sdkRoot, "pebble/");
			for (int i = 0; i < ArgPlatformCount; i++) {
				std::string libPath = joinPath(libDirPath, PlatformNames[i]);
				libPath = joinPath(libPath, "lib/libpebble.a");
				if (isFile(libPath.c_str())) {
					foundSomeLib = true;

					loadPlatform(platforms, PlatformNames[i], libPath.c_str(), args.verbose);
				}
			}

			if (!foundSomeLib && args.verbose)
				std::cerr << "Could not load any libraries from core sdk" << std::endl;
		}
	}

	if (platforms.size() == 0) {
		std::cerr << "Could not load any library" << std::endl;
		cleanPlatforms(platforms);
		return 2;
	}

	// Load pebble app and detect API functions
	PblAppArchive appArchive;
	std::vector<PblAppBinary*> binaries;
	if (args.inputFile != "null") {
		if (!appArchive.load(args.inputFile.c_str(), args.verbose)) {
			cleanPlatforms(platforms);
			return 3;
		}
		for (uint32_t i = 0; i < appArchive.getBinaryCount(); i++) {
			PlatformList::iterator itPlatform = findPlatform(platforms, appArchive.getBinaryPlatform(i));
			if (itPlatform == platforms.end()) {
				args.verbose && std::cerr << "Library for pebble binary \"" << appArchive.getBinaryPlatform(i) << "\" not loaded" << std::endl;
				continue;
			}

			// Load binary
			uint32_t size;
			void* buffer = appArchive.extractBinary(i, &size, args.verbose);
			if (!buffer)
				continue;
			PblAppBinary* binary = new PblAppBinary(buffer, size, &(*itPlatform)->library);

			args.verbose && std::cerr << "Scanning pebble binary \"" << appArchive.getBinaryPlatform(i) << "\"" << std::endl;
			uint32_t foundAPIs = binary->scan();
			args.verbose && std::cerr << "Found " << foundAPIs << " in pebble binary \"" << appArchive.getBinaryPlatform(i) << "\"" << std::endl;

			binaries.push_back(binary);
		}

		if (binaries.size() == 0) {
			std::cerr << "Could not scan any pebble binary" << std::endl;
			cleanPlatforms(platforms);
			return 4;
		}
	}

	// Output (as JSON)
	std::ofstream outputFile;
	if (args.outputFile != "") {
		outputFile.open(args.outputFile.c_str(), std::ostream::out);
		if (!outputFile) {
			std::cerr << "Could not open output file" << std::endl;
			return 5;
		}
	}
	std::ostream& output = (args.outputFile == "" ? std::cout : outputFile);

	output << "{" << std::endl;
	if (args.mapLibFunctions) {
		outputIndent(output, 1 * INDENT_WIDTH);
		output << "\"functions\": ";
		outputFunctions(output, platforms, 1 * INDENT_WIDTH);
		if (args.inputFile != "null")
			output << ",";
		output << std::endl;
	}

	if (args.inputFile != "null") {
		outputIndent(output, 1 * INDENT_WIDTH);
		output << "\"platforms\": {" << std::endl;
		for (uint32_t i = 0; i < binaries.size(); i++) {
			if (i > 0)
				output << "," << std::endl;
			outputIndent(output, 2 * INDENT_WIDTH);
			output << "\"" << binaries[i]->getPlatformName() << "\": ";
			outputBinary(output, binaries[i], 2 * INDENT_WIDTH);
		}
		output << std::endl;
		outputIndent(output, 1 * INDENT_WIDTH);
		output << "}" << std::endl;
	}
	output << "}" << std::endl;
	output.flush();

	// Clean up and go home
	for (uint32_t i = 0; i < binaries.size(); i++)
		delete binaries[i];
	binaries.clear();
	cleanPlatforms(platforms);

	return 0;
}
