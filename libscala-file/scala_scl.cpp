/****************************************************************
          libscala-file, (C) 2020 Mark Conway Wirt
            See LICENSE for licensing terms (MIT)
 ****************************************************************/

#include <iostream>
#include <fstream>
#include <regex>
#include <math.h>
#include <stdexcept>
#include <exception>

#include "scala_file.hpp"

namespace scala {
    scale read_scl(std::ifstream& input_file){ 
        /* 
        C++ Code to parse the Scala scale file, as documented here:

            http://www.huygens-fokker.org/scala/scl_format.html

        A couple of extensions:

        - Allow white-space before the comment character.  Spec is a little ambiguous.
        - Allow blank lines. In the standard only the scale name is mentioned as potentially blank.
        */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"        
        int non_commnets_processed = 0;
        unsigned long entries = 0;
        int numerator, denominator;
        std::string buffer;
        std::smatch match, trimmed;
        scale scala_scale;
        bool description_parsed = false ;
#pragma GCC diagnostic pop

#ifdef SCALA_STRICT
        std::regex COMMENT_REGEX = std::regex("^!.*");
#else
        std::regex COMMENT_REGEX = std::regex("[ \t]*!.*");
#endif
        try {
        while (input_file) {
            getline(input_file, buffer);
            if (std::regex_match (buffer, COMMENT_REGEX)) {
                // We're defining a comment as the first non-whitespace character being a "!".
            } else if (std::regex_match (buffer, std::regex("[ \t]*") )) {
                // Blank line. Discard. This may be an extension of the format.
#ifdef SCALA_STRICT
                if (description_parsed) {
                    // If we're at a blank line which is not the description, assume it's 
                    // a final linefeed at the end of the file.
                    break;
                }
#endif
                if (non_commnets_processed == 0){
                    description_parsed = true;
                }

                non_commnets_processed = non_commnets_processed + 1;

            } else {
                // Extract the part after optional leading whitespace and before an optional space and label
                std::regex_search(buffer, trimmed, std::regex("([ \t]*)([^ \t]*)(.*)"));
                std::string entry = trimmed[2];
                if (non_commnets_processed == 0) {
                    // First non-comment is the description. Can be ignored
                    non_commnets_processed = non_commnets_processed + 1;
                    description_parsed = true;
                    continue;
                }
                else if (non_commnets_processed == 1){
                    // Second non-comment line containers the number of entries.
                    entries  = std::stoul(entry);
                    non_commnets_processed = non_commnets_processed + 1;
                    continue;
                }
                else if (std::regex_match (entry, std::regex(".*[.]+.*") )) {
                    // Cent values *must* have a period. It's the law.
                    double cents = std::stod(entry);
                    scala_scale.add_degree(*new degree(cents));
                }
                else if (std::regex_match (entry, std::regex(".*[/]{1}.*") )) {
                    // A ratio
                    std::regex_search(entry, match, std::regex("(.*)/(.*)"));
                    numerator = std::stoi(match.str(1)); 
                    denominator = std::stoi(match.str(2)); 
                    scala_scale.add_degree(*new degree(numerator, denominator));
                } else if (std::regex_match (entry, std::regex("[0-9]*") )) {
                    // According to the standard, single numbers should be treated as ratios
                    numerator = std::stoi(entry); 
                    denominator = 1;
                    scala_scale.add_degree(*new degree(numerator, denominator));
                }
#ifdef SCALA_STRICT
                else {
                    // In strict mode we'll make sure to throw an error if the line
                    // can't be interpreted.  In lax mode we just give up and move on.
                    std::string message = "Scala parse error: cannot interpret: ";
                    throw std::runtime_error(message.append(buffer));
                }
#endif
            }
        }
        } catch (std::exception& e) {
            std::cout << e.what() << '\n';
        }
#ifdef SCALA_STRICT
        if (scala_scale.get_scale_length() != entries + 1){
            // If we make is here one of the entries probably didn't parse, but it wasn't
            // such that an error was thrown.  Strict adherence says you should throw an 
            // error on all file parse errors.
            throw std::runtime_error("Scala file parse error: Unexpected number of entries");
        }
#endif
        
        return scala_scale;
    }
}
