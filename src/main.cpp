#include "pbw_api_info.h"

#include <iostream>
#include <string>

enum ArgPlatform {
	ArgPlatform_Aplite = 0,
	ArgPlatform_Basalt,
	ArgPlatform_Diorite,
	ArgPlatform_Chalk,

	ArgPlatformCount
};

struct ProgramArguments {
	bool verbose = false;
	bool ignoreSingleFailures = false;
	std::string sdkroot = "~/.pebble-sdk";
	std::string inputFile;
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
		<< "usage: pbw_api_info [options] inputfile [outputfile]" << std::endl
		<< "options:" << std::endl
		<< "  -h/--help             -> Shows this help screen and exits the program" << std::endl
		<< "  --sdkroot             -> Sets the path of the sdk *core*" << std::endl
		<< "  --libpath-<platform>  -> Overwrites the path of a single platform import library" << std::endl
		<< "    <platform> may be: aplite, basalt, diorite, chalk" << std::endl
		<< "  --singleFailures      -> Only returns failure if every platform binary processing failed" << std::endl
		<< "  -v/--verbose          -> Outputs detailed information about the progress to stderr" << std::endl
		;
}

/**
 * tests if current argument is <testArg>=<value> or <testArg> <value> and copies the value
 * if testArg is used, but no value is given, an error message is printed
 */
bool isValueArgument(ProgramArgumentParser& parser, const char* testArg, std::string& value) {
	const char* curArg = parser.argv[parser.argi - 1]; // we already incremented the index
	if (strstr(curArg, testArg) == curArg) {
		curArg += strlen(testArg);

		// confirm <testArg> <value>
		if (curArg[0] == '\0' && parser.argi + 1 < parser.argc && *parser.argv[parser.argi + 1] != '-') {
			value.assign(parser.argv[++parser.argi]);
			return true;
		}
		// confirm <testArg>=<value>
		else if (curArg[0] == '=' && curArg[1] != '\0') {
			value.assign(curArg + 2);
			return true;
		}
		else
			std::cerr << "expected a value for option \"" << testArg << "\"" << std::endl;
	}
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
		if (curArg[0] != '-')
			break;
		else if (strcmp(curArg, "-h") == 0 || strcmp(curArg, "--help") == 0) {
			printHelp();
			return false;
		}
		else if (strcmp(curArg, "-v") == 0 || strcmp(curArg, "--verbose") == 0)
			args.verbose = true;
		else if (isValueArgument(parser, "--sdkroot", optionValue))
			args.sdkroot = optionValue;
		else if (isValueArgument(parser, "--libpath-aplite", optionValue))
			args.libPath[ArgPlatform_Aplite] = optionValue;
		else if (isValueArgument(parser, "--libpath-basalt", optionValue))
			args.libPath[ArgPlatform_Basalt] = optionValue;
		else if (isValueArgument(parser, "--libpath-diorite", optionValue))
			args.libPath[ArgPlatform_Diorite] = optionValue;
		else if (isValueArgument(parser, "--libpath-chalk", optionValue))
			args.libPath[ArgPlatform_Chalk] = optionValue;
		else if (strcmp(curArg, "--singleFailures") == 0)
			args.ignoreSingleFailures = true;
		else {
			std::cerr << "unknown option \"" << curArg << "\"" << std::endl;
			return false;
		}
	}

	// Read input/output paths
	if (parser.argi >= parser.argc) {
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
 * The entrypoint to this program
 */
int main(int argc, char* argv[]) {
	using namespace std;
	ArArchive archive;
	if (!archive.load("test\\libpebble_basalt.a"))
		std::cerr << "Could not open archive" << endl;
	else {
		for (uint32_t i = 0; i < archive.getFileCount(); i++) {
			std::cerr << "\"" << archive.getFileName(i) << "\", size: " << archive.getFileSize(i) << endl;
		}
		std::cerr << endl;

		PblLibrary library("basalt");
		if (!library.loadFromArArchive(archive, true))
			std::cerr << "Could not load library" << endl;
		else {

			PblAppArchive app;
			if (!app.load("test\\simplicity.pbw.zip", true))
				std::cerr << "Could not open app" << endl;
			else {
				for (uint32_t i = 0; i < app.getBinaryCount(); i++) {
					if (strcmp(app.getBinaryPlatform(i), library.getPlatformName()) == 0) {
						uint32_t size;
						void* buf = app.extractBinary(i, &size, true);
						if (!buf)
							break;
						PblAppBinary bin(buf, size, &library);

						std::cerr << "Found usage of " << bin.scan() << " API functions" << endl;
						for (uint32_t j = 0; j < bin.getUsedFunctionCount(); j++) {
							std::cerr << "\t" << bin.getUsedFunctionName(j) << endl;
						}
					}
				}
			}
		}
	}

	/*ProgramArguments args;
	if (!parseArguments(args, argc, argv))
		return 1;*/
}
