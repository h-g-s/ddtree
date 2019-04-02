/*
 * Greedy.cpp
 *
 *  Created on: 2 de abr de 2019
 *      Author: haroldo
 */

#include "Greedy.hpp"
#include  "InstanceSet.hpp"
#include  "ResultsSet.hpp"
#include  "Tree.hpp"
#include  "Parameters.hpp"
#include  "SubSetResults.hpp"

#include <vector>
#include <cassert>
#include <cfloat>
#include <algorithm>
#include <utility>
#include <cmath>
#include <cstring>

using namespace std;

typedef struct 
{
    size_t el;
    double val;
} ElVal;

class SplitInfo
{
public:
    SplitInfo( size_t nAlgs, size_t nInsts ) :
        sumRL( new long double[nAlgs] ),
        sumRR( new long double[nAlgs] ),
        elv(new ElVal[nInsts]),
        splitCost(DBL_MAX),
        idxFeature(numeric_limits<size_t>::max())
    {}

    virtual ~SplitInfo() {
        delete[] sumRL;
        delete[] sumRR;
    }

    long double *sumRL;
    long double *sumRR;
    ElVal *elv;
    long double splitCost;
    size_t idxFeature;
    size_t nElLeft;
};

class GNodeData {
public:
    GNodeData( const InstanceSet *_iset, const ResultsSet *_rset ) :
        iset_(_iset),
        rset_(_rset),
        sumResR( new long double[rset_->algsettings().size()] ),
        sumResL( new long double[rset_->algsettings().size()] ),
        elv( new ElVal[iset_->size()] ),
        nEl(iset_->size()),
        nElLeft(0),
        bestSplit(SplitInfo(rset_->algsettings().size(), iset_->size()))
    { 
        for ( size_t i=0 ; (i<iset_->size()) ; ++i )
            elv[i].el = i;
        for ( size_t i=0 ; (i<rset_->algsettings().size()) ; ++i )
            sumResR[i] = rset_->results().sum()[i];
        for ( size_t i=0 ; (i<rset_->algsettings().size()) ; ++i )
            sumResL[i] = 0.0;
    }

    virtual ~GNodeData() {
        delete[] elv;
        delete[] sumResR;
        delete[] sumResL;
    }

    void moveInstanceLeft( size_t idxInst )
    {
        for ( size_t ia=0 ; (ia<rset_->algsettings().size()) ; ++ia )
        {
            sumResR[ia] -= ((long double)rset_->res(idxInst, ia));
            sumResL[ia] += ((long double)rset_->res(idxInst, ia));
        }
    }

    void updateBestAlg() {
        long double costBestAlgL = DBL_MAX;

        for ( size_t ia=0 ; (ia<rset_->algsettings().size()) ; ++ia )
            if (sumResL[ia]<costBestAlgL)
                costBestAlgL = sumResL[ia];

        long double costBestAlgR = DBL_MAX;

        for ( size_t ia=0 ; (ia<rset_->algsettings().size()) ; ++ia )
            if (sumResR[ia]<costBestAlgR)
                costBestAlgR = sumResR[ia];

        splitCost = costBestAlgL + costBestAlgR;

        if (splitCost<bestSplit.splitCost) {
            bestSplit.splitCost = splitCost;
            bestSplit.nElLeft = this->nElLeft;
            memcpy( bestSplit.sumRL, this->sumResL, sizeof(long double)*rset_->algsettings().size() );
            memcpy( bestSplit.sumRR, this->sumResR, sizeof(long double)*rset_->algsettings().size() );
            bestSplit.idxFeature = idxFeature;
            memcpy( bestSplit.elv, this->elv, sizeof(ElVal)*this->nEl );
        }
    }

    double cutValue() const {
        // is in a valid branching position
        assert( nElLeft >=1 and nElLeft<nEl );

        return elv[((int)nElLeft)-1].val;
    }

    bool next() {
        moveInstanceLeft( elv[nElLeft].el );

        while ( nElLeft<nEl and nElLeft<Parameters::minElementsBranch )
        {
            ++nElLeft;
            if (nElLeft>=nEl)
                return false;
            if ( nEl-nElLeft<Parameters::minElementsBranch )
                return false;
            double v = elv[nElLeft].val;

            while (nElLeft<nEl and elv[nElLeft].val==v)
            {
                ++nElLeft;
                if (nElLeft>=nEl)
                    return false;
                if ( nEl-nElLeft<Parameters::minElementsBranch )
                    return false;

                if (elv[nElLeft].val==v)
                    moveInstanceLeft( elv[nElLeft].el );
            }

            // arrived in a different value ready for branching

            return true;
        }

        return false;
    }

