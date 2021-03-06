/*
 * Parameters.hpp
 *
 *  Created on: 8 de mar de 2019
 *      Author: haroldo
 */

#ifndef PARAMETERS_HPP_
#define PARAMETERS_HPP_

#include <stddef.h>
#include <string>

#define MAX_DEPTH 10

/** strategy to fill missing results, if any */
enum FMRStrategy {Worse = 0,       // worse result
                  WorseT2,     // abs worse result times 2
                  WorseInst,   // worse result from instance
                  WorseInstT2, // worse result from instance times 2
                  AverageInst,
                  Value
};

enum Evaluation
{
    Average = 0, // when results of executions in
             // different instances are comparable,
             // such as execution time

    Rank     // if results for different instances
             // are not comparable, such as
             // objective functions with different
             // scales
};

const char *str_eval( const enum Evaluation eval );

const char *str_fmrs( const enum FMRStrategy fmrs );

class Parameters
{
public:
    static void parse( int argc, const char **argv );

    static void print();

    static void help();

    // how missing results in the results set
    // will be filled
    static enum FMRStrategy fmrStrategy;

    // based on average or rank
    static enum Evaluation eval;
    
    // if results for each instances should be
    // shifted so that the best result for this
    // instance is zero
    static bool bestIsZero;
    
    // turns worse of all results to 1
    // and best to zero
    static bool normalizeResults;

    // minimum absolute difference
    // between two results to change ranking
    static double rankEps;

    // minimum percentage difference between
    // two values to increase ranking
    static double rankPerc;

    // compute top "storeTop" configurations, just to
    // display summary
    static size_t storeTop;

    static int minElementsBranch;
    
    static double minPercElementsBranch;

    static size_t maxDepth;

    // minimum percentage performance improvement
    static double minPerfImprov;

    // minimum absolute performance improvement
    static double minAbsPerfImprov;

    static std::string instancesFile;

    static std::string resultsFile;
    
    // files to save info
    static std::string mipPDTFile;
    
    static std::string gtreeFile;

    static std::string treeFile;
    
    static std::string gtreeFileGV;

    static std::string treeFileGV;
    
    static std::string summFile;

    static std::string isetCSVNorm;

    static std::string isetCSVNormR;
    
    static std::string rsetCSV;
    
    static double maxSeconds;

    static double fillMissingValue;

    // when running algorithm filtering,
    // how many select
    static int maxAlgs;

    // in the algorithm filter, minimum number 
    // of algorithm configurations for
    // covering each problem instance
    static int afMinAlgsInst;

    // if only the greedy algorithm will be executed
    static bool onlyGreedy;
};

#endif /* PARAMETERS_HPP_ */

