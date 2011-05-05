//
// Copyright (C) 2005 Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

#include "ANSimMobility.h"
#include "FWMath.h"


Define_Module(ANSimMobility);


static cXMLElement *firstChildWithTag(cXMLElement *node, const char *tagname)
{
    cXMLElement *child = node->getFirstChild();
    while (child && strcmp(child->getTagName(), tagname)!=0)
        child = child->getNextSibling();
    if (!child)
        opp_error("element <%s> has no <%s> child at %s", node->getTagName(), tagname, node->getSourceLocation());
    return child;
}


void ANSimMobility::initialize(int stage)
{
    LineSegmentsMobilityBase::initialize(stage);

    debugEV << "initializing ANSimMobility stage " << stage << endl;

    if (stage == 0)
    {
        nodeId = par("nodeId");
        if (nodeId == -1)
            nodeId = getParentModule()->getIndex();

        // get script: param should point to <simulation> element
        cXMLElement *rootElem = par("ansimTrace");
        if (strcmp(rootElem->getTagName(),"simulation")!=0)
            error("ansimTrace: <simulation> is expected as root element not <%s> at %s",
                  rootElem->getTagName(), rootElem->getSourceLocation());
        nextPosChange = rootElem->getElementByPath("mobility/position_change");
        if (!nextPosChange)
            error("element doesn't have <mobility> child or <position_change> grandchild at %s",
                  rootElem->getSourceLocation());

        // set initial position;
        setTargetPosition();
        move.setStart(targetPos, simTime());
        stepTarget = move.getStartPos();

		targetTime = simTime();

		// dummy value; speed not used in ansim
		move.setSpeed(1);
    }
    else
    {
    	if(!world->use2D()) {
			opp_warning("This mobility module does not yet support 3 dimensional movement."\
						"Movements will probably be incorrect.");
		}
    }
}


void ANSimMobility::setTargetPosition()
{
    // find next <position_update> element with matching <node_id> tag (current one also OK)
    while (nextPosChange)
    {
        const char *nodeIdStr = firstChildWithTag(nextPosChange, "node_id")->getNodeValue();
        if (nodeIdStr && atoi(nodeIdStr)==nodeId)
            break;
        nextPosChange = nextPosChange->getNextSibling();
    }

    if (!nextPosChange)
    {
        move.setSpeed(0);
        return;
    }

    // extract data from it
    extractDataFrom(nextPosChange);

    // skip this one
    nextPosChange = nextPosChange->getNextSibling();
}

void ANSimMobility::extractDataFrom(cXMLElement *node)
{
    // extract data from <position_update> element
    //const char *startTimeStr = firstChildWithTag(node, "start_time")->getNodeValue();
    const char *endTimeStr = firstChildWithTag(node, "end_time")->getNodeValue();
    cXMLElement *destElem = firstChildWithTag(node, "destination");
    const char *xStr = firstChildWithTag(destElem, "xpos")->getNodeValue();
    const char *yStr = firstChildWithTag(destElem, "ypos")->getNodeValue();

    if (!endTimeStr || !xStr || !yStr)
        error("no content in <end_time>, <destination>/<xpos> or <ypos> element at %s", node->getSourceLocation());

    targetTime = atof(endTimeStr);
    targetPos.setX(atof(xStr));
    targetPos.setY(atof(yStr));
}

void ANSimMobility::fixIfHostGetsOutside()
{
    Coord dummy(world->use2D());
    double dum;
    handleIfOutside( RAISEERROR, stepTarget, dummy, dummy, dum );
}