    const InstanceSet *iset_;
    const ResultsSet *rset_;
    
    size_t idx;
    long double *sumResR;
    long double *sumResL;
    ElVal *elv;
    size_t nEl;

    size_t nElLeft;

    long double splitCost;

    // feature being branched on
    size_t idxFeature;

    SplitInfo bestSplit;
};

Greedy::Greedy (const InstanceSet *_iset, const ResultsSet *_rset) :
    iset_(_iset),
    rset_(_rset),
    ndata(nullptr),
    tnodes(0),
    maxDepth(Parameters::maxDepth)
{
    for ( size_t i=0 ; (i<maxDepth) ; ++i )
        tnodes += (size_t)pow( 2.0, i)+1e-10;

    ndata = new GNodeData*[tnodes];
    for ( size_t i=0 ; (i<tnodes) ; ++i ) 
    {
        ndata[i] = new GNodeData(iset_, rset_);
        ndata[i]->idx = 0;
    }
}

Tree *Greedy::build()
{
    vector< pair< size_t, Node *> > nqueue;

    Tree *res = new Tree(iset_, rset_);

    Node *root = res->create_root();

    nqueue.push_back( make_pair((size_t)0, root) );

    while (nqueue.size())
    {
        pair< size_t, Node *> np = nqueue.back();
        nqueue.pop_back();

        size_t idxLeft = 2*np.first+1;
        size_t idxRight = 2*np.first+1;

        // if children will be will within max depth
        if (idxLeft >= tnodes) 
            break;

        GNodeData *gnd = ndata[np.first];

        gnd->bestSplit.splitCost = DBL_MAX;
        gnd->bestSplit.idxFeature = numeric_limits<size_t>::max();
        for ( size_t idxFeature=0 ; (idxFeature<iset_->features().size()) ; ++idxFeature )
        {
            prepareBranch( np.first, idxFeature );
            while (gnd->next())
                gnd->updateBestAlg();
        }
        
        // found a valid branch
        if (gnd->bestSplit.idxFeature != numeric_limits<size_t>::max())
        {
            // recover to best state
            gnd->nElLeft = gnd->bestSplit.nElLeft;
            memcpy( gnd->sumResL, gnd->bestSplit.sumRL, sizeof(long double)*rset_->algsettings().size() );
            memcpy( gnd->sumResR, gnd->bestSplit.sumRR, sizeof(long double)*rset_->algsettings().size() );
            gnd->splitCost = gnd->bestSplit.splitCost;
            memcpy(gnd->elv , gnd->bestSplit.elv, sizeof(ElVal)*gnd->nEl );
            gnd->idxFeature = gnd->bestSplit.idxFeature;
        }
    }

    return res;
}

bool compElVal(const ElVal &lhs, const ElVal &rhs) { return lhs.val < rhs.val; }

void Greedy::prepareBranch( size_t n, size_t f )
{
    GNodeData *gnd = ndata[n];

    gnd->nElLeft = 0;
    gnd->idxFeature = f;


    if (gnd->idx==0)
    {
        // root node
        for ( size_t i=0 ; (i<rset_->algsettings().size()) ; ++i )
            gnd->sumResR[i] = rset_->results().sum()[i];
        for ( size_t i=0 ; (i<rset_->algsettings().size()) ; ++i )
            gnd->sumResL[i] = 0.0;

    }
    else
    {
        size_t parent = (((int)n)-1)/2;
        bool isLeft = (n%2);
        const long double *srp = (isLeft) ? ndata[parent]->sumResL : ndata[parent]->sumResR;
        memcpy( gnd->sumResR, srp, sizeof(long double)*rset_->algsettings().size() );
        for ( size_t i=0 ; (i<rset_->algsettings().size() ) ; ++i )
            gnd->sumResL[i] = 0.0;
    }

    for ( size_t i=0 ; (i<gnd->nEl) ; ++i )
        gnd->elv[i].val = iset_->instance(gnd->elv[i].el).float_feature(f);
    
    std::sort( gnd->elv, gnd->elv+gnd->nEl, compElVal );

    gnd->nElLeft = 0;
}

Greedy::~Greedy ()
{
    for ( size_t i=0 ; (i<tnodes) ; ++i )
        delete ndata[i];
    delete[] ndata;
}