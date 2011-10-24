

#include <iCub/utils.h>




//--------------- Object Properties Port ------------------//


bool ObjectPropertiesCollectorPort::getStereoPosition(const string &obj_name, Vector &stereo)
{
    //if the object property collector port is connected use it to obtain the object 2D position
    if(this->getInputCount()==0)
        return false;

    //ask for the object's id
    Bottle bAsk,bGet,bReply;
    bAsk.addVocab(Vocab::encode("ask"));
    Bottle &bTempAsk=bAsk.addList().addList();
    bTempAsk.addString("name");
    bTempAsk.addString("==");
    bTempAsk.addString(obj_name.c_str());

    this->write(bAsk,bReply);

    if(bReply.size()==0 ||
       bReply.get(0).asVocab()!=Vocab::encode("ack") ||
       bReply.get(1).asList()->check("id") ||
       bReply.get(1).asList()->find("id").asList()->size()==0)
        return false;

    bGet.addVocab(Vocab::encode("get"));
    Bottle &bTempGet=bGet.addList().addList();
    bTempGet.addString("id");
    bTempGet.addInt(bReply.get(1).asList()->find("id").asList()->get(0).asInt());

    this->write(bGet,bReply);

    if(bReply.size()==0 || bReply.get(0).asVocab()!=Vocab::encode("ack"))
        return false;

    if(!bReply.get(1).asList()->check("position_2d_left") && !bReply.get(1).asList()->check("position_2d_right"))
        return false;

    stereo.resize(4);
    stereo=0.0;

    if(bReply.get(1).asList()->check("position_2d_left"))
    {
        Bottle *bStereo=bReply.get(1).asList()->find("position_2d_left").asList();

        stereo[0]=0.5*(bStereo->get(0).asDouble()+bStereo->get(2).asDouble());
        stereo[1]=0.5*(bStereo->get(1).asDouble()+bStereo->get(3).asDouble());
    }

    if(bReply.get(1).asList()->check("position_2d_right"))
    {
        Bottle *bStereo=bReply.get(1).asList()->find("position_2d_right").asList();

        stereo[2]=0.5*(bStereo->get(0).asDouble()+bStereo->get(2).asDouble());
        stereo[3]=0.5*(bStereo->get(1).asDouble()+bStereo->get(3).asDouble());
    }

    return true;
}


bool ObjectPropertiesCollectorPort::getCartesianPosition(const string &obj_name, Vector &x)
{
    //if the object property collector port is connected use it to obtain the object 2D position
    if(this->getInputCount()==0)
        return false;

    //ask for the object's id
    Bottle bAsk,bGet,bReply;
    bAsk.addVocab(Vocab::encode("ask"));
    Bottle &bTempAsk=bAsk.addList().addList();
    bTempAsk.addString("name");
    bTempAsk.addString("==");
    bTempAsk.addString(obj_name.c_str());

    this->write(bAsk,bReply);

    if(bReply.size()==0 ||
       bReply.get(0).asVocab()!=Vocab::encode("ack") ||
       bReply.get(1).asList()->check("id") ||
       bReply.get(1).asList()->find("id").asList()->size()==0)
        return false;

    bGet.addVocab(Vocab::encode("get"));
    Bottle &bTempGet=bGet.addList().addList();
    bTempGet.addString("id");
    bTempGet.addInt(bReply.get(1).asList()->find("id").asList()->get(0).asInt());

    this->write(bGet,bReply);

    if(bReply.size()==0 || bReply.get(0).asVocab()!=Vocab::encode("ack"))
        return false;

    if(!bReply.get(1).asList()->check("position_3d"))
        return false;

    x.resize(3);

    if(bReply.get(1).asList()->check("position_3d"))
    {
        Bottle *bX=bReply.get(1).asList()->find("position_3d").asList();

        for(int i=0; i<bX->size(); i++)
            x[i]=bX->get(i).asDouble();
    }

    return true;
}



