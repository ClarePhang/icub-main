/*
 * Copyright (C) 2010-2011 RobotCub Consortium
 * Author: Andrea Del Prete
 * CopyPolicy: Released under the terms of the GNU GPL v2.0.
 *
 */

#include <iostream>
#include <iomanip>
#include <string>
#include "iCub/skinDynLib/common.h"

using namespace std;
using namespace yarp::sig;
using namespace iCub::skinDynLib;

#ifdef OPTION1
map<SkinPart,BodyPart> iCub::skinDynLib::createSkinPart_2_BodyPart()
{
    map<SkinPart,BodyPart> m;
    m[UNKNOWN_SKIN_PART]    = UNKNOWN_BODY_PART;
    m[LEFT_HAND]            = LEFT_ARM;
    m[LEFT_FOREARM]         = LEFT_ARM;
    m[LEFT_UPPER_ARM]       = LEFT_ARM;
    m[RIGHT_HAND]           = RIGHT_ARM;
    m[RIGHT_FOREARM]        = RIGHT_ARM;
    m[RIGHT_UPPER_ARM]      = RIGHT_ARM;
    m[FRONT_TORSO]          = TORSO;
    return m;
}

vector<SkinPart> iCub::skinDynLib::getSkinParts(BodyPart b)
{
    vector<SkinPart> res;
    for(map<SkinPart,BodyPart>::const_iterator it=SkinPart_2_BodyPart.begin(); it!=SkinPart_2_BodyPart.end(); it++)
        if(it->second==b)
            res.push_back(it->first);
    return res;
}

BodyPart iCub::skinDynLib::getBodyPart(SkinPart s)
{   
    return SkinPart_2_BodyPart.at(s);
}
#endif

#ifdef OPTION2
vector<SkinPart> iCub::skinDynLib::getSkinParts(BodyPart b)
{
    vector<SkinPart> res;
    for(unsigned int i=0; i<SKIN_PART_SIZE; i++)
        if(SkinPart_2_BodyPart[i].body==b)
            res.push_back(SkinPart_2_BodyPart[i].skin);
    return res;
}

BodyPart iCub::skinDynLib::getBodyPart(SkinPart s)
{   
    for(unsigned int i=0; i<SKIN_PART_SIZE; i++)
        if(SkinPart_2_BodyPart[i].skin==s)
            return SkinPart_2_BodyPart[i].body;
    return BODY_PART_UNKNOWN;
}
#endif


#ifdef OPTION3
vector<SkinPart> iCub::skinDynLib::getSkinParts(BodyPart b)
{
    vector<SkinPart> res;
    for(map<SkinPart,BodyPart>::const_iterator it=A::SkinPart_2_BodyPart.begin(); it!=A::SkinPart_2_BodyPart.end(); it++)
        if(it->second==b)
            res.push_back(it->first);
    return res;
}

BodyPart iCub::skinDynLib::getBodyPart(SkinPart s)
{   
    return A::SkinPart_2_BodyPart.at(s);
}
#endif

int iCub::skinDynLib::getLinkNum(SkinPart s)
{   
    for(unsigned int i=0; i<SKIN_PART_SIZE; i++)
        if(SkinPart_2_LinkNum[i].skin==s)
            return SkinPart_2_LinkNum[i].linkNum;
    return -1;
}

yarp::sig::Vector iCub::skinDynLib::toVector(yarp::sig::Matrix m)
{
    Vector res(m.rows()*m.cols(),0.0);
    
    for (size_t r = 0; r < m.rows(); r++)
    {
        res.setSubvector(r*m.cols(),m.getRow(r));
    }

    return res;
}

yarp::sig::Vector iCub::skinDynLib::vectorFromBottle(const yarp::os::Bottle b, int in, const int size)
{
    yarp::sig::Vector v(size,0.0);

    for (int i = 0; i < size; i++)
    {
        v[i] = b.get(in).asDouble();
        in++;
    }
    return v;
}

void iCub::skinDynLib::vectorIntoBottle(const yarp::sig::Vector v, yarp::os::Bottle &b)
{
    for (unsigned int i = 0; i < v.size(); i++)
    {
        b.addDouble(v[i]);
    }
}

yarp::sig::Matrix iCub::skinDynLib::matrixFromBottle(const yarp::os::Bottle b, int in, const int r, const int c)
{
    yarp::sig::Matrix m(r,c);
    m.zero();
    
    for (size_t i = 0; i<r; i++)
    {
        for (size_t j = 0; j<c; j++)
        {
            m(i,j) =  b.get(in).asDouble();
            in++;
        }
    }
    
    return m;
}

void iCub::skinDynLib::matrixIntoBottle(const yarp::sig::Matrix m, yarp::os::Bottle &b)
{
    Vector v = toVector(m);
    
    for (unsigned int i = 0; i < v.size(); i++)
    {
        b.addDouble(v[i]);
    }
}
