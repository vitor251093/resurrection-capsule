#ifndef __TRANFORMATION_HISTORY_H
#define __TRANFORMATION_HISTORY_H

#include "RakNetTypes.h"
#include "OgreVector3.h"
#include "OgreQuaternion.h"
#include "DS_Queue.h"
#include "RakMemoryOverride.h"

struct TransformationHistoryCell
{
	TransformationHistoryCell();
	TransformationHistoryCell(RakNetTime t, const Ogre::Vector3& pos, const Ogre::Vector3& vel, const Ogre::Quaternion& quat  );

	RakNetTime time;
	Ogre::Vector3 position;
	Ogre::Quaternion orientation;
	Ogre::Vector3 velocity;
};

class TransformationHistory
{
public:
	void Init(RakNetTime maxWriteInterval, RakNetTime maxHistoryTime);
	void Write(const Ogre::Vector3 &position, const Ogre::Vector3 &velocity, const Ogre::Quaternion &orientation, RakNetTime curTimeMS);
	void Overwrite(const Ogre::Vector3 &position, const Ogre::Vector3 &velocity, const Ogre::Quaternion &orientation, RakNetTime when);
	enum ReadResult
	{
		// We are reading so far in the past there is no data yet
		READ_OLDEST,
		// We are not reading in the past, so the input parameters stay the same
		VALUES_UNCHANGED,
		// We are reading in the past
		INTERPOLATED
	};
	// Parameters are in/out, modified to reflect the history
	ReadResult Read(Ogre::Vector3 *position, Ogre::Vector3 *velocity, Ogre::Quaternion *orientation,
		RakNetTime when, RakNetTime curTime);
	void Clear(void);
protected:
	DataStructures::Queue<TransformationHistoryCell> history;
	unsigned maxHistoryLength;
	RakNetTime writeInterval;
};

#endif
