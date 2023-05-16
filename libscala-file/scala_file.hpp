/****************************************************************
          libscala-file, (C) 2020 Mark Conway Wirt
            See LICENSE for licensing terms (MIT)
 ****************************************************************/
#pragma once

#include <fstream>
#include <math.h>
#include <vector>


// Define this before compilation if you want a stricter adherence 
// to the specification.

// #define SCALA_STRICT

#define KBM_NON_ENTRY -1

namespace scala {

    struct degree {

        double ratio;

        degree (int n, int d){
            // Two inputs: a ratio
            ratio  = static_cast <double> (n) / static_cast <double> (d);
        }

        explicit degree (double cents){
            // One input: cents
            ratio = pow(pow(2, 1.0 / 12.0), cents/100.0);
        }

        double get_ratio() {
            // Use to get the value
            return ratio;
        }
    };

    struct scale {
        
        std::vector <degree> degrees;

        scale () {
            // The first degree is a scala file is always implicit. Make it explicit.
            degrees.push_back( *(new degree(0.0)));
        }

        ~scale(){
            degrees.clear();
            std::vector<degree>().swap(degrees);
        }

        scale(const scale &s2) {
            for (size_t i = 0; i < s2.degrees.size(); i++) {
                degrees.push_back(s2.degrees[i]); 
            }
        }

        scale& operator=(const scale &s2){
            std::vector <degree> new_degrees;
            for (size_t i = 0; i < s2.degrees.size(); i++) {
                new_degrees.push_back(s2.degrees[i]);
            }
            degrees.swap(new_degrees);
            new_degrees.clear();
            return *this;
        }

        void add_degree(degree d) {
            degrees.push_back(d);
        }

        double get_ratio(size_t i){
            return degrees[i].get_ratio();
        }

        size_t get_scale_length() {
            return degrees.size();
        }

    };

    struct kbm {
        double reference_frequency;
        int map_size;
        int first_note;
        int last_note;
        int middle_note;
        int reference_note;
        int octave_degree;
        std::vector <int> mapping;

        kbm(){
            map_size = 0;
            first_note = 0;
            last_note = 0;
            middle_note = 0;
            reference_note = 0;
            reference_frequency = 0.0;
            octave_degree = 0;
        }

        ~kbm(){
            mapping.clear();
            std::vector<int>().swap(mapping);
        }

        kbm( const kbm &k2){
            for (size_t i = 0; i < k2.mapping.size(); i++) {
                mapping.push_back(k2.mapping[i]); 
            }
            map_size = k2.map_size;
            first_note = k2.first_note;
            last_note = k2.last_note;
            middle_note = k2.middle_note;
            reference_note = k2.reference_note;
            reference_frequency = k2.reference_frequency;
            octave_degree = k2.octave_degree;
        }

        kbm& operator=(const kbm &k2){
            std::vector <int> new_mapping;
            for (size_t i = 0; i < k2.mapping.size(); i++) {
                new_mapping.push_back(k2.mapping[i]);
            }
            mapping.swap(new_mapping);
            new_mapping.clear();

            map_size = k2.map_size;
            first_note = k2.first_note;
            last_note = k2.last_note;
            middle_note = k2.middle_note;
            reference_note = k2.reference_note;
            reference_frequency = k2.reference_frequency;
            octave_degree = k2.octave_degree;

            return *this;
        }

        void add_mapping(int n) {
            mapping.push_back(n);
        }

    };

    scale read_scl(std::ifstream& input_file);
    kbm read_kbm(std::ifstream& input_file);
}
