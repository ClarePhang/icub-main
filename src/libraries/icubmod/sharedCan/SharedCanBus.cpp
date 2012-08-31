/*
 * Copyright (C) 2012 RobotCub Consortium
 * Author: Alessandro Scalzo
 * CopyPolicy: Released under the terms of the GNU GPL v2.0.
 *
 */

#include <vector>

#include <yarp/os/Property.h>
#include <yarp/os/RateThread.h>
#include <yarp/dev/PolyDriver.h>

#include "SharedCanBus.h"

class SharedCanBus : public yarp::os::RateThread
{
private:
    SharedCanBus() : RateThread(10), writeMutex(1), configMutex(1)
    {
        initialized=false;

        for (int i=0; i<0x800; ++i) reqIdsUnion[i]=UNREQ;
    }

public:
    ~SharedCanBus()
    {
        stop();

        polyDriver.close();
    }

    static SharedCanBus& getInstance()
    {
        static SharedCanBus instance;

        return instance;
    }

    void attachAccessPoint(yarp::dev::CanBusAccessPoint* ap)
    {
        configMutex.wait();
        accessPoints.push_back(ap);
        configMutex.post();
    }

    void detachAccessPoint(yarp::dev::CanBusAccessPoint* ap)
    {
        configMutex.wait();

        int n=accessPoints.size()-1;

        yarp::dev::CanBusAccessPoint* dap=NULL;

        for (int i=0; i<=n; ++i)
        {
            if (ap==accessPoints[i])
            {
                dap=accessPoints[i];
                accessPoints[i]=accessPoints[n];
                accessPoints.pop_back();

                break;
            }
        }

        if (dap)
        {
            for (int id=0; id<0x800; ++id)
            {
                if (dap->hasId(id)) canIdDelete(id);
            }

            delete dap;
        }

        if (accessPoints.size()==0)
        {
            // should close the driver here?
        }

        configMutex.post();
    }

    void run()
    {
        static const unsigned int MAX_MSGS=1024;
        static const bool NOWAIT=false;
        unsigned int msgsNum=0;

        configMutex.wait();

        if (!initialized)
        {
            fprintf(stderr, "Error: can bus driver not initilized, could not read from bus\n");
            configMutex.post();
            return;
        }

        bool ret=theCanBus->canRead(readBufferUnion,MAX_MSGS,&msgsNum,NOWAIT);

        if (ret)
        {
            for (unsigned int i=0; i<msgsNum; ++i)
            {
                unsigned int id=readBufferUnion[i].getId();

                for (unsigned int p=0; p<accessPoints.size(); ++p)
                {
                    if (accessPoints[p]->hasId(id))
                    {
                        accessPoints[p]->pushReadMsg(readBufferUnion[i]);    
                    }
                }
            }
        }

        configMutex.post();
    }

    bool canWrite(const yarp::dev::CanBuffer &msgs, unsigned int size, unsigned int *sent, bool wait)
    {
        writeMutex.wait();
        bool ret=theCanBus->canWrite(msgs,size,sent,wait);
        writeMutex.post();

        return ret;
    }

    void canIdAdd(unsigned int id)
    {
        configMutex.wait();

        if (reqIdsUnion[id]==UNREQ)
        {
            reqIdsUnion[id]=REQST;
            theCanBus->canIdAdd(id);
        }

        configMutex.post();
    }

    void canIdDelete(unsigned int id)
    {
        configMutex.wait();

        if (reqIdsUnion[id]==REQST)
        {
            for (int i=0; i<(int)accessPoints.size(); ++i)
            {
                if (accessPoints[i]->hasId(id))
                {
                    configMutex.post();
                    return;
                }
            }

            reqIdsUnion[id]=UNREQ;

            theCanBus->canIdDelete(id);
        }

        configMutex.post();
    }
    
    yarp::dev::ICanBus* getCanBus()
    {
        return theCanBus;
    }

    yarp::dev::ICanBufferFactory* getCanBufferFactory()
    {
        return theBufferFactory;
    }

