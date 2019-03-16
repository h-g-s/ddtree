/*
 * Node.cpp
 *
 *  Created on: 7 de mar de 2019
 *      Author: haroldo
 */

#include "Node.hpp"
#include "Dataset.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

#include "Dataset.hpp"
#include "FeatureBranching.hpp"
#include "Parameters.hpp"
#include "ResultsSet.hpp"
#include "tinyxml2.h"



using namespace std;

Node::Node( const InstanceSet &_iset, const ResultsSet &_rset ) :
    iset_(_iset),
    rset_(_rset),
    ssres_(_rset.results()),
    ossr(nullptr),
    nEl_(iset_.size()),
    el_(new size_t[iset_.size()]),
    parent_(nullptr),
    depth(0),
    idx(0)
{
    for (size_t i=0 ; (i<nEl_) ; ++i )
        el_[i] = i;

    id = "root";
}

Node::Node( const Node *_parent, size_t _nEl, const size_t *_el, const SubSetResults &_ssres, size_t _idx ) :
    iset_(_parent->iset_),
    rset_(_parent->rset_),
    ssres_(_ssres),
    ossr(nullptr),
    nEl_(_nEl),
    el_(new size_t[_nEl]),
    parent_(_parent),
    depth(_parent->depth+1),
    idx(_idx)
{
    assert(ssres_.nElSS == nEl_ );
    memcpy(el_, _el, sizeof(size_t)*nEl_ );
    stringstream ss;
    ss << "nL" << depth << "I" << idx;
    id = ss.str();
}

std::vector<Node *> &Node::perform_branch()
{
    if (child_.size())
    {
        cerr << "branch was already done" << endl;
        abort();
    }

    if (this->depth >= Parameters::maxDepth)
        return child_;

//    if (ssres_.eval_ == Rank and ssres_.bestAlgRes() <= 1.00001)

    bestBranch_.parentCost_ = this->ssres_.bestAlgRes();
    bestBranch_.eval_ = numeric_limits<double>::max();

    const size_t nFeatures = iset_.features().size();
    for ( size_t idxF=0 ; (idxF<nFeatures) ; ++idxF )
    {
        if (iset_.feature_is_integer(idxF))
        {
            FeatureBranching<int> fbi(iset_, rset_, idxF, el_, nEl_, ssres_, Parameters::minElementsBranch, Parameters::maxEvalBranches[this->depth]);
            if (fbi.branch_values().size())
            {
                do
                {
                    bestBranch_.update_best_branching(fbi);
                } while (fbi.next());
            }
        }
        else
        {
            if (iset_.types()[idxF]==Float)
            {
                FeatureBranching<double> fbf(iset_, rset_, idxF, el_, nEl_, ssres_, Parameters::minElementsBranch, Parameters::maxEvalBranches[this->depth]);
                if (fbf.branch_values().size())
                {
                    do
                    {
                        bestBranch_.update_best_branching(fbf);
                    } while (fbf.next());
                }
            }
            else
            {
                cerr << "Type " << iset_.types()[idxF] <<
                    " of column " << iset_.features()[idxF] <<
                    " not handled yet" << endl;
                abort();
            }
        }
    }

    if (bestBranch_.found())
    {
        size_t nElLeft = bestBranch_.branches()[0].size();
        const size_t *elLeft = &(bestBranch_.branches()[0][0]);
        size_t nElRight = bestBranch_.branches()[1].size();
        const size_t *elRight = &(bestBranch_.branches()[1][0]);

        assert(nElLeft+nElRight == nEl_);

        size_t pidx = idx*2;
        Node *nodeLeft = new Node(this, nElLeft, elLeft, bestBranch_.ssrLeft, pidx  );
        Node *nodeRight = new Node(this, nElRight, elRight, bestBranch_.ssrRight, pidx + 1);
        child_.push_back( nodeLeft );
        child_.push_back( nodeRight );
    }

    return child_;
}

using namespace tinyxml2;

void Node::writeXML(tinyxml2::XMLDocument *doc, tinyxml2::XMLElement *parent ) const
{
    XMLElement *node = doc->NewElement("node");
    parent->InsertEndChild(node);
    node->SetAttribute("depth", (int) this->depth);
    node->SetAttribute("bestAlg", rset_.algsettings()[this->ssres_.bestAlg()].c_str());

    XMLElement *insts = doc->NewElement("instances");
    node->InsertEndChild(insts);
    for ( size_t i=0 ; (i<n_elements()) ; ++i )
    {
        auto inst = iset_.instance(elements()[i]);
        auto instEl = doc->NewElement("instance");
        instEl->SetAttribute("name", inst.name() );
        insts->InsertEndChild(instEl);
    }

    if (this->bestBranch_.found())
    {
        auto elb = doc->NewElement("branching");
        elb->SetAttribute( "feature", iset_.features()[bestBranch_.idxF_].c_str() );
        switch (bestBranch_.type())
        {
            case Integer:
                elb->SetAttribute("value", bestBranch_.get_int() );
                break;
            case Float:
                elb->SetAttribute("value", bestBranch_.get_float() );
                break;
            case String:
                elb->SetAttribute("value", bestBranch_.get_str() );
                break;
            default:
                cout << "branching type not supported" << endl;
                break;
        }
        node->InsertEndChild(elb);
    }

    node->SetAttribute("id", this->id.c_str() );

    for ( const auto &n : child_ )
        n->writeXML( doc, node );
}


Node::~Node ()
{
    delete[] el_;
    for ( auto ch : child_ )
        delete ch;

    if (ossr)
        delete ossr;
}
