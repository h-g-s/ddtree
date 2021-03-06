/*
 * SubSetResults.hpp
 *
 *  Created on: 9 de mar de 2019
 *      Author: haroldo
 */


#ifndef SUBSETRESULTS_HPP_
#define SUBSETRESULTS_HPP_

#include "ResultsSet.hpp"

class ResultsSet;

typedef long double SumType;

class SubSetResults
{
public:
    SubSetResults ( const ResultsSet *_rset,
                    const Evaluation _eval = Parameters::eval,
                    bool addElements = true,
                    size_t n_elements = 0,
                    const size_t *elements = nullptr
                    );

    SubSetResults( const SubSetResults &other );

    SubSetResults();

    SubSetResults &operator=(const SubSetResults &other);

    void add( size_t n, const size_t *el );

    void remove( size_t n, const size_t *el );

    size_t bestAlg() const {
        return this->idxBestAlg_;
    }

    double bestAlgRes() const {
        return this->resBestAlg_;
    }

    double resAlg( size_t idxAlg ) const;

    // algorithms/settings sorted from
    // best to worse
    std::vector< size_t > computeBestAlgorithms() const;

    // number of instances in this subset
    size_t nElSS;

    const SumType *sum() const {
        return sum_;
    }

    virtual ~SubSetResults ();
private:
    void updateBest();

    size_t idxBestAlg_;
    double resBestAlg_;

    const ResultsSet *rset_;
    Evaluation eval_;
    SumType *sum_;
    friend class Node;
    friend class Branching;
};

#endif /* SUBSETRESULTS_HPP_ */