    yarp::dev::ICanBusErrors* getCanBusErrors()
    {
        return theCanBusErrors;
    }

    bool open(yarp::os::Searchable &config)
    {
        configMutex.wait();

        if (initialized)
        {
            configMutex.post();
            return true;
        }

        if (!config.check("carrier"))
        {
            fprintf(stderr, "Error: could not find low level can driver specification\n");
            configMutex.post();         
            return false;
        }

        yarp::os::ConstString carrier=config.find("carrier").asString();

        yarp::os::Property prop;
        prop.fromString(config.toString().c_str());

        prop.unput("carrier");
        prop.unput("device");
        prop.put("device",carrier.c_str());

        // low level driver
        polyDriver.open(prop);
    
        if (!polyDriver.isValid())
        {
            fprintf(stderr, "Error: could not instantiate can device\n");
            configMutex.post();
            return false;
        }

        polyDriver.view(theCanBus);

        if (theCanBus==NULL)
        {
            fprintf(stderr, "Error: could not get ICanBus interface\n");
            configMutex.post();
            return false;
        }

        polyDriver.view(theBufferFactory);

        if (theBufferFactory==NULL)
        {
            fprintf(stderr, "Error: could not get ICanBufferFactory interface\n");
            configMutex.post();
            return false;
        }

        polyDriver.view(theCanBusErrors);

        readBufferUnion=theBufferFactory->createBuffer(BUF_SIZE);

        initialized=start();

        configMutex.post();

        return initialized;
    }

private:
    yarp::os::Semaphore writeMutex;
    yarp::os::Semaphore configMutex;

    bool initialized;
    yarp::dev::PolyDriver polyDriver;

    yarp::dev::ICanBus           *theCanBus;
    yarp::dev::ICanBufferFactory *theBufferFactory;
    yarp::dev::ICanBusErrors     *theCanBusErrors;

    std::vector<yarp::dev::CanBusAccessPoint*> accessPoints;

    yarp::dev::CanBuffer readBufferUnion;

    char reqIdsUnion[0x800];
};



//////////////////////////////
// CanBusAccessPoint methods
//////////////////////////////

bool yarp::dev::CanBusAccessPoint::open(yarp::os::Searchable& config)
{
    if (!SharedCanBus::getInstance().open(config)) return false;

    SharedCanBus::getInstance().attachAccessPoint(this);

    return true;
}

bool yarp::dev::CanBusAccessPoint::close()
{
    SharedCanBus::getInstance().detachAccessPoint(this);

    return true;
}

bool yarp::dev::CanBusAccessPoint::canWrite(const CanBuffer &msgs, unsigned int size, unsigned int *sent, bool wait)
{
    return SharedCanBus::getInstance().canWrite(msgs,size,sent,wait);
}

bool yarp::dev::CanBusAccessPoint::canGetBaudRate(unsigned int *rate)
{
    return SharedCanBus::getInstance().getCanBus()->canGetBaudRate(rate);
}

bool yarp::dev::CanBusAccessPoint::canIdAdd(unsigned int id)
{
    if (id>=0x800)
    {
        fprintf(stderr, "Error: Id=%d is out of 11 bit address range\n",id);

        return false;
    }

    reqIds[id]=REQST;

    SharedCanBus::getInstance().canIdAdd(id);

    return true;
}

bool yarp::dev::CanBusAccessPoint::canIdDelete(unsigned int id)
{
    if (id>=0x800)
    {
        fprintf(stderr, "Error: Id=%d is out of 11 bit address range\n",id);

        return false;
    }

    reqIds[id]=UNREQ;

    SharedCanBus::getInstance().canIdDelete(id);

    return true;
}

yarp::dev::CanBuffer yarp::dev::CanBusAccessPoint::createBuffer(int nmessage)
{
    return SharedCanBus::getInstance().getCanBufferFactory()->createBuffer(nmessage);
}

void yarp::dev::CanBusAccessPoint::destroyBuffer(CanBuffer &msgs)
{
    SharedCanBus::getInstance().getCanBufferFactory()->destroyBuffer(msgs);
}