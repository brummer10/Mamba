/****************************************************************
          libscala-file, (C) 2020 Mark Conway Wirt
            See LICENSE for licensing terms (MIT)
 ****************************************************************/

#include <iostream>
#include <regex>
#include <vector>
#include <stdexcept>
#include <exception>

#include "scala_file.hpp"

enum current_entry_map {
    MAP_SIZE,
    FIRST_NOTE,
    LAST_NOTE,
    MIDDLE_NOTE,
    REFERENCE_NOTE,
    REFERENCE_FREQUENCY,
    OCTAVE_DEGREE,
    ACTUAL_MAP
};

namespace scala {

    kbm read_kbm(std::ifstream& input_file){
        std::string buffer;
        unsigned int current_entry = 0;
        kbm keyboard_mapping;

#ifdef SCALA_STRICT
        std::regex COMMENT_REGEX = std::regex("^!.*");
        std::regex EX = std::regex("^[ \t]*[x]{1}.*");
#else
        std::regex COMMENT_REGEX = std::regex("[ \t]*!.*");
        std::regex EX = std::regex("^[ \t]*[xX]{1}.*");
#endif
    
        try {
        while(input_file){
            getline(input_file, buffer);
            if (std::regex_match (buffer, COMMENT_REGEX)) {
                // Ignore comments
            } else if (std::regex_match (buffer, std::regex("[ \t]*") )) {
                // Blank line. If we're in strict mode this means end of file.
                // Stop parsing.
#ifdef SCALA_STRICT
                break;
#endif
            } else {
                switch(current_entry){
                    case MAP_SIZE: 
                        keyboard_mapping.map_size = std::stoi(buffer);
                        current_entry += 1;
                        break;
                    case FIRST_NOTE: 
                        keyboard_mapping.first_note = std::stoi(buffer);
                        current_entry += 1;
                        break;
                    case LAST_NOTE: 
                        keyboard_mapping.last_note = std::stoi(buffer);
                        current_entry += 1;
                        break;
                    case MIDDLE_NOTE: 
                        keyboard_mapping.middle_note = std::stoi(buffer);
                        current_entry += 1;
                        break;
                    case REFERENCE_NOTE: 
                        keyboard_mapping.reference_note = std::stoi(buffer);
                        current_entry += 1;
                        break;
                    case REFERENCE_FREQUENCY: 
                        keyboard_mapping.reference_frequency = std::stod(buffer);
                        current_entry += 1;
                        break;
                    case OCTAVE_DEGREE: 
                        keyboard_mapping.octave_degree = std::stoi(buffer);
                        current_entry += 1;
                        break;
                    case ACTUAL_MAP:
                        // This one is a little more complicated.  If we have an x
                        if (std::regex_match (buffer, EX)) {
                            // A non-entry
                            keyboard_mapping.add_mapping(KBM_NON_ENTRY);
                        } else {
                            // A mapping entry
                            keyboard_mapping.add_mapping(std::stoi(buffer));
                        }
                }
            }
        }
        if (static_cast <size_t> (keyboard_mapping.map_size) != keyboard_mapping.mapping.size()){
            // This is an error, strict mode or not 
            if (static_cast <size_t> (keyboard_mapping.map_size) < keyboard_mapping.mapping.size()){
                throw std::runtime_error("ERROR: Too few entires in mapping file");
            } else {
                throw std::runtime_error("ERROR: Too many entires in mapping file");
            }
        }
        } catch (std::exception& e) {
            std::cout << e.what() << '\n';
        }
        return keyboard_mapping;
    }
}