bool ObjectPropertiesCollectorPort::getKinematicOffsets(const string &obj_name, Vector *kinematic_offset)
{
    //if the object property collector port is connected use it to obtain the object 2D position
    if(this->getInputCount()==0)
        return false;

    //ask for the object's id
    Bottle bAsk,bGet,bReply;
    bAsk.addVocab(Vocab::encode("ask"));
    Bottle &bTempAsk=bAsk.addList().addList();
    bTempAsk.addString("name");
    bTempAsk.addString("==");
    bTempAsk.addString(obj_name.c_str());

    this->write(bAsk,bReply);

    if(bReply.size()==0 ||
       bReply.get(0).asVocab()!=Vocab::encode("ack") ||
       bReply.get(1).asList()->check("id") ||
       bReply.get(1).asList()->find("id").asList()->size()==0)
        return false;

    bGet.addVocab(Vocab::encode("get"));
    Bottle &bTempGet=bGet.addList().addList();
    bTempGet.addString("id");
    bTempGet.addInt(bReply.get(1).asList()->find("id").asList()->get(0).asInt());

    this->write(bGet,bReply);

    if(bReply.size()==0 || bReply.get(0).asVocab()!=Vocab::encode("ack"))
        return false;

    if(bReply.get(1).asList()->check("kinematic_offset_left"))
    {
        kinematic_offset[LEFT].resize(3);
        Bottle *bCartesianOffset=bReply.get(1).asList()->find("kinematic_offset_left").asList();
        for(int i=0; i<bCartesianOffset->size(); i++)
            kinematic_offset[LEFT]=bCartesianOffset->get(i).asDouble();
    }

    if(bReply.get(1).asList()->check("kinematic_offset_right"))
    {
        kinematic_offset[RIGHT].resize(3);
        Bottle *bCartesianOffset=bReply.get(1).asList()->find("kinematic_offset_right").asList();
        for(int i=0; i<bCartesianOffset->size(); i++)
            kinematic_offset[RIGHT]=bCartesianOffset->get(i).asDouble();
    }

    return true;
}


bool ObjectPropertiesCollectorPort::setKinematicOffset(const string &obj_name, const Vector *kinematic_offset)
{
    //if the object property collector port is connected use it to obtain the object 2D position
    if(this->getOutputCount()==0)
        return false;

    //ask for the object's id
    Bottle bAsk,bSet,bReply;
    bAsk.addVocab(Vocab::encode("ask"));
    Bottle &bTempAsk=bAsk.addList().addList();
    bTempAsk.addString("name");
    bTempAsk.addString("==");
    bTempAsk.addString(obj_name.c_str());

    this->write(bAsk,bReply);

    if(bReply.size()==0 ||
       bReply.get(0).asVocab()!=Vocab::encode("ack") ||
       bReply.get(1).asList()->check("id") ||
       bReply.get(1).asList()->find("id").asList()->size()==0)
        return false;

    bSet.addVocab(Vocab::encode("set"));
    Bottle &bTempSet=bSet.addList();

    Bottle &bTempSetId=bTempSet.addList();
    bTempSetId.addString("id");
    bTempSetId.addInt(bReply.get(1).asList()->find("id").asList()->get(0).asInt());

    //Kinematic offset left
    Bottle &bTempSetKinematicOffsetLeft=bTempSet.addList();
    bTempSetKinematicOffsetLeft.addString("kinematic_offset_left");
    Bottle &bTempSetVectorLeft=bTempSetKinematicOffsetLeft.addList();
    for(int i=0; i<kinematic_offset[LEFT].size(); i++)
        bTempSetVectorLeft.addDouble(kinematic_offset[LEFT][i]);

    //Kinematic offset right
    Bottle &bTempSetKinematicOffsetRight=bTempSet.addList();
    bTempSetKinematicOffsetRight.addString("kinematic_offset_right");
    Bottle &bTempSetVectorRight=bTempSetKinematicOffsetRight.addList();
    for(int i=0; i<kinematic_offset[RIGHT].size(); i++)
        bTempSetVectorRight.addDouble(kinematic_offset[RIGHT][i]);

    this->write(bSet,bReply);

    return bReply.get(0).asVocab()==Vocab::encode("ack");
}